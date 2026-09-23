#ifndef _EGGS_H_
#define _EGGS_H_

#include "../MexTK/mex.h"
#include "events.h"

enum options_main
{
    OPT_RETRY,
    OPT_FREEPRACTICE,
    OPT_DAMAGETHRESHOLD,
    OPT_SCALE,
    OPT_VELOCITY,
    OPT_COLLISION,
    OPT_HITBOXTRAILS,
    OPT_EXIT,

    OPT_COUNT
};

void StartFreePractice(GOBJ *gobj);
GOBJ *Egg_Spawn(void);
void Exit(GOBJ *menu);
void Retry(GOBJ *menu);
void Egg_OnChangeSize(GOBJ *menu, int value);
void ChangeHitDisplay(GOBJ *menu_gobj, int value);
void Event_Init(GOBJ *gobj);
float RandomRange(float low, float high);
int Egg_OnTakeDamage(GOBJ *gobj);
void Event_Think(GOBJ *event);
void Event_PostThink(GOBJ *event);
void HitboxTrails_GX(GOBJ *gobj, int pass);
void HitboxTrails_Think(void);

static GXColor text_gold = {255, 211, 0, 255};
static GXColor text_white = {255, 255, 255, 255};

static float EggOptions_Size[] = {1.0f, .5f, 2.0f};
static const char *EggOptions_SizeText[] = {"Normal", "Small", "Large"};

enum options_hitbox_trails
{
    OPT_HITBOXTRAILS_ENABLED,
    OPT_HITBOXTRAILS_DECAY,

    OPT_HITBOXTRAILS_COUNT
};

static const u8 HitboxTrailDecayConst[] = {15, 10, 30, 0};
static const u8 HitboxTrailDecayFactor[] = {4, 8, 2, 0};
static const char *HitboxTrailDecayText[] = {"Normal", "Fast", "Slow", "Off"};

static EventOption Options_HitboxTrails[OPT_HITBOXTRAILS_COUNT] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Enable",
        .desc = {"Enable hitbox trails."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = countof(HitboxTrailDecayText),
        .name = "Decay",
        .desc = {"How quickly the hitbox will fade away."},
        .values = HitboxTrailDecayText,
    },
};

static EventMenu Menu_HitboxTrails = {
    .name = "Hitbox Trails",
    .option_num = countof(Options_HitboxTrails),
    .options = Options_HitboxTrails,
};

static EventOption Options_Main[OPT_COUNT] = {
    {
        .kind = OPTKIND_FUNC,
        .name = "Retry",
        .desc = { "Retry this event. Pressing Z while paused also does this." },
        .OnSelect = Retry
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Enable Free Practice",
        .desc = { "Start Free Practice mode." },
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
        .value_num = countof(EggOptions_SizeText),
        .name = "Egg Scale",
        .desc = { "Adjust egg size." },
        .values = EggOptions_SizeText,
        .disable = 1,
        .OnChange = Egg_OnChangeSize
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Egg Spawn Velocity",
        .desc = { "Toggle whether eggs spawn with vertical velocity." },
        .val = 1,
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
        .kind = OPTKIND_MENU,
        .menu = &Menu_HitboxTrails,
        .name = "Hitbox Trails",
        .desc = {"Create a trail of your hitboxes to visualize spacing."},
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
