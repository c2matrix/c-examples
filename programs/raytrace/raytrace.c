// This is a raytracer written in C.
// This code is verrrry much based on: www.scratchapixel.com
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ISECT_MT
// #define ISECT_PRECOMP12

#define FANCY_SHADING
// #define PLAIN_SHADING

#include "datatypes/common.h"
#include "linalg/linalg.h"
#include "triangle_mesh.h"
#include "intersection.h"

bool
vec3_array_to_ppm(vec3 *arr, const char *filename,
                  int width, int height) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        return false;
    }
    if (fprintf(f, "P6\n%d %d\n255\n", width, height) <= 0) {
        return false;
    }
    for (int i = 0; i < width * height; i++) {
        int r = CLAMP(255.0f * arr[i].x, 0, 255);
        int g = CLAMP(255.0f * arr[i].y, 0, 255);
        int b = CLAMP(255.0f * arr[i].z, 0, 255);
        if (fprintf(f, "%c%c%c", r, g, b) <= 0) {
            return false;
        }
    }
    fclose(f);
    return true;
}

typedef struct _ray_intersection {
    float t;
    vec2 uv;
    int tri_idx;
} ray_intersection;

void
tm_get_surface_props(triangle_mesh *me, ray_intersection *ri,
                     vec3 *normal, vec2 *tex_coords) {
    int t0 = 3 * ri->tri_idx;
    int t1 = 3 * ri->tri_idx + 1;
    int t2 = 3 * ri->tri_idx + 2;

    // Texture coordinates
    vec2 st0 = me->coords[t0];
    vec2 st1 = me->coords[t1];
    vec2 st2 = me->coords[t2];
    vec2 st0_scaled = v2_scale(st0, 1 - ri->uv.x - ri->uv.y);
    vec2 st1_scaled = v2_scale(st1, ri->uv.x);
    vec2 st2_scaled = v2_scale(st2, ri->uv.y);
    *tex_coords = v2_add(v2_add(st0_scaled, st1_scaled), st2_scaled);

    vec3 n0 = me->normals[t0];
    vec3 n1 = me->normals[t1];
    vec3 n2 = me->normals[t2];
    vec3 n0_scaled = v3_scale(n0, 1 - ri->uv.x - ri->uv.y);
    vec3 n1_scaled = v3_scale(n1, ri->uv.x);
    vec3 n2_scaled = v3_scale(n2, ri->uv.y);
    *normal = v3_add(v3_add(n0_scaled, n1_scaled), n2_scaled);
}

bool
tm_intersect(triangle_mesh *me, vec3 orig, vec3 dir,
             ray_intersection *ri) {
    float nearest = FLT_MAX;
    int *at_idx = me->indices;
    for (int i = 0; i < me->n_tris; i++) {
        vec3 v0 = me->positions[*at_idx++];
        vec3 v1 = me->positions[*at_idx++];
        vec3 v2 = me->positions[*at_idx++];
        float u, v, t;

#if defined(ISECT_MT)
        bool isect = moeller_trumbore_isect(orig, dir,
                                            v0, v1, v2,
                                            &t, &u, &v);
#elif defined(ISECT_PRECOMP12)
        float *trans = &me->precomp12[i*12];
        bool isect = precomp12_isect(orig, dir,
                                     v0, v1, v2,
                                     &t, &u, &v, trans);
#endif
        if (isect && t < nearest) {
            nearest = t;
            ri->t = t;
            ri->uv.x = u;
            ri->uv.y = v;
            ri->tri_idx = i;
        }
    }
    return nearest < FLT_MAX;
}

typedef struct _raytrace_settings {
    // Camera position
    vec3 position;
    mat4 view;
    float fov;
    int width;
    int height;
    char *mesh_file;
    char *image_file;
    vec3 bg_col;
} raytrace_settings;

raytrace_settings *
rt_from_args(int argc, char *argv[]) {
    if (argc != 9) {
        return NULL;
    }
    int width = atoi(argv[3]);
    int height = atoi(argv[4]);
    if (width <= 0 || width > 2048 ||
        height <= 0 || height > 2048) {
        return NULL;
    }
    double fov_d = atof(argv[5]);
    if (fov_d <= 0.0 || fov_d >= 100.0) {
        return NULL;
    }

    double r = atof(argv[6]);
    double g = atof(argv[7]);
    double b = atof(argv[8]);
    if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1) {
        return NULL;
    }
    raytrace_settings *me = (raytrace_settings *)
        malloc(sizeof(raytrace_settings));
    me->width = width;
    me->height = height;
    me->mesh_file = strdup(argv[1]);
    me->image_file = strdup(argv[2]);
    me->fov = fov_d;
    me->bg_col.x = r;
    me->bg_col.y = g;
    me->bg_col.z = b;
    me->position = (vec3){0, 0, -20};
    mat4 tmp = {
        {
            {0.707107, -0.331295, 0.624695, 0},
            {0, 0.883452, 0.468521, 0},
            {-0.707107, -0.331295, 0.624695, 0},
            {-1.63871, -5.747777, -40.400412, 1}
        }
    };
    me->view = m4_inverse(tmp);
    return me;
}

void
rt_free(raytrace_settings *me) {
    free(me->mesh_file);
    free(me->image_file);
    free(me);
}

vec3
rt_ray_direction(raytrace_settings *rt, int x, int y,
                 float aspect_ratio, float scale) {

    float ray_x = (2 * (x + 0.5) / (float)rt->width - 1)
        * aspect_ratio * scale;
    float ray_y = (1 - 2 * (y + 0.5) / (float)rt->height)
        * scale;
    vec3 dir = {ray_x, ray_y, -1};
    dir = m4_mul_v3d(rt->view, dir);
    dir = v3_normalize(dir);
    return dir;
}

static vec3
shade_intersection(vec3 orig, vec3 dir, ray_intersection *ri,
                   triangle_mesh *tm) {
    #ifdef FANCY_SHADING
    vec3 normal;
    vec2 coords;
    tm_get_surface_props(tm, ri, &normal, &coords);
    float n_dot_view = MAX(0.0f, v3_dot(normal, v3_neg(dir)));
    int m = 10;
    float checker = (fmod(coords.x * m, 1.0) > 0.5) ^
        (fmod(coords.y * m, 1.0) < 0.5);
    float c = 0.3 * (1 - checker) + 0.7 * checker;
    return v3_from_scalar(c * n_dot_view);
    #else
    return (vec3){1.0, 1.0, 1.0};
    #endif
}

vec3
cast_ray(vec3 orig, vec3 dir, vec3 bg_col, triangle_mesh *tm) {
    ray_intersection ri;
    if (tm_intersect(tm, orig, dir, &ri)) {
        return shade_intersection(orig, dir, &ri, tm);
    }
    return bg_col;
}

void
render(raytrace_settings *rt, triangle_mesh *tm, vec3 *fbuf) {
    int w = rt->width;
    int h = rt->height;
    vec3 orig = m4_mul_v3p(rt->view, rt->position);
    float scale = tan(to_rad(rt->fov * 0.5));
    float aspect_ratio = (float)w / (float)h;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            vec3 dir = rt_ray_direction(rt, x, y, aspect_ratio, scale);
            *fbuf = cast_ray(orig, dir, rt->bg_col, tm);
            fbuf++;
        }
    }
}

void
usage() {
    printf("usage: raytrace mesh-file image "
           "width[0-2048] height[0-2048] fov[0-100] "
           "bg_r[0-1] bg_g[0-1] bg_b[0-1]\n");
    exit(1);
}

int
main(int argc, char *argv[]) {
    raytrace_settings *rt = rt_from_args(argc, argv);
    if (!rt) {
        usage();
    }
    triangle_mesh *tm = tm_from_file(rt->mesh_file);
    if (!tm) {
        error("Failed to read mesh from file '%s'.\n", rt->mesh_file);
    }
    int w = rt->width;
    int h = rt->height;
    vec3 *fbuf = (vec3 *)malloc(w * h * sizeof(vec3));
    render(rt, tm, fbuf);
    if (!vec3_array_to_ppm(fbuf, rt->image_file, w, h)) {
        error("Failed to save to '%s'.", rt->image_file);
    }
    free(fbuf);
    tm_free(tm);
    rt_free(rt);
    return 0;
}