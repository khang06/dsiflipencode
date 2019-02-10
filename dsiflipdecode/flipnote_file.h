#pragma once
#include "util.h"

namespace dsiflipdecode {
    #pragma pack (push, 1)
    typedef struct {
        uint32_t magic;
        uint32_t anim_data_size;
        uint32_t sound_data_size;
        uint16_t frame_count;
        uint16_t unk1;
        uint16_t locked;
        uint16_t thumbnail_frame_index;
        char16_t root_author_name[11];
        char16_t parent_author_name[11];
        char16_t current_author_name[11];
        char parent_author_id[8];
        char current_author_id[8];
        char parent_filename[18];
        char current_filename[18];
        char root_author_id[8];
        char partial_filename[8];
        uint32_t timestamp;
        uint16_t unk2;
    } FileHeader;
    // size check is needed because raw bytes will be casted to this
    static_assert(sizeof(FileHeader) == 0xA0, "dsiflipdecode::FileHeader must be 0xA0 bytes long");

    typedef struct {
        uint16_t frame_offset_table_size;
        uint16_t unk1;
        uint32_t flags;
    } AnimationSectionHeader;
    // size check is needed because raw bytes will be casted to this
    static_assert(sizeof(AnimationSectionHeader) == 8, "dsiflipdecode::AnimationSectionHeader must be 8 bytes long");

    typedef struct {
        uint32_t bgm_size;
        uint32_t se1_size;
        uint32_t se2_size;
        uint32_t se3_size;
        uint8_t frame_speed;
        uint8_t bgm_frame_speed;
        uint8_t encoded; // encoder-exclusive flag for encoded flipnote detection
        INSERT_PADDING_BYTES(13);
    } SoundSectionHeader;
    // size check is needed because raw bytes will be casted to this
    static_assert(sizeof(SoundSectionHeader) == 0x20, "dsiflipdecode::SoundSectionHeader must be 0x8 bytes long");
    #pragma pack(pop)
}