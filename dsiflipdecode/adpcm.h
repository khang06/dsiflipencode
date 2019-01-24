#pragma once
#include <utility>
#include "util.h"

class AdpcmDecoder {
public:
    int16_t previous_sample;
    int16_t previous_index;

    int16_t DecodeSample(int16_t sample);
    std::pair<int16_t*, size_t> DecodeAdpcm(const char* src, size_t src_size);
};