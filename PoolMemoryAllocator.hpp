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
        std::size_t _poolMemoryIncreaseCount;
        FreeStore *_freeStoreHead;
        void *_poolHead;
        std::mutex _allocator_mutex;
        std::mutex _free_mutex;
        std::map<void *, void *> _p2pTable;

        void expandPoolSize(void);
        void init(void);
        void cleanUp(void);
        void *allocateSingleObject(void);

public:
        PoolMemoryAllocator()
        {
                _poolSize = 10;
                init();
        }
        PoolMemoryAllocator(const std::size_t &poolSize)
        {
                _poolSize = poolSize;
                init();
        }
        virtual ~PoolMemoryAllocator()
        {
                cleanUp();
        }
        void *allocate(const std::size_t &) override;
        void free(void *) override;
        void checkMemory(void);
};

template <class T>
inline void *PoolMemoryAllocator<T>::allocateSingleObject(void)
{
        if (nullptr == _freeStoreHead)
        {
                expandPoolSize();
        }
        FreeStore *head = _freeStoreHead;
        _freeStoreHead = head->next;
        return head;
}

template <class T>
inline void *PoolMemoryAllocator<T>::allocate(const std::size_t &size)
{
        const std::lock_guard<std::mutex> lock(this->_allocator_mutex);

        if (size == _objectSize)
        {
                return this->allocateSingleObject();
        }

        const std::size_t n_objects = size / _objectSize + (size % _objectSize);
        void *startAddress = nullptr;

        switch (n_objects)
        {
        case 0:
                break;
        case 1:
                startAddress = this->allocateSingleObject();
                _p2pTable.insert(std::pair<void *, void *>(startAddress, nullptr));
                break;
        default:
                void *currentAddress = nullptr;
                void *nextAddress = nullptr;
                const std::size_t nAllocations = (n_objects % 2) ? n_objects - 1 : n_objects;
                startAddress = this->allocateSingleObject();
                printf("%p reserved as start Address\n",startAddress);
                currentAddress = startAddress;
                for (int i = 0; i < nAllocations; i += 1) // [BUG]: doesn't work with odd array sizes.
                {
                        if (i == n_objects - 1 && !(n_objects % 2))
                                break;
                        nextAddress = this->allocateSingleObject();
                        printf("[%p]:[%p] reserved as [%i]\n",(char*)currentAddress + _objectSize,nextAddress,i+1);
                        _p2pTable.insert(std::pair<void *, void *>(currentAddress, nextAddress));
                        currentAddress = nextAddress;
                }
                _p2pTable.insert(std::pair<void *, void *>(nextAddress, nullptr));

                break;
        }

        return startAddress;
}

template <class T>
inline void PoolMemoryAllocator<T>::free(void *deleted)
{
        const std::lock_guard<std::mutex> lock(this->_free_mutex);
        auto freeSingleAllocatedObject = [this](void *deletedObject) -> void {
                FreeStore *head = static_cast<FreeStore *>(deletedObject);
                head->next = _freeStoreHead;
                _freeStoreHead = head;
        };
        auto startAddressIter = _p2pTable.find(deleted);
        if (startAddressIter != _p2pTable.end())
        {
                void *nextAddress = startAddressIter->first;
                void *deletedAddress = nullptr;
                do
                {
                        freeSingleAllocatedObject(nextAddress);
                        deletedAddress = nextAddress;
                        nextAddress = _p2pTable.at(nextAddress);
                        _p2pTable.erase(deletedAddress);
                } while (nextAddress != nullptr);
        }
        else
        {
                freeSingleAllocatedObject(deleted);
        }
}

template <class T>
void PoolMemoryAllocator<T>::expandPoolSize(void)
{
        // FreeStore *head = reinterpret_cast<FreeStore *>(new char[_objectSize]);
        // _freeStoreHead = head;
        // printf("Allocating Head^ = %p\n", head);
        // for (std::size_t i = 0; i < _poolSize - 1; i++)
        // {
        //         head->next = reinterpret_cast<FreeStore *>(new char[_objectSize]);
        //         printf("Allocating ^ = %p , diff_sz = %d\n", head->next, head->next - head);
        //         head = head->next;
        // }
        // head->next = nullptr;
        _poolHead = new char[_objectSize * _poolSize];
        printf("Pool->Head [%p] ,Pool->Tail [%p]\n", (char *)_poolHead, (char *)_poolHead + _objectSize * (_poolSize - 1));
        printf("Pool Size = %d Objects\n", _poolSize);
        printf("Object Size = %u bytes\n", _objectSize);
        FreeStore *head = static_cast<FreeStore *>(_poolHead);
        _freeStoreHead = head;
        printf("Alloc_Head = %p\n", head);
        for (std::size_t i = 1; i < _poolSize; i++)
        {
                head->next = reinterpret_cast<FreeStore*>(reinterpret_cast<char*>(_freeStoreHead) + i*_objectSize);
                printf("Alloc_%p => %p\n",head ,head->next);
                head = head->next;
        }
        printf("Alloc_%p => %p\n",head ,nullptr);
        head->next = nullptr;
        _poolMemoryIncreaseCount++;
}

template <class T>
void PoolMemoryAllocator<T>::init(void)
{
        _objectSize = (sizeof(T) > sizeof(FreeStore *)) ? sizeof(T) : sizeof(FreeStore *);
        _freeStoreHead = nullptr;
        _poolMemoryIncreaseCount = 0;
        expandPoolSize();
}

template <class T>
void PoolMemoryAllocator<T>::cleanUp(void)
{
        // FreeStore *nextPtr = _freeStoreHead;
        // for (; nextPtr; nextPtr = _freeStoreHead)
        // {

        //         _freeStoreHead = _freeStoreHead->next;
        //         if (_freeStoreHead == nullptr)
        //         {
        //                 printf("FreeStore Head = null\n");
        //         }
        //         printf("De-Allocate = %p\n", nextPtr);

        //         delete[] nextPtr; // remember this was a char array
        // }
        printf("De-Allocate Pool = %p\n", _poolHead);
        delete[] _poolHead;
}

template <class T>
void PoolMemoryAllocator<T>::checkMemory(void)
{
        FreeStore *FSHead = _freeStoreHead;
        printf("================== [Free Blocks] ================== \n");
        printf("Free Head = 0x%X\n", reinterpret_cast<std::uintptr_t>(_freeStoreHead));
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
                        printf("0x%X -> ", reinterpret_cast<std::uintptr_t>(sPtr));
                        FSHead = FSHead->next;

                        if (idx > _poolSize * _poolMemoryIncreaseCount)
                        {
                                printf("\n\033[1;31mBUG: @ idx = %u more than pool size = %lu objects\033[0m\n", idx, _poolSize * _poolMemoryIncreaseCount);
                        }
                        ++idx;
                }
                printf("null \n");
        }
}