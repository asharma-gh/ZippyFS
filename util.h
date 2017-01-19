#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <sys/time.h>

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

    static
    unsigned long long
    get_time() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return (unsigned long long)(tv.tv_sec) * 1000
               + (unsigned long long)(tv.tv_usec) / 1000;
    }
};
#endif
