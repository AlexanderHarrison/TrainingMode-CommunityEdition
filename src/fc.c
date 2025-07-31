#include "../MexTK/mex.h"
#include "events.h"

static struct Assets {
    JOBJDesc *hud;
} *assets;
static JOBJ *hud_jobj;

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

static u32 action_log_cur;
static u8 action_log[30];

static GXColor action_colours[Action_Count] = {
    {40, 40, 40, 180},  // dark gray
    {255, 128, 128, 180}, // red
    {230, 22, 198, 180}, // magenta
    {52, 202, 228, 180}, // cyan
    {128, 128, 255, 180}, // blue
    {128, 255, 128, 255}, // green
};

void HitboxGX(GOBJ *gobj, int pass) {
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
        
        PRIM *gx = PRIM_NEW(4, 0xffffffff, 0xffffffff);
    }
}

void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

/*void Square(void) {
    // init hud object
    GOBJ *g = GObj_Create(0, 0, 0);
    JOBJ *j = JOBJ_LoadJoint(assets->hud);
    JOBJ *b;
    JOBJ_GetChild(j, &b, 4, -1);
    b->trans.Y += 1.f;
    GObj_AddObject(g, 3, b);
    GObj_AddGXLink(g, GXLink_Common, 18, 80);
}*/

void Event_Init(GOBJ *menu) {
    // re-use the ledgedash hud for now
    assets = Archive_GetPublicAddress(event_vars->event_archive, "ledgedash");
    
    // create hud cobj
    GOBJ *hudcam_gobj = GObj_Create(19, 20, 0);
    COBJDesc ***dmgScnMdls = Archive_GetPublicAddress(*stc_ifall_archive, (void *)0x803f94d0);
    COBJDesc *cam_desc = dmgScnMdls[1][0];
    COBJ *hud_cobj = COBJ_LoadDesc(cam_desc);
    
    // init hud camera
    GObj_AddObject(hudcam_gobj, R13_U8(-0x3E55), hud_cobj);
    GOBJ_InitCamera(hudcam_gobj, HUDCamThink, 7);
    hudcam_gobj->cobj_links = 1 << 18;

    // init hud object
    GOBJ *hud_gobj = GObj_Create(0, 0, 0);
    hud_jobj = JOBJ_LoadJoint(assets->hud);
    hud_jobj->child->flags |= JOBJ_HIDDEN;
    GObj_AddObject(hud_gobj, 3, hud_jobj);
    GObj_AddGXLink(hud_gobj, GXLink_Common, 18, 80);
    
    // init hitbox display gobj
    GOBJ *hitbox_gobj = GObj_Create(0, 0, 0);
    GObj_AddGXLink(hitbox_gobj, HitboxGX, 5, 0);
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
        
    // colour action log
    JOBJ *timingbar_jobj;
    JOBJ_GetChild(hud_jobj, &timingbar_jobj, 4, -1);
    DOBJ *d = timingbar_jobj->dobj;
    
    OSReport("pobj %p\n", d->pobj);
    
    // iter backwards... dobjs are stored right to left for some reason
    for (u32 i = countof(action_log); i > 0; --i) {
        int action = action_log[i-1];
        d->mobj->mat->diffuse = action_colours[action];
        d = d->next;
    }
}

EventMenu *Event_Menu = &Menu_Main;
