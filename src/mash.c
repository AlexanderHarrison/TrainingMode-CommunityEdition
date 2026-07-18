#include "../MexTK/mex.h"
#include "events.h"

// // currently unused
// #define TEXT_X_POS          520
// #define TEXT_Y_POS_START    107
// #define TEXT_Y_DIST_HEADER_TO_DATA  13
// #define TEXT_Y_DIST_DATA_TO_NEXT    32
// #define TEXT_Y_DIST_DATA_TO_BEST    24
// #define TEXT_Y_DIST_BEST_TO_NEXT    TEXT_Y_DIST_DATA_TO_NEXT - TEXT_Y_DIST_DATA_TO_BEST
// #define TEXT_KERNING        1
// #define TEXT_ALIGN          0
// #define TEXT_USE_ASPECT     1
// #define TEXT_SCALE_HEADER   0.65
// #define TEXT_SCALE_DATA     0.35
// #define TEXT_SCALE_BEST     0.25

#define TEXT_BORDER_PASSES  8

static int canvas;

int prev_buttons_held = 0;
int prev_state = 0;
int prev_state_frame = 0;
int cur_cyclone_state_count = 0;
int prev_cyclone_state_count = 0;

bool has_touched_ground = false;
bool cannot_gain_height = false;
bool is_super_tornado = false;
bool saved_is_super_tornado = false;
bool is_new_high_score = false;
// bool is_repeating_cyclone = false;

// stats
int total_presses = 0;
float cur_presses_per_second = 0.0f;
float cur_height_gained = 0.0f;
float saved_presses_per_second = 0.0f;
float saved_height_gained = 0.0f;
float prev_height = 0.0f;

void Exit(GOBJ *menu);
void Mash_ChangeCamMode(GOBJ *menu_gobj, int value);
void Mash_ChangeCharacterRng_Misfire(GOBJ *menu_gobj, int value);

static const char *MashValues_CharacterRng_Misfire[] =
    { "Default", "Always Misfire", "Never Misfire" };
static const char *MashOptions_CamMode[] = {"Normal", "Zoom", "Fixed"};

enum options_main {
    OPT_INF_CHARGE,
    OPT_INF_JUMP,
    OPT_MISFIRE,
    OPT_CAMERA,
    OPT_HELP,
    OPT_EXIT,
    
    OPT_COUNT
};

static EventOption Options_Main[OPT_COUNT] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Infinite B-move Charge",
        .desc = { "Can mash forever." },
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Infinite Jumps",
        .desc = { "Can jump forever. Still worse than puff." },
        .val = 1,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "Misfire RNG",
        .desc = {"Toggle misfire's RNG."},
        .value_num = countof(MashValues_CharacterRng_Misfire),
        .values = MashValues_CharacterRng_Misfire,
        .OnChange = Mash_ChangeCharacterRng_Misfire,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(MashOptions_CamMode) / 4,
        .name = "Camera Mode",
        .desc = {"Adjust the camera's behavior."},
        .values = MashOptions_CamMode,
        .OnChange = Mash_ChangeCamMode,
    },
    {
        .kind = OPTKIND_INFO,
        .name = "Info",
        .desc =
            {"Luigi's mash window is from frame 6 to frame 43.",
             "Max B presses = 18, Max Presses/s = 30.00",
             "Max Height Gain = 40.727"},
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Save and Exit",
        .desc = 
            {"Saves your best score, and returns to the Event Select Screen.",
             "",
             "!!!!!  YOU MUST SELECT THIS IN ORDER TO SAVE  !!!!!" },
        .OnSelect = Exit,
    },
};

static EventMenu Menu_Main = {
    .name = "Luigi Mash",
    .option_num = countof(Options_Main),
    .options = Options_Main,
};

enum LuigiActionStates {
    ASID_CYCLONE_GROUND = 357,
    ASID_CYCLONE_AIR,  // 358,
    CYCLONE_FIRST_FRAME = 1,
    CYCLONE_FIRST_MASH_FRAME = 6,
    CYCLONE_LAST_MASH_FRAME = 43,
    CYCLONE_LAST_FRAME = 79,
};

enum Action {
    Action_None,
    Action_Wait,
    // Action_Cyclone_Air_Cant_Mash,
    Action_Cyclone_Air_Nopress,
    Action_Cyclone_Air_Press,
    Action_Cyclone_Air_Hold,
    // Action_Cyclone_Air_Cant_Plus_Hold,
    
    Action_Count
};

static char *action_names[] = {
    "None",
    "Wait",
    // "Can't Mash",
    "No Press",
    "B Press",
    "B Hold",
    // "Can't + B Hold",
};

static u8 action_log[48];
static u32 action_log_cur = countof(action_log); // start with log disabled

static GXColor action_colors[Action_Count] = {
    {0, 0, 0, 180},  // black - none
    {80, 80, 80, 180},  // light gray - wait (unused)
    // {0, 0, 0, 180},  // black - can't mash
    {40, 40, 40, 180},  // dark gray - no press
    {255, 0, 0, 180}, // red - press
    {214, 138, 138, 180}, // salmon red - held
    // {80, 40, 40, 180}, // dark red - held
};

void GX(GOBJ *gobj, int pass) {
    // hitbox GX
    if (pass == 1) {
        GOBJ *ft = Fighter_GetGObj(0);
        FighterData *ft_data = ft->userdata;
        
        static GXColor diffuse = {255, 0, 0, 100};
        static GXColor ambient = {255, 255, 255, 255};
        for (u32 i = 0; i < countof(ft_data->hitbox); ++i) {
            ftHit *hit = &ft_data->hitbox[i];
            if (hit->active)
                Develop_DrawSphere(hit->size, &hit->pos, &hit->pos_prev, &diffuse, &ambient);
        }
    }
    
    // action log gx
    if (pass == 2) {
        event_vars->HUD_DrawActionLogBar(
            action_log,
            action_colors,
            countof(action_log)
        );
        
        // skip none and wait states
        event_vars->HUD_DrawActionLogKey(
            &action_names[2],
            &action_colors[2],
            countof(action_names) - 2
        );
    }
}

void Mash_ChangeCamMode(GOBJ *menu_gobj, int value){
    // normal cam
    if (value == 0)
    {
        Match_SetNormalCamera();
    }
    // zoom cam
    else if (value == 1)
    {
        Match_SetFreeCamera(0, 3);
        stc_matchcam->freecam_fov.X = 140;
        stc_matchcam->freecam_rotate.Y = 10;
    }
    // fixed
    else if (value == 2 || value == 3)
    {
        Match_SetFixedCamera();
    }
    Match_CorrectCamera();
}

void Mash_ChangeCharacterRng_Misfire(GOBJ *menu_gobj, int value) {
    event_vars->rng->luigi_misfire = value;
}

void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

static GXColor text_gold = {255, 211, 0, 255};
static GXColor text_white = {255, 255, 255, 255};
static GXColor text_black = {0, 0, 0, 255};
 
// Sets all Text properties, now with borders
void setTextWithBorder(Text **textReference, int *canvas, float x, float y, float scale, char *string, ...) { 
    (*textReference) = Text_CreateText(2, *canvas);
    (*textReference)->kerning = 1;
    (*textReference)->align = 0;
    (*textReference)->use_aspect = 1;
    (*textReference)->aspect.X = 165;
    (*textReference)->hidden = false;

    float diagonal_border_offset = 1.0f;
    float cardinal_border_offset = diagonal_border_offset * 1.4142f;
    float cardinal_border_shift = diagonal_border_offset / 2; // makes cardinal border look wayyyy better for some reason

    // border texts
    Text_AddSubtext(*textReference, x-diagonal_border_offset, y-diagonal_border_offset, string);
    Text_AddSubtext(*textReference, x+diagonal_border_offset, y-diagonal_border_offset, string);
    Text_AddSubtext(*textReference, x-diagonal_border_offset, y+diagonal_border_offset, string);
    Text_AddSubtext(*textReference, x+diagonal_border_offset, y+diagonal_border_offset, string);
    Text_AddSubtext(*textReference, x-cardinal_border_offset+cardinal_border_shift, y+cardinal_border_shift, string);
    Text_AddSubtext(*textReference, x+cardinal_border_offset+cardinal_border_shift, y+cardinal_border_shift, string);
    Text_AddSubtext(*textReference, x+cardinal_border_shift, y-cardinal_border_offset+cardinal_border_shift, string);
    Text_AddSubtext(*textReference, x+cardinal_border_shift, y+cardinal_border_offset+cardinal_border_shift, string);
    
    // final text, centered
    Text_AddSubtext(*textReference, x, y, string);
    
    // set bordered text to black
    for (int i = 0; i < TEXT_BORDER_PASSES; ++i)
        Text_SetColor(*textReference, i, &text_black);
    // i = 8, the final text, is automatically white

    // set all text size and scale
    for (int i = 0; i < TEXT_BORDER_PASSES + 1; ++i) {
        Text_SetText(*textReference, i, string);
        Text_SetScale(*textReference, i, scale, scale);
    }
}

// All hud Text used. If you want more text, add to this.
Text *hud_presses_header_text, *hud_rate_header_text, *hud_height_header_text;
Text *hud_presses_text, *hud_rate_text, *hud_height_text;
Text *hud_best_height_text;
Text *arrow_1, *arrow_2; // points at first and last frame of cyclone mash window
// Text *debug_1, *debug_2;

void Event_Init(GOBJ *menu) {
    // init display gobj
    GOBJ *gobj = GObj_Create(0, 0, 6);
    GObj_AddGXLink(gobj, GX, 5, 21);
    canvas = Text_CreateCanvas(2, 0, 0, 0, 0, GXLINK_HUD, 81, 10);

    setTextWithBorder(&hud_presses_header_text, &canvas, 520.f, 117.f, 0.45f, "B Presses");
    setTextWithBorder(&hud_presses_text, &canvas, 520.f, 144.f, 0.70f, "0");

    setTextWithBorder(&hud_rate_header_text, &canvas, 520.f, 176.f, 0.45f, "Presses/s");
    setTextWithBorder(&hud_rate_text, &canvas, 520.f, 203.f, 0.70f, "0.00");

    setTextWithBorder(&hud_height_header_text, &canvas,  520.f, 235.f, 0.45f, "Height Gain");
    setTextWithBorder(&hud_height_text, &canvas, 520.f, 262.f, 0.70f, "0.000");

    setTextWithBorder(&hud_best_height_text, &canvas, 520.f, 280.f, 0.45f, "loading...");

    setTextWithBorder(&arrow_1, &canvas, 96.f, 76.f, 0.45f, "");
    setTextWithBorder(&arrow_2, &canvas, 532.f, 76.f, 0.45f, "");

    // setText(&debug_1, &canvas, 50, 234, 0.25, "-");
    // setText(&debug_2, &canvas, 50, 264, 0.25, "-");
}


void Event_Think(GOBJ *menu) {
    GOBJ *ft = Fighter_GetGObj(0);
    FighterData *ft_data = ft->userdata;

    int event_id = stc_memcard->EventBackup.event;
    bool is_event_played = Events_CheckIfEventWasPlayedYet(event_id);
    
    int cur_state = ft_data->state_id;
    int cur_action = Action_None;

    // Get controller inputs (for B button)
    int pad_index = Fighter_GetControllerPort(0);
    HSD_Pad *pad = PadGetMaster(pad_index);
    int cur_buttons_held = pad->held;

    // Infinite down-b charge
    if (Options_Main[OPT_INF_CHARGE].val == 1)
        ft_data->fighter_var.ft_var1 = 0; 

    // Infinite jumps
    if (Options_Main[OPT_INF_JUMP].val == 1)
        ft_data->jump.jumps_used = 0; 

    // If in cyclone
    if (!(ft_data->flags.hitlag) && (cur_state == ASID_CYCLONE_GROUND || cur_state == ASID_CYCLONE_AIR) ) {
        cur_cyclone_state_count++; // tracks combined air/ground cyclone state count

        // New cyclone
        if (ft_data->TM.state_frame == 0 && 
            ((prev_state != ASID_CYCLONE_GROUND && prev_state != ASID_CYCLONE_AIR) || 
            prev_cyclone_state_count == CYCLONE_LAST_FRAME) ) 
        {
            memset(action_log, Action_None, sizeof(action_log)); // colour in bar with default
            // memset(action_log + CYCLONE_FIRST_MASH_FRAME - 1, Action_Wait, CYCLONE_LAST_MASH_FRAME - CYCLONE_FIRST_MASH_FRAME + 1); // set mash window
            action_log_cur = 0;
            total_presses = 0;
            cur_presses_per_second = 0.0f;
            cur_height_gained = prev_height;
            has_touched_ground = false;
            is_super_tornado = false;
            cannot_gain_height = false;
            cur_cyclone_state_count = CYCLONE_FIRST_FRAME;
        }
        
        // Super cyclone check
        if (has_touched_ground && (cur_cyclone_state_count == CYCLONE_FIRST_MASH_FRAME - 1 && cur_state == ASID_CYCLONE_AIR))
            is_super_tornado = true;
        else if (cur_state == ASID_CYCLONE_GROUND && cur_cyclone_state_count >= CYCLONE_FIRST_MASH_FRAME) {
            cannot_gain_height = true;
            is_super_tornado = false;
        }

        // Partially grounded cyclone check
        if (cur_state == ASID_CYCLONE_GROUND)
            has_touched_ground = true;

        // Within mash window BUT IT DISPLAYS OUTSIDE OF MASH WINDOW AS WELL
        if (!cannot_gain_height && (cur_cyclone_state_count >= CYCLONE_FIRST_MASH_FRAME && cur_cyclone_state_count <= CYCLONE_LAST_MASH_FRAME) ) {
            if (cur_buttons_held & HSD_BUTTON_B && !(prev_buttons_held & HSD_BUTTON_B) ){
                cur_action = Action_Cyclone_Air_Press;
                total_presses++;
            } else if ( cur_buttons_held & HSD_BUTTON_B )
                cur_action = Action_Cyclone_Air_Hold;
            else
                cur_action = Action_Cyclone_Air_Nopress;
        } else if (cur_buttons_held & HSD_BUTTON_B) {
            cur_action = Action_Cyclone_Air_Hold;
        } else {
            cur_action = Action_Cyclone_Air_Nopress;
        }

        // // Looping down-b (wip, mostly just for testing future ideas)
        // if (cur_cyclone_state_count == CYCLONE_LAST_MASH_FRAME && cur_state == ASID_CYCLONE_AIR) {
        //     ActionStateChange(5, 1, 0, ft, ASID_CYCLONE_AIR, 0x40, 0);
        //     ft_data->figatree_curr->frame_num = 6;
        //     // ft_data->state.frame = 6;
        //     Fighter_UpdateStateFrameInfo(ft);
        //     is_repeating_cyclone = true;
        //     cur_cyclone_state_count = 6;
        // }
        // if (is_repeating_cyclone && cur_state == ASID_CYCLONE_AIR) {
        // }
    }

    // Update prev's
    prev_state = cur_state;
    prev_state_frame = ft_data->state.frame;
    prev_height = ft_data->phys.pos.Y;
    prev_cyclone_state_count = cur_cyclone_state_count;
    if (!(ft_data->flags.hitlag))
        prev_buttons_held = cur_buttons_held;

    // If dead
    if (ft_data->flags.dead)
        cur_cyclone_state_count = 0;

    // If in last frame of cyclone, save high score and update stats
    if (cur_cyclone_state_count == CYCLONE_LAST_FRAME) {
        cur_cyclone_state_count = 0;

        // Stats
        cur_height_gained = ft_data->phys.pos.Y - cur_height_gained;
        cur_presses_per_second = total_presses + (total_presses * (1 - 0.6333333)); // 0.6333333 = 38/60

        // Saved, used for display
        saved_presses_per_second = cur_presses_per_second;
        saved_height_gained = cur_height_gained;
        saved_is_super_tornado = is_super_tornado; 

        // Get high_score
        int high_score;
        if (has_touched_ground) // invalid cyclone
            high_score = 9999999; // higher than the highest possible cur_height_gained (will be ignored)
        else if (is_event_played) // valid cyclone
            high_score = Events_GetSavedScore(event_id); // grab real high score
        else { // valid cyclone, but event not played yet
            high_score = -9999999; // lower than the lowest possible cur_height_gained (will be overwritten)
            Events_SetEventAsPlayed(event_id);
        }

        // Convert cur_height_gained from float to int
        int current_score;
        if (cur_height_gained >= 0) 
            current_score = (int)(cur_height_gained * 1000.0f + 0.5f);
        else
            current_score = (int)(cur_height_gained * 1000.0f - 0.5f);
        
        // Save high score
        if (current_score > high_score) { 
            is_new_high_score = true;
            Events_StoreEventScore(event_id, current_score);
            high_score = current_score;
        } else if (is_event_played){
            is_new_high_score = false;
        }
    }

    // Display text every frame
    if (Pause_CheckStatus(1) != 2) { 
        // Grab int high_score, and separate into whole and fraction parts for displaying as a float
        int high_score = Events_GetSavedScore(event_id);
        int whole = high_score / 1000;
        int fraction = high_score % 1000;

        // loop needed to display the text border. A bit jank currently
        for (int i = 0; i < TEXT_BORDER_PASSES + 1; i++) {
            Text_SetText(hud_presses_header_text, i, "B Presses");
            Text_SetText(hud_presses_text, i, "%d", total_presses);
    
            Text_SetText(hud_rate_header_text, i, "Presses/s");
            Text_SetText(hud_rate_text, i, "%.2f", saved_presses_per_second);
    
            Text_SetText(hud_height_header_text, i, "Height Gain");
            Text_SetText(hud_height_text, i, "%.3f", saved_height_gained);
            if (saved_is_super_tornado && i == TEXT_BORDER_PASSES) {
                Text_SetColor(hud_height_header_text, i, &text_gold); // super tornado easter egg
            } else if (i == TEXT_BORDER_PASSES) {
                Text_SetColor(hud_height_header_text, i, &text_white); // default
            }
    
            if (is_event_played) {
                Text_SetText(hud_best_height_text, i, "Best: %s%d.%03d%s", 
                    (whole == 0 && fraction < 0) ? "-" : "", // if -0.something, then force negative symbol
                    whole, 
                    (fraction < 0) ? -fraction : fraction, // display only positive fraction
                    (whole == 40 && fraction == 727) ? "!!!" : ""); // max possible height gained easter egg
            } else {
                Text_SetText(hud_best_height_text, i, "Best: -");
            } 
            if (is_new_high_score && i == TEXT_BORDER_PASSES) {
                Text_SetColor(hud_best_height_text, i, &text_gold); // new high score
            } else if (i == TEXT_BORDER_PASSES) {
                Text_SetColor(hud_best_height_text, i, &text_white); // default
            }

            Text_SetText(arrow_1, i, "%d", CYCLONE_FIRST_MASH_FRAME);
            Text_SetText(arrow_2, i, "%d", CYCLONE_LAST_MASH_FRAME);

    
            // Text_SetText(debug_1, i, "%d", state_curr_debug);
            // Text_SetText(debug_2, i, "%d", state_prev_debug);
        }
        
    } else { // hide text if paused
        for (int i = 0; i < TEXT_BORDER_PASSES + 1; i++) {
            Text_SetText(hud_presses_header_text, i, "");
            Text_SetText(hud_presses_text, i, "");            
            Text_SetText(hud_rate_header_text, i, "");
            Text_SetText(hud_rate_text, i, "");
            Text_SetText(hud_height_header_text, i, "");
            Text_SetText(hud_height_text, i, "");
            Text_SetText(hud_best_height_text, i, "");
            Text_SetText(arrow_1, i, "");
            Text_SetText(arrow_2, i, "");
        }
    }

    // Update Bar using cur_action
    if (cur_cyclone_state_count >= 0 && (action_log_cur < countof(action_log))) {
        action_log[action_log_cur++] = cur_action;
    }
}

EventMenu *Event_Menu = &Menu_Main;