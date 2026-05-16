#include "../MexTK/mex.h"
#include "events.h"

#define LEDGE_X 70.f
#define FULL_FALCO_SHORTHOP_DISTANCE 30.f
#define SLIGHT_FALCO_SHORTHOP_DISTANCE 13.f

void Exit(GOBJ *menu);
void ChangeFireSpeedOption(GOBJ *event_menu, int value);
void ChangeDirection(GOBJ *event_menu, int value);
void Reset(void);
void ChangeRandomFireDelayMin(GOBJ *event_menu, int value);
void ChangeRandomFireDelayMax(GOBJ *event_menu, int value);
int GetRandomLaserDelay(void);

enum menu_options {
    OPT_FIRE_SPEED,
    OPT_FIRE_DELAY_RANDOM_MIN,
    OPT_FIRE_DELAY_RANDOM_MAX,
    OPT_LASER_HEIGHT,
    OPT_DIRECTION,
    OPT_MOVEMENT,
};

enum fire_speed {
    FIRE_SPEED_RANDOM,
    FIRE_SPEED_SLOW,
    FIRE_SPEED_MEDIUM,
    FIRE_SPEED_FAST,
};

enum laser_height {
    LASER_HEIGHT_RANDOM,
    LASER_HEIGHT_VERY_LOW,
    LASER_HEIGHT_LOW,
    LASER_HEIGHT_MID,
    LASER_HEIGHT_HIGH,
};

enum falco_direction {
    DIRECTION_RIGHT,
    DIRECTION_LEFT,
};

enum movement {
    MOVEMENT_IN_PLACE,
    MOVEMENT_RANDOM,
    MOVEMENT_APPROACHING,
    MOVEMENT_RETREATING,
};

static const char *Options_FireSpeed[] = { "Random", "Slow", "Medium", "Fast" };
static const char *Options_LaserHeight[] = { "Random", "Very Low", "Low", "Mid", "High" };
static const char *Options_Direction[] = { "Right", "Left" };
static const char *Options_Movement[] = { "In-Place", "Random", "Approaching", "Retreating" };

static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(Options_FireSpeed) / 4,
        .name = "Fire Speed",
        .desc = {"Change the rate of fire."},
        .values = Options_FireSpeed,
        .OnChange = ChangeFireSpeedOption,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 61,
        .val = 0,
        .name = "Min Fire Delay",
        .desc = {"Adjust the minimum number of frames between lasers"},
        .format = "%d",
        .OnChange = ChangeRandomFireDelayMin,
        .disable = false,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 61,
        .val = 20,
        .name = "Max Fire Delay",
        .desc = {"Adjust the maximum number of frames between lasers"},
        .format = "%d",
        .OnChange = ChangeRandomFireDelayMax,
        .disable = false,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(Options_LaserHeight) / 4,
        .name = "Laser Height",
        .desc = {"Change the laser height."},
        .values = Options_LaserHeight,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(Options_Direction) / 4,
        .name = "Direction",
        .desc = {"Change which way falco shoots a laser."},
        .values = Options_Direction,
        .OnChange = ChangeDirection,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(Options_Movement) / 4,
        .name = "Movement",
        .desc = {"Change how falco lasers around the stage."},
        .values = Options_Movement,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Select Screen."},
        .OnSelect = Exit,
    },
};

static EventMenu Menu_Main = {
    .name = "Powershield Training",
    .option_num = sizeof(Options_Main) / sizeof(EventOption),
    .options = Options_Main,
};

static int falco_wait_delay = -1;
static int falco_shoot_delay = -1;
static int falco_fastfall_delay = -1;

static int falco_dash_direction = 0;
static int falco_jump_direction = 0;
static int falco_shoot_direction = 0;

static void PutOnGround(GOBJ *ft);

void Event_Think(GOBJ *menu) {
    GOBJ *player = Fighter_GetGObj(0);
    FighterData *player_data = player->userdata;
    GOBJ *falco = Fighter_GetGObj(1);
    FighterData *falco_data = falco->userdata;

    if (event_vars->game_timer == 1) {
        player_data->facing_direction = -1;
        PutOnGround(player);
        PutOnGround(falco);
        Match_CorrectCamera();

        event_vars->Savestate_Save_v1(event_vars->savestate, Savestate_Silent);
        Reset();
    }

    Fighter_ZeroCPUInputs(falco_data);
    falco_data->flags.no_reaction_always = true;
    falco_data->grab.vuln = 0x1FF;
    player_data->shield.health = 60;

    int new_direction = -1;
    if (player_data->input.down & HSD_BUTTON_DPAD_LEFT) {
        new_direction = DIRECTION_RIGHT;
    } else if (player_data->input.down & HSD_BUTTON_DPAD_RIGHT) {
        new_direction = DIRECTION_LEFT;
    }
    if (new_direction != -1) {
        Options_Main[OPT_DIRECTION].val = new_direction;
        Reset();
    }

    int state = falco_data->state_id;
    int state_frame = falco_data->TM.state_frame;
    
    if (ASID_DEADDOWN <= state && state <= ASID_REBIRTHWAIT)
        Reset();

    bool ground_actionable = (state == ASID_LANDING && state_frame > 4) || state == ASID_WAIT;
    bool in_turn = state == ASID_TURN;
    bool in_dash = state == ASID_DASH;
    bool in_jumpsquat = state == ASID_KNEEBEND;
    bool air_actionable = state == ASID_JUMPF || state == ASID_JUMPB || (state == ASID_KNEEBEND && state_frame == 5);
    bool can_fastfall = falco_data->phys.air_state == 1 && falco_data->phys.self_vel.Y <= 0.f;
    
    if (ground_actionable && falco_wait_delay > 0)
        falco_wait_delay--;

    if (air_actionable && falco_shoot_delay > 0)
        falco_shoot_delay--;

    if (can_fastfall && falco_fastfall_delay > 0)
        falco_fastfall_delay--;


    if (ground_actionable && falco_wait_delay == -1) {
        // set wait timer

        int delay_option = Options_Main[OPT_FIRE_SPEED].val;

        if (delay_option == FIRE_SPEED_RANDOM)
            falco_wait_delay = GetRandomLaserDelay();
        else if (delay_option == FIRE_SPEED_SLOW)
            falco_wait_delay = 20;
        else if (delay_option == FIRE_SPEED_MEDIUM)
            falco_wait_delay = 10;
        else if (delay_option == FIRE_SPEED_FAST)
            falco_wait_delay = 0;
    }

    if (ground_actionable && falco_wait_delay == 0) {
        // choose shoot type
        if (falco_shoot_direction == 0) {
            float falco_x = falco_data->phys.pos.X;
            float player_x = player_data->phys.pos.X;
        
            float approaching_dir = player_x < falco_x ? -1.f : 1.f;
            float retreating_dir = -approaching_dir;
            bool can_in_place = true;
            bool can_full_approach = fabs(falco_x + FULL_FALCO_SHORTHOP_DISTANCE*approaching_dir) < LEDGE_X;
            bool can_slight_approach = fabs(falco_x + SLIGHT_FALCO_SHORTHOP_DISTANCE*approaching_dir) < LEDGE_X;
            bool can_full_retreat = fabs(falco_x + FULL_FALCO_SHORTHOP_DISTANCE*retreating_dir) < LEDGE_X;
            bool can_slight_retreat = fabs(falco_x + SLIGHT_FALCO_SHORTHOP_DISTANCE*retreating_dir) < LEDGE_X;

            typedef struct {
                int in_place_odds;
                int full_approaching_odds;
                int slight_approaching_odds;
                int full_retreating_odds;
                int slight_retreating_odds;
            } ShootOdds;
            
            static ShootOdds odds_table[] = {
                { 1, 0, 0, 0, 0 }, // MOVEMENT_IN_PLACE
                { 4, 3, 3, 1, 3 }, // MOVEMENT_RANDOM
                { 2, 4, 3, 0, 2 }, // MOVEMENT_APPROACHING
                { 4, 0, 2, 2, 4 }, // MOVEMENT_RETREATING
            };

            int movement = Options_Main[OPT_MOVEMENT].val;
            ShootOdds *odds = &odds_table[movement];

            int odds_total = 0;
            if (can_in_place) odds_total += odds->in_place_odds;
            if (can_full_approach) odds_total += odds->full_approaching_odds;
            if (can_slight_approach) odds_total += odds->slight_approaching_odds;
            if (can_full_retreat) odds_total += odds->full_retreating_odds;
            if (can_slight_retreat) odds_total += odds->slight_retreating_odds;

            int dir_to_player = falco_data->phys.pos.X < player_data->phys.pos.X ? 1 : -1; 

            int rng = HSD_Randi(odds_total);
            if (can_in_place && (rng -= odds->in_place_odds) < 0) {
                falco_shoot_direction = dir_to_player;
                falco_dash_direction = 0;
                falco_jump_direction = 0;
            }
            else if (can_full_approach && (rng -= odds->full_approaching_odds) < 0) {
                if (fabs(falco_data->phys.pos.X - player_data->phys.pos.X) > FULL_FALCO_SHORTHOP_DISTANCE) {
                    falco_shoot_direction = dir_to_player;
                } else { 
                    falco_shoot_direction = -dir_to_player;
                }
                falco_dash_direction = dir_to_player;
                falco_jump_direction = dir_to_player;
            }
            else if (can_slight_approach && (rng -= odds->slight_approaching_odds) < 0) {
                if (fabs(falco_data->phys.pos.X - player_data->phys.pos.X) > SLIGHT_FALCO_SHORTHOP_DISTANCE) {
                    falco_shoot_direction = dir_to_player;
                } else { 
                    falco_shoot_direction = -dir_to_player;
                }
                falco_dash_direction = dir_to_player;
                falco_jump_direction = 0;
            }
            else if (can_full_retreat && (rng -= odds->full_retreating_odds) < 0) {
                falco_shoot_direction = dir_to_player;
                falco_dash_direction = -dir_to_player;
                falco_jump_direction = -dir_to_player;
            }
            else if (can_slight_retreat && (rng -= odds->slight_retreating_odds) < 0) {
                falco_shoot_direction = dir_to_player;
                falco_dash_direction = -dir_to_player;
                falco_jump_direction = 0;
            }
            else {
                assert("no shoot type available!");
            }
        }

        // start movement
        falco_wait_delay = -1;
        
        if (falco_dash_direction == 0 || state == ASID_DASH) {
            // start jump

            falco_data->cpu.held |= PAD_BUTTON_Y;
        } else {
            // start dash
    
            falco_data->cpu.lstickX = 127 * falco_dash_direction;
        }
    }
    
    if (in_turn) {
        // finish dashback

        falco_data->cpu.lstickX = 127 * falco_dash_direction;
    }
    
    if (in_dash) {
        // start jump

        falco_data->cpu.held |= PAD_BUTTON_Y;
    }
    
    if (in_jumpsquat) {
        // maintain jump momentum

        falco_data->cpu.lstickX = 127 * falco_jump_direction;
    }

    if (air_actionable && falco_shoot_delay == -1) {
        // set shoot timer

        int delay_option = Options_Main[OPT_LASER_HEIGHT].val;

        if (delay_option == LASER_HEIGHT_RANDOM) {
            falco_shoot_delay = HSD_Randi(4) + 2;
            falco_fastfall_delay = 1;
        } else if (delay_option == LASER_HEIGHT_VERY_LOW) {
            falco_shoot_delay = 11;
            falco_fastfall_delay = 8;
        } else if (delay_option == LASER_HEIGHT_LOW) {
            falco_shoot_delay = 5;
            falco_fastfall_delay = 1;
        } else if (delay_option == LASER_HEIGHT_MID) {
            falco_shoot_delay = 4;
            falco_fastfall_delay = 1;
        } else if (delay_option == LASER_HEIGHT_HIGH) {
            falco_shoot_delay = 2;
            falco_fastfall_delay = 0;
        }
    }
    
    if (air_actionable && falco_shoot_delay > 1) {
        // drift before shooting

        falco_data->cpu.lstickX = 127 * falco_jump_direction;
    }
    
    if (air_actionable && falco_shoot_delay == 1) {
        // set laser direction 1f before shooting

        falco_data->cpu.lstickX = 80 * falco_shoot_direction;
        falco_shoot_direction = 0;
    } 

    if (air_actionable && falco_shoot_delay == 0) {
        // start laser

        falco_shoot_delay = -1;
        falco_data->cpu.held |= PAD_BUTTON_B;
    }

    if (can_fastfall && falco_fastfall_delay == 0) {
        falco_data->cpu.lstickY = -127;
        falco_fastfall_delay = -1;
    }
}

void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

void ChangeFireSpeedOption(GOBJ *event_menu, int value) {
    bool disable_random_bounds = value != FIRE_SPEED_RANDOM;
    Options_Main[OPT_FIRE_DELAY_RANDOM_MIN].disable = disable_random_bounds;
    Options_Main[OPT_FIRE_DELAY_RANDOM_MAX].disable = disable_random_bounds;
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

void Reset(void) {
    int direction = Options_Main[OPT_DIRECTION].val;
    int mirror = direction == DIRECTION_LEFT ? Savestate_Mirror : 0;
    event_vars->Savestate_Load(&event_vars->savestate->header, Savestate_Silent | mirror);

    falco_wait_delay = -1;
    falco_shoot_delay = -1;
    falco_fastfall_delay = -1;
    falco_dash_direction = 0;
    falco_jump_direction = 0;
    falco_shoot_direction = 0;
    
    GOBJ *player = Fighter_GetGObj(0);
    GOBJ *falco = Fighter_GetGObj(1);
    
    PutOnGround(player);
    PutOnGround(falco);
    Match_CorrectCamera();
}

void ChangeDirection(GOBJ *event_menu, int value) {
    Reset();
}

void ChangeRandomFireDelayMin(GOBJ *event_menu, int value) {
    int random_fire_delay_min = Options_Main[OPT_FIRE_DELAY_RANDOM_MIN].val;
    int random_fire_delay_max = Options_Main[OPT_FIRE_DELAY_RANDOM_MAX].val;

    // Ensure the min is never greater than the max
    if (random_fire_delay_min > random_fire_delay_max) {
        Options_Main[OPT_FIRE_DELAY_RANDOM_MAX].val = random_fire_delay_min;
    }
}

void ChangeRandomFireDelayMax(GOBJ *event_menu, int value) {
    int random_fire_delay_min = Options_Main[OPT_FIRE_DELAY_RANDOM_MIN].val;
    int random_fire_delay_max = Options_Main[OPT_FIRE_DELAY_RANDOM_MAX].val;

    // Ensure the max is never less than the min
    if (random_fire_delay_max < random_fire_delay_min) {
        Options_Main[OPT_FIRE_DELAY_RANDOM_MIN].val = random_fire_delay_max;
    }
}

int GetRandomLaserDelay(void) {
    int random_fire_delay_min = Options_Main[OPT_FIRE_DELAY_RANDOM_MIN].val;
    int random_fire_delay_max = Options_Main[OPT_FIRE_DELAY_RANDOM_MAX].val;

    return HSD_Randi(random_fire_delay_max - random_fire_delay_min) + random_fire_delay_min;
}

EventMenu *Event_Menu = &Menu_Main;
