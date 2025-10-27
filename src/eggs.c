#include "../MexTK/mex.h"
#include "eggs.h"

void Exit(GOBJ *menu) 
{
    stc_match->state = 3;
    Match_EndVS();
}

void Retry(GOBJ *menu) 
{
    stc_match->end_kind = MATCHENDKIND_RETRY;
    stc_match->state = 3;
    Match_EndVS();
}

float RandomRange(float low, float high)
{
    return low + (high - low) * HSD_Randf();
}

void ChangeHitDisplay(GOBJ *menu_gobj, int value)
{
    // loop through all fighters
    GOBJ *this_fighter = (*stc_gobj_lookup)[MATCHPLINK_FIGHTER];
    while (this_fighter != 0)
    {
        // get data
        FighterData *this_fighter_data = this_fighter->userdata;

        // toggle
        this_fighter_data->show_hit = value;

        // get next fighter
        this_fighter = this_fighter->next;
    }
}

void StartFreePractice(GOBJ *gobj) {
    Options_Main[OPT_FREEPRACTICE].disable = 1;
    Options_Main[OPT_DAMAGETHRESHOLD].disable = 0;
    Options_Main[OPT_SCALE].disable = 0;
    Options_Main[OPT_VELOCITY].disable = 0;
    Options_Main[OPT_COLLISION].disable = 0;
    stc_match->match.timer = MATCH_TIMER_COUNTUP;
}

void Egg_OnChangeSize(GOBJ *menu, int value)
{
    if (egg_gobj != 0)
    {
        Item_Destroy(egg_gobj);
        egg_gobj = Egg_Spawn();
    }
}

GOBJ *Egg_Spawn(void)
{
    float min_ground_width = 8;
    float camera_left = Stage_GetCameraLeft();
    float camera_right = Stage_GetCameraRight();
    float camera_top = Stage_GetCameraTop();
    float camera_bottom = Stage_GetCameraBottom();
    int is_ground = 0;

    while (1)
    {
        // pick a random point on the stage via raycast down from random point
        float x = RandomRange(camera_left, camera_right);
        float from_y = RandomRange(camera_bottom, camera_top);
        float to_y = from_y - 1000;
        Vec3  line_unk;
        int line_index, line_kind;

        // check main ground collision
        is_ground = GrColl_RaycastGround(&coll_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, 
            x, from_y, x, to_y, 0);
        if (is_ground == 0)
            continue; // try a new random point
        
        // check distance from last collision
        float distance = Math_Vec3Distance(&coll_pos, &last_coll_pos);
        if (distance < 25.f)
            continue; 

        // check if too close to right end of the stage
        Vec3 near_pos;
        float near_x = x + min_ground_width;
        is_ground = GrColl_RaycastGround(&near_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, 
            near_x, from_y, near_x, to_y, 0);
        if (is_ground == 0)
            continue; 

        // check if too close to left end of the stage
        near_x = x - min_ground_width;
        is_ground = GrColl_RaycastGround(&near_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, 
            near_x, from_y, near_x, to_y, 0);
        if (is_ground == 0)
            continue; 
        
        break; 
    }
    
    // random Y velocity on spawn
    Vec3 rand_velocity = {0, (2 + HSD_Randf()) * Options_Main[OPT_VELOCITY].val, 0};
    SpawnItem item_egg = {
        .it_kind = ITEM_EGG,
        .pos = coll_pos,
        .pos2 = coll_pos,
        .vel = rand_velocity,
    };

    last_coll_pos = coll_pos;

    return Item_CreateItem2(&item_egg);
}

int Egg_OnTakeDamage(GOBJ *gobj)
{
    // gfx and sfx
    ItemData *egg_data = egg_gobj->userdata;
    int damage = egg_data->dmg.recent;
    if (damage >= Options_Main[OPT_DAMAGETHRESHOLD].val){
        Effect_SpawnSync(1232, gobj, egg_data->pos);
        Item_PlayOnDestroySFXAgain(egg_data, 244, 127, 64);
        
        // manage old and new egg
        Egg_Destroy(gobj);
        egg_gobj = Egg_Spawn();
        egg_counter++;
    }
    return 0;
}

void Event_Init(GOBJ *gobj)
{
    // initialize egg camera subject
    cam = CameraSubject_Alloc();
    cam->boundleft_proj = -10;
    cam->boundright_proj = 10;
    cam->boundtop_proj = 10;
    cam->boundbottom_proj = -10;

    // hud setup
    hud_score_gobj = GObj_Create(0, 0, 0);
    GObj_AddObject(hud_score_gobj, 3, hud_score_jobj);
    GObj_AddGXLink(hud_score_gobj, GXLink_Common, GXLINK_HUD, 80);

    hud_best_gobj = GObj_Create(0, 0, 0);
    GObj_AddObject(hud_best_gobj, 3, hud_best_jobj);
    GObj_AddGXLink(hud_best_gobj, GXLink_Common, GXLINK_HUD, 80);
    canvas = Text_CreateCanvas(2, 0, 0, 0, 0, GXLINK_HUD, 81, 19);
    
    // Create text objects
    hud_score_text = Text_CreateText(2, canvas);
    hud_score_text->kerning = 1;
    hud_score_text->align = 0;
    hud_score_text->use_aspect = 1;
    hud_score_text->trans.X = 50;
    hud_score_text->trans.Y = 25;
    hud_score_text->viewport_scale.X = 1;
    hud_score_text->viewport_scale.Y = 1;
    
    hud_best_text = Text_CreateText(2, canvas);
    hud_best_text->kerning = 1;
    hud_best_text->align = 0;
    hud_best_text->use_aspect = 1;
    hud_best_text->trans.X = 50;
    hud_best_text->trans.Y = 40;
    hud_best_text->scale.X = 0.35;
    hud_best_text->scale.Y = 0.35;
    hud_best_text->viewport_scale.X = 1;
    hud_best_text->viewport_scale.Y = 1;

    // dummy text
    Text_AddSubtext(hud_score_text, 0, 0, "-");
    Text_AddSubtext(hud_best_text, 0, 0, "-");
    
    high_score = Events_GetSavedScore(stc_memcard->EventBackup.event);

    stc_match->end_kind = MATCHENDKIND_NONE;
}

void Event_Think(GOBJ *event)
{
    // reset stale moves
    for (int i = 0; i < 6; i++)
    {
        // check if fighter exists
        GOBJ *fighter = Fighter_GetGObj(i);
        if (fighter != 0)
        {
            // reset stale move table
            int *stale_move_table = Fighter_GetStaleMoveTable(i);
            memset(stale_move_table, 0, 0x2C);
        }
    }

    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // wait for first frame of player control to spawn first egg
    if (egg_gobj == 0 && hmn_data->flags.input_enable)
    {   
        egg_gobj = Egg_Spawn();
    }

    // when egg exists
    if (egg_gobj != 0) 
    {
        // don't allow holding or nudging
        ItemData *egg_data = egg_gobj->userdata;
        egg_data->can_hold = 0;
        egg_data->can_nudge = 0;
        egg_data->self_vel.X = 0;

        // apply scaling changes
        JOBJ *egg_jobj = Item_GetBoneJOBJ(egg_gobj, 0);
        egg_jobj->scale.X = EggOptions_Size[Options_Main[OPT_SCALE].val];
        egg_jobj->scale.Y = EggOptions_Size[Options_Main[OPT_SCALE].val];
        egg_jobj->scale.Z = EggOptions_Size[Options_Main[OPT_SCALE].val];
        egg_data->scale = EggOptions_Size[Options_Main[OPT_SCALE].val];

        // show egg hurtbox on hit
        egg_data->show_hit = Options_Main[OPT_COLLISION].val;
        egg_data->show_model = !Options_Main[OPT_COLLISION].val;

        // set this callbacks every frame or else gets overwritten
        egg_data->it_func->OnTakeDamage = Egg_OnTakeDamage;

        // update camera position
        cam->cam_pos.X = egg_data->pos.X;
        cam->cam_pos.Y = egg_data->pos.Y + 15; // add so it's not awkwardly low
        cam->cam_pos.Z = egg_data->pos.Z;
    }

    // Update egg count, hide when paused
    if (Pause_CheckStatus(1) != 2)
    {
        Text_SetText(hud_score_text, 0, "%d", egg_counter);
        if (stc_match->match.timer == MATCH_TIMER_COUNTUP)
        {
            Text_SetText(hud_best_text, 0, "Free Practice");
            Text_SetColor(hud_best_text, 0, &text_gold);
        }
        else if (high_score > 0)
        {
            Text_SetText(hud_best_text, 0, "Best: %d", high_score);
            Text_SetColor(hud_best_text, 0, &text_white);
        }
        else
        {
            Text_SetText(hud_best_text, 0, "");
        }
    } 
    else
    {
        Text_SetText(hud_score_text, 0, "");
        Text_SetText(hud_best_text, 0, "");
    }

    // After 1 minute and not in free practice
    if (stc_match->time_frames == 360 && stc_match->match.timer != MATCH_TIMER_COUNTUP)
    {   
        int event_id = stc_memcard->EventBackup.event;
        Events_SetEventAsPlayed(event_id); // I don't know why this doesn't work, but it really should
        Match_SetEndGraphic(MATCHENDKIND_VSGAME);

        // high score logic
        int high_score = Events_GetSavedScore(event_id);
        if (egg_counter > high_score)
        {
            MatchInfo_0x0010_store(0x9c40); // Makes "new record" audio happen, IDK why
            Match_SetPostMatchSFX(325);
            Events_StoreEventScore(event_id, egg_counter);
        }
        else
        {
            Match_SetPostMatchSFX(324);
        }

        Match_EndImmediate();
    }
}

EventMenu *Event_Menu = &Menu_Main;