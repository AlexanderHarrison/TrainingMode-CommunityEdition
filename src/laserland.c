/* laserland.c */

#include "../MexTK/mex.h"
#include "events.h"

#define LL_SUCCESS_FRAMES_THRESHOLD 9

typedef struct LLData
{
    int success_count;
    int total_count;
    int frames_in_laserstart;
    int laserstart_counter;
    bool in_laserstart;
    bool in_f0_laserstart;
} LLData;

void Event_Exit(GOBJ *menu);

static char *panel_labels[3] = { "Last Attempt", "Timing", "Success Rate" };
static char attempt_text[24] = "-";
static char frames_text[24] = "-";
static char success_rate_text[24] = "-";
static char *panel_info[3] = {attempt_text, frames_text, success_rate_text};

// Setup a minimal menu with just an Exit option
static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_INFO,
        .name = "Help",
        .desc = {"To laser land, start a laser as the peak of your",
                 "double jump aligns with the height of a platform.",
                 "Done successfully, you will appear to warp onto the",
                 "platform with no landing lag."},
    },       
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Selection Screen."},
        .OnSelect = Event_Exit,
    },
};

static EventMenu LabMenu_Main = {
    .name = "Laser Land Training",
    .option_num = sizeof(Options_Main) / sizeof(EventOption),
    .options = Options_Main,
};

EventMenu *Event_Menu = &LabMenu_Main;

void LL_GX(GOBJ *gobj, int pass)
{
    if (pass != 2) return;
    event_vars->HUD_DrawInfoPanel((const char**)panel_labels, (const char**)panel_info, countof(panel_labels));
}

bool LL_InLaserstart(int current_state)
{
    /* fox aerial laser start & loop */
    return (current_state == 344 || current_state == 345);
}

void LL_FailWith(const char *msg)
{
    sprintf(attempt_text, "Failed");
    sprintf(frames_text, msg);
    SFX_PlayCommon(3);

}

void Event_Init(GOBJ *gobj)
{
    LLData *event_data = gobj->userdata;

    event_data->success_count = 0;
    event_data->total_count = 0;
    event_data->frames_in_laserstart = 0;
    event_data->laserstart_counter = 0;
    event_data->in_laserstart = false;
    event_data->in_f0_laserstart = false;

    GObj_AddGXLink(gobj, LL_GX, GXLINK_HUD, 80);
}

void Event_Think(GOBJ *event)
{
    LLData *event_data = event->userdata;

    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    int current_state = hmn_data->state_id;

    // if you leave laserstart and end up in wait instantly, you succeeded; anything else is a failure
    if ((!LL_InLaserstart(current_state) && event_data->in_laserstart) || (event_data->in_f0_laserstart && current_state == ASID_WAIT)) {
        event_data->total_count++;
        
        event_data->frames_in_laserstart = event_data->laserstart_counter;

        /* experiments suggest that laser lands slower than about 6f are worse than wavelands */
        /* but we'll be a bit more forgiving because a slightly slow landing is not really a failure */
        if (current_state == ASID_WAIT && event_data->frames_in_laserstart <= LL_SUCCESS_FRAMES_THRESHOLD) {
            event_data->success_count++;
            sprintf(attempt_text, "Success");
            sprintf(frames_text, event_data->in_f0_laserstart ? "Perfect" : "%df", event_data->frames_in_laserstart);
            SFX_PlayRaw(303, 255, 128, 20, 3);
        } else {
            if (event_data->frames_in_laserstart > LL_SUCCESS_FRAMES_THRESHOLD) {
                LL_FailWith("Early");
            } else {
                switch (current_state) {
                    case 346:
                        LL_FailWith("Early");
                        break;
                    case ASID_LANDING:
                        LL_FailWith("Bad height");
                        break;
                    default:
                        LL_FailWith("N/A");
                        break;
                }
            }
        }

        float success_rate = (float) event_data->success_count * 100.f / (float) event_data->total_count;
        sprintf(success_rate_text, "%d (%.2f%%)", event_data->success_count, success_rate);

        event_data->in_laserstart = false;
        event_data->laserstart_counter = 0;
    }

    event_data->in_f0_laserstart = false;

    if (LL_InLaserstart(current_state)) {
        event_data->in_laserstart = true;
        event_data->laserstart_counter++;
    // you can actually laserland on 'frame 0' of laser
    // which will skip state 344 completely and put you in wait
    } else if (hmn_data->input.down & HSD_BUTTON_B) {
        if (current_state == ASID_LANDING) {
            LL_FailWith("Late");
        }
        event_data->in_f0_laserstart = true;
    }
}

void Event_Exit(GOBJ *menu)
{
    stc_match->state = 3;
    Match_EndVS();
}
