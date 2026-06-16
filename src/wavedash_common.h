#ifndef _WAVEDASH_COMMON_H_
#define _WAVEDASH_COMMON_H_

#include "../MexTK/mex.h"

typedef struct WavedashCore {
    int timer;
    int airdodge_count;
    int airdodge_frame[8];
    int result;
    float angle;
    int short_hop;
    int wd_attempted;
    int wd_succeeded;
} WavedashCore;

typedef struct WavedashCoreConfig {
    int show_hud;
    int short_hop_indicator;
} WavedashCoreConfig;

void WavedashCore_Think(WavedashCore *core, FighterData *hmn_data, const WavedashCoreConfig *cfg);
void WavedashCore_DrawHUD(WavedashCore *core, FighterData *hmn_data, const WavedashCoreConfig *cfg, int pass);

#endif // _WAVEDASH_COMMON_H_
