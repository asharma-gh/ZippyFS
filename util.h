#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/syscall.h>
#include <string>
#include <sys/time.h>
#include <linux/random.h>

class Util {
  public:

    Util();

    /**
     * @param a is the divisor
     * @param b is the dividend
     * @return ceil(a / b)
     */
    static
    uint64_t
    ulong_ceil(uint64_t a, uint64_t b) {
        if (a % b == 0)
            return a / b;
        else
            return (a / b) + 1;
    }

    /**
     * @return the current time in milliseconds
     */
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
                mask = -(crc&1) ^ special_bits;
                crc = crc ^ mask;
                crc = crc >> 1;
            }
        }
        return crc;
    }


    /**
     * verifies the checksum specified in contents
     * assumes that contents is in the form of an .idx file
     * @param contents is the contents to verify, with checksum appendde at end
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

    std::string
    generate_rand_hex_name() {
        // create archive name
        char num[16] = {'\0'};
        char hex_name[33] = {'\0'};
        syscall(SYS_getrandom, num, sizeof(num), GRND_NONBLOCK);

        for (int i = 0; i < 16; i++) {
            sprintf(hex_name + i*2,"%02X", num[i]);
        }
        return std::string(hex_name);
    }

};
#endif
