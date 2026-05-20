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

    // Flags: added in v2. Originally was the unused high byte of `u16 image_fmt`.
    u8 enable_hazards: 1;

    u8 image_fmt;
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
    u32 event_count;
    u32 *events;
    u32 *event_offsets;
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
    int rng_seed_recording_num;
    u32 *rng_seed_recording;
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

// These work for v1 as well. It will convert v1 losslessly into the v2 format.
ParsedExportData_v2 ExportData_Import(u8 *transfer_buf);
void ExportData_Free(ParsedExportData_v2 *ed);
bool ExportData_ApplyEvent(void *data, u32 event);
int ExportData_Compress(u8 *dst, u8 *src, u32 size);

/*
    Instead of a big static struct, this format saves a sequence of events (much like SLP files).
    However, instead of having the event id at the start of the event data,
     we put them in a big array at the start of the file.
    This allows the parser to seek to a specific event and determine whether an event exists easier.
    It also happens to be better for cache locality.
*/

typedef struct ExportData_v2 {
    ExportMetadata metadata;
    u32 event_count;
    u32 decompressed_event_data_stream_size;

    // The stream field is equivalent to these dynamically sized fields:
    // u32 events[event_count];
    // u32 event_offsets[event_count];
    // u8 compressed_event_data_stream[];
    u8 stream[];
} ExportData_v2;

// Serialized! Do not reorder events. Only add new events to the end.
// Be sure to update `rec_event_data_sizes`.
typedef enum RecEvent {
    RecEvent_Null,
    RecEvent_MatchInit,
    RecEvent_Savestate_v1,
    RecEvent_Savestate_v2,
    RecEvent_RecordingSlot_v1,
    RecEvent_RecordingSlot_v2,
    RecEvent_RNGSeedRecording,
    RecEvent_MenuSettings_Record_v1,
    RecEvent_MenuSettings_Record_v2,
    RecEvent_MenuSettings_BehaviorOptions,
    RecEvent_MenuSettings_DIOptions,
    RecEvent_MenuSettings_TechOptions,
    RecEvent_MenuSettings_InfoDisplay,
    RecEvent_MenuSettings_ActionLog,
    RecEvent_MenuSettings_CustomOSDs,
    RecEvent_MenuSettings_Overlays,
    RecEvent_MenuSettings_RNGControl,
    // TODO Stage 

    RecEvent_Count
} RecEvent;

extern u32 rec_event_data_sizes[RecEvent_Count];

#define ALIGN_4 __attribute__((aligned(4)))

typedef struct ALIGN_4 RecEventData_MatchInit {
    MatchInit match_init;
} RecEventData_MatchInit;

typedef struct ALIGN_4 RecEventData_Savestate_v1 {
    Savestate_v1 savestate;
} RecEventData_Savestate_v1;

typedef struct ALIGN_4 RecEventData_Savestate_v2 {
    Savestate_v2 savestate;
} RecEventData_Savestate_v2;

typedef struct ALIGN_4 RecEventData_RecordingSlot_v1 {
    RecInputData_v1 rec_input_data;
} RecEventData_RecordingSlot_v1;

typedef struct ALIGN_4 RecEventData_RecordingSlot_v2 {
    u8 ply;
    u8 slot_idx;
    u16 input_count;
    int start_frame;
    RecInputs inputs[REC_LENGTH];
} RecEventData_RecordingSlot_v2;

typedef struct ALIGN_4 RecEventData_RNGSeedRecording {
    u16 count;
    u16 _unused;
    int start_frame;
    u32 seeds[REC_LENGTH];
} RecEventData_RNGSeedRecording;

typedef struct ALIGN_4 RecEventData_MenuSettings_Record_v1 {
    u8 hmn_mode;
    u8 hmn_slot;
    u8 cpu_mode;
    u8 cpu_slot;
    u8 loop_inputs;
    u8 auto_restore;
} RecEventData_MenuSettings_Record_v1;

typedef struct ALIGN_4 RecEventData_MenuSettings_Record_v2 {
    RecEventData_MenuSettings_Record_v1 v1; 
    u8 mirror;
    u8 cpu_counter;
    u8 start_paused;
    u8 hmn_random_percent;
    u8 cpu_random_percent;
    u8 hmn_slot_chances[6];
    u8 cpu_slot_chances[6];
} RecEventData_MenuSettings_Record_v2;

typedef struct CustomTDI {
    float lstickX;
    float lstickY;
    float cstickX;
    float cstickY;
    u32 reversing: 1;
    u32 direction: 1; // 0 = left of player, 1 = right of player
} CustomTDI;

typedef struct AdvancedCounterAction {
    u8 counter_logic;
    u8 counter_air;
    u8 counter_ground;
    u8 counter_shield;
    u8 counter_delay_air;
    u8 counter_delay_ground;
    u8 counter_delay_shield;
} AdvancedCounterAction;

typedef struct ALIGN_4 RecEventData_MenuSettings_BehaviorOptions {
    u8 behavior;
    u8 shield_angle;
    u8 mash;
    u8 inf_shield;
    u8 inf_shield_health;
    u8 shield_dir;
    u8 intang;
    u8 grab_release;
    u8 counter_air;
    u8 counter_ground;
    u8 counter_shield;
    u8 counter_delay;
    AdvancedCounterAction counter_advanced[10];
} RecEventData_MenuSettings_BehaviorOptions;

typedef struct ALIGN_4 RecEventData_MenuSettings_DIOptions {
    u8 tdi_direction;
    u8 sdi_count;
    u8 sdi_direction;
    u8 asdi_direction;
    u8 custom_tdi_count; 
    CustomTDI custom_tdi[10];
} RecEventData_MenuSettings_DIOptions;

typedef struct ALIGN_4 RecEventData_MenuSettings_TechOptions {
    u8 tech_direction;
    u8 getup_direction;
    u8 invisibility;
    u8 invisibility_delay;
    u8 tech_sound;
    u8 simulate_tech_trap;
    u8 tech_lockout;
    u8 chance_tech_in_place;
    u8 chance_tech_away;
    u8 chance_tech_toward;
    u8 chance_tech_miss;
    u8 chance_miss_wait;
    u8 chance_miss_getup_in_place;
    u8 chance_miss_getup_away;
    u8 chance_miss_getup_toward;
    u8 chance_miss_getup_attack;
} RecEventData_MenuSettings_TechOptions;

typedef struct ALIGN_4 RecEventData_MenuSettings_InfoDisplay {
    u8 ply;
    u8 info[8];
} RecEventData_MenuSettings_InfoDisplay;

// TODO: convert arrays to separate objects. Maybe. Think about it.

typedef struct ActionLogAction {
    u8 behavior;
    s8 min_lstick_x;
    s8 min_lstick_y;
    u8 fastfall : 1;
    u8 iasa : 1;
    u16 min_state_frame;
    u16 state;
} ActionLogAction;

typedef struct ALIGN_4 RecEventData_MenuSettings_ActionLog {
    ActionLogAction actions[10];
} RecEventData_MenuSettings_ActionLog;

typedef struct ALIGN_4 RecEventData_MenuSettings_CustomOSDs {
    s16 states[8]; // -1 for n/a
} RecEventData_MenuSettings_CustomOSDs;

typedef struct ALIGN_4 RecEventData_MenuSettings_Overlays {
    u8 ply;
    u8 actionable;
    u8 hitstun;
    u8 invincible;
    u8 ledge_actionable;
    u8 missed_lcancel;
    u8 can_fastfall;
    u8 autocancel;
    u8 crouch;
    u8 wait;
    u8 walk;
    u8 dash;
    u8 run;
    u8 jumps_used;
    u8 fullhop;
    u8 shorthop;
    u8 iasa;
    u8 shield_stun;
} RecEventData_MenuSettings_Overlays;

// TODO store rng seed for each frame and stuff

typedef struct ALIGN_4 RecEventData_MenuSettings_RNGControl {
    u8 peach_item;
    u8 peach_fsmash;
    u8 luigi_misfire;
    u8 gnw_hammer;
    u8 nana_throw;
} RecEventData_MenuSettings_RNGControl;

#endif
