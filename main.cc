#include <iostream>
#include <vector>
#include "timer.hpp"
#include "complex.hpp"
#include "PoolMemoryAllocator.hpp"

PoolMemoryAllocator<Complex> poolMemoryManager(7, false, true);

int main()
{
        const int nIteration = 1000;
        const int nAllocation = 2000;
        poolMemoryManager.resetPoolSize(nIteration * nAllocation);
        std::cout << "Time Test : ( Many (De-)Allocation Per Iteration )\n";
        std::cout << "no.Iteration = " << nIteration << " ,no.(De-)Allocation/Iteration = " << nAllocation << "\n";
        Complex *array[nAllocation];
        Timer poolAllocationTime, allocationTime, freeTime;
        poolAllocationTime.setTimeUnit(TimeUnit::ms);
        allocationTime.setTimeUnit(TimeUnit::ns);
        freeTime.setTimeUnit(TimeUnit::ns);
        poolAllocationTime.start();
        for (int i = 0; i < nIteration; i++)
        {
                allocationTime.start();
                for (int j = 0; j < nAllocation; j++)
                        array[j] = new Complex(i, j);
                allocationTime.end();
                freeTime.start();
                for (int j = 0; j < nAllocation; j++)
                        delete array[j];
                freeTime.end();
        }
        poolAllocationTime.end();
        std::cout << "Time Test = " << poolAllocationTime.getTime() << poolAllocationTime.getTimeUnitString() << std::endl;
        std::cout << "Allocation Time = " << allocationTime.getTime() << allocationTime.getTimeUnitString() << std::endl;
        std::cout << "De-Allocation Time = " << freeTime.getTime() << freeTime.getTimeUnitString() << std::endl;
        std::cout << "=======================================\n\n";

        std::cout << "Random (De-)Allocations Test\n";
        poolMemoryManager.resetPoolSize(10);
        poolMemoryManager.PrintMemory();
        Complex *c1 = new Complex(50, 50);
        Complex *c2 = new Complex(100, 100);
        Complex *c3 = new Complex(150, 150);
        printf("Allocated Addresses for (c1,c2,c3) = %p , %p , %p\n", c1, c2, c3);
        delete c2;
        delete c3;
        delete c1;
        c1 = c2 = c3 = nullptr;
        poolMemoryManager.PrintMemory();
        std::cout << "=======================================\n\n";

        std::cout << "Contiguous Allocated array Test\n";
        std::cout << "Allocating Complex[4] \n";
        Complex *ComplexArray = new Complex[4];
        if (ComplexArray == nullptr)
        {
                printf("Cannot allocate Memory \n");
                return -1;
        }
        printf("Array Blocks Addresses:\n");
        for (auto i = 0; i < 4; i++)
        {
                ComplexArray[i].r = i;
                ComplexArray[i].c = i * 2;
                printf(" --> [%i] = %p , (r,c) = (%f,%f)\n", i, ComplexArray + i, ComplexArray[i].r, ComplexArray[i].c);
        }
        c1 = new Complex(6, 6);
        poolMemoryManager.PrintMemory();
        delete[] ComplexArray;
        poolMemoryManager.PrintMemory();
        ComplexArray = nullptr;
        delete c1;
        std::cout << "=======================================\n\n";
        std::cout << "Contiguous Allocated array Test\n";
        std::cout << "Allocating Complex[5] \n";
        ComplexArray = new Complex[5];
        if (ComplexArray == nullptr)
        {
                printf("Cannot allocate Memory \n");
                return -1;
        }
        printf("Array Blocks Addresses:\n");
        for (auto i = 0; i < 5; i++)
        {
                ComplexArray[i].r = i + 2;
                ComplexArray[i].c = i * 4;
                printf(" --> [%i] = %p , (r,c) = (%f,%f)\n", i, ComplexArray + i, ComplexArray[i].r, ComplexArray[i].c);
        }
        poolMemoryManager.PrintMemory();
        delete[] ComplexArray;
        poolMemoryManager.PrintMemory();
        ComplexArray = nullptr;
        std::cout << "=======================================\n\n";
        poolMemoryManager.resetPoolSize(5000);
        printf("Array De/Allocation of 4000 objects Test Time\n");
        allocationTime.start();
        ComplexArray = new Complex[4000];
        allocationTime.end();
        freeTime.start();
        delete[] ComplexArray;
        freeTime.end();
        std::cout << "Allocation Time = " << allocationTime.getTime() << allocationTime.getTimeUnitString() << std::endl;
        std::cout << "De-Allocation Time = " << freeTime.getTime() << freeTime.getTimeUnitString() << std::endl;
        return 0;
}