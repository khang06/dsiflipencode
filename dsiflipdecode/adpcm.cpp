#include <utility>
#include <vector>
#include "util.h"
#include "adpcm.h"

// based on https://github.com/jaames/flipnote.js/blob/master/src/utils/adpcm.js, which is based on http://www.cs.columbia.edu/~gskc/Code/AdvancedInternetServices/SoundNoiseRatio/dvi_adpcm.c
// by "based" i mean i copied as much javascript as i could so i didn't have to put effort
constexpr int16_t index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

constexpr int16_t step_size_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

int16_t AdpcmDecoder::DecodeSample(int16_t sample) {
    int predicted_sample = previous_sample;
    int index = previous_index;
    int step = step_size_table[index];
    int vpdiff = step >> 3;

    if (sample & 4)
        vpdiff += step;
    if (sample & 2)
        vpdiff += (step >> 1);
    if (sample & 1)
        vpdiff += (step >> 2);

    predicted_sample += (sample & 8) ? -vpdiff : vpdiff;

    index += index_table[sample];
    index = clamp(index, 0, 88);

    predicted_sample = clamp(predicted_sample, -32767, 32767);
    previous_sample = predicted_sample;
    previous_index = index;
    return predicted_sample;
}

// decodes *nibble-flipped* 4-bit adpcm to 16-bit pcm
std::pair<int16_t*, size_t> AdpcmDecoder::DecodeAdpcm(const char* src, size_t src_size) {
    previous_sample = 0;
    previous_index = 0;
    int16_t* output = new int16_t[src_size * 2];
    int output_offset = 0;
    for (int i = 0; i < src_size; ++i) {
        output[output_offset] = DecodeSample(src[i] & 0xF);
        output[output_offset + 1] = DecodeSample((src[i] >> 4) & 0xF);
        output_offset += 2;
    }
    return std::make_pair(output, src_size * 2 * 2);
}