// implementation is from libavcodec
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "adpcm_encoder.h"

uint8_t AdpcmEncoder::EncodeSample(int16_t sample) {
    int32_t delta = sample - prev_sample;
    int32_t enc_sample = 0;

    if (delta < 0) {
        enc_sample = 8;
        delta = -delta;
    }
    enc_sample += std::min(7, delta * 4 / ff_adpcm_step_table[step_index]);
    prev_sample = sample;
    step_index = std::clamp(step_index + ff_adpcm_index_table[enc_sample], 0, 79);
    return enc_sample;
}