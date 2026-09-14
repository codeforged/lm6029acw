#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

class Print {
public:
    Print() {}
    virtual ~Print() {}

    // Pure virtual method yang harus diimplementasikan oleh Adafruit_GFX
    virtual size_t write(uint8_t c) = 0;

    size_t write(const char *str) {
        if (str == NULL) return 0;
        return write((const uint8_t *)str, strlen(str));
    }

    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            if (write(*buffer++)) n++;
            else break;
        }
        return n;
    }

    size_t print(const char str[]) {
        return write(str);
    }

    size_t print(char c) {
        return write((uint8_t)c);
    }

    size_t print(int n) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", n);
        return write(buf);
    }
};

#endif
