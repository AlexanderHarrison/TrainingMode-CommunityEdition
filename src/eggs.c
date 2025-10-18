#include "../MexTK/mex.h"
#include "events.h"

static int in_free_practice = 0;

void Event_Init(GOBJ *gobj)
{
    
}

void Event_Think(GOBJ *event)
{
    // reset stale moves
    for (int i = 0; i < 6; i++)
    {
        // check if fighter exists
        GOBJ *fighter = Fighter_GetGObj(i);
        if (fighter != 0)
        {
            // reset stale move table
            int *staleMoveTable = Fighter_GetStaleMoveTable(i);
            memset(staleMoveTable, 0, 0x2C);
        }
    }

    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // on DPAD down set free practice on
    if (hmn_data->input.down & PAD_BUTTON_DPAD_DOWN && in_free_practice == 0) {
        OSReport("enter free practice\n");
        in_free_practice = 1;

        // play sound
        SFX_Play(0x82);

        // count timer up
        stc_match->match.timer = MATCH_TIMER_COUNTUP;
    }
}