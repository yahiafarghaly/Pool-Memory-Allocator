#include "complex.hpp"
#include "PoolMemoryAllocator.hpp"
#include <iostream>

extern PoolMemoryAllocator<Complex> poolMemoryManager;

#ifdef OVERLOADED_MEMORY

void *Complex::operator new(size_t size)
{
#ifdef POOL_MEMORY
        return poolMemoryManager.allocate(size);
#endif
}

void Complex::operator delete(void *pointerToDelete)
{
#ifdef POOL_MEMORY
        poolMemoryManager.free(pointerToDelete);
#endif
}

void *Complex::operator new[](size_t size)
{
#ifdef POOL_MEMORY
        return poolMemoryManager.allocate(size);
#endif
}

void Complex::operator delete[](void *arrayToDelete)
{
#ifdef POOL_MEMORY
        poolMemoryManager.free(arrayToDelete);
#endif
}

#endif