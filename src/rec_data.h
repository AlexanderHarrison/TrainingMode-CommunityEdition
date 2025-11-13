typedef struct RecInputs
{
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

typedef struct RecInputData
{
    int start_frame; // the frame these inputs start on
    int num;
    RecInputs inputs[REC_LENGTH];
} RecInputData;

typedef struct Recording
{
    RecInputData hmn_inputs[REC_SLOTS];
} Recording;

typedef struct RecData
{
    int hmn_rndm_slot;
    RecInputData *hmn_inputs[REC_SLOTS];
    RecInputData *hmn_rerecord_inputs;
    int cpu_rndm_slot;
    RecInputData *cpu_inputs[REC_SLOTS];
    RecInputData *cpu_rerecord_inputs;
    int restore_timer;
    JOBJ *seek_jobj;
    Text *text;
    float seek_left;
    float seek_right;
} RecData;

typedef struct RecordingSave
{
    MatchInit match_data; // this will point to a struct containing match info
    Savestate_v2 savestate;
    RecInputData hmn_inputs[REC_SLOTS];
    RecInputData cpu_inputs[REC_SLOTS];
} RecordingSave;

