
//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <oplk/oplk.h>
#include <oplk/debugstr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define CYCLE_LEN           50000
#define NODEID              1                   // could be changed by command param
#define IP_ADDR             0xc0a86401          // 192.168.100.1
#define DEFAULT_GATEWAY     0xC0A864FE          // 192.168.100.C_ADR_RT1_DEF_NODE_ID
#define SUBNET_MASK         0xFFFFFF00          // 255.255.255.0


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
    int                     cycleCounter;
} tCnInstance;

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static tCnInstance cnInstance_l;
static const UINT8  aMacAddr_l[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static tOplkError eventCallback(tOplkApiEventType eventType_p,
                                const tOplkApiEventArg* pEventArg_p,
                                void* pUserArg_p);
static void signalHandler(int sig);
static tOplkError initCn();
static tOplkError setupProcessImages(void);
static void processCnLogic(void);
static void cleanup(void);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Main function for CN

The main function implements the Controlled Node application logic.
*/
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    tOplkError  ret = kErrorOk;

    printf("openPOWERLINK Controlled Node Example Application\n");
    printf("=================================================\n");

    // Initialize instance
    memset(&cnInstance_l, 0, sizeof(cnInstance_l));
    cnInstance_l.fExit = FALSE;
    cnInstance_l.cycleCounter = 0;

    // Register signal handler for clean shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize openPOWERLINK CN stack
    ret = initCn();
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Failed to initialize openPOWERLINK CN: %s\n", 
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

    // Start the CN by sending a reset command
    printf("Starting CN with software reset...\n");
    ret = oplk_execNmtCommand(kNmtEventSwReset);
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Failed to send software reset command: %s\n", 
                debugstr_getRetValStr(ret));
        goto ExitMain;
    }

    printf("CN started successfully. Press Ctrl+C to exit.\n");

    // Main loop
    while (!cnInstance_l.fExit)
    {
        // Process stack events
        ret = oplk_process();
        if (ret != kErrorOk)
        {
            fprintf(stderr, "Error in oplk_process(): %s\n", 
                    debugstr_getRetValStr(ret));
            break;
        }

        // Perform CN-specific logic
        processCnLogic();

        // Sleep briefly to avoid busy-waiting
        usleep(1000); // 1ms
    }

ExitMain:
    cleanup();
    return cnInstance_l.exitCode;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Initialize CN

The function initializes the openPOWERLINK CN stack with appropriate parameters.
*/
//------------------------------------------------------------------------------
static tOplkError initCn()
{
    tOplkError          ret;
    const UINT8         macAddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // Initialize the openPOWERLINK environment
    ret = oplk_initialize();
    if (ret != kErrorOk)
    {
        fprintf(stderr, "oplk_initialize() failed with \"%s\"\n", 
                debugstr_getRetValStr(ret));
        return ret;
    }

    // Configure initialization parameters for CN
    memset(&cnInstance_l.initParam, 0, sizeof(cnInstance_l.initParam));
    cnInstance_l.initParam.sizeOfInitParam = sizeof(tOplkApiInitParam);
    
    // pass selected device name to Edrv
    cnInstance_l.initParam.hwParam.pDevName = "eth0";
    cnInstance_l.initParam.ipAddress = (0xFFFFFF00 & IP_ADDR) | cnInstance_l.initParam.nodeId;
    cnInstance_l.initParam.nodeId = NODEID;
    
    // Set MAC address
    memcpy(cnInstance_l.initParam.aMacAddress, macAddr, sizeof(cnInstance_l.initParam.aMacAddress));
    
    cnInstance_l.initParam.fAsyncOnly              = FALSE;
    cnInstance_l.initParam.featureFlags            = UINT_MAX;
    cnInstance_l.initParam.cycleLen                = CYCLE_LEN;             // required for error detection
    cnInstance_l.initParam.isochrTxMaxPayload      = C_DLL_ISOCHR_MAX_PAYL;  // const
    cnInstance_l.initParam.isochrRxMaxPayload      = C_DLL_ISOCHR_MAX_PAYL;  // const
    cnInstance_l.initParam.presMaxLatency          = 50000;                  // const; only required for IdentRes
    cnInstance_l.initParam.preqActPayloadLimit     = 36;                     // required for initialization (+28 bytes)
    cnInstance_l.initParam.presActPayloadLimit     = 36;                     // required for initialization of Pres frame (+28 bytes)
    cnInstance_l.initParam.asndMaxLatency          = 150000;                 // const; only required for IdentRes
    cnInstance_l.initParam.multiplCylceCnt         = 0;                      // required for error detection
    cnInstance_l.initParam.asyncMtu                = 1500;                   // required to set up max frame size
    cnInstance_l.initParam.prescaler               = 2;                      // required for sync
    cnInstance_l.initParam.lossOfFrameTolerance    = 500000;
    cnInstance_l.initParam.asyncSlotTimeout        = 3000000;
    cnInstance_l.initParam.waitSocPreq             = 1000;
    cnInstance_l.initParam.deviceType              = UINT_MAX;               // NMT_DeviceType_U32
    cnInstance_l.initParam.vendorId                = UINT_MAX;               // NMT_IdentityObject_REC.VendorId_U32
    cnInstance_l.initParam.productCode             = UINT_MAX;               // NMT_IdentityObject_REC.ProductCode_U32
    cnInstance_l.initParam.revisionNumber          = UINT_MAX;               // NMT_IdentityObject_REC.RevisionNo_U32
    cnInstance_l.initParam.serialNumber            = UINT_MAX;               // NMT_IdentityObject_REC.SerialNo_U32
    cnInstance_l.initParam.applicationSwDate       = 0;
    cnInstance_l.initParam.applicationSwTime       = 0;
    cnInstance_l.initParam.subnetMask              = SUBNET_MASK;
    cnInstance_l.initParam.defaultGateway          = DEFAULT_GATEWAY;
    sprintf((char*)cnInstance_l.initParam.sHostname, "%02x-%08x", cnInstance_l.initParam.nodeId, cnInstance_l.initParam.vendorId);
    cnInstance_l.initParam.syncNodeId              = C_ADR_SYNC_ON_SOA;
    cnInstance_l.initParam.fSyncOnPrcNode          = FALSE;
 
    // Set callback function for events
    cnInstance_l.initParam.pfnCbEvent = eventCallback;
    cnInstance_l.initParam.pfnCbSync = NULL;

    ret = oplk_initialize();
    if (ret != kErrorOk)
    {
        fprintf(stderr,
                "oplk_initialize() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret),
                ret);
        printf("oplk_initialize() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret), ret);
        return ret;
    }

    ret = oplk_create(&cnInstance_l.initParam);
    if (ret != kErrorOk)
    {
        fprintf(stderr,
                "oplk_create() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret),
                ret);
        printf("oplk_initialize() failed with \"%s\" (0x%04x)\n",
                debugstr_getRetValStr(ret), ret);
        return ret;
    }

    printf("openPOWERLINK CN (ID: %d) initialized successfully\n", NODEID);
    return kErrorOk;
}

//------------------------------------------------------------------------------
/**
\brief  Setup process images for CN

The function allocates and sets up the input and output process images for CN.
*/
//------------------------------------------------------------------------------
static tOplkError setupProcessImages(void)
{
    tOplkError  ret;
    size_t      inputSize = 64;   // 32 bytes for input image (from MN)
    size_t      outputSize = 64;  // 32 bytes for output image (to MN)

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
    cnInstance_l.inputImage.pImage = oplk_getProcessImageIn();
    cnInstance_l.inputImage.imageSize = inputSize;
    cnInstance_l.outputImage.pImage = oplk_getProcessImageOut();
    cnInstance_l.outputImage.imageSize = outputSize;

    if ((cnInstance_l.inputImage.pImage == NULL) || 
        (cnInstance_l.outputImage.pImage == NULL))
    {
        fprintf(stderr, "Could not get process image pointers!\n");
        return kErrorNoResource;
    }

    return kErrorOk;
}

//------------------------------------------------------------------------------
/**
\brief  Process CN-specific logic

The function implements CN-specific logic that runs periodically.
*/
//------------------------------------------------------------------------------
static void processCnLogic(void)
{
    static int counter = 0;
    UINT8* outputImage = (UINT8*)cnInstance_l.outputImage.pImage;
    UINT8* inputImage = (UINT8*)cnInstance_l.inputImage.pImage;

    // Update counter in output image every 100 cycles (~100ms with 1ms cycle)
    if (++counter % 100 == 0)
    {
        // Write counter value and node ID to output image
        outputImage[0] = (UINT8)(counter / 100);
        outputImage[1] = cnInstance_l.initParam.nodeId;
        
        // Print some debug info
        printf("CN Cycle: %d, Input[0]: %d, Output[0]: %d, Output[1]: %d\n", 
               cnInstance_l.cycleCounter++, inputImage[0], outputImage[0], outputImage[1]);
    }

    // Exchange process images
    oplk_exchangeProcessImageOut(); // Send outputs to MN
    oplk_exchangeProcessImageIn();  // Receive inputs from MN
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

    printf("\nShutting down openPOWERLINK CN...\n");

    // Stop the node by switching it off
    ret = oplk_execNmtCommand(kNmtEventSwitchOff);
    if (ret != kErrorOk)
    {
        fprintf(stderr, "Error stopping node: %s\n", debugstr_getRetValStr(ret));
    }

    // Give some time for graceful shutdown
    usleep(100000); // 100ms

    // Free process images
    if ((cnInstance_l.inputImage.pImage != NULL) || 
        (cnInstance_l.outputImage.pImage != NULL))
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

    printf("openPOWERLINK CN shutdown complete.\n");
}

//------------------------------------------------------------------------------
/**
\brief  Event callback function for CN

The function handles events from the openPOWERLINK stack.
*/
//------------------------------------------------------------------------------
static tOplkError eventCallback(tOplkApiEventType eventType_p,
                                const tOplkApiEventArg* pEventArg_p,
                                void* pUserArg_p)
{
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
                    
                case kNmtCsOperational:
                    printf("NMT State: Operational - CN is fully operational\n");
                    break;
                    
                case kNmtCsPreOperational1:
                case kNmtCsPreOperational2:
                    printf("NMT State: Pre-operational\n");
                    break;
                    
                case kNmtCsNotActive:
                    printf("NMT State: Not Active\n");
                    break;
                    
                default:
                    break;
            }
            break;

        case kOplkApiEventCriticalError:
            fprintf(stderr, "Critical Error Event\n");
            cnInstance_l.fExit = TRUE;
            cnInstance_l.exitCode = -1;
            break;

        case kOplkApiEventWarning:
            printf("Warning Event:\n");
            break;

        case kOplkApiEventNode:
            printf("Node Event: Node 0x%02X state changed to %s\n", 
                   pEventArg_p->nodeEvent.nodeId,
                   debugstr_getNmtStateStr(pEventArg_p->nodeEvent.nmtState));
            break;

        case kOplkApiEventSdo:
            printf("SDO Transfer Finished\n");
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
    printf("\nReceived signal %d, shutting down CN gracefully...\n", sig);
    cnInstance_l.fExit = TRUE;
}