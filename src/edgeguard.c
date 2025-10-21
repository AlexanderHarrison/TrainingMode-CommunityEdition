#include "edgeguard.h"
#include "recovery.c"

// how far inland the hmn will start
#define DISTANCE_FROM_LEDGE 15

// how many frames it takes to reset after the cpu dies or successfully recovers
#define RESET_DELAY 30

static int reset_timer = -1;
EventMenu *Event_Menu = &Menu_Main;

static void UpdatePosition(GOBJ *fighter) {
    FighterData *data = fighter->userdata;

    Vec3 pos = data->phys.pos;
    data->coll_data.topN_Curr = pos;
    data->coll_data.topN_CurrCorrect = pos;
    data->coll_data.topN_Prev = pos;
    data->coll_data.topN_Proj = pos;
    data->coll_data.coll_test = *stc_colltest;
}

static void UpdateCameraBox(GOBJ *fighter) {
    Fighter_UpdateCameraBox(fighter);

    FighterData *data = fighter->userdata;
    CmSubject *subject = data->camera_subject;
    subject->boundleft_curr = subject->boundleft_proj;
    subject->boundright_curr = subject->boundright_proj;

    Match_CorrectCamera();
}

static bool IsGroundActionable(GOBJ *fighter) {
    FighterData *data = fighter->userdata;

    // ensure grounded
    if (data->phys.air_state == 1)
        return false;

    int state = data->state_id;

    if (InHitstunAnim(state) && HitstunEnded(fighter))
        return true;

    if (state == ASID_LANDING && data->state.frame >= data->attr.normal_landing_lag)
        return true;
        
    return state == ASID_WAIT
        || state == ASID_WALKSLOW
        || state == ASID_WALKMIDDLE
        || state == ASID_WALKFAST
        || state == ASID_RUN
        || state == ASID_SQUATWAIT
        || state == ASID_OTTOTTOWAIT
        || state == ASID_GUARD;
}

static void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

static void Reset_Subchar(GOBJ *sub, GOBJ *main) {
    // Every subchar has a force that attracts them to the main char's position a few frames ago.
    // When the main character teleports during a reset,
    // we need to reset the past position data of the subchar so that 
    // they aren't attracted to the main char's previous position across the stage.

    FighterData *main_data = main->userdata;
    FighterData *sub_data = sub->userdata;
    Vec3 pos = main_data->phys.pos;
    float facing = main_data->facing_direction;
    for (u32 i = 0; i < countof(sub_data->cpu.leader_log); ++i) {
        CPULeaderLog *log = &sub_data->cpu.leader_log[i];
        log->pos = pos;
        log->facing_direction = facing;
    }
}

static void Reset_HMN(GOBJ *hmn, int side_idx, int dmg) {
    FighterData *hmn_data = hmn->userdata;
    
    int side = side_idx * 2 - 1;
    hmn_data->facing_direction = -side; 
    float ledge_x = ledge_positions[side_idx].X - DISTANCE_FROM_LEDGE * side;
    
    // set phys
    hmn_data->phys.kb_vel.X = 0.f;
    hmn_data->phys.kb_vel.Y = 0.f;
    hmn_data->phys.self_vel.X = 0.f;
    hmn_data->phys.self_vel.Y = 0.f;
    hmn_data->phys.pos.X = ledge_x;
    hmn_data->phys.pos.Y = 5.f;
    UpdatePosition(hmn);
    
    hmn_data->jump.jumps_used = 1;
    
    // set hmn action state
    Fighter_EnterAerial(hmn, ASID_ATTACKAIRB);
    Fighter_ApplyAnimation(hmn, 7, 1, 0);
    hmn_data->state.frame = 7;
    hmn_data->script.script_event_timer = 0;
    Fighter_SubactionFastForward(hmn);

    // ensure the player L-cancels the initial bair.
    hmn_data->flags.is_fastfall = 1;
    hmn_data->input.timer_trigger_any_ignore_hitlag = 0;
    
    Fighter_UpdateStateFrameInfo(hmn);
    Fighter_HitboxDisableAll(hmn);
    hmn_data->script.script_current = 0;
    
    // fix camera
    UpdateCameraBox(hmn);

    // give hitlag
    hmn_data->dmg.hitlag_frames = 7;
    hmn_data->flags.hitlag = 1;
    hmn_data->flags.hitlag_unk = 1;
    
    // percent
    hmn_data->dmg.percent = dmg;
    Fighter_SetHUDDamage(hmn_data->ply, dmg);
}

static void Reset_CPU(GOBJ *cpu, int side_idx, int dmg, float kb_mag, float kb_angle) {
    FighterData *cpu_data = cpu->userdata;
    cpu_data->cpu.ai = 15;
    
    int side = side_idx * 2 - 1;
    cpu_data->facing_direction = -side;
    float ledge_x = ledge_positions[side_idx].X - DISTANCE_FROM_LEDGE * side;
    
    // set phys
    cpu_data->phys.kb_vel.X = 0.f;
    cpu_data->phys.kb_vel.Y = 0.f;
    cpu_data->phys.self_vel.X = 0.f;
    cpu_data->phys.self_vel.Y = 0.f;
    cpu_data->phys.pos.X = ledge_x;
    cpu_data->phys.pos.Y = 0.f;
    UpdatePosition(cpu);
    
    cpu_data->jump.jumps_used = 1;
    
    // set cpu action state
    int state = kb_mag > 130.f ? ASID_DAMAGEFLYHI : ASID_DAMAGEFLYN;
    Fighter_EnterFall(cpu);
    ActionStateChange(0, 1, 0, cpu, state, 0x40, 0);
    Fighter_UpdateStateFrameInfo(cpu);
    
    // fix camera
    UpdateCameraBox(cpu);
    
    // give cpu knockback
    float angle_rad = kb_angle * M_1DEGREE;

    float vel = kb_mag * (*stc_ftcommon)->force_applied_to_kb_mag_multiplier;
    float vel_x = cos(angle_rad) * vel * (float)side;
    float vel_y = sin(angle_rad) * vel;
    cpu_data->phys.kb_vel.X = vel_x;
    cpu_data->phys.kb_vel.Y = vel_y;

    float kb_frames = (float)(int)((*stc_ftcommon)->x154 * kb_mag);
    memcpy(&cpu_data->state_var.state_var1, &kb_frames, sizeof(float));
    cpu_data->flags.hitstun = 1;
    Fighter_EnableCollUpdate(cpu_data);

    // give hitlag
    cpu_data->dmg.hitlag_frames = 7;
    cpu_data->flags.hitlag = 1;
    cpu_data->flags.hitlag_unk = 1;
    
    // percent
    cpu_data->dmg.percent = dmg;
    Fighter_SetHUDDamage(cpu_data->ply, dmg);
}

static void Reset(void) {
    event_vars->Savestate_Load_v1(event_vars->savestate, Savestate_Silent);

    for (int ply = 0; ply < 2; ++ply) {
        MatchHUDElement *hud = &stc_matchhud->element_data[ply];
        if (hud->is_removed == 1)
            Match_CreateHUD(ply);
    }

    int side_idx = HSD_Randi(2);
    KBValues vals = HitStrength_KBRange[Options_Main[OPT_MAIN_HITSTRENGTH].val];
    float kb_mag = vals.mag_min + (vals.mag_max - vals.mag_min) * HSD_Randf();
    float kb_angle = vals.ang_min + (vals.ang_max - vals.ang_min) * HSD_Randf();
    int hmn_dmg = Options_Main[OPT_MAIN_PERCENT].val;
    int cpu_dmg = vals.dmg_min + HSD_Randi(vals.dmg_max - vals.dmg_min);
    
    GOBJ *hmn = Fighter_GetSubcharGObj(0, 0);
    GOBJ *cpu = Fighter_GetSubcharGObj(1, 0);
    Reset_HMN(hmn, side_idx, hmn_dmg);
    Reset_CPU(cpu, side_idx, cpu_dmg, kb_mag, kb_angle);
    
    GOBJ *hmn_sub = Fighter_GetSubcharGObj(0, 1);
    GOBJ *cpu_sub = Fighter_GetSubcharGObj(1, 1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;
    if (hmn_sub && hmn_data->flags.has_follower) {
        Reset_HMN(hmn_sub, side_idx, hmn_dmg);
        Reset_Subchar(hmn_sub, hmn);
    }
    if (cpu_sub && cpu_data->flags.has_follower) {
        Reset_CPU(cpu_sub, side_idx, cpu_dmg, kb_mag, kb_angle);
        Reset_Subchar(cpu_sub, hmn);
    }
}

static void ChangePlayerPercent(GOBJ *menu_gobj, int dmg) {
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    hmn_data->dmg.percent = dmg;
    Fighter_SetHUDDamage(hmn_data->ply, dmg);
}

static void WriteCustomKBValues(void) {
    HitStrength_KBRange[KBVALS_CUSTOM].mag_min = Options_CustomHitStrength[OPT_CUSTOMHIT_MAGMIN].val;
    HitStrength_KBRange[KBVALS_CUSTOM].mag_max = Options_CustomHitStrength[OPT_CUSTOMHIT_MAGMAX].val;
    HitStrength_KBRange[KBVALS_CUSTOM].ang_min = Options_CustomHitStrength[OPT_CUSTOMHIT_ANGMIN].val;
    HitStrength_KBRange[KBVALS_CUSTOM].ang_max = Options_CustomHitStrength[OPT_CUSTOMHIT_ANGMAX].val;
    HitStrength_KBRange[KBVALS_CUSTOM].dmg_min = Options_CustomHitStrength[OPT_CUSTOMHIT_DMGMIN].val;
    HitStrength_KBRange[KBVALS_CUSTOM].dmg_max = Options_CustomHitStrength[OPT_CUSTOMHIT_DMGMAX].val;
    Options_Main[OPT_MAIN_HITSTRENGTH].val = KBVALS_CUSTOM;
}

static void ChangeCustomKB_MagMin(GOBJ *menu_gobj, int value) {
    s16 *min = &Options_CustomHitStrength[OPT_CUSTOMHIT_MAGMIN].val;
    s16 *max = &Options_CustomHitStrength[OPT_CUSTOMHIT_MAGMAX].val;
    if (*min > *max) *max = *min;
    WriteCustomKBValues();
}
static void ChangeCustomKB_MagMax(GOBJ *menu_gobj, int value) {
    s16 *min = &Options_CustomHitStrength[OPT_CUSTOMHIT_MAGMIN].val;
    s16 *max = &Options_CustomHitStrength[OPT_CUSTOMHIT_MAGMAX].val;
    if (*min > *max) *min = *max;
    WriteCustomKBValues();
}
static void ChangeCustomKB_AngMin(GOBJ *menu_gobj, int value) {
    s16 *min = &Options_CustomHitStrength[OPT_CUSTOMHIT_ANGMIN].val;
    s16 *max = &Options_CustomHitStrength[OPT_CUSTOMHIT_ANGMAX].val;
    if (*min > *max) *max = *min;
    WriteCustomKBValues();
}
static void ChangeCustomKB_AngMax(GOBJ *menu_gobj, int value) {
    s16 *min = &Options_CustomHitStrength[OPT_CUSTOMHIT_ANGMIN].val;
    s16 *max = &Options_CustomHitStrength[OPT_CUSTOMHIT_ANGMAX].val;
    if (*min > *max) *min = *max;
    WriteCustomKBValues();
}
static void ChangeCustomKB_DmgMin(GOBJ *menu_gobj, int value) {
    s16 *min = &Options_CustomHitStrength[OPT_CUSTOMHIT_DMGMIN].val;
    s16 *max = &Options_CustomHitStrength[OPT_CUSTOMHIT_DMGMAX].val;
    if (*min > *max) *max = *min;
    WriteCustomKBValues();
}
static void ChangeCustomKB_DmgMax(GOBJ *menu_gobj, int value) {
    s16 *min = &Options_CustomHitStrength[OPT_CUSTOMHIT_DMGMIN].val;
    s16 *max = &Options_CustomHitStrength[OPT_CUSTOMHIT_DMGMAX].val;
    if (*min > *max) *min = *max;
    WriteCustomKBValues();
}

void Event_Init(GOBJ *gobj) {
    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;
    
    // change team - homing missiles don't move towards characters on same team
    hmn_data->team = 0;
    cpu_data->team = 1;

    EventMenu *recovery_menu = RecoveryMenus[cpu_data->kind];
    if (recovery_menu == 0)
        assert("unimplemented character in edgeguard training");
    Options_Main[OPT_MAIN_RECOVERY].menu = recovery_menu;
    
    GetLedgePositions(ledge_positions);
}

void Event_Think(GOBJ *menu) {
    // For some reason, Savestate_Save crashes in Event_Init.
    // So we need to init on first think frame.
    if (event_vars->game_timer == 1) {
        event_vars->Savestate_Save_v1(event_vars->savestate, Savestate_Silent);
        Reset();
    }
    
    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;

    if (reset_timer > 0) 
        reset_timer--;

    if (reset_timer == 0) {
        reset_timer = -1;
        Reset();
    }
    
    Vec2 cpu_pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    
    if (
        reset_timer == -1
        && (
            cpu_data->flags.dead
            || hmn_data->flags.dead
            || cpu_data->state_id == ASID_CLIFFCATCH
            || cpu_data->state_id == ASID_DOWNWAITU
            || cpu_data->state_id == ASID_DOWNWAITD
            || IsGroundActionable(cpu)
            
            // if the CPU is actionable high above the stage, just reset early.
            || (cpu_pos.Y > 100.f 
                && fabs(cpu_pos.X) < ledge_positions[1].X 
                && IsAirActionable(cpu))
        )
    ) {
        reset_timer = RESET_DELAY;
    }
    
    HSD_Pad *pad = PadGetMaster(hmn_data->pad_index);
    if (pad->down & HSD_BUTTON_DPAD_LEFT)
        reset_timer = 0;
        
    u32 opt_flags = RecoveryMenuOptFlags(cpu);
    Recovery_Think(cpu, opt_flags);
}
