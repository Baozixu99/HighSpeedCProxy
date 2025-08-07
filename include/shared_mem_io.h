#ifndef SHARED_MEM_IO_H
#define SHARED_MEM_IO_H

#include <stdint.h>


struct SharedMemoryPool{
    int value; // Just for placehoder. We will redefine this struct after receiving the partner's document.
};


struct SharedMemoryPoolLock{
    int value; // Just for placehoder. We will redefine this struct after receiving the partner's document.
};

uint64_t alloc_shared_mem();
void free_shared_mem(uint64_t addr);


#endif