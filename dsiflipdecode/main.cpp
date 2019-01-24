#include <cstdio>
#include "flipnote_parser.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("you forgot to specify a path");
        return 1;
    }
    dsiflipdecode::FlipnoteParser parser;
    try {
        parser.ParseFromFile(argv[1]);
    }
    catch (const char* e) {
        printf("ParseFromFile threw an exception: %s\n", e);
        return 1;
    }
    printf("parsed header\n");
    printf("decoding thumbnail...\n");

    // i could've just used the indexes from the original file, but i need to test this *somehow*...
    unsigned char* decoded_thumbnail = parser.DecodeThumbnail();
    bitmap_image converted_thumbnail(64, 48);

    // i wrote this code at 2 am and it's better than anything i will ever write when i'm fully awake
    // anyways thumbnails use 8x8 tiles for some reason
    int source_pixel = 0;
    rgb_t color;
    for (int tile_y = 0; tile_y < 6; ++tile_y) {
        for (int tile_x = 0; tile_x < 8; ++tile_x) {
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    color = {decoded_thumbnail[source_pixel * 4], decoded_thumbnail[source_pixel * 4 + 1], decoded_thumbnail[source_pixel * 4 + 2]};
                    converted_thumbnail.set_pixel(tile_x * 8 + x, tile_y * 8 + y, color);
                    ++source_pixel;
                }
            }
        }
    }
    converted_thumbnail.save_image("thumbnail.bmp");
    printf("saved to thumbnail.bmp\n");

    if (parser.flipnote_file.sound_header.bgm_size) {
        printf("decoding bgm audio...\n");
        std::pair<int16_t*, size_t> bgm = parser.DecodeBGM();
        FILE* decoded_bgm_file;
        int bgm_errcode = fopen_s(&decoded_bgm_file, "bgm.raw", "wb");
        if (bgm_errcode) {
            printf("failed to save bgm output\n");
            return 1;
        }
        fwrite(bgm.first, bgm.second, 1, decoded_bgm_file);
        printf("saved to bgm.raw\n");
    }

    if (parser.flipnote_file.sound_header.se1_size) {
        printf("decoding se1 audio...\n");
        std::pair<int16_t*, size_t> se1 = parser.DecodeSE1();
        FILE* decoded_se1_file;
        int se1_errcode = fopen_s(&decoded_se1_file, "se1.raw", "wb");
        if (se1_errcode) {
            printf("failed to save se1 output\n");
            return 1;
        }
        fwrite(se1.first, se1.second, 1, decoded_se1_file);
        printf("saved to se1.raw\n");
    }

    if (parser.flipnote_file.sound_header.se2_size) {
        printf("decoding se2 audio...\n");
        std::pair<int16_t*, size_t> se2 = parser.DecodeSE2();
        FILE* decoded_se2_file;
        int se2_errcode = fopen_s(&decoded_se2_file, "se2.raw", "wb");
        if (se2_errcode) {
            printf("failed to save se2 output\n");
            return 1;
        }
        fwrite(se2.first, se2.second, 1, decoded_se2_file);
        printf("saved to se2.raw\n");
    }

    if (parser.flipnote_file.sound_header.se3_size) {
        printf("decoding se3 audio...\n");
        std::pair<int16_t*, size_t> se3 = parser.DecodeSE3();
        FILE* decoded_se3_file;
        int se3_errcode = fopen_s(&decoded_se3_file, "se3.raw", "wb");
        if (se3_errcode) {
            printf("failed to save se3 output\n");
            return 1;
        }
        fwrite(se3.first, se3.second, 1, decoded_se3_file);
        printf("saved to se3.raw\n");
    }
    return 0;
}