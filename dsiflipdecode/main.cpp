#include <array>
#include <cstdio>
#include <deque>
#include <iostream>
#include <string>
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
void write_whatever_ptr(FILE* file, T good_variable_name, int size) {
    for (int i = 0; i < size; ++i) {
        char temp = reinterpret_cast<char*>(good_variable_name)[i];
        fputc(temp, file);
    }
}

void write_zeros(FILE* file, int count) {
    for (int i = 0; i < count; ++i) {
        fputc(0, file);
    }
}

int encode() {
    constexpr unsigned int frame_count = 6573; // hardcoding this because doing it properly is too much work
    constexpr bool use_bgm = 1;
    std::string directory = "data/badapplefull"; // only so i can concatenate
    constexpr char fsid[8] = { 0xFA, 0x70, 0xBC, 0xA0, 0x00, 0xEB, 0x73, 0x54 }; // what shows on my dsi, but in reverse
    constexpr char filename[18] = { 0xBC, 0x70, 0xFA, 0x31, 0x31, 0x32, 0x41, 0x38, 0x39, 0x36, 0x36, 0x42, 0x33, 0x37, 0x38, 0x39, 0x00, 0x00 }; // don't know how this is generated
    constexpr char partial_filename[8] = { 0xBC, 0x70, 0xFA, 0x11, 0x2A, 0x89, 0x66, 0xB3 }; // doesn't match above, even on real flipnotes
    const wchar_t* author = L"Khang";

    constexpr rgb_t rgb_white = { 255, 255, 255 };
    constexpr rgb_t rgb_black = { 0, 0, 0 };
    std::deque<bitmap_image>* input_frames = new std::deque<bitmap_image>; // only on heap so i can delete it
    std::vector<bitmap_image> two_color_frames;
    std::vector<std::vector<char>> frame_data;
    std::vector<char> ppm_file;
    std::vector<char> bgm;

    std::cout << "Stage 0: Load BGM, thumbnail, and frames" << std::endl;
    if (use_bgm) {
        // load the bgm, input should be already encoded to adpcm with ffmpeg
        std::string bgm_path = directory + "/audio.wav";
        FILE* bgm_file;
        int bgm_err = fopen_s(&bgm_file, bgm_path.c_str(), "rb");
        if (bgm_err)
            throw "Failed to open audio.wav!";
        fseek(bgm_file, 0, SEEK_END);
        size_t bgm_size = ftell(bgm_file) - 0x5E; // don't want the header
        rewind(bgm_file);
        fseek(bgm_file, 0x5E, SEEK_SET);
        for (size_t i = 0; i < bgm_size; ++i) {
            char byte;
            fread(&byte, 1, 1, bgm_file);
            //bgm.push_back((byte & 0x0F) << 4 | (byte & 0xF0) >> 4);
            bgm.push_back(byte);
        }
        fclose(bgm_file);
    }

    // load the thumbnail, expected size is 1536 or 0x600 bytes
    std::string thumbnail_path = directory + "/thumbnail.bin";
    FILE* thumbnail_file;
    int thumbnail_err = fopen_s(&thumbnail_file, thumbnail_path.c_str(), "rb");
    if (thumbnail_err)
        throw "Failed to open thumbnail.bin!";
    fseek(thumbnail_file, 0, SEEK_END);
    if (ftell(thumbnail_file) != 0x600)
        throw "thumbnail.bin isn't 0x600 bytes long!";
    rewind(thumbnail_file);

    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        std::string frame_format = directory + "/frame_%d.bmp";
        char formatted_filename[0x100]; // this should really be std::string
        snprintf(formatted_filename, 0x100, frame_format.c_str(), frame);
        bitmap_image input(formatted_filename);
        if (!input) {
            printf("could not open %s\n", formatted_filename);
            delete input_frames;
            return 1;
        }
        input_frames->push_back(input);
    }

    std::cout << "Stage 1: Convert frames to 2 color" << std::endl;
    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        const bitmap_image current_frame = input_frames->front();
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
        input_frames->pop_front();
    }
    delete input_frames;

    // time to actually start doing flipnote stuff
    // create the animation data first
    // only doing line type 0 (empty) and 1 (compressed bitmap)
    std::cout << "Stage 2: Encode frames" << std::endl;
    char previous_frame[256 * 192];
    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        char temp_frame[256 * 192];
        char frame_to_encode[256 * 192];
        std::vector<char> encoded_frame;
        bitmap_image current_frame = two_color_frames.at(frame);
        unsigned char current_line_encoding_flags[192] = {}; // only layer 1 for now,
        // layer 2 will be blank
        unsigned char packed_line_encoding_flags[48] = {};
        rgb_t pixel_color;
        // convert the frame to something simpler so it's easier to work with
        for (int y = 0; y < 192; ++y) {
            for (int x = 0; x < 256; ++x) {
                pixel_color = current_frame.get_pixel(x, y);
                if (pixel_color == rgb_black)
                    temp_frame[y * 256 + x] = 1;
                else
                    temp_frame[y * 256 + x] = 0;
            }
        }

        if (frame == 0) { // first frame, don't do diffing
            memcpy(frame_to_encode, temp_frame, 256 * 192);
            memcpy(previous_frame, temp_frame, 256 * 192);
        } else {
            for (int i = 0; i < 256 * 192; ++i) {
                frame_to_encode[i] = temp_frame[i] ^ previous_frame[i];
            }
            memcpy(previous_frame, temp_frame, 256 * 192);
        }

        // make the frame header
        char frame_header = 0;
        WRITETO(frame_header, 0, 1, 1); // white paper
        WRITETO(frame_header, 1, 1, 1); // layer 1 will be inverse of paper color
        WRITETO(frame_header, 4, 1, 1); // layer 2 will be red, not that it really matters
        //if (frame == 0)
            WRITETO(frame_header, 7, 1, 1); // new frame
        encoded_frame.push_back(frame_header);

        int line_type;
        int byte_index;
        int bit_index;
        // check if lines are blank or not, and write to line_encoding_flags
        // accordingly
        for (unsigned int line = 0; line < 192; ++line) {
            line_type = 0; // empty
            for (unsigned int pixel = 0; pixel < 256; ++pixel) {
                if (frame_to_encode[line * 256 + pixel] == 1) {
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
            bit_index = line % 4;
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

        // time to encode the frame!
        for (unsigned int line = 0; line < 192; ++line) {
            switch (current_line_encoding_flags[line]) {
            case 0: // empty
                continue;
            case 1: // compressed bitmap
                std::vector<char> chunks;
                char used_chunks[32] = {};
                unsigned int chunk_flag = 0;
                unsigned int bit_index = 0; // 0 to 31

                // first we need to figure out what chunks are used and write the chunk flag
                for (unsigned int chunk = 0; chunk < 32; ++chunk) {
                    for (unsigned int pixel = 0; pixel < 8; ++pixel) {
                        if (frame_to_encode[line * 256 + (chunk * 8 + pixel)]) {
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
                        if (frame_to_encode[line * 256 + (chunk * 8 + pixel)]) {
                            WRITETO(encoded_chunk, pixel, 1, 1);
                        }
                    }
                    encoded_frame.push_back(encoded_chunk);
                }
            }
        }

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
    std::cout << "Stage 3: Write PPM to file" << std::endl;
    FILE* asdf;
    dsiflipdecode::FileHeader header;
    header.magic = 0x41524150; // PARA endian-flipped
    header.anim_data_size = anim_data_size + 8 + frame_count * 4;
    if (use_bgm)
        header.sound_data_size = bgm.size();
    else
        header.sound_data_size = 0;
    header.frame_count = frame_count - 1;
    header.unk1 = 0x24;
    header.locked = 1;
    header.thumbnail_frame_index = 0;
    memset(header.root_author_name, 0, 22);
    memset(header.parent_author_name, 0, 22);
    memset(header.current_author_name, 0, 22);
    memcpy(header.root_author_name, author, wcslen(author) * 2 - 1); // not using wcs whatever because we don't want that null terminator
    memcpy(header.parent_author_name, author, wcslen(author) * 2 - 1);
    memcpy(header.current_author_name, author, wcslen(author) * 2 - 1);
    memcpy(header.parent_author_id, fsid, 8);
    memcpy(header.current_author_id, fsid, 8);
    memcpy(header.root_author_id, fsid, 8);
    memcpy(header.parent_filename, filename, 18);
    memcpy(header.current_filename, filename, 18);
    memcpy(header.partial_filename, partial_filename, 8);
    header.timestamp = 602785704;
    header.unk2 = 0;

    dsiflipdecode::AnimationSectionHeader anim_header;
    anim_header.frame_offset_table_size = frame_count * 4;
    anim_header.unk1 = 0;
    anim_header.flags = 0x410000; // no idea

    dsiflipdecode::SoundSectionHeader sound_header;
    if (use_bgm)
        sound_header.bgm_size = bgm.size();
    else
        sound_header.bgm_size = 0;
    sound_header.se1_size = 0;
    sound_header.se2_size = 0;
    sound_header.se3_size = 0;
    sound_header.frame_speed = 0;
    sound_header.bgm_frame_speed = 0;
    sound_header.encoded = 1;
    memset(sound_header.pad45, 0, 13);

    int aghhh = fopen_s(&asdf, "AC70FA_112A8966B3789_000.ppm", "wb");
    if (aghhh) {
        printf("%d", aghhh);
        throw "WHYYYYYY";
    }
    // write ppm header
    write_whatever_ptr(asdf, &header, sizeof(header));
    // write thumbnail
    for (int i = 0; i < 0x600; ++i) {
        char byte;
        fread(&byte, 1, 1, thumbnail_file);
        fputc(byte, asdf);
    }
    // write animation section header
    write_whatever_ptr(asdf, &anim_header, sizeof(anim_header));
    // write frame offset table
    for (int i = 0; i < frame_offset_table.size(); ++i) {
        write_whatever_ptr(asdf, &frame_offset_table.at(i), 4);
    }
    // write frame data
    for (int frame = 0; frame < frame_data.size(); ++frame) {
        write_whatever_ptr(asdf, frame_data.at(frame).data(), frame_data.at(frame).size());
    }
    // write sound effect usage flags
    write_zeros(asdf, frame_count);
    // needs to be aligned to 4 bytes now
    int align_size = 4 - ((0x6A0 + header.anim_data_size + frame_count) % 4);
    if (align_size != 4)
        write_zeros(asdf, align_size);
    // write sound header
    write_whatever_ptr(asdf, &sound_header, sizeof(sound_header));
    // write bgm (no sound effects for now...)
    if (use_bgm)
        write_whatever_ptr(asdf, bgm.data(), bgm.size());
    // write dummy signature
    write_zeros(asdf, 0x80);
    // write padding
    write_zeros(asdf, 0x10);
    fclose(asdf);
    return 0;
}

int main(int argc, char** argv) {
    try {
        encode();
        return 0;
    }
    catch(std::string e) {
        std::cout << e << std::endl;
        return 1;
    }
}