#include "util.h"
#include "flipnote_parser.h"

namespace dsiflipdecode {
    void FlipnoteParser::ParseFromFile(const char* path) {
        // i could use a buffer but i'm too lazy
        // todo: use macros so this code looks less shitty
        FILE* input;
        int err_code = fopen_s(&input, path, "rb");
        if (err_code)
            throw "Could not open input file!";

        // read the main header
        fread_s(reinterpret_cast<void*>(&flipnote_file.header), 0xA0, 0xA0, 1, input);
        if (flipnote_file.header.magic != 0x41524150) // PARA endian-flipped
            throw "Not a Flipnote Studio PPM file!";

        // read the thumbnail 
        flipnote_file.thumbnail = new char[0x600]; // no compression is used on thumbnails, so they're always the same size
        fread_s(reinterpret_cast<void*>(flipnote_file.thumbnail), 0x600, 0x600, 1, input);

        // read the animation header, frame offset table, and data
        fread_s(reinterpret_cast<void*>(&flipnote_file.anim_header), 8, 8, 1, input);
        if (flipnote_file.anim_header.frame_offset_table_size % 4 != 0)
            throw "Frame offset table size is not a multiple of 4!";
        flipnote_file.frame_offset_table = new uint32_t[flipnote_file.anim_header.frame_offset_table_size / 4];
        fread_s(reinterpret_cast<void*>(flipnote_file.frame_offset_table), flipnote_file.anim_header.frame_offset_table_size, flipnote_file.anim_header.frame_offset_table_size, 1, input);
        // the subtraction for this array is because the sound effect flags are stored at 0x6A0 + anim_data_size
        // if i didn't have this subtraction, the flags would overlap with the animation data, which wouldn't make any sense
        // actually it still doesn't make sense but it works
        uint32_t actual_anim_data_size = flipnote_file.header.anim_data_size - 8 - flipnote_file.anim_header.frame_offset_table_size;
        flipnote_file.anim_data = new char[actual_anim_data_size];
        fread_s(reinterpret_cast<void*>(flipnote_file.anim_data), actual_anim_data_size, actual_anim_data_size, 1, input);

        // read sound effect flags, sound header, and data
        flipnote_file.sound_effect_flags = new char[flipnote_file.header.frame_count];
        fread_s(reinterpret_cast<void*>(flipnote_file.sound_effect_flags), flipnote_file.header.frame_count, flipnote_file.header.frame_count, 1, input);
        // everything past this point must be aligned for some reason
        if (ftell(input) % 4 != 0)
            fseek(input, 4 - (ftell(input) % 4), SEEK_CUR);
        fread_s(reinterpret_cast<void*>(&flipnote_file.sound_header), 0x20, 0x20, 1, input);
        flipnote_file.bgm_data = new char[flipnote_file.sound_header.bgm_size];
        fread_s(reinterpret_cast<void*>(flipnote_file.bgm_data), flipnote_file.sound_header.bgm_size, flipnote_file.sound_header.bgm_size, 1, input);
        flipnote_file.se1_data = new char[flipnote_file.sound_header.se1_size];
        fread_s(reinterpret_cast<void*>(flipnote_file.se1_data), flipnote_file.sound_header.se1_size, flipnote_file.sound_header.se1_size, 1, input);
        flipnote_file.se2_data = new char[flipnote_file.sound_header.se2_size];
        fread_s(reinterpret_cast<void*>(flipnote_file.se2_data), flipnote_file.sound_header.se2_size, flipnote_file.sound_header.se2_size, 1, input);
        flipnote_file.se3_data = new char[flipnote_file.sound_header.se3_size];
        fread_s(reinterpret_cast<void*>(flipnote_file.se3_data), flipnote_file.sound_header.se3_size, flipnote_file.sound_header.se3_size, 1, input);
    }

    // returns char array, 0xRRGGBBAA
    unsigned char* FlipnoteParser::DecodeThumbnail() {
        unsigned char* out = new unsigned char[0x600 * 2 * 4]; // multiply by 2 because thumbnail color values are stored as 4-bit values and not bytes, and multiply by 4 because we need to store as argb
        unsigned char color;
        for (int i = 0; i < 0x600; ++i) {
            color = flipnote_file.thumbnail[i] & 0x0F; // low nibble
            out[i * 8 + 3] = 0xFF; // always opaque
            out[i * 8 + 2] = thumbnail_color_map[color].blue;
            out[i * 8 + 1] = thumbnail_color_map[color].green;
            out[i * 8] = thumbnail_color_map[color].red;
            color = (flipnote_file.thumbnail[i] & 0xF0) >> 4; // high nibble
            out[i * 8 + 7] = 0xFF;
            out[i * 8 + 6] = thumbnail_color_map[color].blue;
            out[i * 8 + 5] = thumbnail_color_map[color].green;
            out[i * 8 + 4] = thumbnail_color_map[color].red;
        }
        return out;
    }
}