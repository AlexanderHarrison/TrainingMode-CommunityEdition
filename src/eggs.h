#ifndef _EGGS_H_
#define _EGGS_H_

#include "../MexTK/mex.h"
#include "events.h"

void *(*Events_SetEventAsPlayed)(int event_id) = (void *(*)(int event_id)) 0x8015ceb4;
void *(*Events_StoreEventScore)(int event_id, int score) = (void *(*)(int event_id, int score)) 0x8015cf70;
int (*Events_GetSavedScore)(int event_id) = (int (*)(int event_id)) 0x8015cf5c;
void *(*MatchInfo_0x0010_store)(int unk) = (void *(*)(int unk)) 0x8016b350;
void *(*Egg_Destroy)(GOBJ *egg_gobj) = (void *(*)(GOBJ *egg_gobj)) 0x80289158;
void *(*Event_Retry)(void) = (void *(*)(void)) 0x8016cf4c;

static int egg_counter = 0;
static int high_score = 0;
static Vec3 coll_pos, last_coll_pos;
static GOBJ *egg_gobj;
static CmSubject *cam;
static GOBJ *hud_score_gobj, *hud_best_gobj;
static JOBJ *hud_score_jobj, *hud_best_jobj;
static int canvas;
static Text *hud_score_text, *hud_best_text;

enum options_main
{
    OPT_RETRY,
    OPT_FREEPRACTICE,
    OPT_DAMAGETHRESHOLD,
    OPT_SCALE,
    OPT_VELOCITY,
    OPT_COLLISION,
    OPT_EXIT,

    OPT_COUNT
};

void StartFreePractice(GOBJ *gobj);
GOBJ *Egg_Spawn(void);
void Exit(GOBJ *menu);
void Retry(GOBJ *menu);
static void Egg_OnChangeSize(GOBJ *menu, int value);
void ChangeHitDisplay(GOBJ *menu_gobj, int value);
void Event_Init(GOBJ *gobj);
float RandomRange(float low, float high);
int Egg_OnTakeDamage(GOBJ *gobj);
void Event_Think(GOBJ *event);

static GXColor text_gold = {255, 211, 0, 255};
static GXColor text_white = {255, 255, 255, 255};

static float EggOptions_Size[] = {1.0f, .5f, 2.0f};
static const char *EggOptions_SizeText[] = {"Normal", "Small", "Large"};

static EventOption Options_Main[OPT_COUNT] = {
    {
        .kind = OPTKIND_FUNC,
        .name = "Retry",
        .desc = { "Retry this event. Pressing Z while paused also does this." },
        .val = 0,
        .disable = 0,
        .OnSelect = Retry
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Enable Free Practice",
        .desc = { "Start Free Practice mode." },
        .val = 0,
        .disable = 0,
        .OnSelect = StartFreePractice
    },
    {
        .kind = OPTKIND_INT,
        .name = "Egg Damage Threshold",
        .desc = { "Adjust the minimum damage needed to break an egg." },
        .val = 12,
        .format = "%d",
        .value_min = 0,
        .value_num = 200,
        .disable = 1
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(EggOptions_SizeText) / 
                     sizeof(*EggOptions_SizeText),
        .name = "Egg Scale",
        .desc = { "Adjust egg size." },
        .values = EggOptions_SizeText,
        .disable = 1,
        .OnChange = Egg_OnChangeSize
    },
    {
        .kind = OPTKIND_INT,
        .name = "Egg Spawn Velocity",
        .desc = { "Adjust egg spawn velocity on a scale of 0-10." },
        .val = 3,
        .format = "%d",
        .value_min = 0,
        .value_num = 11,
        .disable = 1
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fighter Collision",
        .desc = {"Toggle hitbox and hurtbox visualization.",
                 "Hurtboxes: yellow=hurt, purple=ungrabbable, blue=shield.",
                 "Hitboxes: (by priority) red, green, blue, purple."},
        .disable = 1,
        .OnChange = ChangeHitDisplay,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = { "Return to the Event Select Screen." },
        .OnSelect = Exit,
    },
};
static EventMenu Menu_Main = {
    .name = "Eggs-ercise",
    .option_num = countof(Options_Main),
    .options = Options_Main,
};

#endif // _EGGS_H_
