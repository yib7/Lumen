#if !defined(_WIN32)
#  define _POSIX_C_SOURCE 200809L   /* expose isatty()/fileno() under -std=c11 on glibc */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "render.h"
#include "geometry.h"

#ifdef _OPENMP
#include <omp.h>
#endif

/* Platform header for isatty()/fileno() (underscore-prefixed on Windows). */
#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

#define SHADOW_EPS 1e-4

/* 1 when stderr is an interactive terminal. Progress is only emitted then, so
 * redirected/piped stderr (files, CI logs) stays free of carriage-return spam. */
static int stderr_is_tty(void) {
#ifdef _WIN32
    return _isatty(_fileno(stderr));
#else
    return isatty(fileno(stderr));
#endif
}

/* Resolve the surface color at a hit, accounting for plane checkerboards. */
static Color surface_albedo(const Object *obj, Vec3 point) {
    const Material *m = &obj->material;
    if (m->checker) {
        /* Checker parity from floor(x)+floor(y)+floor(z), kept in double rather
         * than cast to long: a plane hit billions of units out (a near-grazing
         * ray, or a hostile scene) would overflow a 32-bit long, and the
         * double->long conversion is undefined in that case (C11 6.3.1.4).
         * fmod is defined for all finite inputs and matches the old parity
         * exactly for normal coordinates. */
        double parity = fmod(floor(point.x) + floor(point.y) + floor(point.z), 2.0);
        if (fabs(parity) > 0.5) {
            return m->albedo2;
        }
    }
    return m->albedo;
}

/* 1 if any object blocks the straight path from `point` to `light`. */
static int in_shadow(const Scene *scene, Vec3 point, const Light *light) {
    Vec3 to_light = vec3_sub(light->position, point);
    double dist = vec3_len(to_light);
    Ray shadow = { point, vec3_scale(to_light, 1.0 / dist) };
    return scene_intersect_any(scene, shadow, SHADOW_EPS, dist);
}

/* Phong-style local shading from all lights at a single hit point. */
static Color shade_local(const Scene *scene, Ray ray, const Hit *hit) {
    Color base = surface_albedo(hit->object, hit->point);
    const Material *m = &hit->object->material;

    /* Ambient term keeps shadowed areas from going pure black. */
    Color color = vec3_scale(base, 0.1);
    Vec3 normal = hit->normal;
    Vec3 view = vec3_scale(ray.dir, -1.0);

    for (int i = 0; i < scene->light_count; i++) {
        const Light *light = &scene->lights[i];
        if (in_shadow(scene, hit->point, light)) {
            continue;
        }

        Vec3 to_light = vec3_sub(light->position, hit->point);
        double dist = vec3_len(to_light);
        Vec3 light_dir = vec3_scale(to_light, 1.0 / dist);

        /* Quadratic-ish falloff, kept gentle so scenes stay lit. */
        double atten = light->intensity /
                       (0.002 * dist * dist + 0.02 * dist + 0.2);

        double diff = vec3_dot(light_dir, normal);
        if (diff <= 0.0) {
            continue;
        }

        Color lit = vec3_mul(base, light->color);
        color = vec3_add(color, vec3_scale(lit, diff * atten));

        /* Specular highlight from the reflected light direction. */
        Vec3 reflect = vec3_reflect(vec3_scale(light_dir, -1.0), normal);
        double spec_dot = vec3_dot(reflect, view);
        if (spec_dot > 0.0) {
            double spec = pow(spec_dot, m->shininess) * atten;
            color = vec3_add(color, vec3_scale(light->color, spec));
        }
    }
    return color;
}

/* Trace one ray and return its color, recursing for reflective materials. */
static Color trace(const Scene *scene, Ray ray, int depth, int max_depth) {
    Hit hit;
    if (!scene_intersect(scene, ray, 1e-4, &hit)) {
        /* Vertical sky gradient based on the ray's upward component. */
        double t = 0.5 * (vec3_normalize(ray.dir).y + 1.0);
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        return vec3_lerp(scene->sky_bottom, scene->sky_top, t);
    }

    Color local = shade_local(scene, ray, &hit);

    const Material *m = &hit.object->material;
    if (m->kind == MAT_REFLECTIVE && depth < max_depth && m->reflectivity > 0.0) {
        Vec3 refl_dir = vec3_normalize(vec3_reflect(ray.dir, hit.normal));
        Ray refl = { hit.point, refl_dir };
        Color reflected = trace(scene, refl, depth + 1, max_depth);
        local = vec3_lerp(local, reflected, m->reflectivity);
    }
    return local;
}

/* Clamp a linear color to [0,1] and convert to an 8-bit channel. When gamma
 * is set, gamma-encode (pow 1/2.2) after clamping so pow never sees a negative;
 * with gamma == 0 this is byte-for-byte the old linear computation. */
static unsigned char to_byte(double c, int gamma) {
    if (c < 0.0) c = 0.0;
    if (c > 1.0) c = 1.0;
    if (gamma) c = pow(c, 1.0 / 2.2);
    return (unsigned char)(c * 255.0 + 0.5);
}

unsigned char *render_scene(const Scene *scene, const RenderConfig *cfg) {
    int w = cfg->width;
    int h = cfg->height;
    int n = cfg->samples < 1 ? 1 : cfg->samples;

    /* Cast every operand to size_t so the product is computed in size_t
     * arithmetic, never in 32-bit int (which would overflow for large w*h). */
    unsigned char *pixels = malloc((size_t)w * (size_t)h * 3);
    if (!pixels) {
        return NULL;
    }

    double aspect = (double)w / (double)h;
    double pix = (2.0 * scene->camera.fov_scale) / h;
    double start_x = -aspect * scene->camera.fov_scale;
    double start_y = scene->camera.fov_scale;
    double inv_samples = 1.0 / (double)(n * n);

    /* Build one orthonormal camera basis (forward/right/up) up front. For the
     * legacy path this is exactly +z/+x/+y, so the ray direction below reduces
     * to vec3(px, py, 1) byte-for-byte; scenes without a look-at are unchanged. */
    Vec3 forward, right, up;
    if (scene->camera.has_look) {
        Vec3 world_up = vec3(0.0, 1.0, 0.0);
        forward = vec3_normalize(vec3_sub(scene->camera.target, scene->camera.position));
        right = vec3_cross(world_up, forward);
        if (vec3_len(right) < 1e-9) {          /* looking straight up/down: pick another ref */
            world_up = vec3(0.0, 0.0, 1.0);
            right = vec3_cross(world_up, forward);
        }
        right = vec3_normalize(right);
        up = vec3_cross(forward, right);
    } else {
        /* legacy: look down +z, +y up, +x right (identical to vec3(px,py,1)) */
        forward = vec3(0.0, 0.0, 1.0);
        right   = vec3(1.0, 0.0, 0.0);
        up      = vec3(0.0, 1.0, 0.0);
    }

#ifdef _OPENMP
    if (cfg->threads > 0) {
        omp_set_num_threads(cfg->threads);
    }
#endif

    /* Live progress for long renders: an atomic completed-row counter drives a
     * "\rrow d/d" line on STDERR only. show_progress gates it to a TTY, and
     * prog_step yields ~100 updates regardless of height. STDERR-only, so the
     * pixel buffer / stdout are untouched and image output is byte-identical. */
    int done_rows = 0;
    int show_progress = stderr_is_tty();
    int prog_step = h / 100;
    if (prog_step < 1) prog_step = 1;

    /* Rows are independent, which makes the outer loop trivially parallel. */
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 4)
#endif
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Color sum = {0, 0, 0};

            /* Uniform NxN grid of sub-samples for anti-aliasing. */
            for (int sy = 0; sy < n; sy++) {
                for (int sx = 0; sx < n; sx++) {
                    double ox = (sx + 0.5) / n;
                    double oy = (sy + 0.5) / n;
                    double px = start_x + (x + ox) * pix;
                    double py = start_y - (y + oy) * pix;

                    Ray ray;
                    ray.origin = scene->camera.position;
                    Vec3 dir = vec3_add(vec3_add(vec3_scale(right, px),
                                                 vec3_scale(up, py)), forward);
                    ray.dir = vec3_normalize(dir);
                    sum = vec3_add(sum, trace(scene, ray, 0, cfg->max_depth));
                }
            }

            Color avg = vec3_scale(sum, inv_samples);
            size_t idx = ((size_t)y * w + x) * 3;
            pixels[idx + 0] = to_byte(avg.x, cfg->gamma);
            pixels[idx + 1] = to_byte(avg.y, cfg->gamma);
            pixels[idx + 2] = to_byte(avg.z, cfg->gamma);
        }

        /* Row finished: atomically bump the shared counter and capture the new
         * value into a private local, then emit a milestone (~every prog_step
         * rows, plus the final row). The critical section only stops two
         * concurrent milestone prints from interleaving (~100 entries total). */
        int my_row;
#ifdef _OPENMP
        #pragma omp atomic capture
        my_row = ++done_rows;
#else
        my_row = ++done_rows;
#endif
        if (show_progress && (my_row % prog_step == 0 || my_row == h)) {
#ifdef _OPENMP
            #pragma omp critical(progress)
#endif
            {
                fprintf(stderr, "\rrow %d/%d", my_row, h);
                fflush(stderr);
            }
        }
    }

    /* Terminate the progress line so later stderr output starts on a fresh row. */
    if (show_progress) {
        fprintf(stderr, "\n");
    }

    return pixels;
}
