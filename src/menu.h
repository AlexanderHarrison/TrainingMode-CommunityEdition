#include "../MexTK/mex.h"
#include <stdint.h>

#define MENU_MAXOPTION 9
#define MENU_POPMAXOPTION 5

// Custom File Structs
typedef struct evMenu
{
    JOBJDesc *menu;
    JOBJDesc *popup;
    JOBJDesc *scroll;
    JOBJDesc *check;
    JOBJDesc *arrow;
    JOBJDesc *playback;
    JOBJDesc *message;
    COBJDesc *hud_cobjdesc;
    JOBJ *tip_jobj;
    void **tip_jointanim; // pointer to array
} evMenu;

// Structure Definitions
typedef struct EventMenu EventMenu;
typedef struct EventOption
{
    u8 kind;                                        // the type of option this is; string, integers, etc
    u8 disable;                                     // boolean for disabling the option
    s16 value_min;                                  // minimum value
    u16 value_num;                                  // number of values
    s16 val;                                        // value of this option
    s16 val_prev;                                   // previous value of this option
    EventMenu *menu;                                // pointer to the menu that pressing A opens
    char *name;                                     // pointer to the name of this option
    char *desc;                                     // pointer to the description string for this option
    void **values;                                  // pointer to an array of strings
    void (*OnChange)(GOBJ *menu_gobj, int value);   // function that runs when option is changed
    void (*OnSelect)(GOBJ *menu_gobj);              // function that runs when option is selected
} EventOption;
typedef struct Shortcut {
    int buttons_mask;
    EventOption *option;
} Shortcut;
typedef struct ShortcutList {
    int count;
    Shortcut *list;
} ShortcutList;
struct EventMenu
{
    char *name;                    // name of this menu
    u8 option_num;                 // number of options this menu contains
    u8 scroll;                     //
    u8 state;                      // bool used to know if this menu is focused
    u8 cursor;                     // index of the option currently selected
    EventOption *options;          // pointer to all of this menu's options
    EventMenu *prev;               // pointer to previous menu, used at runtime
    int (*menu_think)(GOBJ *menu); // function that runs every frame.
    ShortcutList *shortcuts;       // pointer to shortcuts when shortcut mode is entered on this menu
};
typedef enum MenuMode {
    MenuMode_Normal,
    MenuMode_Paused,
    MenuMode_Shortcut,
    MenuMode_ShortcutWaitForRelease,
} MenuMode;
typedef struct MenuData
{
    EventMenu *currMenu;
    u16 canvas_menu;
    u16 canvas_popup;
    u8 mode;
    u8 controller_index; // index of the controller who paused
    Text *text_name;
    Text *text_value;
    Text *text_popup;
    Text *text_title;
    Text *text_desc;
    u16 popup_cursor;
    u16 popup_scroll;
    GOBJ *popup;
    evMenu *menu_assets;
    JOBJ *row_joints[MENU_MAXOPTION][2]; // pointers to row jobjs
    JOBJ *highlight_menu;                // pointer to the highlight jobj
    JOBJ *highlight_popup;               // pointer to the highlight jobj
    JOBJ *scroll_top;
    JOBJ *scroll_bot;
    GOBJ *custom_gobj;                               // onSelect gobj
    int (*custom_gobj_think)(GOBJ *custom_gobj);     // per frame function. Returns bool indicating if the program should check to unpause
    void *(*custom_gobj_destroy)(GOBJ *custom_gobj); // on destroy function
} MenuData;


GOBJ *EventMenu_Init(EventMenu *start_menu);
void EventMenu_Think(GOBJ *eventMenu, int pass);
void EventMenu_COBJThink(GOBJ *gobj);
void EventMenu_Draw(GOBJ *eventMenu);

// EventOption kind definitions
enum option_kind {
    OPTKIND_MENU,
    OPTKIND_STRING,
    OPTKIND_INT,
    OPTKIND_FLOAT,
    OPTKIND_FUNC,
};

// EventMenu state definitions
enum event_menu_state {
    EMSTATE_FOCUS,
    EMSTATE_OPENSUB,
    EMSTATE_OPENPOP,
    EMSTATE_WAIT, // pauses menu logic, used for when a custom window is being shown
};

// GX Link args
#define GXLINK_MENUMODEL 12
#define GXPRI_MENUMODEL 80
#define GXLINK_MENUTEXT 12
#define GXPRI_MENUTEXT GXPRI_MENUMODEL + 1
// popup menu
#define GXPRI_POPUPMODEL GXPRI_MENUTEXT + 1
#define GXLINK_POPUPMODEL 12
#define GXPRI_POPUPTEXT GXPRI_POPUPMODEL + 1
#define GXLINK_POPUPTEXT 12
// cobj
#define MENUCAM_COBJGXLINK (1 << GXLINK_MENUMODEL) | (1 << GXLINK_MENUTEXT) | (1 << GXLINK_POPUPMODEL) | (1 << GXLINK_POPUPTEXT)
#define MENUCAM_GXPRI 9

// menu model
#define OPT_SCALE 1
#define OPT_X 0 //0.5
#define OPT_Y -1
#define OPT_Z 0
#define OPT_WIDTH 55 / OPT_SCALE
#define OPT_HEIGHT 40 / OPT_SCALE
// menu text object
#define MENU_CANVASSCALE 0.05
#define MENU_TEXTSCALE 1
#define MENU_TEXTZ 0
// menu title
#define MENU_TITLEXPOS -430
#define MENU_TITLEYPOS -366
#define MENU_TITLESCALE 2.3
#define MENU_TITLEASPECT 870
// menu description
#define MENU_DESCXPOS -21.5
#define MENU_DESCYPOS 12
#define MENU_DESCSCALE 1
// menu option name
#define MENU_OPTIONNAMEXPOS -430
#define MENU_OPTIONNAMEYPOS -230
#define MENU_NAMEASPECT 440
// menu option value
#define MENU_OPTIONVALXPOS 250
#define MENU_OPTIONVALYPOS -230
#define MENU_TEXTYOFFSET 50
#define MENU_VALASPECT 280
// menu highlight
#define MENUHIGHLIGHT_SCALE 1 // OPT_SCALE
#define MENUHIGHLIGHT_HEIGHT ROWBOX_HEIGHT
#define MENUHIGHLIGHT_WIDTH (OPT_WIDTH * 0.785)
#define MENUHIGHLIGHT_X OPT_X
#define MENUHIGHLIGHT_Y 10.8 //10.3
#define MENUHIGHLIGHT_Z 0.01
#define MENUHIGHLIGHT_YOFFSET ROWBOX_YOFFSET
#define MENUHIGHLIGHT_COLOR \
    {                       \
        255, 211, 0, 255    \
    }
// menu scroll
#define MENUSCROLL_SCALE 2                         // OPT_SCALE
#define MENUSCROLL_SCALEY 1.105 * MENUSCROLL_SCALE // OPT_SCALE
#define MENUSCROLL_X 22.5
#define MENUSCROLL_Y 12
#define MENUSCROLL_Z 0.01
#define MENUSCROLL_PEROPTION 1
#define MENUSCROLL_MINLENGTH -1
#define MENUSCROLL_MAXLENGTH -10
#define MENUSCROLL_COLOR \
    {                    \
        255, 211, 0, 255 \
    }

// row jobj
#define ROWBOX_HEIGHT 2.3
#define ROWBOX_WIDTH 18
#define ROWBOX_X 12.5 //13
#define ROWBOX_Y 10.8 //10.3
#define ROWBOX_Z 0
#define ROWBOX_YOFFSET -2.5
#define ROWBOX_COLOR       \
    {                      \
        104, 105, 129, 100 \
    }
// arrow jobj
#define TICKBOX_SCALE 1.8
#define TICKBOX_X 11.7
#define TICKBOX_Y 11.7

// popup model
#define POPUP_WIDTH ROWBOX_WIDTH
#define POPUP_HEIGHT 19
#define POPUP_SCALE 1
#define POPUP_X 12.5
#define POPUP_Y 8.3
#define POPUP_Z 0.01
#define POPUP_YOFFSET -2.5
// popup text object
#define POPUP_CANVASSCALE 0.05
#define POPUP_TEXTSCALE 1
#define POPUP_TEXTZ 0.01
// popup text
#define POPUP_OPTIONVALXPOS 250
#define POPUP_OPTIONVALYPOS -280
#define POPUP_TEXTYOFFSET 50
// popup highlight
#define POPUPHIGHLIGHT_HEIGHT ROWBOX_HEIGHT
#define POPUPHIGHLIGHT_WIDTH (POPUP_WIDTH * 0.785)
#define POPUPHIGHLIGHT_X 0
#define POPUPHIGHLIGHT_Y 5
#define POPUPHIGHLIGHT_Z 1
#define POPUPHIGHLIGHT_YOFFSET ROWBOX_YOFFSET
#define POPUPHIGHLIGHT_COLOR \
    {                        \
        255, 211, 0, 255     \
    }
