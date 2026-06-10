#include "../MexTK/mex.h"
#include "events.h"

typedef struct LCancelData LCancelData;
typedef struct LCancelAssets LCancelAssets;

struct LCancelData
{
    EventDesc *event_desc;
    LCancelAssets *lcancel_assets;
    GOBJ *barrel_gobj;
    Vec3 barrel_lastpos;
    int barrel_intangible_timer; // frame counter for intangible cycles (0-119)

    s8 lcancel;   // if the player failed this aerial's l-cancel. Unset: 0, Fail: -1, Success: 1
    s8 success;   // if the player either lcanceled or edgecanceled. Unset: 0, Fail: -1, Success: 1
    bool fastfell; // if the player fastfell this aerial
    
    int fastfall_frame;
    int lrz_input_count;
    int lrz_input_frame[8];
    int land_frame;
    int cur_frame;
    
    int successful_attempts;
    int total_attempts;

    struct
    {
        u8 shield_isdisp;   // whether tip has been shown to the player
        u8 shield_num;      // number of times condition has been met
        u8 hitbox_active;   // whether or not the last aerial used had a hitbox active
        u8 hitbox_isdisp;   // whether tip has been shown to the player
        u8 hitbox_num;      // number of times condition has been met
        u8 fastfall_active; // whether or not the last aerial used had a hitbox active
        u8 fastfall_isdisp; // whether tip has been shown to the player
        u8 fastfall_num;    // number of times condition has been met
        u8 late_isdisp;     // whether tip has been shown to the player
        u8 late_num;        // number of times condition has been met
    } tip;
};

struct LCancelAssets
{
    JOBJDesc *hud;
    void **hudmatanim; // pointer to array
};

#define LCLTEXT_SCALE 4.2
#define LCLARROW_ANIMFRAMES 4
#define LCLARROW_JOBJ 7
#define LCLARROW_OFFSET 0.365

// Maximum number of frames before a miss is considered "No Press", using zero-based indexing.
#define MAX_L_PRESS_TIMING 29

void Tips_Toggle(GOBJ *menu_gobj, int value);
void Tips_Think(LCancelData *event_data, FighterData *hmn_data);
void LCancel_Think(LCancelData *event_data, FighterData *hmn_data);
void LCancel_ChangeBarrel(GOBJ *gobj, int value);
void Barrel_Think(LCancelData *event_data);
void Barrel_Toggle(GOBJ *menu_gobj, int value);
GOBJ *Barrel_Spawn(int pos_kind);
int Barrel_OnHurt(GOBJ *barrel_gobj);
int Barrel_OnDestroy(GOBJ *barrel_gobj);
void Barrel_Null(void);
void Event_Exit(GOBJ *menu);
