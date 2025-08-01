#include "../MexTK/mex.h"
#include "events.h"

static void Exit(GOBJ *menu);
static void Reset(void);

static void Think_Spacies(void);
static void Think_Sheik(void);
static void Think_Falcon(void);
static void Think_Marth(void);
static void ChangePlayerPercent(GOBJ *menu_gobj, int value);
static void ChangeCustomKB_MagMin(GOBJ *menu_gobj, int value);
static void ChangeCustomKB_MagMax(GOBJ *menu_gobj, int value);
static void ChangeCustomKB_AngMin(GOBJ *menu_gobj, int value);
static void ChangeCustomKB_AngMax(GOBJ *menu_gobj, int value);
static void ChangeCustomKB_DmgMin(GOBJ *menu_gobj, int value);
static void ChangeCustomKB_DmgMax(GOBJ *menu_gobj, int value);

static EventMenu Menu_Main;
static EventMenu Menu_CustomHitStrength;

// Common values -------------------------------------------------

typedef struct KBValues {
    float mag_min, mag_max;
    float ang_min, ang_max;
    float dmg_min, dmg_max;
} KBValues;

enum options_kbvals {
    KBVALS_WEAK,
    KBVALS_NORMAL,
    KBVALS_HARD,
    KBVALS_SHALLOW,
    KBVALS_CUSTOM,
    
    KBVALS_COUNT
};

#define CUSTOMKB_INIT_MAGMIN 100
#define CUSTOMKB_INIT_MAGMAX 120
#define CUSTOMKB_INIT_ANGMIN 45
#define CUSTOMKB_INIT_ANGMAX 60
#define CUSTOMKB_INIT_DMGMIN 50
#define CUSTOMKB_INIT_DMGMAX 80

static const char *Values_HitStrength[] = {"Weak", "Normal", "Hard", "Shallow", "Custom"};
static KBValues HitStrength_KBRange[] = {
    {
         80.f, 100.f, // mag
         45.f,  60.f, // ang
         10.f,  40.f, // dmg
    },
    {
        106.f, 120.f, // mag
         45.f,  60.f, // ang
         50.f,  80.f, // dmg
    },
    {
        140.f, 200.f, // mag
         65.f,  70.f, // ang
         80.f, 110.f, // dmg
    },
    {
        110.f, 130.f, // mag
         20.f,  40.f, // ang
         60.f,  80.f, // dmg
    },
    {
        CUSTOMKB_INIT_MAGMIN, CUSTOMKB_INIT_MAGMAX,
        CUSTOMKB_INIT_ANGMIN, CUSTOMKB_INIT_ANGMAX,
        CUSTOMKB_INIT_DMGMIN, CUSTOMKB_INIT_DMGMAX,
    },
};

enum options_initial {
    OPT_MAIN_RECOVERY,
    OPT_MAIN_PERCENT,
    OPT_MAIN_HITSTRENGTH,
    OPT_MAIN_HITSTRENGTH_CUSTOMMENU,
    OPT_MAIN_EXIT,
    
    OPT_MAIN_COUNT
};

static EventOption Options_Main[] = {
    {
        .kind = OPTKIND_MENU,
        .menu = 0, // Set in Event_Init
        .name = "Recovery Options",
        .desc = {"Alter the CPU's recovery options."},
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 999,
        .name = "Player Percent",
        .desc = {"Adjust the player's percent."},
        .format = "%d%%",
        .OnChange = ChangePlayerPercent,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "Hit Strength",
        .desc = {"How far the CPU will be knocked back."},
        .values = Values_HitStrength,
        .value_num = countof(Values_HitStrength),
        .val = 1,
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &Menu_CustomHitStrength,
        .name = "Customize Hit Strength",
        .desc = {"Manually adjust where and how far the CPU",
                 "will be knocked back."},
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Select Screen."},
        .OnSelect = Exit,
    }};

static EventMenu Menu_Main = {
    .name = "Edgeguard",
    .option_num = sizeof(Options_Main) / sizeof(EventOption),
    .options = Options_Main,
};

// Custom Hit Strength ---------------------------------

enum options_custom_hit_strength {
    OPT_CUSTOMHIT_MAGMIN,
    OPT_CUSTOMHIT_MAGMAX,
    OPT_CUSTOMHIT_ANGMIN,
    OPT_CUSTOMHIT_ANGMAX,
    OPT_CUSTOMHIT_DMGMIN,
    OPT_CUSTOMHIT_DMGMAX,
    
    OPT_CUSTOMHIT_COUNT
};

static EventOption Options_CustomHitStrength[] = {
    {
        .kind = OPTKIND_INT,
        .value_num = 301,
        .val = CUSTOMKB_INIT_MAGMIN,
        .name = "Knockback Min",
        .desc = {"Adjust the minimum of how far the CPU will fly."},
        .format = "%d",
        .OnChange = ChangeCustomKB_MagMin,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 301,
        .val = CUSTOMKB_INIT_MAGMAX,
        .name = "Knockback Max",
        .desc = {"Adjust the maximum of how far the CPU will fly."},
        .format = "%d",
        .OnChange = ChangeCustomKB_MagMax,
    },
    
    {
        .kind = OPTKIND_INT,
        .value_num = 91,
        .val = CUSTOMKB_INIT_ANGMIN,
        .name = "Angle Min",
        .desc = {"Adjust the minimum angle the CPU will fly."},
        .format = "%d",
        .OnChange = ChangeCustomKB_AngMin,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 91,
        .val = CUSTOMKB_INIT_ANGMAX,
        .name = "Angle Max",
        .desc = {"Adjust the maximum angle the CPU will fly."},
        .format = "%d",
        .OnChange = ChangeCustomKB_AngMax,
    },
    
    {
        .kind = OPTKIND_INT,
        .value_num = 1000,
        .val = CUSTOMKB_INIT_DMGMIN,
        .name = "Percent Min",
        .desc = {"Adjust the CPU's minimum percent."},
        .format = "%d%",
        .OnChange = ChangeCustomKB_DmgMin,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 1000,
        .val = CUSTOMKB_INIT_DMGMAX,
        .name = "Percent Max",
        .desc = {"Adjust the CPU's maximum percent."},
        .format = "%d%",
        .OnChange = ChangeCustomKB_DmgMax,
    },
};

static EventMenu Menu_CustomHitStrength = {
    .name = "Custom Hit Strength",
    .option_num = sizeof(Options_CustomHitStrength) / sizeof(EventOption),
    .options = Options_CustomHitStrength,
};

// Fox -------------------------------------------------

enum options_spacies {
    OPT_SPACIES_FF_LOW,
    OPT_SPACIES_FF_MID,
    OPT_SPACIES_FF_HIGH,
    OPT_SPACIES_JUMP,
    OPT_SPACIES_ILLUSION,
    OPT_SPACIES_FASTFALL,
};

static EventOption Options_Fox[] = {
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
static EventMenu Menu_Fox = {
    .name = "Fox Recovery",
    .option_num = sizeof(Options_Fox) / sizeof(EventOption),
    .options = Options_Fox,
};

// Falco -------------------------------------------------

static EventOption Options_Falco[] = {
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
static EventMenu Menu_Falco = {
    .name = "Falco Recovery",
    .option_num = sizeof(Options_Falco) / sizeof(EventOption),
    .options = Options_Falco,
};

// Sheik -------------------------------------------------

enum options_sheik {
    OPT_SHEIK_UPB_LEDGE,
    OPT_SHEIK_UPB_STAGE,
    OPT_SHEIK_UPB_HIGH,
    OPT_SHEIK_JUMP,
    OPT_SHEIK_FAIR,
    OPT_SHEIK_AMSAH_TECH,
    
    OPT_SHEIK_COUNT
};

static EventOption Options_Sheik[] = {
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

static EventMenu Menu_Sheik = {
    .name = "Sheik Recovery",
    .option_num = sizeof(Options_Sheik) / sizeof(EventOption),
    .options = Options_Sheik,
};

// Captain Falcon --------------------------------------

enum options_falcon {
    OPT_FALCON_UPB,
    OPT_FALCON_DRIFT_BACK,
    OPT_FALCON_JUMP,
    OPT_FALCON_FASTFALL,
    OPT_FALCON_DOWNB,

    OPT_FALCON_COUNT
};

static EventOption Options_Falcon[] = {
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

static EventMenu Menu_Falcon = {
    .name = "Falcon Recovery",
    .option_num = sizeof(Options_Falcon) / sizeof(EventOption),
    .options = Options_Falcon,
};

// Marth --------------------------------------

enum options_marth {
    OPT_MARTH_UPB_EARLY,
    OPT_MARTH_JUMP,
    OPT_MARTH_SIDEB,
    OPT_MARTH_FAIR,
    OPT_MARTH_COUNT
};

static EventOption Options_Marth[] = {
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

static EventMenu Menu_Marth = {
    .name = "Marth Recovery",
    .option_num = sizeof(Options_Marth) / sizeof(EventOption),
    .options = Options_Marth,
};

// Info lookup -----------------------------------------

typedef struct EdgeguardInfo {
    EventMenu *recovery_menu;
    void (*Think)(void);
} EdgeguardInfo;

static EdgeguardInfo InfoLookup[] = {
    {0}, // MARIO
    {
        .recovery_menu = &Menu_Fox,
        .Think = Think_Spacies,
    },
    {
        .recovery_menu = &Menu_Falcon,
        .Think = Think_Falcon,
    },
    {0}, // DK
    {0}, // KIRBY
    {0}, // BOWSER
    {0}, // LINK
    {
        .recovery_menu = &Menu_Sheik,
        .Think = Think_Sheik,
    },
    {0}, // NESS
    {0}, // PEACH
    {0}, // POPO
    {0}, // NANA
    {0}, // PIKACHU
    {0}, // SAMUS
    {0}, // YOSHI
    {0}, // JIGGLYPUFF
    {0}, // MEWTWO
    {0}, // LUIGI
    {
        .recovery_menu = &Menu_Marth,
        .Think = Think_Marth,
    },
    {0}, // ZELDA
    {0}, // YOUNGLINK
    {0}, // DRMARIO
    {
        .recovery_menu = &Menu_Falco,
        .Think = Think_Spacies,
    },
    {0}, // PICHU
    {0}, // GAW,
    {0}, // GANONDORF
    {0}, // ROY
};
