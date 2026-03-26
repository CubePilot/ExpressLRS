/*
 * Adapted from ESP32 Arduino StreamString by Markus Sattler
 * Original: LGPL 2.1+
 *
 * StreamString shim for STM32duino — provides a String that is also a Stream.
 */
#ifndef STM32_STREAMSTRING_H_
#define STM32_STREAMSTRING_H_

#include <Stream.h>
#include <WString.h>

class StreamString : public Stream, public String {
public:
    size_t write(const uint8_t *data, size_t size) override {
        if (size && data) {
            const unsigned int newlen = len + size;
            if (reserve(newlen + 1)) {
                memcpy(buffer + len, data, size);
                len = newlen;
                *(buffer + newlen) = 0x00;
                return size;
            }
        }
        return 0;
    }

    size_t write(uint8_t data) override {
        return concat((char)data) ? 1 : 0;
    }

    int available() override {
        return length();
    }

    int read() override {
        if (length()) {
            char c = charAt(0);
            remove(0, 1);
            return c;
        }
        return -1;
    }

    int peek() override {
        if (length()) {
            return charAt(0);
        }
        return -1;
    }

    void flush() override {}

    void clear() {
        String::operator=(String(""));
    }
};

#endif // STM32_STREAMSTRING_H_
