#include <thread>
#include <mutex>
#include <utility>
#include <map>
#include <stdio.h>
#include "IMemoryManager.hpp"

template <class T>
class PoolMemoryAllocator : public IMemoryManager
{
        struct FreeStore
        {
                FreeStore *next;
        };

        std::size_t _objectSize;
        std::size_t _poolSize;

        FreeStore *_freeStore_ObjectHead;
        void *_pool_ObjectHead;
        void *_pool_ObjectTail;

        const bool _supportMultiThreading;
        const bool _supportArrayAllocation;

        std::mutex _allocator_mutex;
        std::mutex _free_mutex;
        std::map<void *, std::size_t> _arrayAllocationTable;

        void init(void);
        void cleanUp(void);

public:
        explicit PoolMemoryAllocator(const std::size_t &poolSize = 1024, const bool &supportMultiThreading = false, const bool &supportArrayAllocation = false)
            : _supportMultiThreading(supportArrayAllocation), _supportArrayAllocation(supportArrayAllocation)
        {
                _poolSize = poolSize;
                _objectSize = (sizeof(T) > sizeof(FreeStore *)) ? sizeof(T) : sizeof(FreeStore *);
                init();
        }
        virtual ~PoolMemoryAllocator()
        {
                cleanUp();
        }
        void *allocate(const std::size_t &) override;
        void free(void *) override;
        // Memory Methods
        void PrintMemory(void);
        void resetPoolSize(const std::size_t &poolSize);
};

template <class T>
inline void *PoolMemoryAllocator<T>::allocate(const std::size_t &size)
{
        if (_supportMultiThreading)
        {
                const std::lock_guard<std::mutex> lock(this->_allocator_mutex);
        }
        auto allocateObject = [this](void) -> void * {
                FreeStore *head = _freeStore_ObjectHead;
                _freeStore_ObjectHead = head->next;
                return head;
        };

        if (nullptr == _freeStore_ObjectHead)
        {
                return nullptr;
        }
        if (size == _objectSize)
        {
                return allocateObject();
        }

        void *startAddress = nullptr;
        if (_supportArrayAllocation)
        {
                const std::size_t n_objects = (size / _objectSize) + (size % _objectSize);
                auto head = _freeStore_ObjectHead;
                auto objectCount = 0;
                startAddress = reinterpret_cast<void *>(head);
                for (; n_objects != objectCount || head->next == nullptr;)
                {
                        if (reinterpret_cast<char *>(head) + _objectSize == reinterpret_cast<char *>(head->next))
                        {
                                objectCount++;
                                head = head->next;
                                continue;
                        }
                        else
                        {
                                objectCount = 0;
                                break;
                        }
                }

                if (n_objects != objectCount)
                {
                        return nullptr;
                }
                else
                {
                        startAddress = allocateObject();
                        _arrayAllocationTable.insert(std::pair<void *, std::size_t>(startAddress, n_objects));
                        for (auto i = 0; i < n_objects - 1; i++)
                        {
                                allocateObject();
                        }
                }
        }
        return startAddress;
}

template <class T>
inline void PoolMemoryAllocator<T>::free(void *deleted)
{
        if (_supportMultiThreading)
        {
                const std::lock_guard<std::mutex> lock(this->_free_mutex);
        }
        if (nullptr == deleted)
        {
                return;
        }
        FreeStore *restoredAddress = reinterpret_cast<FreeStore *>(deleted);
        if (_supportArrayAllocation)
        {
                auto isItAllocated = _arrayAllocationTable.find(reinterpret_cast<void *>(restoredAddress));
                std::size_t n_objects = 0;
                if (isItAllocated != _arrayAllocationTable.end())
                {
                        n_objects = isItAllocated->second;
                }
                else
                {
                        n_objects = 1;
                }
                do
                {
                        n_objects -= 1;
                        if (nullptr == _freeStore_ObjectHead || restoredAddress < _freeStore_ObjectHead)
                        {
                                restoredAddress->next = _freeStore_ObjectHead;
                                _freeStore_ObjectHead = restoredAddress;
                        }
                        else
                        {
                                //Normal Sort
                                FreeStore *fs = _freeStore_ObjectHead;
                                while (fs->next != nullptr && restoredAddress > fs->next)
                                {
                                        fs = fs->next;
                                }
                                restoredAddress->next = fs->next;
                                fs->next = restoredAddress;
                        }
                        restoredAddress = reinterpret_cast<FreeStore *>(reinterpret_cast<char *>(restoredAddress) + _objectSize);
                } while (n_objects);
        }
        else
        {
                restoredAddress->next = _freeStore_ObjectHead;
                _freeStore_ObjectHead = restoredAddress;
        }
}

template <class T>
void PoolMemoryAllocator<T>::resetPoolSize(const std::size_t &poolSize)
{
        cleanUp();
        _poolSize = poolSize;
        init();
}

template <class T>
void PoolMemoryAllocator<T>::init(void)
{
        _freeStore_ObjectHead = nullptr;
        /* Allocating The Objects Pool  */
        _pool_ObjectHead = new char[_objectSize * _poolSize];
        FreeStore *head = reinterpret_cast<FreeStore *>(_pool_ObjectHead);
        _freeStore_ObjectHead = head;
        for (std::size_t i = 1; i < _poolSize; i++)
        {
                head->next = reinterpret_cast<FreeStore *>(reinterpret_cast<char *>(_freeStore_ObjectHead) + i * _objectSize);
                head = head->next;
        }
        _pool_ObjectTail = head;
        head->next = nullptr;
}

template <class T>
void PoolMemoryAllocator<T>::cleanUp(void)
{
        if (nullptr != _pool_ObjectHead)
                delete[] _pool_ObjectHead;
}

template <class T>
void PoolMemoryAllocator<T>::PrintMemory(void)
{
        FreeStore *FSHead = _freeStore_ObjectHead;
        printf("================== [Free Blocks] ================== \n");
        if (nullptr == FSHead)
        {
                printf("\tNo Free Blocks in the Pool\n");
        }
        else
        {
                FreeStore *sPtr = FSHead;
                unsigned int idx = 1;
                for (; sPtr; sPtr = FSHead)
                {
                        if (_freeStore_ObjectHead == sPtr)
                        {
                                printf("--> [0x%X] (pool head)\n", reinterpret_cast<std::uintptr_t>(sPtr));
                        }
                        else if (_pool_ObjectTail == sPtr)
                        {
                                printf("--> [0x%X] (pool end)\n", reinterpret_cast<std::uintptr_t>(sPtr));
                        }
                        else
                        {
                                printf("--> 0x%X\n", reinterpret_cast<std::uintptr_t>(sPtr));
                        }

                        FSHead = FSHead->next;
                        if (idx > _poolSize)
                        {
                                printf("\n\033[1;31mBUG: @ idx = %u more than pool size = %lu objects\033[0m\n", idx, _poolSize);
                        }
                        ++idx;
                }
                printf("null \n");
        }
}