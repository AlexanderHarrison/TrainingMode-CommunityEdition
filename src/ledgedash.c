#include "ledgedash.h"

static GXColor tmgbar_black = {40, 40, 40, 255};
static GXColor tmgbar_grey = {120, 120, 120, 255};
static GXColor tmgbar_blue = {128, 128, 255, 255};
static GXColor tmgbar_green = {128, 255, 128, 255};
static GXColor tmgbar_cyan = {52, 202, 228, 255};
static GXColor tmgbar_red = {255, 128, 128, 255};
static GXColor tmgbar_magenta = {230, 22, 198, 255};
static GXColor tmgbar_white = {255, 255, 255, 255};
static GXColor *tmgbar_colors[] = {
    &tmgbar_black,
    &tmgbar_grey,
    &tmgbar_green,
    &tmgbar_cyan,
    &tmgbar_magenta,
    &tmgbar_white,
    &tmgbar_red,
    &tmgbar_blue,
};

enum menu_options
{
    OPT_POS,
    OPT_RESET,
    OPT_HUD,
    OPT_TIPS,
    OPT_CAM,
    OPT_INV,
    OPT_SPEED,
    OPT_OVERLAYS,
    OPT_RESETDELAY,
    OPT_ABOUT,
    OPT_EXIT,
};

enum reset_mode
{
    OPTRESET_NONE,
    OPTRESET_SAME_SIDE,
    OPTRESET_SWAP,
    OPTRESET_SWAP_ON_SUCCESS,
    OPTRESET_RANDOM,
};

enum reset_pos
{
    OPTPOS_LEDGE,
    OPTPOS_FALLING,
    OPTPOS_STAGE,
    OPTPOS_RESPAWNPLAT,
};

// Main Menu
static const char *LdshOptions_CamMode[] = {"Normal", "Zoom", "Fixed", "Advanced"};
static const char *LdshOptions_Start[] = {"Ledge", "Falling", "Stage", "Respawn Platform"};
static float LdshOptions_GameSpeeds[] = {1.f, 5.f/6.f, 2.f/3.f, 1.f/2.f, 1.f/4.f};
static const char *LdshOptions_GameSpeedText[] = {"1", "5/6", "2/3", "1/2", "1/4"};
static const char *LdshOptions_Reset[] = {"None", "Same Side", "Swap", "Swap on Success", "Random"};
static const char *LdshOptions_ResetDelay[] = {"Slow", "Normal", "Fast", "Instant"};
static int LdshOptions_ResetDelaySuccess[] = { 120, 60, 30, 1 };
static int LdshOptions_ResetDelayFailure[] = { 60, 20, 1, 1 };

static EventOption LdshOptions_Main[] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LdshOptions_Start) / 4,
        .name = "Starting Position",
        .desc = {"Choose where the fighter is placed ",
                 "after resetting positions."},
        .values = LdshOptions_Start,
        .OnChange = Ledgedash_ToggleStartPosition,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LdshOptions_Reset) / 4,
        .val = OPTRESET_SAME_SIDE,
        .name = "Reset",
        .desc = {"Change where the fighter gets placed",
                 "after a ledgedash attempt."},
        .values = LdshOptions_Reset,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "HUD",
        .desc = {"Toggle visibility of the HUD."},
        .val = 1,
        .OnChange = Ledgedash_ChangeShowHUD,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Tips",
        .desc = {"Toggle the onscreen display of tips."},
        .val = 1,
        .OnChange = Tips_Toggle,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LdshOptions_CamMode) / 4,
        .name = "Camera Mode",
        .desc = {"Adjust the camera's behavior.",
                 "In advanced mode, use C-Stick while holding",
                 "A/B/Y to pan, rotate and zoom, respectively."},
        .values = LdshOptions_CamMode,
        .OnChange = Ledgedash_ChangeCamMode,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Keep Ledge Invincibility",
        .desc = {"Keep maximum invincibility while on the ledge",
                 "to practice the ledgedash inputs."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LdshOptions_GameSpeedText) /
                     sizeof(*LdshOptions_GameSpeedText),
        .name = "Game Speed",
        .desc = {"Change how fast the game engine runs."},
        .values = LdshOptions_GameSpeedText,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Color Overlays",
        .desc = {"Show which state you are in with a color overlay."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num =
            sizeof(LdshOptions_ResetDelay) / sizeof(*LdshOptions_ResetDelay),
        .val = 1,
        .name = "Reset Delay",
        .desc = {"Change how quickly you can start a new ledgedash."},
        .values = LdshOptions_ResetDelay,
    },
    {
        .kind = OPTKIND_INFO,
        .name = "HELP",
        .desc =
            {"Ledgedashing is the act of wavedashing onto stage from ledge.",
             "This is most commonly done by dropping off ledge, double jumping ",
             "immediately, and quickly airdodging onto stage. Each input",
             "is performed quickly after the last, making it difficult and risky."},
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Selection Screen."},
        .OnSelect = Event_Exit,
    },
};

static EventOption Ldsh_FrameAdvance = {
    .kind = OPTKIND_TOGGLE,
    .name = "Frame Advance",
    .desc = {"Enable frame advance. Press to advance one",
             "frame. Hold to advance at normal speed."},
};

static Shortcut Ldsh_Shortcuts[] = {
    {
        .button_mask = HSD_BUTTON_A,
        .option = &Ldsh_FrameAdvance,
    }
};

static ShortcutList Ldsh_ShortcutList = {
    .count = countof(Ldsh_Shortcuts),
    .list = Ldsh_Shortcuts,
};

static EventMenu LdshMenu_Main = {
    .name = "Ledgedash Training",
    .option_num = sizeof(LdshOptions_Main) / sizeof(EventOption),
    .options = LdshOptions_Main,
    .shortcuts = &Ldsh_ShortcutList,
};

// Init Function
void Event_Init(GOBJ *gobj)
{
    LedgedashData *event_data = gobj->userdata;

    // get assets
    event_data->assets = Archive_GetPublicAddress(event_vars->event_archive, "ledgedash");

    HSD_Update *hsd_update = stc_hsd_update;
    hsd_update->checkPause = Update_CheckPause;
    hsd_update->checkAdvance = Update_CheckAdvance;

    // standardize camera
    float *unk_cam = (void *)0x803bcca0;
    stc_stage->fov_r = 0; // no camera rotation
    stc_stage->x28 = 1;   // pan value?
    stc_stage->x2c = 1;   // pan value?
    stc_stage->x30 = 1;   // pan value?
    stc_stage->x34 = 130; // zoom out
    unk_cam[0x40 / 4] = 30;

    // Init hitlog
    event_data->hitlog_gobj = Ledgedash_HitLogInit();

    // Init HUD
    Ledgedash_HUDInit(event_data);

    // Init Fighter
    Ledgedash_FtInit(event_data);

    Fighter_PlaceOnLedge();
}
// Think Function
void Event_Think(GOBJ *event)
{
    LedgedashData *event_data = event->userdata;

    // get fighter data
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // no ledgefall
    FtCliffCatch *ft_state = (void *)&hmn_data->state_var;
    if (hmn_data->state_id == ASID_CLIFFWAIT)
        ft_state->fall_timer = 2;

    if (LdshOptions_Main[OPT_INV].val == 1) {
        if (hmn_data->state_id == ASID_CLIFFWAIT) {
            hmn_data->hurt.intang_frames.ledge = 30;
            hmn_data->TM.state_frame = 1;
        }
    }

    if (hmn_data->input.down & HSD_BUTTON_DPAD_LEFT) {
        event_data->ledge = -1;
        Fighter_PlaceOnLedge();
    } else if (hmn_data->input.down & HSD_BUTTON_DPAD_RIGHT) {
        event_data->ledge = 1;
        Fighter_PlaceOnLedge();
    }

    Ledgedash_ResetThink(event_data, hmn);
    Ledgedash_HUDThink(event_data, hmn_data);
    Ledgedash_HitLogThink(event_data, hmn);

    if (LdshOptions_Main[OPT_OVERLAYS].val == 1) {
        memset(&hmn_data->color[1], 0, sizeof(ColorOverlay));
        memset(&hmn_data->color[0], 0, sizeof(ColorOverlay));

        u32 curr_frame = event_data->action_state.timer - 1;
        if (curr_frame < 30) {
            int action = event_data->action_state.action_log[curr_frame];

            GXColor color;
            if (action == LDACT_NONE) {
                if (hmn_data->hurt.intang_frames.ledge != 0) {
                    color = tmgbar_blue;
                    color.a = 180;
                } else {
                    color = (GXColor) {0, 0, 0, 0};
                }
            } else {
                color = *tmgbar_colors[action];
                color.a = 180;
            }

            hmn_data->color[1].hex = color;
            hmn_data->color[1].color_enable = 1;
        }
    }
}
void Event_Exit(GOBJ *menu)
{
    // end game
    stc_match->state = 3;

    // cleanup
    Match_EndVS();
}

// Ledgedash functions
void Ledgedash_HUDInit(LedgedashData *event_data)
{
    GOBJ *hud_gobj = GObj_Create(0, 0, 0);
    event_data->hud.gobj = hud_gobj;
    // Load jobj
    JOBJ *hud_jobj = JOBJ_LoadJoint(event_data->assets->hud);
    GObj_AddObject(hud_gobj, 3, hud_jobj);
    GObj_AddGXLink(hud_gobj, GXLink_Common, GXLINK_HUD, 80);

    // create text canvas
    int canvas = Text_CreateCanvas(2, hud_gobj, 14, 15, 0, GXLINK_HUD, 81, 19);
    event_data->hud.canvas = canvas;

    // init text
    Text **text_arr = &event_data->hud.text_angle;
    for (int i = 0; i < 2; i++)
    {
        // Create text object
        Text *hud_text = Text_CreateText(2, canvas);
        text_arr[i] = hud_text;
        hud_text->kerning = 1;
        hud_text->align = 1;
        hud_text->use_aspect = 1;

        // Get position
        Vec3 text_pos;
        JOBJ *text_jobj;
        JOBJ_GetChild(hud_jobj, &text_jobj, 2 + i, -1);
        JOBJ_GetWorldPosition(text_jobj, 0, &text_pos);

        // adjust scale
        Vec3 *scale = &hud_jobj->scale;
        // text scale
        hud_text->viewport_scale.X = (scale->X * 0.01) * LCLTEXT_SCALE;
        hud_text->viewport_scale.Y = (scale->Y * 0.01) * LCLTEXT_SCALE;
        hud_text->aspect.X = 165;

        // text position
        hud_text->trans.X = text_pos.X + (scale->X / 4.0);
        hud_text->trans.Y = (text_pos.Y * -1) + (scale->Y / 4.0);

        // dummy text
        Text_AddSubtext(hud_text, 0, 0, "-");
    }

    // reset all bar colors
    JOBJ *timingbar_jobj;
    JOBJ_GetChild(hud_jobj, &timingbar_jobj, LCLJOBJ_BAR, -1); // get timing bar jobj
    JOBJ_SetFlagsAll(timingbar_jobj, JOBJ_HIDDEN);
}
void Ledgedash_HUDThink(LedgedashData *event_data, FighterData *hmn_data)
{
    // run tip logic
    Tips_Think(event_data, hmn_data);

    JOBJ *hud_jobj = event_data->hud.gobj->hsd_object;

    // check to initialize timer
    if ((hmn_data->state_id == ASID_CLIFFWAIT) && (hmn_data->TM.state_frame == 1))
    {
        Ledgedash_InitVariables(event_data);

        event_data->tip.refresh_num++;
    }

    u32 curr_frame = event_data->action_state.timer++;
    int hud_updating =
        curr_frame < countof(event_data->action_state.action_log)
        && hmn_data->hurt.intang_frames.ledge != 0;

    if (hud_updating) {
        int state_id = hmn_data->state_id;
        int action = LDACT_GALINT;

        // look for cliffwait
        if (state_id == ASID_CLIFFWAIT)
             action = LDACT_CLIFFWAIT;

        // look for release
        else if (state_id == ASID_FALL)
            action = LDACT_FALL;

        // look for jump
        else if (
            state_id == ASID_JUMPAERIALF
            || state_id == ASID_JUMPAERIALB
            // check for kirby and jiggs jump
            || ((hmn_data->kind == 4 || hmn_data->kind == 15) && (state_id >= 341 && state_id <= 345)))
            action = LDACT_JUMP;

        // look for airdodge
        else if (state_id == ASID_ESCAPEAIR)
            action = LDACT_AIRDODGE;

        // look for attack
        else if (hmn_data->atk_kind != 1 || state_id == ASID_CATCH || state_id == ASID_CATCHDASH)
            action = LDACT_ATTACK;

        // look for landing
        else if (
            (state_id == ASID_LANDING && hmn_data->TM.state_frame < hmn_data->attr.normal_landing_lag)
            || state_id == ASID_LANDINGFALLSPECIAL
        )
            action = LDACT_LANDING;

        // look for ledge options
        else if (ASID_CLIFFCLIMBSLOW <= state_id && state_id <= ASID_CLIFFJUMPQUICK2)
            action = LDACT_NONE;

        event_data->action_state.action_log[curr_frame] = action;
    }

    // grab airdodge angle
    if (event_data->action_state.is_airdodge == 0)
    {
        if ((hmn_data->state_id == ASID_ESCAPEAIR) || (hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR))
        {
            // determine airdodge angle
            float angle = atan2(fabs(hmn_data->input.lstick.Y), fabs(hmn_data->input.lstick.X));

            // save airdodge angle
            event_data->hud.airdodge_angle = angle;
            event_data->action_state.is_airdodge = 1;
        }
    }

    if (hmn_data->state_id == ASID_CLIFFWAIT)
        event_data->action_state.is_ledgegrab = 1;
    if (event_data->action_state.is_ledgegrab && hmn_data->state_id == ASID_FALL)
        event_data->action_state.is_release = 1;

    int released_ledge = event_data->action_state.is_release == 1;
    int not_finished = event_data->action_state.is_finished == 0;
    int just_finished = false;
    if (not_finished && released_ledge) {
        // if actionable after normal landing
        just_finished |= hmn_data->state_id == ASID_LANDING
            && hmn_data->TM.state_frame >= hmn_data->attr.normal_landing_lag;

        // if actionable after normal landing, but performed an action immediately after
        just_finished |= hmn_data->TM.state_prev[0] == ASID_LANDING
            || hmn_data->TM.state_prev[1] == ASID_LANDING;

        // if actionable after airdodge landing
        just_finished |= hmn_data->TM.state_prev[0] == ASID_LANDINGFALLSPECIAL
            || hmn_data->TM.state_prev[1] == ASID_LANDINGFALLSPECIAL;

        // if entered wait without entering landing - probably from NIL
        just_finished |= hmn_data->state_id == ASID_WAIT;

        event_data->action_state.is_finished |= just_finished;
    }

    if (just_finished) {
        // destroy any tips
        event_vars->Tip_Destroy();

        // output remaining airdodge angle
        if (event_data->action_state.is_airdodge == 1)
            Text_SetText(event_data->hud.text_angle, 0, "%.2f", fabs(event_data->hud.airdodge_angle / M_1DEGREE));
        else
            Text_SetText(event_data->hud.text_angle, 0, "-");

        // output remaining GALINT
        void *matanim;
        Text *text_galint = event_data->hud.text_galint;
        if (hmn_data->hurt.intang_frames.ledge > 0)
        {
            event_data->was_successful = true;
            matanim = event_data->assets->hudmatanim[0];
            Text_SetText(text_galint, 0, "%df", hmn_data->hurt.intang_frames.ledge);
        }
        else if (hmn_data->TM.vuln_frames < 25)
        {
            event_data->was_successful = false;
            matanim = event_data->assets->hudmatanim[1];
            Text_SetText(text_galint, 0, "-%df", hmn_data->TM.vuln_frames);
        }
        else
        {
            event_data->was_successful = false;
            matanim = event_data->assets->hudmatanim[1];
            Text_SetText(text_galint, 0, "-");
        }

        // init hitbox num
        LdshHitlogData *hitlog_data = event_data->hitlog_gobj->userdata;
        hitlog_data->num = 0;

        // apply HUD animation
        JOBJ_RemoveAnimAll(hud_jobj);
        JOBJ_AddAnimAll(hud_jobj, 0, matanim, 0);
        JOBJ_ReqAnimAll(hud_jobj, 0);
    }
}

void Ledgedash_ResetThink(LedgedashData *event_data, GOBJ *hmn)
{
    FighterData *hmn_data = hmn->userdata;

    int reset_mode = LdshOptions_Main[OPT_RESET].val;

    if (reset_mode == OPTRESET_NONE)
        return;

    if (event_data->reset_timer > 0) {
        event_data->reset_timer--;
        
        if (event_data->reset_timer > 0)
            return;

        int swap = false;
        switch (reset_mode) {
            case OPTRESET_SAME_SIDE:
                break;
            case OPTRESET_SWAP:
                swap = true;
                break;
            case OPTRESET_SWAP_ON_SUCCESS:
                swap = event_data->was_successful;
                break;
            case OPTRESET_RANDOM:
                swap = HSD_Randi(2);
                break;
        }

        if (swap)
            event_data->ledge = -event_data->ledge;

        Fighter_PlaceOnLedge();
    } else if (event_data->action_state.is_finished) {
        
        int reset_idx = LdshOptions_Main[OPT_RESETDELAY].val;
        event_data->reset_timer = LdshOptions_ResetDelaySuccess[reset_idx];
        if (event_data->was_successful)
            SFX_Play(303);
        else
            SFX_PlayCommon(3);
    } else {
        int state = hmn_data->state_id;

        bool dead = hmn_data->flags.dead;
        bool missed_airdodge = hmn_data->state_id == ASID_ESCAPEAIR && hmn_data->TM.state_frame >= 9;
        bool ledge_action = ASID_CLIFFCLIMBSLOW <= state && state <= ASID_CLIFFJUMPQUICK2;
        bool non_landing_grounded = hmn_data->phys.air_state == 0
            && event_data->action_state.is_release
            && state != ASID_LANDING
            && state != ASID_LANDINGFALLSPECIAL
            && state != ASID_REBIRTHWAIT
            && hmn_data->TM.state_frame >= 12;

        if (dead || missed_airdodge || ledge_action || non_landing_grounded) {
            int reset_idx = LdshOptions_Main[OPT_RESETDELAY].val;
            event_data->reset_timer = LdshOptions_ResetDelayFailure[reset_idx];
            event_data->was_successful = false;
            SFX_PlayCommon(3);
        }
    }
}
void Ledgedash_InitVariables(LedgedashData *event_data)
{
    event_data->action_state.timer = 0;
    event_data->action_state.is_ledgegrab = 0;
    event_data->action_state.is_release = 0;
    event_data->action_state.is_airdodge = 0;
    event_data->action_state.is_finished = 0;

    // init action log
    for (u32 i = 0; i < countof(event_data->action_state.action_log); i++)
    {
        event_data->action_state.action_log[i] = 0;
    }
}

// Menu Toggle functions
void Ledgedash_ToggleStartPosition(GOBJ *menu_gobj, int value)
{
    Fighter_PlaceOnLedge();
}

// Hitlog functions
GOBJ *Ledgedash_HitLogInit(void)
{

    GOBJ *hit_gobj = GObj_Create(0, 0, 0);
    LdshHitlogData *hit_data = calloc(sizeof(LdshHitlogData));
    GObj_AddUserData(hit_gobj, 4, HSD_Free, hit_data);
    GObj_AddGXLink(hit_gobj, Ledgedash_HitLogGX, 5, 0);

    // init array
    hit_data->num = 0;

    return hit_gobj;
}
void Ledgedash_HitLogThink(LedgedashData *event_data, GOBJ *hmn)
{
    FighterData *hmn_data = hmn->userdata;
    LdshHitlogData *hitlog_data = event_data->hitlog_gobj->userdata;

    // log hitboxes
    if (event_data->action_state.is_finished && hmn_data->hurt.intang_frames.ledge > 0)
    {

        // iterate through fighter hitboxes
        for (u32 i = 0; i < countof(hmn_data->hitbox); i++)
        {

            ftHit *this_hit = &hmn_data->hitbox[i];

            if ((this_hit->active != 0) &&           // if hitbox is active
                (hitlog_data->num < LDSH_HITBOXNUM)) // if not over max
            {

                // log info
                LdshHitboxData *this_ldsh_hit = &hitlog_data->hitlog[hitlog_data->num];
                this_ldsh_hit->size = this_hit->size;
                this_ldsh_hit->pos_curr = this_hit->pos;
                this_ldsh_hit->pos_prev = this_hit->pos_prev;
                this_ldsh_hit->kind = this_hit->attribute;

                // increment hitboxes
                hitlog_data->num++;
            }
        }

        // iterate through items belonging to fighter
        GOBJ *this_item = (*stc_gobj_lookup)[MATCHPLINK_ITEM];
        while (this_item != 0)
        {
            ItemData *this_itemdata = this_item->userdata;

            // ensure belongs to the fighter
            if (this_itemdata->fighter_gobj == hmn)
            {
                // iterate through item hitboxes
                for (u32 i = 0; i < countof(hmn_data->hitbox); i++)
                {

                    itHit *this_hit = &this_itemdata->hitbox[i];

                    if ((this_hit->active != 0) &&           // if hitbox is active
                        (hitlog_data->num < LDSH_HITBOXNUM)) // if not over max
                    {

                        // log info
                        LdshHitboxData *this_ldsh_hit = &hitlog_data->hitlog[hitlog_data->num];
                        this_ldsh_hit->size = this_hit->size;
                        this_ldsh_hit->pos_curr = this_hit->pos;
                        this_ldsh_hit->pos_prev = this_hit->pos_prev;
                        this_ldsh_hit->kind = this_hit->attribute;

                        // increment hitboxes
                        hitlog_data->num++;
                    }
                }
            }

            this_item = this_item->next;
        }
    }
}

void Ledgedash_HitLogGX(GOBJ *gobj, int pass)
{
    static GXColor hitlog_ambient = {128, 0, 0, 50};
    static GXColor hit_diffuse = {255, 99, 99, 50};
    static GXColor grab_diffuse = {255, 0, 255, 50};
    static GXColor detect_diffuse = {255, 255, 255, 50};

    LdshHitlogData *hitlog_data = gobj->userdata;
    
    if (pass == 2) {
        LedgedashData *event_data = event_vars->event_gobj->userdata;
        static GXColor colors[] = {
            {40, 40, 40, 180},
            {120, 120, 120, 180},
            {128, 255, 128, 180},
            {52, 202, 228, 180},
            {230, 22, 198, 180},
            {255, 255, 255, 180},
            {255, 128, 128, 180},
            {128, 128, 255, 180},
        };
        static char *names[] = {
            "Cliffwait",
            "Fall",
            "Jump",
            "Airdodge",
            "Attack",
            "Landing",
            "GALINT",
        };
        event_vars->HUD_DrawActionLogBar(
            event_data->action_state.action_log,
            colors,
            countof(event_data->action_state.action_log)
        );
        event_vars->HUD_DrawActionLogKey(
            names,
            &colors[1],
            countof(names)
        );
    }
    
    for (int i = 0; i < hitlog_data->num; i++)
    {
        LdshHitboxData *this_ldsh_hit = &hitlog_data->hitlog[i];

        // determine color
        GXColor *diffuse;
        if (this_ldsh_hit->kind == 0)
            diffuse = &hit_diffuse;
        else if (this_ldsh_hit->kind == 8)
            diffuse = &grab_diffuse;
        else if (this_ldsh_hit->kind == 11)
            diffuse = &detect_diffuse;
        else
            diffuse = &hit_diffuse;

        Develop_DrawSphere(this_ldsh_hit->size, &this_ldsh_hit->pos_curr, &this_ldsh_hit->pos_prev, diffuse, &hitlog_ambient);
    }
}

// Fighter fuctions
void Ledgedash_FtInit(LedgedashData *event_data)
{
    // create camera box
    CmSubject *cam = CameraSubject_Alloc();
    cam->boundleft_proj = -10;
    cam->boundright_proj = 10;
    cam->boundtop_proj = 10;
    cam->boundbottom_proj = -10;
    cam->boundleft_curr = cam->boundleft_proj;
    cam->boundright_curr = cam->boundright_proj;
    cam->boundtop_curr = cam->boundtop_proj;
    cam->boundbottom_curr = cam->boundbottom_proj;
    event_data->cam = cam;
    event_data->was_successful = false;
    event_data->reset_timer = 0;
    event_data->ledge = -1; // start on left ledge

    //if (event_vars->ledge_l == -1 || event_vars->ledge_r == -1) {
    //    event_data->cam->is_disable = 0;
    //    event_vars->Tip_Display(500 * 60, "Error:\nIt appears there are no\ngood ledges on this stage...");
    //}
}

void Ledgedash_ChangeShowHUD(GOBJ *menu_gobj, int show) {
    HUDCamData *cam = event_vars->hudcam_gobj->userdata;
    cam->hide = !show;
}

void Ledgedash_ChangeCamMode(GOBJ *menu_gobj, int value)
{
    MatchCamera *cam = stc_matchcam;

    // normal cam
    if (value == 0)
    {
        Match_SetNormalCamera();
    }
    // zoom cam
    else if (value == 1)
    {
        Match_SetFreeCamera(0, 3);
        cam->freecam_fov.X = 140;
        cam->freecam_rotate.Y = 10;
    }
    // fixed
    else if (value == 2)
    {
        Match_SetFixedCamera();
    }
    else if (value == 3)
    {
        Match_SetDevelopCamera();
    }
    Match_CorrectCamera();
}

void Event_Update(void)
{
    if (Pause_CheckStatus(1) != 2) {
        float speed = LdshOptions_GameSpeeds[LdshOptions_Main[OPT_SPEED].val];
        HSD_SetSpeedEasy(speed);
    } else {
        HSD_SetSpeedEasy(1.0);
    }
}

int Ledge_Find(int search_dir, float xpos_start, float *ledge_dir)
{
    // get line and vert pointers
    CollLine *collline = *stc_collline;

    // get initial closest
    float xpos_closest;
    if (search_dir == -1) // search left
        xpos_closest = -5000;
    else if (search_dir == 1) // search right
        xpos_closest = 5000;
    else // search both
        xpos_closest = 5000;

    // look for the closest ledge
    int index_closest = -1;
    CollGroup *this_group = *stc_firstcollgroup;
    while (this_group != 0) // loop through ground links
    {

        // 2 passes, one for ground and one for dynamic lines
        int line_index, line_num;
        for (int i = 0; i < 2; i++)
        {
            // first pass, use floors
            if (i == 0)
            {
                line_index = this_group->desc->floor_start;          // first ground link
                line_num = line_index + this_group->desc->floor_num; // ground link num
            }
            // second pass, use dynamics
            else if (i == 1)
            {
                line_index = this_group->desc->dyn_start;          // first ground link
                line_num = line_index + this_group->desc->dyn_num; // ground link num
            }

            // loop through lines
            while (line_index < line_num)
            {
                // get all data for this line
                CollLine *this_line = &collline[line_index]; // ??? i actually dont know why i cant access this directly
                CollLineDesc *this_linedesc = this_line->desc;

                // check if this link is a ledge
                if (this_linedesc->is_ledge)
                {

                    // check both sides of this ledge
                    Vec3 ledge_pos;
                    for (int j = 0; j < 2; j++)
                    {
                        // first pass, check left
                        if (j == 0)
                        {
                            GrColl_GetGroundLineEndLeft(line_index, &ledge_pos);
                        }
                        else if (j == 1)
                        {
                            GrColl_GetGroundLineEndRight(line_index, &ledge_pos);
                        }

                        // is within the camera range
                        if ((ledge_pos.X > Stage_GetCameraLeft()) && (ledge_pos.X < Stage_GetCameraRight()) && (ledge_pos.Y > Stage_GetCameraBottom()) && (ledge_pos.Y < Stage_GetCameraTop()))
                        {

                            // check for any obstructions
                            float dir_mult;
                            if (j == 0) // left ledge
                                dir_mult = -1;
                            else if (j == 1) // right ledge
                                dir_mult = 1;
                            int ray_index;
                            int ray_kind;
                            Vec3 ray_angle;
                            Vec3 ray_pos;
                            float from_x = ledge_pos.X + (2 * dir_mult);
                            float to_x = from_x;
                            float from_y = ledge_pos.Y + 5;
                            float to_y = from_y - 10;
                            int is_ground = GrColl_RaycastGround(&ray_pos, &ray_index, &ray_kind, &ray_angle, -1, -1, -1, 0, from_x, from_y, to_x, to_y, 0);
                            if (is_ground == 0)
                            {
                                int is_closer = 0;

                                if (search_dir == -1) // check if to the left
                                {
                                    if ((ledge_pos.X > xpos_closest) && (ledge_pos.X < xpos_start))
                                        is_closer = 1;
                                }
                                else if (search_dir == 1) // check if to the right
                                {
                                    if ((ledge_pos.X < xpos_closest) && (ledge_pos.X > xpos_start))
                                        is_closer = 1;
                                }
                                else // check if any direction
                                {
                                    float dist_old = fabs(xpos_start - xpos_closest);
                                    float dist_new = fabs(xpos_start - ledge_pos.X);
                                    if (dist_new < dist_old)
                                        is_closer = 1;
                                }

                                // determine direction
                                if (is_closer)
                                {

                                    // now determine if this line is a ledge in this direction
                                    if (j == 0) // left ledge
                                    {
                                        CollLine *prev_line = &collline[this_linedesc->line_prev];          // ??? i actually dont know why i cant access this directly
                                        if ((this_linedesc->line_prev == -1) || (prev_line->is_rwall == 1)) // if prev line is a right wall / if prev line doesnt exist
                                        {

                                            // save info on this line
                                            xpos_closest = ledge_pos.X; // save left vert's X position
                                            index_closest = line_index;
                                            *ledge_dir = 1;
                                        }
                                    }
                                    else if (j == 1) // right ledge
                                    {
                                        CollLine *next_line = &collline[this_linedesc->line_next];          // ??? i actually dont know why i cant access this directly
                                        if ((this_linedesc->line_prev == -1) || (next_line->is_lwall == 1)) // if prev line is a right wall / if prev line doesnt exist
                                        {

                                            // save info on this line
                                            xpos_closest = ledge_pos.X; // save left vert's X position
                                            index_closest = line_index;
                                            *ledge_dir = -1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                line_index++;
            }
        }

        // get next
        this_group = this_group->next;
    }

    return index_closest;
}
void Fighter_PlaceOnLedge(void)
{
    LedgedashData *event_data = event_vars->event_gobj->userdata;
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    float ledge_dir;
    int line_index = Ledge_Find(event_data->ledge, 0.f, &ledge_dir);

    if (line_index == -1) {
        event_vars->Tip_Display(500 * 60, "Error:\nIt appears there are no \nledges on this stage..");
        return;
    }

    Ledgedash_InitVariables(event_data);
    event_data->tip.refresh_displayed = 0;
    event_data->tip.is_input_release = 0;
    event_data->tip.refresh_num = 0;

    // get ledge position
    Vec3 ledge_pos;
    if (ledge_dir > 0)
        GrColl_GetGroundLineEndLeft(line_index, &ledge_pos);
    else
        GrColl_GetGroundLineEndRight(line_index, &ledge_pos);

    // remove velocity
    hmn_data->phys.self_vel.X = 0;
    hmn_data->phys.self_vel.Y = 0;

    // restore tether
    hmn_data->flags.used_tether = 0;


    // Sleep first
    Fighter_EnterSleep(hmn, 0);
    Fighter_EnterRebirth(hmn);

    // face the ledge
    hmn_data->facing_direction = ledge_dir;

    // check starting position
    switch (LdshOptions_Main[OPT_POS].val)
    {
    case OPTPOS_LEDGE:
    {
        // place player on this ledge
        event_data->tip.refresh_num = -1; // setting this to -1 because the per frame code will add 1 and make it 0
        FtCliffCatch *ft_state = (void *)&hmn_data->state_var;
        ft_state->ledge_index = line_index; // store line index
        Fighter_EnterCliffWait(hmn);
        ft_state->timer = 0; // spoof as on ledge for a frame already
        Fighter_SetAirborne(hmn_data);
        Fighter_EnableCollUpdate(hmn_data);
        Coll_CheckLedge(&hmn_data->coll_data);
        Fighter_MoveToCliff(hmn);
        Fighter_UpdatePosition(hmn);
        ftCommonData *ftcommon = *stc_ftcommon;

        // This needs to be +1 for some reason, otherwise we get an off-by-one in galint calculations
        Fighter_ApplyIntang(hmn, ftcommon->cliff_invuln_time+1);
        break;
    }
    case OPTPOS_FALLING:
    {
        // place player falling above the ledge
        hmn_data->phys.pos.X = ledge_pos.X + -5 * ledge_dir; // slight nudge to prevent accidentally landing on stage
        hmn_data->phys.pos.Y = ledge_pos.Y + 20;
        Fighter_UpdatePosition(hmn);
        Fighter_EnterFall(hmn);
        break;
    }
    case OPTPOS_STAGE:
    {
        // place player on stage next to ledge
        Vec3 coll_pos, line_unk;
        int line_index, line_kind;
        float x = ledge_pos.X + 12 * ledge_dir;
        float from_y = ledge_pos.Y + 5;
        float to_y = from_y - 10;
        int is_ground = GrColl_RaycastGround(&coll_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, x, from_y, x, to_y, 0);
        if (is_ground == 1)
        {
            hmn_data->phys.pos = coll_pos;
            Fighter_UpdatePosition(hmn);
            Fighter_EnterWait(hmn);
        }
        break;
    }
    case OPTPOS_RESPAWNPLAT:
    {
        // place player in a random position in respawn wait
        float xpos_min = 40;
        float xpos_max = 65;
        float ypos_min = -30;
        float ypos_max = 30;
        hmn_data->phys.pos.X = ((ledge_dir * -1) * (xpos_min + HSD_Randi(xpos_max - xpos_min) + HSD_Randf())) + (ledge_pos.X);
        hmn_data->phys.pos.Y = ((ledge_dir * -1) * (ypos_min + HSD_Randi(ypos_max - ypos_min) + HSD_Randf())) + (ledge_pos.Y);

        // enter rebirth
        Fighter_EnterRebirthWait(hmn);
        hmn_data->cb.Phys = RebirthWait_Phys;
        hmn_data->cb.IASA = RebirthWait_IASA;

        Fighter_UpdateRebirthPlatformPos(hmn);

        break;
    }
    }

    // avoid double reset in case user manually resets
    event_data->reset_timer = 0;

    // update camera box
    CmSubject *cam = event_data->cam;
    cam->cam_pos.X = ledge_pos.X + (ledge_dir * 20);
    cam->cam_pos.Y = ledge_pos.Y + 15;

    Fighter_UpdateCamera(hmn);

    // remove all particles
    for (int i = 0; i < PTCL_LINKMAX; i++)
    {
        Particle **ptcls = &stc_ptcl[i];
        Particle *ptcl = *ptcls;
        while (ptcl != 0)
        {

            Particle *ptcl_next = ptcl->next;

            // begin destroying this particle

            // subtract some value, 8039c9f0
            if (ptcl->gen != 0)
            {
                ptcl->gen->particle_num--;
            }
            // remove from generator? 8039ca14
            if (ptcl->gen != 0)
                psRemoveParticleAppSRT(ptcl);

            // delete parent jobj, 8039ca48
            psDeletePntJObjwithParticle(ptcl);

            // update most recent ptcl pointer
            *ptcls = ptcl->next;

            // free alloc, 8039ca54
            HSD_ObjFree((void *)0x804d0f60, ptcl);

            // decrement ptcl total
            u16 ptclnum = *stc_ptclnum;
            ptclnum--;
            *stc_ptclnum = ptclnum;

            // get next
            ptcl = ptcl_next;
        }
    }

    // remove all camera shake gobjs (p_link 18, entity_class 3)
    GOBJ *gobj = (*stc_gobj_lookup)[MATCHPLINK_MATCHCAM];
    while (gobj != 0)
    {

        GOBJ *gobj_next = gobj->next;

        // if entity class 3 (quake)
        if (gobj->entity_class == 3)
        {
            GObj_Destroy(gobj);
        }

        gobj = gobj_next;
    }
}
void Fighter_UpdatePosition(GOBJ *fighter)
{

    FighterData *fighter_data = fighter->userdata;

    // Update Position (Copy Physics XYZ into all ECB XYZ)
    fighter_data->coll_data.topN_Curr.X = fighter_data->phys.pos.X;
    fighter_data->coll_data.topN_Curr.Y = fighter_data->phys.pos.Y;
    fighter_data->coll_data.topN_Prev.X = fighter_data->phys.pos.X;
    fighter_data->coll_data.topN_Prev.Y = fighter_data->phys.pos.Y;
    fighter_data->coll_data.topN_CurrCorrect.X = fighter_data->phys.pos.X;
    fighter_data->coll_data.topN_CurrCorrect.Y = fighter_data->phys.pos.Y;
    fighter_data->coll_data.topN_Proj.X = fighter_data->phys.pos.X;
    fighter_data->coll_data.topN_Proj.Y = fighter_data->phys.pos.Y;

    // Update Collision Frame ID
    fighter_data->coll_data.coll_test = *stc_colltest;

    // Adjust JObj position (code copied from 8006c324)
    JOBJ *fighter_jobj = fighter->hsd_object;
    fighter_jobj->trans.X = fighter_data->phys.pos.X;
    fighter_jobj->trans.Y = fighter_data->phys.pos.Y;
    fighter_jobj->trans.Z = fighter_data->phys.pos.Z;
    JOBJ_SetMtxDirtySub(fighter_jobj);

    // Update Static Player Block Coords
    Fighter_SetPosition(fighter_data->ply, fighter_data->flags.ms, &fighter_data->phys.pos);
}
void Fighter_UpdateCamera(GOBJ *fighter)
{
    FighterData *fighter_data = fighter->userdata;

    // Update camerabox pos
    Fighter_UpdateCameraBox(fighter);

    // Update tween
    fighter_data->camera_subject->boundleft_curr = fighter_data->camera_subject->boundleft_proj;
    fighter_data->camera_subject->boundright_curr = fighter_data->camera_subject->boundright_proj;

    // update camera position
    Match_CorrectCamera();

    // reset onscreen bool
    //Fighter_UpdateOnscreenBool(fighter);
    fighter_data->flags.is_offscreen = 0;
}
void RebirthWait_Phys(GOBJ *fighter)
{

    FighterData *fighter_data = fighter->userdata;

    // infinite time
    fighter_data->state_var.state_var1 = 2;
}
int RebirthWait_IASA(GOBJ *fighter)
{

    FighterData *fighter_data = fighter->userdata;
    if (!Fighter_IASACheck_JumpAerial(fighter))
    {
        ftCommonData *ftcommon = *stc_ftcommon;

        // check for lstick movement
        float stick_x = fabs(fighter_data->input.lstick.X);
        float stick_y = fighter_data->input.lstick.Y;
        if (
            (stick_x > 0.2875f && fighter_data->input.timer_lstick_tilt_x < 2)
            || (stick_y < -ftcommon->lstick_rebirthfall && fighter_data->input.timer_lstick_tilt_y < 4)
        ) {
            Fighter_EnterFall(fighter);
            return 1;
        }
    }

    return 0;
}
int Fighter_IsFallInput(FighterData *hmn_data)
{
    float thresh = (*stc_ftcommon)->ledge_drop_thresh; // 0.2875
    float drop_angle = (*stc_ftcommon)->lstick_tilt;  // 0.872665 (50 deg)
    float lx = hmn_data->input.lstick.X;
    float ly = hmn_data->input.lstick.Y;
    float angle = atan2(ly, fabs(lx));
    float cx = hmn_data->input.cstick.X;
    float cy = hmn_data->input.cstick.Y;

    // Technically there are some false positives here that result in a ledgejump.
    // However, this shouldn't affect the tip displays.
    int lstick_drop = (fabs(lx) >= thresh || fabs(ly) >= thresh) &&
        !(angle > drop_angle || (angle > -drop_angle && lx * hmn_data->facing_direction >= 0));
    int cstick_drop = (fabs(cx) >= thresh || fabs(cy) >= thresh);
    return (lstick_drop || cstick_drop) && !Fighter_IsFallBlocked(hmn_data);
}
int Fighter_IsFallBlocked(FighterData *hmn_data)
{
    float thresh = (*stc_ftcommon)->ledge_drop_thresh; // 0.2875
    float lx = hmn_data->input.lstick_prev.X;
    float ly = hmn_data->input.lstick_prev.Y;
    float cx = hmn_data->input.cstick_prev.X;
    float cy = hmn_data->input.cstick_prev.Y;

    return fabs(lx) >= thresh || fabs(ly) >= thresh ||
        fabs(cx) >= thresh || fabs(cy) >= thresh;
}
// Tips Functions
void Tips_Toggle(GOBJ *menu_gobj, int value)
{
    // destroy existing tips when disabling
    if (value == 1)
        event_vars->Tip_Destroy();
}
void Tips_Think(LedgedashData *event_data, FighterData *hmn_data)
{
    // skip if tips turned off
    if (LdshOptions_Main[OPT_TIPS].val == 1)
        return;

    // check for early fall input in cliffcatch
    if (!event_data->tip.is_input_release && hmn_data->state_id == ASID_CLIFFCATCH && Fighter_IsFallInput(hmn_data))
    {
        event_data->tip.is_input_release = 1;
        event_vars->Tip_Destroy();

        // determine how many frames early
        Figatree *anim = Fighter_GetAnimData(hmn_data, hmn_data->action_id);
        float frame_num = anim->frame_num;
        float frames_early = frame_num - hmn_data->state.frame;
        event_vars->Tip_Display(3 * 60, "Misinput:\nFell %d frames early.", (int)frames_early + 1);
    }

    // check for early fall input on cliffwait frame 0
    if (!event_data->tip.is_input_release && hmn_data->state_id == ASID_CLIFFWAIT && hmn_data->TM.state_frame == 1 && Fighter_IsFallInput(hmn_data))
    {
        event_data->tip.is_input_release = 1;
        event_vars->Tip_Destroy();
        event_vars->Tip_Display(LSDH_TIPDURATION, "Misinput:\nFell 1 frame early.");
    }

    if (!event_data->tip.is_input_release && hmn_data->state_id == ASID_CLIFFJUMPQUICK1)
    {
        if (Fighter_IsFallInput(hmn_data))
        {
            event_data->tip.is_input_release = 1;
            event_vars->Tip_Destroy();

            // jumped before fall
            event_vars->Tip_Display(LSDH_TIPDURATION, "Misinput:\nJumped %d frame(s) early.", hmn_data->TM.state_frame + 1);
        }
        else if (Fighter_IsFallBlocked(hmn_data))
        {
            event_data->tip.is_input_release = 1;
            event_vars->Tip_Destroy();

            // failed to release sticks to neutral
            event_vars->Tip_Display(LSDH_TIPDURATION, "Misinput:\nDid not reset sticks \nto neutral before dropping.", hmn_data->TM.state_frame);
        }
    }

    // check for ledgedash without refreshing
    if (!event_data->tip.refresh_displayed && event_data->action_state.is_finished && event_data->tip.refresh_num == 0)
    {

        event_data->tip.refresh_displayed = 1;

        // increment condition count
        event_data->tip.refresh_cond_num++;

        // after 3 conditions, display tip
        if (event_data->tip.refresh_cond_num >= 3)
        {
            // if tip is displayed, reset cond num
            if (event_vars->Tip_Display(5 * 60, "Warning:\nIt is higly recommended to\nre-grab ledge after \nbeing reset to simulate \na realistic scenario!"))
                event_data->tip.refresh_cond_num = 0;
        }
    }
}

int Update_CheckPause(void)
{
    HSD_Update *update = stc_hsd_update;
    int isChange = 0;

    // menu paused
    if (Ldsh_FrameAdvance.val == 1)
    {
        // check if unpaused
        if (update->pause_kind != PAUSEKIND_SYS)
            isChange = 1;
    }
    // menu unpaused
    else
    {
        // check if paused
        if (update->pause_kind == PAUSEKIND_SYS)
            isChange = 1;
    }

    return isChange;
}
int Update_CheckAdvance(void)
{
    static int timer = 0;

    HSD_Update *update = stc_hsd_update;
    int isAdvance = 0;

    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    int controller = hmn_data->pad_index;

    // get their pad
    HSD_Pad *pad = PadGetMaster(controller);
    HSD_Pad *engine_pad = PadGetEngine(controller);

    // get their advance input
    static int stc_advance_btns[] = {HSD_TRIGGER_L, HSD_TRIGGER_Z, HSD_BUTTON_X, HSD_BUTTON_Y, HSD_TRIGGER_R};
    Memcard *memcard = stc_memcard;
    u32 btn_idx = memcard->TM_LabFrameAdvanceButton;
    if (btn_idx >= countof(stc_advance_btns))
        btn_idx = 0;
    int advance_btn = stc_advance_btns[btn_idx];

    // check if holding L
    if (Ldsh_FrameAdvance.val == 1 && (pad->held & advance_btn))
    {
        timer++;

        // advance if first press or holding more than 10 frames
        if (timer == 1 || timer > 30)
        {
            isAdvance = 1;

            // remove button input
            pad->down &= ~advance_btn;
            pad->held &= ~advance_btn;
            engine_pad->down &= ~advance_btn;
            engine_pad->held &= ~advance_btn;

            // if using L, remove analog press too
            if (advance_btn == HSD_TRIGGER_L)
            {
                pad->triggerLeft = 0;
                pad->ftriggerLeft = 0;
                engine_pad->triggerLeft = 0;
                engine_pad->ftriggerLeft = 0;
            }
            else if (advance_btn == HSD_TRIGGER_R)
            {
                pad->triggerRight = 0;
                pad->ftriggerRight = 0;
                engine_pad->triggerRight = 0;
                engine_pad->ftriggerRight = 0;
            }
        }
    }
    else
    {
        update->advance = 0;
        timer = 0;
    }

    return isAdvance;
}

// Initial Menu
EventMenu *Event_Menu = &LdshMenu_Main;
