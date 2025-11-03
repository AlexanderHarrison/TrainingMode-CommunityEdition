#include "events.h"

union ID {
    void *ptr;
    struct IDInfo {
        // for locating gobj
        char p_link;
        char list_idx;
        
        // for locating jobj (unused for FighterData and GOBJ)
        char jobj_idx;
    } id;
};

static bool ItemAttachBone2(ItemSaveState_v2 *st, JOBJ *item_jobj) {
    if (item_jobj == 0)
        return false;
    
    // OSReport("it checking %i!\n", *bone);
    ROBJ *robj = item_jobj->robj;
    if (
        robj
        && (robj->flags & ROBJ_HAS_TYPE)
        && (robj->flags & ROBJ_TYPE_MASK) == ROBJ_JOBJ
    ) {
        st->attached = item_jobj;
        st->attached_to = robj->u.jobj;
        OSReport("save: attached %p->%p!\n", st->attached, st->attached_to);
        return true;
    }
    
    if (ItemAttachBone2(st, item_jobj->child))
        return true;
    return ItemAttachBone2(st, item_jobj->sibling);
}

static JOBJ *ItemAttachBone(char *bone, JOBJ *item_jobj) {
    if (item_jobj == 0)
        return 0;
    
    OSReport("it checking %i!\n", *bone);
    ROBJ *robj = item_jobj->robj;
    if (
        robj
        && (robj->flags & ROBJ_HAS_TYPE)
        && (robj->flags & ROBJ_TYPE_MASK) == ROBJ_JOBJ
    ) {
        OSReport("it attached %i!\n", *bone);
        return robj->u.jobj;
    }
    
    (*bone)++;
    JOBJ *child_jobj = ItemAttachBone(bone, item_jobj->child);
    if (child_jobj)
        return child_jobj;
    return ItemAttachBone(bone, item_jobj->sibling);
}

// static JOBJ *FighterAttachBone(char *bone, JOBJ *item_jobj, JOBJ *fighter_jobj) {
//     if (fighter_jobj == 0)
//         return false;
    
//     OSReport("FT checking %i!\n", *bone);
//     ROBJ *robj = fighter_jobj->robj;
//     if (
//         robj
//         && (robj->flags & ROBJ_HAS_TYPE)
//         && (robj->flags & ROBJ_TYPE_MASK) == ROBJ_JOBJ
//         // && robj->u.jobj == item_jobj
//     ) {
//         OSReport("FT attached %i!\n", *bone);
//         return fighter_jobj;
//     }
    
//     (*bone)++;
//     JOBJ *child_jobj = FighterAttachBone(bone, item_jobj, fighter_jobj->child);
//     if (child_jobj)
//         return child_jobj;
//     return FighterAttachBone(bone, item_jobj, fighter_jobj->sibling);
// }

static GOBJ *GOBJToID(GOBJ *gobj) {
    union ID pun = { .ptr = 0 };
    
    if (gobj) {
        short entity_class = gobj->entity_class;
        char p_link = gobj->p_link;
        if (
            entity_class == HSD_GOBJ_CLASS_FIGHTER
            || entity_class == HSD_GOBJ_CLASS_ITEM
            || entity_class == HSD_GOBJ_CLASS_ITEMLINK
            || entity_class == HSD_GOBJ_CLASS_STAGE
        ) {
            char list_idx = 0;
            for (GOBJ *item = (*stc_gobj_lookup)[p_link]; item; item = item->next) {
                if (item == gobj) {
                    pun.id = (struct IDInfo) { p_link, list_idx, 0 };
                    break;
                }
                list_idx++;
            }
        }
    }
    
    return pun.ptr;
}

static GOBJ *IDToGOBJ(GOBJ *gobj) {
    if (gobj) {
        union ID pun = { .ptr = gobj };
        char p_link = pun.id.p_link;
        char list_idx = pun.id.list_idx;
        
        for (GOBJ *item = (*stc_gobj_lookup)[p_link]; item; item = item->next) {
            if (list_idx == 0)
                return item;
            list_idx--;
        }
    }
    
    return 0;
}

static FighterData *FtDataToID(FighterData *ft_data) {
    if (ft_data)
        return (FighterData *)GOBJToID(ft_data->fighter);
    return 0;
}

static FighterData *IDToFtData(FighterData *ft_data) {
    if (ft_data)
        return IDToGOBJ((GOBJ *)ft_data)->userdata;
    return 0;
}

static bool JOBJFindBoneIdx(JOBJ *bone, JOBJ *target, char *bone_idx) {
    if (bone == 0)
        return false;
    if (bone == target)
        return true;
    (*bone_idx) += 1;
    
    if (JOBJFindBoneIdx(bone->child, target, bone_idx))
        return true;
    return JOBJFindBoneIdx(bone->sibling, target, bone_idx);
}

static JOBJ *JOBJFindBonePtr(JOBJ *bone, char *bone_idx) {
    if (bone == 0)
        return 0;
    if (*bone_idx == 0)
        return bone;
    (*bone_idx)--;
    
    JOBJ *target = JOBJFindBonePtr(bone->child, bone_idx);
    if (target)
        return target;
    return JOBJFindBonePtr(bone->sibling, bone_idx);
}

static JOBJ *JOBJToID(GOBJ *gobj, JOBJ *jobj) {
    if (jobj) {
        union ID pun = { .id = { 1, 0, 0 } }; // set as non-zero to prevent returning 0 if jobj_idx == 0
        JOBJFindBoneIdx(gobj->hsd_object, jobj, &pun.id.jobj_idx);
        return pun.ptr;
    }
    return 0;
}

static JOBJ *IDToJOBJ(GOBJ *gobj, JOBJ *jobj) {
    if (jobj) {
        union ID pun = { .ptr = jobj };
        return JOBJFindBonePtr(gobj->hsd_object, &pun.id.jobj_idx);
    }
    return 0;
}

// use enum savestate_flags for flags
int Savestate_Save_v2(Savestate_v2 *savestate, int flags)
{
    // ensure no players are in problematic states
    if (flags & Savestate_Checks) {
        GOBJ **gobj_list = R13_PTR(-0x3E74);
        GOBJ *fighter = gobj_list[8];
        while (fighter != 0)
        {
            FighterData *fighter_data = fighter->userdata;
            
            // TODO remove
            if ((fighter_data->cb.OnDeath_Persist != 0) ||
                (fighter_data->cb.OnDeath_State != 0) ||
                (fighter_data->cb.OnDeath3 != 0) ||
                (fighter_data->item.held != 0) ||
                (fighter_data->item.held_2 != 0) ||
                (fighter_data->accessory != 0) ||
                ((fighter_data->kind == FTKIND_NESS) && ((fighter_data->state_id >= 342) && (fighter_data->state_id <= 344)))) // hardcode ness' usmash because it doesnt destroy the yoyo via onhit callback...
            {
                // cannot save
                if ((flags & Savestate_Silent) == 0)
                    SFX_PlayCommon(3);
                return false;
            }
    
            fighter = fighter->next;
        }
    }

    savestate->is_exist = 1;

    // save frame
    savestate->frame = event_vars->game_timer;

    // save event data
    memcpy(&savestate->event_data, event_vars->event_gobj->userdata, sizeof(savestate->event_data));

    // backup all players
    for (int i = 0; i < 6; i++)
    {
        FtSaveState_v2 *ft_state = &savestate->ft_state[i];

        Playerblock *playerblock = Fighter_GetPlayerblock(i);
        memcpy(&ft_state->playerblock, playerblock, sizeof(ft_state->playerblock));
        
        int *stale_queue = Fighter_GetStaleMoveTable(i);
        memcpy(&ft_state->stale_queue, stale_queue, sizeof(ft_state->stale_queue));

        // backup each subfighters data
        for (int j = 0; j < 2; j++)
        {
            FtSaveStateData_v2 *ft_data = &ft_state->data[j];
            GOBJ *fighter = Fighter_GetSubcharGObj(i, j);
        
            // if exists
            if (fighter == 0) {
                ft_data->is_exist = 0;
                continue;
            }
            
            FighterData *fighter_data = fighter->userdata;

            // copy to ft_state
            ft_data->is_exist = 1;
            ft_data->state_id = fighter_data->state_id;
            ft_data->facing_direction = fighter_data->facing_direction;
            ft_data->state_frame = fighter_data->state.frame;
            ft_data->state_rate = fighter_data->state.rate;
            ft_data->state_blend = fighter_data->state.blend;
            memcpy(&ft_data->phys, &fighter_data->phys, sizeof(fighter_data->phys));
            memcpy(&ft_data->color, &fighter_data->color, sizeof(fighter_data->color));
            memcpy(&ft_data->input, &fighter_data->input, sizeof(fighter_data->input));
            memcpy(&ft_data->coll_data, &fighter_data->coll_data, sizeof(fighter_data->coll_data));
            memcpy(&ft_data->camera_subject, fighter_data->camera_subject, sizeof(*fighter_data->camera_subject));
            memcpy(&ft_data->flags, &fighter_data->flags, sizeof(fighter_data->flags));
            memcpy(&ft_data->fighter_var, &fighter_data->fighter_var, sizeof(fighter_data->fighter_var));
            memcpy(&ft_data->state_var, &fighter_data->state_var, sizeof(fighter_data->state_var));
            memcpy(&ft_data->ftcmd_var, &fighter_data->ftcmd_var, sizeof(fighter_data->ftcmd_var));
            memcpy(&ft_data->jump, &fighter_data->jump, sizeof(fighter_data->jump));
            memcpy(&ft_data->smash, &fighter_data->smash, sizeof(fighter_data->smash));
            memcpy(&ft_data->hurt, &fighter_data->hurt, sizeof(fighter_data->hurt));
            memcpy(&ft_data->shield, &fighter_data->shield, sizeof(fighter_data->shield));
            memcpy(&ft_data->shield_bubble, &fighter_data->shield_bubble, sizeof(fighter_data->shield_bubble));
            memcpy(&ft_data->reflect_bubble, &fighter_data->reflect_bubble, sizeof(fighter_data->reflect_bubble));
            memcpy(&ft_data->absorb_bubble, &fighter_data->absorb_bubble, sizeof(fighter_data->absorb_bubble));
            memcpy(&ft_data->reflect_hit, &fighter_data->reflect_hit, sizeof(fighter_data->reflect_hit));
            memcpy(&ft_data->absorb_hit, &fighter_data->absorb_hit, sizeof(fighter_data->absorb_hit));
            memcpy(&ft_data->cpu, &fighter_data->cpu, sizeof(fighter_data->cpu)); // TODO check ptrs
            memcpy(&ft_data->wall, &fighter_data->wall, sizeof(fighter_data->wall));
            ft_data->jab2_timer = fighter_data->jab2_timer;
            
            // TODO check if we can id *_bubble struct bones
            
            // copy items
            ft_data->item = (struct item) {
                fighter_data->item.x1970,
                GOBJToID(fighter_data->item.held),
                GOBJToID(fighter_data->item.held_2),
                GOBJToID(fighter_data->item.held_3),
                GOBJToID(fighter_data->item.head),
                GOBJToID(fighter_data->item.held_special)
            };

            // copy dmg
            memcpy(&ft_data->dmg, &fighter_data->dmg, sizeof(fighter_data->dmg));
            ft_data->dmg.hit_log.source = GOBJToID(ft_data->dmg.hit_log.source);

            // copy grab
            memcpy(&ft_data->grab, &fighter_data->grab, sizeof(fighter_data->grab));
            ft_data->grab.attacker = GOBJToID(ft_data->grab.attacker);
            ft_data->grab.victim = GOBJToID(ft_data->grab.victim);

            // copy callbacks
            memcpy(&ft_data->cb, &fighter_data->cb, sizeof(fighter_data->cb));

            // copy regular hitboxes
            memcpy(&ft_data->hitbox, &fighter_data->hitbox, sizeof(fighter_data->hitbox));
            for (u32 k = 0; k < countof(fighter_data->hitbox); k++)
            {
                ft_data->hitbox[k].bone = JOBJToID(fighter, ft_data->hitbox[k].bone);
                for (u32 l = 0; l < countof(fighter_data->hitbox->victims); l++)
                    ft_data->hitbox[k].victims[l].data = FtDataToID(ft_data->hitbox[k].victims[l].data);
            }
            
            // copy throw hitboxes
            memcpy(&ft_data->throw_hitbox, &fighter_data->throw_hitbox, sizeof(fighter_data->throw_hitbox));
            for (u32 k = 0; k < countof(fighter_data->throw_hitbox); k++)
            {
                ft_data->throw_hitbox[k].bone = JOBJToID(fighter, ft_data->throw_hitbox[k].bone);
                for (u32 l = 0; l < countof(fighter_data->throw_hitbox->victims); l++)
                    ft_data->throw_hitbox[k].victims[l].data = FtDataToID(ft_data->throw_hitbox[k].victims[l].data);
            }

            // copy thrown hitboxes
            memcpy(&ft_data->thrown_hitbox, &fighter_data->thrown_hitbox, sizeof(fighter_data->thrown_hitbox));
            ft_data->thrown_hitbox.bone = JOBJToID(fighter, ft_data->thrown_hitbox.bone);
            for (u32 k = 0; k < countof(fighter_data->thrown_hitbox.victims); k++)
                ft_data->thrown_hitbox.victims[k].data = FtDataToID(ft_data->thrown_hitbox.victims[k].data);
        }
    }
    
    // store items
    memset(savestate->item_state, 0, sizeof(savestate->item_state));
    int item_count = 0;
    for (GOBJ *item = (*stc_gobj_lookup)[MATCHPLINK_ITEM]; item; item = item->next) {
        if (item_count == countof(savestate->item_state)) {
            OSReport("too many items!\n");
            break;
        }

        ItemSaveState_v2 *item_state = &savestate->item_state[item_count++];
        
        item_state->is_exist = 1;
        item_state->gobj = (struct itgobjinfo) {
            .entity_class = item->entity_class,
            .p_link = item->p_link,
            .gx_link = item->gx_link,
            .p_priority = item->p_priority,
            .gx_pri = item->gx_pri,
            .obj_kind = item->obj_kind,
            .data_kind = item->data_kind,
            .gx_cb = item->gx_cb,
            .cobj_links = item->cobj_links,
            .destructor_function = item->destructor_function,
        };
        
        // int proc_count = 0;
        // for (GOBJProc *proc = item->proc; proc; proc = proc->next) {
        //     item_state->gobj.proc[proc_count] = proc->cb;
        //     item_state->gobj.proc_s_link[proc_count] = proc->s_link;
        //     item_state->gobj.proc_flags[proc_count] = proc->flags;
        //     proc_count++;
            
        //     if (proc_count == countof(item_state->gobj.proc)) {
        //         OSReport("too many item procs!\n");
        //         break;
        //     }
        // }
        
        // TODO: do something smarter
        int proc_count = 0;
        GOBJProc **procs = *(GOBJProc ***)0x804D7844;
        for (int i = 0; i < 1600; ++i) {
            GOBJProc *proc = procs[i];
            if (proc == 0 || proc->parentGOBJ != item) continue;
             
            item_state->gobj.proc[proc_count] = proc->cb;
            item_state->gobj.proc_s_link[proc_count] = proc->s_link;
            item_state->gobj.proc_flags[proc_count] = proc->flags;
            proc_count++;
            
            if (proc_count == countof(item_state->gobj.proc)) {
                OSReport("too many item procs!\n");
                break;
            }
        }
        ItemData *item_data = item->userdata;

        item_state->attached = 0;
        item_state->attached_to = 0;
        ItemAttachBone2(item_state, item->hsd_object);
        item_state->attached = JOBJToID(item, item_state->attached);
        item_state->attached_to = JOBJToID(item_data->fighter_gobj, item_state->attached_to);

        memcpy(&item_state->data, item_data, sizeof(ItemData));
        
        item_state->data.item = GOBJToID(item_data->item);
        item_state->data.dmg.xc90 = GOBJToID(item_data->dmg.xc90);
        item_state->data.dmg.source_fighter = GOBJToID(item_data->dmg.source_fighter);
        item_state->data.dmg.source_item = GOBJToID(item_data->dmg.source_item);
        item_state->data.dmg.reflect = GOBJToID(item_data->dmg.reflect);
        item_state->data.hit_fighter = GOBJToID(item_data->hit_fighter);
        item_state->data.detected_fighter = GOBJToID(item_data->detected_fighter);
        item_state->data.unk_fighter = GOBJToID(item_data->unk_fighter);
        item_state->data.grabbed_fighter = GOBJToID(item_data->grabbed_fighter);
        item_state->data.attacker_item = GOBJToID(item_data->attacker_item);
        item_state->data.fighter_gobj = GOBJToID(item_data->fighter_gobj);
        item_state->data.coll_data.gobj = GOBJToID(item_data->coll_data.gobj);
        
        for (int i = 0; i < 4; ++i)
            item_state->data.hitbox[i].bone = JOBJToID(item, item_data->hitbox[i].bone);

        for (int i = 0; i < 2; ++i)
            item_state->data.it_hurt[i].jobj = JOBJToID(item, item_data->it_hurt[i].jobj);

        // TODO find pointers in item_data->item_var
    }

    if ((flags & Savestate_Silent) == 0) {
        SFX_PlayCommon(1);
    
        // if not in frame advance, flash screen. I wrote it like this because the second condition kept getting optimized out
        if (Pause_CheckStatus(0) != 1 && Pause_CheckStatus(1) != 2)
            ScreenFlash_Create(2, 0);
    }

    return true;
}
// use enum savestate_flags for flags
int Savestate_Load_v2(Savestate_v2 *savestate, int flags)
{
    if (!savestate->is_exist) {
        if ((flags & Savestate_Silent) == 0)
            SFX_PlayCommon(3);
        return false;
    }
    
    // remove all existing items
    for (GOBJ *item = (*stc_gobj_lookup)[MATCHPLINK_ITEM]; item != 0; item = item->next)
        Item_Destroy(item);
    
    // spawn new items
    // We do this before loading fighters so we can restore item pointers
    for (u32 i = 0; i < countof(savestate->item_state); ++i) {
        ItemSaveState_v2 *item_state = &savestate->item_state[i];
        if (item_state->is_exist) {
            // create item
            // adapted from Item_8026862C in decomp (Item_CreateItem in mex-land)
            struct itgobjinfo *gobj = &item_state->gobj;
            GOBJ *item = GObj_Create(gobj->entity_class, gobj->p_link, gobj->p_priority);
            GObj_AddGXLink(item, gobj->gx_cb, gobj->gx_link, gobj->gx_pri);
            item->cobj_links = gobj->cobj_links;

            ItemData *item_data = HSD_ObjAlloc(stc_item_alloc_data);
            memcpy(item_data, &item_state->data, sizeof(ItemData));

            // restore gobj pointers

            item_data->item = IDToGOBJ(item_state->data.item);
            item_data->dmg.xc90 = IDToGOBJ(item_state->data.dmg.xc90);
            item_data->dmg.source_fighter = IDToGOBJ(item_state->data.dmg.source_fighter);
            item_data->dmg.source_item = IDToGOBJ(item_state->data.dmg.source_item);
            item_data->dmg.reflect = IDToGOBJ(item_state->data.dmg.reflect);
            item_data->hit_fighter = IDToGOBJ(item_state->data.hit_fighter);
            item_data->detected_fighter = IDToGOBJ(item_state->data.detected_fighter);
            item_data->unk_fighter = IDToGOBJ(item_state->data.unk_fighter);
            item_data->grabbed_fighter = IDToGOBJ(item_state->data.grabbed_fighter);
            item_data->attacker_item = IDToGOBJ(item_state->data.attacker_item);
            item_data->fighter_gobj = IDToGOBJ(item_state->data.fighter_gobj);
            item_data->coll_data.gobj = IDToGOBJ(item_state->data.coll_data.gobj);
            
            for (int i = 0; i < 12; ++i) {
                void *fn = gobj->proc[i];
                if (fn == 0) break;
                OSReport("add proc %p\n", fn);
                GOBJProc *proc = GObj_AddProc(item, fn, gobj->proc_s_link[i]);
                proc->flags = gobj->proc_flags[i];
            }

            GObj_AddUserData(item, gobj->data_kind, gobj->destructor_function, item_data);
            Item_InitGObjObject(item);
            
            // restore jobj pointers

            for (int i = 0; i < 4; ++i) {
                item_data->hitbox[i].bone = IDToJOBJ(item, item_state->data.hitbox[i].bone);
                OSReport("load from %p to %p\n", item_state->data.hitbox[i].bone, item_data->hitbox[i].bone);
            }
                
            for (int i = 0; i < 2; ++i)
                item_data->it_hurt[i].jobj = IDToJOBJ(item, item_state->data.it_hurt[i].jobj);
            
            if (item_data->fighter_gobj) {
                FighterData *ft_data = item_data->fighter_gobj->userdata;
                Fighter_IncrementReferenceCount(ft_data->kind);
            }
            
            JOBJ *attached = IDToJOBJ(item, item_state->attached);
            JOBJ *attached_to = IDToJOBJ(item_data->fighter_gobj, item_state->attached_to);
            if (attached)
                JOBJ_AttachPositionRotation(attached, attached_to);
        } else {
            break;
        }
    }

    // loop through all players
    for (int i = 0; i < 6; i++)
    {
        // if the main fighter and backup exists
        FtSaveState_v2 *ft_state = &savestate->ft_state[i];

        // restore playerblock
        Playerblock *playerblock = Fighter_GetPlayerblock(i);
        // TODO convert to ptr/id tform
        GOBJ *g0 = playerblock->gobj[0];
        GOBJ *g1 = playerblock->gobj[1];
        memcpy(playerblock, &ft_state->playerblock, sizeof(ft_state->playerblock));
        playerblock->gobj[0] = g0;
        playerblock->gobj[1] = g1;

        // restore stale moves
        int *stale_queue = Fighter_GetStaleMoveTable(i);
        memcpy(stale_queue, &ft_state->stale_queue, sizeof(ft_state->stale_queue));

        // restore fighter data
        for (int j = 0; j < 2; j++)
        {
            FtSaveStateData_v2 *ft_data = &ft_state->data[j];
            if (ft_data->is_exist)
            {
                // GOBJ *fighter = Fighter_GetSubcharGObj(i, j);
                // FighterData *fighter_data = fighter->userdata;
                // memcpy(fighter_data, &ft_data->ft_data, sizeof(FighterData));
            
                // fighter_data->anim_curr_ARAM = 0;
                // fighter_data->anim_persist_ARAM = 0;
                
                // get state
                GOBJ *fighter = Fighter_GetSubcharGObj(i, j);
                FighterData *fighter_data = fighter->userdata;

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

                // restore grab
                memcpy(&fighter_data->grab, &ft_data->grab, sizeof(fighter_data->grab));
                fighter_data->grab.attacker = IDToGOBJ(fighter_data->grab.attacker);
                fighter_data->grab.victim = IDToGOBJ(fighter_data->grab.victim);
                
                // restore cpu
                memcpy(&fighter_data->cpu, &ft_data->cpu, sizeof(fighter_data->cpu)); // TODO check ptrs

                memcpy(&fighter_data->wall, &ft_data->wall, sizeof(fighter_data->wall));
                fighter_data->jab2_timer = ft_data->jab2_timer;
                
                // TODO check if we can id *_bubble struct bones
                
                // restore items
                // OSReport("Loading ID %p -> %p\n", ft_data->item.held, IDToGOBJ(ft_data->item.held));
                // OSReport("Loading ID %p -> %p\n", ft_data->item.held_2, IDToGOBJ(ft_data->item.held_2));
                // OSReport("Loading ID %p -> %p\n", ft_data->item.held_3, IDToGOBJ(ft_data->item.held_3));
                // OSReport("Loading ID %p -> %p\n", ft_data->item.head, IDToGOBJ(ft_data->item.head));
                // OSReport("Loading ID %p -> %p\n", ft_data->item.held_special, IDToGOBJ(ft_data->item.held_special));
                
                fighter_data->item = (struct item) {
                    ft_data->item.x1970,
                    IDToGOBJ(ft_data->item.held),
                    IDToGOBJ(ft_data->item.held_2),
                    IDToGOBJ(ft_data->item.held_3),
                    IDToGOBJ(ft_data->item.head),
                    IDToGOBJ(ft_data->item.held_special)
                };
                
                // convert pointers

                for (u32 k = 0; k < countof(fighter_data->hitbox); k++)
                {
                    fighter_data->hitbox[k].bone = IDToJOBJ(fighter, ft_data->hitbox[k].bone);
                    for (u32 l = 0; l < countof(fighter_data->hitbox->victims); l++) // pointers to hitbox victims
                    {
                        fighter_data->hitbox[k].victims[l].data = IDToFtData(ft_data->hitbox[k].victims[l].data);
                    }
                }
                for (u32 k = 0; k < countof(fighter_data->throw_hitbox); k++)
                {
                    fighter_data->throw_hitbox[k].bone = IDToJOBJ(fighter, ft_data->throw_hitbox[k].bone);
                    for (u32 l = 0; l < countof(fighter_data->throw_hitbox->victims); l++) // pointers to hitbox victims
                    {
                        fighter_data->throw_hitbox[k].victims[l].data = IDToFtData(ft_data->throw_hitbox[k].victims[l].data);
                    }
                }
                fighter_data->thrown_hitbox.bone = IDToJOBJ(fighter, ft_data->thrown_hitbox.bone);
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
                memcpy(&fighter_data->smash, &ft_data->smash, sizeof(fighter_data->smash));

                // restore shield/reflect/absorb variables
                memcpy(&fighter_data->shield, &ft_data->shield, sizeof(fighter_data->shield));
                memcpy(&fighter_data->shield_bubble, &ft_data->shield_bubble, sizeof(fighter_data->shield_bubble));
                memcpy(&fighter_data->reflect_bubble, &ft_data->reflect_bubble, sizeof(fighter_data->reflect_bubble));
                memcpy(&fighter_data->absorb_bubble, &ft_data->absorb_bubble, sizeof(fighter_data->absorb_bubble));
                memcpy(&fighter_data->reflect_hit, &ft_data->reflect_hit, sizeof(fighter_data->reflect_hit));
                memcpy(&fighter_data->absorb_hit, &ft_data->absorb_hit, sizeof(fighter_data->absorb_hit));

                // restore callback functions
                memcpy(&fighter_data->cb, &ft_data->cb, sizeof(fighter_data->cb));

                // stop player SFX
                SFX_StopAllFighterSFX(fighter_data);

                // update colltest frame
                fighter_data->coll_data.coll_test = *stc_colltest;

                // restore camera subject
                CmSubject *thiscam = fighter_data->camera_subject;
                CmSubject *savedcam = &ft_data->camera_subject;
                void *alloc = thiscam->alloc;
                CmSubject *next = thiscam->next;
                memcpy(thiscam, savedcam, sizeof(CmSubject));
    
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
            }
        }

        // check to recreate HUD
        MatchHUDElement *hud = &stc_matchhud->element_data[i];

        // check if fighter is perm dead
        if (Match_CheckIfStock() == 1 && Fighter_GetStocks(i) <= 0)
            hud->is_removed = 0;

        // check to create it
        if (hud->is_removed == 1)
            Match_CreateHUD(i);
    }

    // snap camera to the new positions
    Match_CorrectCamera();

    // stop crowd cheer
    SFX_StopCrowd();
    
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
    while (gobj) {
        GOBJ *gobj_next = gobj->next;

        // if entity class 3 (quake)
        if (gobj->entity_class == 3)
            GObj_Destroy(gobj);

        gobj = gobj_next;
    }
    
    if ((flags & Savestate_Silent) == 0)
        SFX_PlayCommon(0);

    return true;
}
