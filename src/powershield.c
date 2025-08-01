#include "../MexTK/mex.h"
#include "events.h"

#define CPU_LEFT_STAGE_POS_X 70.f
#define CPU_LEFT_DIRECTION -1.f

void Exit(GOBJ *menu);
void ChangeFireSpeedOption(GOBJ *event_menu, int value);
void ChangeDirection(GOBJ *event_menu, int value);
void ChangeRandomFireDelayMin(GOBJ *event_menu, int value);
void ChangeRandomFireDelayMax(GOBJ *event_menu, int value);
int GetRandomLaserDelay(void);

enum menu_options {
    OPT_FIRE_SPEED,
    OPT_FIRE_DELAY_RANDOM_MIN,
    OPT_FIRE_DELAY_RANDOM_MAX,
    OPT_LASER_HEIGHT,
    OPT_DIRECTION,
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

static const char *Options_FireSpeed[] = { "Random", "Slow", "Medium", "Fast" };
static const char *Options_LaserHeight[] = { "Random", "Very Low", "Low", "Mid", "High" };
static const char *Options_Direction[] = { "Right", "Left" };

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

void Event_Think(GOBJ *menu) {
    GOBJ *player = Fighter_GetGObj(0);
    FighterData *player_data = player->userdata;
    GOBJ *falco = Fighter_GetGObj(1);
    FighterData *falco_data = falco->userdata;

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
        ChangeDirection(menu, new_direction);
    }

    int state = falco_data->state_id;
    int state_frame = falco_data->TM.state_frame;

    bool ground_actionable = (state == ASID_LANDING && state_frame > 4) || state == ASID_WAIT;
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
        // start jump

        falco_wait_delay = -1;
        falco_data->cpu.held |= PAD_BUTTON_Y;
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

void ChangeDirection(GOBJ *event_menu, int value) {
    GOBJ *falco = Fighter_GetGObj(1);
    FighterData *falco_data = falco->userdata;

    if (value == DIRECTION_LEFT) {
        falco_data->facing_direction = CPU_LEFT_DIRECTION;
        falco_data->phys.pos.X = CPU_LEFT_STAGE_POS_X;
    } else {
        falco_data->facing_direction = -CPU_LEFT_DIRECTION;
        falco_data->phys.pos.X = -CPU_LEFT_STAGE_POS_X;    
    }
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
