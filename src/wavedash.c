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

    // init timer
    event_data->timer = -1;
    
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

    Wavedash_Think(event_data, hmn_data);

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

    if (WdOptions_Main[OPT_HUD].val == 0) return;
    if (pass != 2) return;
    
    // Draw info panel
    static const char *label[3] = {"L1", "L2", "L3"};
    static const char *info[3] = {"I1", "I2", "I3"};
    event_vars->HUD_DrawInfoPanel(label, info, 3);

    #define W 1.6f // width of square
    #define H 2.f // height of square
    #define P 0.1f // amount of padding
    #define SX (-W * 15.f * 0.5f - P/2) // starting x
    #define SY 16.f // starting y
    
    static GXColor colors[16] = {
        {0, 0, 0, 255}, // background
        
        // early
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        
        // interpolate green -> orange
        { 0xb4, 0xff, 0xb4, 0xff },
        { 0xc3, 0xf3, 0x99, 0xff },
        { 0xd5, 0xe3, 0x79, 0xff },
        { 0xe5, 0xd2, 0x61, 0xff },
        { 0xf2, 0xc0, 0x51, 0xff },
        { 0xfc, 0xab, 0x4e, 0xff },
        { 0xff, 0x9d, 0x54, 0xff },
        { 0xff, 0x90, 0x60, 0xff },
    };
    
    static Rect rects[16] = {
        {SX-P/2, SY, 15.f*W+P, H}, // background
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
    };
    event_vars->HUD_DrawRects(rects, colors, countof(rects));

    Tri tris[countof(event_data->airdodge_frame)*2];
    GXColor tri_color[countof(event_data->airdodge_frame)*2];

    int ad_count = event_data->airdodge_count;
    int *ad_frame = event_data->airdodge_frame;
    GOBJ *ft = Fighter_GetGObj(0);
    FighterData *hmn_data = ft->userdata;
    
    static float x_pos[countof(event_data->airdodge_frame)] = {0};
    static int show_count = 0;

    if (event_data->timer == -1) {
        show_count = ad_count;

        // animate
        for (int i = 0; i < ad_count; ++i) {
            int f = ad_frame[i];
            float x = SX + ((float)f + (7.f - hmn_data->attr.jump_startup_time - 1.f) + 0.5f) * W;
    
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
        int f = ad_frame[i];
        if (i != 0 && ad_frame[i-1] == f)
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
    event_vars->HUD_DrawText("Too Early", &early, 0.45f);
    event_vars->HUD_DrawText("Wavedash", &timing0, 0.4f);
    event_vars->HUD_DrawText("Timing", &timing1, 0.4f);
    event_vars->HUD_DrawText("Too Late", &late, 0.45f);
}

void Wavedash_Think(WavedashData *event_data, FighterData *hmn_data)
{
    // start sequence on jump squat
    if (hmn_data->state_id == ASID_KNEEBEND && hmn_data->TM.state_frame == 0)
    {
        event_data->timer = 0;
        event_data->airdodge_count = 0;
    }

    // Do nothing if sequence hasn't started
    if (event_data->timer < 0)
        return;

    // run sequence logic
    event_data->timer++;

    // The game tracks whether the current jump is going to be a short hop in
    // `state_var1`. The value at the end of kneebend is what we want.
    if (hmn_data->state_id == ASID_KNEEBEND)
        event_data->short_hop = hmn_data->state_var.state_var1;
    if (WdOptions_Main[OPT_SHORT_HOP].val && event_data->short_hop)
        Fighter_ColAnim_Apply(hmn_data, 107, 0); // make sparkles
    
    // Record airdodge timings.
    if (hmn_data->input.down & PAD_TRIGGER_L)
        event_data->airdodge_frame[event_data->airdodge_count++] = event_data->timer;
    if (hmn_data->input.down & PAD_TRIGGER_R)
        event_data->airdodge_frame[event_data->airdodge_count++] = event_data->timer;

    // Real airdodge
    if (hmn_data->TM.state_frame == 0 &&
            (hmn_data->state_id == ASID_ESCAPEAIR ||
            (hmn_data->state_id == ASID_LANDINGFALLSPECIAL &&
             hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR &&
             hmn_data->TM.state_prev_frames[0] == 0)))
    {
        PADStatus *stat = PadGetRaw(hmn_data->pad_index);
        event_data->angle = -atan2(stat->stickY, fabs(stat->stickX)) / M_1DEGREE;
    }

    if (hmn_data->state_id == ASID_LANDINGFALLSPECIAL
        && hmn_data->TM.state_frame == 0
        && hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR
        && hmn_data->TM.state_prev[2] == ASID_KNEEBEND)
    {
        // success
        event_data->wd_attempted++;
        event_data->wd_succeeded++;

        // check for perfect
        int perfect_frame = (int)hmn_data->attr.jump_startup_time + 1;
        for (int i = 0; i < event_data->airdodge_count; ++i) {
            if (
                event_data->airdodge_frame[i] == perfect_frame
                || (event_data->airdodge_frame[i] == perfect_frame+1 && event_data->short_hop)
            ) {
                SFX_Play(303);
                break;
            }
        }
    }
    else if (hmn_data->TM.state_frame >= FAILFRAMES &&
             event_data->airdodge_count != 0 &&
            (hmn_data->state_id == ASID_JUMPF ||
             hmn_data->state_id == ASID_JUMPB ||
             hmn_data->state_id == ASID_ESCAPEAIR)) {
        // failure
        event_data->wd_attempted++;
        SFX_PlayCommon(3);
    } else if (hmn_data->state_id == ASID_KNEEBEND ||
              (hmn_data->TM.state_frame < FAILFRAMES &&
              (hmn_data->state_id == ASID_JUMPF ||
               hmn_data->state_id == ASID_JUMPB ||
               hmn_data->state_id == ASID_ESCAPEAIR))) {
        // in progress
        return;
    }

    // Reset
    event_data->timer = -1;
    Fighter_ColAnim_Remove(hmn_data, 107); // remove sparkles
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
            && event_data->angle > 0            // ignore horizontal airdodge
            && !event_data->short_hop)               // full hop
    {
        if (event_data->tip.short_hop++ % 8 == 2)
            event_vars->Tip_Display(5 * 60, "Tip:\nUse short hop for wavedash!\nit can help you land faster\nwhen you airdodge late.");
    }
}

// Initial Menu
EventMenu *Event_Menu = &WdMenu_Main;
