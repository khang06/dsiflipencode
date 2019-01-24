#include "util.h"
#include "flipnote_parser.h"
#include "adpcm.h"

namespace dsiflipdecode {
    // Decodes 4-bit nibble-flipped mono IMA-ADPCM at 8khz to raw 16-bit PCM
    std::pair<int16_t*, size_t> FlipnoteParser::DecodeADPCM(const char* src, size_t src_size) {
        AdpcmDecoder decoder;
        return decoder.DecodeAdpcm(src, src_size);
    }

    std::pair<int16_t*, size_t> FlipnoteParser::DecodeBGM() {
        return DecodeADPCM(flipnote_file.bgm_data, flipnote_file.sound_header.bgm_size);
    }

    std::pair<int16_t*, size_t> FlipnoteParser::DecodeSE1() {
        return DecodeADPCM(flipnote_file.se1_data, flipnote_file.sound_header.se1_size);
    }

    std::pair<int16_t*, size_t> FlipnoteParser::DecodeSE2() {
        return DecodeADPCM(flipnote_file.se2_data, flipnote_file.sound_header.se2_size);
    }

    std::pair<int16_t*, size_t> FlipnoteParser::DecodeSE3() {
        return DecodeADPCM(flipnote_file.se3_data, flipnote_file.sound_header.se3_size);
    }
}