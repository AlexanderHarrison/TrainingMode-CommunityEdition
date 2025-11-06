#ifndef _REVERSAL_H_
#define _REVERSAL_H_

#include "../MexTK/mex.h"
#include "events.h"

void Exit(GOBJ *menu);

enum options_main
{
    OPT_CPU_ATTACK,
    OPT_PLAYER_DIRECTION,
    OPT_CPU_DIRECTION,
    OPT_EXIT,

    OPT_COUNT
};

enum attack
{
    RANDOM_SMASH,
    FSMASH,
    DSMASH,
    UPSMASH,
    RANDOM_AERIAL,
    FAIR,
    NAIR,
    DAIR,
    FTILT,
    DTILT,
    UPTILT,
    DASH_ATTACK,
};
static int Reversal_Attack[] = { RANDOM_SMASH, FSMASH, DSMASH, UPSMASH, RANDOM_AERIAL, FAIR, NAIR, DAIR, FTILT, DTILT, UPTILT, DASH_ATTACK };
static const char *Reversal_AttackText[] = { "Rand. Smash Attack", "FSmash", "DSmash", "UpSmash", "Rand. Aerial", "Fair", "Nair", "Dair", "FTilt", "DTilt", "UpTilt", "Dash Attack" };

enum direction
{
    FORWARD,
    BACKWARD
};
static int ReversalOption_Direction[] = { FORWARD, BACKWARD };
static const char *ReversalOption_DirectionText[] = { "Forward", "Backward" };

static EventOption Options_Main[OPT_COUNT] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = countof(Reversal_Attack),
        .name = "Attack",
        .desc = { "" },
        .values = Reversal_AttackText,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = countof(ReversalOption_Direction),
        .name = "P1 direction",
        .desc = { "" },
        .values = ReversalOption_DirectionText,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = countof(ReversalOption_Direction),
        .name = "Opponent direction",
        .desc = { "" },
        .values = ReversalOption_DirectionText,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = { "Return to the Event Select Screen." },
        .OnSelect = Exit,
    },
};

static EventMenu Menu_Main = {
    .name = "Reversal Training",
    .option_num = countof(Options_Main),
    .options = Options_Main,
};

#endif