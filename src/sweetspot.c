#include "events.h"

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

static void UpdatePosition(GOBJ *fighter) {
    FighterData *data = fighter->userdata;

    Vec3 pos = data->phys.pos;
    data->coll_data.topN_Curr = pos;
    data->coll_data.topN_CurrCorrect = pos;
    data->coll_data.topN_Prev = pos;
    data->coll_data.topN_Proj = pos;
    data->coll_data.coll_test = *stc_colltest;

    JOBJ *jobj = fighter->hsd_object;
    jobj->trans = data->phys.pos;
    JOBJ_SetMtxDirtySub(jobj);

    // Update static hmn block coords
    Fighter_SetPosition(data->ply, data->flags.ms, &data->phys.pos);
}

static bool FindGroundNearPlayer(GOBJ *fighter, Vec3 *pos, int *line_idx) {
    FighterData *data = fighter->userdata;
    float x = data->phys.pos.X;
    float y1 = data->phys.pos.Y + 10;
    float y2 = data->phys.pos.Y - 100;

    Vec3 _line_unk;
    int _line_kind;

    return GrColl_RaycastGround(pos, line_idx, &_line_kind, &_line_unk,
            -1, -1, -1, 0, x, y1, x, y2, 0);
}

static void PlacePlayerOnGround(GOBJ *fighter) {
    FighterData *data = fighter->userdata;

    Vec3 pos;
    int line_idx;
    if (FindGroundNearPlayer(fighter, &pos, &line_idx)) {
        data->phys.pos.X = pos.X;
        data->phys.pos.Y = pos.Y;
        data->coll_data.ground_index = line_idx;
    }
    UpdatePosition(fighter);
    EnvironmentCollision_WaitLanding(fighter);
    Fighter_SetGrounded(data);
}

static void UpdateCameraBox(GOBJ *fighter) {
    Fighter_UpdateCameraBox(fighter);

    FighterData *data = fighter->userdata;
    CmSubject *subject = data->camera_subject;
    subject->boundleft_curr = subject->boundleft_proj;
    subject->boundright_curr = subject->boundright_proj;

    Match_CorrectCamera();
}

void Exit(GOBJ *menu);
void Reset(int side_idx);

enum menu_options {
    OPT_LINES,
};

enum ledge_direction {
    DIRECTION_RANDOM,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
};

static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Enable Line Guides",
        .desc = {"Draws horizontal lines at the ledges and at your",
                 "Calculated ledgegrab position"},
    },
    {
        .kind = OPTKIND_INFO,
        .name = "Help",
        .desc = {"D-Pad left/right to reset manually to that side"}
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Select Screen."},
        .OnSelect = Exit,
    },
};

static EventMenu Menu_Main = {
    .name = "Sweetspot Training",
    .option_num = sizeof(Options_Main) / sizeof(EventOption),
    .options = Options_Main,
};

enum event_state {
    STATE_ENDLAG,
    STATE_WAIT_ATTACK,
    STATE_ATTACK_THINK,
    STATE_RESET,
}; 

static struct sideb {
    bool attempted;
    Vec3 pos;
    int dir;
} sideb = { 0 };
static int reset_timer = 0;
static int state = STATE_ENDLAG;

void Draw_Lines() {
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    CollData *coll_data = &hmn_data->coll_data;

    if (!Options_Main[OPT_LINES].val)
        return;

    COBJ *cur_cam = COBJ_GetCurrent();
    CObj_SetCurrent(*stc_matchcam_cobj);

    PRIM_DrawMode draw_mode = {
        .line_width = 16,
        .z_compare_enable = true,
        .z_logic_eq = true,
        .z_logic_lt = true,
        .shape = PRIM_SHAPE_LINE_STRIP,
    };
    PRIM_BlendMode blend_mode = { 0 };

    // Melee calculation for the top of the ledgegrab box
    float ledgegrab_offset = coll_data->cliffgrab_y_offset + 0.5 * coll_data->cliffgrab_height;
    if (hmn_data->kind == FTKIND_FOX) {
        // Only Fox drops a small amount during sideb before moving
        ledgegrab_offset -= 1.0 / 6.0;
    }

    float y = hmn_data->phys.pos.Y + ledgegrab_offset; 
    float x = hmn_data->phys.pos.X; 
    u32 color = 0x0000ffff;
    if (sideb.attempted) {
        x = sideb.pos.X;
        y = sideb.pos.Y + ledgegrab_offset;

        Vec2 target_ledge;
        if (sideb.dir == 1)
            target_ledge = ledge_positions[0];
        else
            target_ledge = ledge_positions[1];

        float delta = y - target_ledge.Y;

        if (delta <= 0) { // failure (SD)
            color = 0xff0000ff;
        } else if (hmn_data->state_id == ASID_CLIFFCATCH
            || hmn_data->state_id == ASID_CLIFFWAIT) { // success
            color = 0x00ff00ff;
        }
    }

    // draw line at top of ledgegrab box
    PRIM_NEW(2, draw_mode, blend_mode);
    PRIM_DRAW(-300, y, 0, color);
    PRIM_DRAW(300, y, 0, color);

    // draw line from left edge
    color = 0xffffffff;
    x = ledge_positions[0].X;
    y = ledge_positions[0].Y;
    PRIM_NEW(2, draw_mode, blend_mode);
    PRIM_DRAW(x, y, 0, color);
    PRIM_DRAW(x - 100, y, 0, color);

    // draw line from right edge
    x = ledge_positions[1].X;
    y = ledge_positions[1].Y;
    PRIM_NEW(2, draw_mode, blend_mode);
    PRIM_DRAW(x, y, 0, color);
    PRIM_DRAW(x + 100, y, 0, color);

    PRIM_CLOSE();
    CObj_SetCurrent(cur_cam);
}

void Event_Init(GOBJ* gobj) {
    GetLedgePositions(ledge_positions);
    GOBJ *draw_gobj = GObj_Create(0, 0, 0);
    GObj_AddGXLink(draw_gobj, Draw_Lines, 3, 0);
}

void Event_Think(GOBJ *menu) {
    if (event_vars->game_timer == 1) {
        event_vars->Savestate_Save_v1(event_vars->savestate, Savestate_Silent);
        Reset(0);
    }

    // Reset timer is 0 when off, otherwise counts down until reset
    if (reset_timer == 1) {
        reset_timer = 0;
        Reset(2);
    }
    if (reset_timer > 1)
        reset_timer--;
    
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *cpu_data = cpu->userdata;

    // Reset left/right when user presses dpad manually
    if (hmn_data->input.down & HSD_BUTTON_DPAD_LEFT) {
        Reset(0);
    } else if (hmn_data->input.down & HSD_BUTTON_DPAD_RIGHT) {
        Reset(1);
    }

    // Reset when dead
    if (hmn_data->flags.dead)
        Reset(2);

    if (hmn_data->state_id == 350) { // Aerial sideb startup for fox/falco
        sideb.attempted = true;
        sideb.pos = hmn_data->phys.pos;
        sideb.dir = hmn_data->facing_direction;
    }

    // Begin CPU AI logic
    Fighter_ZeroCPUInputs(cpu_data);

    // always hold crouch when state is higher than endlag
    if (state > STATE_ENDLAG)
        cpu_data->cpu.lstickY = -127;

    switch (state) {
        case STATE_ENDLAG:
            if (cpu_data->state_id == ASID_WAIT) {
                cpu_data->cpu.lstickY = -127;
                state = STATE_WAIT_ATTACK;
            }
            break;
        case STATE_WAIT_ATTACK:
            if (hmn_data->state_id == 0x15F) { // side b asid
                // input dtilt
                cpu_data->cpu.lstickY = -38;
                cpu_data->cpu.held = PAD_BUTTON_A;
                state = STATE_ATTACK_THINK;
            } else if (hmn_data->state_id == 0x162 // up b asid
                    || hmn_data->state_id == ASID_ESCAPEAIR) {
                SFX_Play(0xAF);
                reset_timer = 30;
                state = STATE_RESET;
            }
            break;
        case STATE_ATTACK_THINK:
            if (cpu_data->state_id != ASID_ATTACKLW3) break;
            if (cpu_data->state.frame == 7) {
                FrameSpeedChange(cpu, 0);       // freeze cpu
                if (hmn_data->victim) {         // if hit
                    FrameSpeedChange(cpu, 1.0); // unfreeze
                    reset_timer = 30;
                    state = STATE_RESET;
                }
            }
            if (hmn_data->state_id == ASID_CLIFFCATCH) {
                SFX_Play(0xAD);
                FrameSpeedChange(cpu, 1.0);
                reset_timer = 60;
                state = STATE_RESET;
            }
            break;
        case STATE_RESET:
            break;
    }

}

void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

void Reset(int side_idx) {
    if (side_idx == 2)
        side_idx = HSD_Randi(2);
    if (side_idx != 0 && side_idx != 1)
        assert("invalid side");

    event_vars->Savestate_Load_v1(event_vars->savestate, Savestate_Silent);
    sideb.attempted = false;

    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *cpu_data = cpu->userdata;

    int side = side_idx * 2 - 1;
    hmn_data->facing_direction = -side;
    cpu_data->facing_direction = side;

    hmn_data->phys.pos.X = side * 12 + ledge_positions[side_idx].X;
    hmn_data->phys.pos.Y = 6;
    cpu_data->phys.pos.X = side * -15 + ledge_positions[side_idx].X;
    cpu_data->phys.pos.Y = 0;

    UpdatePosition(hmn);
    UpdatePosition(cpu);
    PlacePlayerOnGround(cpu);

    // cpu state
    cpu_data->gfx = 0;
    FSmash_GetASForAnalogAngle(cpu);
    Fighter_ApplyAnimation(cpu, 11, 1, 0);
    cpu_data->state.frame = 11;
    cpu_data->script.script_event_timer = 0;
    Fighter_SubactionFastForward(cpu);
    Fighter_UpdateStateFrameInfo(cpu);
    Fighter_HitboxDisableAll(cpu);
    cpu_data->script.script_current = 0;
    UpdateCameraBox(cpu);

    // hmn state
    ActionStateChange(0, 1, 0, hmn, ASID_DAMAGEFLYN, 0x40, 0);
    Fighter_UpdateStateFrameInfo(hmn);
    UpdateCameraBox(hmn);
    float dmg = 50 * HSD_Randf() + 50;
    hmn_data->dmg.percent = dmg;
    Fighter_SetHUDDamage(hmn_data->ply, dmg);
    
    // hmn knockback
    float angle = (15 * HSD_Randf() + 45) * M_1DEGREE;
    float mag = 30 * HSD_Randf() + 105;
    float vel = mag * (*stc_ftcommon)->force_applied_to_kb_mag_multiplier;
    hmn_data->phys.kb_vel.X = cos(angle) * vel * side;
    hmn_data->phys.kb_vel.Y = sin(angle) * vel;
    float kb_frames = (float)(int)((*stc_ftcommon)->x154 * mag);

    union IntToFloat {
        int i;
        float f;
    } itf;
    itf.f = kb_frames;
    hmn_data->state_var.state_var1 = itf.i;

    hmn_data->flags.hitstun = 1;
    Fighter_EnableCollUpdate(hmn_data);

    // give both hitlag
    hmn_data->dmg.hitlag_frames = 10;
    hmn_data->flags.hitlag = 1;
    hmn_data->flags.hitlag_unk = 1;
    cpu_data->dmg.hitlag_frames = 10;
    cpu_data->flags.hitlag = 1;
    cpu_data->flags.hitlag_unk = 1;

    reset_timer = 0;
    state = STATE_ENDLAG;
}

EventMenu *Event_Menu = &Menu_Main;
