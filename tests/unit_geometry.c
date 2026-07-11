/*
 * Unit checks for the box intersection normal. Regression coverage for the
 * audit finding where a ray exiting a box (starting inside it) was handed the
 * entry-face axis, producing an inward-pointing normal.
 *
 * Build (from repo root):
 *   gcc -std=c11 -Wall -Wextra -Isrc tests/unit_geometry.c \
 *       src/geometry.c src/vec3.c -lm -o unit_geometry
 */
#include <stdio.h>
#include <math.h>
#include "geometry.h"

static int failures = 0;

static void check(int cond, const char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    } else {
        printf("ok:   %s\n", msg);
    }
}

/* A finite value that is neither NaN nor infinite. */
static int finite_val(double v) {
    return !(isnan(v) || isinf(v));
}

int main(void) {
    Box b = { {-1, -1, -1}, {1, 1, 1} };
    double t;
    Vec3 point, normal;

    /* 1. Ray from outside, travelling +z, should hit the -z face (normal -z). */
    {
        Ray r = { {0, 0, -5}, {0, 0, 1} };
        int hit = intersect_box(&b, r, 1e-4, &t, &point, &normal);
        check(hit, "outside ray hits box");
        check(fabs(normal.z + 1.0) < 1e-9 && fabs(normal.x) < 1e-9 &&
              fabs(normal.y) < 1e-9, "entry normal points -z (outward)");
    }

    /* 2. Ray starting INSIDE the box, travelling +z, must exit the +z face.
     *    The outward normal there is +z. The bug produced -z (inward). */
    {
        Ray r = { {0, 0, 0}, {0, 0, 1} };
        int hit = intersect_box(&b, r, 1e-4, &t, &point, &normal);
        check(hit, "inside ray hits box (exit face)");
        check(fabs(normal.z - 1.0) < 1e-9 && fabs(normal.x) < 1e-9 &&
              fabs(normal.y) < 1e-9, "exit normal points +z (outward, not inward)");
        /* An outward exit normal must have a positive dot with the ray dir. */
        check(vec3_dot(normal, r.dir) > 0.0,
              "exit normal agrees with travel direction");
    }

    /* 3. Inside ray travelling -x must exit the -x face with normal -x. */
    {
        Ray r = { {0, 0, 0}, {-1, 0, 0} };
        int hit = intersect_box(&b, r, 1e-4, &t, &point, &normal);
        check(hit, "inside ray hits box (-x exit)");
        check(fabs(normal.x + 1.0) < 1e-9 && fabs(normal.y) < 1e-9 &&
              fabs(normal.z) < 1e-9, "exit normal points -x (outward)");
    }

    /* 4. Normals must always be finite unit-ish vectors, never NaN. */
    {
        Ray r = { {0.2, -0.3, 0.1}, {0, 1, 0} };
        int hit = intersect_box(&b, r, 1e-4, &t, &point, &normal);
        check(hit, "inside ray hits box (+y exit)");
        check(finite_val(normal.x) && finite_val(normal.y) && finite_val(normal.z),
              "exit normal is finite (no NaN/Inf)");
        double len = sqrt(vec3_dot(normal, normal));
        check(fabs(len - 1.0) < 1e-9, "exit normal is unit length");
    }

    /* --- Sphere intersection (unit sphere at the origin) --- */
    Sphere s = { {0, 0, 0}, 1.0 };

    /* 5. Tangent ray grazes the sphere at exactly one point (disc == 0): it hits
     *    the tangent point with an outward normal there. */
    {
        Ray r = { {0, 1, -5}, {0, 0, 1} };
        int hit = intersect_sphere(&s, r, 1e-4, &t, &point, &normal);
        check(hit, "tangent ray hits the sphere");
        check(fabs(t - 5.0) < 1e-9, "tangent hit distance is 5");
        check(fabs(normal.y - 1.0) < 1e-9 && fabs(normal.x) < 1e-9 &&
              fabs(normal.z) < 1e-9, "tangent normal points +y (outward)");
    }

    /* 6. Ray starting at the center exits through the far side; the nearer root
     *    is behind t_min, so the exit point (t = +radius) is used. */
    {
        Ray r = { {0, 0, 0}, {0, 0, 1} };
        int hit = intersect_sphere(&s, r, 1e-4, &t, &point, &normal);
        check(hit, "inside ray hits sphere (exit point)");
        check(fabs(t - 1.0) < 1e-9, "inside-sphere exit distance is the radius");
        check(fabs(normal.z - 1.0) < 1e-9, "inside-sphere exit normal points +z");
    }

    /* 7. A sphere entirely behind the ray origin must not be hit. */
    {
        Ray r = { {0, 0, 5}, {0, 0, 1} };
        int hit = intersect_sphere(&s, r, 1e-4, &t, &point, &normal);
        check(!hit, "sphere behind the ray is a miss");
    }

    /* --- Plane intersection (y = 0 plane: normal +y, distance 0) --- */
    Plane pl = { {0, 1, 0}, 0.0 };

    /* 8. A ray parallel to the plane never intersects it. */
    {
        Ray r = { {0, 5, 0}, {1, 0, 0} };
        int hit = intersect_plane(&pl, r, 1e-4, &t, &point, &normal);
        check(!hit, "ray parallel to plane is a miss");
    }

    /* 9. A plane behind the ray (ray travelling away from it) must not be hit. */
    {
        Ray r = { {0, 5, 0}, {0, 1, 0} };
        int hit = intersect_plane(&pl, r, 1e-4, &t, &point, &normal);
        check(!hit, "plane behind the ray is a miss");
    }

    /* 10. A ray descending onto the plane hits it, and the normal is turned to
     *     face back toward the ray. */
    {
        Ray r = { {0, 5, 0}, {0, -1, 0} };
        int hit = intersect_plane(&pl, r, 1e-4, &t, &point, &normal);
        check(hit, "ray descending onto plane hits it");
        check(fabs(t - 5.0) < 1e-9, "plane hit distance is 5");
        check(fabs(normal.y - 1.0) < 1e-9, "plane normal faces back toward the ray (+y)");
        check(vec3_dot(normal, r.dir) < 0.0, "plane normal opposes the ray direction");
    }

    if (failures == 0) {
        printf("ALL UNIT CHECKS PASSED\n");
        return 0;
    }
    printf("%d UNIT CHECK(S) FAILED\n", failures);
    return 1;
}
