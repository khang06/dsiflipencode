#include <array>
#include <cstdio>
#include <intrin.h>
#include "flipnote_parser.h"

// https://stackoverflow.com/questions/11815894/how-to-read-write-arbitrary-bits-in-c-c
#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
#define READFROM(data, index, size) \
    (((data)&GETMASK((index), (size))) >> (index))
#define WRITETO(data, index, size, value) \
    ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))

// this code sucks. this code sucks. this code sucks. this code sucks.
// this.
// code.
// sucks.
template <class T>
void write_whatever(FILE* file, T good_variable_name, int size) {
    for (int i = 0; i < size; ++i) {
        char temp = reinterpret_cast<char*>(&good_variable_name)[i];
        fwrite(&temp, 1, 1, file);
    }
}

template <class T>
void write_whatever_ptr(FILE* file, T good_variable_name, int size) {
    for (int i = 0; i < size; ++i) {
        char temp = reinterpret_cast<char*>(good_variable_name)[i];
        fwrite(&temp, 1, 1, file);
    }
}

void write_zeros(FILE* file, int count) {
    for (int i = 0; i < count; ++i) {
        fputc(0, file);
    }
}

int main(int argc, char** argv) {
    constexpr rgb_t rgb_white = { 255, 255, 255 };
    constexpr rgb_t rgb_black = { 0, 0, 0 };
    std::vector<bitmap_image> input_frames;
    std::vector<bitmap_image> two_color_frames;
    unsigned int frame_count = 2; // hardcoding this because doing it properly is too much work
    std::vector<std::vector<char>> frame_data;
    std::vector<char> ppm_file;

    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        char formatted_filename[0x100]; // nobody will ever get past this limit...
        snprintf(formatted_filename, 0x100, "frames/frame%d.bmp", frame);
        bitmap_image input(formatted_filename);
        if (!input) {
            printf("could not open %s\n", formatted_filename);
            return 1;
        }
        input_frames.push_back(input);
    }

    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        const bitmap_image current_frame = input_frames.at(frame);
        const unsigned int height = current_frame.height();
        const unsigned int width = current_frame.width();
        bitmap_image output(width, height);

        // convert the image to 2 colors
        unsigned int red_amount;
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                // only reading the red channel since the expected input would be
                // greyscale
                red_amount = current_frame.get_pixel(x, y).red;
                if (red_amount > 127) {
                    output.set_pixel(x, y, rgb_white);
                }
                else {
                    output.set_pixel(x, y, rgb_black);
                }
            }
        }
        two_color_frames.push_back(output);
    }

    // debug purposes
    size_t output_frame_count = two_color_frames.size();
    for (size_t i = 0; i < output_frame_count; ++i) {
        char formatted_filename[0x100];
        snprintf(formatted_filename, 0x100, "output_twocolor%zd.bmp", i);
        two_color_frames.at(i).save_image(formatted_filename);
    }

    // time to actually start doing flipnote stuff
    // create the animation data first
    // only doing line type 0 (empty) and 1 (compressed bitmap)
    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        std::vector<char> encoded_frame;
        bitmap_image current_frame = two_color_frames.at(frame);
        unsigned char current_line_encoding_flags[192] = {}; // only layer 1 for now,
        // layer 2 will be blank
        unsigned char packed_line_encoding_flags[48] = {};

        // first, the frame header
        char frame_header = 0;
        WRITETO(frame_header, 0, 1, 1); // white paper
        WRITETO(frame_header, 1, 1, 1); // layer 1 will be inverse of paper color
        WRITETO(frame_header, 3, 1, 1); // layer 2 too, but it won't matter
        WRITETO(frame_header, 7, 1, 1); // always new frame (don't feel like doing diffing between frames)
        encoded_frame.push_back(frame_header);

        int line_type;
        rgb_t pixel_color;
        int byte_index;
        int bit_index;
        // check if lines are blank or not, and write to line_encoding_flags
        // accordingly
        for (unsigned int line = 0; line < 192; ++line) {
            line_type = 0; // empty
            for (unsigned int pixel = 0; pixel < 256; ++pixel) {
                pixel_color = current_frame.get_pixel(pixel, line);
                if (pixel_color == rgb_black) { // TODO: make this dynamic once i add
                    // most-efficient paper color stuff
                    // line_type = 3; // decompressed bitmap
                    line_type = 1; // compressed bitmap
                    break;
                }
            }
            current_line_encoding_flags[line] = line_type;
        }
        // pack line_encoding_flags
        // this could probably be merged into the above loop
        for (int line = 0; line < 192; ++line) {
            byte_index = line / 4; // 4 2-bit pixels per byte
            bit_index = line % 4; // kind of a misleading name
            switch (current_line_encoding_flags[line]) {
            case 0:
                packed_line_encoding_flags[byte_index] &= ~(1UL << (bit_index * 2 + 1));
                packed_line_encoding_flags[byte_index] &= ~(1UL << (bit_index * 2));
                //WRITETO(packed_line_encoding_flags[byte_index], bit_index, 2, 0);
                break;
            case 1:
                packed_line_encoding_flags[byte_index] &= ~(1UL << (bit_index * 2 + 1));
                packed_line_encoding_flags[byte_index] |= 1UL << (bit_index * 2);
                //WRITETO(packed_line_encoding_flags[byte_index], bit_index, 2, 1);
                break;
            case 2:
                packed_line_encoding_flags[byte_index] |= 1UL << (bit_index * 2 + 1);
                packed_line_encoding_flags[byte_index] &= ~(1UL << (bit_index * 2));
                //WRITETO(packed_line_encoding_flags[byte_index], bit_index, 2, 2);
                break;
            case 3:
                packed_line_encoding_flags[byte_index] |= 1UL << (bit_index * 2 + 1);
                packed_line_encoding_flags[byte_index] |= 1UL << (bit_index * 2);
                //WRITETO(packed_line_encoding_flags[byte_index], bit_index, 2, 3);
                break;
            default:
                printf("wtf\n");
            }
        }
        for (int i = 0; i < 48; ++i) {
            encoded_frame.push_back(packed_line_encoding_flags[i]);
        }
        for (int i = 0; i < 48; ++i) {
            encoded_frame.push_back(0); // layer 2 is blank
        }

        // debug stuff
        FILE* line_encoding_flags_file;
        int file_ret = fopen_s(&line_encoding_flags_file, "line_encoding_debug.bin", "wb");
        if (file_ret) {
            printf("could not save line_encoding_debug.bin: %d\n", file_ret);
        }
        fwrite(packed_line_encoding_flags, 48, 1, line_encoding_flags_file);
        fclose(line_encoding_flags_file);

        // time to encode the frame!
        for (unsigned int line = 0; line < 192; ++line) {
            switch (current_line_encoding_flags[line]) {
            case 0: // empty
                continue;
            case 1: // compressed bitmap
                std::vector<char> chunks;
                char used_chunks[32] = {};
                char bmp_line[256];
                unsigned int chunk_flag = 0;
                unsigned int bit_index = 0; // 0 to 31

                // convert the line to something more simple first
                rgb_t pixel_color;
                for (unsigned int pixel = 0; pixel < 256; ++pixel) {
                    pixel_color = current_frame.get_pixel(pixel, line);
                    pixel_color == rgb_white ? bmp_line[pixel] = 0 : bmp_line[pixel] = 1;
                }

                // first we need to figure out what chunks are used and write the chunk flag
                for (unsigned int chunk = 0; chunk < 32; ++chunk) {
                    for (unsigned int pixel = 0; pixel < 8; ++pixel) {
                        if (bmp_line[chunk * 8 + pixel]) {
                            used_chunks[chunk] = 1;
                            WRITETO(chunk_flag, 31 - chunk, 1, 1);
                            break;
                        }
                    }
                }
                // chunk_flag needs to be endian-flipped because of the way i'm handling it (and this is expected to run on little-endian systems)
                chunk_flag = _byteswap_ulong(chunk_flag);
                for (int i = 0; i < 4; ++i) {
                    encoded_frame.push_back(reinterpret_cast<char*>(&chunk_flag)[i]); // my code just gets worse and worse
                }

                // then we make the chunks
                // could probably be merged with above loop
                for (unsigned int chunk = 0; chunk < 32; ++chunk) {
                    if (!used_chunks[chunk])
                        continue;
                    char encoded_chunk = 0;
                    for (unsigned int pixel = 0; pixel < 8; ++pixel) {
                        if (bmp_line[chunk * 8 + pixel]) {
                            WRITETO(encoded_chunk, pixel, 1, 1);
                        }
                    }
                    encoded_frame.push_back(encoded_chunk);
                }
            }
        }
        FILE* why;
        int who_cares_if_fopen_returns_null = fopen_s(&why, "frame_enc.bin", "wb");
        if (who_cares_if_fopen_returns_null)
            throw "FUCK";
        fwrite(encoded_frame.data(), encoded_frame.size(), 1, why);
        fclose(why);

        frame_data.push_back(encoded_frame);
    }

    int anim_data_size = 0;
    for (int frame = 0; frame < frame_data.size(); ++frame) {
        anim_data_size += frame_data.at(frame).size();
    }

    std::vector<uint32_t> frame_offset_table;
    uint32_t current_offset = 0;
    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        frame_offset_table.push_back(current_offset);
        current_offset += frame_data.at(frame).size();
    }
    // sfadfsdffsffasfasdfasdf
    FILE* asdf;
    dsiflipdecode::FileHeader header;
    header.magic = 0x41524150; // PARA endian-flipped
    header.anim_data_size = anim_data_size + 8 + frame_count * 4;
    header.sound_data_size = 0;
    header.frame_count = frame_count - 1;
    header.unk1 = 0x24;
    header.locked = 1;
    header.thumbnail_frame_index = 0;
    memset(header.root_author_name, 0, 22);
    memset(header.parent_author_name, 0, 22);
    memset(header.current_author_name, 0, 22);
    memcpy(header.root_author_name, L"Khangaroo", 18); // not using wcs whatever because we don't want that null terminator
    memcpy(header.parent_author_name, L"Khangaroo", 18);
    memcpy(header.current_author_name, L"Khangaroo", 18);
    memcpy(header.parent_author_id, "AAAAAAAA", 8);
    memcpy(header.current_author_id, "AAAAAAAA", 8);
    memcpy(header.root_author_id, "AAAAAAAA", 8);
    memcpy(header.parent_filename, "AAAAAAAAAAAAAAAAAA", 18);
    memcpy(header.current_filename, "AAAAAAAAAAAAAAAAAA", 18);
    memcpy(header.partial_filename, "AAAAAAAA", 8);
    header.timestamp = 0x1337666;
    header.unk2 = 0;
    dsiflipdecode::AnimationSectionHeader anim_header;
    anim_header.frame_offset_table_size = frame_count * 4;
    anim_header.unk1 = 0;
    anim_header.flags = 0;
    dsiflipdecode::SoundSectionHeader sound_header;
    sound_header.bgm_size = 0;
    sound_header.se1_size = 0;
    sound_header.se2_size = 0;
    sound_header.se3_size = 0;
    sound_header.frame_speed = 1; // 0.5 fps
    sound_header.bgm_frame_speed = 1;
    memset(sound_header.pad44, 0, 14);

    int aghhh = fopen_s(&asdf, "finalasdf.ppm", "wb");
    if (aghhh) {
        printf("%d", aghhh);
        throw "WHYYYYYY";
    }
    // write ppm header
    write_whatever(asdf, header, sizeof(header));
    // write thumbnail (blank for now)
    write_zeros(asdf, 0x600);
    // write animation section header
    write_whatever(asdf, anim_header, sizeof(anim_header));
    // write frame offset table
    for (int i = 0; i < frame_offset_table.size(); ++i) {
        write_whatever(asdf, frame_offset_table.at(i), 4);
    }
    // write frame data
    for (int frame = 0; frame < frame_data.size(); ++frame) {
        write_whatever_ptr(asdf, frame_data.at(frame).data(), frame_data.at(frame).size());
    }
    // write sound effect usage flags
    write_zeros(asdf, frame_count);
    // needs to be aligned to 4 bytes now
    int align_size = 4 - ((0x6A0 + header.anim_data_size + frame_count) % 4);
    write_zeros(asdf, align_size);
    // write sound header
    write_whatever(asdf, sound_header, sizeof(sound_header));
    // no sound data for now...
    // write dummy signature
    write_zeros(asdf, 0x80);
    // write padding
    write_zeros(asdf, 0x10);
    fclose(asdf);
    return 0;
}