#include "xmdash.h"

static void Dash_ResetToStart(XmDashData *event_data, GOBJ *hmn);

static int canvas;
static Text *hud_time_text, *hud_best_text;

void Retry(GOBJ *menu) {
    stc_match->end_kind = MATCHENDKIND_RETRY;
    stc_match->state = 3;
    Match_EndVS();
}

void Exit(GOBJ *menu) {
    stc_match->state = 3;
    Match_EndVS();
}

void StartFreePractice(GOBJ *gobj) {
    XmDashData *event_data = event_vars->event_gobj->userdata;
    event_data->free_practice = 1;
    Options_Main[OPT_FREEPRACTICE].disable = 1; // one-way toggle; grey it out
    // drop any in-progress run
    event_data->started = 0;
    event_data->finished = 0;
    event_data->run_frames = 0;
    event_data->result_timer = 0;
}

void XmDash_OnChangeDistance(GOBJ *menu, int value) {
    XmDashData *event_data = event_vars->event_gobj->userdata;
    event_data->distance = XmDash_Dist[value];
    // Defer the marker move + player reset to the next think frame (after unpause)
    // so they happen together with the camera follow. Doing it here, in the paused
    // menu, moves the player/marker while the camera is frozen — leaving them
    // off-screen until unpause.
    event_data->reset_pending = 1;
}

void Event_Init(GOBJ *gobj) {
    XmDashData *event_data = gobj->userdata;
    event_data->assets = Archive_GetPublicAddress(event_vars->event_archive, "wavedash");
    event_data->core.timer = -1;
    event_data->core.result = -1;
    stc_match->end_kind = MATCHENDKIND_NONE;
    GObj_AddGXLink(gobj, HUD_GX, 5, 0);

    canvas = Text_CreateCanvas(2, 0, 0, 0, 0, GXLINK_HUD, 81, 19);
    hud_time_text = Text_CreateText(2, canvas);
    hud_time_text->kerning = 1; hud_time_text->align = 0; hud_time_text->use_aspect = 1;
    hud_time_text->trans.X = 50; hud_time_text->trans.Y = 25;
    hud_time_text->viewport_scale.X = 1; hud_time_text->viewport_scale.Y = 1;
    Text_AddSubtext(hud_time_text, 0, 0, "-");
    hud_best_text = Text_CreateText(2, canvas);
    hud_best_text->kerning = 1; hud_best_text->align = 0; hud_best_text->use_aspect = 1;
    hud_best_text->trans.X = 50; hud_best_text->trans.Y = 40;
    hud_best_text->scale.X = 0.35; hud_best_text->scale.Y = 0.35;
    hud_best_text->viewport_scale.X = 1; hud_best_text->viewport_scale.Y = 1;
    Text_AddSubtext(hud_best_text, 0, 0, "-");
    event_data->persist_best = Events_GetSavedScore(stc_memcard->EventBackup.event);
}

static void Finish_Spawn(XmDashData *event_data) {
    GOBJ *gobj = GObj_Create(10, 11, 0);
    JOBJ *jobj = JOBJ_LoadJoint(event_data->assets->target_jobj);
    GObj_AddObject(gobj, 3, jobj);
    GObj_AddGXLink(gobj, GXLink_Common, 5, 0);
    jobj->trans.X = event_data->start_x + event_data->distance;
    jobj->trans.Y = event_data->start_y; // track surface level (calibrated in T9)
    jobj->trans.Z = 0.f;
    JOBJ_SetMtxDirtySub(jobj);
    event_data->finish_gobj = gobj;
}

static void Dash_ResetToStart(XmDashData *event_data, GOBJ *hmn) {
    FighterData *hmn_data = hmn->userdata;
    hmn_data->phys.pos.X = event_data->start_x;
    hmn_data->phys.pos.Y = event_data->start_y;
    hmn_data->phys.self_vel.X = 0.f;
    hmn_data->phys.self_vel.Y = 0.f;
    hmn_data->phys.self_vel_ground.X = 0.f; // clear run/dash ground momentum (the slide)
    hmn_data->phys.self_vel_ground.Y = 0.f;
    Fighter_EnterWait(hmn); // interrupt any skid/stop animation so the player is immediately controllable
    event_data->started = 0;
    event_data->finished = 0;
    event_data->run_frames = 0;
    event_data->result_timer = 0;
}

void Event_Think(GOBJ *event) {
    XmDashData *event_data = event->userdata;
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // First frame: move the player off the tall bat/sandbag platform (Y~38) to
    // above the long flat track (Y~1.7), so the dash runs on the track.
    if (event_vars->game_timer == 1)
        hmn_data->phys.pos.X = XMDASH_START_X;

    // One-time setup once the player lands on the track: capture the start
    // (track-surface Y) + frame the camera + spawn the finish marker.
    if (!event_data->initialized && hmn_data->phys.air_state == 0) {
        event_data->initialized = 1;
        event_data->start_x  = hmn_data->phys.pos.X;
        event_data->start_y  = hmn_data->phys.pos.Y;
        event_data->distance = XmDash_Dist[Options_Main[OPT_DISTANCE].val];
        Match_SetNormalCamera(); // HRC fixes the camera; switch to normal fighter-follow
        Fighter_UpdateCameraBox(hmn);
        CmSubject *subject = hmn_data->camera_subject;
        subject->boundleft_curr  = subject->boundleft_proj;
        subject->boundright_curr = subject->boundright_proj;
        Match_CorrectCamera();
        Finish_Spawn(event_data);
    }

    // Reused wavedash timing HUD.
    WavedashCoreConfig cfg = { .show_hud = Options_Main[OPT_HUD].val, .short_hop_indicator = 1 };
    WavedashCore_Think(&event_data->core, hmn_data, &cfg);

    // On HRC the player's camera subject is left disabled/limited, so the
    // normal-mode camera never focuses the fighter. Force it active each frame
    // (kind 0 = always focus; lcancel.c:652 uses kind=0 to focus on a subject).
    if (hmn_data->camera_subject) {
        hmn_data->camera_subject->kind = 0;
        hmn_data->camera_subject->is_disable = 0;
    }

    // Live timer + best-time HUD (runs every frame, even before initialized,
    // so the display is always visible; run_frames is 0 until a dash starts).
    if (Pause_CheckStatus(1) != 2) {
        if (event_data->free_practice) {
            GXColor text_gold = {255, 211, 0, 255};
            Text_SetText(hud_time_text, 0, "");
            Text_SetText(hud_best_text, 0, "Free Practice");
            Text_SetColor(hud_best_text, 0, &text_gold);
        } else {
            Text_SetText(hud_time_text, 0, "%.2fs", event_data->run_frames / 60.0f);
            int dist_idx = Options_Main[OPT_DISTANCE].val;
            int best = (dist_idx == XMDASH_DEFAULT_DIST_IDX && event_data->persist_best)
                           ? event_data->persist_best : event_data->session_best[dist_idx];
            if (best) {
                GXColor text_white = {255, 255, 255, 255};
                Text_SetText(hud_best_text, 0, "Best: %.2fs", best / 60.0f);
                Text_SetColor(hud_best_text, 0, &text_white);
            } else {
                Text_SetText(hud_best_text, 0, "");
            }
        }
    } else {
        Text_SetText(hud_time_text, 0, "");
        Text_SetText(hud_best_text, 0, "");
    }

    // Dash lifecycle only runs after the one-time grounded setup above.
    if (!event_data->initialized)
        return;

    // Free practice: untimed, no finish/scoring — just the track + wavedash HUD.
    if (event_data->free_practice)
        return;

    // Apply a deferred distance-change reset now that we're running again, so the
    // marker move + teleport + camera follow all happen together this frame.
    if (event_data->reset_pending) {
        event_data->reset_pending = 0;
        if (event_data->finish_gobj) {
            JOBJ *jobj = event_data->finish_gobj->hsd_object;
            jobj->trans.X = event_data->start_x + event_data->distance;
            JOBJ_SetMtxDirtySub(jobj);
        }
        Dash_ResetToStart(event_data, hmn);
        Fighter_UpdateCameraBox(hmn);
    }

    float finish_x = event_data->start_x + event_data->distance;

    // Start the dash timer once the player moves past the start line.
    if (!event_data->started && !event_data->finished &&
        hmn_data->phys.pos.X > event_data->start_x + 1.0f) {
        event_data->started = 1;
        event_data->run_frames = 0;
    }
    if (event_data->started && !event_data->finished)
        event_data->run_frames++;

    // Finish when crossing the finish line.
    if (event_data->started && !event_data->finished && hmn_data->phys.pos.X >= finish_x) {
        event_data->finished = 1;
        SFX_Play(249); // finish cue

        int dist_idx = Options_Main[OPT_DISTANCE].val;
        int t = event_data->run_frames;
        if (event_data->session_best[dist_idx] == 0 || t < event_data->session_best[dist_idx])
            event_data->session_best[dist_idx] = t;
        if (dist_idx == XMDASH_DEFAULT_DIST_IDX) {
            int event_id = stc_memcard->EventBackup.event;
            Events_SetEventAsPlayed(event_id);
            if (event_data->persist_best == 0 || t < event_data->persist_best) {
                event_data->persist_best = t;
                Events_StoreEventScore(event_id, t);
                SFX_Play(325); // new-record cue (in addition to the finish SFX)
            }
        }
    }

    // After finishing, hold the result briefly then auto-teleport back to the
    // start for fast iteration. Hold length tunable in T9.
    if (event_data->finished) {
        event_data->result_timer++;
        if (event_data->result_timer >= 90) {
            Dash_ResetToStart(event_data, hmn);
            Fighter_UpdateCameraBox(hmn);
        }
    }
}

void HUD_GX(GOBJ *gobj, int pass) {
    XmDashData *event_data = gobj->userdata;
    FighterData *hmn_data = Fighter_GetGObj(0)->userdata;
    WavedashCoreConfig cfg = { .show_hud = Options_Main[OPT_HUD].val, .short_hop_indicator = 1 };
    WavedashCore_DrawHUD(&event_data->core, hmn_data, &cfg, pass);
}

EventMenu *Event_Menu = &Menu_Main;
