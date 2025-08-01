#include "lab_common.h"
#include <stddef.h>

// Static Variables
static Arch_ImportData *stc_import_assets;
static ImportData import_data;
static char *slots_names[] = {"A", "B"};
static GXColor text_white = {255, 255, 255, 255};
static GXColor text_gold = {255, 211, 0, 255};

static EventDesc *event_desc;

ExportHeader *GetExportHeaderFromCard(int slot, char *fileName, void *buffer);
int CSS_ID(int ext_id);
int GetSelectedFighterIdOnCssForHmn();
void Memcard_Wait(void);
void Menu_Left_Right(int down, u8* cursor);
void Menu_SelFile_Init(GOBJ *menu_gobj);
void Menu_SelFile_Exit(GOBJ *menu_gobj);
void Menu_SelFile_Think(GOBJ *menu_gobj);
int Menu_SelFile_LoadPage(GOBJ *menu_gobj, int page);
void Menu_SelCard_Init(GOBJ *menu_gobj);
void Menu_SelCard_Exit(GOBJ *menu_gobj);
void Menu_SelCard_Think(GOBJ *menu_gobj);

// OnLoad
void OnCSSLoad(HSD_Archive *archive)
{
    // get assets from this file
    stc_import_assets = Archive_GetPublicAddress(archive, "labCSS");

    // Create a cobj for the menu
    COBJ *cam_cobj = COBJ_LoadDesc(stc_import_assets->import_cam);
    GOBJ *cam_gobj = GObj_Create(2, 3, 128);
    GObj_AddObject(cam_gobj, R13_U8(-0x3E55), cam_cobj);
    GOBJ_InitCamera(cam_gobj, CObjThink_Common, 1);
    GObj_AddProc(cam_gobj, MainMenu_CamRotateThink, 5);
    cam_gobj->cobj_links = 1 << MENUCAM_GXLINK;

    // Create text canvas
    import_data.canvas = Text_CreateCanvas(SIS_ID, 0, 2, 3, 128, MENUCAM_GXLINK, 129, 0);
    import_data.confirm.canvas = Text_CreateCanvas(SIS_ID, 0, 2, 3, 128, MENUCAM_GXLINK, 131, 0);

    // allocate filename array
    import_data.file_info = calloc(CARD_MAX_FILE * sizeof(FileInfo)); // allocate 128 entries
    // allocate filename buffers
    for (int i = 0; i < CARD_MAX_FILE; i++)
    {
        import_data.file_info[i].file_name = calloc(CARD_FILENAME_MAX);
    }

    // alloc file header array
    import_data.header = calloc(IMPORT_FILESPERPAGE * sizeof(ExportHeader));

    // alloc image (needs to be 32 byte aligned)
    import_data.snap.image = calloc(GXGetTexBufferSize(RESIZE_WIDTH, RESIZE_HEIGHT, 4, 0, 0)); // allocate 128 entries

    // HUGE HACK ALERT -- manually gets function offset of TM_GetEventDesc
    EventDesc *(*GetEventDesc)(int page, int event) = RTOC_PTR(TM_FUNC + (1 * 4));
    event_desc = GetEventDesc(1, 0);
    event_desc->stage = -1;
    *onload_fileno = -1;

    Cam_Button_Create();
    Hazards_Button_Create();
}

void Read_Recordings()
{
    // search card for save files
    import_data.file_num = 0;
    import_data.hidden_file_num = 0;
    int slot = import_data.memcard_slot;
    s32 memSize, sectorSize;

    int hmnCSSId = CSS_ID(GetSelectedFighterIdOnCssForHmn());

    if (CARDProbeEx(slot, &memSize, &sectorSize) == CARD_RESULT_READY)
    {
        // mount card
        stc_memcard_work->is_done = 0;
        if (CARDMountAsync(slot, stc_memcard_work->work_area, 0, Memcard_RemovedCallback) == CARD_RESULT_READY)
        {
            // check card
            Memcard_Wait();
            stc_memcard_work->is_done = 0;
            if (CARDCheckAsync(slot, Memcard_RemovedCallback) == CARD_RESULT_READY)
            {
                Memcard_Wait();

                // get free blocks
                s32 byteNotUsed, filesNotUsed;
                if (CARDFreeBlocks(slot, &byteNotUsed, &filesNotUsed) == CARD_RESULT_READY)
                {
                    void *buffer = calloc(CARD_READ_SIZE);

                    // search for files with name TMREC
                    for (int i = 0; i < CARD_MAX_FILE; i++)
                    {
                        CARDStat card_stat;

                        if (CARDGetStatus(slot, i, &card_stat) != CARD_RESULT_READY)
                            continue;

                        // check file matches expectations
                        if (strncmp(os_info->company, (char *)card_stat.company, sizeof(os_info->company)) != 0 ||
                                strncmp(os_info->gameName, (char *)card_stat.gameName, sizeof(os_info->gameName)) != 0 ||
                                strncmp("TMREC", card_stat.fileName, 5) != 0)
                            continue;

                        if (hmnCSSId != -1)
                        {
                            ExportHeader *header = GetExportHeaderFromCard(slot, card_stat.fileName, buffer);

                            if (CSS_ID(header->metadata.hmn) != hmnCSSId && CSS_ID(header->metadata.cpu) != hmnCSSId)
                            {
                                import_data.hidden_file_num++;
                                continue;
                            }
                        }

                        import_data.file_info[import_data.file_num].file_size = card_stat.length;                                      // save file size
                        import_data.file_info[import_data.file_num].file_no = i;                                                       // save file no
                        memcpy(import_data.file_info[import_data.file_num].file_name, card_stat.fileName, sizeof(card_stat.fileName)); // save file name
                        import_data.file_num++;                                                                                        // increment file amount
                    }

                    // free temp read buffer
                    HSD_Free(buffer);
                }
            }

            CARDUnmount(slot);
            stc_memcard_work->is_done = 0;
        }
    }
}

// Button Functions
void Cam_Button_Create()
{
    // Create GOBJ
    GOBJ *button_gobj = GObj_Create(4, 5, 0);
    GObj_AddGXLink(button_gobj, GXLink_Common, 1, 128);
    GObj_AddProc(button_gobj, Cam_Button_Think, 8);
    JOBJ *button_jobj = JOBJ_LoadJoint(stc_import_assets->import_button);
    GObj_AddObject(button_gobj, R13_U8(-0x3E55), button_jobj);

    // scale message jobj
    Vec3 *scale = &button_jobj->scale;
    button_jobj->trans.Y += 3.0;

    // Create text object
    Text *button_text = Text_CreateText(SIS_ID, 0);
    button_text->kerning = 1;
    button_text->align = 1;
    button_text->use_aspect = 1;
    button_text->viewport_scale.X = (scale->X * 0.01) * 3;
    button_text->viewport_scale.Y = (scale->Y * 0.01) * 3;
    button_text->trans.X = button_jobj->trans.X + (0 * (scale->X / 4.0));
    button_text->trans.Y = (button_jobj->trans.Y * -1) + (-1.6 * (scale->Y / 4.0));

    Text_AddSubtext(button_text, 0, 0, "Import");
}

void Cam_Button_Think(GOBJ *button_gobj)
{

#define BUTTON_WIDTH 5
#define BUTTON_HEIGHT 2.2

    // init
    CSSCursor *this_cursor = stc_css_cursors[0]; // get the main player's hand cursor data
    Vec2 cursor_pos = this_cursor->pos;
    cursor_pos.X += 5;
    cursor_pos.Y -= 1;
    HSD_Pad *pads = stc_css_pad;
    HSD_Pad *this_pad = &pads[*stc_css_hmnport];
    int down = this_pad->down;                   // get this cursors inputs
    JOBJ *button_jobj = button_gobj->hsd_object; // get jobj
    Vec3 *button_pos = &button_jobj->trans;

    // check if cursor is hovered over button
    if ((import_data.menu_gobj == 0) &&
        (cursor_pos.X < (button_pos->X + BUTTON_WIDTH)) &&
        (cursor_pos.X > (button_pos->X - BUTTON_WIDTH)) &&
        (cursor_pos.Y < (button_pos->Y + BUTTON_HEIGHT)) &&
        (cursor_pos.Y > (button_pos->Y - BUTTON_HEIGHT)) &&
        (down & HSD_BUTTON_A))
    {
        import_data.menu_gobj = Menu_Create();
        SFX_PlayCommon(1);
    }
}

static char *hazard_button_text_options[] = {"Hazards: On", "Hazards: Off"};
static Text *hazard_button_text;
static int hazard_button_subtext;

void Hazards_Button_Create()
{
    // Create GOBJ
    GOBJ *button_gobj = GObj_Create(4, 5, 0);
    GObj_AddGXLink(button_gobj, GXLink_Common, 1, 128);
    GObj_AddProc(button_gobj, Hazards_Button_Think, 8);
    JOBJ *button_jobj = JOBJ_LoadJoint(stc_import_assets->import_button);
    GObj_AddObject(button_gobj, R13_U8(-0x3E55), button_jobj);

    // scale message jobj
    Vec3 *scale = &button_jobj->scale;
    button_jobj->scale.X = 3.0;
    button_jobj->trans.Y -= 3.0;

    // Create text object
    hazard_button_text = Text_CreateText(SIS_ID, 0);
    hazard_button_text->kerning = 1;
    hazard_button_text->align = 1;
    hazard_button_text->use_aspect = 1;
    hazard_button_text->viewport_scale.X = (scale->X * 0.01) * 2;
    hazard_button_text->viewport_scale.Y = (scale->Y * 0.01) * 3;
    hazard_button_text->trans.X = button_jobj->trans.X + (0 * (scale->X / 4.0));
    hazard_button_text->trans.Y = (button_jobj->trans.Y * -1) + (-1.6 * (scale->Y / 4.0));

    char *text = hazard_button_text_options[event_desc->disable_hazards];
    hazard_button_subtext = Text_AddSubtext(hazard_button_text, 0, 0, text);
}

void Hazards_Button_Think(GOBJ *button_gobj)
{
    // init
    CSSCursor *this_cursor = stc_css_cursors[0]; // get the main player's hand cursor data
    Vec2 cursor_pos = this_cursor->pos;
    cursor_pos.X += 5;
    cursor_pos.Y -= 1;
    HSD_Pad *pads = stc_css_pad;
    HSD_Pad *this_pad = &pads[*stc_css_hmnport];
    int down = this_pad->down;                   // get this cursors inputs
    JOBJ *button_jobj = button_gobj->hsd_object; // get jobj
    Vec3 *button_pos = &button_jobj->trans;

    // check if cursor is hovered over button
    if ((import_data.menu_gobj == 0) &&
        (cursor_pos.X < (button_pos->X + BUTTON_WIDTH)) &&
        (cursor_pos.X > (button_pos->X - BUTTON_WIDTH)) &&
        (cursor_pos.Y < (button_pos->Y + BUTTON_HEIGHT)) &&
        (cursor_pos.Y > (button_pos->Y - BUTTON_HEIGHT)) &&
        (down & HSD_BUTTON_A))
    {
        SFX_PlayCommon(1);
        int new_hazard_state = !event_desc->disable_hazards;
        event_desc->disable_hazards = new_hazard_state;
        char *new_text = hazard_button_text_options[new_hazard_state];
        Text_SetText(hazard_button_text, hazard_button_subtext, new_text);
    }
}

// Import Menu Functions
GOBJ *Menu_Create()
{
    // Create GOBJ
    GOBJ *menu_gobj = GObj_Create(4, 5, 0);
    GObj_AddGXLink(menu_gobj, GXLink_Common, MENUCAM_GXLINK, 128);
    GObj_AddProc(menu_gobj, Menu_Think, 1);
    JOBJ *menu_jobj = JOBJ_LoadJoint(stc_import_assets->import_menu);
    GObj_AddObject(menu_gobj, R13_U8(-0x3E55), menu_jobj);

    // save jobj pointers
    JOBJ_GetChild(menu_jobj, &import_data.memcard_jobj[0], 2, -1);
    JOBJ_GetChild(menu_jobj, &import_data.memcard_jobj[1], 4, -1);
    JOBJ_GetChild(menu_jobj, &import_data.screenshot_jobj, 6, -1);
    JOBJ_GetChild(menu_jobj, &import_data.scroll_jobj, 7, -1);
    JOBJ_GetChild(menu_jobj, &import_data.scroll_top, 9, -1);
    JOBJ_GetChild(menu_jobj, &import_data.scroll_bot, 10, -1);

    // hide all
    JOBJ_SetFlagsAll(import_data.memcard_jobj[0], JOBJ_HIDDEN);
    JOBJ_SetFlagsAll(import_data.memcard_jobj[1], JOBJ_HIDDEN);
    JOBJ_SetFlagsAll(import_data.screenshot_jobj, JOBJ_HIDDEN);
    JOBJ_SetFlagsAll(import_data.scroll_jobj, JOBJ_HIDDEN);

    // create title and desc text
    Text *title_text = Text_CreateText(SIS_ID, import_data.canvas);
    title_text->kerning = 1;
    title_text->use_aspect = 1;
    title_text->aspect.X = 305;
    title_text->trans.X = -28;
    title_text->trans.Y = -21;
    title_text->trans.Z = menu_jobj->trans.Z;
    title_text->viewport_scale.X = (menu_jobj->scale.X * 0.01) * 11;
    title_text->viewport_scale.Y = (menu_jobj->scale.Y * 0.01) * 11;
    Text_AddSubtext(title_text, 0, 0, "-");
    import_data.title_text = title_text;
    Text *desc_text = Text_CreateText(SIS_ID, import_data.canvas);
    desc_text->kerning = 1;
    desc_text->use_aspect = 1;
    desc_text->aspect.X = 930;
    desc_text->trans.X = -28;
    desc_text->trans.Y = 14.5;
    desc_text->trans.Z = menu_jobj->trans.Z;
    desc_text->viewport_scale.X = (menu_jobj->scale.X * 0.01) * 5;
    desc_text->viewport_scale.Y = (menu_jobj->scale.Y * 0.01) * 5;
    Text_AddSubtext(desc_text, 0, 0, "-");
    Text_AddSubtext(desc_text, 0, 80, "");
    import_data.desc_text = desc_text;

    // disable inputs for CSS
    *stc_css_exitkind = 5;

    // init card select menu
    Menu_SelCard_Init(menu_gobj);

    return menu_gobj;
}
void Menu_Destroy(GOBJ *menu_gobj)
{
    // run specific menu state code
    switch (import_data.menu_state)
    {
    case (IMP_SELCARD):
    {
        Menu_SelCard_Exit(menu_gobj);
        break;
    }
    case (IMP_SELFILE):
    {
        Menu_SelFile_Exit(menu_gobj);
        break;
    }
    }

    // destroy text
    Text_Destroy(import_data.title_text);
    import_data.title_text = 0;
    Text_Destroy(import_data.desc_text);
    import_data.desc_text = 0;

    // destroy gobj
    GObj_Destroy(menu_gobj);
    import_data.menu_gobj = 0;

    // enable CSS inputs
    *stc_css_exitkind = 0;
    *stc_css_delay = 0;
}
void Menu_Think(GOBJ *menu_gobj)
{

    *stc_css_delay = 2;

    switch (import_data.menu_state)
    {
    case (IMP_SELCARD):
    {
        Menu_SelCard_Think(menu_gobj);
        break;
    }
    case (IMP_SELFILE):
    {
        Menu_SelFile_Think(menu_gobj);
        break;
    }
    case (IMP_CONFIRM):
    {
        Menu_Confirm_Think(menu_gobj);
        break;
    }
    }
}
// Select Card
void Menu_SelCard_Init(GOBJ *menu_gobj)
{

    // get jobj
    JOBJ *menu_jobj = menu_gobj->hsd_object;

    // init state and cursor
    import_data.menu_state = IMP_SELCARD;
    import_data.cursor = 0;

    // init memcard inserted state
    import_data.memcard_inserted[0] = 0;
    import_data.memcard_inserted[1] = 0;

    // show memcards
    JOBJ_ClearFlagsAll(import_data.memcard_jobj[0], JOBJ_HIDDEN);
    JOBJ_ClearFlagsAll(import_data.memcard_jobj[1], JOBJ_HIDDEN);

    // create memcard text
    Text *memcard_text = Text_CreateText(SIS_ID, import_data.canvas);
    memcard_text->kerning = 1;
    memcard_text->align = 1;
    memcard_text->trans.Z = menu_jobj->trans.Z + 2;
    memcard_text->viewport_scale.X = (menu_jobj->scale.X * 0.01) * 5;
    memcard_text->viewport_scale.Y = (menu_jobj->scale.Y * 0.01) * 5;
    import_data.option_text = memcard_text;
    Text_AddSubtext(memcard_text, -159, 45, "Slot A");
    Text_AddSubtext(memcard_text, 159, 45, "Slot B");

    // edit title
    Text_SetText(import_data.title_text, 0, "Select Memory Card");

    // edit description
    Text_SetText(import_data.desc_text, 0, "");
}
void Menu_SelCard_Think(GOBJ *menu_gobj)
{
    // update memcard info
    for (int i = 0; i <= 1; i++)
    {
        // probe slot
        s32 memSize, sectorSize;
        if (CARDProbeEx(i, &memSize, &sectorSize) != CARD_RESULT_READY)
        {
            import_data.memcard_inserted[i] = 0;
            continue;
        }

        // skip good memcards
        if (import_data.memcard_inserted[i] == 1)
            continue;

        // mount card
        stc_memcard_work->is_done = 0;
        if (CARDMountAsync(i, stc_memcard_work->work_area, 0, Memcard_RemovedCallback) != CARD_RESULT_READY)
            continue;

        // check card
        Memcard_Wait();
        stc_memcard_work->is_done = 0;
        if (CARDCheckAsync(i, Memcard_RemovedCallback) == CARD_RESULT_READY)
        {
            Memcard_Wait();

            // if we get this far, a valid memcard is inserted
            import_data.memcard_inserted[i] = 1;
            SFX_PlayCommon(2);
        }

        CARDUnmount(i);
        stc_memcard_work->is_done = 0;
    }

    // cursor movement
    int down = Pad_GetRapidHeld(*stc_css_hmnport);
    Menu_Left_Right(down, &import_data.cursor);

    // highlight cursor
    for (int i = 0; i < 2; i++)
    {
        Text_SetColor(import_data.option_text, i, &text_white);
    }
    Text_SetColor(import_data.option_text, import_data.cursor, &text_gold);

    if (import_data.memcard_inserted[import_data.cursor] == 0) {
        Text_SetText(import_data.desc_text, 0, "No device is inserted in Slot %s.", slots_names[import_data.cursor]);
    } else {
        Text_SetText(import_data.desc_text, 0, "Load recording from the memory card in Slot %s.", slots_names[import_data.cursor]);
        int hmnFighterId = GetSelectedFighterIdOnCssForHmn();
        if (hmnFighterId != -1) {
            Text_SetText(import_data.desc_text, 1, "(!) Filtered to selected character on CSS.");
            Text_SetColor(import_data.desc_text, 1, &text_gold);
        }
    }

    if (down & HSD_BUTTON_B)
    {
        Menu_Destroy(menu_gobj);
        SFX_PlayCommon(0);
    }
    else if (down & HSD_BUTTON_A)
    {
        // check if valid memcard inserted
        if (import_data.memcard_inserted[import_data.cursor])
        {
            import_data.memcard_slot = import_data.cursor;
            SFX_PlayCommon(1);
            Menu_SelCard_Exit(menu_gobj);
            Menu_SelFile_Init(menu_gobj);
        }
        else
            SFX_PlayCommon(3);
    }
}
void Menu_SelCard_Exit(GOBJ *menu_gobj)
{
    JOBJ_SetFlagsAll(import_data.memcard_jobj[0], JOBJ_HIDDEN);
    JOBJ_SetFlagsAll(import_data.memcard_jobj[1], JOBJ_HIDDEN);

    // destroy memcard text
    Text_Destroy(import_data.option_text);
    import_data.option_text = 0;
}
// Select File
void Menu_SelFile_Init(GOBJ *menu_gobj)
{
    Read_Recordings();
    if (import_data.file_num == 0)
    {
        Menu_Confirm_Init(menu_gobj, CFRM_NONE);
        SFX_PlayCommon(3);
        return;
    }

    // get jobj
    JOBJ *menu_jobj = menu_gobj->hsd_object;

    // init state and cursor
    import_data.menu_state = IMP_SELFILE;
    import_data.cursor = 0;
    import_data.page = 0;

    JOBJ_ClearFlagsAll(import_data.scroll_jobj, JOBJ_HIDDEN);

    // create file name text
    Text *filename_text = Text_CreateText(SIS_ID, import_data.canvas);
    filename_text->kerning = 1;
    filename_text->align = 0;
    filename_text->use_aspect = 1;
    filename_text->aspect.X = 525;
    filename_text->trans.X = -27.8;
    filename_text->trans.Y = -13.6;
    filename_text->trans.Z = menu_jobj->trans.Z;
    filename_text->viewport_scale.X = (menu_jobj->scale.X * 0.01) * 5;
    filename_text->viewport_scale.Y = (menu_jobj->scale.Y * 0.01) * 5;
    import_data.filename_text = filename_text;
    for (int i = 0; i < IMPORT_FILESPERPAGE; i++)
    {
        Text_AddSubtext(filename_text, 0, i * 40, "");
    }

#define FILEINFO_X 235
#define FILEINFO_Y -20
#define FILEINFO_YOFFSET 35
#define FILEINFO_STAGEY FILEINFO_Y
#define FILEINFO_HMNY FILEINFO_STAGEY + FILEINFO_YOFFSET
#define FILEINFO_CPUY FILEINFO_HMNY + FILEINFO_YOFFSET
#define FILEINFO_DATEY FILEINFO_CPUY + (FILEINFO_YOFFSET * 2)
#define FILEINFO_TIMEY FILEINFO_DATEY + FILEINFO_YOFFSET

    // create file info text
    Text *fileinfo_text = Text_CreateText(SIS_ID, import_data.canvas);
    fileinfo_text->kerning = 1;
    fileinfo_text->align = 0;
    fileinfo_text->use_aspect = 1;
    fileinfo_text->aspect.X = 300;
    fileinfo_text->trans.Z = menu_jobj->trans.Z;
    fileinfo_text->viewport_scale.X = (menu_jobj->scale.X * 0.01) * 4.5;
    fileinfo_text->viewport_scale.Y = (menu_jobj->scale.Y * 0.01) * 4.5;
    import_data.fileinfo_text = fileinfo_text;
    Text_AddSubtext(fileinfo_text, FILEINFO_X, FILEINFO_STAGEY, "Stage: ");
    Text_AddSubtext(fileinfo_text, FILEINFO_X, FILEINFO_HMNY, "HMN: ");
    Text_AddSubtext(fileinfo_text, FILEINFO_X, FILEINFO_CPUY, "CPU: ");
    Text_AddSubtext(fileinfo_text, FILEINFO_X, FILEINFO_DATEY, "Date: ");
    Text_AddSubtext(fileinfo_text, FILEINFO_X, FILEINFO_TIMEY, "Time: ");

    // edit title
    Text_SetText(import_data.title_text, 0, "Select Recording");

    // edit description
    Text_SetText(import_data.desc_text, 0, "A = Select   B = Return   X = Delete   Y = Swap Sheik/Zelda");
    if (import_data.hidden_file_num > 0)
    {
        Text_SetText(import_data.desc_text, 1, "(!) Filtered to selected character on CSS, %d hidden.", import_data.hidden_file_num);
        Text_SetColor(import_data.desc_text, 1, &text_gold);
    }

    // init scroll bar according to import_data.file_num
    int page_total = (import_data.file_num + IMPORT_FILESPERPAGE - 1) / IMPORT_FILESPERPAGE;
    if (page_total <= 1)
        JOBJ_SetFlagsAll(import_data.scroll_jobj, JOBJ_HIDDEN);
    else
        import_data.scroll_bot->trans.Y = -16.2 / page_total;

    // load in first page recordings
    int page_result = Menu_SelFile_LoadPage(menu_gobj, 0);
    if (page_result == -1)
    {
        // create dialog
        Menu_Confirm_Init(menu_gobj, CFRM_ERR);
        SFX_PlayCommon(3);
    }
}
void Menu_SelFile_Think(GOBJ *menu_gobj)
{
    // init
    int down = Pad_GetRapidHeld(*stc_css_hmnport);

    // first ensure memcard is still inserted
    s32 memSize, sectorSize;
    if (CARDProbeEx(import_data.memcard_slot, &memSize, &sectorSize) != CARD_RESULT_READY)
        goto EXIT;

    // if no files exist
    if (import_data.file_num == 0)
    {
        if (down & (HSD_BUTTON_B | HSD_BUTTON_A))
            goto EXIT;
        return;
    }

    // cursor movement
    int curr_page = import_data.page;
    int curr_cursor = import_data.cursor;
    if (down & (HSD_BUTTON_UP | HSD_BUTTON_DPAD_UP))
    {
        import_data.cursor--;
    }
    else if (down & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT))
    {
        import_data.cursor = 0;
        import_data.page--;
    }
    else if (down & (HSD_BUTTON_DOWN | HSD_BUTTON_DPAD_DOWN))
    {
        import_data.cursor++;
    }
    else if (down & (HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT))
    {
        import_data.cursor = 0;
        import_data.page++;
    }

    // handle cursor/page overflow and wrap pages
    int total_pages = (import_data.file_num + IMPORT_FILESPERPAGE - 1) / IMPORT_FILESPERPAGE;
    if (import_data.cursor == 255)
    {
        import_data.cursor = IMPORT_FILESPERPAGE - 1;
        import_data.page--;
    }
    else if (import_data.cursor >= IMPORT_FILESPERPAGE ||
        (import_data.page == total_pages - 1 &&
        import_data.cursor > (import_data.file_num - 1) % IMPORT_FILESPERPAGE))
    {
        import_data.cursor = 0;
        import_data.page++;
    }
    if (import_data.page == 255)
    {
        import_data.page = total_pages - 1;
        if (import_data.cursor > (import_data.file_num - 1) % IMPORT_FILESPERPAGE)
            import_data.cursor = (import_data.file_num - 1) % IMPORT_FILESPERPAGE;
    }
    else if (import_data.page >= total_pages)
    {
        import_data.page = 0;
    }

    // Load new page if necessary and play sound
    if (import_data.page != curr_page)
    {
        // try to load page
        int page_result = Menu_SelFile_LoadPage(menu_gobj, import_data.page);
        if (page_result == -1)
        {
            // create dialog
            Menu_Confirm_Init(menu_gobj, CFRM_ERR);
            SFX_PlayCommon(3);
            return;
        }
        SFX_PlayCommon(2);
    }
    else if (import_data.cursor != curr_cursor)
    {
        SFX_PlayCommon(2);
    }

    // highlight cursor
    int cursor = import_data.cursor;
    for (int i = 0; i < IMPORT_FILESPERPAGE; i++)
    {
        Text_SetColor(import_data.filename_text, i, &text_white);
    }
    Text_SetColor(import_data.filename_text, cursor, &text_gold);

    // update file info text from header data
    ExportHeader *header = &import_data.header[cursor];

    // update file info text
    Text_SetText(import_data.fileinfo_text, 0, "Stage: %s", stage_names[header->metadata.stage_internal]);
    Text_SetText(import_data.fileinfo_text, 1, "HMN: %s", Fighter_GetName(header->metadata.hmn));
    Text_SetText(import_data.fileinfo_text, 2, "CPU: %s", Fighter_GetName(header->metadata.cpu));
    Text_SetText(import_data.fileinfo_text, 3, "Date: %d/%d/%d", header->metadata.month, header->metadata.day, header->metadata.year);
    Text_SetText(import_data.fileinfo_text, 4, "Time: %d:%02d:%02d", header->metadata.hour, header->metadata.minute, header->metadata.second);


    // check for exit
    if (down & HSD_BUTTON_B)
    {
    EXIT:
        SFX_PlayCommon(0);
        Menu_SelFile_Exit(menu_gobj);
        Menu_SelCard_Init(menu_gobj);
    }

    // check to delete
    else if (down & HSD_BUTTON_X)
    {
        Menu_Confirm_Init(menu_gobj, CFRM_DEL);
        SFX_PlayCommon(1);
    }
    
    else if (down & HSD_BUTTON_Y)
    {
        // Rwing exports only swap the hmn. 
        if (header->metadata.hmn == CKIND_ZELDA)
            header->metadata.hmn = CKIND_SHEIK; 
        else if (header->metadata.hmn == CKIND_SHEIK)
            header->metadata.hmn = CKIND_ZELDA;
             
        SFX_PlayCommon(1);
    }

    // check for select
    else if (down & HSD_BUTTON_A)
    {
        int kind;                                               // init confirm kind
        int vers = import_data.header[cursor].metadata.version; // get version number

        // check if version is compatible with this release
        if (vers == REC_VERS)
            kind = CFRM_LOAD;
        else if (vers > REC_VERS)
            kind = CFRM_NEW;
        else if (vers < REC_VERS)
            kind = CFRM_OLD;

        // open confirm dialog
        Menu_Confirm_Init(menu_gobj, kind);
        SFX_PlayCommon(1);
    }
}
void Menu_SelFile_Exit(GOBJ *menu_gobj)
{
    Text_SetText(import_data.desc_text, 1, "");
    JOBJ_SetFlagsAll(import_data.scroll_jobj, JOBJ_HIDDEN);
    JOBJ_SetFlagsAll(import_data.screenshot_jobj, JOBJ_HIDDEN);

    // destroy option text
    Text_Destroy(import_data.filename_text);
    import_data.filename_text = 0;
    Text_Destroy(import_data.fileinfo_text);
    import_data.fileinfo_text = 0;

    // cancel card read if in progress
    int memcard_status = Memcard_CheckStatus();
    if (memcard_status == 11)
    {
        // cancel read
        stc_memcard_work->card_file_info.length = -1;

        // wait for callback to fire
        // i do this because the callback will always set some
        // variable in the workarea regardless of its cancelled
        while (memcard_status == 11)
        {
            memcard_status = Memcard_CheckStatus();
        }
    }

    // free prev buffers
    for (int i = 0; i < IMPORT_FILESPERPAGE; i++)
    {
        // if exists
        if (import_data.snap.file_data[i] != 0)
        {
            HSD_Free(import_data.snap.file_data[i]);
            import_data.snap.file_data[i] = 0;
        }
    }
}
int Menu_SelFile_LoadPage(GOBJ *menu_gobj, int page)
{
    int result = 0;
    int page_total = (import_data.file_num + IMPORT_FILESPERPAGE - 1) / IMPORT_FILESPERPAGE;

    // ensure page exists
    if (page < 0 || page >= page_total)
        assert("page index out of bounds");

    // determine files on page
    int files_on_page = IMPORT_FILESPERPAGE;
    if (page == page_total - 1)
        files_on_page = ((import_data.file_num-1) % IMPORT_FILESPERPAGE) + 1;

    // cancel card read if in progress
    int memcard_status = Memcard_CheckStatus();
    if (memcard_status == 11)
    {
        // cancel read
        stc_memcard_work->card_file_info.length = -1;

        // wait for callback to fire
        while (memcard_status == 11)
        {
            memcard_status = Memcard_CheckStatus();
        }
    }

    void *buffer = calloc(CARD_READ_SIZE);
    import_data.snap.loaded_num = 0;
    import_data.snap.load_inprogress = 0;
    for (int i = 0; i < IMPORT_FILESPERPAGE; i++)
    {
        import_data.snap.is_loaded[i] = 0; // init files as unloaded
    }
    result = 1; // set page as toggled
    int slot = import_data.memcard_slot;

    // update scroll bar position
    import_data.scroll_top->trans.Y = page * import_data.scroll_bot->trans.Y;
    JOBJ_SetMtxDirtySub(menu_gobj->hsd_object);

    // free prev buffers
    for (int i = 0; i < IMPORT_FILESPERPAGE; i++)
    {
        // if exists
        if (import_data.snap.file_data[i] != 0)
        {
            HSD_Free(import_data.snap.file_data[i]);
            import_data.snap.file_data[i] = 0;
        }
    }

    // blank out all text
    for (int i = 0; i < IMPORT_FILESPERPAGE; i++)
    {
        Text_SetText(import_data.filename_text, i, "");
    }

    // mount card
    s32 memSize, sectorSize;
    if (CARDProbeEx(slot, &memSize, &sectorSize) == CARD_RESULT_READY)
    {
        // mount card
        stc_memcard_work->is_done = 0;
        if (CARDMountAsync(slot, stc_memcard_work->work_area, 0, Memcard_RemovedCallback) == CARD_RESULT_READY)
        {
            Memcard_Wait();

            // check card
            stc_memcard_work->is_done = 0;
            if (CARDCheckAsync(slot, Memcard_RemovedCallback) == CARD_RESULT_READY)
            {
                Memcard_Wait();

                // begin loading this page's files
                for (int i = 0; i < files_on_page; i++)
                {
                    // get file info
                    int this_file_index = (page * IMPORT_FILESPERPAGE) + i;
                    char *file_name = import_data.file_info[this_file_index].file_name;
                    int file_no = import_data.file_info[this_file_index].file_no;

                    // get comment from card
                    CARDStat card_stat;

                    // get status
                    if (CARDGetStatus(slot, file_no, &card_stat) != CARD_RESULT_READY)
                        continue;

                    ExportHeader *header = GetExportHeaderFromCard(slot, file_name, buffer);
                    if (!header)
                        continue;

                    memcpy(&import_data.header[i], header, sizeof(ExportHeader));
                    Text_SetText(import_data.filename_text, i, header->metadata.filename);
                }
            }
            // unmount
            CARDUnmount(slot);
            stc_memcard_work->is_done = 0;
        }
    }

    // free temp read buffer
    HSD_Free(buffer);

    return result;
}

int Menu_SelFile_DeleteFile(GOBJ *menu_gobj, int file_index)
{
    int result = 0;
    int slot = import_data.memcard_slot;

    // mount card
    s32 memSize, sectorSize;
    if (CARDProbeEx(slot, &memSize, &sectorSize) == CARD_RESULT_READY)
    {
        // mount card
        stc_memcard_work->is_done = 0;
        if (CARDMountAsync(slot, stc_memcard_work->work_area, 0, Memcard_RemovedCallback) == CARD_RESULT_READY)
        {
            Memcard_Wait();

            // check card
            stc_memcard_work->is_done = 0;
            if (CARDCheckAsync(slot, Memcard_RemovedCallback) == CARD_RESULT_READY)
            {
                Memcard_Wait();

                // get file info
                char *file_name = import_data.file_info[file_index].file_name;
                CARDFileInfo card_file_info;

                // open card (get file info)
                if (CARDOpen(slot, file_name, &card_file_info) == CARD_RESULT_READY)
                {
                    // delete this file
                    stc_memcard_work->is_done = 0;
                    if (CARDDeleteAsync(slot, file_name, Memcard_RemovedCallback) == CARD_RESULT_READY)
                    {
                        Memcard_Wait();
                        result = 1;
                    }

                    CARDClose(&card_file_info);
                }
            }
            // unmount
            CARDUnmount(slot);
            stc_memcard_work->is_done = 0;
        }
    }

    return result;
}
// Confirm Dialog
void Menu_Confirm_Init(GOBJ *menu_gobj, int kind)
{

    // init cursor
    import_data.confirm.cursor = 0;
    import_data.menu_state = IMP_CONFIRM;
    import_data.confirm.kind = kind;

    // Create GOBJ
    GOBJ *confirm_gobj = GObj_Create(4, 5, 0);
    GObj_AddGXLink(confirm_gobj, GXLink_Common, MENUCAM_GXLINK, 130);
    JOBJ *confirm_jobj = JOBJ_LoadJoint(stc_import_assets->import_popup);
    GObj_AddObject(confirm_gobj, R13_U8(-0x3E55), confirm_jobj);
    import_data.confirm.gobj = confirm_gobj;

    // create text
    Text *text = Text_CreateText(SIS_ID, import_data.confirm.canvas);
    text->kerning = 1;
    text->align = 1;
    text->use_aspect = 1;
    text->aspect.X = 380;
    text->trans.Z = confirm_jobj->trans.Z;
    text->viewport_scale.X = (confirm_jobj->scale.X * 0.01) * 6;
    text->viewport_scale.Y = (confirm_jobj->scale.Y * 0.01) * 6;
    import_data.confirm.text = text;

    // decide text based on kind
    switch (kind)
    {
    case (CFRM_LOAD):
    {
        Text_AddSubtext(text, 0, -50, "Load this recording?");
        Text_AddSubtext(text, -65, 20, "Yes");
        Text_AddSubtext(text, 65, 20, "No");
        break;
    }
    case (CFRM_OLD):
    {
        Text_AddSubtext(text, 0, -50, "Cannot load outdated recording.");
        Text_AddSubtext(text, 0, 20, "OK");
        Text_SetColor(import_data.confirm.text, 1, &text_gold);
        break;
    }
    case (CFRM_NEW):
    {
        Text_AddSubtext(text, 0, -50, "Cannot load newer recording.");
        Text_AddSubtext(text, 0, 20, "OK");
        Text_SetColor(import_data.confirm.text, 1, &text_gold);
        break;
    }
    case(CFRM_NONE):
    {
        Text_AddSubtext(text, 0, -50, "No recordings found");
        Text_AddSubtext(text, 0, 20, "OK");
        Text_SetColor(import_data.confirm.text, 1, &text_gold);
        break;
    }
    case (CFRM_DEL):
    {
        Text_AddSubtext(text, 0, -50, "Delete this recording?");
        Text_AddSubtext(text, -65, 20, "Yes");
        Text_AddSubtext(text, 65, 20, "No");
        break;
    }
    case (CFRM_ERR):
    {
        Text_AddSubtext(text, 0, -70, "Corrupted recording(s) detected.");
        Text_AddSubtext(text, 0, -35, "Would you like to delete them?");
        Text_AddSubtext(text, -65, 40, "Yes");
        Text_AddSubtext(text, 65, 40, "No");
        break;
    }
    }
}

void Menu_Left_Right(int down, u8* cursor)
{
    if (import_data.confirm.cursor > 1)
        assert("cursor out of bounds");

    // cursor movement
    if (down & (HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT))
    {
        if (*cursor == 0)
        {
            *cursor = 1;
            SFX_PlayCommon(2);
        }
    }
    else if (down & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT))
    {
        if (*cursor == 1)
        {
            *cursor = 0;
            SFX_PlayCommon(2);
        }
    }
}

void Menu_Confirm_Think(GOBJ *menu_gobj)
{

    // init
    int down = Pad_GetRapidHeld(*stc_css_hmnport);

    // first ensure memcard is still inserted
    s32 memSize, sectorSize;
    if (CARDProbeEx(import_data.memcard_slot, &memSize, &sectorSize) != CARD_RESULT_READY)
    {
        Menu_Confirm_Exit(menu_gobj);
        SFX_PlayCommon(0);
        import_data.menu_state = IMP_SELFILE;
        return;
    }

    switch (import_data.confirm.kind)
    {
    case (CFRM_LOAD):
    {
        Menu_Left_Right(down, &import_data.confirm.cursor);

        // highlight cursor
        int cursor = import_data.confirm.cursor;
        for (int i = 0; i < 2; i++)
        {
            Text_SetColor(import_data.confirm.text, i + 1, &text_white);
        }
        Text_SetColor(import_data.confirm.text, cursor + 1, &text_gold);

        // check for exit
        if (down & HSD_BUTTON_B || (down & HSD_BUTTON_A && cursor == 1))
        {
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }
        else if (down & HSD_BUTTON_A)
        {
            // get variables and junk
            VSMinorData *css_minorscene = *stc_css_minorscene;
            int this_file_index = (import_data.page * IMPORT_FILESPERPAGE) + import_data.cursor;
            ExportHeader *header = &import_data.header[import_data.cursor];
            Preload *preload = Preload_GetTable();

            // get match data
            u8 hmn_kind = header->metadata.hmn;
            u8 hmn_costume = header->metadata.hmn_costume;
            u8 cpu_kind = header->metadata.cpu;
            u8 cpu_costume = header->metadata.cpu_costume;
            u16 stage_kind = header->metadata.stage_external;

            // set fighters
            css_minorscene->vs_data.match_init.playerData[0].c_kind = hmn_kind;
            css_minorscene->vs_data.match_init.playerData[0].costume = hmn_costume;
            preload->queued.fighters[0].kind = hmn_kind;
            preload->queued.fighters[0].costume = hmn_costume;
            css_minorscene->vs_data.match_init.playerData[1].c_kind = cpu_kind;
            css_minorscene->vs_data.match_init.playerData[1].costume = cpu_costume;
            preload->queued.fighters[1].kind = cpu_kind;
            preload->queued.fighters[1].costume = cpu_costume;

            // set stage
            css_minorscene->vs_data.match_init.stage = stage_kind;
            preload->queued.stage = stage_kind;

            // load files
            Preload_Update();

            // advance scene
            *stc_css_exitkind = 1;

            // HUGE HACK ALERT
            event_desc->stage = stage_kind;
            *onload_fileno = import_data.file_info[this_file_index].file_no;
            *onload_slot = import_data.memcard_slot;
            
            // prevent sheik/zelda transformation
            for (int i = 0; i < 4; ++i)
                stc_css_pad[i].held &= ~PAD_BUTTON_A;

            SFX_PlayCommon(1);
        }

        break;
    }
    case (CFRM_OLD):
    {
        // check for select
        if (down & (HSD_BUTTON_A | HSD_BUTTON_B))
        {
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }
        break;
    }
    case (CFRM_NEW):
    {
        // check for select
        if (down & (HSD_BUTTON_A | HSD_BUTTON_B))
        {
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }
        break;
    }
    case(CFRM_NONE):
    {
        // check for select
        if (down & (HSD_BUTTON_A | HSD_BUTTON_B))
        {
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELCARD;
            Menu_SelCard_Init(menu_gobj);
        }
        break;
    }
    case (CFRM_DEL):
    {
        Menu_Left_Right(down, &import_data.confirm.cursor);

        // highlight cursor
        int cursor = import_data.confirm.cursor;
        for (int i = 0; i < 2; i++)
        {
            Text_SetColor(import_data.confirm.text, i + 1, &text_white);
        }
        Text_SetColor(import_data.confirm.text, cursor + 1, &text_gold);

        if (down & HSD_BUTTON_B || (down & HSD_BUTTON_A && cursor == 1))
        {
            // go back
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }
        else if (down & HSD_BUTTON_A)
        {
            SFX_PlayCommon(1);

            // delete selected recording
            int file_idx = (import_data.page * IMPORT_FILESPERPAGE) + import_data.cursor;
            Menu_SelFile_DeleteFile(menu_gobj, file_idx);

            // close dialog
            Menu_Confirm_Exit(menu_gobj);

            // reload selfile menu
            Menu_SelFile_Exit(menu_gobj);
            Menu_SelFile_Init(menu_gobj);

            // if there's no errors and we're still selecting files
            // reset the cursor to the location of the prev deleted file
            if (import_data.menu_state == IMP_SELFILE) {
                if (file_idx == import_data.file_num && file_idx > 0)
                    file_idx--;
                import_data.page = file_idx / IMPORT_FILESPERPAGE;
                import_data.cursor = file_idx % IMPORT_FILESPERPAGE;

                int page_result = Menu_SelFile_LoadPage(menu_gobj, import_data.page);
                if (page_result == -1)
                {
                    // create dialog
                    Menu_Confirm_Init(menu_gobj, CFRM_ERR);
                    SFX_PlayCommon(3);
                }
            }
        }
        break;
    }
    case (CFRM_ERR):
    {
        Menu_Left_Right(down, &import_data.confirm.cursor);

        // highlight cursor
        int cursor = import_data.confirm.cursor;
        for (int i = 0; i < 2; i++)
        {
            Text_SetColor(import_data.confirm.text, i + 2, &text_white);
        }
        Text_SetColor(import_data.confirm.text, cursor + 2, &text_gold);

        // check for back
        if (down & HSD_BUTTON_B || (down & HSD_BUTTON_A && cursor == 1))
        {
            Menu_Confirm_Exit(menu_gobj); // close dialog
            Menu_SelFile_Exit(menu_gobj); // close select file
            Menu_SelCard_Init(menu_gobj); // open select card
            SFX_PlayCommon(0);
            //import_data.menu_state = IMP_SELCARD;
        }
        else if (down & HSD_BUTTON_A)
        {
            SFX_PlayCommon(1);

            // close dialog
            Menu_Confirm_Exit(menu_gobj);

            // reload selfile
            Menu_SelFile_Exit(menu_gobj); // close select file
            Menu_SelFile_Init(menu_gobj); // open select file
        }
        break;
    }
    }
}
void Menu_Confirm_Exit(GOBJ *menu_gobj)
{
    // destroy option text
    Text_Destroy(import_data.confirm.text);
    import_data.confirm.text = 0;

    // destroy gobj
    GObj_Destroy(import_data.confirm.gobj);
    import_data.confirm.gobj = 0;
}

// Misc functions
void Memcard_Wait(void)
{
    while (stc_memcard_work->is_done == 0)
    {
        blr2();
    }
}
ExportHeader *GetExportHeaderFromCard(int slot, char *fileName, void *buffer) {
    CARDFileInfo card_file_info;

    // open card (get file info)
    if (CARDOpen(slot, fileName, &card_file_info) != CARD_RESULT_READY) {
        return NULL;
    }

    if (CARDRead(&card_file_info, buffer, CARD_READ_SIZE, 0x1E00) != CARD_RESULT_READY) {
        CARDClose(&card_file_info);
        return NULL;
    }

    // deobfuscate stupid melee bullshit
    Memcard_Deobfuscate(buffer, CARD_READ_SIZE);
    ExportHeader *header = buffer + 0x90; // get to header (need to find a less hardcoded way of doing this)

    CARDClose(&card_file_info);
    return header;
}

// converts from external character IDs to CSSs idx - i.e. sheik/zelda popo/nana become the same.
int CSS_ID(int ext_id) {
    if (ext_id == CKIND_SHEIK) return CKIND_ZELDA;
    if (ext_id == CKIND_POPO) return CKIND_ICECLIMBERS;
    return ext_id;
}

int GetSelectedFighterIdOnCssForHmn() {
    // Get selected fighter for hmn in CSS
    VSMinorData *css_minorscene = *stc_css_minorscene;
    int hmnFighterId = css_minorscene->vs_data.match_init.playerData[*stc_css_hmnport].c_kind;

    // When any fighters are not selected, this ID seems to be unplayable fighter's ID (26=Masterhand or 33=None basically?). 0-25 are playable fighter's ID.
    return hmnFighterId < 26 ? hmnFighterId : -1;
}
