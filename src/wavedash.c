#include "wavedash.h"

enum options {
    OPT_TARGET,
    OPT_HUD,
    OPT_TIPS,
    OPT_HELP,
    OPT_EXIT,
    
    OPT_COUNT
};

// Main Menu
static EventOption WdOptions_Main[] = {
    // Target
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Target",
        .desc = {"Highlight an area of the stage to wavedash towards."},
    },
    // HUD
    {
        .kind = OPTKIND_TOGGLE,
        .name = "HUD",
        .desc = {"Toggle visibility of the HUD."},
        .val = 1,
        .OnChange = Wavedash_ChangeShowHUD,
    },
    // Tips
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Tips",
        .desc = {"Toggle the onscreen display of tips."},
        .val = 1,
    },
    // Help
    {
        .kind = OPTKIND_INFO,
        .name = "Help",
        .desc =
            {"A wavedash is performed by air-dodging diagonally down",
             "as soon you leave the ground from a jump, causing the fighter",
             "to slide a short distance. This technique will allow you to quickly",
             "adjust your position and even attack while sliding."},
    },
    // Exit
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Selection Screen."},
        .OnSelect = Event_Exit,
    },
};
static EventMenu WdMenu_Main = {
    .name = "Wavedash Training",
    .option_num = sizeof(WdOptions_Main) / sizeof(EventOption),
    .options = WdOptions_Main,
};

// Init Function
void Event_Init(GOBJ *gobj)
{
    WavedashData *event_data = gobj->userdata;
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // get wavedash assets
    event_data->assets = Archive_GetPublicAddress(event_vars->event_archive, "wavedash");

    // create HUD
    Wavedash_Init(event_data);

    // init target
    Target_Init(event_data, hmn_data);
}
// Think Function
void Event_Think(GOBJ *event)
{
    WavedashData *event_data = event->userdata;
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // infinite shields
    hmn_data->shield.health = 60;

    Wavedash_Think(event_data, hmn_data);
}
void Event_Exit(GOBJ *menu)
{
    // end game
    stc_match->state = 3;

    // cleanup
    Match_EndVS();
}

// Event functions
void Wavedash_Init(WavedashData *event_data)
{
    GOBJ *hud_gobj = GObj_Create(0, 0, 0);
    event_data->hud.gobj = hud_gobj;
    // Load jobj
    JOBJ *hud_jobj = JOBJ_LoadJoint(event_data->assets->hud);
    GObj_AddObject(hud_gobj, 3, hud_jobj);
    GObj_AddGXLink(hud_gobj, GXLink_Common, GXLINK_HUD, 80);

    // create text canvas
    int canvas = Text_CreateCanvas(2, hud_gobj, 14, 15, 0, GXLINK_HUD, 81, 19);
    event_data->hud.canvas = canvas;

    // init text
    Text **text_arr = &event_data->hud.text_timing;
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
        JOBJ_GetChild(hud_jobj, &text_jobj, WDJOBJ_TEXT + i, -1);
        JOBJ_GetWorldPosition(text_jobj, 0, &text_pos);

        // adjust scale
        Vec3 *scale = &hud_jobj->scale;
        // text scale
        hud_text->viewport_scale.X = (scale->X * 0.01) * TEXT_SCALE;
        hud_text->viewport_scale.Y = (scale->Y * 0.01) * TEXT_SCALE;
        hud_text->aspect.X = 165;

        // text position
        hud_text->trans.X = text_pos.X + (scale->X / 4.0);
        hud_text->trans.Y = (text_pos.Y * -1) + (scale->Y / 4.0);

        // dummy text
        Text_AddSubtext(hud_text, 0, 0, "-");
    }

    // init timer
    event_data->timer = -1;
    event_data->since_wavedash = 255;
}
void Wavedash_Think(WavedashData *event_data, FighterData *hmn_data)
{

    // check to enter is_wavedashing
    if (event_data->is_wavedashing == 0)
    {
        // increment time since wavedash
        if (event_data->since_wavedash < 255)
            event_data->since_wavedash++;

        // check to enter wavedash state
        if ((hmn_data->state_id == ASID_LANDINGFALLSPECIAL) && hmn_data->TM.state_prev[2] == ASID_KNEEBEND)
        {
            event_data->is_wavedashing = 1;
            event_data->since_wavedash = 0;
        }

        // check to null timer
        if (((hmn_data->state_id >= ASID_WALKSLOW) && (hmn_data->state_id <= ASID_KNEEBEND)) ||                    // no ground movement or jumping
            (hmn_data->phys.air_state == 1) ||                                                               // airborne
            ((hmn_data->atk_kind >= ATKKIND_SPECIALN) && (hmn_data->atk_kind <= ATKKIND_SPECIALLW)) || // any special move
            ((hmn_data->state_id >= ASID_ESCAPEF) && (hmn_data->state_id >= ASID_ESCAPEB)))                        // rolls
            event_data->since_wavedash = 255;
    }
    // check to exit is_wavedashing
    if (event_data->is_wavedashing == 1)
    {
        if ((hmn_data->state_id != ASID_LANDINGFALLSPECIAL))
            event_data->is_wavedashing = 0;
    }

    JOBJ *hud_jobj = event_data->hud.gobj->hsd_object;

    // start sequence on jump squat
    if ((hmn_data->state_id == ASID_KNEEBEND) && (hmn_data->TM.state_frame == 0))
    {
        event_data->is_airdodge = 0;

        // start timer
        event_data->timer = 0;

        // save line and position
        event_data->restore.pos.X = hmn_data->phys.pos.X;
        event_data->restore.pos.Y = hmn_data->phys.pos.Y;
        event_data->restore.line_index = hmn_data->coll_data.ground_index;
    }

    // if sequence started
    if (event_data->timer >= 0)
    {
        event_data->timer++; // inc timer

        // if grounded and not in kneebend, stop sequence
        if ((hmn_data->state_id != ASID_KNEEBEND) && (hmn_data->state_id != ASID_LANDINGFALLSPECIAL) && (hmn_data->phys.air_state == 0))
        {
            event_data->timer = -1;
            event_data->is_airdodge = 0;
            event_data->is_early_airdodge = 0;
        }

        // run sequence logic
        else
        {

            // catch early airdodge input
            if (hmn_data->input.down & (PAD_TRIGGER_L | PAD_TRIGGER_R))
            {
                event_data->airdodge_frame = event_data->timer; // save airdodge frame
                event_data->is_early_airdodge = 1;
            }

            // save airdodge angle
            if ((event_data->is_airdodge == 0) &&
                (((hmn_data->state_id == ASID_ESCAPEAIR) && (hmn_data->TM.state_frame == 0)) || // if entered airdodge
                 ((hmn_data->state_id == ASID_LANDINGFALLSPECIAL) && (hmn_data->TM.state_frame == 0) && (hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR) && (hmn_data->TM.state_prev_frames[0] == 0))))
            {
                // save airdodge angle
                Vec2 lstick = hmn_data->input.lstick;
                event_data->wd_angle = atan2(fabs(lstick.Y), fabs(lstick.X));

                event_data->is_early_airdodge = 0;
                event_data->is_airdodge = 1;
                event_data->airdodge_frame = event_data->timer; // save airdodge frame
            }

            int is_finished = 0;
            void *mat_anim = 0;

            // look for successful WD
            if ((hmn_data->state_id == ASID_LANDINGFALLSPECIAL) && (hmn_data->TM.state_frame == 0) && // is in special landing
                (hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR) &&                                  // came from airdodge
                (hmn_data->TM.state_prev[2] == ASID_KNEEBEND))                                     // came from jump
            {

                is_finished = 1;
                mat_anim = event_data->assets->hudmatanim[0];
                event_data->wd_succeeded++;

                // check for perfect
                if (event_data->airdodge_frame == ((int)hmn_data->attr.jump_startup_time + 1))
                    SFX_Play(303);
            }

            // look for failed WD
            else if (
                (
                    (event_data->is_early_airdodge == 1)
                    && (
                        (hmn_data->state_id == ASID_JUMPF)
                        || (hmn_data->state_id == ASID_JUMPB)
                    ) && (hmn_data->TM.state_frame >= 10)
                ) || (
                    (hmn_data->state_id == ASID_ESCAPEAIR) 
                    && (hmn_data->TM.state_frame >= 10) 
                    && (hmn_data->TM.state_prev[1] == ASID_KNEEBEND)
                )
            ) {
                is_finished = 1;
                mat_anim = event_data->assets->hudmatanim[1];
                SFX_PlayCommon(3);

                // restore position
                int ray_index;
                int ray_kind;
                Vec3 ray_angle;
                Vec3 ray_pos;
                float from_x = event_data->restore.pos.X;
                float to_x = from_x;
                float from_y = event_data->restore.pos.Y + 3;
                float to_y = from_y - 6;
                int is_ground = GrColl_RaycastGround(&ray_pos, &ray_index, &ray_kind, &ray_angle, -1, -1, -1, 0, from_x, from_y, to_x, to_y, 0);
                if ((is_ground == 1) && (ray_index == event_data->restore.line_index))
                {
                    // do this for every subfighter (thanks for complicated code ice climbers)
                    for (int i = 0; i < 2; i++)
                    {
                        GOBJ *this_fighter = Fighter_GetSubcharGObj(hmn_data->ply, i);

                        if (this_fighter != 0)
                        {

                            FighterData *this_fighter_data = this_fighter->userdata;

                            if ((this_fighter_data->flags.sleep == 0) && (this_fighter_data->flags.dead == 0))
                            {

                                // place CPU here
                                this_fighter_data->phys.pos = ray_pos;
                                this_fighter_data->coll_data.ground_index = ray_index;

                                // set grounded
                                this_fighter_data->phys.air_state = 0;
                                //Fighter_SetGrounded(this_fighter);

                                // kill velocity
                                Fighter_KillAllVelocity(this_fighter);

                                // enter wait
                                ActionStateChange(0, 1, -1, this_fighter, ASID_WAIT, 0, 0);
                                this_fighter_data->state.blend = 0;

                                // update ECB
                                this_fighter_data->coll_data.topN_Curr = this_fighter_data->phys.pos; // move current ECB location to new position
                                Coll_ECBCurrToPrev(&this_fighter_data->coll_data);
                                this_fighter_data->cb.Coll(this_fighter);

                                // update camera box
                                Fighter_UpdateCameraBox(this_fighter);
                                this_fighter_data->camera_subject->boundleft_curr = this_fighter_data->camera_subject->boundleft_proj;
                                this_fighter_data->camera_subject->boundright_curr = this_fighter_data->camera_subject->boundright_proj;

                                // init CPU logic (for nana's popo position history...)
                                int cpu_kind = Fighter_GetCPUKind(this_fighter_data->ply);
                                int cpu_level = Fighter_GetCPULevel(this_fighter_data->ply);
                                Fighter_CPUInitialize(this_fighter_data, cpu_kind, cpu_level, 0);

                                // place subfighter in the Z axis
                                if (this_fighter_data->flags.ms == 1)
                                {
                                    ftCommonData *ft_common = *stc_ftcommon;
                                    this_fighter_data->phys.pos.Z = ft_common->ms_zjostle_max * -1;
                                }
                            }
                        }
                    }

                    // update camera
                    Match_CorrectCamera();
                }
            }

            // update bar
            if (is_finished)
            {
                // reset variables
                event_data->timer = -1;
                event_data->is_airdodge = 0;
                event_data->is_early_airdodge = 0;

                // update bar frame colors
                JOBJ *arrow_jobj;
                JOBJ_GetChild(hud_jobj, &arrow_jobj, WDJOBJ_ARROW, -1); // get timing bar jobj
                // get in terms of bar timeframe
                int jump_frame = ((WDFRAMES - 1) / 2) - (int)hmn_data->attr.jump_startup_time;
                int input_frame = jump_frame + event_data->airdodge_frame - 1;

                // update arrow position
                if (input_frame < WDFRAMES)
                {
                    event_data->hud.arrow_prevpos = arrow_jobj->trans.X;
                    event_data->hud.arrow_nextpos = (-WDARROW_OFFSET * ((WDFRAMES - 1) / 2)) + (input_frame * 0.36);
                    JOBJ_ClearFlags(arrow_jobj, JOBJ_HIDDEN);
                    event_data->hud.arrow_timer = WDARROW_ANIMFRAMES;
                }
                // hide arrow for this wd attempt
                else
                {
                    event_data->hud.arrow_timer = 0;
                    arrow_jobj->trans.X = 0;
                    JOBJ_SetFlags(arrow_jobj, JOBJ_HIDDEN);
                }

                // updating timing text
                if (input_frame < ((WDFRAMES - 1) / 2)) // is early
                    Text_SetText(event_data->hud.text_timing, 0, "%df Early", ((WDFRAMES - 1) / 2) - input_frame);
                else if (input_frame == ((WDFRAMES - 1) / 2))
                    Text_SetText(event_data->hud.text_timing, 0, "Perfect");
                else if (input_frame > ((WDFRAMES - 1) / 2))
                    Text_SetText(event_data->hud.text_timing, 0, "%df Late", input_frame - ((WDFRAMES - 1) / 2));

                // update airdodge angle
                Text_SetText(event_data->hud.text_angle, 0, "%.2f", fabs(event_data->wd_angle / M_1DEGREE));

                // update succession
                event_data->wd_attempted++;
                int successful = event_data->wd_succeeded;
                float succession = ((float)event_data->wd_succeeded / (float)event_data->wd_attempted) * 100.0;
                Text_SetText(event_data->hud.text_succession, 0, "%d (%.2f%)", successful, succession);

                // apply HUD animation
                JOBJ_RemoveAnimAll(hud_jobj);
                JOBJ_AddAnimAll(hud_jobj, 0, mat_anim, 0);
                JOBJ_ReqAnimAll(hud_jobj, 0);
            }
        }
    }

    // update target
    Target_Manager(event_data, hmn_data);

    // run tip logic
    Tips_Think(event_data, hmn_data);

    // update HUD anim
    JOBJ_AnimAll(hud_jobj);

    // update arrow animation
    if (event_data->hud.arrow_timer > 0)
    {
        // decrement timer
        event_data->hud.arrow_timer--;

        // get this frames position
        float time = 1 - ((float)event_data->hud.arrow_timer / (float)WDARROW_ANIMFRAMES);
        float xpos = smooth_lerp(time, event_data->hud.arrow_prevpos, event_data->hud.arrow_nextpos);

        // update position
        JOBJ *arrow_jobj;
        JOBJ_GetChild(hud_jobj, &arrow_jobj, WDJOBJ_ARROW, -1); // get timing bar jobj
        arrow_jobj->trans.X = xpos;
        JOBJ_SetMtxDirtySub(arrow_jobj);
    }
}

// Target functions
void Target_Init(WavedashData *event_data, FighterData *hmn_data)
{

    ftCommonData *ftcommon = *stc_ftcommon;
    float mag;

    // determine best wavedash distance (not taking into account friction doubling)
    mag = ftcommon->escapeair_vel * cos(atan2(-0.2875, 0.9500));
    float wd_maxdstn = Target_GetWdashDistance(hmn_data, mag);
    event_data->wd_maxdstn = wd_maxdstn;

    // determine scale based on wd distance
    float dist = event_data->wd_maxdstn;
    if (dist < TRGTSCL_DISTMIN)
        dist = TRGTSCL_DISTMIN;
    else if (dist > TRGTSCL_DISTMAX)
        dist = TRGTSCL_DISTMAX;
    event_data->target.scale = (((dist - TRGTSCL_DISTMIN) / (TRGTSCL_DISTMAX - TRGTSCL_DISTMIN)) * (TRGTSCL_SCALEMAX - TRGTSCL_SCALEMIN)) + TRGTSCL_SCALEMIN;

    // get width of the target
    JOBJ *target = JOBJ_LoadJoint(event_data->assets->target_jobj); // create dummy
    float scale = event_data->target.scale;                         // scale
    target->scale.X *= scale;
    target->scale.Z *= scale;

    // get children
    JOBJ *left_jobj, *right_jobj;
    JOBJ_GetChild(target, &left_jobj, TRGTJOBJ_LBOUND, -1);
    JOBJ_GetChild(target, &right_jobj, TRGTJOBJ_RBOUND, -1);
    // get offsets
    JOBJ_GetWorldPosition(left_jobj, 0, &event_data->target.left_offset);
    JOBJ_GetWorldPosition(right_jobj, 0, &event_data->target.right_offset);

    // free jobj
    JOBJ_RemoveAll(target);
}
void Target_Manager(WavedashData *event_data, FighterData *hmn_data)
{
    GOBJ *target_gobj = event_data->target.gobj;

    if (WdOptions_Main[OPT_TARGET].val)
    { // on
        // if not spawned, spawn
        if (target_gobj == 0)
        {
            if (hmn_data->phys.air_state == 0)
            {
                // spawn target
                target_gobj = Target_Spawn(event_data, hmn_data);
                event_data->target.gobj = target_gobj;
            }
        }

        // update target logic
        if (target_gobj != 0)
        {
            TargetData *target_data = target_gobj->userdata;

            // check current target state
            if (target_data->state == TRGSTATE_DESPAWN)
            {
                // create new one
                target_gobj = Target_Spawn(event_data, hmn_data);
                event_data->target.gobj = target_gobj;
            }
        }
    }
    else
    { // off
        // if spawned, remove
        if (target_gobj != 0)
        {
            Target_ChangeState(target_gobj, TRGSTATE_DESPAWN);
            event_data->target.gobj = 0;
        }
    }
}
GOBJ *Target_Spawn(WavedashData *event_data, FighterData *hmn_data)
{

    Vec3 ray_angle;
    Vec3 ray_pos;
    int ray_index;
    float max = (event_data->wd_maxdstn * TRGT_RANGEMAX);
    float min = (event_data->wd_maxdstn * TRGT_RANGEMIN);

    // ensure min exists
    int min_exists = 0;
    min_exists += Target_CheckArea(event_data, hmn_data->coll_data.ground_index, &hmn_data->phys.pos, max, 0, 0, 0);
    min_exists += Target_CheckArea(event_data, hmn_data->coll_data.ground_index, &hmn_data->phys.pos, max * -1, 0, 0, 0);
    if (min_exists)
    {

        // begin looking for valid ground at a random distance
        int is_ground = 0;
        while (!is_ground)
        {

            // select random direction
            float direction;
            int temp = HSD_Randi(2);
            if (temp == 0)
                direction = -1;
            else
                direction = 1;

            // random distance
            float distance = (HSD_Randf() * (max - min)) + min;

            // check if valid
            is_ground = Target_CheckArea(event_data, hmn_data->coll_data.ground_index, &hmn_data->phys.pos, distance * direction, &ray_index, &ray_pos, &ray_angle);
        }

        // create target gobj
        GOBJ *target_gobj = GObj_Create(10, 11, 0);

        // target data
        TargetData *target_data = calloc(sizeof(TargetData));
        GObj_AddUserData(target_gobj, 4, HSD_Free, target_data);

        // add proc
        GObj_AddProc(target_gobj, Target_Think, 16);

        // create jobj
        JOBJ *target_jobj = JOBJ_LoadJoint(event_data->assets->target_jobj);
        GObj_AddObject(target_gobj, 3, target_jobj);
        GObj_AddGXLink(target_gobj, GXLink_Common, 5, 0);

        // scale target
        float scale = event_data->target.scale;
        target_jobj->scale.X *= scale;
        target_jobj->scale.Z *= scale;

        // move target
        target_jobj->trans.X = ray_pos.X;
        target_jobj->trans.Y = ray_pos.Y;
        target_jobj->trans.Z = ray_pos.Z;

        // adjust rotation based on line slope
        JOBJ *aura_jobj;
        JOBJ_GetChild(target_jobj, &aura_jobj, TRGTJOBJ_AURA, -1);
        aura_jobj->rot.Z = -1 * atan2(ray_angle.X, ray_angle.Y);

        // create camera box
        CmSubject *cam = CameraSubject_Alloc();
        cam->boundleft_proj = -10;
        cam->boundright_proj = 10;
        cam->boundtop_proj = 10;
        cam->boundbottom_proj = -10;

        // update camerabox position
        cam->cam_pos.X = target_jobj->trans.X;
        cam->cam_pos.Y = target_jobj->trans.Y + 15;
        cam->cam_pos.Z = target_jobj->trans.Z;

        // init target data
        Target_ChangeState(target_gobj, TRGSTATE_SPAWN); // enter spawn state
        target_data->cam = cam;                          // save camera
        target_data->line_index = ray_index;             // save line index
        target_data->pos = ray_pos;                      // save position
        target_data->left = event_data->target.left_offset.X;
        target_data->right = event_data->target.right_offset.X;

        return target_gobj;
    }
    else
    {
        return 0;
    }
}
void Target_Think(GOBJ *target_gobj)
{
    JOBJ *target_jobj = target_gobj->hsd_object;
    TargetData *target_data = target_gobj->userdata;
    WavedashData *event_data = event_vars->event_gobj->userdata;

    // update anim
    JOBJ_AnimAll(target_jobj);

    // ensure line still exists
    if (GrColl_CheckIfLineEnabled(target_data->line_index) == 0)
    {
        // enter exit state
        Target_ChangeState(target_gobj, TRGSTATE_DESPAWN);
    }

    // update target position (look into how fighters are rooted on ground)
    Vec3 pos_diff;
    GrColl_GetPosDifference(target_data->line_index, &target_data->pos, &pos_diff);
    VECAdd(&target_data->pos, &pos_diff, &target_data->pos);

    // update target orientation
    Vec3 slope;
    GrColl_GetLineSlope(target_data->line_index, &slope);
    JOBJ *aura_jobj;
    JOBJ_GetChild(target_jobj, &aura_jobj, TRGTJOBJ_AURA, -1);
    aura_jobj->rot.Z = -1 * atan2(slope.X, slope.Y);

    // update camerabox position
    CmSubject *cam = target_data->cam;
    cam->cam_pos.X = target_data->pos.X;
    cam->cam_pos.Y = target_data->pos.Y + 15;
    cam->cam_pos.Z = target_data->pos.Z;

    // update position
    target_jobj->trans = target_data->pos;
    JOBJ_SetMtxDirtySub(target_jobj);

    // state based logic
    switch (target_data->state)
    {
    case (TRGSTATE_SPAWN):
    {
        // check if ended
        if (JOBJ_CheckAObjEnd(target_jobj) == 0)
            Target_ChangeState(target_gobj, TRGSTATE_WAIT);

        break;
    }
    case (TRGSTATE_WAIT):
    {
        // get position
        Vec3 pos;
        JOBJ_GetWorldPosition(target_jobj, 0, &pos);

        // check for collision
        FighterData *hmn_data = Fighter_GetGObj(0)->userdata;

        Vec3 *ft_pos = &hmn_data->phys.pos;
        if ((hmn_data->phys.air_state == 0) &&
            ((event_data->since_wavedash > 0) && (event_data->since_wavedash < 255) && (fabs(hmn_data->phys.self_vel.X) < 0.5)) && // check if a wavedash
            (ft_pos->X > (pos.X + target_data->left)) &&
            (ft_pos->X < (pos.X + target_data->right)) &&
            (ft_pos->Y > (pos.Y + -1)) &&
            (ft_pos->Y < (pos.Y + 1)))
        {

            // sfx
            SFX_Play(173);

            Target_ChangeState(target_gobj, TRGSTATE_DESPAWN);
        }

        break;
    }
    case (TRGSTATE_DESPAWN):
    {
        // check if ended
        if (JOBJ_CheckAObjEnd(target_jobj) == 0)
        {
            // destroy camera
            CameraSubject_Destroy(target_data->cam);

            // destroy this target
            GObj_Destroy(target_gobj);
        }

        break;
    }
    }
}
void Target_ChangeState(GOBJ *target_gobj, int state)
{
    WavedashData *event_data = event_vars->event_gobj->userdata;
    TargetData *target_data = target_gobj->userdata;
    JOBJ *target_jobj = target_gobj->hsd_object;

    // update state
    target_data->state = state;

    // add anim
    JOBJ_AddAnimAll(target_jobj, event_data->assets->target_jointanim[state], event_data->assets->target_matanim[state], 0);
    JOBJ_ReqAnimAll(target_jobj, 0); // req anim
}

void Wavedash_ChangeShowHUD(GOBJ *menu_gobj, int show)
{
    HUDCamData *cam = event_vars->hudcam_gobj->userdata;
    cam->hide = !show;
}

float Target_GetWdashDistance(FighterData *hmn_data, float mag)
{

    float distance = 0;
    ftCommonData *ftcommon = *stc_ftcommon;

    // now simulate
    mag *= ftcommon->escapeair_veldecaymult; // first frame multiply by 0.9 ( in airdodge still)
    distance += mag;

    // subsequent, apply friction until at 0
    while (mag > 0)
    {

        // get friction
        float friction = hmn_data->attr.ground_friction;
        if (mag > hmn_data->attr.walk_maximum_velocity) // double friction if speed > walk max speed
            friction *= ftcommon->friction_mult;

        // apply it
        mag -= friction;

        // ensure not under 0
        if (mag < 0)
            mag = 0;

        distance += mag;
    }

    return distance;
}
int Target_CheckArea(WavedashData *event_data, int line, Vec3 *pos, float x_offset, int *ret_line, Vec3 *ret_pos, Vec3 *ret_slope)
{

    int status = 0;

    // init
    int is_ground = 0;
    // check left
    is_ground += GrColl_CrawlGround(line, pos, ret_line, ret_pos, 0, ret_slope, x_offset + event_data->target.left_offset.X, 0);
    // check right
    is_ground += GrColl_CrawlGround(line, pos, ret_line, ret_pos, 0, ret_slope, x_offset + event_data->target.right_offset.X, 0);
    // check center
    is_ground += GrColl_CrawlGround(line, pos, ret_line, ret_pos, 0, ret_slope, x_offset + event_data->target.center_offset.X, 0);

    if (is_ground == 3)
        status = 1;

    return status;
}

// Tips
void Tips_Think(WavedashData *event_data, FighterData *hmn_data)
{

    // only if enabled
    if (!WdOptions_Main[OPT_TIPS].val)
    {
        // shield after wavedash
        // look successful wavedash
        if (event_data->since_wavedash <= 10)
        {
            // look for frame 1 of guard off
            if ((hmn_data->state_id == ASID_GUARDOFF) && (hmn_data->TM.state_frame == 0) &&                  // just let go of shield
                ((hmn_data->TM.state_prev[0] == ASID_GUARD) && (hmn_data->TM.state_prev_frames[0] == 1))) // only guarded for 1 frame
            {
                event_data->tip.shield_num++;

                if (event_data->tip.shield_num >= 3)
                {
                    event_vars->Tip_Display(5 * 60, "Tip:\nDon't hold the trigger! Quickly \npress and release to prevent \nshielding after wavedashing.");
                    event_data->tip.shield_num = 0;
                }
            }
        }
    }
}

// Initial Menu
EventMenu *Event_Menu = &WdMenu_Main;
