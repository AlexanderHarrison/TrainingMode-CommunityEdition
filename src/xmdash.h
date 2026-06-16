#ifndef _XMDASH_H_
#define _XMDASH_H_

#include "../MexTK/mex.h"
#include "events.h"
#include "wavedash_common.h"

#define XMDASH_START_X      80.0f    // start on the flat track, just right of the bat platform (HRC); calibrated in T9

// Calibrated to HRC's distance signs: 600 units = the 200m sign (~3 units/m).
static float XmDash_Dist[] = { 150.f, 300.f, 600.f };
static const char *XmDash_DistText[] = { "50m", "100m", "200m" };
#define XMDASH_DEFAULT_DIST_IDX 1   // default 100m

typedef struct XmDashAssets {
    JOBJDesc *hud;
    void **hudmatanim;
    JOBJDesc *target_jobj;
    void **target_jointanim;
    void **target_matanim;
} XmDashAssets;

typedef struct XmDashData {
    EventDesc *event_desc;
    XmDashAssets *assets;
    WavedashCore core;
    int   started;        // dash timer running
    int   finished;       // crossed the finish this run
    int   run_frames;     // elapsed frames since the dash started
    int   result_timer;   // frames since finishing (result hold before auto-reset)
    int   initialized;    // one-time grounded setup (start capture + marker) done
    int   reset_pending;  // distance changed in the menu; apply the reset on the next think frame
    float start_x;        // start line (captured on the first grounded frame)
    float start_y;        // track-surface Y, for clean teleport-back
    float distance;       // finish = start_x + distance
    GOBJ *finish_gobj;    // finish-line marker model
    int   session_best[countof(XmDash_Dist)]; // frames per distance; 0 = none
    int   persist_best;                        // frames for the default (100m) distance; 0 = none
    int   free_practice;                       // untimed practice mode; no finish/scoring
} XmDashData;

enum options_main {
    OPT_RETRY,
    OPT_FREEPRACTICE,
    OPT_DISTANCE,
    OPT_HUD,
    OPT_EXIT,
    OPT_COUNT
};

void Retry(GOBJ *menu);
void Exit(GOBJ *menu);
void StartFreePractice(GOBJ *gobj);
void XmDash_OnChangeDistance(GOBJ *menu, int value);
void Event_Init(GOBJ *gobj);
void Event_Think(GOBJ *event);
void HUD_GX(GOBJ *gobj, int pass);

static EventOption Options_Main[OPT_COUNT] = {
    {
        .kind = OPTKIND_FUNC,
        .name = "Retry",
        .desc = { "Restart the dash. Or press Z while paused." },
        .OnSelect = Retry,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Enable Free Practice",
        .desc = { "Untimed practice - no finish line or scoring." },
        .OnSelect = StartFreePractice,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "Distance",
        .desc = { "Select the dash distance." },
        .value_num = countof(XmDash_DistText),
        .values = XmDash_DistText,
        .val = XMDASH_DEFAULT_DIST_IDX,
        .OnChange = XmDash_OnChangeDistance,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "HUD",
        .val = 1,
        .desc = {"Toggle visibility of the wavedash HUD."},
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = { "Return to the Event Selection Screen." },
        .OnSelect = Exit,
    },
};
static EventMenu Menu_Main = {
    .name = "100m Dash",
    .option_num = countof(Options_Main),
    .options = Options_Main,
};

#endif // _XMDASH_H_
