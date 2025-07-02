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
#define STICK_DEADZONE 0.2

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
	GOBJ *barrel_gobj;
    Vec3 barrel_lastpos;
	bool barrel_has_spawned; // Rename? 
	int barrel_lastledge;
};

struct LedgedashAssets
{
    JOBJ *hud;
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

typedef enum LDSH_ACTION
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

void Event_Exit();
void Event_Update();
void Tips_Toggle(GOBJ *menu_gobj, int value);
void Ledgedash_ToggleStartPosition(GOBJ *menu_gobj, int value);
void Ledgedash_HUDCamThink(GOBJ *gobj);
void Ledgedash_ChangeCamMode(GOBJ *gobj, int value);
GOBJ *Ledgedash_HitLogInit();
void Ledgedash_HitLogGX(GOBJ *gobj, int pass);
void RebirthWait_Phys(GOBJ *fighter);
int RebirthWait_IASA(GOBJ *fighter);
int Ledge_Find(int search_dir, float xpos_start, float *ledge_dir);
int Update_CheckPause(void);
int Update_CheckAdvance(void);
