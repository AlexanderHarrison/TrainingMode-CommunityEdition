#include "../MexTK/mex.h"
#include "events.h"

static void Exit(GOBJ *menu);
static void Reset(void);
static void PutOnGround(GOBJ *ft);

static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Select Screen."},
        .OnSelect = Exit,
    },
};

static EventMenu Menu_Main = {
    .name = "Escape DThrow Knee",
    .option_num = countof(Options_Main),
    .options = Options_Main,
};

EventMenu *Event_Menu = &Menu_Main;

static Vec2 SimulateKB(GOBJ *ft, u32 frames) {
    FighterData *ft_data = ft->userdata;

    float terminal_velocity = -ft_data->attr.terminal_velocity;
    float decay = (*stc_ftcommon)->kb_frameDecay;

    Vec2 pos = { ft_data->phys.pos.X, ft_data->phys.pos.Y };
    Vec2 kb = { ft_data->phys.kb_vel.X, ft_data->phys.kb_vel.Y };
    float y_vel = 0;

    for (; frames; --frames) {
        if (y_vel < terminal_velocity)
            y_vel = terminal_velocity;

        float angle = atan2(kb.Y, kb.X);
        kb.X = kb.X - cos(angle) * decay;
        kb.Y = kb.Y - sin(angle) * decay;

        pos.X += kb.X;
        pos.Y += kb.Y + y_vel;
    }

    return pos;
}

void Event_Think(GOBJ *menu) {
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *cpu_data = cpu->userdata;
    
    if (hmn_data->input.down & HSD_BUTTON_DPAD_LEFT)
        Reset();

    if (event_vars->game_timer == 1) {
        PutOnGround(hmn);
        PutOnGround(cpu);
        Match_CorrectCamera();
        event_vars->Savestate_Save_v1(event_vars->savestate, Savestate_Silent);
        Reset();
    }

    Fighter_ZeroCPUInputs(cpu_data);
    
    int hmn_state = hmn_data->state_id;
    int cpu_state = cpu_data->state_id;

    if (ASID_DAMAGEHI1 <= hmn_state && hmn_state <= ASID_DAMAGEFLYROLL) {
        Vec2 knee_pos = SimulateKB(hmn, 20);
        Vec2 to_knee = { knee_pos.X - cpu_data->phys.pos.X, knee_pos.Y - cpu_data->phys.pos.Y }; 
        
        if (cpu_state == ASID_WAIT) {
            if (HSD_Randi(2) == 0 || cpu_data->TM.state_frame > 2)
                cpu_data->cpu.lstickX = 127;
        }

        if (cpu_state == ASID_DASH || cpu_state == ASID_RUN) {
            if (to_knee.X > 50.f) {
                cpu_data->cpu.lstickX = 127;
            } else {
                cpu_data->cpu.held = PAD_BUTTON_Y;
                cpu_data->cpu.lstickX = 127;
            }
        }

        if (cpu_state == ASID_KNEEBEND) {
            if (to_knee.Y > 55.f)
                cpu_data->cpu.held = PAD_BUTTON_Y;

            if (to_knee.X > 20.f)
                cpu_data->cpu.lstickX = 127;
            if (to_knee.X < 10.f)
                cpu_data->cpu.lstickX = -60;
        }

        if (cpu_state == ASID_JUMPF) {
            cpu_data->cpu.lstickX = 127;

            if (HSD_Randi(3) == 0 || cpu_data->TM.state_frame > 3)
                cpu_data->cpu.held = PAD_BUTTON_A;
        }

        if (
            cpu_state == ASID_ATTACKAIRF
            && cpu_data->phys.self_vel.Y < 0.f 
            && !cpu_data->flags.is_fastfall
            && cpu_data->phys.pos.Y > hmn_data->phys.pos.Y
        ) {
            cpu_data->cpu.lstickX = 0;
            cpu_data->cpu.lstickY = -127;
        }

        if (cpu_state == ASID_LANDING || cpu_state == ASID_LANDINGAIRF)
            Reset();
    } else {
        if (cpu_state == ASID_WAIT)
            cpu_data->cpu.lstickX = 127;
        
        if (cpu_state == ASID_DASH || cpu_state == ASID_RUN) {
            if (hmn_data->phys.pos.X - cpu_data->phys.pos.X < 20.f)
                cpu_data->cpu.held = PAD_BUTTON_Y;
            else
                cpu_data->cpu.lstickX = 127;
    
            if (hmn_data->phys.pos.X < cpu_data->phys.pos.X)
                Reset();
        }
        
        if (cpu_state == ASID_KNEEBEND)
            cpu_data->cpu.held = PAD_TRIGGER_Z;

        if (cpu_state == ASID_CATCHWAIT)
            cpu_data->cpu.lstickY = -127;
    }
}

static void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

static void PutOnGround(GOBJ *ft) {
    FighterData *ft_data = ft->userdata;
    ft_data->coll_data.ground_index = 1;

    Vec3 pos = { ft_data->phys.pos.X, 0, 0 };
    ft_data->phys.pos = pos;
    ft_data->coll_data.topN_Curr = pos;
    ft_data->coll_data.topN_CurrCorrect = pos;
    ft_data->coll_data.topN_Prev = pos;
    ft_data->coll_data.topN_Proj = pos;
    ft_data->coll_data.coll_test = *stc_colltest;

    JOBJ *jobj = ft->hsd_object;
    jobj->trans = pos;
    JOBJ_SetMtxDirtySub(jobj);
    
    Fighter_SetPosition(ft_data->ply, ft_data->flags.ms, &ft_data->phys.pos);

    EnvironmentCollision_WaitLanding(ft);
    Fighter_SetGrounded(ft_data);
    Fighter_EnterWait(ft);

    Fighter_UpdateCameraBox(ft);
    CmSubject *subject = ft_data->camera_subject;
    subject->boundtop_curr = subject->boundtop_proj;
    subject->boundbottom_curr = subject->boundbottom_proj;
    subject->boundleft_curr = subject->boundleft_proj;
    subject->boundright_curr = subject->boundright_proj;
}

static void Reset(void) {
    event_vars->Savestate_Load_v1(event_vars->savestate, Savestate_Silent);
    
    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;

    PutOnGround(hmn);
    PutOnGround(cpu);
    Match_CorrectCamera();

    int percent = 60 + HSD_Randi(31);
    hmn_data->dmg.percent = percent;
    Fighter_SetHUDDamage(hmn_data->ply, percent);
}
