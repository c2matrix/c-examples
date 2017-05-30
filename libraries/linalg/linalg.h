#ifndef LINALG_H
#define LINALG_H

#include <math.h>
#include <stdbool.h>

#define LINALG_EPSILON 1e-8
#define LINALG_APPROX_EQ(x, y) (fabs(x - y) < LINALG_EPSILON)

inline float
to_rad(const float deg) {
    return deg * M_PI / 180;
}

inline float
to_deg(const float rad) {
    return rad * (180.0f / M_PI);
}

// A simple linear algebra library for C.
typedef struct _vec3 {
    float x, y, z;
} vec3;

inline vec3
v3_sub(vec3 l, vec3 r) {
    return (vec3){l.x - r.x, l.y - r.y, l.z - r.z};
}

inline vec3
v3_add(vec3 l, vec3 r) {
    return (vec3){l.x + r.x, l.y + r.y, l.z + r.z};
}

inline vec3
v3_cross(vec3 l, vec3 r) {
    vec3 ret = {
        l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x
    };
    return ret;
}

inline float
v3_dot(vec3 l, vec3 r) {
    return l.x * r.x + l.y * r.y + l.z * r.z;
}

inline vec3
v3_normalize(vec3 in) {
    vec3 out = in;
    float norm = in.x * in.x + in.y * in.y + in.z * in.z;
    if (norm > 0) {
        float factor = 1.0f / sqrt(norm);
        out.x *= factor;
        out.y *= factor;
        out.z *= factor;
    }
    return out;
}

inline bool
v3_approx_eq(vec3 l, vec3 r) {
    return fabs(l.x - r.x) < LINALG_EPSILON &&
        fabs(l.y - r.y) < LINALG_EPSILON &&
        fabs(l.z - r.z) < LINALG_EPSILON;
}

typedef struct _mat4 {
    float d[4][4];
} mat4;

// Most matrix functions doesn't need to be inline.
mat4 m4_identity();
void m4_print(mat4 m, int n_dec);
mat4 m4_inverse(mat4 m);
bool m4_approx_eq(mat4 l, mat4 r);

// But some do
inline vec3
m4_mul_v3(mat4 m, vec3 v) {
    float x = v.x, y = v.y, z = v.z;
    float a = x * m.d[0][0] + y * m.d[1][0] + z * m.d[2][0] + m.d[3][0];
    float b = x * m.d[0][1] + y * m.d[1][1] + z * m.d[2][1] + m.d[3][1];
    float c = x * m.d[0][2] + y * m.d[1][2] + z * m.d[2][2] + m.d[3][2];
    float w = x * m.d[0][3] + y * m.d[1][3] + z * m.d[2][3] + m.d[3][3];
    return (vec3){a / w, b / w, c / w};
}

#endif