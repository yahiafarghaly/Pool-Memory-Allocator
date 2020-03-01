
#pragma once

#include <sys/types.h>

class Complex
{
public:
        Complex()
        {
                this->r = 0xff;
                this->c = 0xff;
        }
        Complex(double a, double b) : r(a), c(b) {}
        Complex add(const Complex &c2)
        {
                auto r = this->r + c2.r;
                auto c = this->c + c2.c;
                return Complex(r, c);
        }
#ifdef OVERLOADED_MEMORY
        void *operator new(size_t size);
        void *operator new[](size_t size);
        void operator delete(void *pointerToDelete);
        void operator delete[](void *arrayToDelete);
#endif
public:
        double r; // Real Part
        double c; // Complex Part
};