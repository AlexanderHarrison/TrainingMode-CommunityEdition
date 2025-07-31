#include "lcancel.h"

// Main Menu
enum lcancel_option
{
    OPTLC_BARREL,
    OPTLC_HUD,
    OPTLC_TIPS,
    OPTLC_HELP,
    OPTLC_EXIT,

    OPTLC_COUNT
};
static const char *LcOptions_Barrel[] = {"Off", "Stationary", "Move"};
static EventOption LcOptions_Main[OPTLC_COUNT] = {
    // Target
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LcOptions_Barrel) / 4,
        .name = "Target",
        .desc = {"Enable a target to attack. Use DPad down to",
                 "manually move it."},
        .values = LcOptions_Barrel,
    },
    // HUD
    {
        .kind = OPTKIND_TOGGLE,
        .val = 1,
        .name = "HUD",
        .desc = {"Toggle visibility of the HUD."},
        .OnChange = LCancel_ChangeShowHUD,
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

// Init Function
void Event_Init(GOBJ *gobj)
{
    LCancelData *event_data = gobj->userdata;

    // get l-cancel assets
    event_data->lcancel_assets = Archive_GetPublicAddress(event_vars->event_archive, "lcancel");

    // create HUD
    LCancel_Init(event_data);

    // set CPU AI to no_act 15
    //cpu_data->cpu.ai = 0;
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

// L-Cancel functions
void LCancel_Init(LCancelData *event_data)
{
    GOBJ *hud_gobj = GObj_Create(0, 0, 0);
    event_data->hud.gobj = hud_gobj;
    // Load jobj
    JOBJ *hud_jobj = JOBJ_LoadJoint(event_data->lcancel_assets->hud);
    GObj_AddObject(hud_gobj, 3, hud_jobj);
    GObj_AddGXLink(hud_gobj, GXLink_Common, GXLINK_HUD, 80);

    // create text canvas
    int canvas = Text_CreateCanvas(2, hud_gobj, 14, 15, 0, GXLINK_HUD, 81, 19);
    event_data->hud.canvas = canvas;

    // init text
    Text **text_arr = &event_data->hud.text_time;
    for (int i = 0; i < 3; i++)
    {

        // Create text object
        Text *hud_text = Text_CreateText(2, canvas);
        text_arr[i] = hud_text;
        hud_text->kerning = 1;
        hud_text->align = 1;
        hud_text->use_aspect = 1;

        // Get position
        Vec3 text_pos;
        JOBJ *text_jobj;
        JOBJ_GetChild(hud_jobj, &text_jobj, 2 + i, -1);
        JOBJ_GetWorldPosition(text_jobj, 0, &text_pos);

        // adjust scale
        Vec3 *scale = &hud_jobj->scale;
        // text scale
        hud_text->viewport_scale.X = (scale->X * 0.01) * LCLTEXT_SCALE;
        hud_text->viewport_scale.Y = (scale->Y * 0.01) * LCLTEXT_SCALE;
        hud_text->aspect.X = 165;

        // text position
        hud_text->trans.X = text_pos.X + (scale->X / 4.0);
        hud_text->trans.Y = (text_pos.Y * -1) + (scale->Y / 4.0);

        // dummy text
        Text_AddSubtext(hud_text, 0, 0, "-");
    }

    // save initial arrow position
    JOBJ *arrow_jobj;
    JOBJ_GetChild(hud_jobj, &arrow_jobj, LCLARROW_JOBJ, -1);
    event_data->hud.arrow_base_x = arrow_jobj->trans.X;
    event_data->hud.arrow_timer = 0;
    event_data->is_current_aerial_counted = false;
    arrow_jobj->trans.X = 0;
    JOBJ_SetFlags(arrow_jobj, JOBJ_HIDDEN);
}
void LCancel_ChangeShowHUD(GOBJ *menu_gobj, int show) {
    HUDCamData *cam = event_vars->hudcam_gobj->userdata;
    cam->hide = !show;
}
void LCancel_Think(LCancelData *event_data, FighterData *hmn_data)
{

    // run tip logic
    if (LcOptions_Main[OPTLC_TIPS].val)
        Tips_Think(event_data, hmn_data);

    JOBJ *hud_jobj = event_data->hud.gobj->hsd_object;

    bool should_update_fastfall_hud = false;
    bool can_fastfall = hmn_data->phys.self_vel.Y < 0;

    // log fastfall frame
    int state = hmn_data->state_id;
    if (can_fastfall)
    {
        // did i fastfall yet?
        if (hmn_data->flags.is_fastfall)
            event_data->is_fastfall = true; // set as fastfall this session
        else
            event_data->fastfall_frame++; // increment frames
    }

    if (IsAerialLandingState(hmn_data->state_id) && hmn_data->state.frame == 0) {
        // Record L input timing on the first frame of aerial landing
        event_data->current_l_input_timing = hmn_data->input.timer_trigger_any_ignore_hitlag;
    }

    bool has_horizontal_velocity = hmn_data->phys.self_vel.X != 0;
    bool is_success_l_input = event_data->current_l_input_timing < 7;
    bool was_previous_state_aerial_landing = IsAerialLandingState(hmn_data->TM.state_prev[0]);

    bool is_success_l_cancel = IsAerialLandingState(hmn_data->state_id) && is_success_l_input;

    bool is_missed_l_cancel = IsAerialLandingState(hmn_data->state_id)
        && !is_success_l_input
        // If there's no horizontal velocity then it's not possible to edge cancel and a miss is confirmed
        // OR if the number of landing frames has exceeded the number of frames of the l-cancelled animation then it's considered a miss (even if an edge cancel can still occur)
        && (!has_horizontal_velocity || ((hmn_data->state.frame + 1) > (hmn_data->figatree_curr->frame_num / 2)));

    bool is_edge_cancel = IsEdgeCancelState(hmn_data->state_id)
        && was_previous_state_aerial_landing;

    if (!event_data->is_current_aerial_counted && (is_success_l_cancel || is_missed_l_cancel || is_edge_cancel))
    {
        event_data->is_current_aerial_counted = true;

        // increment total lcls
        event_data->hud.lcl_total++;

        // determine succession
        bool is_fail = true;
        if (is_success_l_cancel || is_edge_cancel)
        {
            is_fail = false;
            event_data->hud.lcl_success++;
        }
        event_data->is_fail = is_fail; // save l-cancel bool

        // Play appropriate sfx
        if (is_fail)
            SFX_PlayCommon(3);
        else
            SFX_PlayRaw(303, 255, 128, 20, 3);

        // update timing text
        if (is_edge_cancel)
        {
            Text_SetText(event_data->hud.text_time, 0, "EC %df", hmn_data->TM.state_prev_frames[0]);
        }
        else if (event_data->current_l_input_timing > MAX_L_PRESS_TIMING)
        {
            Text_SetText(event_data->hud.text_time, 0, "No Press");
        }
        else
        {
            Text_SetText(event_data->hud.text_time, 0, "%df/7f", event_data->current_l_input_timing + 1);
        }
        int frame_box_id = min(event_data->current_l_input_timing, MAX_L_PRESS_TIMING);

        // update arrow
        JOBJ *arrow_jobj;
        JOBJ_GetChild(hud_jobj, &arrow_jobj, LCLARROW_JOBJ, -1);
        event_data->hud.arrow_prevpos = arrow_jobj->trans.X;
        event_data->hud.arrow_nextpos = event_data->hud.arrow_base_x - (frame_box_id * LCLARROW_OFFSET);
        JOBJ_ClearFlags(arrow_jobj, JOBJ_HIDDEN);
        event_data->hud.arrow_timer = LCLARROW_ANIMFRAMES;

        should_update_fastfall_hud = true;

        // Print succession
        int successful = event_data->hud.lcl_success;
        float succession = ((float)event_data->hud.lcl_success / (float)event_data->hud.lcl_total) * 100.0;
        Text_SetText(event_data->hud.text_scs, 0, "%d (%.2f%)", successful, succession);

        // Play HUD anim
        JOBJ_RemoveAnimAll(hud_jobj);
        JOBJ_AddAnimAll(hud_jobj, 0, event_data->lcancel_assets->hudmatanim[is_fail], 0);
        JOBJ_ReqAnimAll(hud_jobj, 0);
    } 
    else if (!IsAerialLandingState(hmn_data->state_id) && !IsEdgeCancelState(hmn_data->state_id))
    {
        event_data->is_current_aerial_counted = false;
    }

    if (IsAutoCancelLanding(hmn_data))
    {
        // state as autocancelled
        Text_SetText(event_data->hud.text_time, 0, "Auto-canceled");

        should_update_fastfall_hud = true;

        // Play HUD anim
        JOBJ_RemoveAnimAll(hud_jobj);
        JOBJ_AddAnimAll(hud_jobj, 0, event_data->lcancel_assets->hudmatanim[2], 0);
        JOBJ_ReqAnimAll(hud_jobj, 0);
    }

    // update arrow animation
    if (event_data->hud.arrow_timer > 0)
    {
        // decrement timer
        event_data->hud.arrow_timer--;

        // get this frames position
        float time = 1 - ((float)event_data->hud.arrow_timer / (float)LCLARROW_ANIMFRAMES);
        float xpos = smooth_lerp(time, event_data->hud.arrow_prevpos, event_data->hud.arrow_nextpos);

        // update position
        JOBJ *arrow_jobj;
        JOBJ_GetChild(hud_jobj, &arrow_jobj, LCLARROW_JOBJ, -1); // get timing bar jobj
        arrow_jobj->trans.X = xpos;
        JOBJ_SetMtxDirtySub(arrow_jobj);
    }

    if (should_update_fastfall_hud) {
        // Print airborne frames
        if (event_data->is_fastfall)
            Text_SetText(event_data->hud.text_air, 0, "%df", event_data->fastfall_frame - 1);
        else
            Text_SetText(event_data->hud.text_air, 0, "-");
    }

    if (!can_fastfall && !IsAerialLandingState(state)) // cant fastfall, reset frames
    {
        event_data->fastfall_frame = 0;
        event_data->is_fastfall = false;
    }

    // update HUD anim
    JOBJ_AnimAll(hud_jobj);
}
void LCancel_HUDCamThink(GOBJ *gobj)
{

    // if HUD enabled and not paused
    if ((LcOptions_Main[OPTLC_HUD].val) && (Pause_CheckStatus(1) != 2))
    {
        CObjThink_Common(gobj);
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

    // shield tip
    if (event_data->tip.shield_isdisp == 0) // if not shown
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
                {
                    // set as shown
                    //event_data->tip.shield_isdisp = 1;
                    event_data->tip.shield_num = 0;
                }
            }
        }
    }

    // hitbox tip
    if (event_data->tip.hitbox_isdisp == 0) // if not shown
    {
        // update hitbox active bool
        if ((hmn_data->state_id >= ASID_ATTACKAIRN) && (hmn_data->state_id <= ASID_ATTACKAIRLW)) // check if currently in aerial attack)                                                      // check if in first frame of aerial attack
        {

            // reset hitbox bool on first frame of aerial attack
            if (hmn_data->TM.state_frame == 0)
                event_data->tip.hitbox_active = 0;

            // check if hitbox active
            for (u32 i = 0; i < countof(hmn_data->hitbox); i++)
            {
                if (hmn_data->hitbox[i].active != 0)
                {
                    event_data->tip.hitbox_active = 1;
                    break;
                }
            }
        }

        // update tip conditions
        if ((hmn_data->state_id >= ASID_LANDINGAIRN) && (hmn_data->state_id <= ASID_LANDINGAIRLW) && (hmn_data->TM.state_frame == 0) && // is in aerial landing
            (!event_data->is_fail) &&
            (event_data->tip.hitbox_active == 0)) // succeeded the last aerial landing
        {
            // increment condition count
            event_data->tip.hitbox_num++;

            // if condition met X times, show tip
            if (event_data->tip.hitbox_num >= 3)
            {
                // display tip
                char *hitbox_string = "Tip:\nDon't land too quickly! Make \nsure you land after the \nattack becomes active.";
                if (event_vars->Tip_Display(5 * 60, hitbox_string))
                {

                    // set as shown
                    //event_data->tip.hitbox_isdisp = 1;
                    event_data->tip.hitbox_num = 0;
                }
            }
        }
    }

    // fastfall tip
    if (event_data->tip.fastfall_isdisp == 0) // if not shown
    {
        // update fastfell bool
        if ((hmn_data->state_id >= ASID_ATTACKAIRN) && (hmn_data->state_id <= ASID_ATTACKAIRLW)) // check if currently in aerial attack)                                                      // check if in first frame of aerial attack
        {

            // reset hitbox bool on first frame of aerial attack
            if (hmn_data->TM.state_frame == 0)
                event_data->tip.fastfall_active = 0;

            // check if fastfalling
            if (hmn_data->flags.is_fastfall)
                event_data->tip.fastfall_active = 1;
        }

        // update tip conditions
        if (IsAerialLandingState(hmn_data->state_id) && (hmn_data->TM.state_frame == 0) && // is in aerial landing
            ((event_data->current_l_input_timing >= 7) && (event_data->current_l_input_timing <= 15)) &&      // was early for an l-cancel
            (event_data->tip.fastfall_active == 0))                                           // succeeded the last aerial landing
        {
            // increment condition count
            event_data->tip.fastfall_num++;

            // if condition met X times, show tip
            if (event_data->tip.fastfall_num >= 3)
            {
                // display tip
                char *fastfall_string = "Tip:\nDon't forget to fastfall!\nIt will let you act sooner \nand help with your \ntiming.";
                if (event_vars->Tip_Display(5 * 60, fastfall_string))
                {

                    // set as shown
                    //event_data->tip.hitbox_isdisp = 1;
                    event_data->tip.fastfall_num = 0;
                }
            }
        }
    }

    // late tip
    if (event_data->tip.late_isdisp == 0) // if not shown
    {
        if ((hmn_data->state_id >= ASID_LANDINGAIRN) && (hmn_data->state_id <= ASID_LANDINGAIRLW) && // is in aerial landing
            (event_data->is_fail) &&                                                      // failed the l-cancel
            (hmn_data->input.down & (HSD_TRIGGER_L | HSD_TRIGGER_R | HSD_TRIGGER_Z)))          // was late for an l-cancel by pressing it just now
        {
            // increment condition count
            event_data->tip.late_num++;

            // if condition met X times, show tip
            if (event_data->tip.late_num >= 3)
            {
                // display tip
                char *late_string = "Tip:\nTry pressing the trigger a\nbit earlier, before the\nfighter lands.";
                if (event_vars->Tip_Display(5 * 60, late_string))
                {
                    // set as shown
                    //event_data->tip.hitbox_isdisp = 1;
                    event_data->tip.late_num = 0;
                }
            }
        }
    }
}

// Barrel Functions
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

bool IsAerialLandingState(int state_id) {
    return ASID_LANDINGAIRN <= state_id && state_id <= ASID_LANDINGAIRLW;
}

bool IsEdgeCancelState(int state_id) {
    return state_id == ASID_FALL || state_id == ASID_OTTOTTO;
}

bool IsAutoCancelLanding(FighterData *hmn_data) {
    return hmn_data->state_id == ASID_LANDING
        && hmn_data->TM.state_frame == 0                   // if first frame of landing
        && hmn_data->TM.state_prev[0] >= ASID_ATTACKAIRN
        && hmn_data->TM.state_prev[0] <= ASID_ATTACKAIRLW; // came from aerial attack
}

// Initial Menu
EventMenu *Event_Menu = &LabMenu_Main;
