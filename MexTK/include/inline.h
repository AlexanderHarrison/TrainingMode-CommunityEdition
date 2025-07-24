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
    int integerPart = x; // Get the integer part of the number

    if (x == (float)integerPart)
    {
        // No fractional part, return the number as is
        return x;
    }
    else if (x > 0)
    {
        // Positive number with a fractional part, return the next integer
        return integerPart + 1;
    }
    else
    {
        // Negative number with a fractional part, return the current integer
        return integerPart;
    }
}

static void PRIM_DRAW(PRIM *gx, float x, float y, float z, int color)
{
    *(volatile float *)(gx->data) = x;
    *(volatile float *)(gx->data) = y;
    *(volatile float *)(gx->data) = z;
    *(volatile int   *)(gx->data) = color;
}

static HSD_Pad *PadGet(int playerIndex, int padType)
{
    HSD_Pads *pads = 0;

    // get the correct pad
    if (padType == PADGET_MASTER)
        pads = (HSD_Pads *)0x804c1fac;
    else if (padType == PADGET_ENGINE)
        pads = (HSD_Pads *)0x804c21cc;

    if (pads == 0)
        return 0;

    return (&pads->pad[playerIndex]);
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
