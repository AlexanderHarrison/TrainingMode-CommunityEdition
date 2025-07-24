#ifndef MEX_H_AUDIO
#define MEX_H_AUDIO

#include "structs.h"
#include "datatypes.h"

typedef enum FGMGroup
{
    FGMGROUP_PERSIST,   // main, pokemon, nr_name, end
    FGMGROUP_ANNOUNCER, // nr_title, nr_select, nr_1p, nr_vs
    FGMGROUP_CHARACTER, // character ssms + null.ssm
    FGMGROUP_STAGE,     // stage ssms + last.ssm
    FGMGROUP_1P,        // ending, 1pend
} FGMGroup;

typedef enum FGM_Main
{
    FGMMAIN_KAIFUKU = 125,
    FGMMAIN_SCORECOUNT,
    FGMMAIN_COIN_GET = 147,
    FGMMAIN_CS_CANCEL = 172,
    FGMMAIN_CS_KETTEI,
    FGMMAIN_CS_MV,
    FGMMAIN_CS_BEEP1,
    FGMMAIN_INFOBEEP,
    FGMMAIN_PAUSE,
    FGMMAIN_CS_ERASE_CAUTION1 = 188,
    FGMMAIN_CS_ERASE_CAUTION2,
} FGM_Main;

enum FGMID
{
    FGM_MAIN,
    FGM_POKEMON,
    FGM_NR_TITLE,
    FGM_NR_SELECT,
    FGM_NR_1P,
    FGM_NR_VS,
    FGM_CAPTAIN,
    FGM_CLINK,
    FGM_DK,
    FGM_DRMARIO,
    FGM_FALCO,
    FGM_FOX,
    FGM_GKOOPA,
    FGM_ICE,
    FGM_KIRBY,
    FGM_KOOPA,
    FGM_LINK,
    FGM_LUIGI,
    FGM_MARIO,
    FGM_MARS,
    FGM_MEWTWO,
    FGM_NESS,
    FGM_PEACH,
    FGM_PICHU,
    FGM_PIKACHU,
    FGM_PURIN,
    FGM_SAMUS,
    FGM_ZS,
    FGM_YOSHI,
    FGM_GW,
    FGM_GANON,
    FGM_EMBLEM,
    FGM_MHANDS,
    FGM_KIRBYTM,
    FGM_CASTLE,
    FGM_CORNERIA,
    FGM_GREATBAY,
    FGM_KONGO,
    FGM_MUTECITY,
    FGM_ONETT,
    FGM_ZEBES,
    FGM_GARDEN,
    FGM_KLAID,
    FGM_GREEN,
    FGM_VENOM,
    FGM_BIGBLUE,
    FGM_FOURSIDE,
    FGM_PUPUPU,
    FGM_PSTADIUM,
    FGM_1PADV,
    FGM_ENDING,
    FGM_NR_NAME,
    FGM_1PEND,
    FGM_LAST,
    FGM_END,
    FGM_NULL,
};

typedef struct _AXPBADDR
{
    u16 loopFlag;         // 0x0, one shot or looped sample (see above)
    u16 format;           // 0x2, sample format used (see above)
    u16 loopAddressHi;    // 0x4, Loop start
    u16 loopAddressLo;    // 0x6
    u16 endAddressHi;     // 0x8, Loop/sample end, including last sample played
    u16 endAddressLo;     // 0xa
    u16 currentAddressHi; // 0xc, Current playback position
    u16 currentAddressLo; // 0xe
} AXPBADDR;

typedef struct _AXPBMIX
{
    u16 vL;          // volume left
    u16 vDeltaL;     // volume ramp left
    u16 vR;          // volume right
    u16 vDeltaR;     // volume ramp right
    u16 vAuxAL;      // volume AUX A left
    u16 vDeltaAuxAL; // volume ramp AUX A left
    u16 vAuxAR;      // volume AUX A right
    u16 vDeltaAuxAR; // volume ramp AUX A right
    u16 vAuxBL;      // volume AUX B left
    u16 vDeltaAuxBL; // volume ramp AUX B left
    u16 vAuxBR;      // volume AUX B right
    u16 vDeltaAuxBR; // volume ramp AUX B right
    u16 vAuxBS;      // volume AUX B surround
    u16 vDeltaAuxBS; // volume ramp AUX B surround
    u16 vS;          // volume surround
    u16 vDeltaS;     // volume ramp surround
    u16 vAuxAS;      // volume AUX A surround
    u16 vDeltaAuxAS; // volume ramp AUX A surround
} AXPBMIX;

typedef struct _AXPBITD
{
    u16 flag;         // on or off for this voice
    u16 bufferHi;     // buffer in RAM
    u16 bufferLo;     // buffer in RAM
    u16 shiftL;       // phase shift samples left (current)
    u16 shiftR;       // phase shift samples right (current)
    u16 targetShiftL; // phase shift samples left (target)
    u16 targetShiftR; // phase shift samples right (target)
} AXPBITD;

typedef struct _AXPBUPDATE
{
    u16 updNum[5]; // number of updates per 1ms step
    u16 dataHi;    // location of update data in main memory
    u16 dataLo;
} AXPBUPDATE;

typedef struct _AXPBDPOP
{
    s16 aL;
    s16 aAuxAL;
    s16 aAuxBL;
    s16 aR;
    s16 aAuxAR;
    s16 aAuxBR;
    s16 aS;
    s16 aAuxAS;
    s16 aAuxBS;
} AXPBDPOP;

typedef struct _AXPBVE
{
    u16 currentVolume; // .15 volume at start of frame
    s16 currentDelta;  // signed per sample delta delta
} AXPBVE;

typedef struct _AXPBADPCM
{
    u16 a[8][2];    // coef table a1[0],a2[0],a1[1],a2[1]....
    u16 gain;       // gain to be applied (0 for ADPCM, 0x0800 for PCM8/16)
    u16 pred_scale; // predictor / scale combination (nibbles, as in hardware)
    u16 yn1;        // y[n - 1]
    u16 yn2;        // y[n - 2]
} AXPBADPCM;

typedef struct _AXPBFIR
{
    u16 numCoefs; // reserved, keep zero
    u16 coefsHi;  // reserved, keep zero
    u16 coefsLo;  // reserved, keep zero
} AXPBFIR;

typedef struct _AXPBSRC
{
    // ratio = (f32)ratio * 0x10000;
    u16 ratioHi;            // sampling ratio, integer
    u16 ratioLo;            // sampling ratio, fraction
    u16 currentAddressFrac; // current fractional sample position
    u16 last_samples[4];    // last 4 input samples
} AXPBSRC;

typedef struct _AXPBADPCMLOOP
{
    u16 loop_pred_scale; // predictor / scale combination (nibbles, as in hardware)
    u16 loop_yn1;        // y[n - 1]
    u16 loop_yn2;        // y[n - 2]
} AXPBADPCMLOOP;

typedef struct _AXPB
{
    u16 nextHi;              // 0x0, pointer to next parameter buffer (MRAM)
    u16 nextLo;              // 0x2,
    u16 currHi;              // 0x4, pointer to this parameter buffer (MRAM)
    u16 currLo;              // 0x6
    u16 srcSelect;           // 0x8, Select type of SRC (none,4-tap,linear)
    u16 coefSelect;          // 0xa, Coef. to be used with 4-tap SRC
    u16 mixerCtrl;           // 0xc, Mixer control bits
    u16 state;               // 0xe, current state
    u16 type;                // 0x10, type of voice
    AXPBMIX mix;             // 0x12
    AXPBITD itd;             // 0x36
    AXPBUPDATE update;       // 0x44
    AXPBDPOP dpop;           // 0x52
    AXPBVE ve;               // 0x64
    AXPBFIR fir;             // 0x68
    AXPBADDR addr;           // 0x6E
    AXPBADPCM adpcm;         // 0x7E
    AXPBSRC src;             // 0xA6
    AXPBADPCMLOOP adpcmLoop; // 0xB4
    u16 pad[3];              // 0xBA, 32 byte alignment
} AXPB;

typedef struct _AXVPB
{
    void *next;  // 0x0, used in priority stacks
    void *prev;  // 0x4, used in priority stacks
    void *next1; // 0x8, used in callback stack
    // these are used in voice allocation
    u32 priority;    // 0xc, index to stack
    void *callback;  // 0x10, user callback for specified
    u32 userContext; // 0x14, user assigned context for callback
    // vars & flags for updating and sync PBs
    u32 index;           // 0x18, index of VPB in array
    u32 sync;            // 0x1c, bit mask for each PB item to sync
    u32 depop;           // 0x20, should depop voice
    u32 updateMS;        // 0x24, update current ms
    u32 updateCounter;   // 0x28, counter for n updates
    u32 updateTotal;     // 0x2c, bounds checking for update block
    u16 *updateWrite;    // 0x30, write pointer for PB updates
    u16 updateData[128]; // 0x34, data for PB updates
    void *itdBuffer;     // 0x134, pointer to ITD buffer
    AXPB pb;             // 0x138, write params to this PB 0x
} AXVPB;

typedef struct FGMLive // i really need to look into this more
{
    u8 x0[0x8];      // 0x0
    int flags;       // 0x8
    int index;       // 0xc, index in the FGMLive array
    int x10 : 24;    // 0x10, if this is -1, the FGM is not playing yet, still queued
    u16 sound_index; // 0x14 local to the bank
    u16 x16;         // 0x16
    u8 x18;          // 0x18
    u8 priority;     // 0x19
    u8 volume;       // 0x1a
    u8 x1b;          // 0x1b
    u8 x1c;          // 0x1c
    u8 x1d;          // 0x1d
    u8 x1e;          // 0x1e
    u8 x1f;          // 0x1f
    u8 x20[0x9];     // 0x20
    u8 x29;          // 0x29, reverb maybe
    u8 x2a[0xa];     // 0x2a

} FGMLive;

typedef struct HPSHeader
{
    char magic[8];               // 0x0
    int freq;                    // 0x8
    int channel_num;             // 0xc
    u16 is_loop;                 // 0x10
    u16 format;                  // 0x12
    int sa;                      // 0x14
    int ea;                      // 0x18
    int ca;                      // 0x1c
    u16 coef[16];                // 0x20
    u16 gain;                    // 0x40
    u16 initial_predictor_scale; // 0x42
    u16 initial_sample_history1; // 0x44
    u16 initial_sample_history2; // 0x46
} HPSHeader;

typedef struct HPSChunkHeader
{
    int length;           // 0x0
    int length_minus_one; // 0x4
    int next;             // 0x8
    u16 initPS;           // 0xc
    u16 initsh1;          // 0xe
    u16 initsh2;          // 0x10
    u16 gain;             // 0x12
    int extra;            // 0x14
    int x18;              // 0x18
    int x1c;              // 0x1c
} HPSChunkHeader;

struct BGMData // this name sucks, dont know enough about it to rename it
{
    unsigned int unk : 26;
    unsigned int vbp_index : 6;
};

struct VPB
{
    int voice_index;        // 0x0, internally "vID"
    int x4;                 // 0x4
    u8 x8;                  // 0x8
    u8 x9_80 : 1;           // 0x9, 0x80
    u8 x9_40 : 1;           // 0x9, 0x40
    u8 x9_20 : 1;           // 0x9, 0x20
    u8 x9_10 : 1;           // 0x9, 0x10
    u8 is_initializing : 1; // 0x9, 0x08, is raised when initalizing the hps load process @ 8038b76c, is lowered when the first chunk is played @ 8038b310
    u8 x9_04 : 1;           // 0x9, 0x04
    u8 pause : 1;           // 0x9, 0x02, if this is not set it inits x24 when playing the hps @ 8038b180
    u8 axvpb_num;           // 0xa, number of daisy chained axvpb's at 0xc
    u8 xb;                  // 0xb
    AXVPB *axvpb;           // 0xc
    AXVPB *axvpb2;          // 0x10
    float freq;             // 0x14, are combined together to set ratioHi of AXPBSRC @ 8038b244
    float pitch_x18;        // 0x18, are combined together to set ratioHi of AXPBSRC @ 8038b244
    float pitch_x1c;        // 0x1c, are combined together to set ratioHi of AXPBSRC @ 8038b244
    VPB *prev;              // 0x20, is equal to the previous value of stc_bgm_vpb, before it was replaced with a ptr to this one
    u16 currentVolume;      // 0x24, name derived from r4 @ 8038b200
    u8 is_updated_prev;     // 0x26, raised when prev is set @ 8038b33c
    u8 x27;                 // 0x27
    float volume_x28;       // 0x28, is used to calculate x24 @ 8038b194
    u8 x2c[0x8];            // 0x2c
    float volume_x34;       // 0x34, is used to calculate x24 @ 8038b194
    float volume_x38;       // 0x38 (gets ignored completely?)
    u8 x3c[0x14];           // 0x3c
};

struct AXLive
{
    u8 x0[0x384];                        // 0x0
    VPB voice_data[64];                  // 0x384
    u8 x1784[0xc0];                      // 0x1784, some constants
    BGMData bgm_data_lookup[263];        // 0x1844, array of BGMData corresponding to SFX that were played with an instance_slot param > 0 when calling SFX_PlayRaw. this is used to stop previous instances when playing the same sfx
    HPSChunkHeader hps_chunk_headers[3]; // 0x1c60, circular buffer of 3 most recent hps headers
};

static FGMLive *fgm_live = (void *)0x804c45a0; // points to an array of ? FGMLive structs
static AXLive *ax_live = (void *)0x804c28e0;
static VPB *stc_voice_data = (void *)0x804c2c64;
static float *stc_fgm_volume = R13_OFFSET(-0x7dbc);
static float *stc_bgm_volume = R13_OFFSET(-0x7db8);
static BGMData *stc_bgm_data = R13_OFFSET(-0x5668);                      // voice index of the bgm currently playing, is -1 when nothing is playing
static VPB *stc_bgm_vpb = R13_OFFSET(-0x3f54);                           // is set when the first hps sample is played @ 8038b33c. value is copied to the prev member of the vpb struct before being updated
static int *stc_bgm_data_seed = R13_OFFSET(-0x3f50);                     // is used to generate the next bgm's bgm_data
static int *stc_bgm_tick = R13_OFFSET(-0x3f44);                          // how many times bgm audio has been updated (incremented @ 8038ad44)
static BGMData *stc_bgm_data2 = R13_OFFSET(-0x3f40);                     // contains some data about the bgm, including its voice_index
static int *stc_bgm_entrynum = R13_OFFSET(-0x3f3c);                      // entrynum of the hps file currently playing again
static int *stc_bgm_is_playing = R13_OFFSET(-0x5668);                    // is -1 when not playing
static int *stc_bgm_curLoadingHpsChunkHeaderIndex = R13_OFFSET(-0x3f38); // index of the cur hps chunk header loaded (0-2 circular buffer)
static int *stc_bgm_lastLoadedHPSChunkIndex = R13_OFFSET(-0x3f34);       // index of the most recent hps chunk loaded (0-2 circular buffer), changes after hps chunk is loaded @ 8038ad6c
static int *stc_bgm_curPlayingHpsChunkHeaderIndex = R13_OFFSET(-0x3f2c); // index of the cur hps chunk header playing (0-2 circular buffer)
static u8 *stc_bgm_isLoadingHPSChunk = R13_OFFSET(-0x3f28);              // flag that indicates an hps chunk is being loaded
static int *stc_bgm_aramAlloc = R13_OFFSET(-0x3f20);                     // start of the 3 hps chunk circular buffer
static int *stc_fgm_tick = R13_OFFSET(-0x3f14);                          // how many times fgm audio has been updated (incremented @ 8038ad44)
static FGMLive *stc_last_fgmlive = R13_OFFSET(-0x3f0c);                  // points to the most recently created FGMLive struct

int SFX_Play(int sfxID);
int SFX_PlayRaw(int sfx, int volume, int pan, int instance_slot, int fgm_kind); // any instance_slot other than 0 will remember the current instance and destroy it if another is requested to play with that slot
int SFX_PlayCommon(int sfxID);
int SFX_PlayCrowd(int sfxID);
void SFX_StopCrowd();

char *Nametag_GetText(int tag_index);
void Audio_ResetCache(int group_index);
void Audio_QueueFileLoad(int group_index, u64 ssm_index);
void Audio_UpdateCache();
void Audio_SyncLoadAll();
void BGM_DecideMenuBGM();
int BGM_GetMenuBGM();
void BGM_PlayFile(char *filename, int volume, int unk);
void BGM_Play(int hpsID);
void BGM_Stop();
void BGM_Pause();
void BGM_Resume();
void BGM_SetVolume(float volume); // 0 - 1
int FGM_CheckActive(u32 fgm_id);
void FGM_Stop(u32 fgm_id);
int FGM_SetVolume(u32 sfxid, u8 volume);
int FGM_SetPanning(u32 sfxid, u8 panning);
void FGM_PauseKind(int kind); // pausing in-game pauses kinds 5,6,7,8
void FGM_ResumeKind(int kind);

AXVPB *AXAcquireVoice(u32 priority, void *callback, u32 userContext);

#endif
