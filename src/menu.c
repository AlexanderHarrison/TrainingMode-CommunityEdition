#include "events.h"

GOBJ *EventMenu_Init(EventMenu *start_menu)
{
    // Ensure this event has a menu
    if (start_menu == 0)
        assert("No menu");

    // Create a cobj for the event menu
    COBJDesc ***dmgScnMdls = Archive_GetPublicAddress(*stc_ifall_archive, (void *)0x803f94d0);
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

    EventMenu_CreateModel(gobj);
    EventMenu_CreateText(gobj);
    menu_data->hide_menu = 1;

    return gobj;
};

void EventMenu_EnterMenu(GOBJ *gobj) {
    MenuData *menu_data = gobj->userdata;

    menu_data->mode = MenuMode_Paused;
    EventMenu_UpdateText(gobj);

    // Speed up repeat behavior
    stc_padlibdata->repeat_interval = MENU_RAPID_INTERVAL; // default is 8
    stc_padlibdata->repeat_start = MENU_RAPID_START;       // default is 45

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

    // Allow the custom submenu to clean itself up before we exit
    if (menu_data->custom_gobj_destroy) {
        menu_data->custom_gobj_destroy(menu_data->custom_gobj);
        menu_data->custom_gobj = 0;
        menu_data->custom_gobj_think = 0;
        menu_data->custom_gobj_destroy = 0;
    }

    // Reset repeat behavior
    stc_padlibdata->repeat_interval = stc_default_padlibdata->repeat_interval;
    stc_padlibdata->repeat_start = stc_default_padlibdata->repeat_start;

    // Unfreeze the game
    Match_UnfreezeGame(1);
    Match_ShowHUD();
    Match_AdjustSoundOnPause(0);
}

void EventMenu_PrevMenu(GOBJ *gobj) {
    MenuData *menu_data = gobj->userdata;
    EventMenu *curr_menu = menu_data->curr_menu;

    if (!curr_menu->prev)
        assert("No previous menu");

    // reset cursor so it starts at top on reentry
    curr_menu->scroll = 0;
    curr_menu->cursor = 0;

    curr_menu = curr_menu->prev;
    menu_data->curr_menu = curr_menu;

    EventMenu_UpdateText(gobj);
}

void EventMenu_NextMenu(GOBJ *gobj, EventMenu* next_menu) {
    MenuData *menu_data = gobj->userdata;

    if (!next_menu)
        assert("Missing next menu");

    // Propogate shortcuts forward from the base menu. Kinda hacky for now.
    next_menu->shortcuts = menu_data->curr_menu->shortcuts;
    next_menu->prev = menu_data->curr_menu;
    menu_data->curr_menu = next_menu;

    EventMenu_UpdateText(gobj);
}

void EventMenu_ChangeOptionVal(GOBJ *gobj, EventOption* option, int val) {
    switch(option->kind) {
        case OPTKIND_STRING:
        case OPTKIND_INT:
            if (val < option->value_min || val - option->value_min >= option->value_num)
                assert("Illegal option value");
            break;
        case OPTKIND_TOGGLE:
            if (val != 0 && val != 1)
                assert("Illegal option value");
            break;
        default:
            assert("Option kind has no value");
            break;
    }

    option->val_prev = option->val;
    option->val = val;
    if (option->OnChange)
        option->OnChange(event_vars->menu_gobj, val);

    EventMenu_UpdateText(gobj);
}

void EventMenu_Update(GOBJ *gobj)
{
    MenuData *menu_data = gobj->userdata;
    EventMenu *curr_menu = menu_data->curr_menu;

    bool pause_pressed = false;
    int pause_index = 0;
    for (int i = 0; i < 4; i++) {
        HSD_Pad *pad = PadGetMaster(i);
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
    HSD_Pad *pad = PadGetMaster(menu_data->controller_index);

    if ((pad->held & HSD_BUTTON_Y) && menu_data->curr_menu->shortcuts) {
        ShortcutList *shortcuts = menu_data->curr_menu->shortcuts;
        for (int i = 0; i < shortcuts->count; ++i) {
            Shortcut *shortcut = shortcuts->list + i;
            if (!shortcut || !shortcut->option || !shortcut->button_mask)
                assert("Invalid shortcut definition");
            if (pad->down & shortcut->button_mask) {
                EventOption *option = shortcut->option;

                if (option->disable) {
                    SFX_PlayCommon(4);
                    continue;
                }

                if (option->kind == OPTKIND_TOGGLE) {
                    EventMenu_ChangeOptionVal(gobj, option, !option->val);
                    SFX_PlayCommon(option->val ? 2 : 0);
                    menu_data->mode = MenuMode_Shortcut;
                }
                else if (option->kind == OPTKIND_STRING || option->kind == OPTKIND_INT) {
                    int val = (option->val + 1 - option->value_min) % option->value_num + option->value_min;
                    EventMenu_ChangeOptionVal(gobj, option, val);
                    SFX_PlayCommon(2);
                    menu_data->mode = MenuMode_Shortcut;
                }
                else if (option->kind == OPTKIND_MENU) {
                    if (menu_data->curr_menu == option->menu)
                        continue;
                    // rewind to top level menu before going to the next menu
                    while (menu_data->curr_menu->prev) {
                        EventMenu_PrevMenu(gobj);
                    }
                    EventMenu_NextMenu(gobj, option->menu);
                    SFX_PlayCommon(1);
                    EventMenu_UpdateText(gobj);
                }
                else {
                    assert("Invalid shortcut option kind");
                }

                break;
            }
        }
    }
    else if (menu_data->mode == MenuMode_Shortcut && pad->held == 0) {
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

    HSD_Pad *pad = PadGetMaster(menu_data->controller_index);
    int inputs = pad->repeat;

    // Speed up scrolling when R is held
    if ((pad->held & HSD_TRIGGER_R)
        || pad->triggerRight >= ANALOG_TRIGGER_THRESHOLD) {
        if (pad->repeat_timer > MENU_RAPID_R)
            pad->repeat_timer = MENU_RAPID_R;
    }

    // get menu variables
    s32 cursor = curr_menu->cursor;
    s32 scroll = curr_menu->scroll;
    EventOption *curr_option = &curr_menu->options[cursor + scroll];

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
        if (cursor_next == 0)
            return;

        cursor += cursor_next;

        // If cursor went too far down, scroll down to compensate
        if (cursor >= MENU_MAXOPTION - MENU_SCROLLOFF) {
            int max_scroll = curr_menu->option_num - MENU_MAXOPTION - scroll;
            int want_scroll = cursor - MENU_MAXOPTION + MENU_SCROLLOFF + 1;
            int delta_scroll = min(want_scroll, max_scroll);

            scroll += delta_scroll;
            cursor -= delta_scroll;
        }

        curr_menu->cursor = cursor;
        curr_menu->scroll = scroll;

        EventMenu_UpdateText(gobj);
        SFX_PlayCommon(2);
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
        if (cursor_next == 0)
            return;

        cursor -= cursor_next;

        // If cursor went too far up, scroll up to compensate
        if (cursor < MENU_SCROLLOFF) {
            int delta_scroll = min(MENU_SCROLLOFF - cursor, scroll);

            scroll -= delta_scroll;
            cursor += delta_scroll;
        }
        curr_menu->cursor = cursor;
        curr_menu->scroll = scroll;

        EventMenu_UpdateText(gobj);
        SFX_PlayCommon(2);
    }
    else if (inputs & HSD_BUTTON_B && curr_menu->prev)
    {
        EventMenu_PrevMenu(gobj);
        SFX_PlayCommon(0);
    }
    else if (curr_option->kind == OPTKIND_TOGGLE &&
            inputs & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT
                | HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT
                | HSD_BUTTON_A)) {
        EventMenu_ChangeOptionVal(gobj, curr_option, !curr_option->val);
        SFX_PlayCommon(2);
    }
    else if (inputs & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT))
    {
        if ((curr_option->kind == OPTKIND_STRING || curr_option->kind == OPTKIND_INT)
                && curr_option->val > curr_option->value_min)
        {
            EventMenu_ChangeOptionVal(gobj, curr_option, curr_option->val - 1);
            SFX_PlayCommon(2);
        }
    }
    else if (inputs & (HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT))
    {
        if ((curr_option->kind == OPTKIND_STRING || curr_option->kind == OPTKIND_INT)
                && curr_option->val - curr_option->value_min < curr_option->value_num - 1)
        {
            EventMenu_ChangeOptionVal(gobj, curr_option, curr_option->val + 1);
            SFX_PlayCommon(2);
        }
    }
    else if (inputs & HSD_BUTTON_A)
    {
        if (curr_option->kind == OPTKIND_MENU)
        {
            EventMenu_NextMenu(gobj, curr_option->menu);
            SFX_PlayCommon(1);
        }
        else if (curr_option->kind == OPTKIND_FUNC)
        {
            // execute function option
            if (!curr_option->OnSelect)
                assert("Missing menu function");
            curr_option->OnSelect(gobj);

            EventMenu_UpdateText(gobj);
            SFX_PlayCommon(1);
        }
    }
}

void EventMenu_CreateModel(GOBJ *gobj)
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

    // Create a rowbox for every option row. Rowboxes are the background behind
    // the option value.
    for (int i = 0; i < MENU_MAXOPTION; i++)
    {
        // create a border and attach to root jobj
        JOBJ *jobj_rowbox = JOBJ_LoadJoint(menu_assets->popup);
        JOBJ_AddChild(gobj->hsd_object, jobj_rowbox);
        // Modify scale and position
        JOBJ_GetChild(jobj_rowbox, corners, 2, 3, 4, 5, -1);
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
        menu_data->rowboxes[i] = jobj_rowbox;
    }

    // create a highlight jobj and attach to root jobj
    JOBJ *jobj_highlight = JOBJ_LoadJoint(menu_assets->popup);
    JOBJ_AddChild(gobj->hsd_object, jobj_highlight);
    // remove outline
    DOBJ_SetFlags(jobj_highlight->dobj, DOBJ_HIDDEN);
    // move it into position
    JOBJ_GetChild(jobj_highlight, corners, 2, 3, 4, 5, -1);
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

    // create scroll bar and attach to root jobj
    JOBJ *scroll_jobj = JOBJ_LoadJoint(menu_assets->scroll);
    JOBJ_AddChild(gobj->hsd_object, scroll_jobj);
    JOBJ_GetChild(scroll_jobj, corners, 2, 3, -1);
    scroll_jobj->scale.X = MENUSCROLL_SCALE;
    scroll_jobj->scale.Y = MENUSCROLL_SCALEY;
    scroll_jobj->trans.X = MENUSCROLL_X;
    scroll_jobj->trans.Y = MENUSCROLL_Y;
    menu_data->scroll_top = corners[0];
    menu_data->scroll_bot = corners[1];
    GXColor scroll_color = MENUSCROLL_COLOR;
    scroll_jobj->dobj->next->mobj->mat->alpha = 0.6;
    scroll_jobj->dobj->next->mobj->mat->diffuse = scroll_color;
    menu_data->scrollbar = scroll_jobj;
}

void EventMenu_CreateText(GOBJ *gobj)
{
    MenuData *menu_data = gobj->userdata;
    Text *text;
    int canvas_index = menu_data->canvas_menu;

    if (menu_data->text_name || menu_data->text_value ||
            menu_data->text_title || menu_data->text_desc)
        assert("Menu text already created");

    // Create Title
    text = Text_CreateText(2, canvas_index);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_title = text;
    text->kerning = 1;
    text->use_aspect = 1;
    text->aspect.X = MENU_TITLEASPECT;
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;

    // output menu title
    float x = MENU_TITLEXPOS;
    float y = MENU_TITLEYPOS;
    int subtext = Text_AddSubtext(text, x, y, "");
    Text_SetScale(text, subtext, MENU_TITLESCALE, MENU_TITLESCALE);

    // Create Description
    text = Text_CreateText(2, canvas_index);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_desc = text;

    text->trans.X = MENU_DESCXPOS;
    text->trans.Y = MENU_DESCYPOS;
    text->kerning = 1;
    text->use_aspect = 1;
    text->aspect.X = MENU_DESCTXTASPECT;
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;

    for (int i = 0; i < MENU_DESCLINEMAX; i++)
    {
        float y = i * MENU_DESCYOFFSET;
        Text_AddSubtext(text, 0, y, "");
    }

    // Create Names
    text = Text_CreateText(2, canvas_index);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_name = text;
    text->kerning = 1;
    text->use_aspect = 1;
    text->aspect.X = MENU_NAMEASPECT;
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;

    for (int i = 0; i < MENU_MAXOPTION; i++)
    {
        float x = MENU_OPTIONNAMEXPOS;
        float y = MENU_OPTIONNAMEYPOS + (i * MENU_TEXTYOFFSET);
        Text_AddSubtext(text, x, y, "");
    }

    // Create Values
    text = Text_CreateText(2, canvas_index);
    text->gobj->gx_cb = EventMenu_TextGX;
    menu_data->text_value = text;
    text->align = 1;
    text->kerning = 1;
    text->use_aspect = 1;
    text->aspect.X = MENU_VALASPECT;
    text->viewport_scale.X = MENU_CANVASSCALE;
    text->viewport_scale.Y = MENU_CANVASSCALE;

    // Output all values
    for (int i = 0; i < MENU_MAXOPTION; i++)
    {
        float x = MENU_OPTIONVALXPOS;
        float y = MENU_OPTIONVALYPOS + (i * MENU_TEXTYOFFSET);
        Text_AddSubtext(text, x, y, "");
    }
}

void EventMenu_UpdateText(GOBJ *gobj)
{
    MenuData *menu_data = gobj->userdata;
    EventMenu *menu = menu_data->curr_menu;
    s32 cursor = menu->cursor;
    s32 scroll = menu->scroll;
    EventOption *curr_option = &menu->options[cursor + scroll];
    Text *text;

    // Update Title
    text = menu_data->text_title;
    Text_SetText(text, 0, menu->name);

    // Update Description
    text = menu_data->text_desc;
    for (int i = 0; i < MENU_DESCLINEMAX; i++) {
        char *line = curr_option->desc[i];
        if (line)
            Text_SetText(text, i, line);
        else
            Text_SetText(text, i, "");
    }

    for (int i = 0; i < min(menu->option_num, MENU_MAXOPTION); i++)
    {
        EventOption *option = &menu->options[scroll + i];

        // Update option name
        Text_SetText(menu_data->text_name, i, option->name);

        // Update option value and rowbox
        JOBJ* rowbox = menu_data->rowboxes[i];
        JOBJ_ClearFlags(rowbox, JOBJ_HIDDEN);
        GXColor rowbox_color = ROWBOX_COLOR;
        rowbox->dobj->next->mobj->mat->diffuse = rowbox_color;

        if (option->kind == OPTKIND_STRING) {
            Text_SetText(menu_data->text_value, i, option->values[option->val]);
        }
        else if (option->kind == OPTKIND_INT) {
            Text_SetText(menu_data->text_value, i, option->format, option->val);
        }
        else if (option->kind == OPTKIND_TOGGLE) {
            if (option->val) {
                Text_SetText(menu_data->text_value, i, "On");
                GXColor color = ROWBOX_ONCOLOR;
                rowbox->dobj->next->mobj->mat->diffuse = color;
            }
            else {
                Text_SetText(menu_data->text_value, i, "Off");
            }
        }
        else {
            Text_SetText(menu_data->text_value, i, "");
            JOBJ_SetFlags(rowbox, JOBJ_HIDDEN);
        }

        // Grey out text if the option is disabled
        GXColor color = option->disable ?
            (GXColor){ 128, 128, 128, 0 } :
            (GXColor){ 255, 255, 255, 255 };
        Text_SetColor(menu_data->text_name, i, &color);
        Text_SetColor(menu_data->text_value, i, &color);
    }

    // Blank out lines past end of options
    for (int i = menu->option_num; i < MENU_MAXOPTION; i++)
    {
        Text_SetText(menu_data->text_name, i, "");
        Text_SetText(menu_data->text_value, i, "");
        JOBJ_SetFlags(menu_data->rowboxes[i], JOBJ_HIDDEN);
    }

    // update cursor position
    JOBJ *highlight_joint = menu_data->highlight_menu;
    highlight_joint->trans.Y = cursor * MENUHIGHLIGHT_YOFFSET;

    // update scrollbar position
    if (menu->option_num > MENU_MAXOPTION) {
        if (menu->scroll > menu->option_num - MENU_MAXOPTION)
            assert("Invalid scroll position");
        float len = ((float) MENU_MAXOPTION / menu->option_num) * MENUSCROLL_MAXLENGTH;
        float space = MENUSCROLL_MAXLENGTH - len;
        float top = space * menu->scroll / (menu->option_num - MENU_MAXOPTION);

        // scroll bar grows/moves in the negative Y direction, hence the -
        menu_data->scroll_top->trans.Y = -top;
        menu_data->scroll_bot->trans.Y = -len;
        JOBJ_ClearFlags(menu_data->scrollbar, JOBJ_HIDDEN);
    }
    else {
        JOBJ_SetFlags(menu_data->scrollbar, JOBJ_HIDDEN);
    }

    JOBJ_SetMtxDirtySub(gobj->hsd_object);
}
