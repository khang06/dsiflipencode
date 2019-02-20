#include "util.h"

// see util.h for credits
namespace Util {
    uint8_t ToHexNibble(char c1) {
        if (c1 >= 65 && c1 <= 70)
            return c1 - 55;
        if (c1 >= 97 && c1 <= 102)
            return c1 - 87;
        if (c1 >= 48 && c1 <= 57)
            return c1 - 48;
        return 0;
    }

    std::vector<uint8_t> HexStringToVector(std::string_view str, bool little_endian) {
        std::vector<uint8_t> out(str.size() / 2);
        if (little_endian) {
            for (std::size_t i = str.size() - 2; i <= str.size(); i -= 2)
                out[i / 2] = (ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]);
        }
        else {
            for (std::size_t i = 0; i < str.size(); i += 2)
                out[i / 2] = (ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]);
        }
        return out;
    }
}