#include <thread>
#include <mutex>
#include <utility>
#include <map>
#include <stdio.h>
#include <ctime>
#include "IMemoryManager.hpp"

template <class T>
class PoolMemoryAllocator : public IMemoryManager
{
        const static int MAX_LEVEL = 5;
        struct FreeStore
        {
                FreeStore *next[MAX_LEVEL]; // No.Of Levels = MAX_LEVEL + 1
        };

        std::size_t _objectSize;
        std::size_t _poolSize;

        FreeStore *_freeStoreHead;
        void *_poolHead;
        void *_poolTail;

        const bool _supportMultiThreading;
        const bool _supportArrayAllocation;

        std::mutex _poolAccessMutex;
        std::map<void *, std::size_t> _allocatedArraysTable;

        void init(void);
        void cleanUp(void);
        const inline int randomOffest(const int &currentlevel)
        {
                static int lastIdx = 0;
                auto getRand = [](const int &lo, const int &hi) {
                        return rand() % ((hi - lo) + 1) + lo;
                };
                srand(time(NULL) * currentlevel);
                if ((lastIdx + 1) >= _poolSize)
                        lastIdx = 0;
                lastIdx++;
                lastIdx = getRand(lastIdx, _poolSize - 1);
                return lastIdx;
        }
        const inline int randomLevel(const int &min, const int &max)
        {
                static int idx = 0;
                auto getRand = [](const int &lo, const int &hi) {
                        return rand() % ((hi - lo) + 1) + lo;
                };
                srand(time(NULL) * idx);
                if ((idx + 1) >= _poolSize)
                        idx = 0;
                idx++;
                return getRand(min, max);
        }

public:
        explicit PoolMemoryAllocator(const std::size_t &poolSize = 1024, const bool &supportMultiThreading = false, const bool &supportArrayAllocation = false)
            : _supportMultiThreading(supportArrayAllocation), _supportArrayAllocation(supportArrayAllocation)
        {
                _poolSize = poolSize;
                _objectSize = (sizeof(T) > sizeof(FreeStore)) ? sizeof(T) : sizeof(FreeStore);
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
        void printSkipListMemory();
        void resetPoolSize(const std::size_t &poolSize);
};

template <class T>
inline void *PoolMemoryAllocator<T>::allocate(const std::size_t &size)
{
        if (_supportMultiThreading)
        {
                const std::lock_guard<std::mutex> lock(this->_poolAccessMutex);
        }
        auto allocateObject = [this](void) -> void * {
                FreeStore *head = _freeStoreHead;
                _freeStoreHead = head->next[0];
                return head;
        };

        if (nullptr == _freeStoreHead)
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
                auto head = _freeStoreHead;
                auto objectCount = 0;
                startAddress = reinterpret_cast<void *>(head);
                for (; n_objects != objectCount || head->next[0] == nullptr;)
                {
                        if (reinterpret_cast<char *>(head) + _objectSize == reinterpret_cast<char *>(head->next[0]))
                        {
                                objectCount++;
                                head = head->next[0];
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
                        _allocatedArraysTable.insert(std::pair<void *, std::size_t>(startAddress, n_objects));
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
                const std::lock_guard<std::mutex> lock(this->_poolAccessMutex);
        }
        if (nullptr == deleted)
        {
                return;
        }
        FreeStore *restoredAddress = reinterpret_cast<FreeStore *>(deleted);
        if (_supportArrayAllocation)
        {
                auto isItAllocated = _allocatedArraysTable.find(reinterpret_cast<void *>(restoredAddress));
                std::size_t n_objects = 0;
                if (isItAllocated != _allocatedArraysTable.end())
                {
                        n_objects = isItAllocated->second;
                        _allocatedArraysTable.erase(reinterpret_cast<void *>(restoredAddress));
                }
                else
                {
                        n_objects = 1;
                }

                n_objects -= 1;
                FreeStore *fs = _freeStoreHead;
                if (nullptr == _freeStoreHead || restoredAddress < _freeStoreHead)
                {
                        restoredAddress->next[0] = _freeStoreHead;
                        _freeStoreHead = restoredAddress;
                }
                else
                {
                        while (fs->next[0] != nullptr && restoredAddress > fs->next[0])
                        {
                                fs = fs->next[0];
                        }
                        // Found the start point to restore array objects.
                        restoredAddress->next[0] = fs->next[0];
                        fs->next[0] = restoredAddress;
                }
                // Continue from where we stopped.
                while (n_objects)
                {
                        fs = restoredAddress;
                        restoredAddress = reinterpret_cast<FreeStore *>(reinterpret_cast<char *>(restoredAddress) + _objectSize);
                        restoredAddress->next[0] = fs->next[0];
                        fs->next[0] = restoredAddress;
                        n_objects -= 1;
                }
        }
        else
        {
                restoredAddress->next[0] = _freeStoreHead;
                _freeStoreHead = restoredAddress;
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
        if (_poolSize <= 0)
        {
                printf("\n\033[1;31mCannot allocate zero pool size\033[0m\n");
                return;
        }
        _freeStoreHead = nullptr;
        /* Allocating The Objects Pool  */
        _poolHead = new char[_objectSize * _poolSize];
        _poolTail = reinterpret_cast<FreeStore *>(reinterpret_cast<char *>(_poolHead) + (_poolSize - 1) * _objectSize);
        _freeStoreHead = reinterpret_cast<FreeStore *>(_poolHead);

        for (int lv = 0; lv < MAX_LEVEL; lv++)
        {
                _freeStoreHead->next[lv] = reinterpret_cast<FreeStore *>(_poolTail);
                (reinterpret_cast<FreeStore *>(_poolTail))->next[lv] = nullptr;
        }

        FreeStore *head = _freeStoreHead;
        for (std::size_t i = 1; i < _poolSize; i++)
        {
                //! bug
                /*!
                 * @ i = 2, what is written to head->next[0] is written also to _freeStoreHead->next[2] !!
                 * earsing what is initally level 2 pointing to pool tail.
                 * even trying to re-write to _freeStoreHead->next[2], it
                 * affects head->next[0] @ i = 2 (what is pointing to ?)
                 */
                head->next[0] = reinterpret_cast<FreeStore *>(reinterpret_cast<char *>(_freeStoreHead) + i * _objectSize);
                head = head->next[0];
        }

        FreeStore *lastNodePerLevel[MAX_LEVEL] = {nullptr};
        FreeStore *walkingPtr = nullptr;

        for (int lv = 1; lv < MAX_LEVEL; lv++)
        {
                lastNodePerLevel[lv] = _freeStoreHead;
                lastNodePerLevel[lv]->next[lv] = _freeStoreHead->next[lv];
                printf("init LNPL[%d] = 0x%x\n", lv, reinterpret_cast<std::uintptr_t>(lastNodePerLevel[lv]));
                printf("init LNPL[%d]->next[%d] = 0x%x\n", lv, lv, reinterpret_cast<std::uintptr_t>(lastNodePerLevel[lv]->next[lv]));
        }
        walkingPtr = _freeStoreHead;
        walkingPtr = walkingPtr->next[0];
        int stop = 0;
        while (walkingPtr != reinterpret_cast<FreeStore *>(_poolTail))
        {
                int rLevel = randomLevel(0, MAX_LEVEL - 1);
                printf("-----------------------\n");
                printf("wp = 0x%x, Lv = %d,", reinterpret_cast<std::uintptr_t>(walkingPtr), rLevel);
                for (int lv = 1; lv <= rLevel; lv++)
                {
                        printf("LNPL[%d] = 0x%x , ", lv, reinterpret_cast<std::uintptr_t>(lastNodePerLevel[lv]));
                        printf("LNPL[%d]->next[%d] = 0x%x\n", lv, lv, reinterpret_cast<std::uintptr_t>(lastNodePerLevel[lv]->next[lv]));
                        walkingPtr->next[lv] = lastNodePerLevel[lv]->next[lv];
                        lastNodePerLevel[lv]->next[lv] = walkingPtr;
                        lastNodePerLevel[lv] = lastNodePerLevel[lv]->next[lv];
                        printf("(update)LNPL[%d] = 0x%x\n", lv, reinterpret_cast<std::uintptr_t>(lastNodePerLevel[lv]));
                }
                walkingPtr = walkingPtr->next[0];
                printf("\n");
                if (_poolSize < stop)
                {
                        printf("Exceeds pool nodes by %d\n", stop);
                        exit(EXIT_FAILURE);
                }
                ++stop;
        }
}

template <class T>
void PoolMemoryAllocator<T>::cleanUp(void)
{
        if (nullptr != _poolHead)
                delete[] _poolHead;
}

template <class T>
void PoolMemoryAllocator<T>::PrintMemory(void)
{
        FreeStore *FSHead = _freeStoreHead;
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
                        if (_freeStoreHead == sPtr)
                        {
                                printf("--> [0x%X] (pool head)\n", reinterpret_cast<std::uintptr_t>(sPtr));
                        }
                        else if (_poolTail == sPtr)
                        {
                                printf("--> [0x%X] (pool end)\n", reinterpret_cast<std::uintptr_t>(sPtr));
                        }
                        else
                        {
                                printf("--> 0x%X\n", reinterpret_cast<std::uintptr_t>(sPtr));
                        }

                        FSHead = FSHead->next[0];
                        if (idx > _poolSize)
                        {
                                printf("\n\033[1;31mBUG: @ idx = %u more than pool size = %lu objects\033[0m\n", idx, _poolSize);
                        }
                        ++idx;
                }
                printf("null \n");
        }
}

template <class T>
inline void PoolMemoryAllocator<T>::printSkipListMemory()
{
        FreeStore *FSHead = _freeStoreHead;
        printf("================== [Free Blocks] ================== \n");
        if (nullptr == FSHead)
        {
                printf("\tNo Free Blocks in the Pool\n");
        }
        else
        {
                for (int lv = 0; lv < MAX_LEVEL; lv++)
                {
                        int index = 0;
                        printf("Lvl.No (%d)\n", lv);
                        printf("=============\n");
                        FSHead = _freeStoreHead;
                        FreeStore *sPtr = FSHead;
                        for (; sPtr; sPtr = FSHead)
                        {
                                printf("[0x%x] => ", reinterpret_cast<std::uintptr_t>(sPtr));
                                FSHead = FSHead->next[lv];
                                if (index > _poolSize)
                                {
                                        printf("STOPPED Exceeds the pool size !\n");
                                        exit(EXIT_FAILURE);
                                }
                                index++;
                        }
                        printf("null \n");
                }
        }
}