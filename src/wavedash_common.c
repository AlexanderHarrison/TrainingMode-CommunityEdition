#include "wavedash_common.h"
#include "events.h"

#define FAILFRAMES 8

static char *panel_label[3] = {"Timing", "Angle", "Success Rate"};
static char timing_text[24] = "-";
static char angle_text[24] = "-";
static char success_rate_text[24] = "-";
static char *panel_info[3] = {timing_text, angle_text, success_rate_text};

void WavedashCore_DrawHUD(WavedashCore *core, FighterData *hmn_data, const WavedashCoreConfig *cfg, int pass)
{
    if (!cfg->show_hud) return;
    if (pass != 2) return;

    // Draw info panel
    event_vars->HUD_DrawInfoPanel((const char**)panel_label, (const char**)panel_info, countof(panel_label));

    #define W 1.6f // width of square
    #define H 2.f // height of square
    #define P 0.1f // amount of padding
    #define SX (-W * 15.f * 0.5f - P/2) // starting x
    #define SY 16.f // starting y

    static GXColor colors[16] = {
        {0, 0, 0, 255}, // background

        // early
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},
        {0xff, 0x60, 0x60, 0xff},

        // interpolate green -> orange
        { 0xb4, 0xff, 0xb4, 0xff },
        { 0xc3, 0xf3, 0x99, 0xff },
        { 0xd5, 0xe3, 0x79, 0xff },
        { 0xe5, 0xd2, 0x61, 0xff },
        { 0xf2, 0xc0, 0x51, 0xff },
        { 0xfc, 0xab, 0x4e, 0xff },
        { 0xff, 0x9d, 0x54, 0xff },
        { 0xff, 0x90, 0x60, 0xff },
    };

    static Rect rects[16] = {
        {SX-P/2, SY, 15.f*W+P, H}, // background
        {SX+P/2+W*0, SY+P, W-P, H-P*2},
        {SX+P/2+W*1, SY+P, W-P, H-P*2},
        {SX+P/2+W*2, SY+P, W-P, H-P*2},
        {SX+P/2+W*3, SY+P, W-P, H-P*2},
        {SX+P/2+W*4, SY+P, W-P, H-P*2},
        {SX+P/2+W*5, SY+P, W-P, H-P*2},
        {SX+P/2+W*6, SY+P, W-P, H-P*2},
        {SX+P/2+W*7, SY+P, W-P, H-P*2},
        {SX+P/2+W*8, SY+P, W-P, H-P*2},
        {SX+P/2+W*9, SY+P, W-P, H-P*2},
        {SX+P/2+W*10, SY+P, W-P, H-P*2},
        {SX+P/2+W*11, SY+P, W-P, H-P*2},
        {SX+P/2+W*12, SY+P, W-P, H-P*2},
        {SX+P/2+W*13, SY+P, W-P, H-P*2},
        {SX+P/2+W*14, SY+P, W-P, H-P*2},
    };
    event_vars->HUD_DrawRects(rects, colors, countof(rects));

    Tri tris[countof(core->airdodge_frame)*2];
    GXColor tri_color[countof(core->airdodge_frame)*2];

    int ad_count = core->airdodge_count;
    int *ad_frame = core->airdodge_frame;

    static float x_pos[countof(core->airdodge_frame)] = {0};
    static int show_count = 0;

    if (core->timer == -1) {
        show_count = ad_count;

        // animate
        for (int i = 0; i < ad_count; ++i) {
            int f = ad_frame[i];
            float x = SX + ((float)f + (7.f - hmn_data->attr.jump_startup_time - 1.f) + 0.5f) * W;

            float px = x_pos[i];
            float dx = x - px;
            if (fabs(dx) > 0.01f)
                x = px + dx * 0.3f;
            x_pos[i] = x;
        }
    }

    for (int i = 0; i < show_count; ++i) {
        float x = x_pos[i];
        float y = SY - 0.8f;
        int f = ad_frame[i];
        if (i != 0 && ad_frame[i-1] == f)
            y -= H;
        #define B 0.3f
        #define B2 0.1f
        #define B3 0.15f
        tris[i*2+1][0] = (Vec2) {x, y};
        tris[i*2+1][1] = (Vec2) {x+W*0.3f, y-H*0.6f};
        tris[i*2+1][2] = (Vec2) {x-W*0.3f, y-H*0.6f};
        tri_color[i*2+1] = (GXColor) {255, 255, 255, 255};

        tris[i*2+0][0].X = tris[i*2+1][0].X;
        tris[i*2+0][0].Y = tris[i*2+1][0].Y + 0.3f;
        tris[i*2+0][1].X = tris[i*2+1][1].X + 0.15f;
        tris[i*2+0][1].Y = tris[i*2+1][1].Y - 0.1f;
        tris[i*2+0][2].X = tris[i*2+1][2].X - 0.15f;
        tris[i*2+0][2].Y = tris[i*2+1][2].Y - 0.1f;
        tri_color[i*2+0] = (GXColor) {0, 0, 0, 200};
    }

    event_vars->HUD_DrawTris(tris, tri_color, show_count * 2);

    static Rect early = { SX, SY + 3, 0, 0 };
    static Rect timing0 = { 0, SY + 4.5f, 0, 0 };
    static Rect timing1 = { 0, SY + 3, 0, 0 };
    static Rect late = { -SX, SY + 3, 0, 0 };
    event_vars->HUD_DrawText("Early", &early, 0.45f);
    event_vars->HUD_DrawText("Wavedash", &timing0, 0.4f);
    event_vars->HUD_DrawText("Timing", &timing1, 0.4f);
    event_vars->HUD_DrawText("Late", &late, 0.45f);
}

void WavedashCore_Think(WavedashCore *core, FighterData *hmn_data, const WavedashCoreConfig *cfg)
{
    // start sequence on jump squat
    if (hmn_data->state_id == ASID_KNEEBEND && hmn_data->TM.state_frame == 0)
    {
        core->timer = 0;
        core->airdodge_count = 0;
    }

    // Do nothing if sequence hasn't started
    if (core->timer < 0)
        return;

    // run sequence logic
    core->timer++;

    // The game tracks whether the current jump is going to be a short hop in
    // `state_var1`. The value at the end of kneebend is what we want.
    if (hmn_data->state_id == ASID_KNEEBEND)
        core->short_hop = hmn_data->state_var.state_var1;
    if (cfg->short_hop_indicator && core->short_hop)
        Fighter_ColAnim_Apply(hmn_data, 107, 0); // make sparkles

    // Record airdodge timings.
    if (core->airdodge_count < (int)countof(core->airdodge_frame) && (hmn_data->input.down & PAD_TRIGGER_L))
        core->airdodge_frame[core->airdodge_count++] = core->timer;
    if (core->airdodge_count < (int)countof(core->airdodge_frame) && (hmn_data->input.down & PAD_TRIGGER_R))
        core->airdodge_frame[core->airdodge_count++] = core->timer;

    // Real airdodge
    if (hmn_data->TM.state_frame == 0 &&
            (hmn_data->state_id == ASID_ESCAPEAIR ||
            (hmn_data->state_id == ASID_LANDINGFALLSPECIAL &&
             hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR &&
             hmn_data->TM.state_prev_frames[0] == 0)))
    {
        PADStatus *stat = PadGetRaw(hmn_data->pad_index);
        core->angle = -atan2(stat->stickY, fabs(stat->stickX)) / M_1DEGREE;
    }

    if (core->result == -1) {
        if (hmn_data->state_id == ASID_LANDINGFALLSPECIAL
            && hmn_data->TM.state_frame == 0
            && hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR
            && hmn_data->TM.state_prev[2] == ASID_KNEEBEND)
        {
            // success
            core->wd_attempted++;
            core->wd_succeeded++;

            // check for perfect
            int perfect_frame = (int)hmn_data->attr.jump_startup_time + 1;
            for (int i = 0; i < core->airdodge_count; ++i) {
                if (
                    core->airdodge_frame[i] == perfect_frame
                    || (core->airdodge_frame[i] == perfect_frame+1 && core->short_hop)
                ) {
                    SFX_Play(303);
                    break;
                }
            }
            core->result = 1;
        }
        else if (hmn_data->TM.state_frame >= FAILFRAMES &&
                core->airdodge_count != 0 &&
                (hmn_data->state_id == ASID_JUMPF ||
                hmn_data->state_id == ASID_JUMPB ||
                hmn_data->state_id == ASID_ESCAPEAIR)) {
            // failure
            core->wd_attempted++;
            SFX_PlayCommon(3);
            core->result = 0;
        }
    }

    // We need to wait until the wavedash finishes to reset in order to capture late LR presses
    if (core->timer == 13) {
        // Reset
        core->result = -1;
        core->timer = -1;
        Fighter_ColAnim_Remove(hmn_data, 107); // remove sparkles

        // Wavedash not attempted
        if (core->airdodge_count == 0)
            return;

        // update timing
        int timing = core->airdodge_frame[0] - (int)hmn_data->attr.jump_startup_time - 1;
        for (int i = 1; i < core->airdodge_count; ++i) {
            int new_timing = core->airdodge_frame[i] - (int)hmn_data->attr.jump_startup_time - 1;
            if (timing < 0)
                timing = new_timing;
            else
                break;
        }

        if (timing < 0)
            sprintf(timing_text, "%df Early", -timing);
        else if (timing > 0)
            sprintf(timing_text, "%df Late", timing);
        else
            sprintf(timing_text, "Perfect");

        // update angle
        sprintf(angle_text, "%.1f", core->angle);

        // update success rate
        int success = core->wd_succeeded;
        int attempted = core->wd_attempted;
        float success_rate = (float)success * 100.f / (float)attempted;
        sprintf(success_rate_text, "%d (%.2f%%)", success, success_rate);
    }
}
