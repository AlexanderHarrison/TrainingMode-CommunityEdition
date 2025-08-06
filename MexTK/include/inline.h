#ifndef MEX_H_INLINE
#define MEX_H_INLINE

#include "structs.h"
#include "fighter.h"
#include "obj.h"
#include "mex.h"
#include "datatypes.h"
#include "gx.h"
#include "hsd.h"
#include "math.h"
#include "useful.h"
#include "scene.h"
#include "preload.h"

/*** Macros ***/
#define countof(A) (sizeof(A)/sizeof(*(A)))

/*** Functions ***/

static inline int min(int a, int b) {
    return a < b ? a : b;
}

static inline int max(int a, int b) {
    return a > b ? a : b;
}

static inline float lerp(float start, float end, float t)
{
    // clamp
    if (t < 0)
        t = 0;
    else if (t > 1)
        t = 1;

    return start + t * (end - start);
}

static inline int abs(int x)
{
    if (x < 0)
        return -x;
    else
        return x;
}

static float ceil(float x)
{
    if (x != x) return x;  // NaN
    if (x >= 2147483647.0f || x <= -2147483648.0f) return x;  // Too large for int
    
    int trunc = (int)x;
    
    if (x == (float)trunc)
        return x;
    if (x > 0)
        return (float)(trunc + 1);
    return (float)trunc;
}

static void PRIM_DRAW(float x, float y, float z, u32 color)
{
    gx_pipe->d.F32 = x;
    gx_pipe->d.F32 = y;
    gx_pipe->d.F32 = z;
    gx_pipe->d.U32 = color;
}

static HSD_Pad *PadGetMaster(int player_index)
{
    return &stc_master_pads->pad[player_index];
}
static HSD_Pad *PadGetEngine(int player_index)
{
    return &stc_engine_pads->pad[player_index];
}

static float Math_Vec2Distance(Vec2 *a, Vec2 *b)
{
    return sqrtf(squaref(a->X - b->X) + squaref(a->Y - b->Y));
}

static float Math_Vec3Distance(Vec3 *a, Vec3 *b)
{
    return sqrtf(squaref(a->X - b->X) + squaref(a->Y - b->Y) + squaref(a->Z - b->Z));
}

static inline float fmin(float a, float b)
{
    return (((a) < (b)) ? (a) : (b));
}

static inline float fmax(float a, float b)
{
    return (((a) > (b)) ? (a) : (b));
}

static inline float fclamp(float n, float min, float max) {
    return fmin(max, fmax(min, n));
}

static inline float smooth_lerp(float time, float start, float end) {
    float delta = time * time * (3.0f - 2.0f * time);
    return delta * (end - start) + start;
}

#endif
