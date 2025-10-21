// Include this file in your event source and call Recovery_Think. Easy!
//
// You can set opt_flags manually, or use RecoveryMenuOptFlags with the given menus.

#include "../MexTK/mex.h"
#include "events.h"

static void Think_Spacies(GOBJ *cpu, u32 opt_flags);
static void Think_Sheik(GOBJ *cpu, u32 opt_flags);
static void Think_Falcon(GOBJ *cpu, u32 opt_flags);
static void Think_Marth(GOBJ *cpu, u32 opt_flags);

typedef void (*ThinkFn)(GOBJ *, u32);
static ThinkFn ThinkFnLookup[] = {
    0, // MARIO
    Think_Spacies, // FOX
    Think_Falcon, // FALCON
    0, // DK
    0, // KIRBY
    0, // BOWSER
    0, // LINK
    Think_Sheik, // SHEIK
    0, // NESS
    0, // PEACH
    0, // POPO
    0, // NANA
    0, // PIKACHU
    0, // SAMUS
    0, // YOSHI
    0, // JIGGLYPUFF
    0, // MEWTWO
    0, // LUIGI
    Think_Marth, // MARTH
    0, // ZELDA
    0, // YOUNGLINK
    0, // DRMARIO
    Think_Spacies, // FALCO
    0, // PICHU
    0, // GAW,
    0, // GANONDORF
    0, // ROY
};

static Vec2 ledge_positions[2];

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

#define OptEnabled(opt) (((opt_flags) & (1u << (opt))) != 0)

#define CMN_FALL_MIN_DISTANCE 20.0f

void Recovery_Think(GOBJ *cpu, u32 opt_flags) {
    if (ledge_positions[0].X == 0.0f)
        GetLedgePositions(ledge_positions);
        
    FighterData *cpu_data = cpu->userdata;
    cpu_data->cpu.ai = 15; // noact
    
    // Common thinking -----------------------
    
    float x = cpu_data->phys.pos.X;
    int state = cpu_data->state_id;
    
    // wall tech
    cpu_data->input.timer_LR = 0;
    
    // if falling on stage, don't do anything
    if (
        (ASID_FALL <= state && state <= ASID_FALLAERIALB)
        || (ASID_JUMPF <= state && state <= ASID_JUMPAERIALB)
    ) {
        float lx = ledge_positions[0].X;
        float rx = ledge_positions[1].X;
        
        // if withing stage, at greater than CMN_FALL_MIN_DISTANCE units from ledge, just drop down.
        if (lx + CMN_FALL_MIN_DISTANCE <= x && x <= rx - CMN_FALL_MIN_DISTANCE) {
            cpu_data->cpu.lstickX = 0;
            cpu_data->cpu.lstickY = 0;
            return;
        
        // if within stage, but not greater than CMN_FALL_MIN_DISTANCE units from ledge, drift inwards.
        } else if (lx <= x && x <= rx) {
            cpu_data->cpu.lstickX = -127 * sign(x);
            cpu_data->cpu.lstickY = 0;
            return;
        } 
    }
    
    // normal getup
    if (state == ASID_CLIFFWAIT && cpu_data->TM.state_frame == 2) {
        cpu_data->cpu.lstickX = -127 * sign(x);
        cpu_data->cpu.lstickY = 0;
        return;
    }
    
    // angel platform
    if (state == ASID_REBIRTHWAIT) {
        cpu_data->cpu.lstickX = 0;
        cpu_data->cpu.lstickY = -127;
        return;
    }
    
    // Character specific thinking ------------
    
    ThinkFn think_fn = ThinkFnLookup[cpu_data->kind];
    if (think_fn) {
        think_fn(cpu, opt_flags);
    } else {
        cpu_data->cpu.ai = 0;
    }
}

enum options_spacies {
    OPT_SPACIES_FF_LOW,
    OPT_SPACIES_FF_MID,
    OPT_SPACIES_FF_HIGH,
    OPT_SPACIES_JUMP,
    OPT_SPACIES_ILLUSION,
    OPT_SPACIES_FASTFALL,
    
    OPT_SPACIES_COUNT
};

enum options_sheik {
    OPT_SHEIK_UPB_LEDGE,
    OPT_SHEIK_UPB_STAGE,
    OPT_SHEIK_UPB_HIGH,
    OPT_SHEIK_JUMP,
    OPT_SHEIK_FAIR,
    OPT_SHEIK_AMSAH_TECH,
    
    OPT_SHEIK_COUNT
};

enum options_falcon {
    OPT_FALCON_UPB,
    OPT_FALCON_DRIFT_BACK,
    OPT_FALCON_JUMP,
    OPT_FALCON_FASTFALL,
    OPT_FALCON_DOWNB,

    OPT_FALCON_COUNT
};

enum options_marth {
    OPT_MARTH_UPB_EARLY,
    OPT_MARTH_JUMP,
    OPT_MARTH_SIDEB,
    OPT_MARTH_FAIR,

    OPT_MARTH_COUNT
};
 
// Fox -------------------------------------------------

static EventOption RecOptions_Fox[] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Firefox Low",
        .desc = {"Allow Fox to aim his up special to the ledge."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Firefox Mid",
        .desc = {"Allow Fox to aim his up special to the stage."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Firefox High",
        .desc = {"Allow Fox to aim his up special high."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Double Jump",
        .desc = {"Allow Fox to double jump."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Illusion",
        .desc = {"Allow Fox to side special."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fast Fall",
        .desc = {"Allow Fox to fast fall."},
    },
};
static EventMenu RecMenu_Fox = {
    .name = "Fox Recovery",
    .option_num = sizeof(RecOptions_Fox) / sizeof(EventOption),
    .options = RecOptions_Fox,
};

// Falco -------------------------------------------------

static EventOption RecOptions_Falco[] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Firebird Low",
        .desc = {"Allow Falco to aim his up special to the ledge."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Firebird Mid",
        .desc = {"Allow Falco to aim his up special to the stage."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Firebird High",
        .desc = {"Allow Falco to aim his up special high."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Double Jump",
        .desc = {"Allow Falco to double jump."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Phantasm",
        .desc = {"Allow Falco to side special."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fast Fall",
        .desc = {"Allow Falco to fast fall."},
    },
};
static EventMenu RecMenu_Falco = {
    .name = "Falco Recovery",
    .option_num = sizeof(RecOptions_Falco) / sizeof(EventOption),
    .options = RecOptions_Falco,
};

// Sheik -------------------------------------------------

static EventOption RecOptions_Sheik[] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Vanish to Ledge",
        .desc = {"Allow Sheik to vanish to ledge and the",
                 "tip of the stage."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Vanish to Stage",
        .desc = {"Allow Sheik to vanish deep into stage."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Vanish High",
        .desc = {"Allow Sheik to vanish above the ledge",
                 "and to platforms."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Double Jump",
        .desc = {"Allow Sheik to double jump."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fair",
        .desc = {"Allow Sheik to fair the player."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Amsah Tech",
        .desc = {"Allow Sheik to Amsah Tech."},
    },
};

static EventMenu RecMenu_Sheik = {
    .name = "Sheik Recovery",
    .option_num = sizeof(RecOptions_Sheik) / sizeof(EventOption),
    .options = RecOptions_Sheik,
};

// Captain Falcon --------------------------------------

static EventOption RecOptions_Falcon[] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Falcon Dive",
        .desc = {"Allow Falcon to Up B."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Drift Back",
        .desc = {"Allow Falcon to drift backwards."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Double Jump",
        .desc = {"Allow Falcon to double jump."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fast Fall",
        .desc = {"Allow Falcon to fast fall."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Falcon Kick",
        .desc = {"Allow Falcon to Down B."},
    },
};

static EventMenu RecMenu_Falcon = {
    .name = "Falcon Recovery",
    .option_num = sizeof(RecOptions_Falcon) / sizeof(EventOption),
    .options = RecOptions_Falcon,
};

// Marth --------------------------------------

static EventOption RecOptions_Marth[] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Dolphin Slash Early",
        .desc = {"Allow Marth to Up B above the ledge."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Double Jump",
        .desc = {"Allow Marth to double jump."},
        .val = 1,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Dancing Blade",
        .desc = {"Allow Marth to use SideB to recover and",
                "delay his timing."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fair",
        .desc = {"Allow Marth to fair the player."},
    },
};

static EventMenu RecMenu_Marth = {
    .name = "Marth Recovery",
    .option_num = sizeof(RecOptions_Marth) / sizeof(EventOption),
    .options = RecOptions_Marth,
};

EventMenu *RecoveryMenus[] = {
    0, // MARIO
    &RecMenu_Fox,
    &RecMenu_Falcon,
    0, // DK
    0, // KIRBY
    0, // BOWSER
    0, // LINK
    &RecMenu_Sheik,
    0, // NESS
    0, // PEACH
    0, // POPO
    0, // NANA
    0, // PIKACHU
    0, // SAMUS
    0, // YOSHI
    0, // JIGGLYPUFF
    0, // MEWTWO
    0, // LUIGI
    &RecMenu_Marth,
    0, // ZELDA
    0, // YOUNGLINK
    0, // DRMARIO
    &RecMenu_Falco,
    0, // PICHU
    0, // GAW,
    0, // GANONDORF
    0, // ROY
};

// Helper FNs ---------------------------------------------

u32 RecoveryMenuOptFlags(GOBJ *cpu) {
    FighterData *cpu_data = cpu->userdata;
    EventMenu *menu = RecoveryMenus[cpu_data->kind];
    if (menu == 0)
        return 0;

    u32 opt_flags = 0;
    for (u32 opt = 0; opt < (u32)menu->option_num; ++opt)
        opt_flags |= (u32)menu->options[opt].val << opt;
    return opt_flags;
}

static inline float Progress(float n, float a, float b) { return (n - a) / (b - a); }
static inline bool Within(float a, float b, float tolerance) { return fabs(b - a) < tolerance; }

static int HitstunEnded(GOBJ *fighter) {
    FighterData *data = fighter->userdata;
    float hitstun;
    memcpy(&hitstun, &data->state_var.state_var1, sizeof(float));
    return hitstun == 0.0;
}

static int InHitstunAnim(int state) {
    return ASID_DAMAGEHI1 <= state && state <= ASID_DAMAGEFLYROLL;
}

static bool IsAirActionable(GOBJ *fighter) {
    FighterData *data = fighter->userdata;
    
    // ensure airborne
    if (data->phys.air_state == 0)
        return false;

    int state = data->state_id;

    if (InHitstunAnim(state)) {
        if (HitstunEnded(fighter))
            return true;
        
        FtDamage *dmg = (FtDamage *)&data->state_var;
        if (dmg->is_meteor && dmg->meteor_lockout == 0)
            return true;
    }
    
    if (state == ASID_PASSIVEWALL && data->TM.state_frame > 5)
        return true;
        
    if (state == ASID_PASSIVEWALLJUMP)
        return true;
        
    if (ASID_ATTACKAIRN <= state && state <= ASID_ATTACKAIRLW && data->flags.past_iasa)
        return true;

    return (ASID_JUMPF <= state && state <= ASID_FALLAERIALB)
        || state == ASID_DAMAGEFALL;
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

static void PhysStep(FighterData *data, Vec2 *pos, Vec2 *vel) {
    float gravity = data->phys.air_state == 0 ? 0.f : data->attr.gravity;
    float min_y_vel = data->flags.is_fastfall ?
        -data->attr.fastfall_velocity
        : -data->attr.terminal_velocity;
        
    pos->X += vel->X;
    pos->Y += vel->Y;
    vel->Y -= gravity;
    vel->Y = fmax(vel->Y, min_y_vel);
}

static Vec2 SimulatePhys(FighterData *data, int future) {
    Vec2 pos = { data->phys.pos.X, data->phys.pos.Y };
    Vec2 vel = { data->phys.self_vel.X, data->phys.self_vel.Y };
    for (int i = 0; i < future; ++i)
        PhysStep(data, &pos, &vel);
    
    return pos;
}

static bool SimulatePhys_CanReachPoint(FighterData *data, Vec2 target) {
    if (data->phys.air_state == 0) return false; 

    Vec2 pos = { data->phys.pos.X, data->phys.pos.Y };
    Vec2 vel = { data->phys.self_vel.X, data->phys.self_vel.Y };
    
    float dir = sign(target.X - pos.X);
    if (sign(vel.X) != dir)
        return false;
    
    while (target.Y < pos.Y || vel.Y >= 0.f)
        PhysStep(data, &pos, &vel);
        
    return dir != sign(target.X - pos.X);
}

static float ProjectedDistance(FighterData *a, FighterData *b, int future) {
    Vec2 a_future = SimulatePhys(a, future);
    Vec2 b_future = SimulatePhys(b, future);
    return Vec2_Distance(&a_future, &b_future);
}

// Spacies ------------------------------------------------

#define SP_UPB_CHANCE 12 
#define SP_JUMP_CHANCE_BELOW_LEDGE 4
#define SP_JUMP_CHANCE_ABOVE_LEDGE 30
#define SP_ILLUSION_CHANCE_ABOVE_LEDGE 30
#define SP_ILLUSION_CHANCE_TO_LEDGE 12
#define SP_FASTFALL_CHANCE 8

#define SP_MAX_ILLUSION_HEIGHT 30.f
#define SP_MIN_ILLUSION_HEIGHT -15.f
#define SP_ILLUSION_DISTANCE 75.f
#define SP_FIREFOX_DISTANCE 80.f
#define SP_FIREBIRD_DISTANCE 70.f

#define SP_SWEETSPOT_OFFSET_X 16
#define SP_SWEETSPOT_OFFSET_Y 5
    
static void Think_Spacies(GOBJ *cpu, u32 opt_flags) {
    FighterData *cpu_data = cpu->userdata;

    Vec2 pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    Vec2 vel = { cpu_data->phys.self_vel.X, cpu_data->phys.self_vel.Y };
    int state = cpu_data->state_id;
    float dir = pos.X > 0.f ? -1.f : 1.f;
    bool can_jump = cpu_data->jump.jumps_used < 2 && OptEnabled(OPT_SPACIES_JUMP);
    
    Vec2 target_ledge = ledge_positions[pos.X > 0.f];
    Vec2 target_ledgegrab = {
        .X = target_ledge.X - SP_SWEETSPOT_OFFSET_X*dir,
        .Y = target_ledge.Y - SP_SWEETSPOT_OFFSET_Y,
    };
    
    Vec2 vec_to_ledgegrab = {
        .X = target_ledgegrab.X - pos.X,
        .Y = target_ledgegrab.Y - pos.Y,
    };
    float distance_to_ledgegrab = Vec2_Length(&vec_to_ledgegrab);
    
    int dj_chance, illusion_chance;
    if (cpu_data->phys.pos.Y > target_ledge.Y) {
        dj_chance = SP_JUMP_CHANCE_ABOVE_LEDGE;
        illusion_chance = SP_ILLUSION_CHANCE_ABOVE_LEDGE;
    } else {
        dj_chance = SP_JUMP_CHANCE_BELOW_LEDGE;
        illusion_chance = SP_ILLUSION_CHANCE_TO_LEDGE;
    }
    
    bool can_upb = OptEnabled(OPT_SPACIES_FF_LOW)
        | OptEnabled(OPT_SPACIES_FF_MID)
        | OptEnabled(OPT_SPACIES_FF_HIGH);
    
    float upb_distance = cpu_data->kind == FTKIND_FOX ?
        SP_FIREFOX_DISTANCE : SP_FIREBIRD_DISTANCE;
        
    // HITSTUN
    if (cpu_data->flags.hitlag) {
        // DI inwards
        cpu_data->cpu.lstickX = 90 * dir;
        cpu_data->cpu.lstickY = 90;
        
    // ACTIONABLE
    } else if (IsAirActionable(cpu)) {

        // JUMP
        if (
            OptEnabled(OPT_SPACIES_JUMP)
            && can_jump
            && (
                // force jump if at end of range
                distance_to_ledgegrab > upb_distance
                
                // otherwise, random chance to jump
                || HSD_Randi(dj_chance) == 0
            )
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_Y;
            if (distance_to_ledgegrab > upb_distance)
                cpu_data->cpu.lstickX = 127 * dir;
            else 
                cpu_data->cpu.lstickX = 127 * (HSD_Randi(3) - 1);
            
        // ILLUSION
        } else if (
            OptEnabled(OPT_SPACIES_ILLUSION) && (
                // force illusion to ledge if no jump and cannot upb
                (
                    !can_upb && !can_jump
                    && SP_MIN_ILLUSION_HEIGHT < pos.Y
                    && pos.Y < SP_MIN_ILLUSION_HEIGHT + 5.f
                )
            
                // random chance to illusion
                || (
                    pos.Y > SP_MIN_ILLUSION_HEIGHT
                    && pos.Y < SP_MAX_ILLUSION_HEIGHT
                    && fabs(vec_to_ledgegrab.X) < SP_ILLUSION_DISTANCE
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
                || (
                    distance_to_ledgegrab < upb_distance
                    && (
                        pos.X < ledge_positions[0].X
                        || pos.X > ledge_positions[1].X
                    ) && HSD_Randi(SP_UPB_CHANCE) == 0
                )
            )
        ) {
            cpu_data->cpu.lstickY = 127;
            cpu_data->cpu.held |= PAD_BUTTON_B;
            
        // FASTFALL
        } else if (
            OptEnabled(OPT_SPACIES_FASTFALL)
            && !cpu_data->flags.is_fastfall
            && vel.Y < 0.f
            && HSD_Randi(SP_FASTFALL_CHANCE) == 0
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
        
        int low = OptEnabled(OPT_SPACIES_FF_LOW);
        int mid = OptEnabled(OPT_SPACIES_FF_MID);
        int high = OptEnabled(OPT_SPACIES_FF_HIGH);
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
        
        Vec2 target = { .X = target_ledgegrab.X };
        if (low && choice-- == 0) {
            target.Y = target_ledgegrab.Y;
        } else if (mid && choice-- == 0) {
            target.Y = (target_ledgegrab.Y + high_y) / 2.f;
        } else if (high && choice-- == 0) {
            target.Y = high_y;
        }
        
        Vec2 vec_to_target = {
            .X = target.X - pos.X,
            .Y = target.Y - pos.Y,
        };
        Vec2_Normalize(&vec_to_target);
        
        // place angles barely inside deadzone outside of deadzone
        if (-0.2875f < vec_to_target.X && vec_to_target.X <= -0.1375f)
            vec_to_target.X = -0.2875f;
        else if (0.2875f > vec_to_target.X && vec_to_target.X >= 0.1375f)
            vec_to_target.X = 0.2875f;

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

#define SK_UPB_POOF_NOT_ON_LEDGE_CHANCE 10
#define SK_UPB_POOF_ON_LEDGE_CHANCE 5
#define SK_UPB_JUMP_HEIGHT 48.f
#define SK_UPB_JUMP_MAX_X 50.f
#define SK_UPB_POOF_DISTANCE 40.f
#define SK_FAIR_SIZE 18

#define SK_JUMP_CHANCE_FAR_BELOW_LEDGE 4
#define SK_JUMP_CHANCE_BELOW_LEDGE 30
#define SK_JUMP_CHANCE_ABOVE_LEDGE 120
#define SK_FAIR_CHANCE 5
#define SK_AMSAH_TECH_CHANCE 1

#define SK_SWEETSPOT_OFFSET_X 15
#define SK_SWEETSPOT_OFFSET_Y 5

// There is a frame between inputting the DI for the amsah tech
// and inputting the tech direction. We need to make sure that
// we tech the same as we DI, so we set this flag to prevent the CPU
// from DIing in as usual in that frame.
static bool amsah_teching = false;

static void Think_Sheik(GOBJ *cpu, u32 opt_flags) {
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;

    Vec2 pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    Vec2 vel = { cpu_data->phys.self_vel.X, cpu_data->phys.self_vel.Y };
    int state = cpu_data->state_id;
    float dir = pos.X > 0.f ? -1.f : 1.f;
    bool can_jump = cpu_data->jump.jumps_used < 2 && OptEnabled(OPT_SHEIK_JUMP);
    bool hmn_on_ledge = hmn_data->state_id == ASID_CLIFFWAIT;
    
    Vec2 target_ledge = ledge_positions[pos.X > 0.f];
    Vec2 target_ledgegrab = {
        .X = target_ledge.X - SK_SWEETSPOT_OFFSET_X*dir,
        .Y = target_ledge.Y - SK_SWEETSPOT_OFFSET_Y,
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
        .X = fmax(fabs(vec_to_ledge.X) - SK_UPB_JUMP_MAX_X, 0.f) * dir,
        .Y = vec_to_ledge.Y - SK_UPB_JUMP_HEIGHT,
    };
    float post_jump_dist_to_ledge = Vec2_Length(&post_jump_vec_to_ledge);
    bool can_upb = post_jump_dist_to_ledge < SK_UPB_POOF_DISTANCE
        && vel.Y < 1.f
        && (!can_jump || post_jump_dist_to_ledge < 1.f)
        && (
            OptEnabled(OPT_SHEIK_UPB_LEDGE)
            || OptEnabled(OPT_SHEIK_UPB_STAGE)
            || OptEnabled(OPT_SHEIK_UPB_HIGH)
        );
    
    int dj_chance;
    if (cpu_data->phys.pos.Y > target_ledge.Y - 10.f) {
        dj_chance = SK_JUMP_CHANCE_ABOVE_LEDGE;
    } else if (cpu_data->phys.pos.Y > target_ledge.Y - 50.f) {
        dj_chance = SK_JUMP_CHANCE_BELOW_LEDGE;
    } else {
        dj_chance = SK_JUMP_CHANCE_FAR_BELOW_LEDGE;
    }
    
    if (!cpu_data->flags.hitstun)
        amsah_teching = false;
    
    // AMSAH TECH
    if (
        OptEnabled(OPT_SHEIK_AMSAH_TECH)
        && InHitstunAnim(state)
        && cpu_data->TM.state_prev[0] == ASID_LANDINGFALLSPECIAL
        && cpu_data->flags.hitlag
        && cpu_data->dmg.hitlag_frames == 1.f
        && HSD_Randi(SK_AMSAH_TECH_CHANCE) == 0
    ) {
        cpu_data->cpu.held |= PAD_TRIGGER_R;
        cpu_data->cpu.lstickX = 80 * -(int)cpu_data->dmg.hit_log.direction;
        cpu_data->cpu.lstickY = -80;
        cpu_data->cpu.cstickY = -127;
        amsah_teching = true;
    
        
    // HITSTUN
    } else if (cpu_data->flags.hitlag && !amsah_teching) {
        // DI inwards
        cpu_data->cpu.lstickX = 90 * dir;
        cpu_data->cpu.lstickY = 90;
    } else if (cpu_data->flags.hitstun && amsah_teching) {
        cpu_data->cpu.lstickX = pos.X < hmn_data->phys.pos.X ? -80 : 80;
        cpu_data->cpu.lstickY = -80;
        
    // ACTIONABLE
    } else if (IsAirActionable(cpu)) {
        // JUMP
        if (
            can_jump
            && (
                // force jump if at end of range
                (
                    post_jump_dist_to_ledge > SK_UPB_POOF_DISTANCE
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
            OptEnabled(OPT_SHEIK_FAIR)
            && !cpu_data->flags.is_fastfall
            && pos.X * dir < hmn_data->phys.pos.X * dir
            && hmn_data->hurt.intang_frames.ledge < 5
            && ProjectedDistance(hmn_data, cpu_data, 6) < SK_FAIR_SIZE
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_A;
            cpu_data->cpu.lstickX = 127 * dir;
            
        // UP B
        } else if (
            // random chance to upb
            (
                vec_to_ledgegrab.Y > 10.f
                && can_upb
                && !can_jump
                && !hmn_on_ledge
                && HSD_Randi(SK_UPB_POOF_NOT_ON_LEDGE_CHANCE) == 0
            )
            
            || (
                can_upb
                && !can_jump
                && hmn_on_ledge
                && HSD_Randi(SK_UPB_POOF_ON_LEDGE_CHANCE) == 0
            )
            
            // force upb if at end of range
            || (
                vec_to_ledgegrab.Y > SK_UPB_JUMP_HEIGHT 
                && can_upb
                && !can_jump
                && post_jump_dist_to_ledge < SK_UPB_POOF_DISTANCE
            )
        ) {
            cpu_data->cpu.lstickY = 127;
            cpu_data->cpu.held |= PAD_BUTTON_B;
        
        // WAIT 
        } else {
            if (pos.X * dir < target_ledgegrab.X * dir)
                cpu_data->cpu.lstickX = 127 * dir;
            else
                cpu_data->cpu.lstickX = -127 * dir;
        }
    // VANISH STARTUP
    } else if (state == 0x166) {
        // choose poof direction
        if (cpu_data->TM.state_frame == 34) {
            int ledge = OptEnabled(OPT_SHEIK_UPB_LEDGE);
            int stage_tip = OptEnabled(OPT_SHEIK_UPB_LEDGE);
            int above_ledge = OptEnabled(OPT_SHEIK_UPB_HIGH) && vec_to_ledgegrab.X < SK_UPB_POOF_DISTANCE;
            int high = OptEnabled(OPT_SHEIK_UPB_HIGH);
            int stage_inland = OptEnabled(OPT_SHEIK_UPB_STAGE) && pos.Y > 0.f;
            
            // double stage_inland chance
            int option_count = ledge + above_ledge + stage_tip + stage_inland + stage_inland + high;
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
                    SK_UPB_POOF_DISTANCE*SK_UPB_POOF_DISTANCE
                    - vec_to_ledgegrab.X*vec_to_ledgegrab.X
                );
                target = (Vec2) { target_ledge.X, high_y };
                
            // upb high to platform
            } else if (high && choice-- == 0) {
                target = (Vec2) { pos.X + dir, pos.Y + 1.f }; // TODO BETTER TARGETING
                
            // upb inwards to stage (doubled chance)
            } else if (stage_inland && choice-- <= 1) {
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

// Falcon ------------------------------------------------

#define CF_SWEETSPOT_OFFSET_X 13
#define CF_SWEETSPOT_OFFSET_Y 8
#define CF_UPB_HEIGHT 48
#define CF_JUMP_HEIGHT 40

#define CF_UPB_CHANCE_FAR 5
#define CF_UPB_CHANCE_WITH_JUMP 50
#define CF_UPB_CHANCE_CLOSE 100
#define CF_UPB_DRIFT_CHANGE_CHANCE 20
#define CF_FALL_DRIFT_CHANGE_CHANCE 40
#define CF_DOWNB_CHANCE 3
#define CF_JUMP_CHANCE_BELOW_LEDGE 20
#define CF_JUMP_CHANCE_ABOVE_LEDGE 40 
#define CF_JUMP_CHANCE_TO_LEDGE 8 
#define CF_FASTFALL_CHANCE 15

#define CF_DRIFT_DURATION_LENGTH_CONST 5
#define CF_DRIFT_DURATION_LENGTH_RNG 30

static int drift_back_timer = 0;

static void Think_Falcon(GOBJ *cpu, u32 opt_flags) {
    FighterData *cpu_data = cpu->userdata;

    Vec2 pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    Vec2 vel = { cpu_data->phys.self_vel.X, cpu_data->phys.self_vel.Y };
    int state = cpu_data->state_id;
    float dir = pos.X > 0.f ? -1.f : 1.f;
    
    Vec2 target_ledge = ledge_positions[pos.X > 0.f];
    Vec2 target_ledgegrab = {
        .X = target_ledge.X - CF_SWEETSPOT_OFFSET_X*dir,
        .Y = target_ledge.Y - CF_SWEETSPOT_OFFSET_Y,
    };
    
    Vec2 vec_to_ledgegrab = {
        .X = target_ledgegrab.X - pos.X,
        .Y = target_ledgegrab.Y - pos.Y,
    };
    Vec2 vec_to_ledge = {
        .X = target_ledge.X - pos.X,
        .Y = target_ledge.Y - pos.Y,
    };
    bool past_ledge = fabs(pos.X) < fabs(target_ledgegrab.X);
    bool can_jump = cpu_data->jump.jumps_used < 2 && OptEnabled(OPT_FALCON_JUMP);
    
    int dj_chance;
    if (Within(vec_to_ledge.Y, CF_JUMP_HEIGHT - 5.f, 10.f) && vec_to_ledge.X * dir < 50.f) {
        dj_chance = CF_JUMP_CHANCE_TO_LEDGE;
    } else if (cpu_data->phys.pos.Y > target_ledge.Y - 10.f) {
        dj_chance = CF_JUMP_CHANCE_ABOVE_LEDGE;
    } else {
        dj_chance = CF_JUMP_CHANCE_BELOW_LEDGE;
    }
    
    bool can_fall_to_ledge = SimulatePhys_CanReachPoint(cpu_data, target_ledgegrab);
    int upb_chance;
    if (can_fall_to_ledge) {
        upb_chance = CF_UPB_CHANCE_CLOSE;
    } else if (can_jump) {
        upb_chance = CF_UPB_CHANCE_WITH_JUMP;
    } else {
        upb_chance = CF_UPB_CHANCE_FAR;
    }
    
    bool in_drift_back_zone = past_ledge
        || (
            vec_to_ledgegrab.Y < -30.f
            && (
                // rising drift
                (vel.Y > 1.f && -vec_to_ledgegrab.Y / 1.5f > fabs(vec_to_ledgegrab.X))
                
                // falling drift
                || (vel.Y <= 1.f && -vec_to_ledgegrab.Y / 3.f > fabs(vec_to_ledgegrab.X))
            )
        );
    
    if (cpu_data->flags.hitlag) {
        // DI inwards
        cpu_data->cpu.lstickX = 90 * dir;
        cpu_data->cpu.lstickY = 90;
        
    // STOP DRIFT BACK
    } else if (!in_drift_back_zone && drift_back_timer) {
        drift_back_timer = 0;
        cpu_data->cpu.lstickX = 0;
    
    // DRIFT BACK
    } else if (drift_back_timer) {
        drift_back_timer--;
        if (drift_back_timer)
            cpu_data->cpu.lstickX = -127 * dir;
        else
            // frame with no drift - this allows faster drift change
            cpu_data->cpu.lstickX = 0;
            
    } else if (IsAirActionable(cpu)) {
        // JUMP
        if (
            OptEnabled(OPT_FALCON_JUMP)
            && can_jump
            && (
                // force jump if at end of range
                vec_to_ledgegrab.Y > CF_UPB_HEIGHT
                
                // otherwise, random chance to jump
                || HSD_Randi(dj_chance) == 0
            )
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_Y;
            float jump_dir;
            if (Within(vec_to_ledge.Y, CF_JUMP_HEIGHT - 5.f, 10.f) && vec_to_ledge.X * dir < 50.f)
                jump_dir = sign(vec_to_ledgegrab.X);
            else
                jump_dir = HSD_Randf() * 2.f - 1.f;
            cpu_data->cpu.lstickX = 127 * jump_dir;
            
        // UPB
        } else if (
            OptEnabled(OPT_FALCON_UPB) 
            && (
                // random change to upb
                (
                    vel.Y <= 1.f
                    && !past_ledge
                    && vec_to_ledgegrab.Y < CF_UPB_HEIGHT
                    && HSD_Randi(upb_chance) == 0
                )
                // force upb if at end of range
                || (
                    vel.Y <= 0.f
                    && !can_jump
                    && vec_to_ledgegrab.Y > CF_UPB_HEIGHT
                )
            )
        ) {
            cpu_data->cpu.lstickY = 127;
            cpu_data->cpu.held |= PAD_BUTTON_B;
            
        // FALCON KICK
        } else if (
            OptEnabled(OPT_FALCON_DOWNB)
            && pos.Y > 60.f
            && vel.Y < 0.f
            && fabs(vec_to_ledge.X) > 50.f
            && cpu_data->jump.jumps_used == 2  
            && HSD_Randi(CF_DOWNB_CHANCE) == 0
        ) {
            cpu_data->cpu.lstickY = -127;
            cpu_data->cpu.held |= PAD_BUTTON_B;
            
        // DRIFT BACK
        } else if (
            OptEnabled(OPT_FALCON_DRIFT_BACK)
            && in_drift_back_zone
            && vec_to_ledgegrab.Y < 0.f
            && HSD_Randi(CF_FALL_DRIFT_CHANGE_CHANCE) == 0
        ) {
            drift_back_timer = HSD_Randi(CF_DRIFT_DURATION_LENGTH_RNG) + CF_DRIFT_DURATION_LENGTH_CONST;
            // frame with no drift - this allows faster drift change
            cpu_data->cpu.lstickX = 0;
        } else {
            cpu_data->cpu.lstickX = 127 * dir;
        }
        
    // UPB DRIFT
    } else if (state == 0x162) {
        if (
            OptEnabled(OPT_FALCON_DRIFT_BACK)
            && vec_to_ledgegrab.Y < 0.f
            && cpu_data->TM.state_frame > 14
            && in_drift_back_zone
            && HSD_Randi(CF_UPB_DRIFT_CHANGE_CHANCE) == 0
        ) {
            drift_back_timer = HSD_Randi(20) + 5;
            // frame with no drift - this allows faster drift change
            cpu_data->cpu.lstickX = 0;
        } else {
            cpu_data->cpu.lstickX = 127 * dir;
        }
    } else if (ASID_FALLSPECIAL <= state && state <= ASID_FALLSPECIALB) {
        // FASTFALL
        if (
            OptEnabled(OPT_FALCON_FASTFALL)
            && !cpu_data->flags.is_fastfall
            && vel.Y < 0.f
            && (past_ledge ||
                -vec_to_ledgegrab.Y / 3.f > fabs(vec_to_ledgegrab.X)
            )
            && HSD_Randi(CF_FASTFALL_CHANCE) == 0
        ) {
            cpu_data->cpu.lstickY = -127;
        } else {
            cpu_data->cpu.lstickX = 127 * dir;
        }
    }
}

// Marth ------------------------------------------------

#define MT_SWEETSPOT_OFFSET_X 12
#define MT_SWEETSPOT_OFFSET_Y 8
#define MT_JUMP_HEIGHT 36
#define MT_JUMP_DISTANCE 30
#define MT_UPB_STRAIGHT_HEIGHT 60
#define MT_UPB_STRAIGHT_DISTANCE 10
#define MT_UPB_CURLED_HEIGHT 55
#define MT_UPB_CURLED_DISTANCE 25
#define MT_FAIR_SIZE 25

#define MT_JUMP_CHANCE 5
#define MT_UPB_EARLY_CHANCE 30
#define MT_SIDEB_DELAY_CHANCE 10
#define MT_FAIR_CHANCE 3

static void Think_Marth(GOBJ *cpu, u32 opt_flags) {
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;
    
    Vec2 pos = { cpu_data->phys.pos.X, cpu_data->phys.pos.Y };
    Vec2 vel = { cpu_data->phys.self_vel.X, cpu_data->phys.self_vel.Y };
    float dir = pos.X > 0.f ? -1.f : 1.f;
    int state = cpu_data->state_id;
    
    Vec2 target_ledge = ledge_positions[pos.X > 0.f];
    Vec2 target_ledgegrab = {
        .X = target_ledge.X - MT_SWEETSPOT_OFFSET_X*dir,
        .Y = target_ledge.Y - MT_SWEETSPOT_OFFSET_Y,
    };
    
    Vec2 vec_to_ledgegrab = {
        .X = target_ledgegrab.X - pos.X,
        .Y = target_ledgegrab.Y - pos.Y,
    };
    
    bool past_ledgegrab = fabs(pos.X) < fabs(target_ledgegrab.X);
    bool can_jump = cpu_data->jump.jumps_used < 2;
    bool can_upb = vec_to_ledgegrab.Y < MT_UPB_STRAIGHT_HEIGHT
        && vec_to_ledgegrab.X * dir < MT_UPB_CURLED_DISTANCE
        && (stc_stage->kind != GRKIND_BATTLE || vec_to_ledgegrab.X * dir > 3.f);

    if (cpu_data->flags.hitlag) {
        // DI inwards
        cpu_data->cpu.lstickX = 90 * dir;
        cpu_data->cpu.lstickY = 90;
    } else if (IsAirActionable(cpu)) {
        // SIDE B FAR
        if (
            OptEnabled(OPT_MARTH_SIDEB)
            && fabs(vec_to_ledgegrab.X) > 60.f
            && !past_ledgegrab
            && vel.Y < -0.5f
            && fabs(vel.X) > 0.8f
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_B;
            cpu_data->cpu.lstickX = 127 * dir;
            
        // SIDE B WAIT
        } else if (
            OptEnabled(OPT_MARTH_SIDEB)
            && vel.Y < 0.0f
            && vec_to_ledgegrab.Y > 10.f 
            && vec_to_ledgegrab.Y < 35.f 
            && (
                stc_stage->kind != GRKIND_BATTLE ||
                fabs(SimulatePhys(cpu_data, 25).X) > fabs(target_ledgegrab.X)
            )
            && HSD_Randi(MT_SIDEB_DELAY_CHANCE) == 0
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_B;
            cpu_data->cpu.lstickX = 127 * dir;
            
        // JUMP
        } else if (
            // random chance
            (
                OptEnabled(OPT_MARTH_JUMP)
                && MT_JUMP_HEIGHT - 5.f < vec_to_ledgegrab.Y
                && vec_to_ledgegrab.Y < MT_JUMP_HEIGHT
                && can_jump
                && fabs(vec_to_ledgegrab.X) < MT_JUMP_DISTANCE
                && HSD_Randi(MT_JUMP_CHANCE) == 0
            )
            
            // force jump if can't upb
            || (
                OptEnabled(OPT_MARTH_JUMP)
                && vec_to_ledgegrab.Y > MT_UPB_STRAIGHT_HEIGHT
                && can_jump
            )
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_Y;
            if (past_ledgegrab)
                cpu_data->cpu.lstickX = -127 * dir;
            else if (vec_to_ledgegrab.X * dir < MT_UPB_CURLED_DISTANCE)
                cpu_data->cpu.lstickX = 0;
            else
                cpu_data->cpu.lstickX = 127 * dir;
        
        // FAIR
        } else if (
            OptEnabled(OPT_MARTH_FAIR)
            && !cpu_data->flags.is_fastfall
            && pos.X * dir < hmn_data->phys.pos.X * dir
            && hmn_data->hurt.intang_frames.ledge < 4
            && ProjectedDistance(hmn_data, cpu_data, 5) < MT_FAIR_SIZE
            && target_ledge.Y - SimulatePhys(cpu_data, 30).Y < MT_UPB_STRAIGHT_HEIGHT
            && HSD_Randi(MT_FAIR_CHANCE) == 0
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_A;
            cpu_data->cpu.lstickX = 127 * dir;
            
        // UP B
        } else if (
            // random chance
            (
                OptEnabled(OPT_MARTH_UPB_EARLY)
                && vec_to_ledgegrab.Y > 35.f
                && vec_to_ledgegrab.Y < MT_UPB_STRAIGHT_HEIGHT
                && can_upb
                && !can_jump
                && vel.Y < 1.f
                && HSD_Randi(MT_UPB_EARLY_CHANCE) == 0
            )
        
            // force upb
            || (
                (
                    vec_to_ledgegrab.Y > MT_UPB_STRAIGHT_HEIGHT
                    || (
                        vec_to_ledgegrab.X * dir < MT_UPB_CURLED_DISTANCE
                        && vec_to_ledgegrab.Y > MT_UPB_CURLED_HEIGHT
                    )
                )
                && (!can_jump || !OptEnabled(OPT_MARTH_JUMP))
                && vel.Y < 0.f
            )
        ) {
            cpu_data->cpu.held |= PAD_BUTTON_B;
            cpu_data->cpu.lstickY = 127;
            
        // WAIT
        } else {
            if (pos.X * dir + 10.f < target_ledgegrab.X * dir)
                cpu_data->cpu.lstickX = 127 * dir;
            else
                cpu_data->cpu.lstickX = -127 * dir;
        }
    
    // DOLPHIN SLASH
    } else if (state == 0x170) {
        // choose curl
        if (cpu_data->TM.state_frame == 3) {
            float curl; // -1 = full curl backwards, 1 = full curl forwards
            
            if (stc_stage->kind == GRKIND_BATTLE && vec_to_ledgegrab.X * dir < 5.f) {
                curl = -1.f;
            } else if (vec_to_ledgegrab.X * dir < MT_UPB_CURLED_DISTANCE && vec_to_ledgegrab.Y < MT_UPB_CURLED_HEIGHT) {
                curl = HSD_Randf() * 0.3f + 0.7f;
            } else if (vec_to_ledgegrab.X * dir > MT_UPB_CURLED_DISTANCE) {
                curl = 1.f;
            } else if (vec_to_ledgegrab.Y > MT_UPB_STRAIGHT_HEIGHT) {
                curl = -1.f;
            } else {
                curl = Progress(vec_to_ledgegrab.X * dir, MT_UPB_STRAIGHT_DISTANCE, MT_UPB_CURLED_DISTANCE);
                curl = fclamp(curl, -1.f, 1.f);
                
                // similar function to sqrt but easier to compute
                curl = (-(curl*curl) + 2.f * fabs(curl)) * sign(curl);
            }
            
            // curl starts from the opposite diagonal notch
            if (curl < 0.f)
                curl /= 2.f;
            
            float ang = curl * M_PI / 2.f;
            cpu_data->cpu.lstickX = (127.f * sin(ang)) * dir;
            cpu_data->cpu.lstickY = 127.f * cos(ang);
        }
        
        else if (cpu_data->TM.state_frame == 5) {
            cpu_data->cpu.lstickX = 127.f * dir;
        }

    } else if (ASID_ATTACKAIRN <= state && state <= ASID_ATTACKAIRLW) {
        cpu_data->cpu.lstickX = 127 * sign(vec_to_ledgegrab.X);
        
    } else if (ASID_FALLSPECIAL <= state && state <= ASID_FALLSPECIALB) {
        cpu_data->cpu.lstickX = 127 * sign(vec_to_ledgegrab.X);
    }
}


