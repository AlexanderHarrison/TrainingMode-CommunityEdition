#include "../MexTK/mex.h"
#include "events.h"

static void Exit(GOBJ *menu);
static void Reset(void);
static void PutOnGround(GOBJ *ft);

enum {
    Leniency_Small,
    Leniency_Large,
    Leniency_None,

    Leniency_Count
};

static u8 dash_random_delay_max[Leniency_Count] = { 2, 3, 1 };
static u8 knee_random_delay_max[Leniency_Count] = { 3, 4, 1 };
static const char *Options_Leniency[] = { "Small", "Large", "None" };

enum {
    Opt_Leniency,
    Opt_Help,
    Opt_Exit,
};

static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_STRING,
        .name = "Leniency",
        .desc = {"How much falcon will delay the knee,",
                 "giving you more time to escape."},
        .values = Options_Leniency,
        .value_num = countof(Options_Leniency),
    },
    {
        .kind = OPTKIND_INFO,
        .name = "Help",
        .desc = {
            "To escape Falcon's down throw into knee kill confirm,",
            "Tap up to buffer a jump right before hitstun ends.",
            "Then, press down both triggers with a slight delay to",
            "airdodge up, dodging the knee."
        },
    },
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

enum Action {
    Action_None,
    Action_Hitstun,
    Action_Actionable,
    Action_DoubleJump,
    Action_Airdodge,

    Action_Count
};
static char *action_names[Action_Count] = {
    "None",
    "Hitstun",
    "Actionable",
    "Double Jump",
    "Airdodge",
};
static GXColor action_colors[Action_Count] = {
    {40, 40, 40, 180},  // dark gray - none
    {120, 80, 80, 180},  // dark red - hitstun
    {230, 22, 198, 180}, // magenta - actionable
    {128, 128, 255, 180}, // blue - double jump
    {128, 255, 128, 180}, // green - airdodge
};

static u8 action_log[20];
static u8 lr_press_log[20];
static u8 jump_press_log[20];
static u32 action_log_cur = countof(action_log); // start with log disabled

static void Event_PostThink(GOBJ *menu) {
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // Update Action Log ---------------------------------------------------------------
    
    ftCommonData *ftcommon = *stc_ftcommon;

    if (hmn_data->flags.hitstun) {
        float hitstun;
        memcpy(&hitstun, &hmn_data->state_var.state_var1, sizeof(float));
        if (hitstun == 10.f) {
            action_log_cur = 0;
            memset(action_log, 0, sizeof(action_log));
            memset(lr_press_log, 0, sizeof(lr_press_log));
            memset(jump_press_log, 0, sizeof(jump_press_log));
        }
    }

    if (action_log_cur < countof(action_log)) {
        int hmn_state = hmn_data->state_id;

        u8 action = Action_None;

        if (hmn_data->flags.hitstun)
            action = Action_Hitstun;

        else if (
            (ASID_DAMAGEHI1 <= hmn_state && hmn_state <= ASID_DAMAGEFLYROLL) 
            || hmn_state == ASID_DAMAGEFALL 
            || hmn_state == ASID_FALL
        )
            action = Action_Actionable;

        else if (hmn_state == ASID_JUMPAERIALF || hmn_state == ASID_JUMPAERIALB)
            action = Action_DoubleJump;

        else if (hmn_state == ASID_ESCAPEAIR)
            action = Action_Airdodge;
        
        action_log[action_log_cur] = action;

        // We need to use the master pad here: hmn_data->input.down is maintained during hitlag
        HSD_Pad *pad = PadGetMaster(hmn_data->pad_index);
        if (pad->down & PAD_TRIGGER_L) lr_press_log[action_log_cur]++;
        if (pad->down & PAD_TRIGGER_R) lr_press_log[action_log_cur]++;
        if (pad->down & PAD_BUTTON_X) jump_press_log[action_log_cur]++;
        if (pad->down & PAD_BUTTON_Y) jump_press_log[action_log_cur]++;
        
        if (
            // If in tap jump range
            hmn_data->input.lstick.Y >= ftcommon->jumpaerial_lsticky
            && hmn_data->input.lstick_prev.Y < ftcommon->jumpaerial_lsticky
        ) {
            // If this jump was buffered, it's counted from the first tilt frame.
            int tilt_y = hmn_data->input.timer_lstick_tilt_y;
            if (
                // ensure first tilt input was during hitstun and in the log
                tilt_y >= 0
                && tilt_y <= (int)action_log_cur
                && action_log_cur - tilt_y < countof(action_log)/2
            )
                jump_press_log[action_log_cur - tilt_y]++;
            else
                jump_press_log[action_log_cur]++;
        }

        action_log_cur++;
    }
}

static void Event_GX(GOBJ *menu, int pass) {
    if (pass == 2) {
        Rect rect_bar = event_vars->HUD_DrawActionLogBar(
            action_log,
            action_colors,
            countof(action_log)
        );

        // skip none state
        event_vars->HUD_DrawActionLogKey(
            &action_names[1],
            &action_colors[1],
            countof(action_names) - 1
        );
        
        // draw indicator shapes
        u32 since_jump = 10000;
        for (u32 i = 0; i < countof(lr_press_log); ++i) {
            u8 lr_press_count = lr_press_log[i];
            u8 jump_press_count = jump_press_log[i];
            
            float x = rect_bar.x + ((float)i + 0.5f) * 0.05f * rect_bar.w;
            float y = rect_bar.y - 1.f;
            
            static const float s = 0.4f;
            static const float t = 0.8f;
            static GXColor red = {255, 150, 150, 255};
            static GXColor green = {150, 255, 200, 255};

            u32 mid = countof(lr_press_log)/2;

            for (; jump_press_count; jump_press_count--) {
                Tri tri = { { x, y }, { x - s, y - t }, { x + s, y - t } };
                y -= 1.f;

                GXColor *color = (mid-3 <= i && i <= mid+3) ? &green : &red;
                event_vars->HUD_DrawTris(&tri, color, 1);
                since_jump = 0;
            }

            for (; lr_press_count; lr_press_count--) {
                Rect rect = { x - s, y - t, t, t };
                y -= 1.f;
                GXColor *color = (1 <= since_jump && since_jump <= 5 && mid < i) ? &green : &red;
                event_vars->HUD_DrawRects(&rect, color, 1);
            }

            since_jump++;
        }
    }
}

void Event_Init(GOBJ *menu) {
    GOBJ *gobj = GObj_Create(0, 0, 0);
    GObj_AddProc(gobj, Event_PostThink, 20);
    GObj_AddGXLink(gobj, Event_GX, 5, 0);
}

static int start_timer = -1;
static int reset_timer = -1;

void Event_Think(GOBJ *menu) {
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *cpu_data = cpu->userdata;

    int hmn_state = hmn_data->state_id;
    int cpu_state = cpu_data->state_id;

    if (event_vars->game_timer == 1) {
        PutOnGround(hmn);
        PutOnGround(cpu);
        Match_CorrectCamera();
        event_vars->Savestate_Save_v1(event_vars->savestate, Savestate_Silent);
        Reset();
    }
    
    // Reset Logic ---------------------------------------------------------------------

    HSD_Pad *pad = PadGetMaster(hmn_data->pad_index);
    if (pad->down & HSD_BUTTON_DPAD_LEFT)
        Reset();

    if (reset_timer < 0 && (cpu_state == ASID_LANDING || cpu_state == ASID_LANDINGAIRF))
        reset_timer = 20;
    if (reset_timer == 0)
        Reset();
    if (reset_timer >= 0)
        reset_timer--;

    // Calculate CPU inputs -------------------------------------------------------------

    Fighter_ZeroCPUInputs(cpu_data);

    if (start_timer >= 0) {
        start_timer--;
    } else if (ASID_DAMAGEHI1 <= hmn_state && hmn_state <= ASID_DAMAGEFLYROLL) {
        Vec2 knee_pos = SimulateKB(hmn, 20);
        Vec2 to_knee = { knee_pos.X - cpu_data->phys.pos.X, knee_pos.Y - cpu_data->phys.pos.Y }; 

        if (cpu_state == ASID_WAIT) {
            int leniency = dash_random_delay_max[Options_Main[Opt_Leniency].val];
            if (HSD_Randi(leniency) == 0 || cpu_data->TM.state_frame > leniency)
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
            if (to_knee.Y > 52.f)
                cpu_data->cpu.held = PAD_BUTTON_Y;

            if (to_knee.X > 20.f)
                cpu_data->cpu.lstickX = 127;
            if (to_knee.X < 10.f)
                cpu_data->cpu.lstickX = -60;
        }

        if (cpu_state == ASID_JUMPF) {
            cpu_data->cpu.lstickX = 127;

            int leniency = knee_random_delay_max[Options_Main[Opt_Leniency].val];
            if (HSD_Randi(leniency) == 0 || cpu_data->TM.state_frame > leniency)
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

    action_log_cur = countof(action_log);
    start_timer = 20;
}
