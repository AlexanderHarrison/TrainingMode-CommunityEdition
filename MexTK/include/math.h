#ifndef MEX_H_MATH
#define MEX_H_MATH

#include "structs.h"
#include "datatypes.h"

#define HUGE_ENUF 1e+300 // _HUGE_ENUF*_HUGE_ENUF must overflow
#define INFINITY ((float)(HUGE_ENUF * HUGE_ENUF))
#define M_PI (3.14159265358979323846)
#define M_1DEGREE (0.0174533)
#define M_NAN ((float)(INFINITY * 0.0F))

static inline float fabs(float v)
{
    if (v < 0.0f)
        return -v;
    else
        return v;
}

static inline float sign(float v)
{
    if (v >= 0.0f)
        return 1.0f;
    else
        return -1.0f;
}

static inline float squaref(float v)
{
    return v*v;
}

static inline float Vec2_Magnitude(Vec2 *v)
{
    return fabs(v->X) + fabs(v->Y);
}

/*** Functions ***/
// float fmod(float a, float b);
// float atan(float in);
float atan2(float y, float x);
float sin(float x);
float cos(float x);
void MTXOrtho(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f); //
void MTXLookAt(Mtx *dest, Vec3 *eye, Vec3 *up, Vec3 *target);
void MTXRotAxisRad(Mtx *m, Vec3 *axis, float rad); // Sets a matrix for rotation about an arbitrary axis whose ( x, y, z ) components are specified by axis.
void MTXMultVec(Mtx *m, Vec3 *src, Vec3 *dst);     // Post-multiplies a 3D vector or 3D point by a matrix ( m x src = dst ).
void MTXLightPerspective(Mtx *m, float fovy, float aspect, float scaleS, float scaleT, float transS, float transT);
void VECNormalize(Vec3 *src, Vec3 *dest);
void VECScale(Vec3 *src, Vec3 *dst, float scale);
void VECAdd(Vec3 *a, Vec3 *b, Vec3 *ab);
void VECSubtract(Vec3 *a, Vec3 *b, Vec3 *a_b);
void PSMTXIdentity(Mtx *dest);
void PSMTXCopy(Mtx *src, Mtx *dest);
void PSMTXConcat(Mtx *a, Mtx *b, Mtx *ab);
void VECMultAndAdd(Vec3 *a, Vec3 *b);
float VECDotProduct(Vec3 *a, Vec3 *b);
void VECCrossProduct(Vec3 *a, Vec3 *b, Vec3 *axb);
float Vec2_Normalize(Vec2 *v);
float Vec3_Normalize(Vec3 *v);
void Vec2_Add(Vec2 *a, Vec2 *b);
Vec3 *Vec3_CrossDirection(Vec3 *a, Vec3 *b, Vec3 *axb);
// void MTXQuat(Mtx *m, Vec4 *dest);           // quat to matrix
void HSD_MkRotationMtx(Mtx *dest, Vec4 *v); // quat to rot matrix
void HSD_MtxGetScale(Mtx *m, Vec3 *dest);
void HSD_MtxGetRotation(Mtx *m, Vec3 *dest);
void HSD_MtxGetTranslate(Mtx *m, Vec3 *dest);
void HSD_MtxSRT(Mtx *m, Vec3 *scale, Vec3 *rot, Vec3 *trans, int unk);
void MatToQuat(Mtx *m, Vec4 *dest); // matrix to quat
float sqrtf(float num);
void MTXRotRad(Mtx m, char axis, f32 rad);
float Vec2_CalculateAngle(Vec2 *a, Vec2 *b);
float Vec3_CalculateAngle(Vec3 *a, Vec3 *b);
void Vec3_RotateAboutUnitAxis(Vec3 *v, Vec3 *axis, float angle);

#endif
