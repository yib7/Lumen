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

    if (failures == 0) {
        printf("ALL UNIT CHECKS PASSED\n");
        return 0;
    }
    printf("%d UNIT CHECK(S) FAILED\n", failures);
    return 1;
}
