#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
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

    /**
     * lazy implementation of crc64 without a table
     * - iterates thru each bit and applies a mask
     * @param msg is the contents of the index file
     * @return the encoded message
     */
    static
    uint64_t
    crc64(const char* message) {
        uint64_t crc = 0xFFFFFFFFFFFFFFFF;
        uint64_t special_bits = 0xDEADBEEFABCD1234;
        uint64_t mask = 0;

        for(int i = 0; message[i]; i++) {
            crc ^= (uint8_t)message[i];
            for (int j = 0; j < 7; j++) {
                // I found that if I don't change the mask in respect to the last bit in crc
                // there are inputs such as "abc123" and "123abc" which will have the same hash
                mask = -(crc&1) ^ special_bits; // mask = 0xbits0 or 0xbits1 depending on crc
                crc = crc ^ mask;
                crc = crc >> 1;
            }
        }
        return crc;
    }


    /**
     * verifies the checksum specified in contents
     * assumes that contents is in the form of an .idx file
     * @return 0 if the checksum is valid, -1 otherwise
     */
    static
    int
    verify_checksum(const char* contents) {
        char contents_cpy[strlen(contents)];
        strcpy(contents_cpy, contents);
        char delim[10];
        strcpy(delim, "CHECKSUM");
        char* checksum;

        if ((checksum = ::strstr(contents_cpy, delim)) == NULL)
            return -1;

        char checksum_cpy[strlen(checksum)];
        strcpy(checksum_cpy, checksum);
        checksum[0] = '\0';
        uint64_t checksum_val;
        char* endptr;
        checksum_val = strtoull(checksum_cpy + 8, &endptr, 10);
        // make new checksum
        uint64_t new_checksum = crc64(contents_cpy);
        if (new_checksum != checksum_val)
            return -1;

        return 0;
    }

};
#endif
