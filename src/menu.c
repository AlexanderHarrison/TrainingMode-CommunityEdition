#include "events.h"
#include <stdarg.h>

static char nullString[] = " ";

GOBJ *EventMenu_Init(EventMenu *start_menu)
{
    event_vars = *event_vars_ptr;

    // Ensure this event has a menu
    if (start_menu == 0)
        return 0;

    // Create a cobj for the event menu
    COBJDesc ***dmgScnMdls = Archive_GetPublicAddress(*stc_ifall_archive, 0x803f94d0);
    COBJDesc *cam_desc = dmgScnMdls[1][0];
    COBJ *cam_cobj = COBJ_LoadDesc(cam_desc);
    GOBJ *cam_gobj = GObj_Create(19, 20, 0);
    GObj_AddObject(cam_gobj, R13_U8(-0x3E55), cam_cobj);
    GOBJ_InitCamera(cam_gobj, CObjThink_Common, MENUCAM_GXPRI);
    cam_gobj->cobj_links = MENUCAM_COBJGXLINK;

    // Create menu gobj
    GOBJ *gobj = GObj_Create(0, 0, 0);
    MenuData *menuData = calloc(sizeof(MenuData));
    GObj_AddUserData(gobj, 4, HSD_Free, menuData);

    // Add gx_link
    GObj_AddGXLink(gobj, GXLink_Common, GXLINK_MENUMODEL, GXPRI_MENUMODEL);

    // Create 2 text canvases (menu and popup)
    menuData->canvas_menu = Text_CreateCanvas(2, cam_gobj, 9, 13, 0, GXLINK_MENUTEXT, GXPRI_MENUTEXT, MENUCAM_GXPRI);
    menuData->canvas_popup = Text_CreateCanvas(2, cam_gobj, 9, 13, 0, GXLINK_MENUTEXT, GXPRI_POPUPTEXT, MENUCAM_GXPRI);

    // Init currMenu
    menuData->currMenu = start_menu;

    // set menu as not hidden
    event_vars->hide_menu = 0;

    return gobj;
};

void EventMenu_Update(GOBJ *gobj)
{

    //MenuCamData *camData = gobj->userdata;
    MenuData *menuData = gobj->userdata;
    EventMenu *currMenu = menuData->currMenu;

    int update_menu = 1;

    // if a custom menu is in use, run its function
    if (menuData->custom_gobj_think != 0)
    {
        update_menu = menuData->custom_gobj_think(menuData->custom_gobj);
    }

    // if this menu has an upate function, run its function
    else if ((menuData->mode == MenuMode_Paused) && (currMenu->menu_think != 0))
    {
        update_menu = currMenu->menu_think(gobj);
        EventMenu_UpdateText(gobj, currMenu);
    }

    int exit_menu = 0;
    int enter_menu = 0;

    if (update_menu == 1)
    {
        // Check if being pressed
        int pause_pressed = 0;
        for (int i = 0; i < 6; i++)
        {

            // humans only
            if (Fighter_GetSlotType(i) == 0)
            {
                GOBJ *fighter = Fighter_GetGObj(i);
                FighterData *fighter_data = fighter->userdata;
                int controller_index = Fighter_GetControllerPort(i);

                HSD_Pad *pad = PadGet(controller_index, PADGET_MASTER);

                // in develop mode, use X+DPad up
                if (*stc_dblevel >= 3)
                {
                    if ((pad->held & HSD_BUTTON_X) && (pad->down & HSD_BUTTON_DPAD_UP))
                    {
                        pause_pressed = 1;
                        menuData->controller_index = controller_index;
                        break;
                    }
                }
                else if ((pad->down & HSD_BUTTON_START) != 0)
                {
                    pause_pressed = 1;
                    menuData->controller_index = controller_index;
                    break;
                }
            }
        }

        HSD_Pad *pad = PadGet(menuData->controller_index, PADGET_MASTER);

        // change pause state
        if (pause_pressed != 0)
        {
            enter_menu = menuData->mode == MenuMode_Normal;
            exit_menu = menuData->mode != MenuMode_Normal;
        }

        // run menu logic if the menu is shown
        if (menuData->mode == MenuMode_Paused && event_vars->hide_menu == 0)
        {
            // Get the current menu
            EventMenu *currMenu = menuData->currMenu;

            if ((pad->down & HSD_BUTTON_Y) != 0 && menuData->currMenu->shortcuts != 0)
                menuData->mode = MenuMode_Shortcut;

            // menu think
            else if (currMenu->state == EMSTATE_FOCUS)
            {
                // check to run custom menu think function
                EventMenu_MenuThink(gobj, currMenu);
            }

            // popup think
            else if (currMenu->state == EMSTATE_OPENPOP)
                EventMenu_PopupThink(gobj, currMenu);
        }

        if (menuData->mode == MenuMode_Shortcut)
        {
            ShortcutList *shortcuts = menuData->currMenu->shortcuts;
            if (shortcuts != 0)
            {
                event_vars->hide_menu = 1;
                int held_shortcut_buttons = pad->held & SHORTCUT_BUTTONS;

                for (int i = 0; i < shortcuts->count; ++i)
                {
                    Shortcut *shortcut = &shortcuts->list[i];

                    if (held_shortcut_buttons == shortcut->buttons_mask)
                    {
                        if (shortcut->option != 0) {
                            EventOption *option = shortcut->option;
                            option->val_prev = option->val;
                            option->val = (option->val + 1) % option->value_num;
                            if (option->OnChange)
                                option->OnChange(event_vars->menu_gobj, option->val);
                            SFX_PlayCommon(2);
                        }

                        menuData->mode = MenuMode_ShortcutWaitForRelease;
                        break;
                    }
                }
            }
        }

        if (menuData->mode == MenuMode_ShortcutWaitForRelease)
        {
            if ((pad->held & SHORTCUT_BUTTONS) == 0)
                exit_menu = 1;
        }
    }

    if (enter_menu != 0)
    {
        // set state
        menuData->mode = MenuMode_Paused;

        // Create menu
        EventMenu_CreateModel(gobj, currMenu);
        EventMenu_CreateText(gobj, currMenu);
        EventMenu_UpdateText(gobj, currMenu);
        if (currMenu->state == EMSTATE_OPENPOP)
        {
            EventOption *currOption = &currMenu->options[currMenu->cursor];
            EventMenu_CreatePopupModel(gobj, currMenu);
            EventMenu_CreatePopupText(gobj, currMenu);
            EventMenu_UpdatePopupText(gobj, currOption);
        }

        // Freeze the game
        Match_FreezeGame(1);
        SFX_PlayCommon(5);
        Match_HideHUD();
        Match_AdjustSoundOnPause(1);
    }

    if (exit_menu != 0)
    {
        menuData->mode = MenuMode_Normal;

        // destroy menu
        EventMenu_DestroyMenu(gobj);

        // Unfreeze the game
        Match_UnfreezeGame(1);
        Match_ShowHUD();
        Match_AdjustSoundOnPause(0);
    }
}

void EventMenu_MenuGX(GOBJ *gobj, int pass)
{
    if (event_vars->hide_menu == 0)
        GXLink_Common(gobj, pass);
}

void EventMenu_TextGX(GOBJ *gobj, int pass)
{
    if (event_vars->hide_menu == 0)
        Text_GX(gobj, pass);
}

void EventMenu_MenuThink(GOBJ *gobj, EventMenu *currMenu) {
    MenuData *menuData = gobj->userdata;

    // get player who paused
    u8 pauser = menuData->controller_index;
    // get their  inputs
    HSD_Pad *pad = PadGet(pauser, PADGET_MASTER);
    int inputs_rapid = pad->rapidFire;
    int inputs_held = pad->held;
    int inputs = inputs_rapid;
    if (
        (inputs_held & HSD_TRIGGER_R) != 0
        || (pad->triggerRight >= ANALOG_TRIGGER_THRESHOLD)
    ) {
        inputs = inputs_held;
    }

    // get menu variables
    int isChanged = 0;
    s32 cursor = currMenu->cursor;
    s32 scroll = currMenu->scroll;
    EventOption *currOption = &currMenu->options[cursor + scroll];
    s32 option_num = currMenu->option_num;
    s32 cursor_min = 0;
    s32 cursor_max = ((option_num > MENU_MAXOPTION) ? MENU_MAXOPTION : option_num) - 1;

    // get option variables
    s16 val = currOption->val;
    s16 value_min = currOption->value_min;
    s16 value_max = value_min + currOption->value_num;

    // check for dpad down
    if (((inputs & HSD_BUTTON_DOWN) != 0) || ((inputs & HSD_BUTTON_DPAD_DOWN) != 0)) {
        // loop to find next option
        int cursor_next = 0; // how much to move the cursor by
        for (int i = 1; (cursor + scroll + i) < option_num; i++) {
            // option exists, check if it's enabled
            if (currMenu->options[cursor + scroll + i].disable == 0) {
                cursor_next = i;
                break;
            }
        }

        // if another option exists, move down
        if (cursor_next > 0) {
            cursor += cursor_next;

            // cursor overflowed, correct it
            if (cursor > cursor_max) {
                scroll += (cursor - cursor_max);
                cursor = cursor_max;
            }
        }

        if (currMenu->cursor != cursor || currMenu->scroll != scroll) {
            isChanged = 1;

            // update cursor
            currMenu->cursor = cursor;
            currMenu->scroll = scroll;

            // also play sfx
            SFX_PlayCommon(2);
        }
    }

    // check for dpad up
    else if (((inputs & HSD_BUTTON_UP) != 0) || ((inputs & HSD_BUTTON_DPAD_UP) != 0)) {
        // loop to find next option
        int cursor_next = 0; // how much to move the cursor by
        for (int i = 1; (cursor + scroll - i) >= 0; i++) {
            // option exists, check if it's enabled
            if (currMenu->options[cursor + scroll - i].disable == 0) {
                cursor_next = i;
                break;
            }
        }

        // if another option exists, move up
        if (cursor_next > 0) {
            cursor -= cursor_next;

            // cursor overflowed, correct it
            if (cursor < 0) {
                scroll += cursor; // effectively scroll up by adding a negative number
                cursor = 0;       // cursor is positioned at 0
            }
        }

        if (currMenu->cursor != cursor || currMenu->scroll != scroll) {
            isChanged = 1;

            // update cursor
            currMenu->cursor = cursor;
            currMenu->scroll = scroll;

            // also play sfx
            SFX_PlayCommon(2);
        }
    }

    // check for left
    else if (((inputs & HSD_BUTTON_LEFT) != 0) || ((inputs & HSD_BUTTON_DPAD_LEFT) != 0))
    {
        if ((currOption->kind == OPTKIND_STRING) || (currOption->kind == OPTKIND_INT) || (currOption->kind == OPTKIND_FLOAT))
        {
            val -= 1;
            if (val >= value_min)
            {
                isChanged = 1;

                // also play sfx
                SFX_PlayCommon(2);

                // update val
                currOption->val_prev = currOption->val;
                currOption->val = val;

                // run on change function if it exists
                if (currOption->OnChange != 0)
                    currOption->OnChange(gobj, currOption->val);
            }
        }
    }
    // check for right
    else if (((inputs & HSD_BUTTON_RIGHT) != 0) || ((inputs & HSD_BUTTON_DPAD_RIGHT) != 0))
    {
        // check for valid option kind
        if ((currOption->kind == OPTKIND_STRING) || (currOption->kind == OPTKIND_INT) || (currOption->kind == OPTKIND_FLOAT))
        {
            val += 1;
            if (val < value_max)
            {
                isChanged = 1;

                // also play sfx
                SFX_PlayCommon(2);

                // update val
                currOption->val_prev = currOption->val;
                currOption->val = val;

                // run on change function if it exists
                if (currOption->OnChange != 0)
                    currOption->OnChange(gobj, currOption->val);
            }
        }
    }

    // check for A
    else if (inputs_rapid & HSD_BUTTON_A)
    {
        // check to advance a menu
        if ((currOption->kind == OPTKIND_MENU))
        {
            // access this menu
            currMenu->state = EMSTATE_OPENSUB;

            // update currMenu
            EventMenu *nextMenu = currMenu->options[cursor + scroll].menu;
            nextMenu->prev = currMenu;
            nextMenu->state = EMSTATE_FOCUS;
            currMenu = nextMenu;
            menuData->currMenu = currMenu;

            // recreate everything
            EventMenu_DestroyMenu(gobj);
            EventMenu_CreateModel(gobj, currMenu);
            EventMenu_CreateText(gobj, currMenu);
            EventMenu_UpdateText(gobj, currMenu);

            // also play sfx
            SFX_PlayCommon(1);
        }

        /*
        // check to create a popup
        if ((currOption->kind == OPTKIND_STRING) || (currOption->kind == OPTKIND_INT))
        {
            // access this menu
            currMenu->state = EMSTATE_OPENPOP;

            // init cursor and scroll value
            s32 cursor = 0;
            s32 scroll = currOption->val;

            // correct scroll
            s32 max_scroll;
            if (currOption->value_num <= MENU_POPMAXOPTION)
                max_scroll = 0;
            else
                max_scroll = currOption->value_num - MENU_POPMAXOPTION;
            // check if scrolled too far
            if (scroll > max_scroll)
            {
                cursor = scroll - max_scroll;
                scroll = max_scroll;
            }

            // update cursor and scroll
            menuData->popup_cursor = cursor;
            menuData->popup_scroll = scroll;

            // create popup menu and update
            EventMenu_CreatePopupModel(gobj, currMenu);
            EventMenu_CreatePopupText(gobj, currMenu);
            EventMenu_UpdatePopupText(gobj, currOption);

            // also play sfx
            SFX_PlayCommon(1);
        }
        */

        // check to run a function
        if (currOption->kind == OPTKIND_FUNC && currOption->OnSelect != 0)
        {
            // execute function
            currOption->OnSelect(gobj);

            // update text
            EventMenu_UpdateText(gobj, currMenu);

            // also play sfx
            SFX_PlayCommon(1);
        }
    }
    // check to go back a menu
    else if (inputs_rapid & HSD_BUTTON_B)
    {
        // check if a prev menu exists
        EventMenu *prevMenu = currMenu->prev;
        if (prevMenu != 0)
        {

            // clear previous menu
            EventMenu *prevMenu = currMenu->prev;
            currMenu->prev = 0;

            // reset this menu's cursor
            currMenu->scroll = 0;
            currMenu->cursor = 0;

            // update currMenu
            currMenu = prevMenu;
            menuData->currMenu = currMenu;

            // close this menu
            currMenu->state = EMSTATE_FOCUS;

            // recreate everything
            EventMenu_DestroyMenu(gobj);
            EventMenu_CreateModel(gobj, currMenu);
            EventMenu_CreateText(gobj, currMenu);
            EventMenu_UpdateText(gobj, currMenu);

            // also play sfx
            SFX_PlayCommon(0);
        }

        /*
        // no previous menu, unpause
        else
        {
            SFX_PlayCommon(0);

            menuData->isPaused = 0;

            // destroy menu
            EventMenu_DestroyMenu(gobj);

            // Unfreeze the game
            Match_UnfreezeGame(1);
            Match_ShowHUD();
            Match_AdjustSoundOnPause(0);
        }
        */
    }

    // if anything changed, update text
    if (isChanged != 0)
    {
        // update menu
        EventMenu_UpdateText(gobj, currMenu);
    }
}

void EventMenu_PopupThink(GOBJ *gobj, EventMenu *currMenu)
{

    MenuData *menuData = gobj->userdata;

    // get player who paused
    u8 pauser = menuData->controller_index; // get their  inputs
    HSD_Pad *pad = PadGet(pauser, PADGET_MASTER);
    int inputs_rapid = pad->rapidFire;
    int inputs_held = pad->held;
    int inputs = inputs_rapid;
    if ((inputs_held & HSD_TRIGGER_R) != 0)
        inputs = inputs_held;

    // get option variables
    int isChanged = 0;
    s32 cursor = menuData->popup_cursor;
    s32 scroll = menuData->popup_scroll;
    EventOption *currOption = &currMenu->options[currMenu->cursor + currMenu->scroll];
    s32 value_num = currOption->value_num;
    s32 cursor_min = 0;
    s32 cursor_max = value_num;
    if (cursor_max > MENU_POPMAXOPTION)
    {
        cursor_max = MENU_POPMAXOPTION;
    }

    // check for dpad down
    if (((inputs & HSD_BUTTON_DOWN) != 0) || ((inputs & HSD_BUTTON_DPAD_DOWN) != 0))
    {
        cursor += 1;

        // cursor is in bounds, move down
        if (cursor < cursor_max)
        {
            isChanged = 1;

            // update cursor
            menuData->popup_cursor = cursor;

            // also play sfx
            SFX_PlayCommon(2);
        }

        // cursor overflowed, check to scroll
        else
        {
            // cursor+scroll is in bounds, increment scroll
            if ((cursor + scroll) < value_num)
            {
                // adjust
                scroll++;
                cursor--;

                // update cursor
                menuData->popup_cursor = cursor;
                menuData->popup_scroll = scroll;

                isChanged = 1;

                // also play sfx
                SFX_PlayCommon(2);
            }
        }
    }
    // check for dpad up
    else if (((inputs & HSD_BUTTON_UP) != 0) || ((inputs & HSD_BUTTON_DPAD_UP) != 0))
    {
        cursor -= 1;

        // cursor is in bounds, move up
        if (cursor >= 0)
        {
            isChanged = 1;

            // update cursor
            menuData->popup_cursor = cursor;

            // also play sfx
            SFX_PlayCommon(2);
        }

        // cursor overflowed, check to scroll
        else
        {
            // scroll is in bounds, decrement scroll
            if (scroll > 0)
            {
                // adjust
                scroll--;
                cursor++;

                // update cursor
                menuData->popup_cursor = cursor;
                menuData->popup_scroll = scroll;

                isChanged = 1;

                // also play sfx
                SFX_PlayCommon(2);
            }
        }
    }

    // check for A
    else if ((inputs_rapid & HSD_BUTTON_A) != 0)
    {

        // update val
        currOption->val_prev = currOption->val;
        currOption->val = cursor + scroll;

        // run on change function if it exists
        if (currOption->OnChange != 0)
            currOption->OnChange(gobj, currOption->val);

        EventMenu_DestroyPopup(gobj);

        // update menu
        EventMenu_UpdateText(gobj, currMenu);

        // play sfx
        SFX_PlayCommon(1);
    }
    // check to go back a menu
    else if ((inputs_rapid & HSD_BUTTON_B) != 0)
    {

        EventMenu_DestroyPopup(gobj);

        // update menu
        EventMenu_UpdateText(gobj, currMenu);

        // play sfx
        SFX_PlayCommon(0);
    }

    // if anything changed, update text
    if (isChanged != 0)
    {
        // update menu
        EventMenu_UpdatePopupText(gobj, currOption);
    }
}

void EventMenu_CreateModel(GOBJ *gobj, EventMenu *menu)
{

    MenuData *menuData = gobj->userdata;

    // create options background
    evMenu *menuAssets = event_vars->menu_assets;
    JOBJ *jobj_options = JOBJ_LoadJoint(menuAssets->menu);
    // Add to gobj
    GObj_AddObject(gobj, 3, jobj_options);
    GObj_DestroyGXLink(gobj);
    GObj_AddGXLink(gobj, EventMenu_MenuGX, GXLINK_MENUMODEL, GXPRI_MENUMODEL);

    // JOBJ array for getting the corner joints
    JOBJ *corners[4];

    // create a border and arrow for every row
    s32 option_num = menu->option_num;
    if (option_num > MENU_MAXOPTION)
        option_num = MENU_MAXOPTION;
    for (int i = 0; i < option_num; i++)
    {
        // create a border jobj
        JOBJ *jobj_border = JOBJ_LoadJoint(menuAssets->popup);
        // attach to root jobj
        JOBJ_AddChild(gobj->hsd_object, jobj_border);
        // move it into position
        JOBJ_GetChild(jobj_border, &corners, 2, 3, 4, 5, -1);
        // Modify scale and position
        jobj_border->trans.Z = ROWBOX_Z;
        jobj_border->scale.X = 1;
        jobj_border->scale.Y = 1;
        jobj_border->scale.Z = 1;
        corners[0]->trans.X = -(ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[0]->trans.Y = (ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        corners[1]->trans.X = (ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[1]->trans.Y = (ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        corners[2]->trans.X = -(ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[2]->trans.Y = -(ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        corners[3]->trans.X = (ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[3]->trans.Y = -(ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        JOBJ_SetFlags(jobj_border, JOBJ_HIDDEN);
        DOBJ_SetFlags(jobj_border->dobj, DOBJ_HIDDEN);
        jobj_border->dobj->next->mobj->mat->alpha = 0.6;
        //GXColor border_color = ROWBOX_COLOR;
        //jobj_border->dobj->next->mobj->mat->diffuse = border_color;
        // store pointer
        menuData->row_joints[i][0] = jobj_border;

        // create an arrow jobj
        JOBJ *jobj_arrow = JOBJ_LoadJoint(menuAssets->arrow);
        // attach to root jobj
        JOBJ_AddChild(gobj->hsd_object, jobj_arrow);
        // move it into position
        jobj_arrow->trans.X = TICKBOX_X;
        jobj_arrow->trans.Y = TICKBOX_Y + (i * ROWBOX_YOFFSET);
        jobj_arrow->trans.Z = ROWBOX_Z;
        jobj_arrow->scale.X = TICKBOX_SCALE;
        jobj_arrow->scale.Y = TICKBOX_SCALE;
        jobj_arrow->scale.Z = TICKBOX_SCALE;
        // change color
        //GXColor gx_color = {30, 40, 50, 255};
        //jobj_arrow->dobj->next->mobj->mat->diffuse = gx_color;

        JOBJ_SetFlags(jobj_arrow, JOBJ_HIDDEN);
        // store pointer
        menuData->row_joints[i][1] = jobj_arrow;
    }

    // create a highlight jobj
    JOBJ *jobj_highlight = JOBJ_LoadJoint(menuAssets->popup);
    // remove outline
    DOBJ_SetFlags(jobj_highlight->dobj, DOBJ_HIDDEN);
    // attach to root jobj
    JOBJ_AddChild(gobj->hsd_object, jobj_highlight);
    // move it into position
    JOBJ_GetChild(jobj_highlight, &corners, 2, 3, 4, 5, -1);
    // Modify scale and position
    jobj_highlight->trans.Z = MENUHIGHLIGHT_Z;
    jobj_highlight->scale.X = MENUHIGHLIGHT_SCALE;
    jobj_highlight->scale.Y = MENUHIGHLIGHT_SCALE;
    jobj_highlight->scale.Z = MENUHIGHLIGHT_SCALE;
    corners[0]->trans.X = -(MENUHIGHLIGHT_WIDTH / 2) + MENUHIGHLIGHT_X;
    corners[0]->trans.Y = (MENUHIGHLIGHT_HEIGHT / 2) + MENUHIGHLIGHT_Y;
    corners[1]->trans.X = (MENUHIGHLIGHT_WIDTH / 2) + MENUHIGHLIGHT_X;
    corners[1]->trans.Y = (MENUHIGHLIGHT_HEIGHT / 2) + MENUHIGHLIGHT_Y;
    corners[2]->trans.X = -(MENUHIGHLIGHT_WIDTH / 2) + MENUHIGHLIGHT_X;
    corners[2]->trans.Y = -(MENUHIGHLIGHT_HEIGHT / 2) + MENUHIGHLIGHT_Y;
    corners[3]->trans.X = (MENUHIGHLIGHT_WIDTH / 2) + MENUHIGHLIGHT_X;
    corners[3]->trans.Y = -(MENUHIGHLIGHT_HEIGHT / 2) + MENUHIGHLIGHT_Y;
    GXColor highlight = MENUHIGHLIGHT_COLOR;
    jobj_highlight->dobj->next->mobj->mat->alpha = 0.6;
    jobj_highlight->dobj->next->mobj->mat->diffuse = highlight;
    menuData->highlight_menu = jobj_highlight;

    // check to create scroll bar
    if (menuData->currMenu->option_num > MENU_MAXOPTION)
    {
        // create scroll bar
        JOBJ *scroll_jobj = JOBJ_LoadJoint(menuAssets->scroll);
        // attach to root jobj
        JOBJ_AddChild(gobj->hsd_object, scroll_jobj);
        // move it into position
        JOBJ_GetChild(scroll_jobj, &corners, 2, 3, -1);
        // scale scrollbar accordingly
        scroll_jobj->scale.X = MENUSCROLL_SCALE;
        scroll_jobj->scale.Y = MENUSCROLL_SCALEY;
        scroll_jobj->scale.Z = MENUSCROLL_SCALE;
        scroll_jobj->trans.X = MENUSCROLL_X;
        scroll_jobj->trans.Y = MENUSCROLL_Y;
        scroll_jobj->trans.Z = MENUSCROLL_Z;
        menuData->scroll_top = corners[0];
        menuData->scroll_bot = corners[1];
        GXColor highlight = MENUSCROLL_COLOR;
        scroll_jobj->dobj->next->mobj->mat->alpha = 0.6;
        scroll_jobj->dobj->next->mobj->mat->diffuse = highlight;

        // calculate scrollbar size
        int max_steps = menuData->currMenu->option_num - MENU_MAXOPTION;
        float botPos = MENUSCROLL_MAXLENGTH + (max_steps * MENUSCROLL_PEROPTION);
        if (botPos > MENUSCROLL_MINLENGTH)
            botPos = MENUSCROLL_MINLENGTH;

        // set size
        corners[1]->trans.Y = botPos;
    }
    else
    {
        menuData->scroll_bot = 0;
        menuData->scroll_top = 0;
    }
}

void EventMenu_CreateText(GOBJ *gobj, EventMenu *menu)
{

    // Get event info
    MenuData *menuData = gobj->userdata;
    Text *text;
    int subtext;
    int canvasIndex = menuData->canvas_menu;
    s32 cursor = menu->cursor;

    // free text if it exists
    if (menuData->text_name != 0)
    {
        // free text
        Text_Destroy(menuData->text_name);
        menuData->text_name = 0;
        Text_Destroy(menuData->text_value);
        menuData->text_value = 0;
        Text_Destroy(menuData->text_title);
        menuData->text_title = 0;
        Text_Destroy(menuData->text_desc);
        menuData->text_desc = 0;
    }
    if (menuData->text_popup != 0)
    {
        Text_Destroy(menuData->text_popup);
        menuData->text_popup = 0;
    }

    /*******************
    *** Create Title ***
    *******************/

    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menuData->text_title = text;
    // enable align and kerning
    text->align = 0;
    text->kerning = 1;
    text->use_aspect = 1;
    // scale canvas
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;
    text->trans.Z = MENU_TEXTZ;
    text->aspect.X = MENU_TITLEASPECT;

    // output menu title
    float optionX = MENU_TITLEXPOS;
    float optionY = MENU_TITLEYPOS;
    subtext = Text_AddSubtext(text, optionX, optionY, &nullString);
    Text_SetScale(text, subtext, MENU_TITLESCALE, MENU_TITLESCALE);

    /**************************
    *** Create Description ***
    *************************/

    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menuData->text_desc = text;

    /*******************
    *** Create Names ***
    *******************/

    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menuData->text_name = text;
    // enable align and kerning
    text->align = 0;
    text->kerning = 1;
    text->use_aspect = 1;
    // scale canvas
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;
    text->trans.Z = MENU_TEXTZ;
    text->aspect.X = MENU_NAMEASPECT;

    // Output all options
    s32 option_num = menu->option_num;
    if (option_num > MENU_MAXOPTION)
        option_num = MENU_MAXOPTION;
    for (int i = 0; i < option_num; i++)
    {

        // output option name
        float optionX = MENU_OPTIONNAMEXPOS;
        float optionY = MENU_OPTIONNAMEYPOS + (i * MENU_TEXTYOFFSET);
        subtext = Text_AddSubtext(text, optionX, optionY, &nullString);
    }

    /********************
    *** Create Values ***
    ********************/

    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menuData->text_value = text;
    // enable align and kerning
    text->align = 1;
    text->kerning = 1;
    text->use_aspect = 1;
    // scale canvas
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;
    text->trans.Z = MENU_TEXTZ;
    text->aspect.X = MENU_VALASPECT;

    // Output all values
    for (int i = 0; i < option_num; i++)
    {
        // output option value
        float optionX = MENU_OPTIONVALXPOS;
        float optionY = MENU_OPTIONVALYPOS + (i * MENU_TEXTYOFFSET);
        subtext = Text_AddSubtext(text, optionX, optionY, &nullString);
    }
}

void EventMenu_UpdateText(GOBJ *gobj, EventMenu *menu)
{

    // Get event info
    MenuData *menuData = gobj->userdata;
    s32 cursor = menu->cursor;
    s32 scroll = menu->scroll;
    s32 option_num = menu->option_num;
    if (option_num > MENU_MAXOPTION)
        option_num = MENU_MAXOPTION;
    Text *text;

    // Update Title

    text = menuData->text_title;
    Text_SetText(text, 0, menu->name);

    // Update Description
    Text_Destroy(menuData->text_desc); // i think its best to recreate it...
    text = Text_CreateText(2, menuData->canvas_menu);
    text->gobj->gx_cb = EventMenu_TextGX;
    menuData->text_desc = text;
    EventOption *currOption = &menu->options[menu->cursor + menu->scroll];

#define DESC_TXTSIZEX 5
#define DESC_TXTSIZEY 5
#define DESC_TXTASPECT 885
#define DESC_LINEMAX 4
#define DESC_CHARMAX 100
#define DESC_YOFFSET 30

    text->kerning = 1;
    text->align = 0;
    text->use_aspect = 1;

    // scale canvas
    text->viewport_scale.X = 0.01 * DESC_TXTSIZEX;
    text->viewport_scale.Y = 0.01 * DESC_TXTSIZEY;
    text->trans.X = MENU_DESCXPOS;
    text->trans.Y = MENU_DESCYPOS;
    text->trans.Z = MENU_TEXTZ;
    text->aspect.X = (DESC_TXTASPECT);

    char *msg = currOption->desc;

    // count newlines
    int line_num = 1;
    int line_length_arr[DESC_LINEMAX];
    char *msg_cursor_prev, *msg_cursor_curr; // declare char pointers
    msg_cursor_prev = msg;
    msg_cursor_curr = strchr(msg_cursor_prev, '\n'); // check for occurrence
    while (msg_cursor_curr != 0)                     // if occurrence found, increment values
    {
        // check if exceeds max lines
        if (line_num >= DESC_LINEMAX)
            assert("DESC_LINEMAX exceeded!");

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
        if (line_length > DESC_CHARMAX)
            assert("DESC_CHARMAX exceeded!");

        // copy char array
        char msg_line[DESC_CHARMAX + 1];
        memcpy(msg_line, msg, line_length);

        // add null terminator
        msg_line[line_length] = '\0';

        // increment msg
        msg += (line_length + 1); // +1 to skip past newline

        // print line
        int y_delta = (i * DESC_YOFFSET);
        Text_AddSubtext(text, 0, y_delta, msg_line);
    }

    /*
    Update Names
    */

    // Output all options
    text = menuData->text_name;
    for (int i = 0; i < option_num; i++)
    {
        // get this option
        EventOption *currOption = &menu->options[scroll + i];

        // output option name
        int optionVal = currOption->val;
        char *str = currOption->name ? currOption->name : "";
        Text_SetText(text, i, str);

        // output color
        GXColor color;
        if (currOption->disable == 0)
        {
            color.r = 255;
            color.b = 255;
            color.g = 255;
            color.a = 255;
        }
        else
        {
            color.r = 128;
            color.b = 128;
            color.g = 128;
            color.a = 0;
        }
        Text_SetColor(text, i, &color);
    }

    /*
    Update Values
    */

    // Output all values
    text = menuData->text_value;
    for (int i = 0; i < option_num; i++)
    {
        // get this option
        EventOption *currOption = &menu->options[scroll + i];
        int optionVal = currOption->val;

        // hide row models
        JOBJ_SetFlags(menuData->row_joints[i][0], JOBJ_HIDDEN);
        JOBJ_SetFlags(menuData->row_joints[i][1], JOBJ_HIDDEN);

        // if this option has string values
        if (currOption->kind == OPTKIND_STRING)
        {
            // output option value
            Text_SetText(text, i, currOption->values[optionVal]);

            // show box
            JOBJ_ClearFlags(menuData->row_joints[i][0], JOBJ_HIDDEN);
        }

        // if this option has int values
        else if (currOption->kind == OPTKIND_INT)
        {
            // output option value
            Text_SetText(text, i, currOption->values, optionVal);

            // show box
            JOBJ_ClearFlags(menuData->row_joints[i][0], JOBJ_HIDDEN);
        }

        // if this option is a menu or function
        else if ((currOption->kind == OPTKIND_MENU) || (currOption->kind == OPTKIND_FUNC))
        {
            Text_SetText(text, i, &nullString);

            // show arrow
            //JOBJ_ClearFlags(menuData->row_joints[i][1], JOBJ_HIDDEN);
        }

        // output color
        GXColor color;
        if (currOption->disable == 0)
        {
            color.r = 255;
            color.b = 255;
            color.g = 255;
            color.a = 255;
        }
        else
        {
            color.r = 128;
            color.b = 128;
            color.g = 128;
            color.a = 0;
        }
        Text_SetColor(text, i, &color);
    }

    // update cursor position
    JOBJ *highlight_joint = menuData->highlight_menu;
    highlight_joint->trans.Y = cursor * MENUHIGHLIGHT_YOFFSET;

    // update scrollbar position
    if (menuData->scroll_top != 0)
    {
        float curr_steps = menuData->currMenu->scroll;
        float max_steps;
        if (menuData->currMenu->option_num < MENU_MAXOPTION)
            max_steps = 0;
        else
            max_steps = menuData->currMenu->option_num - MENU_MAXOPTION;

        // scrollTop = -1 * ((curr_steps/max_steps) * (botY - -10))
        menuData->scroll_top->trans.Y = -1 * (curr_steps / max_steps) * (menuData->scroll_bot->trans.Y - MENUSCROLL_MAXLENGTH);
    }

    // update jobj
    JOBJ_SetMtxDirtySub(gobj->hsd_object);
}

void EventMenu_DestroyMenu(GOBJ *gobj)
{
    MenuData *menuData = gobj->userdata; // userdata

    // remove
    Text_Destroy(menuData->text_name);
    menuData->text_name = 0;
    // remove
    Text_Destroy(menuData->text_value);
    menuData->text_value = 0;
    // remove
    Text_Destroy(menuData->text_title);
    menuData->text_title = 0;
    // remove
    Text_Destroy(menuData->text_desc);
    menuData->text_desc = 0;

    // if popup box exists
    if (menuData->text_popup != 0)
        EventMenu_DestroyPopup(gobj);

    // if custom menu gobj exists
    if (menuData->custom_gobj != 0)
    {
        // run on destroy function
        if (menuData->custom_gobj_destroy != 0)
            menuData->custom_gobj_destroy(menuData->custom_gobj);

        // null pointers
        menuData->custom_gobj = 0;
        menuData->custom_gobj_destroy = 0;
        menuData->custom_gobj_think = 0;
    }

    // set menu as visible
    event_vars->hide_menu = 0;

    // remove jobj
    GObj_FreeObject(gobj);
    //GObj_DestroyGXLink(gobj);
}

void EventMenu_CreatePopupModel(GOBJ *gobj, EventMenu *menu)
{
    // init variables
    MenuData *menuData = gobj->userdata; // userdata
    s32 cursor = menu->cursor;
    EventOption *option = &menu->options[cursor];

    // create options background
    evMenu *menuAssets = event_vars->menu_assets;

    // create popup gobj
    GOBJ *popup_gobj = GObj_Create(0, 0, 0);

    // load popup joint
    JOBJ *popup_joint = JOBJ_LoadJoint(menuAssets->popup);

    // Get each corner's joints
    JOBJ *corners[4];
    JOBJ_GetChild(popup_joint, &corners, 2, 3, 4, 5, -1);

    // Modify scale and position
    popup_joint->scale.X = POPUP_SCALE;
    popup_joint->scale.Y = POPUP_SCALE;
    popup_joint->scale.Z = POPUP_SCALE;
    popup_joint->trans.Z = POPUP_Z;
    corners[0]->trans.X = -(POPUP_WIDTH / 2);
    corners[0]->trans.Y = (POPUP_HEIGHT / 2);
    corners[1]->trans.X = (POPUP_WIDTH / 2);
    corners[1]->trans.Y = (POPUP_HEIGHT / 2);
    corners[2]->trans.X = -(POPUP_WIDTH / 2);
    corners[2]->trans.Y = -(POPUP_HEIGHT / 2);
    corners[3]->trans.X = (POPUP_WIDTH / 2);
    corners[3]->trans.Y = -(POPUP_HEIGHT / 2);

    /*
    // Change color
    GXColor gx_color = TEXT_BGCOLOR;
    popup_joint->dobj->mobj->mat->diffuse = gx_color;
*/

    // add to gobj
    GObj_AddObject(popup_gobj, 3, popup_joint);
    // add gx link
    GObj_AddGXLink(popup_gobj, EventMenu_MenuGX, GXLINK_POPUPMODEL, GXPRI_POPUPMODEL);
    // save pointer
    menuData->popup = popup_gobj;

    // adjust scrollbar scale

    // position popup X and Y (based on cursor value)
    popup_joint->trans.X = POPUP_X;
    popup_joint->trans.Y = POPUP_Y + (POPUP_YOFFSET * cursor);

    // create a highlight jobj
    JOBJ *jobj_highlight = JOBJ_LoadJoint(menuAssets->popup);
    // attach to root jobj
    JOBJ_AddChild(popup_gobj->hsd_object, jobj_highlight);
    // move it into position
    JOBJ_GetChild(jobj_highlight, &corners, 2, 3, 4, 5, -1);
    // Modify scale and position
    jobj_highlight->trans.Z = POPUPHIGHLIGHT_Z;
    jobj_highlight->scale.X = 1;
    jobj_highlight->scale.Y = 1;
    jobj_highlight->scale.Z = 1;
    corners[0]->trans.X = -(POPUPHIGHLIGHT_WIDTH / 2) + POPUPHIGHLIGHT_X;
    corners[0]->trans.Y = (POPUPHIGHLIGHT_HEIGHT / 2) + POPUPHIGHLIGHT_Y;
    corners[1]->trans.X = (POPUPHIGHLIGHT_WIDTH / 2) + POPUPHIGHLIGHT_X;
    corners[1]->trans.Y = (POPUPHIGHLIGHT_HEIGHT / 2) + POPUPHIGHLIGHT_Y;
    corners[2]->trans.X = -(POPUPHIGHLIGHT_WIDTH / 2) + POPUPHIGHLIGHT_X;
    corners[2]->trans.Y = -(POPUPHIGHLIGHT_HEIGHT / 2) + POPUPHIGHLIGHT_Y;
    corners[3]->trans.X = (POPUPHIGHLIGHT_WIDTH / 2) + POPUPHIGHLIGHT_X;
    corners[3]->trans.Y = -(POPUPHIGHLIGHT_HEIGHT / 2) + POPUPHIGHLIGHT_Y;
    GXColor highlight = POPUPHIGHLIGHT_COLOR;
    jobj_highlight->dobj->next->mobj->mat->alpha = 0.6;
    jobj_highlight->dobj->next->mobj->mat->diffuse = highlight;

    menuData->highlight_popup = jobj_highlight;
}

void EventMenu_CreatePopupText(GOBJ *gobj, EventMenu *menu)
{
    // init variables
    MenuData *menuData = gobj->userdata;
    s32 cursor = menu->cursor;
    EventOption *option = &menu->options[cursor];
    int subtext;
    int canvasIndex = menuData->canvas_popup;
    s32 value_num = option->value_num;
    if (value_num > MENU_POPMAXOPTION)
        value_num = MENU_POPMAXOPTION;

    ///////////////////
    // Create Values //
    ///////////////////

    Text *text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menuData->text_popup = text;
    // enable align and kerning
    text->align = 1;
    text->kerning = 1;
    // scale canvas
    text->viewport_scale.X = POPUP_CANVASSCALE;
    text->viewport_scale.Y = POPUP_CANVASSCALE;
    text->trans.Z = POPUP_TEXTZ;

    // determine base Y value
    float baseYPos = POPUP_OPTIONVALYPOS + (cursor * MENU_TEXTYOFFSET);

    // Output all values
    for (int i = 0; i < value_num; i++)
    {
        // output option value
        float optionX = POPUP_OPTIONVALXPOS;
        float optionY = baseYPos + (i * POPUP_TEXTYOFFSET);
        subtext = Text_AddSubtext(text, optionX, optionY, &nullString);
    }
}

void EventMenu_UpdatePopupText(GOBJ *gobj, EventOption *option)
{
    // init variables
    MenuData *menuData = gobj->userdata; // userdata
    s32 cursor = menuData->popup_cursor;
    s32 scroll = menuData->popup_scroll;
    s32 value_num = option->value_num;
    if (value_num > MENU_POPMAXOPTION)
        value_num = MENU_POPMAXOPTION;

    ///////////////////
    // Update Values //
    ///////////////////

    Text *text = menuData->text_popup;

    // update int list
    if (option->kind == OPTKIND_INT)
    {
        // Output all values
        for (int i = 0; i < value_num; i++)
        {
            // output option value
            Text_SetText(text, i, "%d", scroll + i);
        }
    }

    // update string list
    else if (option->kind == OPTKIND_STRING)
    {
        // Output all values
        for (int i = 0; i < value_num; i++)
        {
            // output option value
            Text_SetText(text, i, option->values[scroll + i]);
        }
    }

    // update cursor position
    JOBJ *highlight_joint = menuData->highlight_popup;
    highlight_joint->trans.Y = cursor * POPUPHIGHLIGHT_YOFFSET;
    JOBJ_SetMtxDirtySub(highlight_joint);
}

void EventMenu_DestroyPopup(GOBJ *gobj)
{
    MenuData *menuData = gobj->userdata; // userdata

    // remove text
    Text_Destroy(menuData->text_popup);
    menuData->text_popup = 0;

    // destory gobj
    GObj_Destroy(menuData->popup);
    menuData->popup = 0;

    // also change the menus state
    EventMenu *currMenu = menuData->currMenu;
    currMenu->state = EMSTATE_FOCUS;
}
