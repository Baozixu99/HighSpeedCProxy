#ifndef HYPERAMP_SHM_QUEUE_H_
#define HYPERAMP_SHM_QUEUE_H_
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
/**
 * @file hyperamp_shm_queue.h
 * @brief HyperAMP Shared Memory (SHM) Queue - Fully Compatible with HighSpeedCProxy Interfaces
 * 
 * This file implements a shared memory queue data structure that is fully compatible with the interface
 * defined in HighSpeedCProxy/include/shared_mem_io.h, and additionally integrates a software spinlock 
 * tailored for uncached memory.
 * 
 * Design Objectives:
 * 1. The structural layout is completely consistent with SharedMemoryPoolQueue of HighSpeedCProxy
 * 2. Supports uncached memory on ARM platforms such as Phytium Pi (Feiteng Pai) (atomic instructions are not used)
 * 3. Provides secure support for multi-process/multi-core access
 */

/* ==================== Constant Definitions ==================== */

#define HYPERAMP_ERROR_ADDR             UINT64_MAX
#define HYPERAMP_MAX_MAP_TABLE_ENTRIES  125  /* Optimized: Queue control area exactly 4KB (1 page) */

/* Queue Operation Results */
#define HYPERAMP_OK                     0
#define HYPERAMP_ERROR                  (-1)
#define HYPERAMP_AGAIN                  1

/* Memory Mapping Mode - Must match HighSpeedCProxy */
typedef enum {
    HYPERAMP_MAP_MODE_CONTIGUOUS_BOTH = 0,              // Physical address contiguous, logical address contiguous
    HYPERAMP_MAP_MODE_CONTIGUOUS_PHYS_DISCRETE_LOGICAL  // Physical address contiguous, logical address discontiguous
} HyperampMapMode;

/* Message Constants - Must match HighSpeedCProxy/message.h */
#define HYPERAMP_MSG_HDR_SIZE           8
#define HYPERAMP_MSG_MIN_SIZE           1
#define HYPERAMP_MSG_MAX_SIZE           4088
#define HYPERAMP_MSG_HDR_PLUS_MAX_SIZE  (HYPERAMP_MSG_HDR_SIZE + HYPERAMP_MSG_MAX_SIZE)

/* ==================== Software Spinlock (Works with uncached memory) ==================== */

/**
 * @brief Software Spinlock Structure
 * 
 * Implements spinlock using volatile variables and memory barriers (no atomic instructions like LDXR/STXR).
 * Safe for use on uncached memory (e.g., FeiTeng Pi platform).
 */
typedef struct {
    volatile uint32_t lock_value;     // 0 = unlocked, 1 = locked
    volatile uint32_t owner_zone_id;  // Zone ID holding the lock (for debugging)
    volatile uint32_t lock_count;     // Lock acquisition count
    volatile uint32_t contention_count; // Lock contention count
} __attribute__((packed)) HyperampSpinlock;


/* ==================== Address Mapping Table Entry ==================== */

/**
 * @brief Address Mapping Table Entry
 * 
 * Matches the MapTableEntry structure in HighSpeedCProxy for cross-system compatibility.
 */
typedef struct {
    uint64_t virt_addr;   ///< Virtual address
    uint64_t phy_addr;    ///< Physical address
} __attribute__((packed)) HyperampMapTableEntry;

/* ==================== Shared Memory Pool Queue ==================== */

/**
 * @brief Shared Memory Pool Queue Structure
 * 
 * Memory layout is fully compatible with HighSpeedCProxy to ensure cross-system interoperability.
 * The structure is divided into two parts:
 * - **Compatibility Section**: Must maintain identical field offsets as HighSpeedCProxy
 * - **Extension Section**: Added at the end to preserve compatibility
 */
typedef struct {
    /* === HighSpeedCProxy Compatibility Section (Field offsets must match exactly) === */
    
    uint8_t  map_mode1;    ///< Memory mapping mode for Linux side
    uint8_t  map_mode2;    ///< Memory mapping mode for microkernel side
    uint16_t header;       ///< Ring buffer head index (next dequeue position)
    uint16_t tail;         ///< Ring buffer tail index (next enqueue position)
    uint16_t capacity;     ///< Maximum number of elements in the queue
    uint16_t block_size;   ///< Size of each memory block (bytes)
    uint16_t _reserved;    ///< Reserved for alignment
    
    uint64_t phy_addr;     ///< Physical address of the queue control block
    uint64_t virt_addr1;   ///< Virtual address on Linux side
    uint64_t virt_addr2;   ///< Virtual address on microkernel side
    
    HyperampMapTableEntry table1[HYPERAMP_MAX_MAP_TABLE_ENTRIES];  ///< Linux-side address mapping table
    HyperampMapTableEntry table2[HYPERAMP_MAX_MAP_TABLE_ENTRIES];  ///< Microkernel-side address mapping table
    
    /* === HyperAMP Extension Section (Placed at end to ensure compatibility) === */
    
    HyperampSpinlock queue_lock;  ///< Spinlock for queue operations
    
    uint32_t magic;               ///< Magic number for initialization verification
    uint32_t version;             ///< Version number
    uint32_t enqueue_count;       ///< Total enqueue operations
    uint32_t dequeue_count;       ///< Total dequeue operations
    
} __attribute__((packed)) HyperampShmQueue;

/* Magic Number Definition */
#define HYPERAMP_QUEUE_MAGIC        0x48415150  // "HAQP" - HyperAmp Queue Protocol

/* ==================== Message Header Structure ==================== */

/**
 * @brief Proxy Message Header - Matches HighSpeedCProxy's ProxyMsgHeader (8 bytes)
 */
typedef struct {
    uint8_t  version;           ///< Protocol version
    uint8_t  proxy_msg_type;    ///< Message type: 0=device, 1=policy, 2=session, 3=data
    uint16_t frontend_sess_id;  ///< Frontend session ID
    uint16_t backend_sess_id;   ///< Backend session ID
    uint16_t payload_len;       ///< Payload length
} __attribute__((packed)) HyperampMsgHeader;

/* Message Type Definitions - Must match HighSpeedCProxy */
typedef enum {
    HYPERAMP_MSG_TYPE_DEV = 0,     // Device message
    HYPERAMP_MSG_TYPE_STRGY = 1,   // Policy message
    HYPERAMP_MSG_TYPE_SESS = 2,    // Session establishment
    HYPERAMP_MSG_TYPE_DATA = 3,    // Data transfer
    HYPERAMP_MSG_TYPE_SERVICE = 0x10, // Service call: frontend_sess_id = service_id
    HYPERAMP_MSG_TYPE_BULK = 0x20  // Bulk transfer (Payload contains descriptor)
} HyperampMsgType;

// Bulk Transfer Configuration
#define BULK_BUFFER_OFFSET        0x100000 // 1MB offset
#define BULK_BUFFER_SIZE          (2 * 1024 * 1024) // 2MB size

// Bulk Transfer Descriptor (transmitted as payload)
typedef struct {
    uint32_t offset;      ///< Data offset
    uint32_t length;      ///< Data length
    uint32_t service_id;  ///< Service ID (1=Encrypt, 2=Decrypt, 4=VerifyEncrypt, 5=VerifyDecrypt)
    uint32_t status;      ///< 0=Request, 1=Success, <0=Error
} HyperampBulkDescriptor;

// ==================== Signature Verification ====================

// Service IDs
#define SERVICE_ECHO              0
#define SERVICE_ENCRYPT           1
#define SERVICE_DECRYPT           2
#define SERVICE_VERIFY_ONLY       3   // Signature verification only
#define SERVICE_VERIFY_ENCRYPT    4   // Signature verification followed by encryption
#define SERVICE_VERIFY_DECRYPT    5   // Signature verification followed by decryption
#define SERVICE_VALIDATE_ENCRYPT  7   // Field validation followed by encryption (targeted data)
#define SERVICE_VALIDATE_DECRYPT  8   // Field validation followed by decryption (targeted data)

// Field Validation Status Codes
#define VALIDATE_OK               0
#define VALIDATE_FAILED_MISSING  -10  // Required field missing

// Signature Verification Status Codes
#define AUTH_OK                   0
#define AUTH_FAILED_BAD_MAGIC    -1  // Invalid signature magic
#define AUTH_FAILED_BAD_SIG      -2  // Signature mismatch
#define AUTH_FAILED_BAD_LEN      -3  // Length verification failed

// Signature Header Magic Number
#define SIG_MAGIC                 0x53494731  // "SIG1"

// Simplified Signature Header
typedef struct {
    uint32_t magic;           ///< Must be SIG_MAGIC (0x53494731)
    uint16_t sig_len;         ///< Signature length (ECDSA: 70-72 bytes)
    uint16_t reserved;
    uint32_t payload_len;     ///< Original data length
    uint8_t  signature[72];   ///< ECDSA-P256 signature (max 72 bytes)
} __attribute__((packed)) HyperampSignedHeader;

/* ==================== Queue Configuration Structure ==================== */

/**
 * @brief Queue Initialization Configuration - Compatible with HighSpeedCProxy's SharedMemoryPoolQueueConfig
 */
typedef struct {
    uint16_t map_mode;      ///< Memory mapping mode
    uint16_t capacity;      ///< Queue capacity
    uint16_t block_size;    ///< Block size
    uint16_t _reserved;
    uint64_t phy_addr;      ///< Physical address
    uint64_t virt_addr;     ///< Virtual address
} HyperampQueueConfig;

/* ==================== Queue Operation Macros ==================== */

/**
 * @brief Calculate total message size
 */
#define HYPERAMP_MSG_TOTAL_SIZE(hdr) \
    (sizeof(HyperampMsgHeader) + (hdr)->payload_len)

/**
 * @brief Get virtual address of header pointer element
 * Note: Data region starts at (header+1)*block_size offset
 * First data block is at block_size offset when header=0
 */
#define HYPERAMP_QUEUE_HEADER_VIRT_ADDR(queue, virt_base) \
    ((uint64_t)(virt_base) + (uint64_t)((queue)->header + 1) * (uint64_t)(queue)->block_size)

/**
 * @brief Get virtual address of tail pointer element
 */
#define HYPERAMP_QUEUE_TAIL_VIRT_ADDR(queue, virt_base) \
    ((uint64_t)(virt_base) + (uint64_t)((queue)->tail + 1) * (uint64_t)(queue)->block_size)

/**
 * @brief Check if queue is empty
 */
#define HYPERAMP_QUEUE_IS_EMPTY(queue) \
    ((queue)->tail == (queue)->header)

/**
 * @brief Check if queue is full
 */
#define HYPERAMP_QUEUE_IS_FULL(queue) \
    ((((queue)->header + 1) % (queue)->capacity) == (queue)->tail)

/**
 * @brief Calculate number of elements in queue
 */
#define HYPERAMP_QUEUE_LENGTH(queue) \
    (((queue)->header >= (queue)->tail) ? \
     ((queue)->header - (queue)->tail) : \
     ((queue)->capacity - (queue)->tail + (queue)->header))


inline void hyperamp_spinlock_init(volatile HyperampSpinlock *lock);

inline void hyperamp_spinlock_lock(volatile HyperampSpinlock *lock, uint32_t zone_id);

inline void hyperamp_spinlock_unlock(volatile HyperampSpinlock *lock);

inline int hyperamp_spinlock_trylock(volatile HyperampSpinlock *lock, uint32_t zone_id);

inline int hyperamp_queue_init(volatile HyperampShmQueue *queue, 
                                       const HyperampQueueConfig *config,
                                       int is_creator);

inline int hyperamp_queue_is_initialized(volatile HyperampShmQueue *queue);

inline int hyperamp_queue_enqueue(volatile HyperampShmQueue *queue,
                                          uint32_t zone_id,
                                          const void *data,
                                          size_t data_len,
                                          volatile void *virt_base);

inline int hyperamp_queue_dequeue(volatile HyperampShmQueue *queue,
                                  uint32_t zone_id,
                                  void *data,
                                  size_t max_len,
                                  size_t *actual_len,
                                  volatile void *virt_base);

inline int hyperamp_queue_peek(volatile HyperampShmQueue *queue,
                               uint32_t zone_id,
                               void *data,
                               size_t max_len,
                               size_t *actual_len,
                               volatile void *virt_base);

inline int hyperamp_queue_alloc_slot(volatile HyperampShmQueue *queue,
                                     uint32_t zone_id,
                                     uint64_t *slot_addr,
                                     volatile void *virt_base);

inline int hyperamp_queue_release_slot(volatile HyperampShmQueue *queue,
                                       uint32_t zone_id);



#endif