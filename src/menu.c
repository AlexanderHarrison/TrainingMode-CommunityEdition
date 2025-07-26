#include "events.h"
#include <assert.h>
#include <stdarg.h>

static char null_string[] = " ";
static char off_string[] = "Off";
static char on_string[] = "On";

GOBJ *EventMenu_Init(EventMenu *start_menu)
{
    // Ensure this event has a menu
    if (start_menu == 0)
        assert("No menu");

    // Create a cobj for the event menu
    COBJDesc ***dmgScnMdls = Archive_GetPublicAddress(*stc_ifall_archive, 0x803f94d0);
    COBJDesc *cam_desc = dmgScnMdls[1][0];
    COBJ *cam_cobj = COBJ_LoadDesc(cam_desc);
    GOBJ *cam_gobj = GObj_Create(19, 20, 0);
    GObj_AddObject(cam_gobj, R13_U8(-0x3E55), cam_cobj);
    GOBJ_InitCamera(cam_gobj, CObjThink_Common, MENUCAM_GXPRI);
    cam_gobj->cobj_links = MENUCAM_COBJGXLINK;

    GOBJ *gobj = GObj_Create(0, 0, 0);
    MenuData *menu_data = calloc(sizeof(MenuData));
    GObj_AddUserData(gobj, 4, HSD_Free, menu_data);

    GObj_AddGXLink(gobj, GXLink_Common, GXLINK_MENUMODEL, GXPRI_MENUMODEL);

    // TODO remove popup canvas ussage in lab.c
    menu_data->canvas_menu = Text_CreateCanvas(2, cam_gobj, 9, 13, 0, GXLINK_MENUTEXT, GXPRI_MENUTEXT, MENUCAM_GXPRI);
    menu_data->canvas_popup = Text_CreateCanvas(2, cam_gobj, 9, 13, 0, GXLINK_MENUTEXT, GXPRI_POPUPTEXT, MENUCAM_GXPRI);
    menu_data->curr_menu = start_menu;

    EventMenu_CreateModel(gobj, start_menu);
    EventMenu_CreateText(gobj, start_menu);
    menu_data->hide_menu = 1;

    return gobj;
};

void EventMenu_EnterMenu(GOBJ *gobj) {
    MenuData *menu_data = gobj->userdata;
    EventMenu *curr_menu = menu_data->curr_menu;

    menu_data->mode = MenuMode_Paused;
    EventMenu_UpdateText(gobj, curr_menu);

    // Freeze the game
    Match_FreezeGame(1);
    SFX_PlayCommon(5);
    Match_HideHUD();
    Match_AdjustSoundOnPause(1);
}

void EventMenu_ExitMenu(GOBJ *gobj) {
    MenuData *menu_data = gobj->userdata;

    menu_data->mode = MenuMode_Normal;
    menu_data->hide_menu = 1;

    // Unfreeze the game
    Match_UnfreezeGame(1);
    Match_ShowHUD();
    Match_AdjustSoundOnPause(0);
}

void EventMenu_Update(GOBJ *gobj)
{
    MenuData *menu_data = gobj->userdata;
    EventMenu *curr_menu = menu_data->curr_menu;

    bool pause_pressed = false;
    int pause_index = 0;
    for (int i = 0; i < 4; i++) {
        HSD_Pad *pad = PadGet(i, PADGET_MASTER);
        if (pad->down & HSD_BUTTON_START) {
            pause_pressed = true;
            pause_index = i;
            break;
        }
    }

    if (menu_data->mode == MenuMode_Normal) {
        if(pause_pressed) {
            EventMenu_EnterMenu(gobj);
            menu_data->controller_index = pause_index;
        }
        return;
    }
    if (menu_data->custom_gobj_think) {
        // We delegate to the custom_gobj if it exists. custom_gobj_think
        // returns nonzero when it allows unpause. While delegating we hide the
        // menu
        menu_data->hide_menu = 1;
        if(menu_data->custom_gobj_think(menu_data->custom_gobj) && pause_pressed)
            EventMenu_ExitMenu(gobj);
        return;
    }
    if (pause_pressed) { //TODO maybe some future modes won't allow unpause
        EventMenu_ExitMenu(gobj);
        return;
    }

    menu_data->hide_menu = 0;
    HSD_Pad *pad = PadGet(menu_data->controller_index, PADGET_MASTER);

    if ((pad->held & HSD_BUTTON_Y) && menu_data->curr_menu->shortcuts) {
        ShortcutList *shortcuts = menu_data->curr_menu->shortcuts;
        for (int i = 0; i < shortcuts->count; ++i) {
            Shortcut *shortcut = shortcuts->list + i;
            if (pad->down & shortcut->buttons_mask) {
                if (!shortcut->option)
                    assert("Shortcut is missing its option");
                EventOption *option = shortcut->option;

                if (option->kind == OPTKIND_TOGGLE) {
                    option->val_prev = option->val;
                    option->val = !option->val;
                    if (!option->OnChange)
                        option->OnChange(event_vars->menu_gobj, option->val);
                    SFX_PlayCommon(2);

                    menu_data->mode = MenuMode_Shortcut;
                    EventMenu_UpdateText(gobj, curr_menu);
                }
                else if (option->kind == OPTKIND_MENU) {
                    //TODO
                }
                break;
            }
        }
    }
    if (menu_data->mode == MenuMode_Shortcut && pad->held == 0) {
        // SC mode unpauses when inputs are released to prevent misinputs
        EventMenu_ExitMenu(gobj);
    } else if (menu_data->mode == MenuMode_Paused) {
        EventMenu_MenuThink(gobj, curr_menu);
    }
}

void EventMenu_MenuGX(GOBJ *gobj, int pass)
{
    MenuData *menu_data = event_vars->menu_gobj->userdata;
    if (!menu_data->hide_menu)
        GXLink_Common(gobj, pass);
}

void EventMenu_TextGX(GOBJ *gobj, int pass)
{
    MenuData *menu_data = event_vars->menu_gobj->userdata;
    if (!menu_data->hide_menu)
        Text_GX(gobj, pass);
}

void EventMenu_MenuThink(GOBJ *gobj, EventMenu *curr_menu) {
    MenuData *menu_data = gobj->userdata;

    HSD_Pad *pad = PadGet(menu_data->controller_index, PADGET_MASTER);
    int inputs = pad->rapidFire;
    if ((pad->held & HSD_TRIGGER_R)
        || (pad->triggerRight >= ANALOG_TRIGGER_THRESHOLD)) {
        inputs = pad->held;
    }

    // get menu variables
    s32 cursor = curr_menu->cursor;
    s32 scroll = curr_menu->scroll;
    EventOption *curr_option = &curr_menu->options[cursor + scroll];

    // check for dpad down
    if (inputs & (HSD_BUTTON_DOWN | HSD_BUTTON_DPAD_DOWN)) {
        // loop to find next option
        int cursor_next = 0; // how much to move the cursor by
        for (int i = 1; (cursor + scroll + i) < curr_menu->option_num; i++) {
            // option exists, check if it's enabled
            if (curr_menu->options[cursor + scroll + i].disable == 0) {
                cursor_next = i;
                break;
            }
        }

        // if another option exists, move down
        if (cursor_next > 0) {
            cursor += cursor_next;

            // cursor overflowed, correct it
            if (cursor > MENU_MAXOPTION) {
                scroll += (cursor - MENU_MAXOPTION);
                cursor = MENU_MAXOPTION;
            }

            curr_menu->cursor = cursor;
            curr_menu->scroll = scroll;

            EventMenu_UpdateText(gobj, curr_menu);
            SFX_PlayCommon(2);
        }
    }
    else if (inputs & (HSD_BUTTON_UP | HSD_BUTTON_DPAD_UP)) {
        // loop to find next option
        int cursor_next = 0; // how much to move the cursor by
        for (int i = 1; (cursor + scroll - i) >= 0; i++) {
            // option exists, check if it's enabled
            if (curr_menu->options[cursor + scroll - i].disable == 0) {
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

            curr_menu->cursor = cursor;
            curr_menu->scroll = scroll;

            EventMenu_UpdateText(gobj, curr_menu);
            SFX_PlayCommon(2);
        }
    }
    // check to go back a menu
    else if (inputs & HSD_BUTTON_B && curr_menu->prev)
    {
        // reset cursor so it starts at top on reentry
        curr_menu->scroll = 0;
        curr_menu->cursor = 0;

        curr_menu = curr_menu->prev;
        menu_data->curr_menu = curr_menu;

        // recreate everything
        EventMenu_UpdateText(gobj, curr_menu);
        SFX_PlayCommon(0);
    }
    else if (curr_option->kind == OPTKIND_TOGGLE &&
            inputs & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT
                | HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT
                | HSD_BUTTON_A)) {
        curr_option->val_prev = curr_option->val;
        curr_option->val = !curr_option->val;
        if (curr_option->OnChange)
            curr_option->OnChange(gobj, curr_option->val);

        EventMenu_UpdateText(gobj, curr_menu);
        SFX_PlayCommon(2);
    }
    else if (inputs & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT))
    {
        if ((curr_option->kind == OPTKIND_STRING) || (curr_option->kind == OPTKIND_INT))
        {
            if (curr_option->val > curr_option->value_min)
            {
                curr_option->val_prev = curr_option->val;
                curr_option->val -= 1;

                if (curr_option->OnChange)
                    curr_option->OnChange(gobj, curr_option->val);

                EventMenu_UpdateText(gobj, curr_menu);
                SFX_PlayCommon(2);
            }
        }
    }
    else if (inputs & (HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT))
    {
        if ((curr_option->kind == OPTKIND_STRING) || (curr_option->kind == OPTKIND_INT))
        {
            s16 value_max = curr_option->value_min + curr_option->value_num;
            if (curr_option->val < value_max - 1)
            {
                curr_option->val_prev = curr_option->val;
                curr_option->val += 1;

                if (curr_option->OnChange)
                    curr_option->OnChange(gobj, curr_option->val);

                EventMenu_UpdateText(gobj, curr_menu);
                SFX_PlayCommon(2);
            }
        }
    }
    else if (inputs & HSD_BUTTON_A)
    {
        if (curr_option->kind == OPTKIND_MENU)
        {
            // Enter submenu
            EventMenu *next_menu = curr_option->menu;
            if (!next_menu)
                assert("Missing submenu");

            // update curr_menu
            next_menu->prev = curr_menu;
            menu_data->curr_menu = next_menu;
            curr_menu = next_menu;

            EventMenu_UpdateText(gobj, curr_menu);
            SFX_PlayCommon(1);
        }
        else if (curr_option->kind == OPTKIND_FUNC)
        {
            // execute function option
            if (!curr_option->OnSelect)
                assert("Missing menu function");
            curr_option->OnSelect(gobj);

            EventMenu_UpdateText(gobj, curr_menu);
            SFX_PlayCommon(1);
        }
    }
}

void EventMenu_CreateModel(GOBJ *gobj, EventMenu *menu)
{
    MenuData *menu_data = gobj->userdata;

    // create options background
    evMenu *menu_assets = event_vars->menu_assets;
    JOBJ *jobj_options = JOBJ_LoadJoint(menu_assets->menu);
    // Add to gobj
    GObj_AddObject(gobj, 3, jobj_options);
    GObj_DestroyGXLink(gobj);
    GObj_AddGXLink(gobj, EventMenu_MenuGX, GXLINK_MENUMODEL, GXPRI_MENUMODEL);

    // JOBJ array for getting the corner joints
    JOBJ *corners[4];

    // create a rowbox and arrow for every row
    s32 option_num = min(menu->option_num, MENU_MAXOPTION);
    for (int i = 0; i < option_num; i++)
    {
        // create a border jobj
        JOBJ *jobj_rowbox = JOBJ_LoadJoint(menu_assets->popup);
        // attach to root jobj
        JOBJ_AddChild(gobj->hsd_object, jobj_rowbox);
        // move it into position
        JOBJ_GetChild(jobj_rowbox, &corners, 2, 3, 4, 5, -1);
        // Modify scale and position
        jobj_rowbox->trans.Z = ROWBOX_Z;
        jobj_rowbox->scale.X = 1;
        jobj_rowbox->scale.Y = 1;
        jobj_rowbox->scale.Z = 1;
        corners[0]->trans.X = -(ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[0]->trans.Y = (ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        corners[1]->trans.X = (ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[1]->trans.Y = (ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        corners[2]->trans.X = -(ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[2]->trans.Y = -(ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        corners[3]->trans.X = (ROWBOX_WIDTH / 2) + ROWBOX_X;
        corners[3]->trans.Y = -(ROWBOX_HEIGHT / 2) + ROWBOX_Y + (i * ROWBOX_YOFFSET);
        JOBJ_SetFlags(jobj_rowbox, JOBJ_HIDDEN);
        DOBJ_SetFlags(jobj_rowbox->dobj, DOBJ_HIDDEN);
        jobj_rowbox->dobj->next->mobj->mat->alpha = 0.6;
        GXColor gx_color = ROWBOX_COLOR;
        jobj_rowbox->dobj->next->mobj->mat->diffuse = gx_color;
        // store pointer
        menu_data->row_joints[i][0] = jobj_rowbox;

        // create an arrow jobj
        JOBJ *jobj_arrow = JOBJ_LoadJoint(menu_assets->arrow);
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
        menu_data->row_joints[i][1] = jobj_arrow;
    }

    // create a highlight jobj
    JOBJ *jobj_highlight = JOBJ_LoadJoint(menu_assets->popup);
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
    jobj_highlight->dobj->next->mobj->mat->alpha = 0.4;
    jobj_highlight->dobj->next->mobj->mat->diffuse = highlight;
    menu_data->highlight_menu = jobj_highlight;

    // check to create scroll bar
    if (menu_data->curr_menu->option_num > MENU_MAXOPTION)
    {
        // create scroll bar
        JOBJ *scroll_jobj = JOBJ_LoadJoint(menu_assets->scroll);
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
        menu_data->scroll_top = corners[0];
        menu_data->scroll_bot = corners[1];
        GXColor highlight = MENUSCROLL_COLOR;
        scroll_jobj->dobj->next->mobj->mat->alpha = 0.6;
        scroll_jobj->dobj->next->mobj->mat->diffuse = highlight;

        // calculate scrollbar size
        int max_steps = menu_data->curr_menu->option_num - MENU_MAXOPTION;
        float botPos = MENUSCROLL_MAXLENGTH + (max_steps * MENUSCROLL_PEROPTION);
        if (botPos > MENUSCROLL_MINLENGTH)
            botPos = MENUSCROLL_MINLENGTH;

        // set size
        corners[1]->trans.Y = botPos;
    }
    else
    {
        menu_data->scroll_bot = 0;
        menu_data->scroll_top = 0;
    }
}

void EventMenu_CreateText(GOBJ *gobj, EventMenu *menu)
{
    MenuData *menu_data = gobj->userdata;
    Text *text;
    int canvasIndex = menu_data->canvas_menu;
    s32 cursor = menu->cursor;

    // free text if it exists
    if (menu_data->text_name || menu_data->text_value ||
            menu_data->text_title || menu_data->text_desc)
        assert("Menu text already created");

    // Create Title
    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_title = text;
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
    float x = MENU_TITLEXPOS;
    float y = MENU_TITLEYPOS;
    int subtext = Text_AddSubtext(text, x, y, &null_string);
    Text_SetScale(text, subtext, MENU_TITLESCALE, MENU_TITLESCALE);

    // Create Description
    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_desc = text;

    // Create Names
    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_name = text;
    // enable align and kerning
    text->align = 0;
    text->kerning = 1;
    text->use_aspect = 1;
    // scale canvas
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;
    text->trans.Z = MENU_TEXTZ;
    text->aspect.X = MENU_NAMEASPECT;

    s32 option_num = min(menu->option_num, MENU_MAXOPTION);
    for (int i = 0; i < option_num; i++)
    {
        float x = MENU_OPTIONNAMEXPOS;
        float y = MENU_OPTIONNAMEYPOS + (i * MENU_TEXTYOFFSET);
        Text_AddSubtext(text, x, y, &null_string);
    }

    // Create Values
    text = Text_CreateText(2, canvasIndex);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_value = text;
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
        float x = MENU_OPTIONVALXPOS;
        float y = MENU_OPTIONVALYPOS + (i * MENU_TEXTYOFFSET);
        Text_AddSubtext(text, x, y, &null_string);
    }
}

void EventMenu_UpdateText(GOBJ *gobj, EventMenu *menu)
{
    MenuData *menu_data = gobj->userdata;
    s32 cursor = menu->cursor;
    s32 scroll = menu->scroll;
    s32 option_num = min(menu->option_num, MENU_MAXOPTION);
    Text *text;

    // Update Title
    text = menu_data->text_title;
    Text_SetText(text, 0, menu->name);

    // Update Description
    Text_Destroy(menu_data->text_desc); // i think its best to recreate it...
    text = Text_CreateText(2, menu_data->canvas_menu);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_desc = text;
    EventOption *curr_option = &menu->options[menu->cursor + menu->scroll];

    text->kerning = 1;
    text->align = 0;
    text->use_aspect = 1;

    // scale canvas
    text->viewport_scale.X = 0.01 * MENU_DESCTXTSIZEX;
    text->viewport_scale.Y = 0.01 * MENU_DESCTXTSIZEY;
    text->trans.X = MENU_DESCXPOS;
    text->trans.Y = MENU_DESCYPOS;
    text->trans.Z = MENU_TEXTZ;
    text->aspect.X = (MENU_DESCTXTASPECT);

    char *msg = curr_option->desc;

    // count newlines
    int line_num = 1;
    int line_length_arr[MENU_DESCLINEMAX];
    char *msg_cursor_prev, *msg_cursor_curr; // declare char pointers
    msg_cursor_prev = msg;
    msg_cursor_curr = strchr(msg_cursor_prev, '\n'); // check for occurrence
    while (msg_cursor_curr != 0)                     // if occurrence found, increment values
    {
        if (line_num >= MENU_DESCLINEMAX)
            assert("MENU_DESCLINEMAX exceeded!");

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
        if (line_length > MENU_DESCCHARMAX)
            assert("MENU_DESCCHARMAX exceeded!");

        char msg_line[MENU_DESCCHARMAX + 1];
        memcpy(msg_line, msg, line_length);
        msg_line[line_length] = '\0';

        // increment msg
        msg += (line_length + 1); // +1 to skip past newline

        // print line
        int y_delta = (i * MENU_DESCYOFFSET);
        Text_AddSubtext(text, 0, y_delta, msg_line);
    }

    // Update Names
    // Output all options
    text = menu_data->text_name;
    for (int i = 0; i < option_num; i++)
    {
        // get this option
        EventOption *curr_option = &menu->options[scroll + i];

        // output option name
        char *str = curr_option->name ? curr_option->name : "";
        Text_SetText(text, i, str);

        GXColor color = curr_option->disable ? 
            (GXColor){ 128, 128, 128, 0 } : 
            (GXColor){ 255, 255, 255, 255 };
        Text_SetColor(text, i, &color);
    }

    // Update Values
    // Output all values
    text = menu_data->text_value;
    for (int i = 0; i < option_num; i++)
    {
        EventOption *curr_option = &menu->options[scroll + i];
        int option_val = curr_option->val;

        // Show rowbox w/ base color
        JOBJ* rowbox = menu_data->row_joints[i][0];
        JOBJ_ClearFlags(rowbox, JOBJ_HIDDEN);
        GXColor rowbox_color = ROWBOX_COLOR;
        rowbox->dobj->next->mobj->mat->diffuse = rowbox_color;

        if (curr_option->kind == OPTKIND_STRING)
        {
            Text_SetText(text, i, curr_option->values[option_val]);
        }
        else if (curr_option->kind == OPTKIND_INT)
        {
            Text_SetText(text, i, curr_option->values, option_val);
        }
        else if (curr_option->kind == OPTKIND_TOGGLE)
        {
            if (curr_option->val == 0) {
                Text_SetText(text, i, &off_string);
            } else if (curr_option->val == 1) {
                Text_SetText(text, i, &on_string);
                GXColor color = ROWBOX_ONCOLOR;
                rowbox->dobj->next->mobj->mat->diffuse = color;
            } else
                assert("Invalid val for toggle option");

        }
        else
        {
            Text_SetText(text, i, &null_string);
            JOBJ_SetFlags(rowbox, JOBJ_HIDDEN);
        }

        GXColor color = curr_option->disable ? 
            (GXColor){ 128, 128, 128, 0 } : 
            (GXColor){ 255, 255, 255, 255 };
        Text_SetColor(text, i, &color);
    }

    // update cursor position
    JOBJ *highlight_joint = menu_data->highlight_menu;
    highlight_joint->trans.Y = cursor * MENUHIGHLIGHT_YOFFSET;

    // update scrollbar position
    if (menu_data->scroll_top != 0)
    {
        float curr_steps = menu_data->curr_menu->scroll;
        float max_steps;
        if (menu_data->curr_menu->option_num < MENU_MAXOPTION)
            max_steps = 0;
        else
            max_steps = menu_data->curr_menu->option_num - MENU_MAXOPTION;

        // scrollTop = -1 * ((curr_steps/max_steps) * (botY - -10))
        menu_data->scroll_top->trans.Y = -1 * (curr_steps / max_steps) * (menu_data->scroll_bot->trans.Y - MENUSCROLL_MAXLENGTH);
    }

    // update jobj
    JOBJ_SetMtxDirtySub(gobj->hsd_object);
}

void EventMenu_DestroyMenu(GOBJ *gobj)
{
    MenuData *menu_data = gobj->userdata; // userdata

    Text_Destroy(menu_data->text_name);
    menu_data->text_name = 0;
    Text_Destroy(menu_data->text_value);
    menu_data->text_value = 0;
    Text_Destroy(menu_data->text_title);
    menu_data->text_title = 0;
    Text_Destroy(menu_data->text_desc);
    menu_data->text_desc = 0;

    // if custom menu gobj exists
    if (menu_data->custom_gobj != 0)
    {
        // run on destroy function
        if (menu_data->custom_gobj_destroy)
            menu_data->custom_gobj_destroy(menu_data->custom_gobj);

        // null pointers
        menu_data->custom_gobj = 0;
        menu_data->custom_gobj_destroy = 0;
        menu_data->custom_gobj_think = 0;
    }

    // remove jobj
    GObj_FreeObject(gobj);
    //GObj_DestroyGXLink(gobj);
}
