#pragma once
#include <cstdint>

/* From the dolphin-emu project */
/// Textually concatenates two tokens. The double-expansion is required by the C preprocessor.
#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x##y

// helper macro to properly align structure members.
// Calling INSERT_PADDING_BYTES will add a new member variable with a name like "pad121",
// depending on the current source line to make sure variable names are unique.
#define INSERT_PADDING_BYTES(num_bytes) uint8_t CONCAT2(pad, __LINE__)[(num_bytes)];
#define INSERT_PADDING_WORDS(num_words) uint32_t CONCAT2(pad, __LINE__)[(num_words)];

template <typename T>
T clamp(T num, T min, T max) {
    if (num < min) {
        return min;
    }
    if (num > max) {
        return max;
    }
    return num;
}

// https://stackoverflow.com/questions/11815894/how-to-read-write-arbitrary-bits-in-c-c
#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
#define READFROM(data, index, size) \
    (((data)&GETMASK((index), (size))) >> (index))
#define WRITETO(data, index, size, value) \
    ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))