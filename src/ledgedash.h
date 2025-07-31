#include "../MexTK/mex.h"
#include "events.h"

typedef struct LedgedashData LedgedashData;
typedef struct LedgedashAssets LedgedashAssets;
typedef struct LdshHitlogData LdshHitlogData;
typedef struct LdshHitboxData LdshHitboxData;

#define LSDH_TIPDURATION 1.7 * 60
#define LDSH_HITBOXNUM 30 * 4
#define LCLTEXT_SCALE 4.5
#define LCLJOBJ_BAR 4

struct LedgedashData
{
    EventDesc *event_desc;
    LedgedashAssets *assets;
    int ledge;
    bool was_successful;
    s16 reset_timer;
    GOBJ *hitlog_gobj;
    CmSubject *cam;
    struct
    {
        GOBJ *gobj;
        Text *text_angle;
        Text *text_galint;
        int canvas;
        float airdodge_angle;
    } hud;
    struct
    {
        int timer;
        u8 is_ledgegrab : 1;
        u8 is_release : 1;
        u8 is_airdodge : 1;
        u8 is_finished : 1;
        u8 action_log[30];
    } action_state;
    struct
    {
        s16 refresh_num;     // number of times refreshed
        u8 refresh_cond_num; // number of times tip condition has been met
        u8 refresh_displayed : 1;
        u8 is_input_release : 1;
    } tip;
};

struct LedgedashAssets
{
    JOBJDesc *hud;
    void **hudmatanim; // pointer to array
};

struct LdshHitboxData
{
    int kind;
    float size;
    Vec3 pos_curr;
    Vec3 pos_prev;
};

struct LdshHitlogData
{
    int num;
    LdshHitboxData hitlog[LDSH_HITBOXNUM];
};

enum ldsh_action
{
    LDACT_NONE,
    LDACT_CLIFFWAIT,
    LDACT_FALL,
    LDACT_JUMP,
    LDACT_AIRDODGE,
    LDACT_ATTACK,
    LDACT_LANDING,
    LDACT_GALINT,
};

void Event_Exit(GOBJ *menu);
void Event_Update(void);
void Tips_Toggle(GOBJ *menu_gobj, int value);
void Tips_Think(LedgedashData *event_data, FighterData *hmn_data);
void Ledgedash_ToggleStartPosition(GOBJ *menu_gobj, int value);
void Ledgedash_HUDCamThink(GOBJ *gobj);
void Ledgedash_ChangeCamMode(GOBJ *gobj, int value);
void Ledgedash_ChangeShowHUD(GOBJ *gobj, int show);
GOBJ *Ledgedash_HitLogInit(void);
void Ledgedash_HitLogGX(GOBJ *gobj, int pass);
void Ledgedash_FtInit(LedgedashData *event_data);
void Ledgedash_UpdateCamera(void);
void Ledgedash_HitLogThink(LedgedashData *event_data, GOBJ *hmn);
void Ledgedash_InitVariables(LedgedashData *event_data);
void Ledgedash_ResetThink(LedgedashData *event_data, GOBJ *hmn);
void Ledgedash_HUDThink(LedgedashData *event_data, FighterData *hmn_data);
void Ledgedash_HUDInit(LedgedashData *event_data);
void RebirthWait_Phys(GOBJ *fighter);
int RebirthWait_IASA(GOBJ *fighter);
int Ledge_Find(int search_dir, float xpos_start, float *ledge_dir);
int Update_CheckPause(void);
int Update_CheckAdvance(void);
int Fighter_IsFallBlocked(FighterData *hmn_data);
void Fighter_UpdateCamera(GOBJ *fighter);
void Fighter_UpdatePosition(GOBJ *fighter);
void Fighter_PlaceOnLedge(void);
