#include <array>
#include <cstdio>
#include <deque>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include "flipnote_file.h"
#include "INIReader.h"
#include "lodepng.h"
#include "util.h"

// this code sucks. this code sucks. this code sucks. this code sucks.
// this.
// code.
// sucks.
// but it sucks a bit less now :)

template <class T>
void write_whatever_ptr(FILE* file, T good_variable_name, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        char temp = reinterpret_cast<char*>(good_variable_name)[i];
        fputc(temp, file);
    }
}

void write_zeros(FILE* file, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        fputc(0, file);
    }
}

class EncoderSettings {
public:
    std::string data_dir;
    std::array<uint8_t, 8> fsid;
    std::array<uint8_t, 8> filename;
    std::array<uint8_t, 8> partial_filename;
    std::wstring author;
    bool use_bgm;
    uint32_t timestamp;
};

int encode(EncoderSettings settings) {
    std::vector<std::vector<char>> frame_data;
    std::vector<char> bgm;
    unsigned int frame_count = 0;

    if (settings.use_bgm) {
    std::cout << "Loading BGM..." << std::endl;
        // load the bgm, input should be already encoded to adpcm with ffmpeg
        std::string bgm_path = settings.data_dir + "/audio.wav";
        FILE* bgm_file = fopen(bgm_path.c_str(), "rb");
        if (!bgm_file)
            throw "Failed to open audio.wav!";
        fseek(bgm_file, 0, SEEK_END);
        size_t bgm_size = ftell(bgm_file) - 0x5E; // don't want the header
        if (bgm_size > UINT32_MAX)
            throw "BGM is too big!";
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
    std::cout << "Loading thumbnail..." << std::endl;
    std::string thumbnail_path = settings.data_dir + "/thumbnail.bin";
    FILE* thumbnail_file = fopen(thumbnail_path.c_str(), "rb");
    if (!thumbnail_file)
        throw "Failed to open thumbnail.bin!";
    fseek(thumbnail_file, 0, SEEK_END);
    if (ftell(thumbnail_file) != 0x600)
        throw "thumbnail.bin isn't 0x600 bytes long!";
    rewind(thumbnail_file);

    std::cout << "Getting frame count..." << std::endl;
    for (unsigned int frame = 0; ; ++frame) {
        std::stringstream frame_filename;
        frame_filename << settings.data_dir << "/frame_" << frame << ".png";
        FILE* input = fopen(frame_filename.str().c_str(), "rb");
        if (!input) {
            break;
        }
        fclose(input);
        ++frame_count;
    }
    if (frame_count == 0)
        throw "Couldn't find any frames! Did you remember a frame_0?";
    std::cout << frame_count << " frames" << std::endl;

    // time to actually start doing flipnote stuff
    // create the animation data first
    // only doing line type 0 (empty) and 1 (compressed bitmap)
    // TODO: implement 2 and 3
    std::cout << "Encoding frames..." << std::endl;
    char previous_frame[256 * 192];
    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        char temp_frame[256 * 192];
        char frame_to_encode[256 * 192];
        std::vector<char> encoded_frame;
        unsigned char current_line_encoding_flags[192] = {}; // only layer 1 for now,
        // layer 2 will be blank
        unsigned char packed_line_encoding_flags[48] = {};

        std::stringstream frame_filename;
        frame_filename << settings.data_dir << "/frame_" << frame << ".png";
        std::vector<unsigned char> decoded_frame;
        unsigned int width, height;
        unsigned int load_ret = lodepng::decode(decoded_frame, width, height, frame_filename.str());
        if (load_ret)
            throw "Failed to decode a frame!"; // TODO: make exceptions std::string so these can be better
        if (width != 256 && height != 192)
            throw "Frames need to be 256x192!";

        // convert the frame to something simpler so it's easier to work with
        for (int y = 0; y < 192; ++y) {
            for (int x = 0; x < 256; ++x) {
                unsigned char r_value = decoded_frame.at((x + y * 256) * 4);
                if (r_value < 127) // input is expected to be greyscale, so it doesn't really matter what channel i check
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
        if (frame == 0)
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
                WRITETO(packed_line_encoding_flags[byte_index], bit_index * 2, 2, 0);
                break;
            case 1:
                WRITETO(packed_line_encoding_flags[byte_index], bit_index * 2, 2, 1);
                break;
            case 2:
                WRITETO(packed_line_encoding_flags[byte_index], bit_index * 2, 2, 2);
                break;
            case 3:
                WRITETO(packed_line_encoding_flags[byte_index], bit_index * 2, 2, 3);
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
                chunk_flag = Util::SwapEndian<unsigned int>(chunk_flag);
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

    size_t anim_data_size = 0;
    for (size_t frame = 0; frame < frame_data.size(); ++frame) {
        anim_data_size += frame_data.at(frame).size();
    }
    if (anim_data_size > UINT32_MAX)
        throw "Animation data is too big! (total frame data size exceeds 32-bit unsigned integer limit)";

    std::vector<uint32_t> frame_offset_table;
    size_t current_offset = 0;
    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        frame_offset_table.push_back(current_offset);
        current_offset += frame_data.at(frame).size();
        if (current_offset > UINT32_MAX)
            throw "Animation data is too big! (frame offset exceeds 32-bit unsigned integer limit)";
    }

    std::cout << "Writing PPM..." << std::endl;
    dsiflipencode::FileHeader header;
    header.magic = 0x41524150; // PARA endian-flipped
    header.anim_data_size = anim_data_size + 8 + frame_count * 4;
    if (settings.use_bgm)
        header.sound_data_size = bgm.size();
    else
        header.sound_data_size = 0;
    header.frame_count = frame_count - 1;
    header.version = 0x24;
    header.locked = 1;
    header.thumbnail_frame_index = 0;
    memset(header.root_author_name, 0, 22);
    memset(header.parent_author_name, 0, 22);
    memset(header.current_author_name, 0, 22);
    memcpy(header.root_author_name, settings.author.c_str(), settings.author.length() * 2 - 1); // we don't want the null terminator
    memcpy(header.parent_author_name, settings.author.c_str(), settings.author.length() * 2 - 1);
    memcpy(header.current_author_name, settings.author.c_str(), settings.author.length() * 2 - 1);
    std::array<uint8_t, 8> reversed_fsid = settings.fsid;
    std::reverse(reversed_fsid.begin(), reversed_fsid.end());
    memcpy(header.parent_author_id, reversed_fsid.data(), 8);
    memcpy(header.current_author_id, reversed_fsid.data(), 8);
    memcpy(header.root_author_id, reversed_fsid.data(), 8);
    std::stringstream filename_hex;
    filename_hex << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) {
        filename_hex << std::uppercase << std::setw(2) << static_cast<unsigned>(settings.filename[i]);
    }
    // yes, the filenames really are internally stored in this way
    memcpy(header.parent_filename, settings.fsid.data() + 5, 3);
    memcpy(header.parent_filename + 3, filename_hex.str().c_str(), 13);
    memcpy(header.current_filename, header.parent_filename, 18);
    memcpy(header.partial_filename, settings.partial_filename.data(), 8);
    header.timestamp = settings.timestamp;
    header.unk2 = 0;

    dsiflipencode::AnimationSectionHeader anim_header;
    anim_header.frame_offset_table_size = frame_count * 4;
    anim_header.unk1 = 0;
    anim_header.flags = 0x410000; // no idea

    dsiflipencode::SoundSectionHeader sound_header;
    if (settings.use_bgm)
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

    // https://stackoverflow.com/questions/7639656/getting-a-buffer-into-a-stringstream-in-hex-representation/
    std::stringstream ppm_filename;
    ppm_filename << std::hex << std::setfill('0');
    for (int i = 0; i < 3; ++i) {
        ppm_filename << std::uppercase << std::setw(2) << static_cast<unsigned>(settings.fsid[i + 5]);
    }
    ppm_filename << "_";
    for (int i = 0; i < 8; ++i) {
        ppm_filename << std::uppercase << std::setw(2) << static_cast<unsigned>(settings.filename[i]);
    }
    ppm_filename << ".ppm";

    FILE* ppm_file = fopen(ppm_filename.str().insert(20, "_").c_str() , "wb");
    if (ppm_file) {
        throw "Could not open output file!";
    }
    // write ppm header
    write_whatever_ptr(ppm_file, &header, sizeof(header));
    // write thumbnail
    for (int i = 0; i < 0x600; ++i) {
        char byte;
        fread(&byte, 1, 1, thumbnail_file);
        fputc(byte, ppm_file);
    }
    // write animation section header
    write_whatever_ptr(ppm_file, &anim_header, sizeof(anim_header));
    // write frame offset table
    for (size_t i = 0; i < frame_offset_table.size(); ++i) {
        write_whatever_ptr(ppm_file, &frame_offset_table.at(i), 4);
    }
    // write frame data
    for (size_t frame = 0; frame < frame_data.size(); ++frame) {
        write_whatever_ptr(ppm_file, frame_data.at(frame).data(), frame_data.at(frame).size());
    }
    // write sound effect usage flags
    write_zeros(ppm_file, frame_count);
    // needs to be aligned to 4 bytes now
    int align_size = 4 - ((0x6A0 + header.anim_data_size + frame_count) % 4);
    if (align_size != 4)
        write_zeros(ppm_file, align_size);
    // write sound header
    write_whatever_ptr(ppm_file, &sound_header, sizeof(sound_header));
    // write bgm (no sound effects for now...)
    if (settings.use_bgm)
        write_whatever_ptr(ppm_file, bgm.data(), bgm.size());
    // write dummy signature
    write_zeros(ppm_file, 0x80);
    // write padding
    write_zeros(ppm_file, 0x10);
    if (ftell(ppm_file) > 1024000)
        std::cout << "The PPM file is over 1MB, so it might not load on a real DSi." << std::endl;
    fclose(ppm_file);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "You need to provide a data directory!";
        return 1;
    }
    
    const std::string data_dir(argv[1]);
    INIReader reader(data_dir + "/config.ini");
    EncoderSettings settings;
    // https://stackoverflow.com/questions/7153935/how-to-convert-utf-8-stdstring-to-utf-16-stdwstring
    std::wstringstream author;
    std::string filename;
    std::string fsid;
    std::string partial_filename;
    std::vector<uint8_t> filename_vector;
    std::vector<uint8_t> fsid_vector;
    std::vector<uint8_t> partial_filename_vector;

    if (reader.ParseError() > 0) {
        std::cout << "Failed to parse " << data_dir << "/config.ini!" << std::endl;
        return 3;
    }

    author << reader.Get("", "author", "dsiflipencode").c_str();
    filename = reader.Get("", "filename", "116AE34C2880B000");
    fsid = reader.Get("", "fsid", "5473EB00A0BC70FA");
    partial_filename = reader.Get("", "partial_filename", "BC70FA116AE34C28");

    filename_vector = Util::HexStringToVector(filename, true);
    fsid_vector = Util::HexStringToVector(fsid, true);
    partial_filename_vector = Util::HexStringToVector(partial_filename, true);

    // https://stackoverflow.com/questions/21276889/copy-stdvector-into-stdarray
    std::copy_n(filename_vector.begin(), 8, settings.filename.begin());
    std::copy_n(fsid_vector.begin(), 8, settings.fsid.begin());
    std::copy_n(partial_filename_vector.begin(), 8, settings.partial_filename.begin());

    settings.author = author.str();
    settings.data_dir = data_dir;
    settings.use_bgm = reader.GetBoolean("", "use_bgm", true);

    uint64_t unix_timestamp = std::stoul(reader.Get("", "timestamp", "3133684800"));
    if (unix_timestamp > UINT32_MAX + static_cast<uint64_t>(946684800)) {
        std::cout << "Timestamp is too large!" << std::endl;
        return 4;
    }
    settings.timestamp = unix_timestamp - 946684800;

    try {
        encode(settings);
        return 0;
    }
    catch(const char* e) {
        std::cout << "Encoder threw an exception: " << e << std::endl;
        return 2;
    }
}