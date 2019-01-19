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
        printf("%s\n", e);
    }

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
    return 0;
}