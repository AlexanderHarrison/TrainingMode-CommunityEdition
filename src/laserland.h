/* laserland.h */

#include "../MexTK/mex.h"
#include "events.h"

struct LLData
{
    int success_count;
    int total_count;
    int frames_in_laserstart;
    int laserstart_counter;
    bool in_laserstart;
    bool in_f0_laserstart;
};

bool LL_InLaserstart(int current_state);
void Event_Exit(GOBJ *menu);
