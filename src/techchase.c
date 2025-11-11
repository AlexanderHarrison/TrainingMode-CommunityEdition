#include "../MexTK/mex.h"
#include "events.h"

#define INTANG_COLANIM 10
static void Exit(GOBJ *menu);

static EventMenu Menu_Main;
static EventMenu Menu_Chances;

enum {
    OPT_CHANCE_MENU,
    OPT_REACTION_OSD,
    OPT_INVIS,
    OPT_EXIT,

    OPT_COUNT
};

static const char *Options_Invis[] = { "None", "Post-Reaction", "Flash" };
enum {
    OPTINVIS_NONE,
    OPTINVIS_POST_REACTION,
    OPTINVIS_FLASH,

    OPTINVIS_COUNT
};

static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_MENU,
        .menu = &Menu_Chances,
        .name = "Tech Chances",
        .desc = {"Adjust option's rates."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Reaction Frame OSD",
        .desc = {"Check which frame you reacted on."},
    },
    {
        .kind = OPTKIND_STRING,
        .name = "Tech Invisibility",
        .desc = {"Toggle the CPU turning invisible during tech", "animations."},
        .values = Options_Invis,
        .value_num = countof(Options_Invis),
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Select Screen."},
        .OnSelect = Exit,
    },
};

static EventMenu Menu_Main = {
    .name = "Tech-Chase Training",
    .option_num = countof(Options_Main),
    .options = Options_Main,
};

EventMenu *Event_Menu = &Menu_Main;

enum {
    OPTCHANCE_TECHINPLACE,
    OPTCHANCE_TECHAWAY,
    OPTCHANCE_TECHTOWARD,
    OPTCHANCE_MISSTECH,
    OPTCHANCE_GETUPWAIT,
    OPTCHANCE_GETUPSTAND,
    OPTCHANCE_GETUPAWAY,
    OPTCHANCE_GETUPTOWARD,
    OPTCHANCE_GETUPATTACK,

    OPTCHANCE_COUNT
};

static void ChangeTechInPlaceChance (GOBJ *menu_gobj, int _new_val);
static void ChangeTechAwayChance    (GOBJ *menu_gobj, int _new_val);
static void ChangeTechTowardChance  (GOBJ *menu_gobj, int _new_val);
static void ChangeMissTechChance    (GOBJ *menu_gobj, int _new_val);
static void ChangeStandChance       (GOBJ *menu_gobj, int _new_val);
static void ChangeRollAwayChance    (GOBJ *menu_gobj, int _new_val);
static void ChangeRollTowardChance  (GOBJ *menu_gobj, int _new_val);
static void ChangeGetupAttackChance (GOBJ *menu_gobj, int _new_val);

static EventOption Options_Chances[] = {
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Tech in Place Chance",
        .desc = {"Adjust the chance the CPU will tech in place."},
        .format = "%d%%",
        .OnChange = ChangeTechInPlaceChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Tech Away Chance",
        .desc = {"Adjust the chance the CPU will tech away."},
        .format = "%d%%",
        .OnChange = ChangeTechAwayChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Tech Toward Chance",
        .desc = {"Adjust the chance the CPU will tech toward."},
        .format = "%d%%",
        .OnChange = ChangeTechTowardChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Miss Tech Chance",
        .desc = {"Adjust the chance the CPU will miss tech."},
        .format = "%d%%",
        .OnChange = ChangeMissTechChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 0,
        .name = "Miss Tech Wait Chance",
        .desc = {"Adjust the chance the CPU will wait 15 frames",
                 "after a missed tech."},
        .format = "%d%%",
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Stand Chance",
        .desc = {"Adjust the chance the CPU will stand."},
        .format = "%d%%",
        .OnChange = ChangeStandChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Roll Away Chance",
        .desc = {"Adjust the chance the CPU will roll away."},
        .format = "%d%%",
        .OnChange = ChangeRollAwayChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Roll Toward Chance",
        .desc = {"Adjust the chance the CPU will roll toward."},
        .format = "%d%%",
        .OnChange = ChangeRollTowardChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Getup Attack Chance",
        .desc = {"Adjust the chance the CPU will getup attack."},
        .format = "%d%%",
        .OnChange = ChangeGetupAttackChance,
    },
};

static EventMenu Menu_Chances = {
    .name = "Option Chances",
    .option_num = countof(Options_Chances),
    .options = Options_Chances,
};

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
}

static int reset_timer = -1;
static int missed_tech_getup_timer = -1;
static int first_action_frame = -1;

static void Reset(void) {
    event_vars->Savestate_Load_v1(event_vars->savestate, Savestate_Silent);

    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;

    hmn_data->facing_direction = 1;
    cpu_data->facing_direction = 1;

    hmn_data->phys.pos.X = 0;
    hmn_data->phys.pos.Y = 0;
    cpu_data->phys.pos.X = 10;
    cpu_data->phys.pos.Y = 30;

    UpdatePosition(hmn);
    UpdatePosition(cpu);
    PlacePlayerOnGround(hmn);
    Fighter_EnterDamageFall(cpu);
    
    UpdateCameraBox(hmn);
    UpdateCameraBox(cpu);

    Match_CorrectCamera();

    reset_timer = -1;
    missed_tech_getup_timer = -1;
    first_action_frame = -1;
}

static int tech_frame_distinguishable[27] = {
     8, // Mario
     4, // Fox
     6, // Captain Falcon
     9, // Donkey Kong
     3, // Kirby
     1, // Bowser
     6, // Link
     8, // Sheik
     8, // Ness
     3, // Peach
     9, // Popo (Ice Climbers)
     9, // Nana (Ice Climbers)
     7, // Pikachu
     6, // Samus
     9, // Yoshi
     3, // Jigglypuff
    16, // Mewtwo
     8, // Luigi
     7, // Marth
     6, // Zelda
     6, // Young Link
     8, // Dr. Mario
     4, // Falco
     8, // Pichu
     3, // Game & Watch
     6, // Ganondorf
     7, // Roy
};

void Event_Think(GOBJ *menu) {
    if (event_vars->game_timer == 1) {
        event_vars->Savestate_Save_v1(event_vars->savestate, Savestate_Silent);
        Reset();
    }

    GOBJ *hmn = Fighter_GetGObj(0);
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *hmn_data = hmn->userdata;
    FighterData *cpu_data = cpu->userdata;

    cpu_data->cpu.ai = 15;
    cpu_data->cpu.held = 0;
    cpu_data->cpu.lstickX = 0;
    cpu_data->cpu.lstickY = 0;
    cpu_data->cpu.cstickX = 0;
    cpu_data->cpu.cstickY = 0;

    int state_id = cpu_data->state_id;
    
    // check for action
    if (
        Options_Main[OPT_REACTION_OSD].val
        && ASID_DOWNSPOTD <= state_id && state_id <= ASID_PASSIVESTANDB
        && first_action_frame == -1
    ) {
        HSD_Pad *engine_pad = PadGetEngine(hmn_data->pad_index);
        
        int input = engine_pad->held | engine_pad->stickX | engine_pad->stickY | engine_pad->substickX | engine_pad->substickY;
        if (input != 0) {
            first_action_frame = cpu_data->TM.state_frame;
            int frame_distinguishable = tech_frame_distinguishable[cpu_data->kind];

            event_vars->Message_Display(
                15, hmn_data->ply, 0, 
                "Reaction: %if\n", first_action_frame - frame_distinguishable
            );
        }
    }
    
    // set intangibility
    if (state_id == ASID_WAIT || state_id == ASID_DAMAGEFALL) {
        cpu_data->hurt.intang_frames.ledge = 1;
        cpu_data->hurt.kind_game = 1;
    } else {
        cpu_data->hurt.intang_frames.ledge = 0;
        cpu_data->hurt.kind_game = 0;
    }

    if (reset_timer == -1) {
        // tech if airborne
        if (cpu_data->phys.air_state == 1 && cpu_data->phys.pos.Y < 5) {
            cpu_data->input.timer_LR = 0;
            
            int rng = HSD_Randi(100);
            s8 x = 0;
            char LR = 0;
            if ((rng -= Options_Chances[OPTCHANCE_TECHINPLACE].val) < 0) {

            } else if ((rng -= Options_Chances[OPTCHANCE_TECHAWAY].val) < 0) {
                x = 40 * (int)sign(cpu_data->phys.pos.X - hmn_data->phys.pos.X);
            } else if ((rng -= Options_Chances[OPTCHANCE_TECHTOWARD].val) < 0) {
                x = 40 * (int)sign(hmn_data->phys.pos.X - cpu_data->phys.pos.X);
            } else {
                LR = -1;
            }
            cpu_data->cpu.lstickX = x;
            cpu_data->input.timer_LR = LR;
        }
        
        // missed tech getup
        if (missed_tech_getup_timer == -1) {
            if (state_id == ASID_DOWNWAITU || state_id == ASID_DOWNWAITD) {
                int rng = HSD_Randi(100);
                if (rng < Options_Chances[OPTCHANCE_GETUPWAIT].val) {
                    missed_tech_getup_timer = 15;
                } else {
                    rng = HSD_Randi(100);
                    s8 x = 0;
                    s8 y = 0;
                    int held = 0;
                    if ((rng -= Options_Chances[OPTCHANCE_GETUPSTAND].val) < 0) {
                        y = 127;
                    } else if ((rng -= Options_Chances[OPTCHANCE_GETUPAWAY].val) < 0) {
                        x = 40 * (int)sign(cpu_data->phys.pos.X - hmn_data->phys.pos.X);
                    } else if ((rng -= Options_Chances[OPTCHANCE_GETUPTOWARD].val) < 0) {
                        x = 40 * (int)sign(hmn_data->phys.pos.X - cpu_data->phys.pos.X);
                    } else {
                        held = PAD_BUTTON_A;
                    }
                    cpu_data->cpu.lstickX = x;
                    cpu_data->cpu.lstickY = y;
                    cpu_data->cpu.held = held;
                }
            }
        } else {
            missed_tech_getup_timer--;
        }

        // fail if finished tech 
        if (state_id == ASID_WAIT) {
            SFX_Play(0xAF);
            reset_timer = 30;
        }
        // succeed if grabbed or hit 
        else if (
            (ASID_CAPTUREPULLEDHI <= state_id && state_id <= ASID_CAPTUREFOOT)
            || (ASID_DAMAGEHI1 <= state_id && state_id <= ASID_DAMAGEFLYROLL)
            || (cpu_data->dmg.hitlag_frames > 0 && cpu_data->flags.hitlag_victim)
        ) {
            SFX_Play(0xAD);
            reset_timer = 30;
        }
    }

    if (reset_timer >= 0 && reset_timer-- == 0) {
        Reset();
    }
}

static void Event_PostThink(GOBJ *menu) {
    GOBJ *cpu = Fighter_GetGObj(1);
    FighterData *cpu_data = cpu->userdata;
    int state_id = cpu_data->state_id;

    int frame_distinguishable = tech_frame_distinguishable[cpu_data->kind];
    switch (Options_Main[OPT_INVIS].val) {
        case OPTINVIS_NONE: {
            cpu_data->flags.invisible = false;
        } break;
        case OPTINVIS_POST_REACTION: {
            bool post_reaction =
                ASID_PASSIVE <= state_id && state_id <= ASID_PASSIVESTANDB
                && frame_distinguishable < cpu_data->TM.state_frame && cpu_data->TM.state_frame < 20;
            cpu_data->flags.invisible = post_reaction;
        } break;
        case OPTINVIS_FLASH: {
            bool on_reaction = (
                    ASID_PASSIVE <= state_id && state_id <= ASID_PASSIVESTANDB
                    && cpu_data->TM.state_frame + 1 != frame_distinguishable
                    && cpu_data->TM.state_frame < 20
                ) 
                || state_id == ASID_DAMAGEFALL;
            cpu_data->flags.invisible = on_reaction;
        } break;
    }
}

void Event_Init(GOBJ *gobj) {
    GObj_AddProc(gobj, Event_PostThink, 20);
}

static void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

// Clean up percentages so the total is always 100.
// The next percentage is modified.
static void ReboundChances(s16 *chances[], unsigned int chance_count, int just_changed_option) {
    if (chance_count == 0) return;

    int sum = 0;
    for (u32 t = 0; t < chance_count; ++t)
        sum += *chances[t];

    int delta = 100 - sum;
    // keep adding/removing from next option chance until all needed change to revert to 100% is applied
    while (delta)
    {
        int rebound_option = (just_changed_option + 1) % chance_count;

        s16 *opt_val = chances[rebound_option];
        int prev_chance = (int)*opt_val;

        int new_chance = prev_chance + delta;
        if (new_chance < 0) {
            *opt_val = 0;
            delta = new_chance;
        } else if (new_chance > 100) {
            *opt_val = 100;
            delta = new_chance - 100;
        } else {
            *opt_val = (u16)new_chance;
            delta = 0;
        }

        just_changed_option = rebound_option;
    }
}

static void ReboundTechChances(EventOption *tech_menu, int slot_idx_changed) {
    s16 *chances[4];
    int enabled_slots = 0;

    for (int i = 0; i < 4; i++) {
        chances[enabled_slots] = &tech_menu[i].val;
        enabled_slots++;
    }

    ReboundChances(chances, 4, slot_idx_changed);
}

static void ChangeTechInPlaceChance (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_TECHINPLACE], 0); }
static void ChangeTechAwayChance    (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_TECHINPLACE], 1); }
static void ChangeTechTowardChance  (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_TECHINPLACE], 2); }
static void ChangeMissTechChance    (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_TECHINPLACE], 3); }

static void ChangeStandChance       (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_GETUPSTAND], 0); }
static void ChangeRollAwayChance    (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_GETUPSTAND], 1); }
static void ChangeRollTowardChance  (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_GETUPSTAND], 2); }
static void ChangeGetupAttackChance (GOBJ *menu_gobj, int _new_val) { ReboundTechChances(&Options_Chances[OPTCHANCE_GETUPSTAND], 3); }
