#include "../MexTK/mex.h"
#include "events.h"

void StartFreePractice(GOBJ *gobj);

void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

enum options_main 
{
    OPT_FREEPRACTICE,
    OPT_EXIT,

    OPT_COUNT
};

static EventOption Options_Main[OPT_COUNT] = {
    {
        .kind = OPTKIND_FUNC,
        .name = "Start Free Practice",
        .desc = { "Enable endless practice." },
        .val = 0,
        .disable = 0,
        .OnSelect = StartFreePractice
    },
    {
        .kind = OPTKIND_FUNC,
        .name = "Exit",
        .desc = { "Return to the Event Select Screen." },
        .OnSelect = Exit,
    },
};

void StartFreePractice(GOBJ *gobj) {
    Options_Main[OPT_FREEPRACTICE].disable = 1;
    stc_match->match.timer = MATCH_TIMER_COUNTUP;
}

static EventMenu Menu_Main = {
    .name = "Eggs-ercise",
    .option_num = countof(Options_Main),
    .options = Options_Main,
};

static int egg_counter = 0;
Vec3 coll_pos, last_coll_pos;
GOBJ *egg_gobj, *effect_gobj;
JOBJ *effect_jobj;
CmSubject *cam;

void Event_Init(GOBJ *gobj)
{
    // initialize egg camera subject
    cam = CameraSubject_Alloc();
    cam->boundleft_proj = -5;
    cam->boundright_proj = 10;
    cam->boundtop_proj = 10;
    cam->boundbottom_proj = -10;

    // gobj for writing gfx to 
    effect_gobj = GObj_Create(10, 11, 0);
    effect_jobj = effect_gobj->hsd_object;
}

// generate random float between low and high
float RandomRange(float low, float high)
{
    return low + (high - low) * HSD_Randf();
}


GOBJ *Egg_Spawn(void)
{
    float ground_width = 8;
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
            continue; // Try a new random point
        
        // check distance from last collision
        float distance = Math_Vec3Distance(&coll_pos, &last_coll_pos);
        if (distance < 25.f)
            continue; 

        // check if too close to right end of the stage
        Vec3 near_pos;
        float near_x = x + ground_width;
        is_ground = GrColl_RaycastGround(&near_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, 
            near_x, from_y, near_x, to_y, 0);
        if (is_ground == 0)
            continue; 

        // check if too close to left end of the stage
        near_x = x - ground_width;
        is_ground = GrColl_RaycastGround(&near_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, 
            near_x, from_y, near_x, to_y, 0);
        if (is_ground == 0)
            continue; 
        
        break; 
    }
    
    // random Y velocity on spawn
    Vec3 rand_velocity = {0, 0.4 + HSD_Randf() * 2, 0};
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
    effect_jobj->trans = egg_data->pos;
    Effect_SpawnSync(1232, effect_gobj, &effect_jobj->trans);
    Item_PlayOnDestroySFXAgain(egg_data, 244, 127, 64);

    // manage old and new egg
    Item_Destroy(gobj);
    egg_gobj = Egg_Spawn();
    egg_counter++;

    char buffer[10];
    sprintf(buffer, "%d\n", egg_counter);
    OSReport(buffer);
    return 0;
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
            int *staleMoveTable = Fighter_GetStaleMoveTable(i);
            memset(staleMoveTable, 0, 0x2C);
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

        // set these callbacks every frame or else gets overwritten
        egg_data->it_func->OnTakeDamage = Egg_OnTakeDamage;

        // update camera position
        cam->cam_pos.X = egg_data->pos.X;
        cam->cam_pos.Y = egg_data->pos.Y + 15; // add so it's not awkwardly low
        cam->cam_pos.Z = egg_data->pos.Z;
    }
}

EventMenu *Event_Menu = &Menu_Main;