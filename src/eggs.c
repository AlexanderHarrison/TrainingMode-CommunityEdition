#include "../MexTK/mex.h"
#include "events.h"

static int in_free_practice = 0;
Vec3 coll_pos = {0, 0, 0};
GOBJ *egg_gobj;
CmSubject *cam;

void Event_Init(GOBJ *gobj)
{
    cam = CameraSubject_Alloc();
    cam->boundleft_proj = -10;
    cam->boundright_proj = 10;
    cam->boundtop_proj = 10;
    cam->boundbottom_proj = -10;
}

// generate random float between low and high
float RandFloatRange(float low, float high)
{
    return HSD_Randf() * (high - low) + low;
}

GOBJ* SpawnEgg(){
    float camera_left = Stage_GetCameraLeft();
    float camera_right = Stage_GetCameraRight();
    float camera_top = Stage_GetCameraTop();
    float camera_bottom = Stage_GetCameraBottom();

    // Pick a random point on the stage via raycast down from random point
    Vec2 egg_spawn_vec = {RandFloatRange(camera_left, camera_right), RandFloatRange(camera_bottom, camera_top)};
    Vec3 line_unk = {0,0,0};
    int line_index, line_kind;
    GrColl_RaycastGround(&coll_pos, &line_index, &line_kind, &line_unk, -1, -1, -1, 0, 
        egg_spawn_vec.X, egg_spawn_vec.Y, egg_spawn_vec.X, egg_spawn_vec.Y-1000, 0);
    
    // random Y velocity on spawn
    Vec3 rand_velocity = {0, HSD_Randf() + 2, 0};
    SpawnItem item_egg = {
        .it_kind = ITEM_EGG,
        .parent_gobj2 = 0,
        .parent_gobj = 0, 
        .pos = coll_pos,
        .pos2 = coll_pos,
        .vel = rand_velocity
    };

    GOBJ *egg_gobj = Item_CreateItem2(&item_egg);

    return egg_gobj;
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

    // on DPAD down set free practice on
    if (hmn_data->input.down & PAD_BUTTON_DPAD_DOWN && in_free_practice == 0) {
        OSReport("enter free practice\n");
        in_free_practice = 1;

        // play sound
        SFX_Play(0x82);

        // count timer up
        stc_match->match.timer = MATCH_TIMER_COUNTUP;
    }

    // wait for first frame of player control to spawn first egg
    if (egg_gobj == 0 && hmn_data->flags.input_enable)
    {
        egg_gobj = SpawnEgg();
    }

    // when egg exists
    if (egg_gobj != 0) 
    {
        // don't let players pick egg up or nudge it
        ItemData *egg_data = egg_gobj->userdata;
        egg_data->can_hold = 0;
        egg_data->can_nudge = 0;

        // update camera position
        cam->cam_pos.X = egg_data->pos.X;
        cam->cam_pos.Y = egg_data->pos.Y + 15;
        cam->cam_pos.Z = egg_data->pos.Z;
    }
}