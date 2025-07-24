#include "events.h"

// use enum savestate_flags for flags
int Savestate_Save_v1(Savestate_v1 *savestate, int flags)
{
    typedef struct BackupQueue
    {
        GOBJ *fighter;
        FighterData *fighter_data;
    } BackupQueue;

    // ensure no players are in problematic states
    int canSave = 1;
    
    if (flags & Savestate_Checks) {
        GOBJ **gobj_list = R13_PTR(-0x3E74);
        GOBJ *fighter = gobj_list[8];
        while (fighter != 0)
        {
           FighterData *fighter_data = fighter->userdata;
    
           if ((fighter_data->cb.OnDeath_Persist != 0) ||
              (fighter_data->cb.OnDeath_State != 0) ||
              (fighter_data->cb.OnDeath3 != 0) ||
              (fighter_data->item_held != 0) ||
              (fighter_data->x1978 != 0) ||
              (fighter_data->accessory != 0) ||
              ((fighter_data->kind == FTKIND_NESS) && ((fighter_data->state_id >= 342) && (fighter_data->state_id <= 344)))) // hardcode ness' usmash because it doesnt destroy the yoyo via onhit callback...
           {
              // cannot save
              canSave = 0;
              break;
           }
    
           fighter = fighter->next;
        }
    }

    // loop through all players
    int isSaved = 0;
    if (canSave == 1)
    {

        savestate->is_exist = 1;

        // save frame
        savestate->frame = event_vars->game_timer;

        // save event data
        memcpy(&savestate->event_data, event_vars->event_gobj->userdata, sizeof(savestate->event_data));

        // backup all players
        for (int i = 0; i < 6; i++)
        {
            // get fighter gobjs
            BackupQueue queue[2];
            for (int j = 0; j < 2; j++)
            {
                GOBJ *fighter = 0;
                FighterData *fighter_data = 0;

                // get fighter gobj and data if they exist
                fighter = Fighter_GetSubcharGObj(i, j);
                if (fighter != 0)
                    fighter_data = fighter->userdata;

                // store fighter pointers
                queue[j].fighter = fighter;
                queue[j].fighter_data = fighter_data;
            }

            // if the main fighter exists
            if (queue[0].fighter != 0)
            {

                FtSaveState_v1 *ft_state = &savestate->ft_state[i];

                isSaved = 1;

                // We use the Savestate's Playerblock size because it's smaller
                // than the Playerblock defined in fighter.h
                Playerblock *playerblock = Fighter_GetPlayerblock(queue[0].fighter_data->ply);
                memcpy(&ft_state->player_block, playerblock, sizeof(ft_state->player_block));

                int *stale_queue = Fighter_GetStaleMoveTable(queue[0].fighter_data->ply);
                memcpy(&ft_state->stale_queue, stale_queue, sizeof(ft_state->stale_queue));

                // backup each subfighters data
                for (int j = 0; j < 2; j++)
                {
                    // if exists
                    if (queue[j].fighter != 0)
                    {

                        FtSaveStateData_v1 *ft_data = &ft_state->data[j];
                        FighterData *fighter_data = queue[j].fighter_data;

                        // backup to ft_state
                        ft_data->is_exist = 1;
                        ft_data->state_id = fighter_data->state_id;
                        ft_data->facing_direction = fighter_data->facing_direction;
                        ft_data->state_frame = fighter_data->state.frame;
                        ft_data->state_rate = fighter_data->state.rate;
                        ft_data->state_blend = fighter_data->state.blend;
                        memcpy(&ft_data->phys, &fighter_data->phys, sizeof(fighter_data->phys));                               // copy physics
                        memcpy(&ft_data->color, &fighter_data->color, sizeof(fighter_data->color));                            // copy color overlay
                        memcpy(&ft_data->input, &fighter_data->input, sizeof(fighter_data->input));                            // copy inputs
                        memcpy(&ft_data->coll_data, &fighter_data->coll_data, sizeof(fighter_data->coll_data));                // copy collision
                        memcpy(&ft_data->camera_subject, fighter_data->camera_subject, sizeof(ft_data->camera_subject));       // copy camerabox
                        memcpy(&ft_data->hitbox, &fighter_data->hitbox, sizeof(fighter_data->hitbox));                         // copy hitbox
                        memcpy(&ft_data->throw_hitbox, &fighter_data->throw_hitbox, sizeof(fighter_data->throw_hitbox));       // copy hitbox
                        memcpy(&ft_data->thrown_hitbox, &fighter_data->thrown_hitbox, sizeof(fighter_data->thrown_hitbox));    // copy hitbox
                        memcpy(&ft_data->flags, &fighter_data->flags, sizeof(fighter_data->flags));                            // copy flags
                        memcpy(&ft_data->fighter_var, &fighter_data->fighter_var, sizeof(fighter_data->fighter_var));          // copy var
                        memcpy(&ft_data->state_var, &fighter_data->state_var, sizeof(fighter_data->state_var));                // copy var
                        memcpy(&ft_data->ftcmd_var, &fighter_data->ftcmd_var, sizeof(fighter_data->ftcmd_var));                // copy var
                        memcpy(&ft_data->jump, &fighter_data->jump, sizeof(fighter_data->jump));                               // copy var
                        memcpy(&ft_data->smash, &fighter_data->smash, sizeof(fighter_data->smash));                            // copy var
                        memcpy(&ft_data->hurt, &fighter_data->hurt, sizeof(fighter_data->hurt));                               // copy var
                        memcpy(&ft_data->shield, &fighter_data->shield, sizeof(fighter_data->shield));                         // copy hitbox
                        memcpy(&ft_data->shield_bubble, &fighter_data->shield_bubble, sizeof(fighter_data->shield_bubble));    // copy hitbox
                        memcpy(&ft_data->reflect_bubble, &fighter_data->reflect_bubble, sizeof(fighter_data->reflect_bubble)); // copy hitbox
                        memcpy(&ft_data->absorb_bubble, &fighter_data->absorb_bubble, sizeof(fighter_data->absorb_bubble));    // copy hitbox
                        memcpy(&ft_data->reflect_hit, &fighter_data->reflect_hit, sizeof(fighter_data->reflect_hit));          // copy hitbox
                        memcpy(&ft_data->absorb_hit, &fighter_data->absorb_hit, sizeof(fighter_data->absorb_hit));             // copy hitbox

                        // copy dmg
                        // latest MexTK and savestate have different sizes for dmg struct, so we use savestates's size
                        memcpy(&ft_data->dmg, &fighter_data->dmg, sizeof(ft_data->dmg));
                        ft_data->dmg.hit_log.source = GOBJToID(ft_data->dmg.hit_log.source);

                        // copy grab
                        memcpy(&ft_data->grab, &fighter_data->grab, sizeof(fighter_data->grab));
                        ft_data->grab.attacker = GOBJToID(ft_data->grab.attacker);
                        ft_data->grab.victim = GOBJToID(ft_data->grab.victim);

                        // copy callbacks
                        memcpy(&ft_data->cb, &fighter_data->cb, sizeof(fighter_data->cb)); // copy hitbox

                        // convert hitbox pointers
                        for (u32 k = 0; k < countof(fighter_data->hitbox); k++)
                        {
                            ft_data->hitbox[k].bone = BoneToID(fighter_data, ft_data->hitbox[k].bone);
                            for (u32 l = 0; l < countof(fighter_data->hitbox->victims); l++) // pointers to hitbox victims
                            {
                                ft_data->hitbox[k].victims[l].data = FtDataToID(ft_data->hitbox[k].victims[l].data);
                            }
                        }
                        for (u32 k = 0; k < countof(fighter_data->throw_hitbox); k++)
                        {
                            ft_data->throw_hitbox[k].bone = BoneToID(fighter_data, ft_data->throw_hitbox[k].bone);
                            for (u32 l = 0; l < countof(fighter_data->throw_hitbox->victims); l++) // pointers to hitbox victims
                            {
                                ft_data->throw_hitbox[k].victims[l].data = FtDataToID(ft_data->throw_hitbox[k].victims[l].data);
                            }
                        }

                        ft_data->thrown_hitbox.bone = BoneToID(fighter_data, ft_data->thrown_hitbox.bone);
                        for (u32 k = 0; k < countof(fighter_data->thrown_hitbox.victims); k++) // pointers to hitbox victims
                        {

                            ft_data->thrown_hitbox.victims[k].data = FtDataToID(ft_data->thrown_hitbox.victims[k].data);
                        }

                        // copy XRotN rotation
                        s8 XRotN_id = Fighter_BoneLookup(fighter_data, XRotN);
                        if (XRotN_id != -1)
                        {
                            ft_data->XRotN_rot = fighter_data->bones[XRotN_id].joint->rot;
                        }
                    }
                }
            }
        }
    }

    if ((flags & Savestate_Silent) == 0) {
        // Play SFX
        if (isSaved == 0)
        {
            SFX_PlayCommon(3);
        }
        if (isSaved == 1)
        {
            // play sfx
            SFX_PlayCommon(1);
    
            // if not in frame advance, flash screen. I wrote it like this because the second condition kept getting optimized out
            if ((Pause_CheckStatus(0) != 1))
            {
                if ((Pause_CheckStatus(1) != 2))
                {
                    ScreenFlash_Create(2, 0);
                }
            }
        }
    }

    return isSaved;
}
// use enum savestate_flags for flags
int Savestate_Load_v1(Savestate_v1 *savestate, int flags)
{
    typedef struct BackupQueue
    {
        GOBJ *fighter;
        FighterData *fighter_data;
    } BackupQueue;

    // loop through all players
    int isLoaded = 0;
    for (int i = 0; i < 6; i++)
    {
        // get fighter gobjs
        BackupQueue queue[2];
        for (int j = 0; j < 2; j++)
        {
            GOBJ *fighter = 0;
            FighterData *fighter_data = 0;

            // get fighter gobj and data if they exist
            fighter = Fighter_GetSubcharGObj(i, j);
            if (fighter != 0)
                fighter_data = fighter->userdata;

            // store fighter pointers
            queue[j].fighter = fighter;
            queue[j].fighter_data = fighter_data;
        }

        // if the main fighter and backup exists
        if ((queue[0].fighter != 0) && (savestate->ft_state[i].data[0].is_exist == 1))
        {

            FtSaveState_v1 *ft_state = &savestate->ft_state[i];

            isLoaded = 1;

            // restore playerblock
            Playerblock *playerblock = Fighter_GetPlayerblock(queue[0].fighter_data->ply);
            GOBJ *fighter_gobj[2];
            fighter_gobj[0] = playerblock->gobj[0];
            fighter_gobj[1] = playerblock->gobj[1];
            memcpy(playerblock, &ft_state->player_block, sizeof(ft_state->player_block));
            playerblock->gobj[0] = fighter_gobj[0];
            playerblock->gobj[1] = fighter_gobj[1];

            // restore stale moves
            int *stale_queue = Fighter_GetStaleMoveTable(queue[0].fighter_data->ply);
            memcpy(stale_queue, &ft_state->stale_queue, sizeof(ft_state->stale_queue));

            // restore fighter data
            for (int j = 0; j < 2; j++)
            {

                GOBJ *fighter = queue[j].fighter;

                if (fighter != 0)
                {

                    // get state
                    FtSaveStateData_v1 *ft_data = &ft_state->data[j];
                    FighterData *fighter_data = queue[j].fighter_data;

                    // sleep
                    Fighter_EnterSleep(fighter, 0);

                    fighter_data->state_id = ft_data->state_id;
                    fighter_data->facing_direction = ft_data->facing_direction;
                    if (flags & Savestate_Mirror)
                    {
                        fighter_data->facing_direction *= -1;
                    }
                    fighter_data->state.frame = ft_data->state_frame;
                    fighter_data->state.rate = ft_data->state_rate;
                    fighter_data->state.blend = ft_data->state_blend;

                    // restore phys struct
                    memcpy(&fighter_data->phys, &ft_data->phys, sizeof(fighter_data->phys)); // copy physics
                    if (flags & Savestate_Mirror)
                    {
                        fighter_data->phys.anim_vel.X *= -1;
                        fighter_data->phys.self_vel.X *= -1;
                        fighter_data->phys.kb_vel.X *= -1;
                        fighter_data->phys.atk_shield_kb_vel.X *= -1;
                        fighter_data->phys.pos.X *= -1;
                        fighter_data->phys.pos_prev.X *= -1;
                        fighter_data->phys.pos_delta.X *= -1;
                        fighter_data->phys.horzitonal_velocity_queue_will_be_added_to_0xec *= -1;
                        fighter_data->phys.self_vel_ground.X *= -1;
                        fighter_data->phys.nudge_vel.X *= -1;
                    }

                    // restore inputs
                    memcpy(&fighter_data->input, &ft_data->input, sizeof(fighter_data->input)); // copy inputs
                    if (flags & Savestate_Mirror)
                    {
                        fighter_data->input.lstick.X *= -1;
                        fighter_data->input.lstick_prev.X *= -1;
                        fighter_data->input.cstick.X *= -1;
                        fighter_data->input.cstick_prev.X *= -1;
                    }

                    // restore coll data
                    CollData *thiscoll = &fighter_data->coll_data;
                    GOBJ *gobj = thiscoll->gobj;                                                            // 0x0
                    JOBJ *joint_1 = thiscoll->joint_1;                                                      // 0x108
                    JOBJ *joint_2 = thiscoll->joint_2;                                                      // 0x10c
                    JOBJ *joint_3 = thiscoll->joint_3;                                                      // 0x110
                    JOBJ *joint_4 = thiscoll->joint_4;                                                      // 0x114
                    JOBJ *joint_5 = thiscoll->joint_5;                                                      // 0x118
                    JOBJ *joint_6 = thiscoll->joint_6;                                                      // 0x11c
                    JOBJ *joint_7 = thiscoll->joint_7;                                                      // 0x120
                    memcpy(&fighter_data->coll_data, &ft_data->coll_data, sizeof(fighter_data->coll_data)); // copy collision
                    thiscoll->gobj = gobj;
                    thiscoll->joint_1 = joint_1;
                    thiscoll->joint_2 = joint_2;
                    thiscoll->joint_3 = joint_3;
                    thiscoll->joint_4 = joint_4;
                    thiscoll->joint_5 = joint_5;
                    thiscoll->joint_6 = joint_6;
                    thiscoll->joint_7 = joint_7;
                    if (flags & Savestate_Mirror)
                    {
                        thiscoll->topN_Curr.X *= -1;
                        thiscoll->topN_CurrCorrect.X *= -1;
                        thiscoll->topN_Prev.X *= -1;
                        thiscoll->topN_Proj.X *= -1;

                        // if the fighter is on a left/right platform, it needs to set the opposite platform's ground_index to prevent it from stucking at edge of the platform
                        int stage_internal_id = Stage_ExternalToInternal(Stage_GetExternalID());
                        struct {
                            int stage_internal_id;
                            int left_index;
                            int right_index;
                        } platform_ground_indices[] = {
                            {GRKIND_STORY, 1, 5}, // platforms
                            {GRKIND_STORY, 2, 6}, // slants
                            {GRKIND_IZUMI, 0, 1}, // platforms
                            {GRKIND_IZUMI, 3, 7}, // edges
                            {GRKIND_IZUMI, 4, 6}, // transition
                            {GRKIND_PSTAD, 35, 36}, // platforms
                            {GRKIND_PSTAD, 51, 54}, // edges
                            {GRKIND_PSTAD, 52, 53}, // transition
                            {GRKIND_OLDPU, 0, 1}, // platforms
                            {GRKIND_OLDPU, 3, 5}, // edges
                            {GRKIND_BATTLE, 2, 4}, // platforms
                            {GRKIND_BATTLE, 0, 5}, // edges
                            {GRKIND_FD, 0, 2}, // edges
                        };
                        for (u32 i = 0; i < countof(platform_ground_indices); i++)
                        {
                            if (platform_ground_indices[i].stage_internal_id == stage_internal_id)
                            {
                                if (thiscoll->ground_index == platform_ground_indices[i].left_index)
                                    thiscoll->ground_index = platform_ground_indices[i].right_index;
                                else if (thiscoll->ground_index == platform_ground_indices[i].right_index)
                                    thiscoll->ground_index = platform_ground_indices[i].left_index;
                            }
                        }
                    }

                    // restore hitboxes
                    memcpy(&fighter_data->hitbox, &ft_data->hitbox, sizeof(fighter_data->hitbox));                   // copy hitbox
                    memcpy(&fighter_data->throw_hitbox, &ft_data->throw_hitbox, sizeof(fighter_data->throw_hitbox)); // copy hitbox
                    memcpy(&fighter_data->thrown_hitbox, &ft_data->thrown_hitbox, sizeof(fighter_data->thrown_hitbox));       // copy hitbox

                    // copy grab
                    memcpy(&fighter_data->grab, &ft_data->grab, sizeof(fighter_data->grab));
                    fighter_data->grab.attacker = IDToGOBJ(fighter_data->grab.attacker);
                    fighter_data->grab.victim = IDToGOBJ(fighter_data->grab.victim);

                    // convert pointers

                    for (u32 k = 0; k < countof(fighter_data->hitbox); k++)
                    {
                        fighter_data->hitbox[k].bone = IDToBone(fighter_data, ft_data->hitbox[k].bone);
                        for (u32 l = 0; l < countof(fighter_data->hitbox->victims); l++) // pointers to hitbox victims
                        {
                            fighter_data->hitbox[k].victims[l].data = IDToFtData(ft_data->hitbox[k].victims[l].data);
                        }
                    }
                    for (u32 k = 0; k < countof(fighter_data->throw_hitbox); k++)
                    {
                        fighter_data->throw_hitbox[k].bone = IDToBone(fighter_data, ft_data->throw_hitbox[k].bone);
                        for (u32 l = 0; l < countof(fighter_data->throw_hitbox->victims); l++) // pointers to hitbox victims
                        {
                            fighter_data->throw_hitbox[k].victims[l].data = IDToFtData(ft_data->throw_hitbox[k].victims[l].data);
                        }
                    }
                    fighter_data->thrown_hitbox.bone = IDToBone(fighter_data, ft_data->thrown_hitbox.bone);
                    for (u32 k = 0; k < countof(fighter_data->thrown_hitbox.victims); k++) // pointers to hitbox victims
                    {
                        fighter_data->thrown_hitbox.victims[k].data = IDToFtData(ft_data->thrown_hitbox.victims[k].data);
                    }

                    // restore fighter variables
                    memcpy(&fighter_data->fighter_var, &ft_data->fighter_var, sizeof(fighter_data->fighter_var)); // copy hitbox

                    // zero pointer to cached animations to force anim load (fixes fall crash)
                    fighter_data->anim_curr_ARAM = 0;
                    fighter_data->anim_persist_ARAM = 0;

                    // enter backed up state
                    GOBJ *anim_source = 0;
                    if (fighter_data->flags.is_robj_child == 1)
                        anim_source = fighter_data->grab.attacker;
                    Fighter_SetAllHurtboxesNotUpdated(fighter);
                    ActionStateChange(ft_data->state_frame, ft_data->state_rate, -1, fighter, ft_data->state_id, 0, anim_source);
                    fighter_data->state.blend = 0;

                    // copy physics again to work around some bugs. Notably, this fixes savestates during dash.
                    memcpy(&fighter_data->phys, &ft_data->phys, sizeof(fighter_data->phys));
                    if (flags & Savestate_Mirror)
                    {
                        fighter_data->phys.anim_vel.X *= -1;
                        fighter_data->phys.self_vel.X *= -1;
                        fighter_data->phys.kb_vel.X *= -1;
                        fighter_data->phys.atk_shield_kb_vel.X *= -1;
                        fighter_data->phys.pos.X *= -1;
                        fighter_data->phys.pos_prev.X *= -1;
                        fighter_data->phys.pos_delta.X *= -1;
                        fighter_data->phys.horzitonal_velocity_queue_will_be_added_to_0xec *= -1;
                        fighter_data->phys.self_vel_ground.X *= -1;
                        fighter_data->phys.nudge_vel.X *= -1;
                    }

                    // restore state variables
                    memcpy(&fighter_data->state_var, &ft_data->state_var, sizeof(fighter_data->state_var)); // copy hitbox

                    // restore ftcmd variables
                    memcpy(&fighter_data->ftcmd_var, &ft_data->ftcmd_var, sizeof(fighter_data->ftcmd_var)); // copy hitbox

                    // restore damage variables
                    memcpy(&fighter_data->dmg, &ft_data->dmg, sizeof(ft_data->dmg)); // copy hitbox
                    fighter_data->dmg.hit_log.source = IDToGOBJ(fighter_data->dmg.hit_log.source);

                    // restore jump variables
                    memcpy(&fighter_data->jump, &ft_data->jump, sizeof(fighter_data->jump)); // copy hitbox

                    // restore flags
                    memcpy(&fighter_data->flags, &ft_data->flags, sizeof(fighter_data->flags)); // copy hitbox

                    // restore hurt variables
                    memcpy(&fighter_data->hurt, &ft_data->hurt, sizeof(fighter_data->hurt)); // copy hurtbox

                    // update jobj position
                    JOBJ *fighter_jobj = fighter->hsd_object;
                    fighter_jobj->trans = fighter_data->phys.pos;
                    // dirtysub their jobj
                    JOBJ_SetMtxDirtySub(fighter_jobj);

                    // update hurtbox position
                    Fighter_UpdateHurtboxes(fighter_data);

                    // remove color overlay
                    Fighter_ColAnim_Remove(fighter_data, 9);

                    // restore color
                    memcpy(fighter_data->color, ft_data->color, sizeof(fighter_data->color));

                    // restore smash variables
                    memcpy(&fighter_data->smash, &ft_data->smash, sizeof(fighter_data->smash)); // copy hitbox

                    // restore shield/reflect/absorb variables
                    memcpy(&fighter_data->shield, &ft_data->shield, sizeof(fighter_data->shield));                         // copy hitbox
                    memcpy(&fighter_data->shield_bubble, &ft_data->shield_bubble, sizeof(fighter_data->shield_bubble));    // copy hitbox
                    memcpy(&fighter_data->reflect_bubble, &ft_data->reflect_bubble, sizeof(fighter_data->reflect_bubble)); // copy hitbox
                    memcpy(&fighter_data->absorb_bubble, &ft_data->absorb_bubble, sizeof(fighter_data->absorb_bubble));    // copy hitbox
                    memcpy(&fighter_data->reflect_hit, &ft_data->reflect_hit, sizeof(fighter_data->reflect_hit));          // copy hitbox
                    memcpy(&fighter_data->absorb_hit, &ft_data->absorb_hit, sizeof(fighter_data->absorb_hit));             // copy hitbox

                    // restore callback functions
                    memcpy(&fighter_data->cb, &ft_data->cb, sizeof(fighter_data->cb)); // copy hitbox

                    // stop player SFX
                    SFX_StopAllFighterSFX(fighter_data);

                    // update colltest frame
                    fighter_data->coll_data.coll_test = *stc_colltest;

                    // restore camera subject
                    CmSubject *thiscam = fighter_data->camera_subject;
                    CmSubject *savedcam = (CmSubject *)&ft_data->camera_subject;
                    void *alloc = thiscam->alloc;
                    CmSubject *next = thiscam->next;
                    memcpy(thiscam, savedcam, sizeof(ft_data->camera_subject));
                    if (flags & Savestate_Mirror)
                    {
                        // These adjustments of mirroring camera are not perfect for now. Please fix this if you know suitable adjustments
                        thiscam->cam_pos.X *= -1;
                        thiscam->bone_pos.X *= -1;
                        thiscam->direction *= -1;
                    }
                    thiscam->alloc = alloc;
                    thiscam->next = next;

                    // update their IK
                    Fighter_UpdateIK(fighter);

                    // if shield is up, update shield
                    if ((fighter_data->state_id >= ASID_GUARDON) && (fighter_data->state_id <= ASID_GUARDREFLECT))
                    {
                        fighter_data->shield_bubble.bone = 0;
                        fighter_data->reflect_bubble.bone = 0;
                        fighter_data->absorb_bubble.bone = 0;

                        GuardOnInitIDK(fighter);
                        Animation_GuardAgain(fighter);
                    }

                    // process dynamics

                    int dyn_proc_num = 45;

                    // simulate dynamics a bunch to fall in place
                    for (int d = 0; d < dyn_proc_num; d++)
                    {
                        Fighter_ProcDynamics(fighter);
                    }

                    // remove all items belonging to this fighter
                    GOBJ *item = (*stc_gobj_lookup)[MATCHPLINK_ITEM];
                    while (item != 0)
                    {
                        // get next
                        GOBJ *next_item = item->next;

                        // check to delete
                        ItemData *item_data = item->userdata;
                        if (fighter == item_data->fighter_gobj)
                        {
                            // destroy it
                            Item_Destroy(item);
                        }

                        item = next_item;
                    }
                }
            }

            // If subchar exists, reset its leader log, tracked position, and facing direction. 
            // This is to fix a bug where the last 30 frames before save state reset would get carried into the subcharacter's
            // queue after loading a savestate. A more ideal fix would be to save and restore the cpu leader log, but implementing
            // that will have to wait until the upcomming save state refactor is done. For now this solves most problems by clearing
            // out the queue on reset so the subcharacter starts with a blank cpu log, which is what you want in most situations. 
            if (queue[1].fighter != 0 && savestate->ft_state[i].data[1].is_exist == 1){

                FtSaveStateData_v1 *primarychar_data = &ft_state->data[0];
                FighterData *subchar_fighter_data = queue[1].fighter_data;

                for (u32 j = 0; j < countof(subchar_fighter_data->cpu.leader_log); j++) 
                {
                    // set the pos target to the primary character's position to avoid a force target at 0,0,0
                    // set the facing direction to the primary character's facing direction to avoid the facing direction being incorrect on the first frame
                    subchar_fighter_data->cpu.leader_log[j] = (struct CPULeaderLog){
                        .pos = primarychar_data->phys.pos,
                        .facing_direction = primarychar_data->facing_direction
                    };
                }
            }

            // check to recreate HUD
            MatchHUDElement *hud = &stc_matchhud->element_data[i];

            // check if fighter is perm dead
            if (Match_CheckIfStock() == 1)
            {
                // remove HUD if no stocks left
                if (Fighter_GetStocks(i) <= 0)
                {
                    hud->is_removed = 0;
                }
            }

            // check to create it
            if (hud->is_removed == 1)
            {
                Match_CreateHUD(i);
            }

            // snap camera to the new positions
            Match_CorrectCamera();

            // stop crowd cheer
            SFX_StopCrowd();
        }
    }
    
    if (isLoaded == 1) {
        // restore frame
        Match *match = stc_match;
        match->time_frames = savestate->frame;
        event_vars->game_timer = savestate->frame;

        // update timer
        int frames = match->time_frames - 1; // this is because the scenethink function runs once before the gobj procs do
        match->time_seconds = frames / 60;
        match->time_ms = frames % 60;

        // restore event data
        memcpy(event_vars->event_gobj->userdata, &savestate->event_data, sizeof(savestate->event_data));

        // remove all particles
        for (int i = 0; i < PTCL_LINKMAX; i++)
        {
            Particle **ptcls = &stc_ptcl[i];
            Particle *ptcl = *ptcls;
            while (ptcl != 0)
            {

                Particle *ptcl_next = ptcl->next;

                // begin destroying this particle

                // subtract some value, 8039c9f0
                if (ptcl->gen != 0)
                {
                    ptcl->gen->particle_num--;
                }
                // remove from generator? 8039ca14
                if (ptcl->gen != 0)
                    psRemoveParticleAppSRT(ptcl);

                // delete parent jobj, 8039ca48
                psDeletePntJObjwithParticle(ptcl);

                // update most recent ptcl pointer
                *ptcls = ptcl->next;

                // free alloc, 8039ca54
                HSD_ObjFree((void *)0x804d0f60, ptcl);

                // decrement ptcl total
                u16 ptclnum = *stc_ptclnum;
                ptclnum--;
                *stc_ptclnum = ptclnum;

                // get next
                ptcl = ptcl_next;
            }
        }

        // remove all camera shake gobjs (p_link 18, entity_class 3)
        GOBJ *gobj = (*stc_gobj_lookup)[MATCHPLINK_MATCHCAM];
        while (gobj != 0)
        {

            GOBJ *gobj_next = gobj->next;

            // if entity class 3 (quake)
            if (gobj->entity_class == 3)
            {
                GObj_Destroy(gobj);
            }

            gobj = gobj_next;
        }
    }
    
    // sfx
    if ((flags & Savestate_Silent) == 0) {
        if (isLoaded == 0) {
            SFX_PlayCommon(3);
        } else {
            SFX_PlayCommon(0);
        }
    }

    return isLoaded;
}
