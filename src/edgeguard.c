#include "edgeguard.h"

// how far inland the hmn will start
#define DISTANCE_FROM_LEDGE 15

// how many frames it takes to reset after the cpu dies or successfully recovers
#define RESET_DELAY 30

#define DEGREES_TO_RADIANS 0.01745329252

static int reset_timer = -1;
static Vec2 ledge_positions[2];
static EdgeguardInfo *info;

static inline float fmax(float a, float b) { return a < b ? b : a; }
static inline float fmin(float a, float b) { return a < b ? a : b; }

// set in Event_Init based on character
EventMenu *Event_Menu = &(EventMenu){0};

static void UpdatePosition(GOBJ *fighter) {
    FighterData *data = fighter->userdata;

    Vec3 pos = data->phys.pos;
    data->coll_data.topN_Curr = pos;
    data->coll_data.topN_CurrCorrect = pos;
    data->coll_data.topN_Prev = pos;
    data->coll_data.topN_Proj = pos;
    data->coll_data.coll_test = R13_INT(COLL_TEST);
}

static void GetLedgePositions(Vec2 coords_out[2]) {
    static char ledge_ids[34][2] = {
        { 0xFF, 0xFF }, { 0xFF, 0xFF }, { 0x03, 0x07 }, { 0x33, 0x36 },
        { 0x03, 0x0D }, { 0x29, 0x45 }, { 0x05, 0x11 }, { 0x09, 0x1A },
        { 0x02, 0x06 }, { 0x15, 0x17 }, { 0x00, 0x00 }, { 0x43, 0x4C },
        { 0x00, 0x00 }, { 0x00, 0x00 }, { 0x0E, 0x0D }, { 0x00, 0x00 },
        { 0x00, 0x05 }, { 0x1E, 0x2E }, { 0x0C, 0x0E }, { 0x02, 0x04 },
        { 0x03, 0x05 }, { 0x00, 0x00 }, { 0x06, 0x12 }, { 0x00, 0x00 },
        { 0xD7, 0xE2 }, { 0x00, 0x00 }, { 0x00, 0x00 }, { 0x00, 0x00 },
        { 0x03, 0x05 }, { 0x03, 0x0B }, { 0x06, 0x10 }, { 0x00, 0x05 },
        { 0x00, 0x02 }, { 0x01, 0x01 },
    };

    int stage_id = Stage_GetExternalID();
    char left_id = ledge_ids[stage_id][0];
    char right_id = ledge_ids[stage_id][1];

    Vec3 pos;
    Stage_GetLeftOfLineCoordinates(left_id, &pos);
    coords_out[0] = (Vec2) { pos.X, pos.Y };
    Stage_GetRightOfLineCoordinates(right_id, &pos);
    coords_out[1] = (Vec2) { pos.X, pos.Y };
}

static void UpdateCameraBox(GOBJ *fighter) {
    Fighter_UpdateCameraBox(fighter);

    FighterData *data = fighter->userdata;
    CmSubject *subject = data->camera_subject;
    subject->boundleft_curr = subject->boundleft_proj;
    subject->boundright_curr = subject->boundright_proj;

    Match_CorrectCamera();
}

static float Vec2_Distance(Vec2 *a, Vec2 *b) {
    float dx = a->X - b->X;
    float dy = a->Y - b->Y;
    return sqrtf(dx*dx + dy*dy);
}

static float Vec2_Length(Vec2 *a) {
    float x = a->X;
    float y = a->Y;
    return sqrtf(x*x + y*y);
}

static void simulate_phys(FighterData *data, int future, Vec2 *out) {
    float gravity = data->phys.air_state == 0 ? 0.f : data->attr.gravity;
    
    *out = (Vec2) { data->phys.pos.X, data->phys.pos.Y };
    float y_vel = data->phys.self_vel.Y;
    
    float min_y_vel = data->flags.is_fastfall ?
        -data->attr.fastfall_velocity
        : -data->attr.terminal_velocity;
                
    for (int i = 0; i < future; ++i) {
        out->X += data->phys.self_vel.X;
        out->Y += y_vel;
        y_vel -= gravity;
        y_vel = fmax(y_vel, min_y_vel);
    }
}

static float projected_distance(FighterData *a, FighterData *b, int future) {
    Vec2 a_future, b_future;
    simulate_phys(a, future, &a_future);
    simulate_phys(b, future, &b_future);
    return Vec2_Distance(&a_future, &b_future);
}

static bool enabled(int opt_idx) {
    return info->menu->options[opt_idx].option_val;
}

static int in_hitstun_anim(int state) {
    return ASID_DAMAGEHI1 <= state && state <= ASID_DAMAGEFLYROLL;
}

static int hitstun_ended(GOBJ *fighter) {
    FighterData *data = fighter->userdata;
    float hitstun = *((float*)&data->state_var.state_var1);
    return hitstun == 0.0;
}

static bool air_actionable(GOBJ *fighter) {
    FighterData *data = fighter->userdata;

    // ensure airborne
    if (data->phys.air_state == 0)
        return false;

    int state = data->state_id;

    if (in_hitstun_anim(state) && hitstun_ended(fighter))
        return true;
        
    if (ASID_ATTACKAIRN <= state && state <= ASID_ATTACKAIRLW && data->flags.past_iasa)
        return true;

    return (ASID_JUMPF <= state && state <= ASID_FALLAERIALB)
        || state == ASID_DAMAGEFALL;
}

static bool ground_actionable(GOBJ *fighter) {
    FighterData *data = fighter->userdata;

    // ensure grounded
    if (data->phys.air_state == 1)
        return false;

    int state = data->state_id;

    if (in_hitstun_anim(data) && hitstun_ended(fighter))
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

static void Exit(int value) {
    Match *match = MATCH;
    match->state = 3;
    Match_EndVS();
}

static void Reset(void) {
    for (int ply = 0; ply < 2; ++ply) {
        MatchHUDElement *hud = &stc_matchhud->element_data[ply];
        if (hud->is_removed == 1)
            Match_CreateHUD(ply);
    }

    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;

    cpu_data->cpu.ai = 15;

    int side_idx = HSD_Randi(2);
    int side = side_idx * 2 - 1;

    hmn_data->facing_direction = -side; 
    cpu_data->facing_direction = -side;

    float ledge_x = ledge_positions[side_idx].X - DISTANCE_FROM_LEDGE * side;
    
    hmn_data->input.down = 0;

    // set phys
    cpu_data->phys.kb_vel.X = 0.f;
    cpu_data->phys.kb_vel.Y = 0.f;
    cpu_data->phys.self_vel.X = 0.f;
    cpu_data->phys.self_vel.Y = 0.f;
    hmn_data->phys.kb_vel.X = 0.f;
    hmn_data->phys.kb_vel.Y = 0.f;
    hmn_data->phys.self_vel.X = 0.f;
    hmn_data->phys.self_vel.Y = 0.f;

    hmn_data->phys.pos.X = ledge_x;
    hmn_data->phys.pos.Y = 0.f;
    cpu_data->phys.pos.X = ledge_x;
    cpu_data->phys.pos.Y = 0.f;

    UpdatePosition(hmn);
    UpdatePosition(cpu);
    
    cpu_data->jump.jumps_used = 1;
    hmn_data->jump.jumps_used = 1;

    // set hmn action state
    Fighter_EnterAerial(hmn, ASID_ATTACKAIRB);
    Fighter_ApplyAnimation(hmn, 7, 1, 0);
    hmn_data->state.frame = 7;
    hmn_data->script.script_event_timer = 0;
    Fighter_SubactionFastForward(hmn);
    Fighter_UpdateStateFrameInfo(hmn);
    Fighter_HitboxDisableAll(hmn);
    hmn_data->script.script_current = 0;

    KBValues vals = HitStrength_KBRange[info->menu->options[OPT_HITSTRENGTH].option_val];
    
    float mag = vals.mag_min + (vals.mag_max - vals.mag_min) * HSD_Randf();
    
    int state = mag > 130.f ? ASID_DAMAGEFLYHI : ASID_DAMAGEFLYN;

    // set cpu action state
    Fighter_EnterFall(cpu);
    ActionStateChange(0, 1, 0, cpu, state, 0x40, 0);
    Fighter_UpdateStateFrameInfo(cpu);

    // fix camera
    UpdateCameraBox(hmn);
    UpdateCameraBox(cpu);
    
    // give cpu knockback
    float angle_deg = vals.ang_min + (vals.ang_max - vals.ang_min) * HSD_Randf();
    float angle_rad = angle_deg * DEGREES_TO_RADIANS;

    float vel = mag * (*stc_ftcommon)->force_applied_to_kb_mag_multiplier;
    float vel_x = cos(angle_rad) * vel * (float)side;
    float vel_y = sin(angle_rad) * vel;
    cpu_data->phys.kb_vel.X = vel_x;
    cpu_data->phys.kb_vel.Y = vel_y;

    float kb_frames = (float)(int)((*stc_ftcommon)->x154 * mag);
    *(float*)&cpu_data->state_var.state_var1 = kb_frames;
    cpu_data->flags.hitstun = 1;
    Fighter_EnableCollUpdate(cpu);

    // give hitlag
    hmn_data->dmg.hitlag_frames = 7;
    cpu_data->dmg.hitlag_frames = 7;

    hmn_data->flags.hitlag = 1;
    hmn_data->flags.hitlag_unk = 1;
    cpu_data->flags.hitlag = 1;
    cpu_data->flags.hitlag_unk = 1;

    // random percent
    int dmg = vals.dmg_min + HSD_Randi(vals.dmg_max - vals.dmg_min);

    cpu_data->dmg.percent = dmg;
    Fighter_SetHUDDamage(cpu_data->ply, dmg);

    hmn_data->dmg.percent = 0;
    Fighter_SetHUDDamage(hmn_data->ply, 0);
}

void Event_Init(GOBJ *gobj) {
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *cpu_data = cpu->userdata;
    
    info = &InfoLookup[cpu_data->kind];
    if (info->menu == 0)
        assert("unimplemented character in edgeguard training");
    Event_Menu = info->menu;
    
    GetLedgePositions(&ledge_positions);
    Reset();
}

void Event_Think(GOBJ *menu) {
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
    
    // ensure the player L-cancels the initial bair.
    hmn_data->input.timer_trigger_any_ignore_hitlag = 0;
    
    Vec2 cpu_pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    
    if (
        reset_timer == -1
        && (
            cpu_data->flags.dead
            || hmn_data->flags.dead
            || cpu_data->state_id == ASID_CLIFFCATCH
            || ground_actionable(cpu)
            || (cpu_pos.Y > fabs(cpu_pos.X) && cpu_pos.Y > 100.f && air_actionable(cpu))
        )
    ) {
        reset_timer = RESET_DELAY;
    }

    if (hmn_data->input.down & PAD_BUTTON_DPAD_LEFT)
        reset_timer = 0;
    
    info->Think();
}

// Spacies ------------------------------------------------

#define UPB_CHANCE 8 
#define DOUBLEJUMP_CHANCE_BELOW_LEDGE 5
#define DOUBLEJUMP_CHANCE_ABOVE_LEDGE 30
#define ILLUSION_CHANCE_ABOVE_LEDGE 15
#define ILLUSION_CHANCE_TO_LEDGE 8
#define FASTFALL_CHANCE 8

#define MAX_ILLUSION_HEIGHT 30.f
#define MIN_ILLUSION_HEIGHT -15.f
#define ILLUSION_DISTANCE 75.f
#define FIREFOX_DISTANCE 80.f
#define FIREBIRD_DISTANCE 70.f

#define SWEETSPOT_OFFSET_X 16
#define SWEETSPOT_OFFSET_Y 5
    
static void Think_Spacies(void) {
    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;

    Vec2 pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    Vec2 vel = { cpu_data->phys.self_vel.X, cpu_data->phys.self_vel.Y };
    int state = cpu_data->state_id;
    float dir = pos.X > 0.f ? -1.f : 1.f;
    bool can_jump = cpu_data->jump.jumps_used < 2 && enabled(OPT_SPACIES_JUMP);
    
    Vec2 target_ledge = ledge_positions[pos.X > 0.f];
    Vec2 target_ledgegrab = {
        .X = target_ledge.X - SWEETSPOT_OFFSET_X*dir,
        .Y = target_ledge.Y - SWEETSPOT_OFFSET_Y,
    };
    
    Vec2 vec_to_ledgegrab = {
        .X = target_ledgegrab.X - pos.X,
        .Y = target_ledgegrab.Y - pos.Y,
    };
    float distance_to_ledgegrab = Vec2_Length(&vec_to_ledgegrab);
    
    int dj_chance, illusion_chance;
    if (cpu_data->phys.pos.Y > target_ledge.Y) {
        dj_chance = DOUBLEJUMP_CHANCE_ABOVE_LEDGE;
        illusion_chance = ILLUSION_CHANCE_ABOVE_LEDGE;
    } else {
        dj_chance = DOUBLEJUMP_CHANCE_BELOW_LEDGE;
        illusion_chance = ILLUSION_CHANCE_TO_LEDGE;
    }
    
    bool can_upb = enabled(OPT_SPACIES_FF_LOW)
        | enabled(OPT_SPACIES_FF_MID)
        | enabled(OPT_SPACIES_FF_HIGH);
    
    float upb_distance = cpu_data->kind == FTKIND_FOX ?
        FIREFOX_DISTANCE : FIREBIRD_DISTANCE;
        
    // HITSTUN
    if (cpu_data->flags.hitstun) {
        // DI inwards
        cpu_data->cpu.lstickX = 90 * dir;
        cpu_data->cpu.lstickY = 90;
        
    // ACTIONABLE
    } else if (air_actionable(cpu)) {

        // JUMP
        if (
            enabled(OPT_SPACIES_JUMP)
            && can_jump
            && (
                // force jump if at end of range
                distance_to_ledgegrab > upb_distance
                
                // otherwise, random chance to jump
                || HSD_Randi(dj_chance) == 0
            )
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_Y;
            cpu_data->cpu.lstickX = 127 * dir;
            
        // ILLUSION
        } else if (
            enabled(OPT_SPACIES_ILLUSION) && (
                // force illusion to ledge if no jump and cannot upb
                (
                    !can_upb && !can_jump
                    && MIN_ILLUSION_HEIGHT < pos.Y
                    && pos.Y < MIN_ILLUSION_HEIGHT + 5.f
                )
            
                // random chance to illusion
                || (
                    pos.Y > MIN_ILLUSION_HEIGHT
                    && pos.Y < MAX_ILLUSION_HEIGHT
                    && fabs(vec_to_ledgegrab.X) < ILLUSION_DISTANCE
                    && HSD_Randi(illusion_chance) == 0
                )
            )
        ) {
            cpu_data->cpu.lstickX = 127 * dir;
            cpu_data->cpu.held |= PAD_BUTTON_B;
            
        // FIREFOX
        } else if (
            vel.Y <= 1.5f
            && can_upb
            && (
                // force upb if at end of range
                (pos.Y < 0.f && distance_to_ledgegrab > upb_distance)
    
                // otherwise, random chance to upb
                || (distance_to_ledgegrab < upb_distance && HSD_Randi(UPB_CHANCE) == 0)
            )
        ) {
            cpu_data->cpu.lstickY = 127;
            cpu_data->cpu.held |= PAD_BUTTON_B;
            
        // FASTFALL
        } else if (
            enabled(OPT_SPACIES_FASTFALL)
            && !cpu_data->flags.is_fastfall
            && vel.Y < 0.f
            && HSD_Randi(FASTFALL_CHANCE) == 0
        ) {
            cpu_data->cpu.lstickY = -127;
            
        // WAIT
        } else {
            // drift towards stage
            cpu_data->cpu.lstickX = 127 * dir;
        }
        
    // FIREFOX STARTUP
    } else if (0x161 <= state && state <= 0x162) {
        // compute firefox angle
        
        int low = enabled(OPT_SPACIES_FF_LOW);
        int mid = enabled(OPT_SPACIES_FF_MID);
        int high = enabled(OPT_SPACIES_FF_HIGH);
        int option_count = low + mid + high;
        int choice = HSD_Randi(option_count);
        
        float high_y;
        if (vec_to_ledgegrab.X < upb_distance) {
            high_y = pos.Y + sqrtf(
                upb_distance*upb_distance
                - vec_to_ledgegrab.X*vec_to_ledgegrab.X
            );
        } else {
            high_y = vec_to_ledgegrab.Y;
        }
        
        Vec2 target = { .X = target_ledge.X };
        if (low && choice-- == 0) {
            target.Y = target_ledge.Y;
        } else if (mid && choice-- == 0) {
            target.Y = (target_ledge.Y + high_y) / 2.f;
        } else if (high && choice-- == 0) {
            target.Y = high_y;
        }
        
        Vec2 vec_to_target = {
            .X = target.X - pos.X,
            .Y = target.Y - pos.Y,
        };
        Vec2_Normalize(&vec_to_target);
        
        cpu_data->cpu.lstickX = (s8)(vec_to_target.X * 127.f);
        cpu_data->cpu.lstickY = (s8)(vec_to_target.Y * 127.f);
        
    // INACTIONABLE WITH DRIFT
    } else if (
        // if in firefox ending states
        (0x162 <= state && state <= 0x167)
         
        // or special fall
        || (ASID_FALLSPECIAL <= state && state <= ASID_FALLSPECIALB)
    ) {
        // drift towards stage
        cpu_data->cpu.lstickX = 127 * dir;
    }
}

// Sheik ------------------------------------------------

#define UPB_POOF_BELOW_LEDGE_CHANCE 10
#define UPB_JUMP_HEIGHT 48.f
#define UPB_JUMP_MAX_X 50.f
#define UPB_POOF_DISTANCE 40.f
#define FAIR_SIZE 18

#define DOUBLEJUMP_CHANCE_BELOW_LEDGE 20
#define DOUBLEJUMP_CHANCE_ABOVE_LEDGE 120
#define FASTFALL_CHANCE 15
#define FAIR_CHANCE 5
#define AMSAH_TECH_CHANCE 2

#define SWEETSPOT_OFFSET_X 20
#define SWEETSPOT_OFFSET_Y 5

// There is a frame between inputting the DI for the amsah tech
// and inputting the tech direction. We need to make sure that
// we tech the same as we DI, so we set this flag to prevent the CPU
// from DIing in as usual in that frame.
static bool amsah_teching = false;

static void Think_Sheik(void) {
    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;

    Vec2 pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    Vec2 vel = { cpu_data->phys.self_vel.X, cpu_data->phys.self_vel.Y };
    int state = cpu_data->state_id;
    int hmn_state = hmn_data->state_id;
    float dir = pos.X > 0.f ? -1.f : 1.f;
    bool can_jump = cpu_data->jump.jumps_used < 2 && enabled(OPT_SPACIES_JUMP);
    bool hmn_on_ledge = ASID_CLIFFCATCH <= hmn_state && hmn_state <= ASID_CLIFFJUMPQUICK2;
    
    Vec2 target_ledge = ledge_positions[pos.X > 0.f];
    Vec2 target_ledgegrab = {
        .X = target_ledge.X - SWEETSPOT_OFFSET_X*dir,
        .Y = target_ledge.Y - SWEETSPOT_OFFSET_Y,
    };
    
    Vec2 vec_to_ledgegrab = {
        .X = target_ledgegrab.X - pos.X,
        .Y = target_ledgegrab.Y - pos.Y,
    };
    Vec2 vec_to_ledge = {
        .X = target_ledge.X - pos.X,
        .Y = target_ledge.Y - pos.Y,
    };
    
    // how close sheik will be to the ledge after the jump
    Vec2 post_jump_vec_to_ledge = {
        .X = fmax(fabs(vec_to_ledge.X) - UPB_JUMP_MAX_X, 0.f) * dir,
        .Y = vec_to_ledge.Y - UPB_JUMP_HEIGHT,
    };
    float post_jump_dist_to_ledge = Vec2_Length(&post_jump_vec_to_ledge);
    bool can_upb = post_jump_dist_to_ledge < UPB_POOF_DISTANCE
        && (
            enabled(OPT_SHEIK_UPB_LEDGE)
            || enabled(OPT_SHEIK_UPB_STAGE)
            || enabled(OPT_SHEIK_UPB_HIGH)
        );
    
    int dj_chance;
    if (cpu_data->phys.pos.Y > target_ledge.Y - 10.f) {
        dj_chance = DOUBLEJUMP_CHANCE_ABOVE_LEDGE;
    } else {
        dj_chance = DOUBLEJUMP_CHANCE_BELOW_LEDGE;
    }
    
    if (!cpu_data->flags.hitstun)
        amsah_teching = false;
    
    // AMSAH TECH
    if (
        enabled(OPT_SHEIK_AMSAH_TECH)
        && in_hitstun_anim(state)
        && cpu_data->TM.state_prev[0] == ASID_LANDINGFALLSPECIAL
        && cpu_data->flags.hitlag
        && cpu_data->dmg.hitlag_frames == 1.f
        && HSD_Randi(AMSAH_TECH_CHANCE) == 0
    ) {
        float hit_angle = Fighter_GetKnockbackAngle(cpu_data);
        cpu_data->cpu.held |= PAD_TRIGGER_R;
        cpu_data->cpu.lstickX = pos.X < hmn_data->phys.pos.X ? -80 : 80;
        cpu_data->cpu.lstickY = -80;
        cpu_data->cpu.cstickY = -127;
        amsah_teching = true;
    
        
    // HITSTUN
    } else if (cpu_data->flags.hitstun) {
        if (amsah_teching) {
            cpu_data->cpu.lstickX = pos.X < hmn_data->phys.pos.X ? -80 : 80;
            cpu_data->cpu.lstickY = -80;
        } else {
            // DI inwards
            cpu_data->cpu.lstickX = 90 * dir;
            cpu_data->cpu.lstickY = 90;
        }
        
    // ACTIONABLE
    } else if (air_actionable(cpu)) {
        // JUMP
        if (
            enabled(OPT_SHEIK_JUMP)
            && can_jump
            && (
                // force jump if at end of range
                (
                    post_jump_dist_to_ledge > UPB_POOF_DISTANCE
                    && vec_to_ledgegrab.Y > 10.f
                )
                
                // otherwise, random chance to jump
                || HSD_Randi(dj_chance) == 0
            )
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_Y;
            cpu_data->cpu.lstickX = 127 * dir;
            
        // FAIR
        } else if (
            enabled(OPT_SHEIK_FAIR)
            && !cpu_data->flags.is_fastfall
            && pos.X * dir < hmn_data->phys.pos.X * dir
            && hmn_data->hurt.intang_frames.ledge < 5
            && projected_distance(hmn_data, cpu_data, 6.f) < FAIR_SIZE
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_A;
            cpu_data->cpu.lstickX = 127 * dir;
            
        // UP B
        } else if (
            // random chance to upb
            (
                vec_to_ledgegrab.Y > 10.f
                && can_upb
                && HSD_Randi(UPB_POOF_BELOW_LEDGE_CHANCE) == 0
            )
            
            // force upb if at end of range
            || (
                vec_to_ledgegrab.Y > UPB_JUMP_HEIGHT 
                && can_upb
                && post_jump_dist_to_ledge < UPB_POOF_DISTANCE
            )
        ) {
            cpu_data->cpu.lstickY = 127;
            cpu_data->cpu.held |= PAD_BUTTON_B;
        
        // FASTFALL
        } else if (
            enabled(OPT_SHEIK_FASTFALL)
            && !cpu_data->flags.is_fastfall
            && vel.Y < 0.f
            && HSD_Randi(FASTFALL_CHANCE) == 0
        ) {
            cpu_data->cpu.lstickY = -127;
            
        // WAIT 
        } else {
            cpu_data->cpu.lstickX = 127 * dir;
        }
    // VANISH STARTUP
    } else if (state == 0x166) {
        // choose poof direction
        if (cpu_data->TM.state_frame == 34) {
            // int ledge = enabled(OPT_SHEIK_UPB_LEDGE);
            int ledge = false;
            int stage_tip = enabled(OPT_SHEIK_UPB_LEDGE);
            int above_ledge = enabled(OPT_SHEIK_UPB_HIGH) && vec_to_ledgegrab.X < UPB_POOF_DISTANCE;
            int high = enabled(OPT_SHEIK_UPB_HIGH);
            int stage_inland = enabled(OPT_SHEIK_UPB_STAGE) && pos.Y > 0.f;
            
            int option_count = ledge + above_ledge + stage_tip + stage_inland + high;
            int choice = HSD_Randi(option_count);
            
            Vec2 target = {0};
            
            // upb to ledge
            if ((ledge && choice-- == 0) || option_count == 0) {
                target = target_ledge;
                target.Y -= 13.f;
                    
            // upb to stage tip
            } else if (stage_tip && choice-- == 0) {
                target = target_ledge;
                target.Y -= 2.f;
                if (pos.Y < target_ledge.Y)
                    target.Y -= 2.f;
                if (pos.Y < target_ledge.Y-10.f)
                    target.Y += 1.f;
                if (pos.Y < target_ledge.Y-28.f)
                    target.Y += 0.5f;
                
            // upb above ledge
            } else if (above_ledge && choice-- == 0) {
                float high_y = pos.Y + sqrtf(
                    UPB_POOF_DISTANCE*UPB_POOF_DISTANCE
                    - vec_to_ledgegrab.X*vec_to_ledgegrab.X
                );
                target = (Vec2) { target_ledge.X, high_y };
                
            // upb high to platform
            } else if (high && choice-- == 0) {
                target = (Vec2) { pos.X + dir, pos.Y + 1.f }; // TODO BETTER TARGETING
                
            // upb inwards to stage
            } else if (stage_inland && choice-- == 0) {
                target = (Vec2) { 0.f, 0.f }; // TODO BETTER TARGETING
            }
            
            Vec2 poof_dir = {
                target.X - pos.X,
                target.Y - pos.Y,
            };
            Vec2_Normalize(&poof_dir);
            
            cpu_data->cpu.lstickX = (s8)(poof_dir.X * 127.f);
            cpu_data->cpu.lstickY = (s8)(poof_dir.Y * 127.f);
            
        } else {
            if (pos.Y > target_ledge.Y) {
                cpu_data->cpu.lstickX = dir * 127.f;
            } else {
                cpu_data->cpu.lstickX = sign(vec_to_ledgegrab.X) * 127.f;
            }
        }
    
    // SPECIAL FALL
    } else if (ASID_FALLSPECIAL <= state && state <= ASID_FALLSPECIALB) {
        // drift towards stage
        cpu_data->cpu.lstickX = 127 * dir;
    }
}
