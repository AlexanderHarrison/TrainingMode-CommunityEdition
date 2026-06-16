#include "wavedash.h"
#include "events.h"

enum options {
    OPT_TARGET,
    OPT_SHORT_HOP,
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
    // Short Hop
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Short Hop Indicator",
        .val = 1,
        .desc = {"A sparkling indicator for when you short hop.",
                 "Short hops can avoid excess frames spent",
                 "in airdodge in case you are late, especially",
                 "at long/sharp wavedash angles"},
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

    // init target
    Target_Init(event_data, hmn_data);

    // init vars
    event_data->core.timer = -1;
    event_data->core.result = -1;
    
    GObj_AddGXLink(gobj, HUD_GX, 5, 0);
}

// Think Function
void Event_Think(GOBJ *event)
{
    WavedashData *event_data = event->userdata;
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // infinite shields
    hmn_data->shield.health = 60;

    WavedashCoreConfig cfg = {
        .show_hud = WdOptions_Main[OPT_HUD].val,
        .short_hop_indicator = WdOptions_Main[OPT_SHORT_HOP].val,
    };
    WavedashCore_Think(&event_data->core, hmn_data, &cfg);

    // update target
    Target_Manager(event_data, hmn_data);

    // run tip logic
    Tips_Think(event_data, hmn_data);
}
void Event_Exit(GOBJ *menu)
{
    // end game
    stc_match->state = 3;

    // cleanup
    Match_EndVS();
}

void HUD_GX(GOBJ *gobj, int pass) {
    WavedashData *event_data = gobj->userdata;
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    WavedashCoreConfig cfg = {
        .show_hud = WdOptions_Main[OPT_HUD].val,
        .short_hop_indicator = WdOptions_Main[OPT_SHORT_HOP].val,
    };
    WavedashCore_DrawHUD(&event_data->core, hmn_data, &cfg, pass);
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
        if (hmn_data->phys.air_state == 0 &&
            fabs(hmn_data->phys.self_vel.X) < 0.5 && // check if a wavedash
            (ft_pos->X > (pos.X + target_data->left)) &&
            (ft_pos->X < (pos.X + target_data->right)) &&
            (ft_pos->Y > (pos.Y - 1)) &&
            (ft_pos->Y < (pos.Y + 1)))
        {
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
            CameraSubject_Destroy(target_data->cam);
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
    if (!WdOptions_Main[OPT_TIPS].val)
        return;

    // Check for player holding shield too after the wavedash landing lag
    if (hmn_data->state_id == ASID_GUARDOFF && hmn_data->TM.state_frame == 0 &&
        hmn_data->TM.state_prev[3] == ASID_LANDINGFALLSPECIAL)
    {
        if (event_data->tip.shield_num++ % 5 == 2)
            event_vars->Tip_Display(5 * 60, "Tip:\nDon't hold the trigger! Quickly \npress and release to prevent \nshielding after wavedashing.");
    }

    // Check for slow airdodge landing after full hop
    if (hmn_data->state_id == ASID_LANDINGFALLSPECIAL
            && hmn_data->TM.state_frame == 0
            && hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR
            && hmn_data->TM.state_prev_frames[0] > 3 // slow to hit ground from airdodge
            && hmn_data->TM.state_prev_frames[1] < 5 // slightly late airdodge timing
            && event_data->core.angle > 0       // ignore horizontal airdodge
            && !event_data->core.short_hop)          // full hop
    {
        if (event_data->tip.short_hop++ % 8 == 2)
            event_vars->Tip_Display(5 * 60, "Tip:\nUse short hop for wavedash!\nit can help you land faster\nwhen you airdodge late.");
    }
}

// Initial Menu
EventMenu *Event_Menu = &WdMenu_Main;
