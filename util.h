#ifndef UTIL_H
#define UTIL_H

#include <cstdint>

class Util {
public:
    Util();
    static
    uint64_t
    ulong_ceil(uint64_t a, uint64_t b) {
        if (a % b == 0)
            return a / b;
        else
            return (a / b) + 1;
    }
};
#endif
