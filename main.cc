#include <iostream>
#include <vector>
#include "timer.hpp"
#include "complex.hpp"

#include "PoolMemoryAllocator.hpp"
extern PoolMemoryAllocator<Complex> poolMemoryManager;

int main()
{
        // Complex *array[2000];
        // Timer allocationTime;
        // allocationTime.setTimeUnit(TimeUnit::ms);

        // std::cout << "Test#1\n";
        // allocationTime.start();
        // for (int i = 0; i < 5000; i++)
        // {
        //         for (int j = 0; j < 2000; j++)
        //                 array[j] = new Complex(i, j);
        //         for (int j = 0; j < 2000; j++)
        //                 delete array[j];
        // }
        // allocationTime.end();
        // std::cout << "Time#1 = " << allocationTime.getTime() << allocationTime.getTimeUnitString() << std::endl;

        std::cout << "Single Allocation TEST\n";
        poolMemoryManager.checkMemory();
        Complex *c1 = new Complex(1, 1);
        Complex *c2 = new Complex(1, 1);
        Complex *c3 = new Complex(1, 1);
        poolMemoryManager.checkMemory();

        delete c1;
        delete c2;
        poolMemoryManager.checkMemory();
        delete c3;
        c1 = c2 = c3 = nullptr;
        poolMemoryManager.checkMemory();

        std::cout << "ARRAY TEST(WR)\n";
        Complex *array2 = new Complex[4];
        printf("array addresses:\n");
        for (auto i = 0; i < 4; i++)
        {
                printf("array[%i] = %p\n",i,array2 + i);
        }
        for (auto i = 0; i < 4; i++)
        {
                array2[i].r = i;
                array2[i].c = i + 1;
                std::cout << "i = " << i << " => (r,c) = "
                          << "(" << array2[i].r << "," << array2[i].c << ")\n";
        }

        poolMemoryManager.checkMemory();
        delete[] array2;
        poolMemoryManager.checkMemory();
        array2 = nullptr;
        return 0;
}