#include "lab_common.h"

// DECLARATIONS #############################################

// todo: move structs from lab_common.h to here

static ShortcutList Lab_ShortcutList;
static EventMenu LabMenu_General;
static EventMenu LabMenu_OverlaysHMN;
static EventMenu LabMenu_OverlaysCPU;
static EventMenu LabMenu_InfoDisplayHMN;
static EventMenu LabMenu_InfoDisplayCPU;
static EventMenu LabMenu_CharacterRng;
static EventMenu LabMenu_CPU;
static EventMenu LabMenu_AdvCounter;
static EventMenu LabMenu_Record;
static EventMenu LabMenu_Tech;
static EventMenu LabMenu_Stage_FOD;
static EventMenu LabMenu_CustomOSDs;
static EventMenu LabMenu_SlotManagement;
static EventMenu LabMenu_AlterInputs;
static EventMenu LabMenu_OSDs;

#define AUTORESTORE_DELAY 20
#define INTANG_COLANIM 10
#define STICK_DEADZONE 0.2
#define TRIGGER_DEADZONE 0.2

#define CPUMASHRNG_MED 35
#define CPUMASHRNG_HIGH 55

typedef struct CPUAction {
    u16 state;                  // state to perform this action. -1 for last
    u8 frameLow;                // first possible frame to perform this action
    u8 frameHi;                 // last possible frame to perfrom this action
    s8 stickX;                  // left stick X value
    s8 stickY;                  // left stick Y value
    s8 cstickX;                 // c stick X value
    s8 cstickY;                 // c stick Y value
    int input;                  // button to input
    u16 isLast        : 1; // flag to indicate this was the final input
    u16 stickDir      : 3; // 0 = none, 1 = towards opponent, 2 = away from opponent, 3 = forward, 4 = backward
    u16 recSlot       : 4; // 0 = none, 1 = slot 1, ..., 6 = slot 6, -1 = random
    u16 noActAfter    : 1; // 0 = goto CPUSTATE_RECOVER, 1 = goto CPUSTATE_NONE
    bool (*custom_check)(GOBJ *);
} CPUAction;

typedef struct CustomTDI {
    float lstickX;
    float lstickY;
    float cstickX;
    float cstickY;
    u32 reversing: 1;
    u32 direction: 1; // 0 = left of player, 1 = right of player
} CustomTDI;

enum stick_dir
{
    STCKDIR_NONE,
    STCKDIR_TOWARD,
    STCKDIR_AWAY,
    STCKDIR_FRONT,
    STCKDIR_BACK,
    STICKDIR_RDM,
};

enum hit_kind
{
    HITKIND_DAMAGE,
    HITKIND_SHIELD,
};

enum custom_asid_groups
{
    ASID_ACTIONABLE = 1000,
    ASID_ACTIONABLEGROUND,
    ASID_ACTIONABLEAIR,
    ASID_DAMAGEAIR,
    ASID_CROUCH,
    ASID_ANY,
};

// FUNCTION PROTOTYPES ##############################################################

static u32 lz77Compress(u8 *uncompressed_text, u32 uncompressed_size, u8 *compressed_text, u8 pointer_length_width);
static u32 lz77Decompress(u8 *compressed_text, u8 *uncompressed_text);
static float GetAngleOutOfDeadzone(float angle, int lastSDIWasCardinal);
static void DistributeChances(s16 *chances[], unsigned int chance_count);
static void ReboundChances(s16 *chances[], unsigned int chance_count, int just_changed_option);
static int IsTechAnim(int state);
static bool CanWalljump(GOBJ* fighter);
static int GetCurrentStateName(GOBJ *fighter, char *buf);
static bool CheckHasJump(GOBJ *g);
static int InHitstunAnim(int state);
static int IsHitlagVictim(GOBJ *character);
static int InShieldStun(int state);
int CustomTDI_Update(GOBJ *gobj);
void CustomTDI_Destroy(GOBJ *gobj);
void CustomTDI_Apply(GOBJ *cpu, GOBJ *hmn, CustomTDI *di);
void CPUResetVars(void);
void Lab_ChangeAdvCounterHitNumber(GOBJ *menu_gobj, int value);
void Lab_ChangeAdvCounterLogic(GOBJ *menu_gobj, int value);
void Lab_ChangeInputs(GOBJ *menu_gobj, int value);
void Lab_ChangeAlterInputsFrame(GOBJ *menu_gobj, int value);
int Lab_SetAlterInputsMenuOptions(GOBJ *menu_gobj);

// ACTIONS #################################################

#define ActionEnd { .state = -1, }

// CPU Action Definitions
static CPUAction Lab_CPUActionShield[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
    },
    {
        .state     = ASID_GUARDREFLECT,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
    },
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
    },
    ActionEnd,
};
static CPUAction Lab_CPUActionGrab[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_BUTTON_A | PAD_TRIGGER_R,
        .isLast    = 1,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_TRIGGER_Z,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionUpB[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_GUARD,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_KNEEBEND,
        .stickY    = 127,
        .input     = PAD_BUTTON_B,
        .isLast    = 1,
    },
    {
        .state     = ASID_ACTIONABLE,
        .stickY    = 127,
        .input     = PAD_BUTTON_B,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionSideBToward[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_GUARD,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLE,
        .stickX    = 127,
        .input     = PAD_BUTTON_B,
        .isLast    = 1,
        .stickDir  = STCKDIR_TOWARD,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionSideBAway[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_GUARD,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLE,
        .stickX    = 127,
        .input     = PAD_BUTTON_B,
        .isLast    = 1,
        .stickDir  = STCKDIR_AWAY,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionDownB[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_GUARD,
        .input     = PAD_BUTTON_X,
        .isLast    = 1,
    },
    {
        .state     = ASID_ACTIONABLE,
        .stickY    = -127,
        .input     = PAD_BUTTON_B,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionNeutralB[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_GUARD,
        .input     = PAD_BUTTON_X,
        .isLast    = 1,
    },

    {
        .state     = ASID_ACTIONABLE,
        .input     = PAD_BUTTON_B,
        .isLast    = 1,
    },
    ActionEnd
};

// We buffer this for a single frame.
// For some reason spotdodge is only possible frame 2 when floorhugging
// an attack that would have otherwise knocked you into the air without knockdown.
// This doesn't occur with rolls for some reason.
static CPUAction Lab_CPUActionSpotdodge[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .stickY    = -127,
        .input     = PAD_TRIGGER_R,
    },
    {
        .state     = ASID_GUARDREFLECT,
        .stickY    = -127,
        .input     = PAD_TRIGGER_R,
    },
    {
        .state     = ASID_ESCAPE,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionRollAway[] = {
    {
        .state     = ASID_GUARD,
        .stickX    = 127,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
        .stickDir  = STCKDIR_AWAY,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_TRIGGER_R,
    },
    {
        .state     = ASID_GUARDREFLECT,
        .stickX    = 127,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
        .stickDir  = STCKDIR_AWAY,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionRollTowards[] = {
    {
        .state     = ASID_GUARD,
        .stickX    = 127,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
        .stickDir  = STCKDIR_TOWARD,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_TRIGGER_R,
    },
    {
        .state     = ASID_GUARDREFLECT,
        .stickX    = 127,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
        .stickDir  = STCKDIR_TOWARD,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionRollRandom[] = {
    {
        .state     = ASID_GUARD,
        .stickX    = 127,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
        .stickDir  = STICKDIR_RDM,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_TRIGGER_R,
    },
    {
        .state     = ASID_GUARDREFLECT,
        .stickX    = 127,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
        .stickDir  = STICKDIR_RDM,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionNair[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .input     = PAD_BUTTON_A,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionFair[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .cstickX   = 127,
        .isLast    = 1,
        .stickDir  = 3,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionDair[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .cstickY   = -127,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionBair[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .cstickX   = 127,
        .isLast    = 1,
        .stickDir  = 4,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionUair[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .cstickY   = 127,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionJump[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
        .isLast    = 1,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionJumpFull[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_KNEEBEND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionJumpAway[] = {
    {
        .state     = ASID_ACTIONABLEAIR,
        .stickX    = 127,
        .input     = PAD_BUTTON_X,
        .isLast    = 1,
        .stickDir  = STCKDIR_AWAY,
    },

    ActionEnd
};
static CPUAction Lab_CPUActionJumpTowards[] = {
    {
        .state     = ASID_ACTIONABLEAIR,
        .stickX    = 127,
        .input     = PAD_BUTTON_X,
        .isLast    = 1,
        .stickDir  = STCKDIR_TOWARD,
    },

    ActionEnd
};
static CPUAction Lab_CPUActionJumpNeutral[] = {
    {
        .state     = ASID_ACTIONABLEAIR,
        .input     = PAD_BUTTON_X,
        .isLast    = 1,
        .custom_check = CheckHasJump,
    },

    // wiggle out if we can't jump
    {
        .state     = ASID_DAMAGEAIR,
        .stickX    = 127,
        .isLast    = 1,
    },

    ActionEnd
};
static CPUAction Lab_CPUActionAirdodge[] = {
    // wiggle out if we are in tumble
    {
        .state     = ASID_DAMAGEAIR,
        .stickX    = 127,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .input     = PAD_TRIGGER_R,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionFFTumble[] = {
    {
        .state      = ASID_DAMAGEAIR,
        .stickY     = -127,
        .isLast     = 1,
        .noActAfter = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionFFWiggle[] = {
    {
        .state     = ASID_DAMAGEAIR,
        .stickX    = 127,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .stickY    = -127,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionJab[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_A,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionFTilt[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .stickX    = 80,
        .input     = PAD_BUTTON_A,
        .isLast    = 1,
        .stickDir  = STCKDIR_TOWARD,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionUTilt[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .stickY    = 80,
        .input     = PAD_BUTTON_A,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionDTilt[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .stickY    = -80,
        .input     = PAD_BUTTON_A,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionUSmash[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .cstickY   = 127,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionDSmash[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .cstickY   = -127,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionFSmash[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .cstickX   = 127,
        .isLast    = 1,
        .stickDir  = STCKDIR_TOWARD,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionUpSmashOOS[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_KNEEBEND,
        .cstickY   = 127,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionWavedashAway[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .input     = PAD_TRIGGER_L,
        .stickY    = -45,
        .stickX    = 65,
        .stickDir  = STCKDIR_AWAY,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionWavedashTowards[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .input     = PAD_TRIGGER_L,
        .stickY    = -45,
        .stickX    = 64,
        .stickDir  = STCKDIR_TOWARD,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionWavedashDown[] = {
    {
        .state     = ASID_GUARD,
        .input     = PAD_TRIGGER_R | PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEGROUND,
        .input     = PAD_BUTTON_X,
    },
    {
        .state     = ASID_ACTIONABLEAIR,
        .input     = PAD_TRIGGER_L,
        .stickY    = -80,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionDashAway[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .stickX    = 127,
        .stickDir  = STCKDIR_AWAY,
    },
    {
        .state     = ASID_TURN,
        .stickX    = 127,
        .stickDir  = STCKDIR_AWAY,
    },
    {
        .state     = ASID_DASH,
        .stickX    = 127,
        .stickDir  = STCKDIR_AWAY,
        .isLast    = 1,
    },
    ActionEnd
};
static CPUAction Lab_CPUActionDashTowards[] = {
    {
        .state     = ASID_ACTIONABLEGROUND,
        .stickX    = 127,
        .stickDir  = STCKDIR_TOWARD,
    },
    {
        .state     = ASID_TURN,
        .stickX    = 127,
        .stickDir  = STCKDIR_TOWARD,
    },
    {
        .state     = ASID_DASH,
        .stickX    = 127,
        .stickDir  = STCKDIR_TOWARD,
        .isLast    = 1,
    },
    ActionEnd
};

#define RECSLOT_RANDOM 15
static CPUAction Lab_CPUActionSlot1[] = { { .recSlot = 1 }, ActionEnd, };
static CPUAction Lab_CPUActionSlot2[] = { { .recSlot = 2 }, ActionEnd, };
static CPUAction Lab_CPUActionSlot3[] = { { .recSlot = 3 }, ActionEnd, };
static CPUAction Lab_CPUActionSlot4[] = { { .recSlot = 4 }, ActionEnd, };
static CPUAction Lab_CPUActionSlot5[] = { { .recSlot = 5 }, ActionEnd, };
static CPUAction Lab_CPUActionSlot6[] = { { .recSlot = 6 }, ActionEnd, };
static CPUAction Lab_CPUActionSlotRandom[] = { { .recSlot = RECSLOT_RANDOM }, ActionEnd, };

static CPUAction *Lab_CPUActions[] = {
    0,
    Lab_CPUActionShield,
    Lab_CPUActionGrab,
    Lab_CPUActionUpB,
    Lab_CPUActionSideBToward,
    Lab_CPUActionSideBAway,
    Lab_CPUActionDownB,
    Lab_CPUActionNeutralB,
    Lab_CPUActionSpotdodge,
    Lab_CPUActionRollAway,
    Lab_CPUActionRollTowards,
    Lab_CPUActionRollRandom,
    Lab_CPUActionNair,
    Lab_CPUActionFair,
    Lab_CPUActionDair,
    Lab_CPUActionBair,
    Lab_CPUActionUair,
    Lab_CPUActionJump,
    Lab_CPUActionJumpFull,
    Lab_CPUActionJumpAway,
    Lab_CPUActionJumpTowards,
    Lab_CPUActionJumpNeutral,
    Lab_CPUActionAirdodge,
    Lab_CPUActionFFTumble,
    Lab_CPUActionFFWiggle,
    Lab_CPUActionJab,
    Lab_CPUActionFTilt,
    Lab_CPUActionUTilt,
    Lab_CPUActionDTilt,
    Lab_CPUActionUSmash,
    Lab_CPUActionDSmash,
    Lab_CPUActionFSmash,
    Lab_CPUActionUpSmashOOS,
    Lab_CPUActionWavedashAway,
    Lab_CPUActionWavedashTowards,
    Lab_CPUActionWavedashDown,
    Lab_CPUActionDashAway,
    Lab_CPUActionDashTowards,
    Lab_CPUActionSlot1,
    Lab_CPUActionSlot2,
    Lab_CPUActionSlot3,
    Lab_CPUActionSlot4,
    Lab_CPUActionSlot5,
    Lab_CPUActionSlot6,
    Lab_CPUActionSlotRandom
};

enum CPU_ACTIONS
{
    CPUACT_NONE,
    CPUACT_SHIELD,
    CPUACT_GRAB,
    CPUACT_UPB,
    CPUACT_SIDEBTOWARD,
    CPUACT_SIDEBAWAY,
    CPUACT_DOWNB,
    CPUACT_NEUTRALB,
    CPUACT_SPOTDODGE,
    CPUACT_ROLLAWAY,
    CPUACT_ROLLTOWARDS,
    CPUACT_ROLLRDM,
    CPUACT_NAIR,
    CPUACT_FAIR,
    CPUACT_DAIR,
    CPUACT_BAIR,
    CPUACT_UAIR,
    CPUACT_SHORTHOP,
    CPUACT_FULLHOP,
    CPUACT_JUMPAWAY,
    CPUACT_JUMPTOWARDS,
    CPUACT_JUMPNEUTRAL,
    CPUACT_AIRDODGE,
    CPUACT_FFTUMBLE,
    CPUACT_FFWIGGLE,
    CPUACT_JAB,
    CPUACT_FTILT,
    CPUACT_UTILT,
    CPUACT_DTILT,
    CPUACT_USMASH,
    CPUACT_DSMASH,
    CPUACT_FSMASH,
    CPUACT_USMASHOOS,
    CPUACT_WAVEDASH_AWAY,
    CPUACT_WAVEDASH_TOWARDS,
    CPUACT_WAVEDASH_DOWN,
    CPUACT_DASH_AWAY,
    CPUACT_DASH_TOWARDS,
    CPUACT_SLOT1,
    CPUACT_SLOT2,
    CPUACT_SLOT3,
    CPUACT_SLOT4,
    CPUACT_SLOT5,
    CPUACT_SLOT6,
    CPUACT_SLOT_RANDOM,

    CPUACT_COUNT
};

#define SLOT_ACTIONS CPUACT_SLOT1, CPUACT_SLOT2, CPUACT_SLOT3, CPUACT_SLOT4, CPUACT_SLOT5, CPUACT_SLOT6, CPUACT_SLOT_RANDOM
#define SLOT_NAMES "Play Slot 1", "Play Slot 2", "Play Slot 3", "Play Slot 4", "Play Slot 5", "Play Slot 6", "Play Random Slot"

static u8 CPUCounterActionsGround[] = {CPUACT_NONE, CPUACT_SPOTDODGE, CPUACT_SHIELD, CPUACT_GRAB, CPUACT_UPB, CPUACT_SIDEBTOWARD, CPUACT_SIDEBAWAY, CPUACT_DOWNB, CPUACT_NEUTRALB, CPUACT_USMASH, CPUACT_DSMASH, CPUACT_FSMASH, CPUACT_ROLLAWAY, CPUACT_ROLLTOWARDS, CPUACT_ROLLRDM, CPUACT_NAIR, CPUACT_FAIR, CPUACT_DAIR, CPUACT_BAIR, CPUACT_UAIR, CPUACT_JAB, CPUACT_FTILT, CPUACT_UTILT, CPUACT_DTILT, CPUACT_SHORTHOP, CPUACT_FULLHOP, CPUACT_WAVEDASH_AWAY, CPUACT_WAVEDASH_TOWARDS, CPUACT_WAVEDASH_DOWN, CPUACT_DASH_AWAY, CPUACT_DASH_TOWARDS, SLOT_ACTIONS};

static u8 CPUCounterActionsAir[] = {CPUACT_NONE, CPUACT_AIRDODGE, CPUACT_JUMPAWAY, CPUACT_JUMPTOWARDS, CPUACT_JUMPNEUTRAL, CPUACT_UPB, CPUACT_SIDEBTOWARD, CPUACT_SIDEBAWAY, CPUACT_DOWNB, CPUACT_NEUTRALB, CPUACT_NAIR, CPUACT_FAIR, CPUACT_DAIR, CPUACT_BAIR, CPUACT_UAIR, CPUACT_FFTUMBLE, CPUACT_FFWIGGLE, SLOT_ACTIONS};

static u8 CPUCounterActionsShield[] = {CPUACT_NONE, CPUACT_GRAB, CPUACT_SHORTHOP, CPUACT_FULLHOP, CPUACT_SPOTDODGE, CPUACT_ROLLAWAY, CPUACT_ROLLTOWARDS, CPUACT_ROLLRDM, CPUACT_USMASHOOS, CPUACT_UPB, CPUACT_DOWNB, CPUACT_NAIR, CPUACT_FAIR, CPUACT_DAIR, CPUACT_BAIR, CPUACT_UAIR, CPUACT_WAVEDASH_AWAY, CPUACT_WAVEDASH_TOWARDS, CPUACT_WAVEDASH_DOWN, SLOT_ACTIONS};

static const char *LabValues_CounterGround[] = {"None", "Spotdodge", "Shield", "Grab", "Up B", "Side B Toward", "Side B Away", "Down B", "Neutral B", "Up Smash", "Down Smash", "Forward Smash", "Roll Away", "Roll Towards", "Roll Random", "Neutral Air", "Forward Air", "Down Air", "Back Air", "Up Air", "Jab", "Forward Tilt", "Up Tilt", "Down Tilt", "Short Hop", "Full Hop", "Wavedash Away", "Wavedash Towards", "Wavedash Down", "Dash Back", "Dash Through", SLOT_NAMES};
static const char *LabValues_CounterAir[] = {"None", "Airdodge", "Jump Away", "Jump Towards", "Jump Neutral", "Up B", "Side B Toward", "Side B Away", "Down B", "Neutral B", "Neutral Air", "Forward Air", "Down Air", "Back Air", "Up Air", "Tumble Fastfall", "Wiggle Fastfall", SLOT_NAMES};
static const char *LabValues_CounterShield[] = {"None", "Grab", "Short Hop", "Full Hop", "Spotdodge", "Roll Away", "Roll Towards", "Roll Random", "Up Smash", "Up B", "Down B", "Neutral Air", "Forward Air", "Down Air", "Back Air", "Up Air", "Wavedash Away", "Wavedash Towards", "Wavedash Down", SLOT_NAMES};

// MENUS ###################################################

// MAIN MENU --------------------------------------------------------------

enum lab_option
{
    OPTLAB_GENERAL_OPTIONS,
    OPTLAB_CPU_OPTIONS,
    OPTLAB_RECORD_OPTIONS,
    OPTLAB_INFODISP_HMN,
    OPTLAB_INFODISP_CPU,
    OPTLAB_CHAR_RNG,
    OPTLAB_STAGE,
    OPTLAB_HELP,
    OPTLAB_EXIT,

    OPTLAB_COUNT
};

static const char *LabOptions_CheckBox[] = {"", "X"};

static EventOption LabOptions_Main[OPTLAB_COUNT] = {
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_General,
        .name = "General",
        .desc = {"Toggle player percent, overlays,",
                 "frame advance, and camera settings."},
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_CPU,
        .name = "CPU Options",
        .desc = {"Configure CPU behavior."},
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_Record,
        .name = "Recording",
        .desc = {"Record and playback inputs."},
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_InfoDisplayHMN,
        .name = "HMN Info Display",
        .desc = {"Display various game information onscreen."},
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_InfoDisplayCPU,
        .name = "CPU Info Display",
        .desc = {"Display various game information onscreen."},
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_CharacterRng,
        .disable = 1,
        //.disable is set in Event_Init depending on fighters
        .name = "Character RNG",
        .desc = {"Change RNG behavior of Peach, Luigi, GnW, and Icies."},
    },
    {
        .kind = OPTKIND_MENU,
        .disable = 1,
        //.menu and .disable are set in Event_Init depending on stage
        .name = "Stage Options",
        .desc = {"Configure stage behavior."},
    },
    {
        .kind = OPTKIND_INFO,
        .name = "Help",
        .desc = {"D-Pad Left - Load State",
                 "D-Pad Right - Save State",
                 "D-Pad Down - Move CPU",
                 "Hold R in the menu for turbo."},
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = {"Return to the Event Select Screen."},
        .OnSelect = Lab_Exit,
    },
};

static EventMenu LabMenu_Main = {
    .name = "Main Menu",
    .option_num = sizeof(LabOptions_Main) / sizeof(EventOption),
    .options = LabOptions_Main,
    .shortcuts = &Lab_ShortcutList,
};

// GENERAL MENU --------------------------------------------------------------

enum input_display_mode {
    INPUTDISPLAY_OFF,
    INPUTDISPLAY_HMN,
    INPUTDISPLAY_CPU,
    INPUTDISPLAY_HMN_AND_CPU,
};

enum model_display {
    MODELDISPLAY_ON,
    MODELDISPLAY_STAGE,
    MODELDISPLAY_CHARACTERS,
};

enum gen_option
{
    OPTGEN_FRAME,
    OPTGEN_FRAMEBTN,
    OPTGEN_HMNPCNT,
    OPTGEN_HMNPCNTLOCK,
    OPTGEN_MODEL,
    OPTGEN_HIT,
    OPTGEN_ITEMGRAB,
    OPTGEN_COLL,
    OPTGEN_CAM,
    OPTGEN_OVERLAYS_HMN,
    OPTGEN_OVERLAYS_CPU,
    OPTGEN_HUD,
    OPTGEN_DI,
    OPTGEN_INPUT,
    OPTGEN_SPEED,
    OPTGEN_STALE,
    OPTGEN_POWERSHIELD,
    OPTGEN_TAUNT,
    OPTGEN_CUSTOM_OSD,
    OPTGEN_OSDS,

    OPTGEN_COUNT
};

static const char *LabOptions_CamMode[] = {"Normal", "Zoom", "Fixed", "Advanced", "Static"};
static const char *LabOptions_ShowInputs[] = {"Off", "HMN", "CPU", "HMN and CPU"};
static const char *LabOptions_FrameAdvButton[] = {"L", "Z", "X", "Y", "R"};

static const char *LabOptions_ModelDisplay[] = {"On", "Stage Only", "Characters Only"};
static const bool LabValues_CharacterModelDisplay[] = {true, false, true};
static const bool LabValues_StageModelDisplay[] = {true, true, false};

static float LabOptions_GameSpeeds[] = {1.f, 5.f/6.f, 2.f/3.f, 1.f/2.f, 1.f/4.f};
static const char *LabOptions_GameSpeedText[] = {"1", "5/6", "2/3", "1/2", "1/4"};

static EventOption LabOptions_General[OPTGEN_COUNT] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Frame Advance",
        .desc = {"Enable frame advance. Press to advance one",
                 "frame. Hold to advance at normal speed."},
        .OnChange = Lab_ChangeFrameAdvance,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_FrameAdvButton) / 4,
        .name = "Frame Advance Button",
        .desc = {"Choose which button will advance the frame"},
        .values = LabOptions_FrameAdvButton,
        .OnChange = Lab_ChangeFrameAdvanceButton,
        .disable = 1,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 999,
        .name = "Player Percent",
        .desc = {"Adjust the player's percent."},
        .format = "%d%%",
        .OnChange = Lab_ChangePlayerPercent,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Lock Player Percent",
        .desc = {"Locks Player percent to current percent"},
        .OnChange = Lab_ChangePlayerLockPercent,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num =
            sizeof(LabOptions_ModelDisplay) / sizeof(*LabOptions_ModelDisplay),
        .val = 0,
        .name = "Model Display",
        .desc = {"Toggle player and item model visibility."},
        .values = LabOptions_ModelDisplay,
        .OnChange = Lab_ChangeModelDisplay,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fighter Collision",
        .desc = {"Toggle hitbox and hurtbox visualization.",
                 "Hurtboxes: yellow=hurt, purple=ungrabbable, blue=shield.",
                 "Hitboxes: (by priority) red, green, blue, purple."},
        .OnChange = Lab_ChangeHitDisplay,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Item Grab Sizes",
        .desc = {"Toggle item grab range visualization.",
                 "Blue=z-catch, light grey=grounded catch,",
                 "dark grey=unknown"},
        .OnChange = Lab_ChangeItemGrabDisplay,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Environment Collision",
        .desc = {"Toggle environment collision visualization.",
                 "Also displays the players' ECB (environmental ",
                 "collision box)."},
        .OnChange = Lab_ChangeEnvCollDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_CamMode) / 4,
        .name = "Camera Mode",
        .desc = {"Adjust the camera's behavior.",
                 "In advanced mode, use C-Stick while holding",
                 "A/B/Y to pan, rotate and zoom, respectively."},
        .values = LabOptions_CamMode,
        .OnChange = Lab_ChangeCamMode,
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_OverlaysHMN,
        .name = "HMN Color Overlays",
        .desc = {"Set up color indicators for",
                 "action states."},
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_OverlaysCPU,
        .name = "CPU Color Overlays",
        .desc = {"Set up color indicators for",
                 "action states."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .val = 1,
        .name = "HUD",
        .desc = {"Toggle player percents and timer visibility."},
        .OnChange = Lab_ChangeHUD,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "DI Display",
        .desc = {"Display knockback trajectories.",
                 "Use frame advance to see the effects of DI",
                 "in realtime during hitstop."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num =
            sizeof(LabOptions_ShowInputs) / sizeof(*LabOptions_ShowInputs),
        .name = "Input Display",
        .desc = {"Display player inputs onscreen."},
        .values = LabOptions_ShowInputs,
        .OnChange = Lab_ChangeInputDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_GameSpeedText) /
                     sizeof(*LabOptions_GameSpeedText),
        .name = "Game Speed",
        .desc = {"Change how fast the game engine runs."},
        .values = LabOptions_GameSpeedText,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .val = 1,
        .name = "Move Staling",
        .desc = {"Toggle the staling of moves. Attacks become ",
                 "weaker the more they are used."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Powershield Projectiles",
        .desc = {"Projectiles will always be reflected when shielded."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .val = 1,
        .name = "Disable Taunt",
        .desc = {"Disable the taunt button (D-pad up)"},
        .OnChange = Lab_ChangeTauntEnabled,
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_CustomOSDs,
        .name = "Custom OSDs",
        .desc = {"Set up a display for any action state."},
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_OSDs,
        .name = "OSD Menu",
        .desc = {"Enable/disable OSDs"},
    },
};
static EventMenu LabMenu_General = {
    .name = "General",
    .option_num = sizeof(LabOptions_General) / sizeof(EventOption),
    .options = LabOptions_General,
};

// INFO DISPLAY MENU --------------------------------------------------------------

enum infdisp_rows
{
    INFDISP_NONE,
    INFDISP_POS,
    INFDISP_STATE,
    INFDISP_FRAME,
    INFDISP_SELFVEL,
    INFDISP_KBVEL,
    INFDISP_TOTALVEL,
    INFDISP_ENGLSTICK,
    INFDISP_SYSLSTICK,
    INFDISP_ENGCSTICK,
    INFDISP_SYSCSTICK,
    INFDISP_ENGTRIGGER,
    INFDISP_SYSTRIGGER,
    INFDISP_LEDGECOOLDOWN,
    INFDISP_INTANGREMAIN,
    INFDISP_HITSTOP,
    INFDISP_HITSTUN,
    INFDISP_SHIELDHEALTH,
    INFDISP_SHIELDSTUN,
    INFDISP_GRIP,
    INFDISP_ECBLOCK,
    INFDISP_ECBBOT,
    INFDISP_JUMPS,
    INFDISP_WALLJUMPS,
    INFDISP_CANWALLJUMP,
    INFDISP_JAB,
    INFDISP_LINE,
    INFDISP_BLASTLR,
    INFDISP_BLASTUD,

    INFDISP_COUNT
};

enum info_disp_option
{
    OPTINF_PRESET,
    OPTINF_SIZE,
    OPTINF_ROW1,
    OPTINF_ROW2,
    OPTINF_ROW3,
    OPTINF_ROW4,
    OPTINF_ROW5,
    OPTINF_ROW6,
    OPTINF_ROW7,
    OPTINF_ROW8,

    OPTINF_COUNT
};

#define OPTINF_ROW_COUNT (OPTINF_COUNT - OPTINF_ROW1)

static const char *LabValues_InfoDisplay[INFDISP_COUNT] = {"None", "Position", "State Name", "State Frame", "Velocity - Self", "Velocity - KB", "Velocity - Total", "Engine LStick", "System LStick", "Engine CStick", "System CStick", "Engine Trigger", "System Trigger", "Ledgegrab Timer", "Intangibility Timer", "Hitlag", "Hitstun", "Shield Health", "Shield Stun", "Grip Strength", "ECB Lock", "ECB Bottom", "Jumps", "Walljumps", "Can Walljump", "Jab Counter", "Line Info", "Blastzone Left/Right", "Blastzone Up/Down"};

static const char *LabValues_InfoSizeText[] = {"Small", "Medium", "Large"};
static float LabValues_InfoSizes[] = {0.7, 0.85, 1.0};

static const char *LabValues_InfoPresets[] = {"None", "State", "Ledge", "Damage"};
static int LabValues_InfoPresetStates[][OPTINF_ROW_COUNT] = {
    // None
    { 0 },

    // State
    {
        INFDISP_STATE,
        INFDISP_FRAME,
    },

    // Ledge
    {
        INFDISP_STATE,
        INFDISP_FRAME,
        INFDISP_SYSLSTICK,
        INFDISP_ENGLSTICK,
        INFDISP_INTANGREMAIN,
        INFDISP_ECBLOCK,
        INFDISP_ECBBOT,
        INFDISP_NONE,
    },

    // Damage
    {
        INFDISP_STATE,
        INFDISP_FRAME,
        INFDISP_SELFVEL,
        INFDISP_KBVEL,
        INFDISP_TOTALVEL,
        INFDISP_HITSTOP,
        INFDISP_HITSTUN,
        INFDISP_SHIELDSTUN,
    },
};

// copied from LabOptions_InfoDisplayDefault in Event_Init
static EventOption LabOptions_InfoDisplayHMN[OPTINF_COUNT];
static EventOption LabOptions_InfoDisplayCPU[OPTINF_COUNT];

static EventOption LabOptions_InfoDisplayDefault[OPTINF_COUNT] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoPresets) / 4,
        .name = "Display Preset",
        .desc = {"Choose between pre-configured selections."},
        .values = LabValues_InfoPresets,

        // set as Lab_ChangeInfoPresetHMN/CPU after memcpy.
        .OnChange = 0,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoSizeText) / 4,
        .val = 1,
        .name = "Size",
        .desc = {"Change the size of the info display window.",
                 "Large is recommended for CRT.",
                 "Medium/Small recommended for Dolphin Emulator."},
        .values = LabValues_InfoSizeText,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 1",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 2",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 3",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 4",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 5",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 6",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 7",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_InfoDisplay) / 4,
        .name = "Row 8",
        .desc = {"Adjust what is displayed in this row."},
        .values = LabValues_InfoDisplay,
    },
};

static EventMenu LabMenu_InfoDisplayHMN = {
    .name = "HMN Info Display",
    .option_num = sizeof(LabOptions_InfoDisplayHMN) / sizeof(EventOption),
    .options = LabOptions_InfoDisplayHMN,
};

static EventMenu LabMenu_InfoDisplayCPU = {
    .name = "CPU Info Display",
    .option_num = sizeof(LabOptions_InfoDisplayCPU) / sizeof(EventOption),
    .options = LabOptions_InfoDisplayCPU,
};

// CHARACTER RNG MENU --------------------------------------------------------

static const char *LabValues_CharacterRng_Turnip[] =
    { "Default", "Regular Turnip", "Winky Turnip", "Dot Eyes Turnip", "Stitch Face Turnip", "Mr. Saturn", "Bob-omb", "Beam Sword" };
static const char *LabValues_CharacterRng_PeachFSmash[] =
    { "Default", "Golf Club", "Frying Pan", "Tennis Racket" };
static const char *LabValues_CharacterRng_Misfire[] =
    { "Default", "Always Misfire", "Never Misfire" };
static const char *LabValues_CharacterRng_Hammer[] =
    { "Default", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static const char *LabValues_CharacterRng_NanaThrow[] =
    { "Default", "Forward Throw", "Backward Throw", "Up Throw", "Down Throw" };
    
static EventOption LabOptions_CharacterRngPeach[] = {
    {
        .kind = OPTKIND_STRING,
        .name = "Peach Turnip",
        .desc = {"Choose the turnip or item Peach will pull."},
        .value_num = countof(LabValues_CharacterRng_Turnip),
        .values = LabValues_CharacterRng_Turnip,
        .OnChange = Lab_ChangeCharacterRng_Turnip,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "Peach Forward Smash",
        .desc = {"Choose what Peach will use in her Forward Smash."},
        .value_num = countof(LabValues_CharacterRng_PeachFSmash),
        .values = LabValues_CharacterRng_PeachFSmash,
        .OnChange = Lab_ChangeCharacterRng_PeachFSmash,
    },
};
static EventOption LabOptions_CharacterRngLuigi[] = {
    {
        .kind = OPTKIND_STRING,
        .name = "Luigi Misfire",
        .desc = {"Choose if Luigi's SideB will misfire."},
        .value_num = countof(LabValues_CharacterRng_Misfire),
        .values = LabValues_CharacterRng_Misfire,
        .OnChange = Lab_ChangeCharacterRng_Misfire,
    },
};
static EventOption LabOptions_CharacterRngGnW[] = {
    {
        .kind = OPTKIND_STRING,
        .name = "GnW Hammer",
        .desc = {"Choose Game and Watch's SideB number."},
        .value_num = countof(LabValues_CharacterRng_Hammer),
        .values = LabValues_CharacterRng_Hammer,
        .OnChange = Lab_ChangeCharacterRng_Hammer,
    },
};
static EventOption LabOptions_CharacterRngIcies[] = {
    {
        .kind = OPTKIND_STRING,
        .name = "Nana Throw",
        .desc = {"Choose Nana's throw direction."},
        .value_num = countof(LabValues_CharacterRng_NanaThrow),
        .values = LabValues_CharacterRng_NanaThrow,
        .OnChange = Lab_ChangeCharacterRng_NanaThrow,
    }
};

#define OPTCHARRNG_MAXCOUNT 8
static EventMenu LabMenu_CharacterRng = {
    .name = "Character RNG",
    .options = (EventOption[OPTCHARRNG_MAXCOUNT]){},
};

// CHARACTER RNG OPTIONS TABLE --------------------------------------------------------

typedef struct CharacterRngOptions {
    int len;
    EventOption *options;
} CharacterRngOptions;

static const CharacterRngOptions character_rng_options[FTKIND_SANDBAG] = {
    [FTKIND_PEACH] = { countof(LabOptions_CharacterRngPeach), LabOptions_CharacterRngPeach },
    [FTKIND_POPO] = { countof(LabOptions_CharacterRngIcies), LabOptions_CharacterRngIcies },
    [FTKIND_GAW] = { countof(LabOptions_CharacterRngGnW), LabOptions_CharacterRngGnW },
    [FTKIND_LUIGI] = { countof(LabOptions_CharacterRngLuigi), LabOptions_CharacterRngLuigi },
};

// STAGE MENUS -----------------------------------------------------------

// STAGE MENU STADIUM --------------------------------------------------------

#define TRANSFORMATION_TIMER_PTR ((s32**)(R13 - 0x4D28))
#define TRANSFORMATION_ID_PTR ((int*)(R13 + 14200))

enum stage_stadium_option
{
    OPTSTAGE_STADIUM_TRANSFORMATION,
    OPTSTAGE_STADIUM_COUNT,
};

static const char *LabValues_StadiumTransformation[] = { "Normal", "Fire", "Grass", "Rock", "Water" };

static EventOption LabOptions_Stage_Stadium[OPTSTAGE_STADIUM_COUNT] = {{
    .kind = OPTKIND_STRING,
    .value_num = sizeof(LabValues_StadiumTransformation) /
                 sizeof(*LabValues_StadiumTransformation),
    .name = "Transformation",
    .desc = {"Set the current Pokemon Stadium transformation.",
             "Requires Stage Hazards to be on.",
             "THIS OPTION IS EXPERMIMENTAL AND HAS ISSUES."},
    .values = LabValues_StadiumTransformation,
    .OnChange = Lab_ChangeStadiumTransformation,
}};

static EventMenu LabMenu_Stage_Stadium = {
    .name = "Stage Options",
    .option_num = sizeof(LabOptions_Stage_Stadium) / sizeof(EventOption),
    .options = LabOptions_Stage_Stadium,
};

// STAGE MENU FOD --------------------------------------------------------

enum stage_fod_option
{
    OPTSTAGE_FOD_PLAT_HEIGHT_LEFT,
    OPTSTAGE_FOD_PLAT_HEIGHT_RIGHT,
    OPTSTAGE_FOD_COUNT,
};

static const char *LabValues_FODPlatform[] = {"Random", "Hidden", "Lowest", "Left Default", "Average", "Right Default", "Highest"};
static const float LabValues_FODPlatformHeights[] = { 0.0f, -3.25f, 15.f, 20.f, 25.f, 28.f, 35.f };

static EventOption LabOptions_Stage_FOD[OPTSTAGE_FOD_COUNT] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_FODPlatformHeights)/sizeof(*LabValues_FODPlatformHeights),
        .name = "Left Platform Height",
        .desc = {"Adjust the left platform's distance from the ground."},
        .values = LabValues_FODPlatform,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_FODPlatformHeights)/sizeof(*LabValues_FODPlatformHeights),
        .name = "Right Platform Height",
        .desc = {"Adjust the right platform's distance from the ground."},
        .values = LabValues_FODPlatform,
    }
};

static EventMenu LabMenu_Stage_FOD = {
    .name = "Stage Options",
    .option_num = sizeof(LabOptions_Stage_FOD) / sizeof(EventOption),
    .options = LabOptions_Stage_FOD,
};

// STAGE MENU TABLE --------------------------------------------------------

static EventMenu *stage_menus[] = {
    0,                          // GRKINDEXT_DUMMY,
    0,                          // GRKINDEXT_TEST,
    &LabMenu_Stage_FOD,         // GRKINDEXT_IZUMI,
    &LabMenu_Stage_Stadium,     // GRKINDEXT_PSTAD,
    0,                          // GRKINDEXT_CASTLE,
    0,                          // GRKINDEXT_KONGO,
    0,                          // GRKINDEXT_ZEBES,
    0,                          // GRKINDEXT_CORNERIA,
    0,                          // GRKINDEXT_STORY,
    0,                          // GRKINDEXT_ONETT,
    0,                          // GRKINDEXT_MUTECITY,
    0,                          // GRKINDEXT_RCRUISE,
    0,                          // GRKINDEXT_GARDEN,
    0,                          // GRKINDEXT_GREATBAY,
    0,                          // GRKINDEXT_SHRINE,
    0,                          // GRKINDEXT_KRAID,
    0,                          // GRKINDEXT_YOSTER,
    0,                          // GRKINDEXT_GREENS,
    0,                          // GRKINDEXT_FOURSIDE,
    0,                          // GRKINDEXT_MK1,
    0,                          // GRKINDEXT_MK2,
    0,                          // GRKINDEXT_AKANEIA,
    0,                          // GRKINDEXT_VENOM,
    0,                          // GRKINDEXT_PURA,
    0,                          // GRKINDEXT_BIGBLUE,
    0,                          // GRKINDEXT_ICEMT,
    0,                          // GRKINDEXT_ICETOP,
    0,                          // GRKINDEXT_FLATZONE,
    0,                          // GRKINDEXT_OLDPU,
    0,                          // GRKINDEXT_OLDSTORY,
    0,                          // GRKINDEXT_OLDKONGO,
    0,                          // GRKINDEXT_BATTLE,
    0,                          // GRKINDEXT_FD,
};

// CUSTOM OSDS MENU --------------------------------------------------------------

enum custom_osds_option
{
    OPTCUSTOMOSD_ADD,

    OPTCUSTOMOSD_FIRST_CUSTOM,
    OPTCUSTOMOSD_MAX_ADDED = 8,
    OPTCUSTOMOSD_MAX_COUNT = OPTCUSTOMOSD_FIRST_CUSTOM + OPTCUSTOMOSD_MAX_ADDED,
};

static EventOption LabOptions_CustomOSDs[OPTCUSTOMOSD_MAX_COUNT] = {
    {
        .kind = OPTKIND_FUNC,
        .name = "Add Custom OSD",
        .desc = {"Add a new OSD based on the player's",
                 "current action state."},
        .OnSelect = Lab_AddCustomOSD,
    },
    { .disable = true },
    { .disable = true },
    { .disable = true },
    { .disable = true },
    { .disable = true },
    { .disable = true },
    { .disable = true },
    { .disable = true },
};

static EventMenu LabMenu_CustomOSDs = {
    .name = "Custom OSDs",
    .option_num = OPTCUSTOMOSD_MAX_COUNT,
    .options = LabOptions_CustomOSDs,
};

// OSDS MENU --------------------------------------------------------------

// Enabled OSDs are stored as a bitmap in memory at the following bit positions.
// These values must match the OSD IDs from "training-mode/Globals.s".
// The order of the OSDs in this array must also match the order that they appear in LabOptions_OSDs.
static int osd_memory_bit_position[] = {
    0,  // Wavedash
    1,  // L-Cancel
    3,  // Act OoS Frame
    5,  // Dashback
    8,  // Fighter-specific
    9,  // Powershield Frame
    10, // SDI Inputs
    12, // Lockout Timers
    13, // Item Throw Interrupts
    14, // Boost Grab
    16, // Act OoLag
    18, // Act OoAirborne
    19, // Jump Cancel Timing
    20, // Fastfall Timing
    21, // Frame Advantage
    22, // Combo Counter
    24, // Grab Breakout
    26, // Ledgedash Info
    28, // Act OoHitstun
};

static EventOption LabOptions_OSDs[] = {
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Wavedash",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "L-Cancel",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Act OoS Frame",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Dashback",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fighter-specific Tech",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Powershield Frame",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "SDI Inputs",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Lockout Timers",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Item Throw Interrupts",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Boost Grab",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Act OoLag",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Act OoAirborne",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Jump Cancel Timing",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Fastfall Timing",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Frame Advantage",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Combo Counter",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Grab Breakout",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Ledgedash Info",
        .OnChange = Lab_ChangeOSDs,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Act OoHitstun",
        .OnChange = Lab_ChangeOSDs,
    },
};

static EventMenu LabMenu_OSDs = {
    .name = "OSDs",
    .option_num = sizeof(LabOptions_OSDs) / sizeof(EventOption),
    .options = LabOptions_OSDs,
};

// CPU MENU --------------------------------------------------------------

enum cpu_behave
{
    CPUBEHAVE_STAND,
    CPUBEHAVE_SHIELD,
    CPUBEHAVE_CROUCH,
    CPUBEHAVE_JUMP,

    CPUBEHAVE_COUNT
};

enum cpu_sdi
{
    CPUSDI_RANDOM,
    CPUSDI_NONE,

    CPUSDI_COUNT
};

enum cpu_tdi
{
    CPUTDI_RANDOM,
    CPUTDI_IN,
    CPUTDI_OUT,
    CPUTDI_CUSTOM,
    CPUTDI_RANDOM_CUSTOM,
    CPUTDI_NONE,
    CPUTDI_NUM,

    CPUTDI_COUNT
};

enum cpu_shield_angle
{
    CPUSHIELDANG_NONE,
    CPUSHIELDANG_UP,
    CPUSHIELDANG_TOWARD,
    CPUSHIELDANG_DOWN,
    CPUSHIELDANG_AWAY,

    CPUSHIELDANG_COUNT
};

enum cpu_tech
{
    CPUTECH_RANDOM,
    CPUTECH_NEUTRAL,
    CPUTECH_AWAY,
    CPUTECH_TOWARDS,
    CPUTECH_NONE,

    CPUTECH_COUNT
};

enum cpu_getup
{
    CPUGETUP_RANDOM,
    CPUGETUP_STAND,
    CPUGETUP_AWAY,
    CPUGETUP_TOWARD,
    CPUGETUP_ATTACK,

    CPUGETUP_COUNT
};
enum cpu_state
{
    CPUSTATE_START,
    CPUSTATE_GRABBED,
    CPUSTATE_SDI,
    CPUSTATE_TDI,
    CPUSTATE_TECH,
    CPUSTATE_GETUP,
    CPUSTATE_COUNTER,
    CPUSTATE_RECOVER,
    CPUSTATE_NONE,

    CPUSTATE_COUNT
};

enum cpu_mash
{
    CPUMASH_NONE,
    CPUMASH_MED,
    CPUMASH_HIGH,
    CPUMASH_PERFECT,

    CPUMASH_COUNT
};

enum cpu_grab_release
{
    CPUGRABRELEASE_GROUNDED,
    CPUGRABRELEASE_AIRBORN,

    CPUGRABRELEASE_COUNT
};

enum cpu_inf_shield {
    CPUINFSHIELD_OFF,
    CPUINFSHIELD_UNTIL_HIT,
    CPUINFSHIELD_ON,

    CPUINFSHIELD_COUNT
};

enum asdi
{
    ASDI_NONE,
    ASDI_AUTO,
    ASDI_AWAY,
    ASDI_TOWARD,
    ASDI_LEFT,
    ASDI_RIGHT,
    ASDI_UP,
    ASDI_DOWN,

    ASDI_COUNT
};

enum sdi_dir
{
    SDIDIR_RANDOM,
    SDIDIR_AWAY,
    SDIDIR_TOWARD,
    SDIDIR_UP,
    SDIDIR_DOWN,
    SDIDIR_LEFT,
    SDIDIR_RIGHT,

    SDIDIR_COUNT
};

enum controlled_by
{
    CTRLBY_NONE,
    CTRLBY_PORT_1,
    CTRLBY_PORT_2,
    CTRLBY_PORT_3,
    CTRLBY_PORT_4,

    CTRLBY_COUNT,
};

enum cpu_option
{
    OPTCPU_PCNT,
    OPTCPU_LOCKPCNT,
    OPTCPU_TECHOPTIONS,
    OPTCPU_TDI,
    OPTCPU_CUSTOMTDI,
    OPTCPU_SDINUM,
    OPTCPU_SDIDIR,
    OPTCPU_ASDI,
    OPTCPU_BEHAVE,
    OPTCPU_CTRGRND,
    OPTCPU_CTRAIR,
    OPTCPU_CTRSHIELD,
    OPTCPU_CTRFRAMES,
    OPTCPU_CTRADV,
    OPTCPU_SHIELD,
    OPTCPU_SHIELDHEALTH,
    OPTCPU_SHIELDDIR,
    OPTCPU_INTANG,
    OPTCPU_MASH,
    OPTCPU_GRABRELEASE,
    OPTCPU_SET_POS,
    OPTCPU_CTRL_BY,
    OPTCPU_FREEZE,

    OPTCPU_COUNT
};

static const char *LabValues_Shield[] = {"Off", "On Until Hit", "On"};
static const char *LabValues_ShieldDir[] = {"Neutral", "Up", "Towards", "Down", "Away"};
static const char *LabValues_CPUBehave[] = {"Stand", "Shield", "Crouch", "Jump"};
static const char *LabValues_TDI[] = {"Random", "Inwards", "Outwards", "Custom", "Random Custom", "None"};
static const char *LabValues_ASDI[] = {"None", "Auto", "Away", "Towards", "Left", "Right", "Up", "Down"};
static const char *LabValues_SDIDir[] = {"Random", "Away", "Towards", "Up", "Down", "Left", "Right"};
static const char *LabValues_Tech[] = {"Random", "In Place", "Away", "Towards", "None"};
static const char *LabValues_Getup[] = {"Random", "Stand", "Away", "Towards", "Attack"};
static const char *LabValues_GrabEscape[] = {"None", "Medium", "High", "Perfect"};
static const char *LabValues_GrabRelease[] = {"Grounded", "Airborn"};
static const char *LabValues_CPUControlledBy[] = {"None", "Port 1", "Port 2", "Port 3", "Port 4"};

static const EventOption LabOptions_CPU_MoveCPU = {
    .kind = OPTKIND_FUNC,
    .name = "Move CPU",
    .desc = {"Manually set the CPU's position."},
    .OnSelect = Lab_StartMoveCPU,
};

static const EventOption LabOptions_CPU_FinishMoveCPU = {
    .kind = OPTKIND_FUNC,
    .name = "Finish Moving CPU",
    .desc = {"Finish setting the CPU's position."},
    .OnSelect = Lab_FinishMoveCPU,
};

static EventOption LabOptions_CPU[OPTCPU_COUNT] = {
    {
        .kind = OPTKIND_INT,
        .value_num = 999,
        .name = "CPU Percent",
        .desc = {"Adjust the CPU's percent."},
        .format = "%d%%",
        .OnChange = Lab_ChangeCPUPercent,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Lock CPU Percent",
        .desc = {"Locks CPU percent to current percent"},
        .OnChange = Lab_ChangeCPULockPercent,
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_Tech,
        .name = "Tech Options",
        .desc = {"Configure CPU Tech Behavior."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_TDI) / 4,
        .name = "Trajectory DI",
        .desc = {"Adjust how the CPU will alter their knockback",
                 "trajectory."},
        .values = LabValues_TDI,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Custom TDI",
        .desc = {"Create custom trajectory DI values for the",
                 "CPU to perform."},
        .OnSelect = Lab_SelectCustomTDI,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 8,
        .name = "Smash DI Amount",
        .desc = {"Adjust how often the CPU will alter their position",
                 "during hitstop."},
        .format = "%d Frames",
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_SDIDir) / 4,
        .name = "Smash DI Direction",
        .desc = {"Adjust the direction in which the CPU will alter ",
                 "their position during hitstop."},
        .values = LabValues_SDIDir,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_ASDI) / 4,
        .val = 1,
        .name = "ASDI",
        .desc = {"Set CPU C-stick ASDI direction"},
        .values = LabValues_ASDI,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CPUBehave) / 4,
        .name = "Behavior",
        .desc = {"Adjust the CPU's default action."},
        .values = LabValues_CPUBehave,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CounterGround) / 4,
        .val = 1,
        .name = "Counter Action (Ground)",
        .desc = {"Select the action to be performed after a",
                 "grounded CPU's hitstun ends."},
        .values = LabValues_CounterGround,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CounterAir) / 4,
        .val = 4,
        .name = "Counter Action (Air)",
        .desc = {"Select the action to be performed after an",
                 "airborne CPU's hitstun ends."},
        .values = LabValues_CounterAir,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CounterShield) / 4,
        .val = 1,
        .name = "Counter Action (Shield)",
        .desc = {"Select the action to be performed after the",
                 "CPU's shield is hit."},
        .values = LabValues_CounterShield,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 100,
        .name = "Counter Delay",
        .desc = {"Adjust the amount of actionable frames before ",
                 "the CPU counters."},
        .format = "%d Frames",
    },
    {
        .kind = OPTKIND_MENU,
        .menu = &LabMenu_AdvCounter,
        .name = "Advanced Counter Options",
        .desc = {"More options for adjusting how the CPU counters."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_Shield) / 4,
        .val = 1,
        .name = "Infinite Shields",
        .desc = {"Adjust how shield health deteriorates."},
        .values = LabValues_Shield,
    },
    {
        .kind = OPTKIND_INT,
        .val = 60,
        .value_num = 61,
        .name = "Infinite Shields Health",
        .format = "%i",
        .desc = {"Adjust the max shield health when using",
                 "infinite shields."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_ShieldDir) / 4,
        .name = "Shield Angle",
        .desc = {"Adjust how CPU angles their shield."},
        .values = LabValues_ShieldDir,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Intangibility",
        .desc = {"Toggle the CPU's ability to take damage."},
        .OnChange = Lab_ChangeCPUIntang,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_GrabEscape) / 4,
        .val = CPUMASH_NONE,
        .name = "Grab Escape",
        .desc = {"Adjust how the CPU will attempt to escape",
                 "grabs."},
        .values = LabValues_GrabEscape,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_GrabRelease) / 4,
        .val = CPUGRABRELEASE_GROUNDED,
        .name = "Grab Release",
        .desc = {"Adjust how the CPU will escape grabs."},
        .values = LabValues_GrabRelease,
    },

    // swapped between LabOptions_CPU_MoveCPU and LabOptions_CPU_FinishMoveCPU
    LabOptions_CPU_MoveCPU,

    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CPUControlledBy) / 4,
        .val = 0,
        .name = "Controlled By",
        .desc = {"Select another port to control the CPU."},
        .values = LabValues_CPUControlledBy,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Freeze CPU",
        .desc = {"Freeze the CPU and their hitboxes."},
        .OnSelect = Lab_FreezeCPU,
    }
};

static EventMenu LabMenu_CPU = {
    .name = "CPU Options",
    .option_num = sizeof(LabOptions_CPU) / sizeof(EventOption),
    .options = LabOptions_CPU,
};

// ADVANCED COUNTER OPTIONS -------------------------------------------------

enum advanced_counter_option {
    OPTCTR_HITNUM,
    OPTCTR_LOGIC,
    
    OPTCTR_CTRGRND,
    OPTCTR_CTRAIR,
    OPTCTR_CTRSHIELD,
    
    OPTCTR_DELAYGRND,
    OPTCTR_DELAYAIR,
    OPTCTR_DELAYSHIELD,
    
    OPTCTR_COUNT,
};

enum counter_logic {
    CTRLOGIC_DEFAULT,
    CTRLOGIC_DISABLED,
    CTRLOGIC_CUSTOM,
    
    CTRLOGIC_COUNT
};

typedef struct CounterInfo {
    int disable;
    int action_id;
    int counter_delay;
} CounterInfo;
CounterInfo GetCounterInfo(void);

static const char *LabValues_CounterLogic[] = {"Default", "Disable", "Custom"};

#define ADV_COUNTER_COUNT 10
#define ADV_COUNTER_SAVED_COUNT (OPTCTR_COUNT - OPTCTR_HITNUM - 1) 

static EventOption LabOptions_AdvCounter[ADV_COUNTER_COUNT][OPTCTR_COUNT];

static EventOption LabOptions_AdvCounter_Default[OPTCTR_COUNT] = {
    {
        .kind = OPTKIND_INT,
        .name = "Hit Number",
        .val = 1,
        .value_min = 1,
        .value_num = ADV_COUNTER_COUNT,
        .format = "%d",
        .desc = {"Which hit number to alter."},
        .OnChange = Lab_ChangeAdvCounterHitNumber,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CounterLogic) / 4,
        .name = "Counter Logic",
        .desc = {"How to alter the counter option.",
                 "Default = use basic counter options.",
                 "Disable = no counter. Custom = custom behavior."},
        .values = LabValues_CounterLogic,
        .OnChange = Lab_ChangeAdvCounterLogic,
    },
    
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CounterGround) / 4,
        .val = 1,
        .name = "Counter Action (Ground)",
        .desc = {"Select the action to be performed after a",
                 "grounded CPU's hitstun ends."},
        .values = LabValues_CounterGround,
        .disable = 1,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CounterAir) / 4,
        .val = 4,
        .name = "Counter Action (Air)",
        .desc = {"Select the action to be performed after an",
                 "airborne CPU's hitstun ends."},
        .values = LabValues_CounterAir,
        .disable = 1,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CounterShield) / 4,
        .val = 1,
        .name = "Counter Action (Shield)",
        .desc = {"Select the action to be performed after the",
                 "CPU's shield is hit."},
        .values = LabValues_CounterShield,
        .disable = 1,
    },
    
    {
        .kind = OPTKIND_INT,
        .value_num = 100,
        .name = "Delay (Ground)",
        .desc = {"Adjust the amount of actionable frames before ",
                 "the CPU counters on the ground."},
        .format = "%d Frames",
        .disable = 1,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 100,
        .name = "Delay (Air)",
        .desc = {"Adjust the amount of actionable frames before ",
                 "the CPU counters in the air."},
        .format = "%d Frames",
        .disable = 1,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 100,
        .name = "Delay (Shield)",
        .desc = {"Adjust the amount of actionable frames before ",
                 "the CPU counters in shield."},
        .format = "%d Frames",
        .disable = 1,
    },
};

static EventMenu LabMenu_AdvCounter = {
    .name = "Advanced Counter Options",
    .option_num = sizeof(LabOptions_AdvCounter_Default) / sizeof(EventOption),
    .options = LabOptions_AdvCounter[0],
};

// TECH MENU --------------------------------------------------------------

enum tech_trap {
    TECHTRAP_NONE,
    TECHTRAP_EARLIEST,
    TECHTRAP_LATEST,
};

enum tech_lockout {
    TECHLOCKOUT_EARLIEST,
    TECHLOCKOUT_LATEST,
};

static const char *LabOptions_TechTrap[] = {"Off", "Earliest Tech Input", "Latest Tech Input"};
static const char *LabOptions_TechLockout[] = {"Earliest Tech Input", "Latest Tech Input"};

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

enum tech_option
{
    OPTTECH_TECH,
    OPTTECH_GETUP,
    OPTTECH_INVISIBLE,
    OPTTECH_INVISIBLE_DELAY,
    OPTTECH_SOUND,
    OPTTECH_TRAP,
    OPTTECH_LOCKOUT,

    OPTTECH_TECHINPLACECHANCE,
    OPTTECH_TECHAWAYCHANCE,
    OPTTECH_TECHTOWARDCHANCE,
    OPTTECH_MISSTECHCHANCE,
    OPTTECH_GETUPWAITCHANCE,
    OPTTECH_GETUPSTANDCHANCE,
    OPTTECH_GETUPAWAYCHANCE,
    OPTTECH_GETUPTOWARDCHANCE,
    OPTTECH_GETUPATTACKCHANCE,

    OPTTECH_COUNT
};

static EventOption LabOptions_Tech[OPTTECH_COUNT] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_Tech) / 4,
        .name = "Tech Option",
        .desc = {"Adjust what the CPU will do upon colliding",
                 "with the stage."},
        .values = LabValues_Tech,
        .OnChange = Lab_ChangeTech,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_Getup) / 4,
        .name = "Get Up Option",
        .desc = {"Adjust what the CPU will do after missing",
                 "a tech input."},
        .values = LabValues_Getup,
        .OnChange = Lab_ChangeGetup,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Tech Invisibility",
        .desc = {"Toggle the CPU turning invisible during tech","animations."},
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 16,
        .name = "Tech Invisibility Delay",
        .format = "%d Frames",
        .desc = {"Set the delay in frames on tech invisibility."},
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Tech Sound",
        .desc = {"Toggle playing a sound when tech is",
                 "distinguishable."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_TechTrap)/sizeof(*LabOptions_TechTrap),
        .name = "Simulate Tech Trap",
        .desc = {"Set a window where the CPU cannot tech",
                 "after being hit out of tumble."},
        .values = LabOptions_TechTrap,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_TechLockout)/sizeof(*LabOptions_TechLockout),
        .name = "Tech Lockout",
        .desc = {"Prevent the CPU from teching in succession.",
                 "Earliest - as little lockout as possible",
                 "Latest - as much lockout as possible."},
        .values = LabOptions_TechLockout,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Tech in Place Chance",
        .desc = {"Adjust the chance the CPU will tech in place."},
        .format = "%d%%",
        .OnChange = Lab_ChangeTechInPlaceChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Tech Away Chance",
        .desc = {"Adjust the chance the CPU will tech away."},
        .format = "%d%%",
        .OnChange = Lab_ChangeTechAwayChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Tech Toward Chance",
        .desc = {"Adjust the chance the CPU will tech toward."},
        .format = "%d%%",
        .OnChange = Lab_ChangeTechTowardChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Miss Tech Chance",
        .desc = {"Adjust the chance the CPU will miss tech."},
        .format = "%d%%",
        .OnChange = Lab_ChangeMissTechChance,
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
        .OnChange = Lab_ChangeStandChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Roll Away Chance",
        .desc = {"Adjust the chance the CPU will roll away."},
        .format = "%d%%",
        .OnChange = Lab_ChangeRollAwayChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Roll Toward Chance",
        .desc = {"Adjust the chance the CPU will roll toward."},
        .format = "%d%%",
        .OnChange = Lab_ChangeRollTowardChance,
    },
    {
        .kind = OPTKIND_INT,
        .value_num = 101,
        .val = 25,
        .name = "Getup Attack Chance",
        .desc = {"Adjust the chance the CPU will getup attack."},
        .format = "%d%%",
        .OnChange = Lab_ChangeGetupAttackChance,
    },
};

static EventMenu LabMenu_Tech = {
    .name = "Tech Options",
    .option_num = sizeof(LabOptions_Tech) / sizeof(EventOption),
    .options = LabOptions_Tech,
};

// PLAYBACK CHANCES MENU -----------------------------------------------------

enum slot_chance_menu
{
    OPTSLOTCHANCE_1,
    OPTSLOTCHANCE_2,
    OPTSLOTCHANCE_3,
    OPTSLOTCHANCE_4,
    OPTSLOTCHANCE_5,
    OPTSLOTCHANCE_6,
    OPTSLOTCHANCE_PERCENT,
    OPTSLOTCHANCE_COUNT
};

static EventOption LabOptions_SlotChancesHMN[OPTSLOTCHANCE_COUNT] = {
    {
        .kind = OPTKIND_INT,
        .name = "Slot 1",
        .desc = {"Chance of slot 1 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot1ChanceHMN,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 2",
        .desc = {"Chance of slot 2 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot2ChanceHMN,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 3",
        .desc = {"Chance of slot 3 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot3ChanceHMN,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 4",
        .desc = {"Chance of slot 4 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot4ChanceHMN,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 5",
        .desc = {"Chance of slot 5 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot5ChanceHMN,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 6",
        .desc = {"Chance of slot 6 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot6ChanceHMN,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Random Percent",
        .desc = {"A random percentage up to this value will be",
                 "added to the character's percentage each load."},
        .format = "%d%%",
        .value_num = 201,
    },
};

static EventOption LabOptions_SlotChancesCPU[OPTSLOTCHANCE_COUNT] = {
    {
        .kind = OPTKIND_INT,
        .name = "Slot 1",
        .desc = {"Chance of slot 1 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot1ChanceCPU,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 2",
        .desc = {"Chance of slot 2 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot2ChanceCPU,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 3",
        .desc = {"Chance of slot 3 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot3ChanceCPU,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 4",
        .desc = {"Chance of slot 4 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot4ChanceCPU,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 5",
        .desc = {"Chance of slot 5 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot5ChanceCPU,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Slot 6",
        .desc = {"Chance of slot 6 occuring."},
        .format = "%d%%",
        .value_num = 101,
        .disable = 1,
        .OnChange = Lab_ChangeSlot6ChanceCPU,
    },
    {
        .kind = OPTKIND_INT,
        .name = "Random Percent",
        .desc = {"A random percentage up to this value will be",
                 "added to the character's percentage each load."},
        .format = "%d%%",
        .value_num = 201,
    },
};

static EventMenu LabMenu_SlotChancesHMN = {
    .name = "HMN Playback Slot Chances",
    .option_num = sizeof(LabOptions_SlotChancesHMN) / sizeof(EventOption),
    .options = LabOptions_SlotChancesHMN,
};

static EventMenu LabMenu_SlotChancesCPU = {
    .name = "CPU Playback Slot Chances",
    .option_num = sizeof(LabOptions_SlotChancesCPU) / sizeof(EventOption),
    .options = LabOptions_SlotChancesCPU,
};

// RECORDING MENU --------------------------------------------------------------

enum autorestore
{
    AUTORESTORE_NONE,
    AUTORESTORE_PLAYBACK_END,
    AUTORESTORE_COUNTER,

    AUTORESTORE_COUNT
};

enum rec_mode_hmn
{
    RECMODE_HMN_OFF,
    RECMODE_HMN_RECORD,
    RECMODE_HMN_PLAYBACK,
    RECMODE_HMN_RERECORD,

    RECMODE_COUNT
};

enum rec_mode_cpu
{
    RECMODE_CPU_OFF,
    RECMODE_CPU_CONTROL,
    RECMODE_CPU_RECORD,
    RECMODE_CPU_PLAYBACK,
    RECMODE_CPU_RERECORD,

    RECMODE_CPU_COUNT
};

enum rec_option
{
   OPTREC_SAVE_LOAD,
   OPTREC_HMNMODE,
   OPTREC_HMNSLOT,
   OPTREC_CPUMODE,
   OPTREC_CPUSLOT,
   OPTREC_MIRRORED_PLAYBACK,
   OPTREC_PLAYBACK_COUNTER,
   OPTREC_LOOP,
   OPTREC_AUTORESTORE,
   OPTREC_STARTPAUSED,
   OPTREC_TAKEOVER,
   OPTREC_RESAVE,
   OPTREC_PRUNE,
   OPTREC_DELETE,
   OPTREC_SLOTMANAGEMENT,
   OPTREC_HMNCHANCE,
   OPTREC_CPUCHANCE,
   OPTREC_EXPORT,

   OPTREC_COUNT
};

enum rec_playback_counter
{
    PLAYBACKCOUNTER_OFF,
    PLAYBACKCOUNTER_ENDS,
    PLAYBACKCOUNTER_ON_HIT_CPU,
    PLAYBACKCOUNTER_ON_HIT_HMN,
    PLAYBACKCOUNTER_ON_HIT_EITHER,
};

enum rec_mirror
{
    OPTMIRROR_OFF,
    OPTMIRROR_ON,
    OPTMIRROR_RANDOM,
};

enum rec_takeover_target
{
    TAKEOVER_HMN,
    TAKEOVER_CPU,
    TAKEOVER_NONE,
};

// Aitch: Please be aware that the order of these options is important.
// The option idx will be serialized when exported, so loading older replays could load the wrong option if we reorder/remove options.
static const char *LabValues_RecordSlot[] = {"Random", "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5", "Slot 6"};
static const char *LabValues_HMNRecordMode[] = {"Off", "Record", "Playback", "Re-Record"};
static const char *LabValues_CPURecordMode[] = {"Off", "Control", "Record", "Playback", "Re-Record"};
static const char *LabValues_AutoRestore[] = {"Off", "Playback Ends", "CPU Counters"};
static const char *LabValues_PlaybackCounterActions[] = {"Off", "After Playback Ends", "On CPU Hit", "On HMN Hit", "On Any Hit"};
static const char *LabOptions_ChangeMirroredPlayback[] = {"Off", "On", "Random"};

static const EventOption Record_Save = {
    .kind = OPTKIND_FUNC,
    .name = "Save Positions",
    .desc = {"Save the current fighter positions",
             "as the initial positions."},
    .OnSelect = Record_InitState,
};

static const EventOption Record_Load = {
    .kind = OPTKIND_FUNC,
    .name = "Restore Positions",
    .desc = {"Load the saved fighter positions and ",
             "start the sequence from the beginning."},
    .OnSelect = Record_RestoreState,
};

static const char *LabOptions_TakeoverTarget[] = {"HMN", "CPU", "None"};

static EventOption LabOptions_Record[OPTREC_COUNT] = {
    // swapped between Record_Save and Record_Load
    Record_Save,

    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_HMNRecordMode) / 4,
        .name = "HMN Mode",
        .desc = {"Toggle between recording and playback of",
                 "inputs."},
        .values = LabValues_HMNRecordMode,
        .OnChange = Record_ChangeHMNMode,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_RecordSlot) / 4,
        .val = 1,
        .name = "HMN Record Slot",
        .desc = {"Toggle which recording slot to save inputs ",
                 "to. Maximum of 6 and can be set to random ",
                 "during playback."},
        .values = LabValues_RecordSlot,
        .OnChange = Record_ChangeHMNSlot,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_CPURecordMode) / 4,
        .name = "CPU Mode",
        .desc = {"Toggle between recording and playback of",
                 "inputs."},
        .values = LabValues_CPURecordMode,
        .OnChange = Record_ChangeCPUMode,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_RecordSlot) / 4,
        .val = 1,
        .name = "CPU Record Slot",
        .desc = {"Toggle which recording slot to save inputs ",
                 "to. Maximum of 6 and can be set to random ","during playback."},
        .values = LabValues_RecordSlot,
        .OnChange = Record_ChangeCPUSlot,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_ChangeMirroredPlayback) / 4,
        .name = "Mirrored Playback",
        .desc = {"Playback with mirrored the recorded inputs,",
                 "positions and facing directions.",
                 "(!) This works properly only on symmetrical ",
                 "stages."},
        .values = LabOptions_ChangeMirroredPlayback,
        .OnChange = Record_ChangeMirroredPlayback,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_PlaybackCounterActions) / 4,
        .name = "CPU Counter",
        .val = 1,
        .desc = {"Choose when CPU will start performing",
                 "counter actions during playback."},
        .values = LabValues_PlaybackCounterActions,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Loop Input Playback",
        .desc = {"Loop the recorded inputs when they end."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabValues_AutoRestore) / 4,
        .name = "Auto Restore",
        .desc = {"Automatically restore saved positions."},
        .values = LabValues_AutoRestore,
    },
    {
        .kind = OPTKIND_TOGGLE,
        .name = "Start Paused",
        .desc = {"Pause the replay until your first input."},
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_TakeoverTarget) / 4,
        .name = "Playback Takeover",
        .desc = {"Which character to takeover when",
                 "inputting during playback."},
        .values = LabOptions_TakeoverTarget,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Re-Save Positions",
        .desc = {"Save the current position, keeping",
                 "all recorded inputs."},
        .OnSelect = Record_ResaveState,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Prune Positions",
        .desc = {"Save the current position, keeping",
                 "recorded inputs from this point onwards."},
        .OnSelect = Record_PruneState,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Delete Positions",
        .desc = {"Delete the current initial position",
                 "and recordings."},
        .OnSelect = Record_DeleteState,
    },
    {
        .kind = OPTKIND_MENU,
        .name = "Slot Management",
        .desc = {"Miscellaneous settings for altering the",
                 "positions and inputs."},
        .menu = &LabMenu_SlotManagement,
    },
    {
        .kind = OPTKIND_MENU,
        .name = "Set HMN Chances",
        .desc = {"Set various randomization settings for the HMN."},
        .menu = &LabMenu_SlotChancesHMN,
    },
    {
        .kind = OPTKIND_MENU,
        .name = "Set CPU Chances",
        .desc = {"Set various randomization settings for the CPU."},
        .menu = &LabMenu_SlotChancesCPU,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Export",
        .desc = {"Export the recording to a memory card",
                 "for later use or to share with others."},
        .OnSelect = Export_Init,
    },
};

static EventMenu LabMenu_Record = {
    .name = "Recording",
    .option_num = sizeof(LabOptions_Record) / sizeof(EventOption),
    .options = LabOptions_Record,
};

// SLOT MANAGEMENT MENU --------------------------------------------------------------

enum state_options {
    OPTSLOT_PLAYER,
    OPTSLOT_SRC,
    OPTSLOT_MODIFY,
    OPTSLOT_DELETE,
    OPTSLOT_DST,
    OPTSLOT_COPY,

    OPTSLOT_COUNT
};

enum player_type {
    PLAYER_HMN,
    PLAYER_CPU,
};

static const char *LabOptions_HmnCpu[] = {"HMN", "CPU"};
static const char *LabOptions_Slot[] = {"Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5", "Slot 6"};

static EventOption LabOptions_SlotManagement[OPTSLOT_COUNT] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_HmnCpu) / 4,
        .name = "Player",
        .desc = {"Select the player to manage."},
        .values = LabOptions_HmnCpu,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_Slot) / 4,
        .name = "Slot",
        .desc = {"Select the slot to manage."},
        .values = LabOptions_Slot,
    },
    {
        .kind = OPTKIND_MENU,
        .name = "Modify Inputs",
        .desc = {"Manually alter this slot's inputs."},
        .menu = &LabMenu_AlterInputs,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Delete Slot",
        .desc = {"Remove the inputs from this slot."},
        .OnSelect = Record_DeleteSlot,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = sizeof(LabOptions_Slot) / 4,
        .name = "Copy Slot To",
        .desc = {"Select the slot to copy to."},
        .values = LabOptions_Slot,
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Copy Slot",
        .desc = {"Copy the inputs from \"Slot\"",
                 "to \"Copy Slot To\"."},
        .OnSelect = Record_CopySlot,
    },
};

static EventMenu LabMenu_SlotManagement = {
    .name = "Slot Management",
    .option_num = sizeof(LabOptions_SlotManagement) / sizeof(EventOption),
    .options = LabOptions_SlotManagement,
};

// ALTER INPUTS MENU ---------------------------------------------------------

enum alter_inputs_options {
    OPTINPUT_FRAME,
    OPTINPUT_LSTICK_X,
    OPTINPUT_LSTICK_Y,
    OPTINPUT_CSTICK_X,
    OPTINPUT_CSTICK_Y,
    OPTINPUT_TRIGGER,

    OPTINPUT_A,
    OPTINPUT_B,
    OPTINPUT_X,
    OPTINPUT_Y,
    OPTINPUT_Z,
    OPTINPUT_L,
    OPTINPUT_R,

    OPTINPUT_COUNT,
};

static EventOption LabOptions_AlterInputs[OPTINPUT_COUNT] = {
    {
        .kind = OPTKIND_INT,
        .value_min = 1,
        .val = 1,
        .value_num = 3600,
        .name = "Frame",
        .desc = {"Which frame's inputs to alter."},
        .format = "%d",
        .OnChange = Lab_ChangeAlterInputsFrame,
    },
    {
        .kind = OPTKIND_INT,
        .value_min = -80,
        .value_num = 161,
        .name = "Stick X",
        .format = "%d",
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_INT,
        .value_min = -80,
        .value_num = 161,
        .name = "Stick Y",
        .format = "%d",
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_INT,
        .value_min = -80,
        .value_num = 161,
        .name = "C-Stick X",
        .format = "%d",
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_INT,
        .value_min = -80,
        .value_num = 161,
        .name = "C-Stick Y",
        .format = "%d",
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_INT,
        .value_min = 0,
        .value_num = 141,
        .name = "Analog Trigger",
        .format = "%d",
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "A",
        .value_num = 2,
        .values = LabOptions_CheckBox,
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "B",
        .value_num = 2,
        .values = LabOptions_CheckBox,
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "X",
        .value_num = 2,
        .values = LabOptions_CheckBox,
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "Y",
        .value_num = 2,
        .values = LabOptions_CheckBox,
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "Z",
        .value_num = 2,
        .values = LabOptions_CheckBox,
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "L",
        .value_num = 2,
        .values = LabOptions_CheckBox,
        .OnChange = Lab_ChangeInputs,
    },
    {
        .kind = OPTKIND_STRING,
        .name = "R",
        .value_num = 2,
        .values = LabOptions_CheckBox,
        .OnChange = Lab_ChangeInputs,
    },
};

static EventMenu LabMenu_AlterInputs = {
    .name = "Alter Inputs",
    .option_num = sizeof(LabOptions_AlterInputs) / sizeof(EventOption),
    .options = LabOptions_AlterInputs,
};

// OVERLAY MENU --------------------------------------------------------------

#define OVERLAY_COLOUR_COUNT 11
static const char *LabValues_OverlayNames[OVERLAY_COLOUR_COUNT] = { 
    "None", "Red", "Green", "Blue", "Yellow", "White", "Black", 
    "Remove Overlay", "Show Collision", "Invisible", "Play Sound"
};

typedef struct Overlay {
    u32 invisible : 1;
    u32 play_sound : 1;
    u32 occur_once : 1;
    u32 show_collision : 1;
    GXColor color;
} Overlay;

static Overlay LabValues_OverlayColours[OVERLAY_COLOUR_COUNT] = {
    { .color = { 0  , 0  , 0  , 0   } },
    { .color = { 255, 20 , 20 , 180 } },
    { .color = { 20 , 255, 20 , 180 } },
    { .color = { 20 , 20 , 255, 180 } },
    { .color = { 220, 220, 20 , 180 } },
    { .color = { 255, 255, 255, 180 } },
    { .color = { 20 , 20 , 20 , 180 } },

    { .color = { 0  , 0  , 0  , 0   } },
    { .show_collision = 1 },
    { .invisible = 1 },
    { .play_sound = 1, .occur_once = 1 }
};

typedef enum overlay_type
{
    OVERLAY_ACTIONABLE,
    OVERLAY_HITSTUN,
    OVERLAY_INVINCIBLE,
    OVERLAY_LEDGE_ACTIONABLE,
    OVERLAY_MISSED_LCANCEL,
    OVERLAY_CAN_FASTFALL,
    OVERLAY_AUTOCANCEL,
    OVERLAY_CROUCH,
    OVERLAY_WAIT,
    OVERLAY_WALK,
    OVERLAY_DASH,
    OVERLAY_RUN,
    OVERLAY_JUMPS_USED,
    OVERLAY_FULLHOP,
    OVERLAY_SHORTHOP,
    OVERLAY_IASA,
    OVERLAY_SHIELD_STUN,

    OVERLAY_COUNT,
} OverlayGroup;

// copied from LabOptions_OverlaysDefault in Event_Init
static EventOption LabOptions_OverlaysCPU[OVERLAY_COUNT];
static EventOption LabOptions_OverlaysHMN[OVERLAY_COUNT];

static EventOption LabOptions_OverlaysDefault[OVERLAY_COUNT] = {
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Actionable",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Hitstun",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Invincible",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Ledge Actionable",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Missed L-Cancel",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Can Fastfall",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Autocancel",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Crouch",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Wait",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Walk",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Dash",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Run",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Jumps",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Full Hop",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Short Hop",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "IASA",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    },
    {
        .kind = OPTKIND_STRING,
        .value_num = OVERLAY_COLOUR_COUNT,
        .name = "Shield Stun",
        .values = LabValues_OverlayNames,
        .OnChange = Lab_ChangeOverlays,
    }

};

static EventMenu LabMenu_OverlaysHMN = {
    .name = "HMN Overlays",
    .option_num = sizeof(LabOptions_OverlaysHMN) / sizeof(EventOption),
    .options = LabOptions_OverlaysHMN,
};

static EventMenu LabMenu_OverlaysCPU = {
    .name = "CPU Overlays",
    .option_num = sizeof(LabOptions_OverlaysCPU) / sizeof(EventOption),
    .options = LabOptions_OverlaysCPU,
};

// SHORTCUTS #########################################################

static Shortcut Lab_Shortcuts[] = {
    {
        .button_mask = HSD_BUTTON_A,
        .option = &LabOptions_General[OPTGEN_FRAME],
    },
    {
        .button_mask = HSD_BUTTON_X,
        .option = &LabOptions_General[OPTGEN_MODEL],
    },
    {
        .button_mask = HSD_BUTTON_DPAD_LEFT,
        .option = &LabOptions_CPU[OPTCPU_TECHOPTIONS],
    },
    {
        .button_mask = HSD_BUTTON_DPAD_RIGHT,
        .option = &LabOptions_Record[OPTREC_SLOTMANAGEMENT],
    },
    {
        .button_mask = HSD_BUTTON_DPAD_UP,
        .option = &LabOptions_General[OPTGEN_OSDS],
    },
    {
        .button_mask = HSD_BUTTON_DPAD_DOWN,
        .option = &LabOptions_Main[OPTLAB_RECORD_OPTIONS],
    },
};

static ShortcutList Lab_ShortcutList = {
    .count = countof(Lab_Shortcuts),
    .list = Lab_Shortcuts,
};
