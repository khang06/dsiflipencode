#pragma once
#include <utility>
#include "util.h"
#include "flipnote_file.h"
#include "bitmap_image.hpp"

namespace dsiflipdecode {
    class FlipnoteParser {
    public:
        FlipnoteFile flipnote_file;
        static constexpr rgb_t thumbnail_color_map[16] = {
            {255,255,255}, {82,82,82}, {255,255,255}, {156,156,156},
            {255,72,68}, {200,81,79}, {255,173,172}, {0,255,0},
            {72,64,255}, {81,79,184}, {173,171,255}, {0,255,0},
            {182,87,183}, {0,255,0}, {0,255,0}, {0,255,0}
        };

        void ParseFromFile(const char* path);
        std::pair<char*, size_t> DecodeFrame(uint32_t index);
        unsigned char* DecodeThumbnail();
        std::pair<int16_t*, size_t> DecodeADPCM(const char* src, size_t src_size);
        std::pair<int16_t*, size_t> DecodeBGM();
        std::pair<int16_t*, size_t> DecodeSE1();
        std::pair<int16_t*, size_t> DecodeSE2();
        std::pair<int16_t*, size_t> DecodeSE3();
    };
}