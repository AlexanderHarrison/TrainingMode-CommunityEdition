#include "../MexTK/mex.h"
#include "events.h"
#include "osds.h"

static GXColor stc_msg_colors[] = {
    {255, 255, 255, 255},
    {141, 255, 110, 255},
    {255, 162, 186, 255},
    {255, 240, 0, 255},
};

static void UpdateIASATracking(GOBJ *ft) {
    FighterData *ft_data = ft->userdata;
    if (ft_data->flags.past_iasa)
        ft_data->TM.iasa_frames++;
    else
        ft_data->TM.iasa_frames = 0;
}

static bool CheckIASA(FighterData *data) {
    // The spotdodge state is weird - its subaction list contains an IASA event which sets the IASA bit,
    // but the initial state entry function does not clear the flag.
    // This causes it to not function - the player can't cancel it,
    // but it causes us to think they can if it is performed after a state that clears the iasa bit (e.g. landing with an aerial).
    if (data->state_id == ASID_ESCAPE)
        return false;

    // For some reason, the iasa flag isn't reset when the state changes, only when a new action that has iasa starts.
    // So we need to do some bs tracking to figure out if the character is in iasa.
    return data->TM.iasa_frames && data->TM.iasa_frames <= data->TM.state_frame;
}

static void RunOSD_FrameAdvantage(GOBJ *ft, GOBJ *ft_sub) {
    /*
    Detect when your shield is hit. This is ez, look for GuardSetOff.
        If the attacker is actionable and the defender is not,
            the advantage is the # frames until shieldstun is gone.
        If the attacker is actionable and the defender is also actionable,
            the advantages is -frames since shieldstun.

    We use the 0 state to wait for GuardSetOff to start.
    We use the -1 state to wait for GuardSetOff to end.
    */

    FighterData *ft_data = ft->userdata;
    FighterData *ft_def_data = ft_data->TM.fighter_hurt_shield;
    if (ft_def_data == 0) return;
    GOBJ *ft_def = ft_def_data->fighter;

    int ply = ft_data->ply;
    static int atk_hit_state[6] = {0};

    // Wait for GuardSetOff to end before running frame advantage code again
    if (atk_hit_state[ply] == -1) {
        if (ft_def_data->state_id != ASID_GUARDSETOFF)
            atk_hit_state[ply] = 0;
        return;
    }

    // Detect when the defender's shield is hit and save the attacker's state
    if (atk_hit_state[ply] == 0) {
        if (ft_def_data->state_id == ASID_GUARDSETOFF)
            atk_hit_state[ply] = ft_data->state_id;
        return;
    }

    int atk_state = ft_data->state_id;

    // "actionable" if either state changed or in iasa, and not in aerial or ac landing lag.
    bool atk_actionable = (atk_hit_state[ply] != atk_state || CheckIASA(ft_data))
        && (atk_state != ASID_LANDING || ft_data->state.frame >= ft_data->attr.normal_landing_lag)
        && (atk_state < ASID_LANDINGAIRN || ASID_LANDINGAIRLW < atk_state);

    if (atk_actionable) {
        int advantage;

        if (ft_def_data->state_id == ASID_GUARDSETOFF) {
            // + on shield: advantage is frames until shieldstun is gone.
            float anim_speed = ft_def_data->state.rate;
            float stun_total = JOBJ_GetJointAnimFrameTotal(ft_def->hsd_object);
            float stun_curr = ft_def_data->state.frame;
            advantage = (int)((stun_total - stun_curr) / anim_speed) + 1;
        } else {
            // = or - on shield: advantage is -frames since shieldstun.
            advantage = -(int)ft_def_data->TM.state_frame;
            for (u32 i = 0; i < countof(ft_def_data->TM.state_prev); ++i) {
                if (ft_def_data->TM.state_prev[i] == ASID_GUARDSETOFF)
                    break;
                advantage -= (int)ft_def_data->TM.state_prev_frames[i];
            }
            advantage += 1; // off-by-one
        }

        Message_Display(
            OSD_FrameAdvantage, ft_data->ply,
            advantage > -7 ? MSGCOLOR_GREEN : MSGCOLOR_WHITE,
            "Frame Advantage\n%d Frames", advantage
        );

        // Wait until GuardSetOff finishes
        atk_hit_state[ply] = -1;
    }
}

typedef struct {
    int osd_start_frame;
    int first_grab_hitbox_frame;
    int enemy_release_frame;
    int thrower_prev_state;
} HandoffState;

static bool IsThrowState(const int state_id) {
    return state_id >= ASID_THROWF && state_id <= ASID_THROWLW;
}
static bool IsThrownState(const int state_id) {
    return (state_id >= ASID_THROWNF && state_id <= ASID_THROWNLWWOMEN) || (
               state_id >= ASID_THROWNFF && state_id <= ASID_THROWNFLW);
}

static bool IsGrabHitboxActive(const FighterData *ft_data) {
    if (ft_data->state_id == ASID_CATCH || ft_data->state_id == ASID_CATCHDASH) {
        for (int i = 0; i < 4; i++) {
            if (ft_data->hitbox[i].active) {
                return true;
            }
        }
    }
    return false;
}

static void RunOsd_Handoff(GOBJ *thrower, GOBJ *grabber, GOBJ *enemy, HandoffState *state) {
    if (!thrower || !grabber || !enemy) return;
    const FighterData *thrower_data = thrower->userdata;
    if (thrower_data->kind != FTKIND_POPO && thrower_data->kind != FTKIND_NANA) return;
    const FighterData *grabber_data = grabber->userdata;
    const FighterData *enemy_data = enemy->userdata;

    //time after the intiial throw this OSD times out
    const int timeout = 60;

    //tick if the thrower has started a throw recently (and not in the future for replay situations).
    const bool should_tick_handoff = state->osd_start_frame != 0 &&
                                         stc_match->time_frames - state->osd_start_frame < timeout &&
                                         state->osd_start_frame <= stc_match->time_frames;
    if (should_tick_handoff) {
        //this number of frames after the enemy is released from the throw, the handoff is considered invalid,
        //even if you successfully regrab. a successful regrab after this timeout is likely a chain grab or backtrack.
        const int post_release_timeout = 15;
        if (state->enemy_release_frame == 0 && IsThrownState(enemy_data->TM.state_prev[0]) && !IsThrownState(enemy_data->state_id)) {
            state->enemy_release_frame = stc_match->time_frames;
        }

        //grab hitbox is only two frames. first condition is to avoid false reporting the nana synced grab as a handoff attempt.
        if (stc_match->time_frames - state->osd_start_frame > 8 && IsGrabHitboxActive(grabber_data) && stc_match->time_frames - state->first_grab_hitbox_frame != 1) {
            state->first_grab_hitbox_frame = stc_match->time_frames;
        }

        if (grabber_data->state_id == ASID_CATCHPULL || grabber_data->state_id == ASID_CATCHDASHPULL) {
            //if you grabbed them without setting these vars it means you grabbed them on the exact frame of release
            state->enemy_release_frame = state->enemy_release_frame == 0 ? stc_match->time_frames : state->enemy_release_frame;
            state->first_grab_hitbox_frame = state->first_grab_hitbox_frame == 0 ? stc_match->time_frames : state->first_grab_hitbox_frame;
            int grab_to_throw_delta = state->first_grab_hitbox_frame - state->enemy_release_frame;
            bool grab_early = grab_to_throw_delta < 1;
            int color_timing = grab_to_throw_delta == -1 || grab_to_throw_delta == 0 ? MSGCOLOR_GREEN : MSGCOLOR_WHITE;
            GOBJ *msg_gobj = Message_Display(OSD_FighterSpecificTech, thrower_data->ply, MSGCOLOR_WHITE,
                                             "Handoff Success\n %dF %s", abs(grab_to_throw_delta),
                                             grab_early ? "early" : "late");

            MsgData *msg = msg_gobj->userdata;
            Text_SetColor(msg->text, 0, &stc_msg_colors[MSGCOLOR_GREEN]);
            Text_SetColor(msg->text, 1, &stc_msg_colors[color_timing]);
            state->osd_start_frame = 0;
        }
        else if (state->enemy_release_frame != 0 && state->first_grab_hitbox_frame != 0 && stc_match->time_frames - state->enemy_release_frame > post_release_timeout) {
            int grab_to_throw_delta = state->first_grab_hitbox_frame - state->enemy_release_frame;
            bool grab_early = grab_to_throw_delta < 1;
            //if the timing was 'perfect', yet you missed, give a custom error message to avoid confusion.
            //'Handoff failed, timing perfect' was confusing to people in testing.
            if (grab_to_throw_delta == -1 || grab_to_throw_delta == 0) {
                Message_Display(OSD_FighterSpecificTech, thrower_data->ply, MSGCOLOR_RED, "Handoff Failure\nConditions not Met");
            }
            else {
                Message_Display(OSD_FighterSpecificTech, thrower_data->ply, MSGCOLOR_RED, "Handoff Failure\n %dF %s",
                                abs(grab_to_throw_delta), grab_early ? "early" : "late");
            }
            state->osd_start_frame = 0;
        }
    } else if (!IsThrowState(state->thrower_prev_state) && IsThrowState(thrower_data->state_id)) {
        state->osd_start_frame = stc_match->time_frames;
    } else {
        state->osd_start_frame = 0;
        state->enemy_release_frame = 0;
        state->first_grab_hitbox_frame = 0;
    }
    state->thrower_prev_state = thrower_data->state_id;
}

static void RunOsd_PnJ(GOBJ *ft, GOBJ *ft_sub) {
    static int pivot_frames[6] = {};
    static int jump_frames[6] = {};
    static char smash_x_prev[6] = {};
    if (!ft || !ft_sub) return;
    const FighterData *ft_data = ft->userdata;
    const FighterData *ft_sub_data = ft_sub->userdata;
    //if nana is dead return
    if (ft_sub_data->flags.dead) return;
    if (ft_data->kind != FTKIND_POPO) return;


    bool pivot_was_active = pivot_frames[ft_data->ply] != 0;

    //SUCCESSFUL AND LATE PNJ DETECTION
    if (pivot_frames[ft_data->ply] != 0) {

        //if nana is disconnected, reset and return. avoids false positives where a disconnect nana
        //happens to jump on the same frame a pnj could occur
        //isCopy flag documented in fighter.h:1325
        const int CPU_FLAG_ISCOPY = 0x100;
        if (!(ft_sub_data->cpu.xf8 & CPU_FLAG_ISCOPY)) {
            pivot_frames[ft_data->ply] = 0;
            jump_frames[ft_data->ply] = 0;
            return;
        }
        const int late_pnj_window = 4;
        const int timeout = 6 + late_pnj_window;
        //timeout.
        if (stc_match->time_frames >  pivot_frames[ft_data->ply] + timeout) {
            pivot_frames[ft_data->ply] = 0;
        }
        //fail: popo jumped late.
        else if (ft_data->state_id == ASID_KNEEBEND && stc_match->time_frames - pivot_frames[ft_data->ply] <= late_pnj_window) {
            int delta = stc_match->time_frames - pivot_frames[ft_data->ply];
            Message_Display(OSD_FighterSpecificTech, ft_data->ply, MSGCOLOR_RED, "PNJ Fail. %dF Late", delta);
            pivot_frames[ft_data->ply] = 0;
        }
        //success: nana jumped 6f after the smash turn, and popo never jumped.
        else if (ft_sub_data->state_id == ASID_KNEEBEND && stc_match->time_frames - pivot_frames[ft_data->ply] == 6) {
            Message_Display(OSD_FighterSpecificTech, ft_data->ply, MSGCOLOR_GREEN, "PNJ Success");
            pivot_frames[ft_data->ply] = 0;
        }
    }
    //start watching for pnjs after a pivot out of a dash.
    else if (ft_data->TM.state_prev[0] == ASID_DASH && (ft_data->state_id == ASID_TURN || ft_data->state_id == ASID_TURNRUN)) {
        pivot_frames[ft_data->ply] = stc_match->time_frames;
    }

    // EARLY PNJ DETECTION
    char smash_x = ft_data->input.timer_lstick_smash_x;
    //smash turns are saved as a decaying byte. if this is < the current frame value, you smash turned this frame.
    bool smash_input_this_frame = smash_x > smash_x_prev[ft_data->ply];
    smash_x_prev[ft_data->ply] = smash_x;
    if (jump_frames[ft_data->ply] != 0) {
        const int early_pnj_window = 4;
        int frames_since_jump = stc_match->time_frames - jump_frames[ft_data->ply];
        //timer_lstick_smash_x reads 1f stale vs state_id, so the real gap is one less.
        int frames_early = frames_since_jump - 1;
        bool smash_turn_correct_direction = ft_data->input.lstick.X * ft_data->facing_direction < 0.0f; //to filter out fox trots.

        if (smash_input_this_frame && smash_turn_correct_direction && frames_early >= 1 && frames_early <= early_pnj_window) {
            Message_Display(OSD_FighterSpecificTech, ft_data->ply, MSGCOLOR_RED, "PNJ Fail. %dF Early", frames_early);
            jump_frames[ft_data->ply] = 0;
        }
        else if (frames_early > early_pnj_window) {
            jump_frames[ft_data->ply] = 0;
        }
    }
    // check if popo jumped out of a dash to start early pnj detection
    else if (!pivot_was_active && ft_data->TM.state_prev[0] == ASID_DASH && ft_data->state_id == ASID_KNEEBEND) {
        jump_frames[ft_data->ply] = stc_match->time_frames;
    }
}

void OSD_Think(GOBJ *event) {
    u32 osd_enabled = stc_memcard->TM_OSDEnabled;

    for (int ply = 0; ply < 6; ++ply) {
        GOBJ *ft = Fighter_GetSubcharGObj(ply, 0);
        if (!ft) continue;

        // subchar is usually null except for nana and the inactive sheik/zelda tform
        GOBJ *ft_sub = Fighter_GetSubcharGObj(ply, 1);

        if (ft) UpdateIASATracking(ft);
        if (ft_sub) UpdateIASATracking(ft_sub);


        if (osd_enabled & (1u << OSD_FrameAdvantage)) RunOSD_FrameAdvantage(ft, ft_sub);
        if (osd_enabled & (1u << OSD_FighterSpecificTech)) {
            // ICE CLIMBERS OSDS
            static HandoffState handoff_states[6][2];
            for (int enm = 0; enm < 6; ++enm) {
                if (enm == ply) continue;
                GOBJ *enm_ft = Fighter_GetSubcharGObj(enm, 0);
                RunOsd_Handoff(ft, ft_sub, enm_ft, &handoff_states[ply][0]);
                RunOsd_Handoff(ft_sub, ft, enm_ft, &handoff_states[ply][1]);
            }
            RunOsd_PnJ(ft, ft_sub);
            // ICE CLIMBERS OSDS END
        }
    }
}
