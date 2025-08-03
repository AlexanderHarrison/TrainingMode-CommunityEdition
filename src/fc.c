#include "../MexTK/mex.h"
#include "events.h"

void Exit(GOBJ *menu);

static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = "Return to the Event Select Screen.",
        .OnSelect = Exit,
    },
};

static EventMenu Menu_Main = {
    .name = "Float Cancel Training",
    .option_num = sizeof(Options_Main) / sizeof(EventOption),
    .options = Options_Main,
};

void HUDCamThink(GOBJ *gobj) {
    // if HUD enabled and not paused
    if (Pause_CheckStatus(1) != 2)
        CObjThink_Common(gobj);
}

enum PeachActionStates {
    ASID_FLOAT = 341,
    ASID_FLOATENDF,
    ASID_FLOATENDB,
    ASID_FLOATATTACKN,
    ASID_FLOATATTACKF,
    ASID_FLOATATTACKB,
    ASID_FLOATATTACKU,
    ASID_FLOATATTACKD,
};

enum Action {
    Action_Wait,
    Action_Float,
    Action_FloatNeutral,
    Action_FloatAttack,
    Action_FloatAttackFall,
    Action_FloatAttackFastFall,
    
    Action_Count
};
static char *action_names[] = {
    "Float",
    "Deadzone",
    "Attack",
    "Fall",
    "Fastfall",
};

static u32 action_log_cur;
static u8 action_log[30];

static GXColor action_colors[Action_Count] = {
    {40, 40, 40, 180},  // dark gray
    {255, 128, 128, 180}, // red
    {230, 22, 198, 180}, // magenta
    {52, 202, 228, 180}, // cyan
    {128, 128, 255, 180}, // blue
    {128, 255, 128, 255}, // green
};

void GX(GOBJ *gobj, int pass) {
    // hitbox GX
    if (pass == 1) {
        GOBJ *ft = Fighter_GetGObj(0);
        FighterData *ft_data = ft->userdata;
        
        static GXColor diffuse = {255, 0, 0, 100};
        static GXColor ambient = {255, 255, 255, 255};
        for (u32 i = 0; i < countof(ft_data->hitbox); ++i) {
            ftHit *hit = &ft_data->hitbox[i];
            if (hit->active)
                Develop_DrawSphere(hit->size, &hit->pos, &hit->pos_prev, &diffuse, &ambient);
        }
    }
    
    // action log gx
    if (pass == 2) {
        event_vars->HUD_DrawActionLogBar(
            action_log,
            action_colors,
            countof(action_log)
        );
        event_vars->HUD_DrawActionLogKey(
            action_names,
            &action_colors[1],
            countof(action_names)
        );
    }
}

void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

void Event_Init(GOBJ *menu) {
    // init display gobj
    GOBJ *gobj = GObj_Create(0, 0, 0);
    GObj_AddGXLink(gobj, GX, 5, 0);
}

void Event_Think(GOBJ *menu) {
    GOBJ *ft = Fighter_GetGObj(0);
    FighterData *ft_data = ft->userdata;
    int state = ft_data->state_id;
    int is_fastfall = ft_data->flags.is_fastfall;
    
    // determine current action
    int cur_action;
    bool reset = false; 
    if ((state == ASID_JUMPF || state == ASID_JUMPB) && ft_data->TM.state_frame == 1) {
        reset = true;
        cur_action = Action_Wait;
    } else if (state == ASID_FLOAT)
        if (ft_data->input.lstick.X == 0.f && ft_data->input.lstick.Y == 0.f)
            cur_action = Action_FloatNeutral;
        else
            cur_action = Action_Float;
    else if (state >= ASID_FLOATATTACKN && state <= ASID_FLOATATTACKD)
        if (ft_data->phys.self_vel.Y == 0.f)
            cur_action = Action_FloatAttack;
        else
            cur_action = is_fastfall ? Action_FloatAttackFastFall : Action_FloatAttackFall;
    else
        cur_action = Action_Wait;
        
    if (reset) {
        memset(action_log, Action_Wait, sizeof(action_log));
        action_log_cur = 0;
    } else if (action_log_cur < countof(action_log)) {
        action_log[action_log_cur++] = cur_action;
    }
}

EventMenu *Event_Menu = &Menu_Main;
