//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <oplk/oplk.h>
#include <oplk/debugstr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <obdcreate/obdcreate.h>
//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define CYCLE_LEN           50000
#define NODEID              0xF0                //=> MN
#define IP_ADDR             0xc0a86401          // 192.168.100.1
#define SUBNET_MASK         0xFFFFFF00          // 255.255.255.0
#define DEFAULT_GATEWAY     0xC0A864FE          // 192.168.100.C_ADR_RT1_DEF_NODE_ID

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------
typedef struct
{
    int                     exitCode;
    volatile BOOL           fExit;
    tOplkApiInitParam       initParam;
    tOplkApiProcessImage    inputImage;
    tOplkApiProcessImage    outputImage;
    int                     cnCount;
    int                     cycleCounter;
} tMnInstance;

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static tMnInstance mnInstance_l;
static BOOL         fGsOff_l;
//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static tOplkError eventCallback(tOplkApiEventType eventType_p,
                                const tOplkApiEventArg* pEventArg_p,
                                void* pUserArg_p);
static void signalHandler(int sig);
static tOplkError initMn(void);
static tOplkError setupProcessImages(void);
static void processMnLogic(void);
static void cleanup(void);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Main function for MN

The main function implements the Managing Node application logic.
*/
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    tOplkError  ret = kErrorOk;

    printf("openPOWERLINK Managing Node Example Application\n");
    printf("==============================================\n");

    // Initialize instance
    memset(&mnInstance_l, 0, sizeof(mnInstance_l));
    mnInstance_l.fExit = FALSE;
    mnInstance_l.cnCount = 0;
    mnInstance_l.cycleCounter = 0;

    // Register signal handler for clean shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize openPOWERLINK MN stack
    ret = initMn();
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Failed to initialize openPOWERLINK MN: %s\n", 
                debugstr_getRetValStr(ret));
        return -1;
    }

    // Setup process images
    ret = setupProcessImages();
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Failed to setup process images: %s\n", 
                debugstr_getRetValStr(ret));
        goto ExitMain;
    }

    // Start the MN by sending a reset command
    printf("Starting MN with software reset...\n");
    ret = oplk_execNmtCommand(kNmtEventSwReset);
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Failed to send software reset command: %s\n", 
                debugstr_getRetValStr(ret));
        goto ExitMain;
    }

    printf("MN started successfully. Waiting for CNs to connect. Press Ctrl+C to exit.\n");

    // Main loop
    while (!mnInstance_l.fExit)
    {
        // Process stack events
        ret = oplk_process();
        if (ret != kErrorOk)
        {
            fprintf(stderr, "Error in oplk_process(): %s\n", 
                    debugstr_getRetValStr(ret));
            break;
        }

        // Perform MN-specific logic
        processMnLogic();

        // Sleep briefly to avoid busy-waiting
        usleep(1000); // 1ms
    }

ExitMain:
    cleanup();
    return mnInstance_l.exitCode;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Initialize MN

The function initializes the openPOWERLINK MN stack with appropriate parameters.
*/
//------------------------------------------------------------------------------
static tOplkError initMn(void)
{
    tOplkError          ret;
    const UINT8         macAddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    printf("Initializing openPOWERLINK stack...\n");
    // Configure initialization parameters for MN
    memset(&mnInstance_l.initParam, 0, sizeof(mnInstance_l.initParam));
    // Set size of init parameter structure
    mnInstance_l.initParam.sizeOfInitParam = sizeof(tOplkApiInitParam);

    mnInstance_l.initParam.hwParam.pDevName = "eth0";
    mnInstance_l.initParam.nodeId = NODEID;
    mnInstance_l.initParam.ipAddress = (0xFFFFFF00 & IP_ADDR) | mnInstance_l.initParam.nodeId;
    
    /* write 00:00:00:00:00:00 to MAC address, so that the driver uses the real hardware address */
    memcpy(mnInstance_l.initParam.aMacAddress, macAddr, sizeof(mnInstance_l.initParam.aMacAddress));
    
    mnInstance_l.initParam.fAsyncOnly              = FALSE;
    mnInstance_l.initParam.featureFlags            = UINT_MAX;
    mnInstance_l.initParam.cycleLen                = CYCLE_LEN;
    mnInstance_l.initParam.isochrTxMaxPayload      = 256;
    mnInstance_l.initParam.isochrRxMaxPayload      = 1490;
    mnInstance_l.initParam.presMaxLatency          = 50000;            // const; only required for IdentRes
    mnInstance_l.initParam.preqActPayloadLimit     = 36;               // required for initialisation (+28 bytes)
    mnInstance_l.initParam.presActPayloadLimit     = 36;               // required for initialisation of Pres frame (+28 bytes)
    mnInstance_l.initParam.asndMaxLatency          = 150000;           // const; only required for IdentRes
    mnInstance_l.initParam.multiplCylceCnt         = 0;                // required for error detection
    mnInstance_l.initParam.asyncMtu                = 1500;             // required to set up max frame size
    mnInstance_l.initParam.prescaler               = 2;                // required for sync
    mnInstance_l.initParam.lossOfFrameTolerance    = 500000;
    mnInstance_l.initParam.asyncSlotTimeout        = 3000000;
    mnInstance_l.initParam.waitSocPreq             = 1000;
    mnInstance_l.initParam.deviceType              = UINT_MAX;         // NMT_DeviceType_U32
    mnInstance_l.initParam.vendorId                = UINT_MAX;         // NMT_IdentityObject_REC.VendorId_U32
    mnInstance_l.initParam.productCode             = UINT_MAX;         // NMT_IdentityObject_REC.ProductCode_U32
    mnInstance_l.initParam.revisionNumber          = UINT_MAX;         // NMT_IdentityObject_REC.RevisionNo_U32
    mnInstance_l.initParam.serialNumber            = UINT_MAX;         // NMT_IdentityObject_REC.SerialNo_U32

    mnInstance_l.initParam.subnetMask              = SUBNET_MASK;
    mnInstance_l.initParam.defaultGateway          = DEFAULT_GATEWAY;
    sprintf((char*)mnInstance_l.initParam.sHostname, "%02x-%08x", 
                mnInstance_l.initParam.nodeId, mnInstance_l.initParam.vendorId);
    mnInstance_l.initParam.syncNodeId              = C_ADR_SYNC_ON_SOA;
    mnInstance_l.initParam.fSyncOnPrcNode          = FALSE;
    
    // Set callback function for events
    mnInstance_l.initParam.pfnCbEvent = eventCallback;
    mnInstance_l.initParam.pfnCbSync  = NULL;

     // Initialize object dictionary
    ret = obdcreate_initObd(&mnInstance_l.initParam.obdInitParam);
    if (ret != kErrorOk)
    {
        printf("obdcreate_initObd() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret), ret);
        return ret;
    }

    // initialize POWERLINK stack
    ret = oplk_initialize();
    if (ret != kErrorOk)
    {
        fprintf(stderr,
                "oplk_initialize() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret),
                ret);
        printf("oplk_init() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret),
                ret);
        return ret;
    }


    // Initialize the stack with our parameters
    ret = oplk_create(&mnInstance_l.initParam);
    if (ret != kErrorOk)
    {
        fprintf(stderr, "oplk_create() failed with \"%s\"\n", 
                debugstr_getRetValStr(ret));
        oplk_exit();
        return ret;
    }

    printf("openPOWERLINK MN initialized successfully\n");
    return kErrorOk;
}

//------------------------------------------------------------------------------
/**
\brief  Setup process images for MN

The function allocates and sets up the input and output process images for MN.
*/
//------------------------------------------------------------------------------
static tOplkError setupProcessImages(void)
{
    tOplkError  ret = kErrorOk;
    size_t      inputSize = 64;   // 64 bytes for input image (from CNs)
    size_t      outputSize = 64;  // 64 bytes for output image (to CNs)

    printf("Initializing process image...\n");
    // Allocate process images
    ret = oplk_allocProcessImage(inputSize, outputSize);
    if (ret != kErrorOk)
    {
        fprintf(stderr, "oplk_allocProcessImage() failed with \"%s\"\n", 
                debugstr_getRetValStr(ret));
        return ret;
    }

    printf("Process images allocated: Input=%zu bytes, Output=%zu bytes\n", 
           inputSize, outputSize);

    // Get pointers to process images
    mnInstance_l.inputImage.pImage = oplk_getProcessImageIn();
    mnInstance_l.inputImage.imageSize = inputSize;
    mnInstance_l.outputImage.pImage = oplk_getProcessImageOut();
    mnInstance_l.outputImage.imageSize = outputSize;

    if ((mnInstance_l.inputImage.pImage == NULL) || 
        (mnInstance_l.outputImage.pImage == NULL))
    {
        fprintf(stderr, "Could not get process image pointers!\n");
        return kErrorNoResource;
    }

    return kErrorOk;
}

//------------------------------------------------------------------------------
/**
\brief  Process MN-specific logic

The function implements MN-specific logic that runs periodically.
*/
//------------------------------------------------------------------------------
static void processMnLogic(void)
{
    tOplkError  ret;

    ret = oplk_waitSyncEvent(100000);
    if (ret != kErrorOk)
        return;

    static int counter = 0;
    UINT8* outputImage = (UINT8*)mnInstance_l.outputImage.pImage;
    UINT8* inputImage = (UINT8*)mnInstance_l.inputImage.pImage;

    // Update counter in output image every 100 cycles (~100ms with 1ms cycle)
    if (++counter % 100 == 0)
    {
        // Write counter value to first byte of output image
        outputImage[0] = (UINT8)(counter / 100);
        
        // Print some debug info
        printf("MN Cycle: %d, CN Count: %d, Input[0]: %d, Output[0]: %d\n", 
               mnInstance_l.cycleCounter++, mnInstance_l.cnCount, inputImage[0], outputImage[0]);
    }

    // Exchange process images
    oplk_exchangeProcessImageOut(); // Send outputs to network
    oplk_exchangeProcessImageIn();  // Receive inputs from network
}

//------------------------------------------------------------------------------
/**
\brief  Cleanup function

The function performs cleanup operations before exiting.
*/
//------------------------------------------------------------------------------
static void cleanup(void)
{
    tOplkError  ret;

    printf("\nShutting down openPOWERLINK MN...\n");

    // Stop the node by switching it off
    ret = oplk_execNmtCommand(kNmtEventSwitchOff);
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Error stopping node: %s\n", debugstr_getRetValStr(ret));
    }

    // Give some time for graceful shutdown
    usleep(100000); // 100ms

    // Free process images
    if ((mnInstance_l.inputImage.pImage != NULL) || 
        (mnInstance_l.outputImage.pImage != NULL))
    {
        ret = oplk_freeProcessImage();
        if (ret != kErrorOk)
        {
            fprintf(stderr, "Error freeing process images: %s\n", 
                    debugstr_getRetValStr(ret));
        }
    }

    // Shutdown the stack
    ret = oplk_destroy();
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Error destroying stack: %s\n", debugstr_getRetValStr(ret));
    }

    // Exit the openPOWERLINK environment
    oplk_exit();

    printf("openPOWERLINK MN shutdown complete.\n");
}

//------------------------------------------------------------------------------
/**
\brief  Event callback function for MN

The function handles events from the openPOWERLINK stack.
*/
//------------------------------------------------------------------------------
static tOplkError eventCallback(tOplkApiEventType eventType_p,
                                const tOplkApiEventArg* pEventArg_p,
                                void* pUserArg_p)
{
    tOplkError  ret = kErrorOk;
    switch (eventType_p)
    {
        case kOplkApiEventNmtStateChange:
            printf("NMT State Changed: %s\n", 
                   debugstr_getNmtStateStr(pEventArg_p->nmtStateChange.newNmtState));
            
            // Handle special states
            switch (pEventArg_p->nmtStateChange.newNmtState)
            {
                case kNmtGsOff:
                    printf("NMT State: Off\n");
                    break;
                    
                case kNmtGsInitialising:
                    printf("NMT State: Initializing\n");
                    break;
                    
                case kNmtMsOperational:
                    printf("NMT State: Operational - MN is fully operational\n");
                    break;
                    
                case kNmtMsPreOperational1:
                case kNmtMsPreOperational2:
                    printf("NMT State: Pre-operational\n");
                    break;
                    
                case kNmtMsNotActive:
                    printf("NMT State: Not Active\n");
                    break;
                    
                default:
                    break;
            }
            break;

        case kOplkApiEventCriticalError:
            fprintf(stderr, "Critical Error Event\n");
            mnInstance_l.fExit = TRUE;
            mnInstance_l.exitCode = -1;
            break;

        case kOplkApiEventWarning:
            printf("Warning Event\n");
            break;

        case kOplkApiEventNode:
            printf("Node Event: Node 0x%02X state changed to %s\n", 
                   pEventArg_p->nodeEvent.nodeId,
                   debugstr_getNmtStateStr(pEventArg_p->nodeEvent.nmtState));
                   
            // Track connected CNs
            if (pEventArg_p->nodeEvent.nmtState == kNmtCsOperational)
            {
                mnInstance_l.cnCount++;
                printf("CN 0x%02X is now operational. Total CNs: %d\n", 
                       pEventArg_p->nodeEvent.nodeId, mnInstance_l.cnCount);
            }
            break;

        case kOplkApiEventSdo:
            printf("SDO Transfer Finished: Result\n");
            break;

        default:
            printf("Other Event: 0x%02X\n", eventType_p);
            break;
    }

    return kErrorOk;
}

//------------------------------------------------------------------------------
/**
\brief  Signal handler function

The function handles signals to allow for graceful shutdown.
*/
//------------------------------------------------------------------------------
static void signalHandler(int sig)
{
    printf("\nReceived signal %d, shutting down MN gracefully...\n", sig);
    mnInstance_l.fExit = TRUE;
}