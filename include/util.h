#pragma once
#include <cstdint>
#include <vector>
#include <codecvt>
#include <locale>
#include <string>

// most of these functions are from yuzu/citra/dolphin
/// Textually concatenates two tokens. The double-expansion is required by the C preprocessor.
#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x##y

// helper macro to properly align structure members.
// Calling INSERT_PADDING_BYTES will add a new member variable with a name like "pad121",
// depending on the current source line to make sure variable names are unique.
#define INSERT_PADDING_BYTES(num_bytes) uint8_t CONCAT2(pad, __LINE__)[(num_bytes)];

namespace Util {
    uint8_t ToHexNibble(char c1);
    std::vector<uint8_t> HexStringToVector(std::string_view str, bool little_endian);

    // https://stackoverflow.com/questions/105252/how-do-i-convert-between-big-endian-and-little-endian-values-in-c
    template <typename T>
    T SwapEndian(T u)
    {
        static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

        union
        {
            T u;
            unsigned char u8[sizeof(T)];
        } source, dest;

        source.u = u;

        for (size_t k = 0; k < sizeof(T); k++)
            dest.u8[k] = source.u8[sizeof(T) - k - 1];

        return dest.u;
    }
}

// https://stackoverflow.com/questions/11815894/how-to-read-write-arbitrary-bits-in-c-c
#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data)&GETMASK((index), (size))) >> (index))
#define WRITETO(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))