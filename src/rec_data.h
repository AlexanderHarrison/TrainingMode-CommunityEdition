#ifndef REC_DATA_H
#define REC_DATA_H

#include <stddef.h>

// COMMON #####################################################

#define REC_LENGTH (1 * 60 * 60)

typedef struct RecInputs {
    u8 btn_dpadup : 1;
    u8 btn_a : 1;
    u8 btn_b : 1;
    u8 btn_x : 1;
    u8 btn_y : 1;
    u8 btn_L : 1;
    u8 btn_R : 1;
    u8 btn_Z : 1;

    s8 stickX;
    s8 stickY;
    s8 substickX;
    s8 substickY;
    u8 trigger;
} RecInputs;

typedef struct ExportMetadata {
    u16 version;
    u16 image_width;
    u16 image_height;
    u16 image_fmt;
    u8 hmn;
    u8 hmn_costume;
    u8 cpu;
    u8 cpu_costume;
    u16 stage_external; // instance of the stage
    u16 stage_internal; //  unique stage
    u8 month;
    u8 day;
    u16 year;
    u8 hour;
    u8 minute;
    u8 second;
    char filename[32];
    char _pad;
} ExportMetadata;

typedef struct ParsedExportData_v2 {
    ExportMetadata *metadata;
    u16 event_size_count;
    u16 event_count; // guaranteed to be a multiple of 4 (to align events to 4b)
    u32 *event_sizes;
    u8 *events;
    u8 *event_data_stream;
} ParsedExportData_v2;

// VERSION 1 #####################################################

#define REC_SLOTS 6

typedef struct ExportData_v1
{
    u16 menu_index;
    u16 menu_state;
    JOBJ *memcard_jobj[2];
    JOBJ *screenshot_jobj;
    JOBJ *textbox_jobj;
    RGB565 *scaled_image;
    Text *text_title;
    Text *text_desc;
    Text *text_misc;
    int slot;
    u8 is_inserted[2];
    int free_blocks[2];
    int free_files[2];
    Text *text_keyboard;
    Text *text_filename;
    Text *text_filedetails;
    u8 key_cursor[2];
    u8 filename_cursor;
    u8 caps_lock;
    char *filename_buffer;
    GOBJ *confirm_gobj;
    Text *confirm_text;
    u8 confirm_cursor;
    u8 confirm_state;
    int hmn_id;
    int cpu_id;
    int stage_id;
} ExportData_v1;

typedef struct ExportMenuSettings_v1
{
    u8 hmn_mode;
    u8 hmn_slot;
    u8 cpu_mode;
    u8 cpu_slot;
    u8 loop_inputs;
    u8 auto_restore;
} ExportMenuSettings_v1;

typedef struct ExportHeader_v1
{
    ExportMetadata metadata;
    struct // lookup
    {
        int ofst_screenshot;
        int ofst_recording;
        int ofst_menusettings;
        // to-do, add menu data offset
    } lookup;
} ExportHeader_v1;

typedef struct RecInputData_v1
{
    int start_frame; // the frame these inputs start on
    int num;
    RecInputs inputs[REC_LENGTH];
} RecInputData_v1;

typedef struct RecData_v1
{
    int hmn_rndm_slot;
    RecInputData_v1 *hmn_inputs[REC_SLOTS];
    RecInputData_v1 *hmn_rerecord_inputs;
    int cpu_rndm_slot;
    RecInputData_v1 *cpu_inputs[REC_SLOTS];
    RecInputData_v1 *cpu_rerecord_inputs;
    int restore_timer;
    JOBJ *seek_jobj;
    Text *text;
    float seek_left;
    float seek_right;
} RecData_v1;

typedef struct RecordingSave_v1
{
    MatchInit match_data; // this will point to a struct containing match info
    Savestate_v1 savestate;
    RecInputData_v1 hmn_inputs[REC_SLOTS];
    RecInputData_v1 cpu_inputs[REC_SLOTS];
} RecordingSave_v1;

// VERSION 2 #####################################################

// These work for v1 as well by converting it losslessly to the v2 format.
ParsedExportData_v2 ExportData_Import(u8 *transfer_buf);
void ExportData_ApplyEvents(ParsedExportData_v2 *ed);
void ExportData_Free(ParsedExportData_v2 *ed);
int ExportData_Compress(u8 *dst, u8 *src, size);

/*
    Instead of a big static struct, this format saves a sequence of events (much like SLP files).
    However, instead of having the event id at the start of the event data,
     we put them in a big array at the start of the file.
    This allows the parser to seek to a specific event and determine whether an event exists easier.
    It also happens to be better for cache locality.
*/

typedef struct ExportData_v2 {
    ExportMetadata metadata;
    u16 event_size_count;
    u16 event_count; // guaranteed to be a multiple of 4 (to align events to 4b)
    u32 compressed_event_data_stream_size;
    u32 decompressed_event_data_stream_size;

    // stream field is equivalent to these dynamically sized fields:
    // u32 event_sizes[event_size_count];
    // u8 events[event_count]; 
    // u8 compressed_event_data_stream[];
    u8 stream[];
} ExportData_v2;

typedef enum RecEvent {
    RecEvent_Null,
    RecEvent_MatchInit,
    RecEvent_Savestate_v1,
    RecEvent_Savestate_v2,
    RecEvent_RecordingSlot_v1,

    // RecEvent_InfoDisplay,
    // RecEvent_RNGOptions,
    // RecEvent_TDIOptions,
    // RecEvent_TechOptions,
    // RecEvent_ActionLog,
    // RecEvent_CustomOSDs,
} RecEvent;

typedef struct RecEventData_MatchInit {
    MatchInit match_init;
} RecEventData_MatchInit;

typedef struct RecEventData_Savestate_v1 {
    Savestate_v1 savestate;
} RecEventData_Savestate_v1;

typedef struct RecEventData_Savestate_v2 {
    Savestate_v2 savestate;
} RecEventData_Savestate_v2;

typedef struct RecEventData_RecordingSlot_v1 {
    RecInputData_v1 rec_input_data;
} RecEventData_RecordingSlot_v1;

typedef struct RecEventData_RecordingSlot_v2 {
    u8 ply;
    u8 slot_idx;
    u16 input_count;
    RecInputs inputs[REC_LENGTH];
} RecEventData_RecordingSlot_v2;

#endif
