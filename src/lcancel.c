#include "lcancel.h"

static char *panel_labels[3] = { "Timing", "Fastfall", "Success Rate" };
static char timing_text[24] = "-";
static char fastfall_text[24] = "-";
static char success_rate_text[24] = "-";
static char *panel_info[3] = {timing_text, fastfall_text, success_rate_text};

static bool IsAerialState(int state_id);
static bool IsAerialLandingState(int state_id);
static bool IsEdgeCancelState(int state_id);
static bool IsAutoCancelLanding(FighterData *hmn_data);

// Main Menu
enum lcancel_option
{
    OPTLC_BARREL,
    OPTLC_BARREL_INTANGIBILITY_RATE,
    OPTLC_HUD,
    OPTLC_TIPS,
    OPTLC_HELP,
    OPTLC_EXIT,

    OPTLC_COUNT
};
static const char *LcOptions_Barrel[] = {"Off", "Stationary", "Move"};
static const char *LcOptions_Barrel_Intangibility[] = {"Off", "Rare", "Medium", "Often"};
static const u8 barrel_intangibility_duration[] = { 0, 40, 60, 80 };

static EventOption LcOptions_Main[OPTLC_COUNT] = {
    // Target
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LcOptions_Barrel) / 4,
        .name = "Target",
        .desc = {"Enable a target to attack. Use DPad down to",
                 "manually move it."},
        .values = LcOptions_Barrel,
        .OnChange = LCancel_ChangeBarrel,
    },
    // Target Intangibility
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LcOptions_Barrel_Intangibility) / 4,
        .name = "Target Intangibility",
        .desc = {"Target cycles intangibility to practice",
                 "L-cancel timing on both hit and whiff"},
        .values = LcOptions_Barrel_Intangibility,
    },
    // HUD
    {
        .kind = OPTKIND_TOGGLE,
        .val = 1,
        .name = "HUD",
        .desc = {"Toggle visibility of the HUD."},
    },
    // Tips
    {
        .kind = OPTKIND_TOGGLE,
        .val = 1,
        .name = "Tips",
        .desc = {"Toggle the onscreen display of tips."},
        .OnChange = Tips_Toggle,
    },
    // Help
    {
        .kind = OPTKIND_INFO,
        .name = "Help", // pointer to a string
        .desc = {"L-canceling is performed by pressing L, R, or Z up to ",
                 "7 frames before landing from a non-special aerial",
                 "attack. This will cut the landing lag in half, allowing ",
                 "you to act sooner after attacking."},
    },
    // Exit
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Selection Screen."},
        .OnSelect = Event_Exit,
    },
};
static EventMenu LabMenu_Main = {
    .name = "L-Cancel Training",
    .option_num = sizeof(LcOptions_Main) / sizeof(EventOption),
    .options = LcOptions_Main,
};

void LCancel_GX(GOBJ *gobj, int pass) {
    if (LcOptions_Main[OPTLC_HUD].val == 0) return;
    if (pass != 2) return;

    LCancelData *event_data = gobj->userdata;
    event_vars->HUD_DrawInfoPanel((const char**)panel_labels, (const char**)panel_info, countof(panel_labels));
    
    #define W 1.1f // width of square
    #define H 1.5f // height of square
    #define P 0.1f // amount of padding
    #define SX (-W * 27.f * 0.5f - P/2) // starting x
    #define SY 17.f // starting y

    #define RED {0xff, 0x60, 0x60, 0xd0}
    #define GREEN {0xb4, 0xff, 0xb4, 0xd0}
    
    static GXColor colors[28] = {
        // background
        {0, 0, 0, 0xc0},
        
        // early (10)
        RED, RED, RED, RED, RED, RED, RED, RED, RED, RED,
        
        // successful (7)
        GREEN, GREEN, GREEN, GREEN, GREEN, GREEN, GREEN,
        
        // late (10)
        RED, RED, RED, RED, RED, RED, RED, RED, RED, RED,
    };
    
    static Rect rects[28] = {
        {SX-P/2, SY, 27.f*W+P, H}, // background
        {SX+P/2+W*0, SY+P, W-P, H-P*2},
        {SX+P/2+W*1, SY+P, W-P, H-P*2},
        {SX+P/2+W*2, SY+P, W-P, H-P*2},
        {SX+P/2+W*3, SY+P, W-P, H-P*2},
        {SX+P/2+W*4, SY+P, W-P, H-P*2},
        {SX+P/2+W*5, SY+P, W-P, H-P*2},
        {SX+P/2+W*6, SY+P, W-P, H-P*2},
        {SX+P/2+W*7, SY+P, W-P, H-P*2},
        {SX+P/2+W*8, SY+P, W-P, H-P*2},
        {SX+P/2+W*9, SY+P, W-P, H-P*2},
        {SX+P/2+W*10, SY+P, W-P, H-P*2},
        {SX+P/2+W*11, SY+P, W-P, H-P*2},
        {SX+P/2+W*12, SY+P, W-P, H-P*2},
        {SX+P/2+W*13, SY+P, W-P, H-P*2},
        {SX+P/2+W*14, SY+P, W-P, H-P*2},
        {SX+P/2+W*15, SY+P, W-P, H-P*2},
        {SX+P/2+W*16, SY+P, W-P, H-P*2},
        {SX+P/2+W*17, SY+P, W-P, H-P*2},
        {SX+P/2+W*18, SY+P, W-P, H-P*2},
        {SX+P/2+W*19, SY+P, W-P, H-P*2},
        {SX+P/2+W*20, SY+P, W-P, H-P*2},
        {SX+P/2+W*21, SY+P, W-P, H-P*2},
        {SX+P/2+W*22, SY+P, W-P, H-P*2},
        {SX+P/2+W*23, SY+P, W-P, H-P*2},
        {SX+P/2+W*24, SY+P, W-P, H-P*2},
        {SX+P/2+W*25, SY+P, W-P, H-P*2},
        {SX+P/2+W*26, SY+P, W-P, H-P*2},
    };
    event_vars->HUD_DrawRects(rects, colors, countof(rects));

    Tri tris[countof(event_data->lrz_input_frame)*2];
    GXColor tri_color[countof(event_data->lrz_input_frame)*2];

    static float x_pos[countof(event_data->lrz_input_frame)] = {0};
    static int show_count = 0;

    if (event_data->success != 0) {
        show_count = event_data->lrz_input_count;

        // animate
        for (int i = 0; i < event_data->lrz_input_count; ++i) {
            int f = event_data->lrz_input_frame[i];
            int land = event_data->land_frame;
            float x = SX + ((float)(16 - (land - f)) + 0.5f) * W;

            float px = x_pos[i];
            float dx = x - px;
            if (fabs(dx) > 0.01f) 
                x = px + dx * 0.3f;
            x_pos[i] = x;
        }
    }

    for (int i = 0; i < show_count; ++i) {
        float x = x_pos[i];
        float y = SY - 0.8f;
        int f = event_data->lrz_input_frame[i];
        if (i != 0 && event_data->lrz_input_frame[i-1] == f)
            y -= H;
        #define B 0.3f
        #define B2 0.1f
        #define B3 0.15f
        tris[i*2+1][0] = (Vec2) {x, y};
        tris[i*2+1][1] = (Vec2) {x+W*0.3f, y-H*0.6f};
        tris[i*2+1][2] = (Vec2) {x-W*0.3f, y-H*0.6f};
        tri_color[i*2+1] = (GXColor) {255, 255, 255, 255};

        tris[i*2+0][0].X = tris[i*2+1][0].X;
        tris[i*2+0][0].Y = tris[i*2+1][0].Y + 0.3f;
        tris[i*2+0][1].X = tris[i*2+1][1].X + 0.15f;
        tris[i*2+0][1].Y = tris[i*2+1][1].Y - 0.1f;
        tris[i*2+0][2].X = tris[i*2+1][2].X - 0.15f;
        tris[i*2+0][2].Y = tris[i*2+1][2].Y - 0.1f;
        tri_color[i*2+0] = (GXColor) {0, 0, 0, 200};
    }

    event_vars->HUD_DrawTris(tris, tri_color, show_count * 2);

    static Rect early = { SX, SY + 3, 0, 0 };
    static Rect timing0 = { 0, SY + 4.5f, 0, 0 };
    static Rect timing1 = { 0, SY + 3, 0, 0 };
    static Rect late = { -SX, SY + 3, 0, 0 };
    event_vars->HUD_DrawText("Early", &early, 0.45f);
    event_vars->HUD_DrawText("L-Cancel", &timing0, 0.4f);
    event_vars->HUD_DrawText("Successful", &timing1, 0.4f);
    event_vars->HUD_DrawText("Late", &late, 0.45f);
}

// Init Function
void Event_Init(GOBJ *gobj)
{
    LCancelData *event_data = gobj->userdata;

    // initialize barrel intangible timer
    event_data->barrel_intangible_timer = 0;

    GObj_AddGXLink(gobj, LCancel_GX, GXLINK_HUD, 80);
}

// Think Function
void Event_Think(GOBJ *event)
{
    LCancelData *event_data = event->userdata;

    // get fighter data
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // set infinite shields
    hmn_data->shield.health = 60;

    LcOptions_Main[OPTLC_BARREL_INTANGIBILITY_RATE].disable = (LcOptions_Main[OPTLC_BARREL].val == 0) ? 1 : 0;

    LCancel_Think(event_data, hmn_data);
    Barrel_Think(event_data);
}
void Event_Exit(GOBJ *menu)
{
    // end game
    stc_match->state = 3;

    // cleanup
    Match_EndVS();
}

void LCancel_ChangeBarrel(GOBJ *menu_gobj, int value) {
    LcOptions_Main[OPTLC_BARREL_INTANGIBILITY_RATE].disable = (value == 0) ? 1 : 0;
}

void LCancel_Think(LCancelData *event_data, FighterData *hmn_data)
{
    // run tip logic
    if (LcOptions_Main[OPTLC_TIPS].val)
        Tips_Think(event_data, hmn_data);
    
    bool should_update_fastfall_hud = false;
    int state = hmn_data->state_id;
    int state_frame = hmn_data->TM.state_frame;
    
    // reset
    if (IsAerialState(state) && state_frame == 0) {
        event_data->lcancel = 0;
        event_data->success = 0;
        event_data->land_frame = 0;
        event_data->cur_frame = 0;
        event_data->lrz_input_count = 0;
    }

    // log lrz inputs
    if (IsAerialState(state) || IsAerialLandingState(state)) {
        int cur_frame = ++event_data->cur_frame;
        if (event_data->lrz_input_count < (int)countof(event_data->lrz_input_frame) && (hmn_data->input.down & PAD_TRIGGER_L))
            event_data->lrz_input_frame[event_data->lrz_input_count++] = cur_frame;
        if (event_data->lrz_input_count < (int)countof(event_data->lrz_input_frame) && (hmn_data->input.down & PAD_TRIGGER_R))
            event_data->lrz_input_frame[event_data->lrz_input_count++] = cur_frame;
        if (event_data->lrz_input_count < (int)countof(event_data->lrz_input_frame) && (hmn_data->input.down & PAD_TRIGGER_Z))
            event_data->lrz_input_frame[event_data->lrz_input_count++] = cur_frame;
    }

    // log fastfall frame
    if (hmn_data->phys.air_state)
        event_data->fastfell = hmn_data->flags.is_fastfall;
    if (hmn_data->phys.self_vel.Y >= 0)
        event_data->fastfall_frame = 0;
    if (hmn_data->phys.self_vel.Y < 0 && !event_data->fastfell)
        event_data->fastfall_frame++;

    if (IsAerialLandingState(hmn_data->state_id) && hmn_data->state.frame == 0) {
        // Record L input timing on the first frame of aerial landing
        bool lcancel = hmn_data->input.timer_trigger_any_ignore_hitlag < 7;
        event_data->lcancel = lcancel ? 1 : -1;
        event_data->land_frame = event_data->cur_frame;
    }

    bool has_horizontal_velocity = hmn_data->phys.self_vel.X != 0;
    bool is_success_l_input = event_data->lcancel > 0;
    bool was_previous_state_aerial_landing = IsAerialLandingState(hmn_data->TM.state_prev[0]);
    bool is_success_l_cancel = IsAerialLandingState(hmn_data->state_id) && is_success_l_input;

    bool is_missed_l_cancel = IsAerialLandingState(hmn_data->state_id)
        && !is_success_l_input
        // If there's no horizontal velocity then it's not possible to edge cancel and a miss is confirmed
        // OR if the number of landing frames has exceeded the number of frames of the l-cancelled animation then it's considered a miss (even if an edge cancel can still occur)
        && (!has_horizontal_velocity || ((hmn_data->state.frame + 1) > (hmn_data->figatree_curr->frame_num / 2)));

    bool is_edge_cancel = IsEdgeCancelState(hmn_data->state_id)
        && was_previous_state_aerial_landing;

    if (event_data->success == 0 && (is_success_l_cancel || is_missed_l_cancel || is_edge_cancel)) {
        // increment total lcls
        event_data->total_attempts++;

        // determine success
        if (is_success_l_cancel || is_edge_cancel) {
            event_data->successful_attempts++;
            event_data->success = 1;
            SFX_PlayRaw(303, 255, 128, 20, 3);
        }
        else {
            event_data->success = -1;
            SFX_PlayCommon(3);
        }

        // update timing text
        if (is_edge_cancel) {
            sprintf(timing_text, "EC %df", hmn_data->TM.state_prev_frames[0]);
        }
        else if (event_data->lrz_input_count == 0) {
            sprintf(timing_text, "No Press");
        }
        else {
            int land_f = event_data->land_frame;
            int lcancel_f = land_f - event_data->lrz_input_frame[0];
            for (int i = 1; i < event_data->lrz_input_count; ++i) {
                int new_lcancel_f = event_data->lrz_input_frame[i] - land_f;
                if (new_lcancel_f < 0) break;
                lcancel_f = new_lcancel_f;
            }
            sprintf(timing_text, "%df/7f", lcancel_f + 1);
        }

        should_update_fastfall_hud = true;

        // update success rate text
        int successful = event_data->successful_attempts;
        int total = event_data->total_attempts;
        float success_rate = (float)successful * 100.f / (float)total;
        sprintf(success_rate_text, "%d (%.2f%%)", successful, success_rate);
    } 

    if (IsAutoCancelLanding(hmn_data)) {
        sprintf(timing_text, "Auto-canceled");
        should_update_fastfall_hud = true;
    }

    if (should_update_fastfall_hud) {
        // Print airborne frames
        if (event_data->fastfell)
            sprintf(fastfall_text, "%df", event_data->fastfall_frame);
        else
            sprintf(fastfall_text, "-");
    }
}

// Tips Functions
void Tips_Toggle(GOBJ *menu_gobj, int value)
{
    // destroy existing tips when disabling
    if (value == 1)
        event_vars->Tip_Destroy();
}

void Tips_Think(LCancelData *event_data, FighterData *hmn_data)
{
    // look for a freshly buffered guard off
    if (((hmn_data->state_id == ASID_GUARDOFF) && (hmn_data->TM.state_frame == 0)) &&                               // currently in guardoff first frame
        (hmn_data->TM.state_prev[0] == ASID_GUARD) &&                                                            // was just in wait
        ((hmn_data->TM.state_prev[3] >= ASID_LANDINGAIRN) && (hmn_data->TM.state_prev[3] <= ASID_LANDINGAIRLW))) // was in aerial landing a few frames ago
    {
        // increment condition count
        event_data->tip.shield_num++;

        // if condition met X times, show tip
        if (event_data->tip.shield_num >= 3)
        {
            // display tip
            char *shield_string = "Tip:\nDon't hold the trigger! Quickly \npress and release to prevent \nshielding after landing.";
            if (event_vars->Tip_Display(5 * 60, shield_string))
                event_data->tip.shield_num = 0;
        }
    }

    // late tip
    if ((hmn_data->state_id >= ASID_LANDINGAIRN) && (hmn_data->state_id <= ASID_LANDINGAIRLW) && // is in aerial landing
        (event_data->success == -1) &&                                                     // failed the l-cancel
        (hmn_data->input.down & (HSD_TRIGGER_L | HSD_TRIGGER_R | HSD_TRIGGER_Z)))          // was late for an l-cancel by pressing it just now
    {
        // increment condition count
        event_data->tip.late_num++;

        // if condition met X times, show tip
        if (event_data->tip.late_num >= 3)
        {
            // display tip
            char *late_string = "Tip:\nPress the trigger a\nbit earlier, before the\nfighter lands.";
            if (event_vars->Tip_Display(5 * 60, late_string))
                event_data->tip.late_num = 0;
        }
    }
}

// Barrel Functions
static void UpdateBarrelIntangibility(LCancelData *event_data, GOBJ *barrel_gobj)
{
    if (LcOptions_Main[OPTLC_BARREL_INTANGIBILITY_RATE].val != 0)
    {
        int timer_cycle = 120;
        event_data->barrel_intangible_timer++;
        if (event_data->barrel_intangible_timer >= timer_cycle)
            event_data->barrel_intangible_timer = 0;

        // determine intangibility duration based on option value
        int intangibility_duration = barrel_intangibility_duration[LcOptions_Main[OPTLC_BARREL_INTANGIBILITY_RATE].val];
        // the timer is lower than intangibility_duration = intangible(2), rest = tangible(0)
        int tangibility = (event_data->barrel_intangible_timer < intangibility_duration) ? 2 : 0; 
        Item_SetHurtboxTangibility(barrel_gobj, tangibility);
    }
    else
    {
        Item_SetHurtboxTangibility(barrel_gobj, 0);
    }
}

void Barrel_Think(LCancelData *event_data)
{
    GOBJ *barrel_gobj = event_data->barrel_gobj;

    switch (LcOptions_Main[OPTLC_BARREL].val)
    {
    case (0): // off
    {
        // if spawned, remove
        if (barrel_gobj != 0)
        {
            Item_Destroy(barrel_gobj);
            event_data->barrel_gobj = 0;
        }

        // reset intangible timer when target is off
        event_data->barrel_intangible_timer = 0;

        break;
    }
    case (1): // stationary
    {
        // if not spawned, spawn
        if (barrel_gobj == 0)
        {
            // spawn barrel at center stage
            barrel_gobj = Barrel_Spawn(0);
            event_data->barrel_gobj = barrel_gobj;
        }

        ItemData *barrel_data = barrel_gobj->userdata;
        barrel_data->can_hold = 0;

        UpdateBarrelIntangibility(event_data, barrel_gobj);

        // check to move barrel
        // get fighter data
        GOBJ *hmn = Fighter_GetGObj(0);
        FighterData *hmn_data = hmn->userdata;
        if (hmn_data->input.down & PAD_BUTTON_DPAD_DOWN)
        {
            // ensure player is grounded
            int isGround = 0;
            if (hmn_data->phys.air_state == 0)
            {

                // check for ground in front of player
                Vec3 coll_pos;
                int line_index;
                int line_kind;
                Vec3 line_unk;
                float fromX = (hmn_data->phys.pos.X) + (hmn_data->facing_direction * 16);
                float toX = fromX;
                float fromY = (hmn_data->phys.pos.Y + 5);
                float toY = fromY - 10;
                isGround = GrColl_RaycastGround(&coll_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, fromX, fromY, toX, toY, 0);
                if (isGround == 1)
                {

                    // update last pos
                    event_data->barrel_lastpos = coll_pos;

                    // place barrel here
                    barrel_data->pos = coll_pos;
                    barrel_data->coll_data.ground_index = line_index;

                    // update ECB
                    barrel_data->coll_data.topN_Curr = barrel_data->pos; // move current ECB location to new position
                    Coll_ECBCurrToPrev(&barrel_data->coll_data);
                    barrel_data->cb.coll(barrel_gobj);

                    SFX_Play(221);
                }
                else
                {
                    // play SFX
                    SFX_PlayCommon(3);
                }
            }
        }
        break;
    }
    case (2): // move
    {
        // if not spawned, spawn
        if (barrel_gobj == 0)
        {
            // spawn barrel at center stage
            barrel_gobj = Barrel_Spawn(1);
            event_data->barrel_gobj = barrel_gobj;
        }

        ItemData *barrel_data = barrel_gobj->userdata;
        barrel_data->can_hold = 0;
        barrel_data->can_nudge = 0;

        UpdateBarrelIntangibility(event_data, barrel_gobj);

        break;
    }
    }
}
GOBJ *Barrel_Spawn(int pos_kind)
{

    LCancelData *event_data = event_vars->event_gobj->userdata;
    Vec3 *barrel_lastpos = &event_data->barrel_lastpos;

    // determine position to spawn
    Vec3 pos;
    pos.Z = 0;
    switch (pos_kind)
    {
    case (0): // center stage
    {
        // get position
        int line_index;
        int line_kind;
        Vec3 line_angle;
        float from_x = 0;
        float to_x = from_x;
        float from_y = 6;
        float to_y = from_y - 1000;
        int is_ground = GrColl_RaycastGround(&pos, &line_index, &line_kind, &line_angle, -1, -1, -1, 0, from_x, from_y, to_x, to_y, 0);
        if (is_ground == 0)
            goto BARREL_RANDPOS;
        break;
    }
    case (1): // random pos
    {
    BARREL_RANDPOS:
    {

        // get position
        int line_index;
        int line_kind;
        Vec3 line_angle;
        float from_x = Stage_GetCameraLeft() + (HSD_Randi(Stage_GetCameraRight() - Stage_GetCameraLeft())) + HSD_Randf();
        float to_x = from_x;
        float from_y = Stage_GetCameraBottom() + (HSD_Randi(Stage_GetCameraTop() - Stage_GetCameraBottom())) + HSD_Randf();
        float to_y = from_y - 1000;
        int is_ground = GrColl_RaycastGround(&pos, &line_index, &line_kind, &line_angle, -1, -1, -1, 0, from_x, from_y, to_x, to_y, 0);
        if (is_ground == 0)
            goto BARREL_RANDPOS;

        // ensure it isnt too close to the previous
        float distance = Math_Vec3Distance(&pos, barrel_lastpos);
        if (distance < 25.f)
            goto BARREL_RANDPOS;

        // ensure left and right have ground
        Vec3 near_pos;
        float near_fromX = pos.X + 8;
        float near_fromY = pos.Y + 4;
        to_y = near_fromY - 4;
        is_ground = GrColl_RaycastGround(&near_pos, &line_index, &line_kind, &line_angle, -1, -1, -1, 0, near_fromX, near_fromY, near_fromX, to_y, 0);
        if (is_ground == 0)
            goto BARREL_RANDPOS;
        near_fromX = pos.X - 8;
        is_ground = GrColl_RaycastGround(&near_pos, &line_index, &line_kind, &line_angle, -1, -1, -1, 0, near_fromX, near_fromY, near_fromX, to_y, 0);
        if (is_ground == 0)
            goto BARREL_RANDPOS;

        break;
    }
    }
    }

    // spawn item
    SpawnItem spawnItem;
    spawnItem.parent_gobj = 0;
    spawnItem.parent_gobj2 = 0;
    spawnItem.it_kind = ITEM_BARREL;
    spawnItem.hold_kind = 0;
    spawnItem.unk2 = 0;
    spawnItem.pos = pos;
    spawnItem.pos2 = pos;
    spawnItem.vel.X = 0;
    spawnItem.vel.Y = 0;
    spawnItem.vel.Z = 0;
    spawnItem.facing_direction = 1;
    spawnItem.damage = 0;
    spawnItem.unk5 = 0;
    spawnItem.unk6 = 0;
    spawnItem.is_raycast_below = 1;
    spawnItem.is_spin = 0;
    GOBJ *barrel_gobj = Item_CreateItem2(&spawnItem);
    Item_CollAir_Bounce(barrel_gobj, Barrel_Null);

    static const void *item_callbacks[] = {
        (void *)0x803f58e0,
        (void *)0x80287458,
        Barrel_OnDestroy, // onDestroy
        (void *)0x80287e68,
        (void *)0x80287ea8,
        (void *)0x80287ec8,
        (void *)0x80288818,
        Barrel_OnHurt, // onhurt
        (void *)0x802889f8,
        (void *)0x802888b8,
        (void *)0x00000000,
        (void *)0x00000000,
        (void *)0x80288958,
        (void *)0x80288c68,
        (void *)0x803f5988,
    };

    // replace collision callback
    ItemData *barrel_data = barrel_gobj->userdata;
    memcpy(barrel_data->it_func, item_callbacks, sizeof(item_callbacks));
    barrel_data->camera_subject->kind = 0;

    // update last barrel pos
    event_data->barrel_lastpos = pos;

    return barrel_gobj;
}
void Barrel_Null(void)
{
    return; // Do nothing on purpose
}
void Barrel_Break(GOBJ *barrel_gobj)
{
    ItemData *barrel_data = barrel_gobj->userdata;
    Effect_SpawnSync(1063, barrel_gobj, &barrel_data->pos);
    SFX_Play(251);
    ScreenRumble_Execute(2, &barrel_data->pos);
    JOBJ *barrel_jobj = barrel_gobj->hsd_object;
    JOBJ_SetFlagsAll(barrel_jobj, JOBJ_HIDDEN);
    barrel_data->xd0c = 2;
    barrel_data->self_vel.X = 0;
    barrel_data->self_vel.Y = 0;
    barrel_data->item_var.var1 = 1;
    barrel_data->item_var.var2 = 40;
    barrel_data->xdcf3 = 1;
    ItemStateChange(barrel_gobj, 7, 2);
}
int Barrel_OnHurt(GOBJ *barrel_gobj)
{
    if (LcOptions_Main[OPTLC_BARREL].val != 2) // move
        return 0;

    LCancelData *event_data = event_vars->event_gobj->userdata;
    Barrel_Break(event_data->barrel_gobj);

    // spawn new barrel at a random position
    barrel_gobj = Barrel_Spawn(1);
    event_data->barrel_gobj = barrel_gobj;

    return 0;
}
int Barrel_OnDestroy(GOBJ *barrel_gobj)
{
    LCancelData *event_data = event_vars->event_gobj->userdata;

    // if this barrel is still the current barrel
    if (barrel_gobj == event_data->barrel_gobj)
        event_data->barrel_gobj = 0;

    return 0;
}

static bool IsAerialState(int state_id) {
    return ASID_ATTACKAIRN <= state_id && state_id <= ASID_ATTACKAIRLW;
}

static bool IsAerialLandingState(int state_id) {
    return ASID_LANDINGAIRN <= state_id && state_id <= ASID_LANDINGAIRLW;
}

static bool IsEdgeCancelState(int state_id) {
    return state_id == ASID_FALL || state_id == ASID_OTTOTTO;
}

static bool IsAutoCancelLanding(FighterData *hmn_data) {
    return hmn_data->state_id == ASID_LANDING
        && hmn_data->TM.state_frame == 0                   // if first frame of landing
        && hmn_data->TM.state_prev[0] >= ASID_ATTACKAIRN
        && hmn_data->TM.state_prev[0] <= ASID_ATTACKAIRLW; // came from aerial attack
}

// Initial Menu
EventMenu *Event_Menu = &LabMenu_Main;
