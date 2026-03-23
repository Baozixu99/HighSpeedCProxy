#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include "hyperamp_shm_queue.h"
#include "common_utils.h"
#include "message.h"

/**
 * @defgroup Memory_Barriers Memory Barrier and Cache Management
 * @{
 */

/**
 * @name Memory Barrier Macros
 * @{
 * 
 * These macros implement architecture-specific memory barriers to enforce ordering
 * constraints between memory operations. They are essential for ensuring visibility
 * of shared memory updates across CPUs or between CPU and device drivers.
 */
#if defined(__aarch64__) || defined(__arm__)
    /**
     * @def HYPERAMP_DMB()
     * Data Memory Barrier: Ensures all memory accesses before this instruction
     * complete before any subsequent memory accesses.
     */
    #define HYPERAMP_DMB()   __asm__ volatile("dmb sy" ::: "memory")

    /**
     * @def HYPERAMP_ISB()
     * Instruction Synchronization Barrier: Flushes the instruction pipeline,
     * ensuring subsequent instructions are fetched after this barrier.
     */
    #define HYPERAMP_ISB()   __asm__ volatile("isb" ::: "memory")

    /**
     * @brief Data Cache Clean Operation
     * 
     * Flushes cache lines to main memory for the specified address range.
     * This ensures data written to shared memory is visible to other cores
     * or devices.
     * 
     * @param[in] addr  Start address of the memory region to flush
     * @param[in] size  Size of the memory region in bytes
     * 
     * @note  Currently simplified to a DMB barrier only (original cache cleaning code commented out)
     */
    inline void hyperamp_cache_clean(volatile void *addr, size_t size) {
        (void)addr; (void)size;
        __asm__ volatile("dmb sy" ::: "memory");
    }

    /**
     * @brief Data Cache Invalidate Operation
     * 
     * Discards cached data and forces reads from main memory for the
     * specified address range. Ensures subsequent accesses see the latest
     * updates from other cores or devices.
     * 
     * @param[in] addr  Start address of the memory region to invalidate
     * @param[in] size  Size of the memory region in bytes
     * 
     * @note  Currently simplified to a DMB barrier only (original cache invalidation code commented out)
     */
    inline void hyperamp_cache_invalidate(volatile void *addr, size_t size) {
        (void)addr; (void)size;
        __asm__ volatile("dmb sy" ::: "memory");
    }
#else
    // x86 fallback (currently commented out)
    #define HYPERAMP_DMB()   __asm__ volatile("mfence" ::: "memory")
    #define HYPERAMP_ISB()   __asm__ volatile("" ::: "memory")
    
    static inline void hyperamp_cache_clean(volatile void *addr, size_t size) {
        (void)addr; (void)size;
        __asm__ volatile("mfence" ::: "memory");
    }
    
    static inline void hyperamp_cache_invalidate(volatile void *addr, size_t size) {
        (void)addr; (void)size;
        __asm__ volatile("mfence" ::: "memory");
    }
#endif

/**
 * @def HYPERAMP_BARRIER()
 * Combined memory barrier combining both DMB and DSB semantics.
 * Ensures full ordering of memory operations before and after this macro.
 */
/* HYPERAMP_BARRIER 仅使用 DMB：DSB 已由 LDXR/STLXR 内置语义取代 */
#define HYPERAMP_BARRIER()   HYPERAMP_DMB()
/** @} */  // End of Memory_Barriers group


/**
 * @brief Initializes a HyperampSpinlock structure to its default unlocked state.
 *
 * @details This function sets up the spinlock for use by:
 *          - Resetting the lock value to 0 (unlocked)
 *          - Clearing the owner zone ID (setting to invalid value)
 *          - Resetting lock acquisition counter
 *          - Resetting contention counter
 *          
 *          Must be called before any other spinlock operations are used on the target lock.
 *
 * @param[in] lock  Pointer to the spinlock structure to initialize.
 *                  Must point to valid memory that remains allocated for the lock's lifetime.
 *
 * @note    This function is not thread-safe and should only be called during initialization
 *          phase before concurrent access to the lock occurs.
 * @note    The lock structure must be allocated in memory that is accessible to all threads
 *          that will use the spinlock.
 */
void hyperamp_spinlock_init(volatile HyperampSpinlock *lock)
{
    if (!lock) return;
    
    lock->lock_value       = 0;
    lock->owner_zone_id    = 0;
    lock->lock_count       = 0;
    lock->contention_count = 0;
    HYPERAMP_BARRIER();

}

/**
 * @brief 获取自旋锁
 *
 * ARM64: 使用 LDAXR (Load-Acquire Exclusive) + STLXR (Store-Release Exclusive)。
 * 要求 lock_value 所在内存为 NORMAL 类型 (WB)；DEVICE_nGnRnE 内存不支持独占访问指令。
 * */
void hyperamp_spinlock_lock(volatile HyperampSpinlock *lock, uint32_t zone_id)
{
    if (!lock) return;
#if defined(__aarch64__)
    uint32_t tmp, newval;
    __asm__ volatile(
        "1: ldaxr   %w0, %2\n"       /* Load-Acquire Exclusive: tmp = lock_value  */
        "   cbnz    %w0, 1b\n"       /* 已被占用则自旋等待                        */
        "   mov     %w1, #1\n"
        "   stlxr   %w0, %w1, %2\n" /* Store-Release Exclusive: lock_value = 1  */
        "   cbnz    %w0, 1b\n"       /* 独占写失败则重试                          */
        : "=&r" (tmp), "=&r" (newval), "+Q" (lock->lock_value)
        :
        : "memory"
    );
    /* LDAXR/STLXR 已提供 acquire 语义，无需额外屏障 */
    lock->owner_zone_id = zone_id;
    lock->lock_count++;
#else
    /* 非 ARM64 保留软件实现 */
    while (1) {
        HYPERAMP_BARRIER();
        if (lock->lock_value == 0) {
            // Attempt to acquire the lock
            lock->lock_value = 1;
            HYPERAMP_BARRIER();
            if (lock->lock_value == 1) {
                lock->owner_zone_id = zone_id;
                lock->lock_count++;
                return;  // Successfully acquired the lock
            }
        }
        
        // Lock is held, spin-wait with backoff strategy
        lock->contention_count++;
        __asm__ volatile("pause" ::: "memory");
    }
#endif
}


/**
 * @brief Releases a spinlock by marking it as available.
 * 
 * @details This function sets the lock to an unlocked state (0) and resets the owner zone ID.
 *          It ensures cache coherence across multiple cores/virtual machines by:
 *          - Using memory barriers to enforce write ordering
 *          - Performing cache clean to flush the lock state to main memory
 * 
 * @note This function assumes the lock is held by the caller (no ownership verification).
 * @note Must be called after all critical section operations are complete.
 * @note Cache maintenance is essential for shared memory visibility in multi-core systems.
 * 
 * @param[in] lock  Pointer to the spinlock structure (must be valid and held by the caller)
 */
void hyperamp_spinlock_unlock(volatile HyperampSpinlock *lock)
{
    if (!lock) return;
    lock->owner_zone_id = 0;
#if defined(__aarch64__)
    __asm__ volatile(
        "stlr    wzr, %0\n"  /* Store-Release: lock_value = 0，保证 release 语义 */
        : "+Q" (lock->lock_value)
        :
        : "memory"
    );
#else
    HYPERAMP_BARRIER();
    lock->lock_value = 0;
    HYPERAMP_BARRIER();
#endif
}


/**
 * @brief Attempts to acquire a spinlock in non-blocking mode
 * 
 * @details This function tries to acquire the spinlock without blocking.
 *          Returns immediately if the lock is already held by another thread.
 * 
 * @param[in]  lock     Pointer to the spinlock structure
 * @param[in]  zone_id  Zone identifier of the current thread (for debugging)
 * 
 * @return HYPERAMP_OK if the lock was successfully acquired, 
 *         HYPERAMP_ERROR if the lock is already held
 * 
 * @note This function assumes the lock is properly initialized
 * @note The lock structure must be in shared memory accessible to all threads
 * @note No cache maintenance operations are performed in this function
 */
int hyperamp_spinlock_trylock(volatile HyperampSpinlock *lock, uint32_t zone_id)
{
    if (!lock) return HYPERAMP_ERROR;
#if defined(__aarch64__)
    uint32_t tmp, newval;
    __asm__ volatile(
        "ldaxr   %w0, %2\n"       /* Load-Acquire Exclusive: tmp = lock_value */
        "cbnz    %w0, 1f\n"       /* 已被占用，直接跳到结束 (tmp != 0)        */
        "mov     %w1, #1\n"
        "stlxr   %w0, %w1, %2\n" /* Store-Release Exclusive; tmp=0 成功       */
        "1:\n"
        : "=&r" (tmp), "=&r" (newval), "+Q" (lock->lock_value)
        :
        : "memory"
    );
    if (tmp == 0) {
        lock->owner_zone_id = zone_id;
        lock->lock_count++;
        return HYPERAMP_OK;
    }
    return HYPERAMP_AGAIN;
#else
    HYPERAMP_BARRIER();
    if (lock->lock_value == 0) {
        lock->lock_value = 1;
        HYPERAMP_BARRIER();
        if (lock->lock_value == 1) {
            lock->owner_zone_id = zone_id;
            lock->lock_count++;
            return HYPERAMP_OK;
        }
    }
    return HYPERAMP_AGAIN;
#endif
}

/* 静态断言：验证 queue_lock.lock_value 在共享内存中满足 LDAXR/STXR 所需的 4 字节对齐 */
_Static_assert(offsetof(HyperampShmQueue, queue_lock) % 4 == 0,
               "HyperampShmQueue.queue_lock must be 4-byte aligned for LDAXR/STXR");
_Static_assert(sizeof(HyperampSpinlock) == 16,
               "HyperampSpinlock size must be 16 bytes");

/* 安全获取 packed 结构体中 queue_lock 的指针（避免 -Waddress-of-packed-member） */
#define HYPERAMP_QUEUE_LOCK(q) \
    ((volatile HyperampSpinlock *)((volatile uint8_t *)(q) + offsetof(HyperampShmQueue, queue_lock)))


/* ==================== Secure Memory Operations ==================== */

/**
 * @brief Secure memset for uncached memory (byte-by-byte access)
 * 
 * @details This function performs a byte-by-byte memset operation on uncached memory regions.
 *          It ensures proper memory barrier after the operation to guarantee visibility across
 *          all cores and virtual machines in a multi-core environment.
 * 
 * @note Must be used for uncached memory regions (e.g., MMIO, shared memory)
 * @note The operation is performed byte-by-byte to avoid cache coherence issues
 * @note Memory barrier (HYPERAMP_BARRIER) is applied after the operation
 * 
 * @param[in] dst   Destination address (uncached memory)
 * @param[in] val   Value to set (8-bit)
 * @param[in] len   Length of memory to set (in bytes)
 */
void hyperamp_safe_memset(volatile void *dst, uint8_t val, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)dst;
    for (size_t i = 0; i < len; i++) {
        p[i] = val;
    }
    HYPERAMP_BARRIER();
}

/**
 * @brief Secure memcpy for uncached memory (byte-by-byte access)
 * 
 * @details This function performs a byte-by-byte memcpy operation on uncached memory regions.
 *          It ensures proper memory barrier after the operation to guarantee visibility across
 *          all cores and virtual machines in a multi-core environment.
 * 
 * @note Must be used for uncached memory regions (e.g., MMIO, shared memory)
 * @note The operation is performed byte-by-byte to avoid cache coherence issues
 * @note Memory barrier (HYPERAMP_BARRIER) is applied after the operation
 * 
 * @param[in] dst   Destination address (uncached memory)
 * @param[in] src   Source address (uncached memory)
 * @param[in] len   Length of memory to copy (in bytes)
 */
void hyperamp_safe_memcpy(volatile void *dst, const volatile void *src, size_t len)
{
    volatile uint8_t *d = (volatile uint8_t *)dst;
    const volatile uint8_t *s = (const volatile uint8_t *)src;
    for (size_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
    HYPERAMP_BARRIER();
}

/**
 * @brief Secure 16-bit read from uncached memory (byte-by-byte)
 * 
 * @details This function reads a 16-bit value from uncached memory using byte-by-byte access.
 *          It ensures proper memory barrier before the operation to guarantee visibility of
 *          the latest data across all cores and virtual machines.
 * 
 * @note Must be used for uncached memory regions (e.g., MMIO, shared memory)
 * @note The operation is performed byte-by-byte to avoid cache coherence issues
 * @note Memory barrier (HYPERAMP_BARRIER) is applied before the read operation
 * 
 * @param[in] addr  Memory address (uncached)
 * @param[in] offset Offset within the memory region
 * 
 * @return 16-bit value read from memory
 */
uint16_t hyperamp_safe_read_u16(const volatile void *addr, size_t offset)
{
    const volatile uint8_t *p = (const volatile uint8_t *)addr;
    uint16_t val = 0;
    for (int i = 0; i < 2; i++) {
        val |= ((uint16_t)p[offset + i]) << (i * 8);
    }
    HYPERAMP_BARRIER();
    return val;
}

/**
 * @brief Secure 32-bit read from uncached memory (byte-by-byte)
 * 
 * @details This function reads a 32-bit value from uncached memory using byte-by-byte access.
 *          It ensures proper memory barrier before the operation to guarantee visibility of
 *          the latest data across all cores and virtual machines.
 * 
 * @note Must be used for uncached memory regions (e.g., MMIO, shared memory)
 * @note The operation is performed byte-by-byte to avoid cache coherence issues
 * @note Memory barrier (HYPERAMP_BARRIER) is applied before the read operation
 * 
 * @param[in] addr  Memory address (uncached)
 * @param[in] offset Offset within the memory region
 * 
 * @return 32-bit value read from memory
 */
uint32_t hyperamp_safe_read_u32(const volatile void *addr, size_t offset)
{
    const volatile uint8_t *p = (const volatile uint8_t *)addr;
    uint32_t val = 0;
    for (int i = 0; i < 4; i++) {
        val |= ((uint32_t)p[offset + i]) << (i * 8);
    }
    HYPERAMP_BARRIER();
    return val;
}


/**
 * @brief Secure 64-bit read from uncached memory (byte-by-byte)
 * 
 * @details This function reads a 64-bit value from uncached memory using byte-by-byte access.
 *          It ensures proper memory barrier before the operation to guarantee visibility of
 *          the latest data across all cores and virtual machines.
 * 
 * @note Must be used for uncached memory regions (e.g., MMIO, shared memory)
 * @note The operation is performed byte-by-byte to avoid cache coherence issues
 * @note Memory barrier (HYPERAMP_BARRIER) is applied before the read operation
 * 
 * @param[in] addr  Memory address (uncached)
 * @param[in] offset Offset within the memory region
 * 
 * @return 64-bit value read from memory
 */
inline uint64_t hyperamp_safe_read_u64(const volatile void *addr, size_t offset)
{
    const volatile uint8_t *p = (const volatile uint8_t *)addr;
    uint64_t val = 0;
    for (int i = 0; i < 8; i++) {
        val |= ((uint64_t)p[offset + i]) << (i * 8);
    }
    HYPERAMP_BARRIER();
    return val;
}


/* ==================== Queue Operation Functions ==================== */

/**
 * @brief Initializes a shared memory queue (with creator-specific initialization)
 * 
 * @details This function initializes a shared memory queue structure. The creator (is_creator=1) 
 *         performs full initialization of all fields using byte-by-byte writes to ensure cache coherence.
 *         Non-creators (is_creator=0) only set their own virtual address.
 * 
 * @note Must be called by the queue creator (is_creator=1) during first-time initialization
 * @note The queue structure must reside in shared memory accessible to all cores
 * @note Byte-by-byte writes are used to avoid cache coherence issues on uncached memory
 * 
 * @param[in]  queue       Pointer to the queue structure (in shared memory)
 * @param[in]  config      Configuration parameters for the queue
 * @param[in]  is_creator  1 if caller is the queue creator, 0 otherwise
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int hyperamp_queue_init(volatile HyperampShmQueue *queue, 
                               const HyperampQueueConfig *config,
                               int is_creator)
{
    printf("[HyperAmp] Initializing shared memory queue...\n");
    if (!queue || !config) return HYPERAMP_ERROR;
    if (config->block_size == 0 || config->capacity == 0) return HYPERAMP_ERROR;
    printf("[HyperAmp] Queue config: block_size=%d, capacity=%d, map_mode=%d\n",
           config->block_size, config->capacity, config->map_mode);
    if (is_creator) {
        // Creator: Initialize all fields using byte-by-byte writes (avoiding cache coherence issues)
        printf("[HyperAmp] Writing fields byte by byte...\n");
        volatile uint8_t *p = (volatile uint8_t *)queue;
        
        // Initialize map_mode1 and map_mode2
        p[0] = config->map_mode;
        p[1] = config->map_mode;
        HYPERAMP_BARRIER();
        
        // Initialize header (uint16_t, offset 2)
        p[2] = 0;
        p[3] = 0;
        HYPERAMP_BARRIER();
        
        // Initialize tail (uint16_t, offset 4)
        p[4] = 0;
        p[5] = 0;
        HYPERAMP_BARRIER();
        
        // Initialize capacity (uint16_t, offset 6)
        uint16_t cap = config->capacity;
        p[6] = cap & 0xFF;
        p[7] = (cap >> 8) & 0xFF;
        HYPERAMP_BARRIER();
        
        // Initialize block_size (uint16_t, offset 8)
        uint16_t bs = config->block_size;
        p[8] = bs & 0xFF;
        p[9] = (bs >> 8) & 0xFF;
        HYPERAMP_BARRIER();
        
        // Initialize _reserved (uint16_t, offset 10)
        p[10] = 0;
        p[11] = 0;
        HYPERAMP_BARRIER();
        
        printf("[HyperAmp] Writing phy_addr...\n");
        // Initialize phy_addr (uint64_t, offset 12) - byte-by-byte
        uint64_t pa = config->phy_addr;
        for (int i = 0; i < 8; i++) {
            p[12 + i] = (pa >> (i * 8)) & 0xFF;
        }
        HYPERAMP_BARRIER();
        
        printf("[HyperAmp] Writing virt_addr1...\n");
        // Initialize virt_addr1 (uint64_t, offset 20)
        uint64_t va = config->virt_addr;
        for (int i = 0; i < 8; i++) {
            p[20 + i] = (va >> (i * 8)) & 0xFF;
        }
        HYPERAMP_BARRIER();
        
        printf("[HyperAmp] Writing virt_addr2...\n");
        // Initialize virt_addr2 (uint64_t, offset 28) - zero
        for (int i = 0; i < 8; i++) {
            p[28 + i] = 0;
        }
        HYPERAMP_BARRIER();
        
        // Skip address mapping tables (table1 and table2) - no initialization needed
        printf("[HyperAmp] Skipping address mapping tables...\n");
        
        // Initialize spinlock (after mapping tables)
        printf("[HyperAmp] Initializing spinlock...\n");
        size_t lock_offset = offsetof(HyperampShmQueue, queue_lock);
        volatile HyperampSpinlock *lock = (volatile HyperampSpinlock *)&p[lock_offset];
        hyperamp_spinlock_init(lock);
        
        // Initialize extension fields
        printf("[HyperAmp] Writing magic and version...\n");
        size_t magic_offset = offsetof(HyperampShmQueue, magic);
        
        // Write magic (uint32_t) - byte-by-byte
        uint32_t magic = HYPERAMP_QUEUE_MAGIC;
        for (int i = 0; i < 4; i++) {
            p[magic_offset + i] = (magic >> (i * 8)) & 0xFF;
        }
        HYPERAMP_BARRIER();
        
        // Write version (uint32_t)
        uint32_t version = 1;
        for (int i = 0; i < 4; i++) {
            p[magic_offset + 4 + i] = (version >> (i * 8)) & 0xFF;
        }
        HYPERAMP_BARRIER();
        
        // Initialize enqueue_count (uint32_t) - zero
        for (int i = 0; i < 4; i++) {
            p[magic_offset + 8 + i] = 0;
        }
        HYPERAMP_BARRIER();
        
        // Initialize dequeue_count (uint32_t) - zero
        for (int i = 0; i < 4; i++) {
            p[magic_offset + 12 + i] = 0;
        }
        HYPERAMP_BARRIER();
        
        /* Flush entire queue control block to main memory for visibility across cores */
        printf("[HyperAmp] Flushing cache to memory...\n");
        hyperamp_cache_clean((volatile void *)queue, sizeof(HyperampShmQueue));
        
        printf("[HyperAmp] Queue initialization complete!\n");
    } else {
        // Non-creator: Set only their virtual address
        queue->virt_addr2 = config->virt_addr;
        HYPERAMP_BARRIER();
        
        /* Flush modified virtual address to main memory */
        hyperamp_cache_clean((volatile void *)&queue->virt_addr2, 8);
    }
    
    return HYPERAMP_OK;
}

/**
 * @brief Checks if the queue has been properly initialized
 * 
 * @details This function safely checks the queue initialization status using byte-by-byte reads.
 *          It uses the capacity field (offset 6) instead of magic to avoid page boundary issues.
 * 
 * @param[in] queue  Pointer to the queue structure
 * 
 * @return 1 if initialized (capacity > 0), 0 otherwise
 */
int hyperamp_queue_is_initialized(volatile HyperampShmQueue *queue)
{
    if (!queue) return 0;
    
    HYPERAMP_BARRIER();
    
    // Use capacity field (offset 6) instead of magic to avoid page boundary issues
    size_t capacity_offset = offsetof(HyperampShmQueue, capacity);
    volatile uint8_t *p = (volatile uint8_t *)queue;
    
    uint16_t capacity = 0;
    for (int i = 0; i < 2; i++) {
        capacity |= ((uint16_t)p[capacity_offset + i]) << (i * 8);
    }
    
    HYPERAMP_BARRIER();
    // Queue is initialized if capacity > 0
    return (capacity > 0);
}

/**
 * @brief Enqueues a message into the queue (with lock protection)
 * 
 * @details This function enqueues a message into the shared memory queue.
 *          It follows the same logic as HighSpeedCProxy's SHMP_QUEUE_ENQUEUE.
 * 
 * @note The queue must be initialized before use
 * @note The data length must not exceed block_size
 * @note Cache maintenance is performed after writing data
 * 
 * @param[in]  queue        Pointer to the queue structure
 * @param[in]  zone_id      Current zone ID (for lock ownership)
 * @param[in]  data         Data to enqueue
 * @param[in]  data_len     Length of data to enqueue
 * @param[in]  virt_base    Base virtual address of the data region
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int hyperamp_queue_enqueue(volatile HyperampShmQueue *queue,
                                  uint32_t zone_id,
                                  const void *data,
                                  size_t data_len,
                                  volatile void *virt_base)
{
    if (!queue || !data || data_len == 0) {
        printf("hyperamp_queue_enqueue failed: queue =%p, data = %p, data_len = %zu\n", queue, data, data_len);
        return HYPERAMP_ERROR;}
    if (data_len > queue->block_size) return HYPERAMP_ERROR;
    
    // Acquire lock
    hyperamp_spinlock_lock(HYPERAMP_QUEUE_LOCK(queue), zone_id);
    
    // Calculate new header
    printf("In %s, tail = %d, header = %d\n", __func__, queue->tail, queue->header);
    uint16_t new_header = queue->header + 1;
    if (new_header >= queue->capacity) {
        new_header -= queue->capacity;
    }
    
    // Check if queue would become full
    if (new_header == queue->tail) {
        printf("hyperamp_queue_enqueue failed: queue is full!\n");
        hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
        return HYPERAMP_AGAIN;  // Queue full
    }
 #if 1
    ProxyMsgHeader *proxy_msg_hdr_tmp;
    proxy_msg_hdr_tmp = (ProxyMsgHeader *)data;
    printf("frontend ID = %d, backend ID = %d, proxy msg type = %d, payload len = %d\n", 
            proxy_msg_hdr_tmp->frontend_sess_id, proxy_msg_hdr_tmp->backend_sess_id, proxy_msg_hdr_tmp->proxy_msg_type, proxy_msg_hdr_tmp->payload_len);
//    DUMP_BUFFER_CONTENT(data, data_len, "%0x2 ");
//    DUMP_PROXY_MSG_HEADER(data);
    parse_proxy_protocol_and_print(data);
 #endif

    // Calculate write address: header+1 position
    uint64_t write_addr = (uint64_t)virt_base + (uint64_t)(queue->header + 1) * queue->block_size;
    
    // Write data
    hyperamp_safe_memcpy((volatile void *)write_addr, data, data_len);
    
    // Update header
    queue->header = new_header;
    queue->enqueue_count++;
    
    HYPERAMP_BARRIER();
    
    /* Flush written data to memory */
    hyperamp_cache_clean((volatile void *)write_addr, data_len);
    /* Flush queue control block to memory */
    hyperamp_cache_clean((volatile void *)queue, 64);
    
    // Release lock
    hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
    
    return HYPERAMP_OK;
}

/**
 * @brief Dequeues a message from the queue (with lock protection)
 * 
 * @details This function dequeues a message from the shared memory queue.
 *          It follows the same logic as HighSpeedCProxy's SHMP_QUEUE_DEQUEUE.
 * 
 * @note The queue must be initialized before use
 * @note Cache invalidation is performed before reading data
 * 
 * @param[in]  queue         Pointer to the queue structure
 * @param[in]  zone_id       Current zone ID (for lock ownership)
 * @param[out] data          Buffer to receive the data
 * @param[in]  max_len       Maximum length of the buffer
 * @param[out] actual_len    Actual length of data received
 * @param[in]  virt_base     Base virtual address of the data region
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int hyperamp_queue_dequeue(volatile HyperampShmQueue *queue,
                                  uint32_t zone_id,
                                  void *data,
                                  size_t max_len,
                                  size_t *actual_len,
                                  volatile void *virt_base)
{
    utils_print("%s\n", __func__);
    if (!queue || !data || max_len == 0) {
        printf("hyperamp_queue_dequeue failed: queue =%p, data = %p, max_len = %zu\n", queue, data, max_len);
        return HYPERAMP_ERROR;
    }
    
    /* Invalidate cache before reading to ensure latest data */
    hyperamp_cache_invalidate((volatile void *)queue, 64);
    
    // Acquire lock
    hyperamp_spinlock_lock(HYPERAMP_QUEUE_LOCK(queue), zone_id);
    
    printf("In %s, tail = %d, header = %d\n", __func__, queue->tail, queue->header);
    // Check if queue is empty
    if (queue->tail == queue->header) {
        printf("hyperamp_queue_dequeue failed: queue is empty!\n");
        hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
        return HYPERAMP_AGAIN;  // Queue empty
    }
    
    // Calculate read address: tail+1 position
    uint64_t read_addr = (uint64_t)virt_base + (uint64_t)(queue->tail + 1) * queue->block_size;
    
    /* Invalidate data region cache before reading */
    hyperamp_cache_invalidate((volatile void *)read_addr, queue->block_size);
    
    // Calculate actual read length
    size_t read_len = (max_len < queue->block_size) ? max_len : queue->block_size;
    
    // Read data
    hyperamp_safe_memcpy(data, (const volatile void *)read_addr, read_len);
    
    if (actual_len) {
        *actual_len = read_len;
    }
    
    parse_proxy_protocol_and_print(data);
    printf("The address of message is %p\n", data);
    // Update tail
    uint16_t new_tail = queue->tail + 1;
    if (new_tail >= queue->capacity) {
        new_tail -= queue->capacity;
    }
    queue->tail = new_tail;
    queue->dequeue_count++;
    
    HYPERAMP_BARRIER();
    
    // Release lock
    hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
    return HYPERAMP_OK;
}

/**
 * @brief Peeks at the front element without dequeuing (no lock)
 * 
 * @details This function reads the front element of the queue without removing it.
 *          It uses the same locking mechanism as dequeue but does not modify the queue.
 * 
 * @note The queue must be initialized before use
 * 
 * @param[in]  queue        Pointer to the queue structure
 * @param[in]  zone_id      Current zone ID (for lock ownership)
 * @param[out] data         Buffer to receive the data
 * @param[in]  max_len      Maximum length of the buffer
 * @param[out] actual_len   Actual length of data received
 * @param[in]  virt_base    Base virtual address of the data region
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int hyperamp_queue_peek(volatile HyperampShmQueue *queue,
                               uint32_t zone_id,
                               void *data,
                               size_t max_len,
                               size_t *actual_len,
                               volatile void *virt_base)
{
    if (!queue || !data || max_len == 0) return HYPERAMP_ERROR;
    
    hyperamp_spinlock_lock(HYPERAMP_QUEUE_LOCK(queue), zone_id);
    
    if (queue->tail == queue->header) {
        hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
        return HYPERAMP_ERROR;
    }
    
    uint64_t read_addr = (uint64_t)virt_base + (uint64_t)(queue->tail + 1) * queue->block_size;
    size_t read_len = (max_len < queue->block_size) ? max_len : queue->block_size;
    
    hyperamp_safe_memcpy(data, (const volatile void *)read_addr, read_len);
    
    if (actual_len) {
        *actual_len = read_len;
    }
    
    HYPERAMP_BARRIER();
    hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
    
    return HYPERAMP_OK;
}

/**
 * @brief Allocates an enqueue slot (no lock, for zero-copy scenarios)
 * 
 * @details This function allocates a slot for enqueue operations without locking.
 *          The caller must use the returned address for zero-copy data placement.
 * 
 * @note The queue must be initialized before use
 * @note The slot address is valid until the next queue_enqueue call
 * 
 * @param[in]  queue        Pointer to the queue structure
 * @param[in]  zone_id      Current zone ID (for lock ownership)
 * @param[out] slot_addr    Output: Virtual address of the allocated slot
 * @param[in]  virt_base    Base virtual address of the data region
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int hyperamp_queue_alloc_slot(volatile HyperampShmQueue *queue,
                                     uint32_t zone_id,
                                     uint64_t *slot_addr,
                                     volatile void *virt_base)
{
    if (!queue || !slot_addr) return HYPERAMP_ERROR;
    
    hyperamp_spinlock_lock(HYPERAMP_QUEUE_LOCK(queue), zone_id);
    
    uint16_t next_header = (queue->header + 1) % queue->capacity;
    
    if (next_header == queue->tail) {
        hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
        return HYPERAMP_ERROR;
    }
    
    // Return address of next available slot
    *slot_addr = (uint64_t)virt_base + (uint64_t)(queue->header + 1) * queue->block_size;
    
    // Update header
    queue->header = next_header;
    queue->enqueue_count++;
    
    HYPERAMP_BARRIER();
    hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
    
    return HYPERAMP_OK;
}

/**
 * @brief Releases a dequeue slot (no lock, for zero-copy scenarios)
 * 
 * @details This function releases a slot after the data has been processed.
 *          It is used in zero-copy scenarios where the slot was allocated by hyperamp_queue_alloc_slot.
 * 
 * @note The queue must be initialized before use
 * 
 * @param[in]  queue        Pointer to the queue structure
 * @param[in]  zone_id      Current zone ID (for lock ownership)
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int hyperamp_queue_release_slot(volatile HyperampShmQueue *queue,
                                       uint32_t zone_id)
{
    if (!queue) return HYPERAMP_ERROR;
    
    hyperamp_spinlock_lock(HYPERAMP_QUEUE_LOCK(queue), zone_id);
    
    if (queue->tail == queue->header) {
        hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
        return HYPERAMP_ERROR;
    }
    
    uint16_t new_tail = (queue->tail + 1) % queue->capacity;
    queue->tail = new_tail;
    queue->dequeue_count++;
    
    HYPERAMP_BARRIER();
    hyperamp_spinlock_unlock(HYPERAMP_QUEUE_LOCK(queue));
    
    return HYPERAMP_OK;
}


/**
 * @brief Maps physical memory to user space
 * 
 * @details This function maps a physical memory region into the current process's 
 *          address space using /dev/hvisor for uncached memory access.
 *          The resulting mapping is stored in the global context (g_ctx).
 * 
 * @param[in] phys_addr  Physical address to map
 * @param[in] size       Size of the memory region to map
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int map_physical_memory(uint64_t phys_addr, size_t size)
{
    // Use /dev/hvisor instead of /dev/mem for uncached memory mapping
    g_ctx.fd_mem = open("/dev/hvisor", O_RDWR | O_SYNC);
    if (g_ctx.fd_mem < 0) {
        perror("[HyperAMP] Failed to open /dev/hvisor");
        return HYPERAMP_ERROR;
    }
    
    // Calculate page-aligned mapping parameters
    size_t page_size = sysconf(_SC_PAGESIZE);
    off_t page_offset = phys_addr & (page_size - 1);  // Offset within first page
    size_t map_size = size + page_offset;              // Total size including offset
    
    // Map physical memory with uncached attributes through /dev/hvisor
    // Note: Pass raw physical address directly to mmap - kernel handles page conversion
    void *mapped = mmap(NULL, map_size, 
                        PROT_READ | PROT_WRITE, 
                        MAP_SHARED, 
                        g_ctx.fd_mem, 
                        phys_addr);  // Direct physical address without pre-alignment
    
    if (mapped == MAP_FAILED) {
        perror("[HyperAMP] mmap failed");
        close(g_ctx.fd_mem);
        g_ctx.fd_mem = -1;
        return HYPERAMP_ERROR;
    }
    
    // Store final mapped address adjusted for page offset
    g_ctx.shm_base = (volatile void *)((char *)mapped + page_offset);
    g_ctx.shm_size = size;
    g_ctx.phys_addr = phys_addr;
    
    // Log mapping details
    printf("[HyperAMP] Physical memory mapped via /dev/hvisor (uncached):\n");
    printf("[HyperAMP]   Physical addr: 0x%lx\n", phys_addr);
    printf("[HyperAMP]   Virtual addr:  %p\n", g_ctx.shm_base);
    printf("[HyperAMP]   Size:          %zu bytes\n", size);
    
    return HYPERAMP_OK;
}


/**
 * @brief Unmaps physical memory from user space
 * 
 * @details This function properly unmaps previously mapped physical memory and
 *          closes the associated file descriptor. It handles the page alignment
 *          offset to ensure correct memory unmapping.
 */
void unmap_physical_memory(void)
{
    if (g_ctx.shm_base) {
        // Calculate original mapping base address and size
        size_t page_size = sysconf(_SC_PAGESIZE);
        off_t page_offset = g_ctx.phys_addr & (page_size - 1);
        void *map_base = (void *)((char *)g_ctx.shm_base - page_offset);
        size_t map_size = g_ctx.shm_size + page_offset;
        
        // Unmap the memory region
        munmap(map_base, map_size);
        g_ctx.shm_base = NULL;
    }
    
    // Close the memory device file descriptor
    if (g_ctx.fd_mem >= 0) {
        close(g_ctx.fd_mem);
        g_ctx.fd_mem = -1;
    }
}


HyperampLinuxContext g_ctx = {0};  // Global state context for HyperAMP Linux client

/* ==================== Public API ==================== */

/**
 * @brief Initializes the HyperAMP Linux client
 * 
 * @details This function initializes the HyperAMP Linux client and establishes 
 *          connection to the shared memory region. If phys_addr is 0, it uses 
 *          the default shared memory address defined in configuration.
 * 
 * @note When is_creator=1, the function will initialize both queues (to and from SEL4)
 *       When is_creator=0, it only connects to existing queues
 * 
 * @param[in] phys_addr  Shared memory physical address (0 to use default value)
 * @param[in] is_creator 1 if the caller is the queue creator (initialize queue), 
 *                       0 if connecting to an existing queue
 * 
 * @return HYPERAMP_OK on success, HYPERAMP_ERROR on failure
 */
int hyperamp_linux_init(uint64_t phys_addr, int is_creator)
{
    if (g_ctx.initialized) {
        printf("[HyperAMP] Already initialized\n");
        return HYPERAMP_OK;
    }
    
    // Use default address (start of TX Queue)
    if (phys_addr == 0) {
        phys_addr = SHM_START_PADDR;
    }
    
    printf("[HyperAMP] ========================================\n");
    printf("[HyperAMP] Initializing HyperAMP Linux Client\n");
    printf("[HyperAMP] ========================================\n");
    printf("[HyperAMP] Mode: %s\n", is_creator ? "CREATOR" : "CONNECTOR");
    printf("[HyperAMP] Physical address: 0x%lx\n", phys_addr);
    
    // Map physical memory (from TX Queue start address, map entire region)
    if (map_physical_memory(phys_addr, SHM_TOTAL_SIZE) != HYPERAMP_OK) {
        return HYPERAMP_ERROR;
    }
    
    // HyperAMP 4KB Queue Layout (matches seL4 side):
    // 0x7E000000: RX Queue (4KB) - seL4 writes, Linux reads (seL4 sends requests to Linux)
    // 0x7E001000: TX Queue (4KB) - Linux writes, seL4 reads (Linux sends responses to seL4)
    // 0x7E002000: Data Region (4MB) - Shared data area
    // 
    // Linux's RX Queue = seL4's TX Queue (physical address 0x7E000000)
    // Linux's TX Queue = seL4's RX Queue (physical address 0x7E001000)
    g_ctx.tx_queue = (volatile HyperampShmQueue *)((char *)g_ctx.shm_base + SHM_QUEUE_SIZE);  // 0x7E001000
    g_ctx.rx_queue = (volatile HyperampShmQueue *)g_ctx.shm_base;                              // 0x7E000000
    g_ctx.data_region = (volatile void *)((char *)g_ctx.shm_base + 2 * SHM_QUEUE_SIZE);
    
    printf("[HyperAMP] Memory layout:\n");
    printf("[HyperAMP]   TX Queue:    %p (phys: 0x%lx)\n", 
           g_ctx.tx_queue, phys_addr + SHM_QUEUE_SIZE);
    printf("[HyperAMP]   RX Queue:    %p (phys: 0x%lx)\n", 
           g_ctx.rx_queue, phys_addr);
    printf("[HyperAMP]   Data Region: %p (phys: 0x%lx, size: %d bytes)\n", 
           g_ctx.data_region, phys_addr + 2 * SHM_QUEUE_SIZE, SHM_DATA_SIZE);
    
    // Initialize queue configurations
    HyperampQueueConfig tx_config = {
        .map_mode = HYPERAMP_MAP_MODE_CONTIGUOUS_BOTH,
        .capacity = DEFAULT_QUEUE_CAPACITY,
        .block_size = DEFAULT_BLOCK_SIZE,
        .phy_addr = phys_addr,  // TX Queue base address
        .virt_addr = (uint64_t)g_ctx.tx_queue,
    };
    
    HyperampQueueConfig rx_config = {
        .map_mode = HYPERAMP_MAP_MODE_CONTIGUOUS_BOTH,
        .capacity = DEFAULT_QUEUE_CAPACITY,
        .block_size = DEFAULT_BLOCK_SIZE,
        .phy_addr = phys_addr + SHM_QUEUE_SIZE,  // RX Queue address
        .virt_addr = (uint64_t)g_ctx.rx_queue,
    };


    // Linux side is the creator of TX Queue, seL4 side is creator of RX Queue
    // But for simplicity, let Linux initialize both queues
    if (is_creator) {
        printf("[HyperAMP] Initializing TX queue......\n");
        if (hyperamp_queue_init(g_ctx.tx_queue, &tx_config, 1) != HYPERAMP_OK) {
            printf("[HyperAMP] Failed to init TX queue\n");
            unmap_physical_memory();
            return HYPERAMP_ERROR;
        }
        
        printf("[HyperAMP] Initializing RX queue...\n");
        if (hyperamp_queue_init(g_ctx.rx_queue, &rx_config, 1) != HYPERAMP_OK) {
            printf("[HyperAMP] Failed to init RX queue\n");
            unmap_physical_memory();
            return HYPERAMP_ERROR;
        }
        
        // Clear data region
        printf("[HyperAMP] Clearing data region...\n");
        hyperamp_safe_memset(g_ctx.data_region, 0, SHM_DATA_SIZE);
    } else {
        // Wait for queues to be initialized (check capacity field instead of magic, 
        // as magic field is beyond 4KB boundary)
        printf("[HyperAMP] Connecting to existing queues (no wait mode)...\n");
        
        /* Important: Flush CPU data cache to ensure reading latest data written by seL4 */
        CACHE_INVALIDATE(g_ctx.tx_queue);
        CACHE_INVALIDATE(g_ctx.rx_queue);
        
        // Print raw data for debugging
        printf("[HyperAMP] DEBUG: Raw TX Queue bytes (first 32):\n[HyperAMP]   ");
        volatile uint8_t *tx_bytes = (volatile uint8_t *)g_ctx.tx_queue;
        for (int i = 0; i < 32; i++) {
            printf("%02x ", tx_bytes[i]);
            if ((i + 1) % 16 == 0 && i < 31) printf("\n[HyperAMP]   ");
        }
        printf("\n");
        
        printf("[HyperAMP] DEBUG: Raw RX Queue bytes (first 32):\n[HyperAMP]   ");
        volatile uint8_t *rx_bytes = (volatile uint8_t *)g_ctx.rx_queue;
        for (int i = 0; i < 32; i++) {
            printf("%02x ", rx_bytes[i]);
            if ((i + 1) % 16 == 0 && i < 31) printf("\n[HyperAMP]   ");
        }
        printf("\n");
        
        // Directly read capacity field
        uint16_t tx_cap = hyperamp_safe_read_u16(g_ctx.tx_queue, offsetof(HyperampShmQueue, capacity));
        uint16_t rx_cap = hyperamp_safe_read_u16(g_ctx.rx_queue, offsetof(HyperampShmQueue, capacity));
        
        printf("[HyperAMP] TX capacity=%u (expected 256), RX capacity=%u (expected 256)\n", tx_cap, rx_cap);
        
        // Debug: Print raw bytes at queue headers
        printf("[HyperAMP] DEBUG: TX Queue raw bytes at offset 0-15:\n");
        printf("[HyperAMP]   ");
        for (int i = 0; i < 16; i++) {
            printf("%02x ", ((volatile uint8_t *)g_ctx.tx_queue)[i]);
        }
        printf("\n");
        
        printf("[HyperAMP] DEBUG: RX Queue raw bytes at offset 0-15:\n");
        printf("[HyperAMP]   ");
        for (int i = 0; i < 16; i++) {
            printf("%02x ", ((volatile uint8_t *)g_ctx.rx_queue)[i]);
        }
        printf("\n");
        
        if (tx_cap == 0 && rx_cap == 0) {
            printf("[HyperAMP] INFO: Queues not yet initialized by seL4\n");
            printf("[HyperAMP] Will wait for seL4 to initialize them...\n");
            // Don't return error, let backend simulator keep polling
        } else if (tx_cap == 256 && rx_cap == 256) {
            printf("[HyperAMP] ? Found initialized queue(s), ready for communication\n");
        } else {
            printf("[HyperAMP] WARNING: Unexpected capacity values (may indicate wrong address or corrupted memory)\n");
        }
    }
#if 1
    g_ctx.initialized = 1;
    g_ctx.tx_count = 0;
    g_ctx.rx_count = 0;
    g_ctx.tx_errors = 0;
    g_ctx.rx_errors = 0;
    
    printf("[HyperAMP] Initialization complete!\n");
    
    // Safely read queue information (byte-by-byte)
    uint32_t tx_magic = hyperamp_safe_read_u32(g_ctx.tx_queue, offsetof(HyperampShmQueue, magic));
    uint16_t tx_capacity = hyperamp_safe_read_u16(g_ctx.tx_queue, offsetof(HyperampShmQueue, capacity));
    uint16_t tx_block_size = hyperamp_safe_read_u16(g_ctx.tx_queue, offsetof(HyperampShmQueue, block_size));
    
    uint32_t rx_magic = hyperamp_safe_read_u32(g_ctx.rx_queue, offsetof(HyperampShmQueue, magic));
    uint16_t rx_capacity = hyperamp_safe_read_u16(g_ctx.rx_queue, offsetof(HyperampShmQueue, capacity));
    uint16_t rx_block_size = hyperamp_safe_read_u16(g_ctx.rx_queue, offsetof(HyperampShmQueue, block_size));
    
    printf("[HyperAMP] TX Queue: magic=0x%08x, capacity=%u, block_size=%u\n",
           tx_magic, tx_capacity, tx_block_size);
    printf("[HyperAMP] RX Queue: magic=0x%08x, capacity=%u, block_size=%u\n",
           rx_magic, rx_capacity, rx_block_size);
    printf("[HyperAMP] ========================================\n");
#endif
    return HYPERAMP_OK;
}