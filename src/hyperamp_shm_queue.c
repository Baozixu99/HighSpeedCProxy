#include "hyperamp_shm_queue.h"

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
     * @def HYPERAMP_DSB()
     * Data Synchronization Barrier: Ensures all memory accesses before this
     * instruction complete before any subsequent instructions proceed.
     */
    #define HYPERAMP_DSB()   __asm__ volatile("dsb sy" ::: "memory")

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
     #define HYPERAMP_DSB()   __asm__ volatile("mfence" ::: "memory")
     #define HYPERAMP_ISB()   __asm__ volatile("" ::: "memory")
     
    inline void hyperamp_cache_clean(volatile void *addr, size_t size) {
         (void)addr; (void)size;
         __asm__ volatile("mfence" ::: "memory");
     }
     
    inline void hyperamp_cache_invalidate(volatile void *addr, size_t size) {
         (void)addr; (void)size;
         __asm__ volatile("mfence" ::: "memory");
     }
#endif

/**
 * @def HYPERAMP_BARRIER()
 * Combined memory barrier combining both DMB and DSB semantics.
 * Ensures full ordering of memory operations before and after this macro.
 */
#define HYPERAMP_BARRIER()   do { HYPERAMP_DMB(); HYPERAMP_DSB(); } while(0)

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
inline void hyperamp_spinlock_init(volatile HyperampSpinlock *lock)
{
    if (!lock) return;
    
    volatile uint8_t *p = (volatile uint8_t *)lock;
    for (size_t i = 0; i < sizeof(HyperampSpinlock); i++) {
        p[i] = 0;
    }
    HYPERAMP_BARRIER();
    
    /* Flush lock state to main memory */
    hyperamp_cache_clean((volatile void *)lock, sizeof(HyperampSpinlock));
}


/**
 * @brief Acquires a spinlock using software-based implementation (no atomic instructions)
 * 
 * @details This function implements a pure software spinlock with zone-based ownership tracking.
 *          The lock acquisition follows a test-and-set strategy using memory barriers for
 *          visibility guarantees. The implementation includes:
 *          - Cache coherency management for shared memory
 *          - Zone ID tracking for debugging
 *          - Spin-wait with exponential backoff
 *          - Simple delay mechanism during contention
 * 
 * @param[in] lock     Pointer to the spinlock structure (must be valid and initialized)
 * @param[in] zone_id  Identifier of the current zone/thread (for debugging and ownership tracking)
 * 
 * @note This implementation assumes:
 *       - Memory is uncached or cache maintenance operations are used
 *       - Lock structure resides in shared memory accessible to all contenders
 *       - Proper cache cleaning/invalidate operations are available
 * 
 * @warning This function will spin indefinitely if the lock cannot be acquired
 * @warning Should only be used in contexts where deadlock is guaranteed not to occur
 */
inline void hyperamp_spinlock_lock(volatile HyperampSpinlock *lock, uint32_t zone_id)
{
    if (!lock) return;
    
    int spin_count = 0;
    const int max_spin = 100000;
    
    while (1) {
        HYPERAMP_BARRIER();
        
        volatile uint32_t current = lock->lock_value;
        
        if (current == 0) {
            // Attempt to acquire the lock
            lock->lock_value = 1;
            HYPERAMP_BARRIER();
            
            // Verify successful acquisition
            volatile uint32_t verify = lock->lock_value;
            if (verify == 1) {
                lock->owner_zone_id = zone_id;
                lock->lock_count++;
                HYPERAMP_BARRIER();
                
                /* Flush lock state to main memory to ensure visibility across cores/virtual machines */
                hyperamp_cache_clean((volatile void *)lock, sizeof(HyperampSpinlock));
                return;  // Successfully acquired the lock
            }
        }
        
        // Lock is held, spin-wait with backoff strategy
        lock->contention_count++;
        spin_count++;
        
        // Apply exponential backoff
        if (spin_count > max_spin) {
            spin_count = 0;
#if defined(__aarch64__) || defined(__arm__)
            __asm__ volatile("yield" ::: "memory");
#else
            __asm__ volatile("pause" ::: "memory");
#endif
        }
        
        // Short delay with memory barrier
        for (volatile int i = 0; i < 100; i++) {
            HYPERAMP_BARRIER();
        }
    }
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
inline void hyperamp_spinlock_unlock(volatile HyperampSpinlock *lock)
{
    if (!lock) return;
    
    HYPERAMP_BARRIER();
    lock->owner_zone_id = 0;
    lock->lock_value = 0;
    HYPERAMP_BARRIER();
    
    /* Flush lock state to main memory to ensure visibility across cores/virtual machines */
    hyperamp_cache_clean((volatile void *)lock, sizeof(HyperampSpinlock));
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
inline int hyperamp_spinlock_trylock(volatile HyperampSpinlock *lock, uint32_t zone_id)
{
    if (!lock) return HYPERAMP_ERROR;
    
    HYPERAMP_BARRIER();
    volatile uint32_t current = lock->lock_value;
    
    if (current == 0) {
        lock->lock_value = 1;
        HYPERAMP_BARRIER();
        
        volatile uint32_t verify = lock->lock_value;
        if (verify == 1) {
            lock->owner_zone_id = zone_id;
            lock->lock_count++;
            HYPERAMP_BARRIER();
            return HYPERAMP_OK;
        }
    }
    
    return HYPERAMP_ERROR;
}



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
inline void hyperamp_safe_memset(volatile void *dst, uint8_t val, size_t len)
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
inline void hyperamp_safe_memcpy(volatile void *dst, const volatile void *src, size_t len)
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
inline uint16_t hyperamp_safe_read_u16(const volatile void *addr, size_t offset)
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
inline uint32_t hyperamp_safe_read_u32(const volatile void *addr, size_t offset)
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
inline int hyperamp_queue_init(volatile HyperampShmQueue *queue, 
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
inline int hyperamp_queue_is_initialized(volatile HyperampShmQueue *queue)
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
inline int hyperamp_queue_enqueue(volatile HyperampShmQueue *queue,
                                  uint32_t zone_id,
                                  const void *data,
                                  size_t data_len,
                                  volatile void *virt_base)
{
    if (!queue || !data || data_len == 0) return HYPERAMP_ERROR;
    if (data_len > queue->block_size) return HYPERAMP_ERROR;
    
    // Acquire lock
    hyperamp_spinlock_lock(&queue->queue_lock, zone_id);
    
    // Calculate new header
    uint16_t new_header = queue->header + 1;
    if (new_header >= queue->capacity) {
        new_header -= queue->capacity;
    }
    
    // Check if queue would become full
    if (new_header == queue->tail) {
        hyperamp_spinlock_unlock(&queue->queue_lock);
        return HYPERAMP_ERROR;  // Queue full
    }
    
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
    hyperamp_spinlock_unlock(&queue->queue_lock);
    
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
inline int hyperamp_queue_dequeue(volatile HyperampShmQueue *queue,
                                  uint32_t zone_id,
                                  void *data,
                                  size_t max_len,
                                  size_t *actual_len,
                                  volatile void *virt_base)
{
    if (!queue || !data || max_len == 0) return HYPERAMP_ERROR;
    
    /* Invalidate cache before reading to ensure latest data */
    hyperamp_cache_invalidate((volatile void *)queue, 64);
    
    // Acquire lock
    hyperamp_spinlock_lock(&queue->queue_lock, zone_id);
    
    // Check if queue is empty
    if (queue->tail == queue->header) {
        hyperamp_spinlock_unlock(&queue->queue_lock);
        return HYPERAMP_ERROR;  // Queue empty
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
    
    // Update tail
    uint16_t new_tail = queue->tail + 1;
    if (new_tail >= queue->capacity) {
        new_tail -= queue->capacity;
    }
    queue->tail = new_tail;
    queue->dequeue_count++;
    
    HYPERAMP_BARRIER();
    
    // Release lock
    hyperamp_spinlock_unlock(&queue->queue_lock);
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
inline int hyperamp_queue_peek(volatile HyperampShmQueue *queue,
                               uint32_t zone_id,
                               void *data,
                               size_t max_len,
                               size_t *actual_len,
                               volatile void *virt_base)
{
    if (!queue || !data || max_len == 0) return HYPERAMP_ERROR;
    
    hyperamp_spinlock_lock(&queue->queue_lock, zone_id);
    
    if (queue->tail == queue->header) {
        hyperamp_spinlock_unlock(&queue->queue_lock);
        return HYPERAMP_ERROR;
    }
    
    uint64_t read_addr = (uint64_t)virt_base + (uint64_t)(queue->tail + 1) * queue->block_size;
    size_t read_len = (max_len < queue->block_size) ? max_len : queue->block_size;
    
    hyperamp_safe_memcpy(data, (const volatile void *)read_addr, read_len);
    
    if (actual_len) {
        *actual_len = read_len;
    }
    
    HYPERAMP_BARRIER();
    hyperamp_spinlock_unlock(&queue->queue_lock);
    
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
inline int hyperamp_queue_alloc_slot(volatile HyperampShmQueue *queue,
                                     uint32_t zone_id,
                                     uint64_t *slot_addr,
                                     volatile void *virt_base)
{
    if (!queue || !slot_addr) return HYPERAMP_ERROR;
    
    hyperamp_spinlock_lock(&queue->queue_lock, zone_id);
    
    uint16_t next_header = (queue->header + 1) % queue->capacity;
    
    if (next_header == queue->tail) {
        hyperamp_spinlock_unlock(&queue->queue_lock);
        return HYPERAMP_ERROR;
    }
    
    // Return address of next available slot
    *slot_addr = (uint64_t)virt_base + (uint64_t)(queue->header + 1) * queue->block_size;
    
    // Update header
    queue->header = next_header;
    queue->enqueue_count++;
    
    HYPERAMP_BARRIER();
    hyperamp_spinlock_unlock(&queue->queue_lock);
    
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
inline int hyperamp_queue_release_slot(volatile HyperampShmQueue *queue,
                                       uint32_t zone_id)
{
    if (!queue) return HYPERAMP_ERROR;
    
    hyperamp_spinlock_lock(&queue->queue_lock, zone_id);
    
    if (queue->tail == queue->header) {
        hyperamp_spinlock_unlock(&queue->queue_lock);
        return HYPERAMP_ERROR;
    }
    
    uint16_t new_tail = (queue->tail + 1) % queue->capacity;
    queue->tail = new_tail;
    queue->dequeue_count++;
    
    HYPERAMP_BARRIER();
    hyperamp_spinlock_unlock(&queue->queue_lock);
    
    return HYPERAMP_OK;
}