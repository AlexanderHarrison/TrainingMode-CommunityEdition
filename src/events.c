#include "events.h"
#include "savestate.h"
#include <stdarg.h>

/////////////////////
// Mod Information //
/////////////////////

static char TM_VersShort[] = TM_VERSSHORT "\n";
static char TM_VersLong[] = TM_VERSLONG "\n";
static char TM_Compile[] = "COMPILED: " __DATE__ " " __TIME__;

////////////////////////
/// Event Defintions ///
////////////////////////

// Lab
static EventMatchData Lab_MatchData = {
    .timer = MATCH_TIMER_COUNTUP,
    .matchType = MATCH_MATCHTYPE_TIME,
    .hideGo = true,
    .hideReady = true,
    .isCreateHUD = true,
    .timerRunOnPause = false,
    .isCheckForZRetry = false,
    .isShowScore = false,

    .isRunStockLogic = false,
    .isDisableHit = false,
    .useKOCounter = false,
    .timerSeconds = 0,
};
EventDesc Lab = {
    // Event Name
    .eventName = "Training Lab\n",
    .eventDescription = "Free practice with\ncomplete control.\n",
    .eventFile = "lab",
    .jumpTableIndex = -1,
    .eventCSSFile = "TM/labCSS.dat",
    .CSSType = SLCHRKIND_TRAINING,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = &Lab_MatchData,
};

// L-Cancel Training
static EventMatchData LCancel_MatchData = {
    .timer = MATCH_TIMER_HIDE,
    .matchType = MATCH_MATCHTYPE_TIME,
    .hideGo = true,
    .hideReady = true,
    .isCreateHUD = false,
    .timerRunOnPause = false,
    .isCheckForZRetry = false,
    .isShowScore = false,

    .isRunStockLogic = false,
    .isDisableHit = false,
    .useKOCounter = false,
    .timerSeconds = 0,
};
EventDesc LCancel = {
    // Event Name
    .eventName = "L-Cancel Training\n",
    .eventDescription = "Practice L-Cancelling on\na stationary CPU.\n",
    .eventFile = "lcancel",
    .jumpTableIndex = -1,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 15,
    .matchData = &LCancel_MatchData,
};

// Ledgedash Training
static EventMatchData Ledgedash_MatchData = {
    .timer = MATCH_TIMER_HIDE,
    .matchType = MATCH_MATCHTYPE_TIME,
    .hideGo = true,
    .hideReady = true,
    .isCreateHUD = false,
    .timerRunOnPause = false,
    .isCheckForZRetry = false,
    .isShowScore = false,

    .isRunStockLogic = false,
    .isDisableHit = false,
    .useKOCounter = false,
    .timerSeconds = 0,
};
EventDesc Ledgedash = {
    .eventName = "Ledgedash Training\n",
    .eventDescription = "Practice Ledgedashes!\nUse D-Pad to change ledge.\n",
    .eventFile = "ledgedash",
    .jumpTableIndex = -1,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = true,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 15,
    .matchData = &Ledgedash_MatchData,
};

// Wavedash Training
static EventMatchData Wavedash_MatchData = {
    .timer = MATCH_TIMER_HIDE,
    .matchType = MATCH_MATCHTYPE_TIME,
    .hideGo = true,
    .hideReady = true,
    .isCreateHUD = false,
    .timerRunOnPause = false,
    .isCheckForZRetry = false,
    .isShowScore = false,

    .isRunStockLogic = false,
    .isDisableHit = false,
    .useKOCounter = false,
    .timerSeconds = 0,
};
EventDesc Wavedash = {
    .eventName = "Wavedash Training\n",
    .eventDescription = "Practice timing your wavedash,\na fundamental movement technique.\n",
    .eventFile = "wavedash",
    .jumpTableIndex = -1,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 15,
    .matchData = &Wavedash_MatchData,
};

// Combo Training
EventDesc Combo = {

    .eventName = "Combo Training\n",
    .eventDescription = "L+DPad adjusts percent | DPadDown moves CPU\nDPad right/left saves and loads positions.",
    .eventFile = 0,
    .jumpTableIndex = JUMP_COMBO,
    .CSSType = SLCHRKIND_TRAINING,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

// Attack On Shield Training
EventDesc AttackOnShield = {
    .eventName = "Attack on Shield\n",
    .eventDescription = "Practice attacks on a shielding opponent\nPause to change their OoS option.\n",
    .eventFile = 0,
    .jumpTableIndex = JUMP_ATTACKONSHIELD,
    .CSSType = SLCHRKIND_TRAINING,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc Reversal = {
    .eventName = "Reversal Training\n",
    .eventDescription = "Practice OoS punishes! DPad left/right\nmoves characters closer and further apart.",
    .eventFile = 0,
    .jumpTableIndex = JUMP_REVERSAL,
    .CSSType = SLCHRKIND_TRAINING,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc SDI = {
    .eventName = "SDI Training\n",
    .eventDescription = "Use Smash DI to escape\nFox's up-air!",
    .eventFile = 0,
    .jumpTableIndex = JUMP_SDITRAINING,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = CKIND_FOX,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,

};

static EventMatchData Powershield_MatchData = {
    .timer = MATCH_TIMER_COUNTUP,
    .matchType = MATCH_MATCHTYPE_TIME,
    .hideGo = true,
    .hideReady = true,
    .isCreateHUD = true,
    .timerRunOnPause = false,
    .isCheckForZRetry = true,
    .isShowScore = false,

    .isRunStockLogic = false,
    .isDisableHit = false,
    .useKOCounter = false,
    .timerSeconds = 0,
};
EventDesc Powershield = {
    .eventName = "Powershield Training\n",
    .eventDescription = "Powershield Falco's laser!",
    .eventFile = "powershield",
    .jumpTableIndex = -1,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = CKIND_FALCO,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = true,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = &Powershield_MatchData,
};
EventDesc Ledgetech = {
    .eventName = "Ledgetech Training\n",
    .eventDescription = "Practice ledgeteching\nFalco's down-smash!",
    .eventFile = 0,
    .jumpTableIndex = JUMP_LEDGETECH,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = CKIND_FALCO,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc AmsahTech = {
    .eventName = "Amsah-Tech Training\n",
    .eventDescription = "Taunt to have Marth Up-B,\nthen ASDI down and tech!\n",
    .eventFile = 0,
    .jumpTableIndex = JUMP_AMSAHTECH,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = CKIND_MARTH,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc ShieldDrop = {
    .eventName = "Shield Drop Training\n",
    .eventDescription = "Counter with a shield-drop aerial!\nDPad left/right moves players apart.",
    .eventFile = 0,
    .jumpTableIndex = JUMP_SHIELDDROP,
    .CSSType = SLCHRKIND_TRAINING,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};
EventDesc WaveshineSDI = {
    .eventName = "Waveshine SDI\n",
    .eventDescription = "Use Smash DI to get out\nof Fox's waveshine!",
    .eventFile = 0,
    .jumpTableIndex = JUMP_WAVESHINESDI,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = {
        .hmn = CSSID_DOCTOR_MARIO | CSSID_MARIO | CSSID_BOWSER | CSSID_PEACH | CSSID_YOSHI
                    | CSSID_DONKEY_KONG | CSSID_CAPTAIN_FALCON | CSSID_GANONDORF | CSSID_NESS
                    | CSSID_SAMUS | CSSID_ZELDA | CSSID_LINK,
        .cpu = -1,
    },
    .playerKind = -1,
    .cpuKind = CKIND_FOX,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc SlideOff = {
    .eventName = "Slide-Off Training\n",
    .eventDescription = "Use Slide-Off DI to slide off\nthe platform and counter attack!\n",
    .eventFile = 0,
    .jumpTableIndex = JUMP_SLIDEOFF,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = CKIND_MARTH,
    .stage = GRKINDEXT_PSTAD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc GrabMash = {
    .eventName = "Grab Mash Training\n",
    .eventDescription = "Mash buttons to escape the grab\nas quickly as possible!\n",
    .eventFile = 0,
    .jumpTableIndex = JUMP_GRABMASH,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = CKIND_MARTH,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};
EventDesc TechCounter = {
    .eventName = "Ledgetech Marth Counter\n",
    .eventDescription = "Practice ledgeteching\nMarth's counter!\n",
    .eventFile = 0,
    .jumpTableIndex = JUMP_LEDGETECHCOUNTER,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = {
        .hmn = CSSID_FALCO | CSSID_FOX,
        .cpu = -1,
    },
    .playerKind = -1,
    .cpuKind = CKIND_MARTH,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

static EventMatchData Edgeguard_MatchData = {
    .timer = MATCH_TIMER_COUNTUP,
    .matchType = MATCH_MATCHTYPE_TIME,
    .hideGo = true,
    .hideReady = true,
    .isCreateHUD = true,
    .timerRunOnPause = false,
    .isCheckForZRetry = true,
    .isShowScore = false,

    .isRunStockLogic = false,
    .isDisableHit = false,
    .useKOCounter = false,
    .timerSeconds = 0,
};
EventDesc Edgeguard = {
    .eventName = "Edgeguard Training\n",
    .eventDescription = "Finish off the enemy\nafter you hit them offstage!",
    .eventFile = "edgeguard",
    .jumpTableIndex = -1,
    .CSSType = SLCHRKIND_TRAINING,
    .allowed_characters = {
        .hmn = -1,
        .cpu = CSSID_FOX | CSSID_FALCO | CSSID_ZELDA | CSSID_CAPTAIN_FALCON
            | CSSID_MARTH
    },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = &Edgeguard_MatchData,
};

EventDesc SideBSweet = {
    .eventName = "Side-B Sweetspot\n",
    .eventDescription = "Use a sweetspot Side-B to avoid Marth's\ndown-tilt and grab the ledge!",
    .eventFile = 0,
    .jumpTableIndex = JUMP_SIDEBSWEET,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = {
        .hmn = CSSID_FALCO | CSSID_FOX,
        .cpu = -1,
    },
    .playerKind = -1,
    .cpuKind = CKIND_MARTH,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};
EventDesc EscapeSheik = {
    .eventName = "Escape Sheik Techchase\n",
    .eventDescription = "Practice escaping the tech chase with a\nframe perfect shine or jab SDI!\n",
    .eventFile = 0,
    .jumpTableIndex = JUMP_ESCAPESHIEK,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = {
        .hmn = CSSID_YOSHI |  CSSID_CAPTAIN_FALCON |  CSSID_FALCO |  CSSID_FOX |  CSSID_PIKACHU,
        .cpu = -1,
    },
    .playerKind = -1,
    .cpuKind = CKIND_SHEIK,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc Eggs = {
    .eventName = "Eggs-ercise\n",
    .eventDescription = "Break the eggs! Only strong hits will\nbreak them. DPad down = free practice.",
    .eventFile = 0,
    .jumpTableIndex = JUMP_EGGS,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = -1,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};
EventDesc Multishine = {
    .eventName = "Shined Blind\n",
    .eventDescription = "How many shines can you\nperform in 10 seconds?",
    .eventFile = 0,
    .jumpTableIndex = JUMP_MULTISHINE,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = {
        .hmn = CSSID_FALCO | CSSID_FOX,
        .cpu = -1,
    },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc Reaction = {
    .eventName = "Reaction Test\n",
    .eventDescription = "Test your reaction time by pressing\nany button when you see/hear Fox shine!",
    .eventFile = 0,
    .jumpTableIndex = JUMP_REACTION,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = CKIND_FOX,
    .stage = GRKINDEXT_FD,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_KO,
    .callbackPriority = 3,
    .matchData = 0,
};

EventDesc Ledgestall = {
    .eventName = "Under Fire\n",
    .eventDescription = "Ledgestall to remain\ninvincible while the lava rises!\n",
    .eventFile = 0,
    .jumpTableIndex = JUMP_LEDGESTALL,
    .CSSType = SLCHRKIND_EVENT,
    .allowed_characters = { .hmn = -1, .cpu = -1 },
    .playerKind = -1,
    .cpuKind = -1,
    .stage = GRKINDEXT_ZEBES,
    .disable_hazards = true,
    .force_sopo = false,
    .scoreType = SCORETYPE_TIME,
    .callbackPriority = 3,
    .matchData = 0,
};

///////////////////////
/// Page Defintions ///
///////////////////////

// Minigames
static EventDesc *Minigames_Events[] = {
    &Eggs,
    &Multishine,
    &Reaction,
    &Ledgestall,
};
static EventPage Minigames_Page = {
    .name = "Minigames",
    .eventNum = countof(Minigames_Events),
    .events = Minigames_Events,
};

// Page 2 Events
static EventDesc *General_Events[] = {
    &Lab,
    &LCancel,
    &Ledgedash,
    &Wavedash,
    &Combo,
    &AttackOnShield,
    &Reversal,
    &SDI,
    &Powershield,
    &Ledgetech,
    &AmsahTech,
    &ShieldDrop,
    &WaveshineSDI,
    &SlideOff,
    &GrabMash,
};
static EventPage General_Page = {
    .name = "General Tech",
    .eventNum = countof(General_Events),
    .events = General_Events,
};

// Page 3 Events
static EventDesc *Spacie_Events[] = {
    &TechCounter,
    &Edgeguard,
    &SideBSweet,
    &EscapeSheik,
};
static EventPage Spacie_Page = {
    .name = "Character-specific Tech",
    .eventNum = countof(Spacie_Events),
    .events = Spacie_Events,
};

EventPage *EventPages[] = {
    &Minigames_Page,
    &General_Page,
    &Spacie_Page,
};

////////////////////////
/// Static Variables ///
////////////////////////

EventVars stc_event_vars = {
    .event_desc = 0,
    .menu_assets = 0,
    .event_gobj = 0,
    .menu_gobj = 0,
    .persistent_data = 0,
    .game_timer = 0,
    .Savestate_Save_v1 = Savestate_Save_v1,
    .Savestate_Load_v1 = Savestate_Load_v1,
    .Message_Display = Message_Display,
    .Tip_Display = Tip_Display,
    .Tip_Destroy = Tip_Destroy,
    .savestate = 0,
};

static GOBJ *stc_msgmgr;
static Savestate_v1 *stc_savestate;
static TipMgr stc_tipmgr;

static Vec2 stc_msg_queue_offsets_vertical[] = {
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, 5.15f},
    {0, -5.15f}
};
static Vec3 stc_msg_queue_pos_sides[] = {
    {-22.f, -13.f, 0},
    {-7.f, -13.f, 0},
    {7.f, -13.f, 0},
    {22.f, -13.f, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, -5.15f}
};

static Vec2 stc_msg_queue_offsets_horizontal[] = {
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {12.5f, 0},
    {-12.5f, 0},
};
static Vec3 stc_msg_queue_pos_top[] = {
    {-22.f, 20.f, 0},
    {-22.f, 14.f, 0},
    {-22.f, 8.f, 0},
    {-22.f, 2.f, 0},
    {0, 0, 0},
    {0, 0, 0},
    {22.f, 20.f, 0}
};
static GXColor stc_msg_colors[] = {
    {255, 255, 255, 255},
    {141, 255, 110, 255},
    {255, 162, 186, 255},
    {255, 240, 0, 255},
};

///////////////////////
/// Event Functions ///
///////////////////////

void EventInit(int page, int eventID, MatchInit *matchData)
{

    /*
    This function runs when leaving the main menu/css and handles
    setting up the match information, such as rules, players, stage.
    All of this data comes from the EventDesc in events.c
    */

    // get event pointer
    EventDesc *event = GetEventDesc(page, eventID);
    EventMatchData *eventMatchData = event->matchData;

    if (event->CSSType == SLCHRKIND_VS)
        memcpy(matchData, &(*stc_css_minorscene)->vs_data.match_init, sizeof(*matchData));

    //Init default match info
    matchData->timer_unk2 = 0;
    matchData->unk2 = 1;
    matchData->unk7 = 1;
    matchData->isCheckStockSteal = 1;
    matchData->unk1f = 3;
    matchData->no_check_end = 1;
    matchData->itemFreq = MATCH_ITEMFREQ_OFF;
    matchData->onStartMelee = EventLoad;
    matchData->isCheckForLRAStart = true;
    matchData->isHidePauseHUD = true;
    matchData->isDisablePause = true;

    //Copy event's match info struct
    matchData->timer = eventMatchData->timer;
    matchData->matchType = eventMatchData->matchType;
    matchData->hideGo = eventMatchData->hideGo;
    matchData->hideReady = eventMatchData->hideReady;
    matchData->isCreateHUD = eventMatchData->isCreateHUD;
    matchData->timerRunOnPause = eventMatchData->timerRunOnPause;
    matchData->isCheckForZRetry = eventMatchData->isCheckForZRetry;
    matchData->isShowScore = eventMatchData->isShowScore;
    matchData->isRunStockLogic = eventMatchData->isRunStockLogic;
    matchData->no_hit = eventMatchData->isDisableHit;
    matchData->timer_seconds = eventMatchData->timerSeconds;

    Preload *preload = Preload_GetTable();

    if (event->CSSType != SLCHRKIND_VS) {
        // Determine player ports
        u8 hmn_port = *stc_css_hmnport + 1;
        u8 cpu_port = *stc_css_cpuport + 1;

        // Initialize all player data
        for (int i = 0; i < 6; i++) {
          CSS_InitPlayerData(&matchData->playerData[i]);
          matchData->playerData[i].stocks = 1;
        }

        // Update the player's nametag ID and rumble
        u8 nametag = stc_memcard->EventBackup.nametag;
        matchData->playerData[0].nametag = nametag;
        matchData->playerData[0].isRumble = CSS_GetNametagRumble(0, nametag);

        // Determine the CPU
        s32 cpu_kind = event->cpuKind;
        s32 cpu_costume = 0;
        if (cpu_kind == -1 && event->CSSType == SLCHRKIND_TRAINING) {
            cpu_kind = preload->queued.fighters[1].kind;
            cpu_costume = preload->queued.fighters[1].costume;
        }

        if (cpu_kind == CKIND_ZELDA)
            cpu_kind = CKIND_SHEIK;

        if (cpu_kind != -1) {
            matchData->playerData[1].costume = cpu_costume;
            matchData->playerData[1].c_kind = cpu_kind;
            matchData->playerData[1].p_kind = PKIND_CPU;
            matchData->playerData[1].portNumberOverride = cpu_port;
        }

        matchData->playerData[0].c_kind = preload->queued.fighters[0].kind;
        matchData->playerData[0].costume = preload->queued.fighters[0].costume;
        matchData->playerData[0].p_kind = PKIND_HMN;
        matchData->playerData[0].portNumberOverride = hmn_port;

        // Force Popo if required
        if (event->force_sopo && matchData->playerData[0].c_kind == CKIND_ICECLIMBERS)
            matchData->playerData[0].c_kind = CKIND_POPO;

        // Determine the correct HUD position for this amount of players
        int hudPos = 0;
        for (int i = 0; i < 6; i++)
        {
            if (matchData->playerData[i].p_kind != PKIND_NONE)
                hudPos++;
        }
        matchData->hudPos = hudPos;
    }

    // set to enter fall on match start
    for (int i = 0; i < 6; i++)
        matchData->playerData[i].isEntry = false;

    // Determine the Stage
    if (event->stage == -1) {
        matchData->stage = preload->queued.stage;
    } else {
        matchData->stage = event->stage;
    }
};

void EventLoad(void)
{
    // get this event
    int page = stc_memcard->TM_EventPage;
    int eventID = stc_memcard->EventBackup.event;
    EventDesc *event_desc = GetEventDesc(page, eventID);
    evFunction *evFunction = &stc_event_vars.evFunction;

    // clear evFunction
    memset(evFunction, 0, sizeof(*evFunction));

    // append extension
    static char *extension = "TM/%s.dat";
    char *buffer[20];
    sprintf(buffer, extension, event_desc->eventFile);

    // load this events file
    HSD_Archive *archive = MEX_LoadRelArchive(buffer, evFunction, "evFunction");
    stc_event_vars.event_archive = archive;

    // Create this event's gobj
    int pri = event_desc->callbackPriority;
    void *cb = evFunction->Event_Think;
    GOBJ *gobj = GObj_Create(0, 7, 0);
    int *userdata = calloc(EVENT_DATASIZE);
    GObj_AddUserData(gobj, 4, HSD_Free, userdata);
    GObj_AddProc(gobj, cb, pri);

    // store pointer to the event's data
    userdata[0] = event_desc;

    // Create a gobj to track match time
    stc_event_vars.game_timer = 0;
    GOBJ *timer_gobj = GObj_Create(0, 7, 0);
    GObj_AddProc(timer_gobj, Event_IncTimer, 0);

    // init savestate struct
    stc_savestate = calloc(sizeof(Savestate_v1));
    stc_savestate->is_exist = 0;
    stc_event_vars.savestate = stc_savestate;

    // disable hazards if enabled
    if (event_desc->disable_hazards == 1)
        Hazards_Disable();

    // Store update function
    HSD_Update *update = stc_hsd_update;
    update->onFrame = EventUpdate;
    
    // Init static structure containing event variables
    stc_event_vars.event_desc = event_desc;
    stc_event_vars.event_gobj = gobj;

    // init the pause menu
    GOBJ *menu_gobj = EventMenu_Init(*evFunction->menu_start);
    stc_event_vars.menu_gobj = menu_gobj;
    
    // Run this event's init function
    if (evFunction->Event_Init != 0)
    {
        evFunction->Event_Init(gobj);
    }
};

void EventUpdate(void)
{
    GOBJ *menu_gobj = stc_event_vars.menu_gobj;
    if (menu_gobj)
        EventMenu_Update(menu_gobj);

    evFunction *evFunction = &stc_event_vars.evFunction;
    if (evFunction->Event_Update)
        evFunction->Event_Update();

    // This is the vanilla callback. This code handles the vanilla develop.
    // mode shortcuts. We could probably delete it if we want to.
    Develop_UpdateMatchHotkeys();
}

//////////////////////
/// Hook Functions ///
//////////////////////

void TM_ConsoleThink(GOBJ *gobj)
{
    DevText *text = gobj->userdata;

    // Toggle console with L/R + Z
    for (int i = 0; i < 4; i++)
    {
        HSD_Pad *pad = PadGet(i, PADGET_MASTER);
        if (pad->held & (HSD_TRIGGER_L | HSD_TRIGGER_R) && (pad->down & HSD_TRIGGER_Z))
        {
            text->show_text ^= 1;
            text->show_background ^= 1;
            break;
        }
    }
}
void TM_CreateConsole(void)
{
    // init dev text
    DevText *text = DevelopText_CreateDataTable(13, 0, 0, 32, 32, HSD_MemAlloc(0x1000));
    DevelopText_Activate(0, text);
    text->show_cursor = 0;

    GOBJ *gobj = GObj_Create(0, 0, 0);
    GObj_AddUserData(gobj, 4, HSD_Free, text);
    GObj_AddProc(gobj, TM_ConsoleThink, 0);

    GXColor color = {21, 20, 59, 80};
    DevelopText_StoreBGColor(text, &color);
    DevelopText_StoreTextScale(text, 10, 12);
    stc_event_vars.db_console_text = text;
}

void OnFileLoad(HSD_Archive *archive) // this function is run right after TmDt is loaded into memory on boot
{
    // init event menu assets
    stc_event_vars.menu_assets = Archive_GetPublicAddress(archive, "eventMenu");

    // store pointer to static variables
    *event_vars_ptr_loc = &stc_event_vars;
}

void OnSceneChange(void)
{
    // Hook exists at 801a4c94
    TM_CreateWatermark();

#if TM_DEBUG == 1   // Create and hide console
    TM_CreateConsole();
    stc_event_vars.db_console_text->show_text ^= 1;
    stc_event_vars.db_console_text->show_background ^= 1;
#elif TM_DEBUG == 2 // Create and show console
    TM_CreateConsole();
#endif
};

void OnBoot(void)
{
    // OSReport("hi this is boot\n");
};

void OnStartMelee(void)
{
    Message_Init();
    Tip_Init();
}

///////////////////////////////
/// Miscellaneous Functions ///
///////////////////////////////

int GOBJToID(GOBJ *gobj)
{
    // ensure valid pointer
    if (gobj == 0)
        return -1;

    // ensure its a fighter
    if (gobj->entity_class != 4)
        return -1;

    // access the data
    FighterData *ft_data = gobj->userdata;
    u8 ply = ft_data->ply;
    u8 ms = ft_data->flags.ms;

    return ((ply << 4) | ms);
}
int FtDataToID(FighterData *fighter_data)
{
    // ensure valid pointer
    if (fighter_data == 0)
        return -1;

    // ensure its a fighter
    if (fighter_data->fighter == 0)
        return -1;

    // get ply and ms
    u8 ply = fighter_data->ply;
    u8 ms = fighter_data->flags.ms;

    return ((ply << 4) | ms);
}
int BoneToID(FighterData *fighter_data, JOBJ *bone)
{

    // ensure bone exists
    if (bone == 0)
        return -1;

    int bone_id = -1;

    // painstakingly look for a match
    for (int i = 0; i < fighter_data->bone_num; i++)
    {
        if (bone == fighter_data->bones[i].joint)
        {
            bone_id = i;
            break;
        }
    }

    // no bone found
    if (bone_id == -1)
        TMLOG("no bone found %x\n", bone);

    return bone_id;
}
GOBJ *IDToGOBJ(int id)
{
    // ensure valid pointer
    if (id == -1)
        return 0;

    // get ply and ms
    u8 ply = (id >> 4) & 0xF;
    u8 ms = id & 0xF;

    // get the gobj for this fighter
    GOBJ *gobj = Fighter_GetSubcharGObj(ply, ms);

    return gobj;
}
FighterData *IDToFtData(int id)
{
    // ensure valid pointer
    if (id == -1)
        return 0;

    // get ply and ms
    u8 ply = (id >> 4) & 0xF;
    u8 ms = id & 0xF;

    // get the gobj for this fighter
    GOBJ *gobj = Fighter_GetSubcharGObj(ply, ms);
    FighterData *fighter_data = gobj->userdata;

    return fighter_data;
}
JOBJ *IDToBone(FighterData *fighter_data, int id)
{
    // ensure valid pointer
    if (id == -1)
        return 0;

    // get the bone
    JOBJ *bone = fighter_data->bones[id].joint;

    return bone;
}

void Event_IncTimer(GOBJ *gobj)
{
    stc_event_vars.game_timer++;
}
void TM_CreateWatermark(void)
{
    // create text canvas
    int canvas = Text_CreateCanvas(10, 0, 9, 13, 0, 14, 0, 19);

    // create text
    Text *text = Text_CreateText(10, canvas);
    // enable align and kerning
    text->align = 2;
    text->kerning = 1;
    // scale canvas
    text->viewport_scale.X = 0.4;
    text->viewport_scale.Y = 0.4;
    text->trans.X = 615;
    text->trans.Y = 446;

    // print string
    int shadow = Text_AddSubtext(text, 2, 2, TM_VersShort);
    GXColor shadow_color = {0, 0, 0, 0};
    Text_SetColor(text, shadow, &shadow_color);

    int shadow1 = Text_AddSubtext(text, 2, -2, TM_VersShort);
    Text_SetColor(text, shadow1, &shadow_color);

    int shadow2 = Text_AddSubtext(text, -2, 2, TM_VersShort);
    Text_SetColor(text, shadow2, &shadow_color);

    int shadow3 = Text_AddSubtext(text, -2, -2, TM_VersShort);
    Text_SetColor(text, shadow3, &shadow_color);

    Text_AddSubtext(text, 0, 0, TM_VersShort);
}
void Hazards_Disable(void)
{
    // get stage id
    int stage_internal = Stage_ExternalToInternal(Stage_GetExternalID());
    int is_fixwind = 0;

    switch (stage_internal)
    {
    case (GRKIND_STORY):
    {
        // remove shyguy map gobj proc
        GOBJ *shyguy_gobj = Stage_GetMapGObj(3);
        GObj_RemoveProc(shyguy_gobj);

        // remove randall
        GOBJ *randall_gobj = Stage_GetMapGObj(2);
        Stage_DestroyMapGObj(randall_gobj);

        is_fixwind = 1;

        break;
    }
    case (GRKIND_PSTAD):
    {
        // remove map gobj proc
        GOBJ *map_gobj = Stage_GetMapGObj(2);
        GObj_RemoveProc(map_gobj);

        is_fixwind = 1;

        break;
    }
    case (GRKIND_OLDPU):
    {
        // remove map gobj proc
        GOBJ *map_gobj = Stage_GetMapGObj(7);
        GObj_RemoveProc(map_gobj);

        // remove map gobj proc
        map_gobj = Stage_GetMapGObj(6);
        GObj_RemoveProc(map_gobj);

        // set wind hazard num to 0
        *ftchkdevice_windnum = 0;

        break;
    }
    case (GRKIND_FD):
    {
        // set bg skip flag
        GOBJ *map_gobj = Stage_GetMapGObj(3);
        MapData *map_data = map_gobj->userdata;
        map_data->xc4 |= 0x40;

        // remove on-go function that changes this flag
        StageOnGO *on_go = stc_stage->on_go;
        stc_stage->on_go = on_go->next;
        HSD_Free(on_go);

        break;
    }
    }

    // Certain stages have an essential ragdoll function
    // in their map_gobj think function. If the think function is removed,
    // the ragdoll function must be re-scheduled to function properly.
    if (is_fixwind == 1)
    {
        GOBJ *wind_gobj = GObj_Create(3, 5, 0);
        GObj_AddProc(wind_gobj, Dynamics_DecayWind, 4);
    }
}

// Message Functions
void Message_Init(void)
{

    // create cobj
    GOBJ *cam_gobj = GObj_Create(19, 20, 0);
    COBJDesc *cam_desc = stc_event_vars.menu_assets->hud_cobjdesc;
    COBJ *cam_cobj = COBJ_LoadDescSetScissor(cam_desc);
    cam_cobj->scissor_bottom = 400;
    // init camera
    GObj_AddObject(cam_gobj, R13_U8(-0x3E55), cam_cobj); //R13_U8(-0x3E55)
    GOBJ_InitCamera(cam_gobj, Message_CObjThink, MSG_COBJLGXPRI);
    cam_gobj->cobj_links = MSG_COBJLGXLINKS;

    // Create manager GOBJ
    GOBJ *mgr_gobj = GObj_Create(0, 7, 0);
    MsgMngrData *mgr_data = calloc(sizeof(MsgMngrData));
    GObj_AddUserData(mgr_gobj, 4, HSD_Free, mgr_data);
    GObj_AddProc(mgr_gobj, Message_Manager, 18);

    // create text canvas
    int canvas = Text_CreateCanvas(2, mgr_gobj, 14, 15, 0, MSG_GXLINK, MSGTEXT_GXPRI, 19);
    mgr_data->canvas = canvas;

    // store cobj
    mgr_data->cobj = cam_cobj;

    // store gobj pointer
    stc_msgmgr = mgr_gobj;
}
GOBJ *Message_Display(int msg_kind, int queue_num, int msg_color, char *format, ...)
{

    va_list args;

    MsgMngrData *mgr_data = stc_msgmgr->userdata;

    // Create GOBJ
    GOBJ *msg_gobj = GObj_Create(0, 7, 0);
    MsgData *msg_data = calloc(sizeof(MsgData));
    GObj_AddUserData(msg_gobj, 4, HSD_Free, msg_data);
    GObj_AddGXLink(msg_gobj, GXLink_Common, MSG_GXLINK, MSG_GXPRI);
    JOBJ *msg_jobj = JOBJ_LoadJoint(stc_event_vars.menu_assets->message);
    GObj_AddObject(msg_gobj, R13_U8(-0x3E55), msg_jobj);
    msg_data->lifetime = MSG_LIFETIME;
    msg_data->kind = msg_kind;
    msg_data->state = MSGSTATE_SHIFT;
    msg_data->anim_timer = MSGTIMER_SHIFT;
    msg_jobj->scale.X = MSGJOINT_SCALE;
    msg_jobj->scale.Y = MSGJOINT_SCALE;
    msg_jobj->scale.Z = MSGJOINT_SCALE;
    msg_jobj->trans.X = MSGJOINT_X;
    msg_jobj->trans.Y = MSGJOINT_Y;
    msg_jobj->trans.Z = MSGJOINT_Z;

    // Create text object
    Text *msg_text = Text_CreateText(2, mgr_data->canvas);
    msg_data->text = msg_text;
    msg_text->kerning = 1;
    msg_text->align = 1;
    msg_text->use_aspect = 1;
    msg_text->color = stc_msg_colors[msg_color];

    // adjust scale
    Vec3 scale = msg_jobj->scale;
    // background scale
    msg_jobj->scale = scale;
    // text scale
    msg_text->viewport_scale.X = (scale.X * 0.01) * MSGTEXT_BASESCALE;
    msg_text->viewport_scale.Y = (scale.Y * 0.01) * MSGTEXT_BASESCALE;
    msg_text->aspect.X = MSGTEXT_BASEWIDTH;

    JOBJ_SetMtxDirtySub(msg_jobj);

    // build string
    char buffer[(MSG_LINEMAX * MSG_CHARMAX) + 1];
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    char *msg = &buffer;

    // count newlines
    int line_num = 1;
    int line_length_arr[MSG_LINEMAX];
    char *msg_cursor_prev, *msg_cursor_curr; // declare char pointers
    msg_cursor_prev = msg;
    msg_cursor_curr = strchr(msg_cursor_prev, '\n'); // check for occurrence
    while (msg_cursor_curr != 0)                     // if occurrence found, increment values
    {
        // check if exceeds max lines
        if (line_num >= MSG_LINEMAX)
            assert("MSG_LINEMAX exceeded!");

        // Save information about this line
        line_length_arr[line_num - 1] = msg_cursor_curr - msg_cursor_prev; // determine length of the line
        line_num++;                                                        // increment number of newlines found
        msg_cursor_prev = msg_cursor_curr + 1;                             // update prev cursor
        msg_cursor_curr = strchr(msg_cursor_prev, '\n');                   // check for another occurrence
    }

    // get last lines length
    msg_cursor_curr = strchr(msg_cursor_prev, 0);
    line_length_arr[line_num - 1] = msg_cursor_curr - msg_cursor_prev;

    // copy each line to an individual char array
    char *msg_cursor = &msg;
    for (int i = 0; i < line_num; i++)
    {

        // check if over char max
        u8 line_length = line_length_arr[i];
        if (line_length > MSG_CHARMAX)
            assert("MSG_CHARMAX exceeded!");

        // copy char array
        char msg_line[MSG_CHARMAX + 1];
        memcpy(msg_line, msg, line_length);

        // add null terminator
        msg_line[line_length] = '\0';

        // increment msg
        msg += (line_length + 1); // +1 to skip past newline

        // print line
        int y_base = (line_num - 1) * ((-1 * MSGTEXT_YOFFSET) / 2);
        int y_delta = (i * MSGTEXT_YOFFSET);
        Text_AddSubtext(msg_text, 0, y_base + y_delta, msg_line);
    }

    // Add to queue
    Message_Add(msg_gobj, queue_num);

    return msg_gobj;
}
void Message_Manager(GOBJ *mngr_gobj)
{
    MsgMngrData *mgr_data = mngr_gobj->userdata;

    // Iterate through each queue
    for (int i = 0; i < MSGQUEUE_NUM; i++)
    {
        GOBJ **msg_queue = &mgr_data->msg_queue[i];

        // anim update (time based logic)
        for (int j = (MSGQUEUE_SIZE - 2); j >= 0; j--) // iterate through backwards (because deletions)
        {
            GOBJ *this_msg_gobj = msg_queue[j];

            // if message exists
            if (this_msg_gobj != 0)
            {
                MsgData *this_msg_data = this_msg_gobj->userdata;
                Text *this_msg_text = this_msg_data->text;
                JOBJ *this_msg_jobj = this_msg_gobj->hsd_object;

                // check if the message moved this frame
                if (this_msg_data->orig_index != j)
                {
                    this_msg_data->orig_index = j;              // moved so update this
                    this_msg_data->state = MSGSTATE_SHIFT;      // enter shift
                    this_msg_data->anim_timer = MSGTIMER_SHIFT; // shift timer
                }

                // decrement state timer if above 0
                if (this_msg_data->anim_timer > 0)
                    this_msg_data->anim_timer--;

                switch (this_msg_data->state)
                {
                case (MSGSTATE_WAIT):
                case (MSGSTATE_SHIFT):
                {

                    // increment alive time
                    this_msg_data->alive_timer++;

                    // if lifetime is ended, enter delete state
                    if (this_msg_data->alive_timer >= this_msg_data->lifetime)
                    {
                        // if using frame advance, instantly remove this message
                        if (Pause_CheckStatus(0) == 1)
                        {
                            Message_Destroy(msg_queue, j);
                        }
                        else
                        {
                            this_msg_data->state = MSGSTATE_DELETE;
                            this_msg_data->anim_timer = MSGTIMER_DELETE;
                        }
                    }

                    break;
                }

                case (MSGSTATE_DELETE):
                {

                    // if timer is ended, remove the message
                    if ((this_msg_data->anim_timer <= 0))
                    {
                        Message_Destroy(msg_queue, j);
                    }

                    break;
                }
                }
            }
        }

        // position update (update messages' onscreen positions)
        for (int j = 0; j < MSGQUEUE_SIZE; j++)
        {
            GOBJ *this_msg_gobj = msg_queue[j];

            // if message exists
            if (this_msg_gobj != 0)
            {
                MsgData *this_msg_data = this_msg_gobj->userdata;
                Text *this_msg_text = this_msg_data->text;
                JOBJ *this_msg_jobj = this_msg_gobj->hsd_object;

                int osd_pos_type = stc_memcard->TM_OSDPosition;
                if (osd_pos_type > 3)
                    osd_pos_type = 0;

                Vec3 base_pos;
                Vec2 pos_delta;

                if (osd_pos_type == 0) {
                    // above hud
                    Vec3 *hud_pos = Match_GetPlayerHUDPos(i);

                    base_pos.X = hud_pos->X;
                    base_pos.Y = hud_pos->Y + MSG_HUDYOFFSET;
                    base_pos.Z = hud_pos->Z;
                    pos_delta = stc_msg_queue_offsets_vertical[i];
                } else if (osd_pos_type == 1) {
                    // sides
                    base_pos = stc_msg_queue_pos_sides[i];
                    pos_delta = stc_msg_queue_offsets_vertical[i];
                } else {
                    // top
                    base_pos = stc_msg_queue_pos_top[i];
                    pos_delta = stc_msg_queue_offsets_horizontal[i];
                    if (osd_pos_type == 3) {
                        // top right
                        base_pos.X *= -1;
                        pos_delta.X *= -1;
                    }
                }

                // Get the onscreen position for this queue
                //float pos_delta = stc_msg_queue_offsets[i];

                Vec3 this_msg_pos = {0, 0, 0};

                switch (this_msg_data->state)
                {
                case (MSGSTATE_WAIT):
                case (MSGSTATE_SHIFT):
                {

                    // get time
                    float t = ((float)MSGTIMER_SHIFT - this_msg_data->anim_timer) / MSGTIMER_SHIFT;

                    // get initial and final position for animation
                    float final_x = base_pos.X + (float)j * pos_delta.X;
                    float final_y = base_pos.Y + (float)j * pos_delta.Y;
                    float initial_x = base_pos.X + (float)this_msg_data->prev_index * pos_delta.X;
                    float initial_y = base_pos.Y + (float)this_msg_data->prev_index * pos_delta.Y;
                    if (Pause_CheckStatus(0) == 1) { // if using frame advance, do not animate
                        this_msg_pos.X = final_x;
                        this_msg_pos.Y = final_y;
                    } else {
                        this_msg_pos.X = smooth_lerp(t, initial_x, final_x);
                        this_msg_pos.Y = smooth_lerp(t, initial_y, final_y);
                    }

                    Vec3 scale = this_msg_jobj->scale;

                    // BG position
                    this_msg_jobj->trans.X = this_msg_pos.X;
                    this_msg_jobj->trans.Y = this_msg_pos.Y;
                    // text position
                    this_msg_text->trans.X = this_msg_pos.X + (MSGTEXT_BASEX * (scale.X / 4.0));
                    this_msg_text->trans.Y = (this_msg_pos.Y * -1) + (MSGTEXT_BASEY * (scale.Y / 4.0));

                    // adjust bar
                    JOBJ *bar;
                    JOBJ_GetChild(this_msg_jobj, &bar, 4, -1);
                    bar->trans.X = (float)(this_msg_data->lifetime - this_msg_data->alive_timer) / (float)this_msg_data->lifetime;

                    break;
                }
                case (MSGSTATE_DELETE):
                {
                    // get time
                    float t = ((this_msg_data->anim_timer) / (float)MSGTIMER_DELETE);

                    Vec3 *scale = &this_msg_jobj->scale;
                    Vec3 *pos = &this_msg_jobj->trans;

                    // BG scale
                    scale->Y = smooth_lerp(t,  0.0, 1.0);
                    // text scale
                    this_msg_text->viewport_scale.Y = (scale->Y * 0.01) * MSGTEXT_BASESCALE;
                    // text position
                    this_msg_text->trans.Y = (pos->Y * -1) + (MSGTEXT_BASEY * (scale->Y / 4.0));

                    break;
                }
                }

                JOBJ_SetMtxDirtySub(this_msg_jobj);
            }
        }
    }
}
void Message_Destroy(GOBJ **msg_queue, int msg_num)
{

    GOBJ *msg_gobj = msg_queue[msg_num];
    MsgData *msg_data = msg_gobj->userdata;

    // Destroy text
    Text *text = msg_data->text;
    if (text != 0)
        Text_Destroy(text);

    // Destroy GOBJ
    GObj_Destroy(msg_gobj);

    // null pointer
    msg_queue[msg_num] = 0;

    // shift others
    for (int i = (msg_num); i < (MSGQUEUE_SIZE - 1); i++)
    {
        msg_queue[i] = msg_queue[i + 1];

        // update its prev pos
        GOBJ *this_msg_gobj = msg_queue[i];
        if (this_msg_gobj != 0)
        {
            MsgData *this_msg_data = this_msg_gobj->userdata;
            if (this_msg_data != 0)
                this_msg_data->prev_index = i + 1; // prev position
        }
    }
}
void Message_Add(GOBJ *msg_gobj, int queue_num)
{

    MsgData *msg_data = msg_gobj->userdata;
    MsgMngrData *mgr_data = stc_msgmgr->userdata;
    GOBJ **msg_queue = &mgr_data->msg_queue[queue_num];

    // ensure this queue exists
    if (queue_num >= MSGQUEUE_NUM)
        assert("queue_num over!");

    // msg kind -1 always sticks around
    if (msg_data->kind != -1) {
        // remove any existing messages of this kind
        for (int i = 0; i < MSGQUEUE_SIZE; i++)
        {
            GOBJ *this_msg_gobj = msg_queue[i];

            // if it exists
            if (this_msg_gobj != 0)
            {
                MsgData *this_msg_data = this_msg_gobj->userdata;

                // Remove this message if its of the same kind
                if ((this_msg_data->kind == msg_data->kind))
                {

                    Message_Destroy(msg_queue, i); // remove the message and shift others

                    // if the message we're replacing is the most recent message, instantly
                    // remove the old one and do not animate the new one
                    if (i == 0)
                    {
                        msg_data->state = MSGSTATE_WAIT;
                        msg_data->anim_timer = 0;
                    }
                }
            }
        }
    }

    // first remove last message in the queue
    if (msg_queue[MSGQUEUE_SIZE - 1] != 0)
    {
        Message_Destroy(msg_queue, MSGQUEUE_SIZE - 1);
    }

    // shift other messages
    for (int i = (MSGQUEUE_SIZE - 2); i >= 0; i--)
    {
        // shift message
        msg_queue[i + 1] = msg_queue[i];

        // update its prev pos
        GOBJ *this_msg_gobj = msg_queue[i + 1];
        if (this_msg_gobj != 0)
        {
            MsgData *this_msg_data = this_msg_gobj->userdata;
            this_msg_data->prev_index = i; // prev position
        }
    }

    // add this new message
    msg_queue[0] = msg_gobj;

    // set prev pos to -1 (slides in)
    msg_data->prev_index = -1;
    msg_data->orig_index = 0;
}
void Message_CObjThink(GOBJ *gobj)
{
    if (Pause_CheckStatus(1) != 2)
        CObjThink_Common(gobj);
}

// Tips Functions
void Tip_Init(void)
{
    // init static struct
    memset(&stc_tipmgr, 0, sizeof(TipMgr));

    // create tipmgr gobj
    GOBJ *tipmgr_gobj = GObj_Create(0, 7, 0);
    GObj_AddProc(tipmgr_gobj, Tip_Think, 18);
}
void Tip_Think(GOBJ *gobj)
{

    GOBJ *tip_gobj = stc_tipmgr.gobj;

    stc_event_vars.menu_assets->tip_jobj;

    // update tip
    if (tip_gobj != 0)
    {

        // update anim
        JOBJ_AnimAll(tip_gobj->hsd_object);

        // update text position
        JOBJ *tip_jobj;
        Vec3 tip_pos;
        JOBJ_GetChild(tip_gobj->hsd_object, &tip_jobj, TIP_TXTJOINT, -1);
        JOBJ_GetWorldPosition(tip_jobj, 0, &tip_pos);
        Text *tip_text = stc_tipmgr.text;
        tip_text->trans.X = tip_pos.X + (0 * (tip_jobj->scale.X / 4.0));
        tip_text->trans.Y = (tip_pos.Y * -1) + (0 * (tip_jobj->scale.Y / 4.0));

        // state logic
        switch (stc_tipmgr.state)
        {
        case (0): // in
        {
            // if anim is done, enter wait
            if (JOBJ_CheckAObjEnd(tip_gobj->hsd_object) == 0)
                stc_tipmgr.state = 1; // enter wait

            break;
        }
        case (1): // wait
        {
            // sub timer
            stc_tipmgr.lifetime--;
            if (stc_tipmgr.lifetime <= 0)
            {
                // apply exit anim
                JOBJ *tip_root = tip_gobj->hsd_object;
                JOBJ_RemoveAnimAll(tip_root);
                JOBJ_AddAnimAll(tip_root, stc_event_vars.menu_assets->tip_jointanim[1], 0, 0);
                JOBJ_ReqAnimAll(tip_root, 0);

                stc_tipmgr.state = 2; // enter wait
            }

            break;
        }
        case (2): // out
        {
            // if anim is done, destroy
            if (JOBJ_CheckAObjEnd(tip_gobj->hsd_object) == 0)
            {
                // remove text
                Text_Destroy(stc_tipmgr.text);
                GObj_Destroy(stc_tipmgr.gobj);
                stc_tipmgr.gobj = 0;
            }
            break;
        }
        }
    }
}
int Tip_Display(int lifetime, char *fmt, ...)
{

#define TIP_TXTSIZE 4.7
#define TIP_TXTSIZEX TIP_TXTSIZE * 0.85
#define TIP_TXTSIZEY TIP_TXTSIZE
#define TIP_TXTASPECT 2430
#define TIP_LINEMAX 5
#define TIP_CHARMAX 48

    va_list args;

    // if tip exists
    if (stc_tipmgr.gobj != 0)
    {
        // if tip is in the process of exiting
        if (stc_tipmgr.state == 2)
        {
            // remove text
            Text_Destroy(stc_tipmgr.text);
            GObj_Destroy(stc_tipmgr.gobj);
            stc_tipmgr.gobj = 0;
        }
        // if is active onscreen do nothing
        else
            return 0;
    }

    MsgMngrData *msgmngr_data = stc_msgmgr->userdata; // using message canvas cause there are so many god damn text canvases

    // Create bg
    GOBJ *tip_gobj = GObj_Create(0, 0, 0);
    stc_tipmgr.gobj = tip_gobj;
    GObj_AddGXLink(tip_gobj, GXLink_Common, MSG_GXLINK, 80);
    JOBJ *tip_jobj = JOBJ_LoadJoint(stc_event_vars.menu_assets->tip_jobj);
    GObj_AddObject(tip_gobj, R13_U8(-0x3E55), tip_jobj);

    // account for widescreen
    /*
    float aspect = (msgmngr_data->cobj->projection_param.perspective.aspect / 1.216667) - 1;
    tip_jobj->trans.X += (tip_jobj->trans.X * aspect);
    JOBJ_SetMtxDirtySub(tip_jobj);
    */

    // Create text object
    Text *tip_text = Text_CreateText(2, msgmngr_data->canvas);
    stc_tipmgr.text = tip_text;
    stc_tipmgr.lifetime = lifetime;
    stc_tipmgr.state = 0;
    tip_text->kerning = 1;
    tip_text->align = 0;
    tip_text->use_aspect = 1;

    // adjust text scale
    Vec3 scale = tip_jobj->scale;
    // background scale
    tip_jobj->scale = scale;
    // text scale
    tip_text->viewport_scale.X = (scale.X * 0.01) * TIP_TXTSIZEX;
    tip_text->viewport_scale.Y = (scale.Y * 0.01) * TIP_TXTSIZEY;
    tip_text->aspect.X = (TIP_TXTASPECT / TIP_TXTSIZEX);

    // apply enter anim
    JOBJ_RemoveAnimAll(tip_jobj);
    JOBJ_AddAnimAll(tip_jobj, stc_event_vars.menu_assets->tip_jointanim[0], 0, 0);
    JOBJ_ReqAnimAll(tip_jobj, 0);

    // build string
    char buffer[(TIP_LINEMAX * TIP_CHARMAX) + 1];
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    char *msg = &buffer;

    // count newlines
    int line_num = 1;
    int line_length_arr[TIP_LINEMAX];
    char *msg_cursor_prev, *msg_cursor_curr; // declare char pointers
    msg_cursor_prev = msg;
    msg_cursor_curr = strchr(msg_cursor_prev, '\n'); // check for occurrence
    while (msg_cursor_curr != 0)                     // if occurrence found, increment values
    {
        // check if exceeds max lines
        if (line_num >= TIP_LINEMAX)
            assert("TIP_LINEMAX exceeded!");

        // Save information about this line
        line_length_arr[line_num - 1] = msg_cursor_curr - msg_cursor_prev; // determine length of the line
        line_num++;                                                        // increment number of newlines found
        msg_cursor_prev = msg_cursor_curr + 1;                             // update prev cursor
        msg_cursor_curr = strchr(msg_cursor_prev, '\n');                   // check for another occurrence
    }

    // get last lines length
    msg_cursor_curr = strchr(msg_cursor_prev, 0);
    line_length_arr[line_num - 1] = msg_cursor_curr - msg_cursor_prev;

    // copy each line to an individual char array
    char *msg_cursor = &msg;
    for (int i = 0; i < line_num; i++)
    {

        // check if over char max
        u8 line_length = line_length_arr[i];
        if (line_length > TIP_CHARMAX)
            assert("TIP_CHARMAX exceeded!");

        // copy char array
        char msg_line[TIP_CHARMAX + 1];
        memcpy(msg_line, msg, line_length);

        // add null terminator
        msg_line[line_length] = '\0';

        // increment msg
        msg += (line_length + 1); // +1 to skip past newline

        // print line
        int y_delta = (i * MSGTEXT_YOFFSET);
        Text_AddSubtext(tip_text, 0, y_delta, msg_line);
    }

    return 1; // tip created
}
void Tip_Destroy(void)
{
    // check if tip exists and isnt in exit state, enter exit
    if ((stc_tipmgr.gobj != 0) && (stc_tipmgr.state != 2))
    {
        // apply exit anim
        JOBJ *tip_root = stc_tipmgr.gobj->hsd_object;
        JOBJ_RemoveAnimAll(tip_root);
        JOBJ_AddAnimAll(tip_root, stc_event_vars.menu_assets->tip_jointanim[1], 0, 0);
        JOBJ_ReqAnimAll(tip_root, 0);
        JOBJ_ForEachAnim(tip_root, 6, 0xfb7f, AOBJ_SetRate, 1, (float)2);

        stc_tipmgr.state = 2; // enter wait
    }
}

///////////////////////////////
/// Member-Access Functions ///
///////////////////////////////

EventDesc *GetEventDesc(int page, int event)
{
    return EventPages[page]->events[event];
}
char *GetEventName(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->eventName;
}
char *GetEventDescription(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->eventDescription;
}
char *GetPageName(int page)
{
    return EventPages[page]->name;
}
char *GetEventFile(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->eventFile;
}
char *GetCSSFile(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->eventCSSFile;
}
int GetPageEventNum(int page)
{
    return EventPages[page]->eventNum - 1;
}
char *GetTMVersShort(void)
{
    return TM_VersShort;
}
char *GetTMVersLong(void)
{
    return TM_VersLong;
}
char *GetTMCompile(void)
{
    return TM_Compile;
}
int GetPageNum(void)
{
    return countof(EventPages) - 1;
}
u8 GetCSSType(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->CSSType;
}
u8 GetIsSelectStage(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->stage == -1;
}
s8 GetFighter(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->playerKind;
}
s8 GetCPUFighter(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->cpuKind;
}
s16 GetStage(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->stage;
}
u8 GetScoreType(int page, int event)
{
    EventDesc *desc = GetEventDesc(page, event);
    return desc->scoreType;
}
int GetPageEventOffset(int pageID) {
    int eventIndex = 0;
    // Add the number of events for each previous page
    for(int i = 0; i < pageID; i++) {
        eventIndex += EventPages[i]->eventNum;
    }
    return eventIndex;
}
int GetJumpTableOffset(int pageID, int eventID) {
    EventDesc *desc = GetEventDesc(pageID, eventID);
    return desc->jumpTableIndex;
}
AllowedCharacters *GetEventCharList(int eventID,int pageID) {
    EventDesc *desc = GetEventDesc(pageID, eventID);
    return &desc->allowed_characters;
}
