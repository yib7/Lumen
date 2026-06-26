#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/*
 * Scene file format (one directive per line, '#' starts a comment):
 *
 *   camera  px py pz fov
 *   sky     topR topG topB  botR botG botB
 *   light   x y z  r g b  intensity
 *   sphere  cx cy cz radius  r g b           [reflect reflectivity shininess]
 *   plane   nx ny nz dist  r1 g1 b1  r2 g2 b2 [reflect reflectivity shininess]
 *   box     minx miny minz  maxx maxy maxz  r g b [reflect reflectivity shininess]
 *
 * The optional trailing "reflect <reflectivity> <shininess>" switches a
 * surface to the reflective material model. Without it a surface is matte.
 */

#define MAX_TOKENS 32
#define LINE_BUF   512

/* Splits `line` in place into whitespace-separated tokens. Returns count. */
static int tokenize(char *line, char *tokens[], int max) {
    int count = 0;
    char *p = line;
    while (*p && count < max) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            p++;
        }
        if (*p == '\0' || *p == '#') {
            break;       /* end of line or start of comment */
        }
        tokens[count++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
            p++;
        }
        if (*p) {
            *p++ = '\0';
        }
    }
    return count;
}

/* Parses one double, reporting non-numeric tokens. Returns 1 on success. */
static int parse_double(const char *tok, double *out) {
    char *end;
    *out = strtod(tok, &end);
    return end != tok && *end == '\0';
}

#define FAIL(...) do { snprintf(err, err_size, __VA_ARGS__); return 0; } while (0)

/* Reads `n` doubles starting at tokens[start] into out[]. */
static int read_doubles(char *tokens[], int start, int n, double *out,
                        int line_no, char *err, int err_size) {
    for (int i = 0; i < n; i++) {
        if (!parse_double(tokens[start + i], &out[i])) {
            FAIL("line %d: '%s' is not a number", line_no, tokens[start + i]);
        }
    }
    return 1;
}

/* Detects an optional "reflect <r> <s>" suffix and fills the material. */
static int apply_reflect(char *tokens[], int count, int expected,
                         Material *mat, int line_no, char *err, int err_size) {
    mat->kind = MAT_DIFFUSE;
    mat->reflectivity = 0.0;
    mat->shininess = 60.0;

    if (count == expected) {
        return 1;        /* no reflect suffix */
    }
    if (count == expected + 3 && strcmp(tokens[expected], "reflect") == 0) {
        double vals[2];
        if (!read_doubles(tokens, expected + 1, 2, vals, line_no, err, err_size)) {
            return 0;
        }
        mat->kind = MAT_REFLECTIVE;
        mat->reflectivity = vals[0];
        mat->shininess = vals[1];
        return 1;
    }
    FAIL("line %d: unexpected extra tokens (got %d)", line_no, count);
}

static Object *new_object(void) {
    Object *o = calloc(1, sizeof(Object));
    return o;
}

int parse_scene_file(const char *path, Scene *scene,
                     char *err, int err_size) {
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(err, err_size, "cannot open scene file '%s'", path);
        return 0;
    }

    char line[LINE_BUF];
    char *tokens[MAX_TOKENS];
    int line_no = 0;
    int object_count = 0;

    while (fgets(line, sizeof(line), f)) {
        line_no++;
        int count = tokenize(line, tokens, MAX_TOKENS);
        if (count == 0) {
            continue;    /* blank or comment line */
        }

        const char *kw = tokens[0];

        if (strcmp(kw, "camera") == 0) {
            double v[4];
            if (count != 5) FAIL("line %d: camera needs 4 numbers", line_no);
            if (!read_doubles(tokens, 1, 4, v, line_no, err, err_size)) goto fail;
            scene->camera.position = vec3(v[0], v[1], v[2]);
            scene->camera.fov_scale = v[3];

        } else if (strcmp(kw, "sky") == 0) {
            double v[6];
            if (count != 7) FAIL("line %d: sky needs 6 numbers", line_no);
            if (!read_doubles(tokens, 1, 6, v, line_no, err, err_size)) goto fail;
            scene->sky_top = vec3(v[0], v[1], v[2]);
            scene->sky_bottom = vec3(v[3], v[4], v[5]);

        } else if (strcmp(kw, "light") == 0) {
            double v[7];
            if (count != 8) FAIL("line %d: light needs 7 numbers", line_no);
            if (scene->light_count >= MAX_LIGHTS)
                FAIL("line %d: too many lights (max %d)", line_no, MAX_LIGHTS);
            if (!read_doubles(tokens, 1, 7, v, line_no, err, err_size)) goto fail;
            Light *l = &scene->lights[scene->light_count++];
            l->position = vec3(v[0], v[1], v[2]);
            l->color = vec3(v[3], v[4], v[5]);
            l->intensity = v[6];

        } else if (strcmp(kw, "sphere") == 0) {
            double v[7];
            if (count < 8) FAIL("line %d: sphere needs at least 7 numbers", line_no);
            if (!read_doubles(tokens, 1, 7, v, line_no, err, err_size)) goto fail;
            Object *o = new_object();
            if (!o) FAIL("out of memory");
            o->kind = OBJ_SPHERE;
            o->sphere.center = vec3(v[0], v[1], v[2]);
            o->sphere.radius = v[3];
            o->material.albedo = vec3(v[4], v[5], v[6]);
            o->material.checker = 0;
            if (!apply_reflect(tokens, count, 8, &o->material, line_no, err, err_size)) {
                free(o); goto fail;
            }
            scene_add_object(scene, o);
            object_count++;

        } else if (strcmp(kw, "plane") == 0) {
            double v[10];
            if (count < 11) FAIL("line %d: plane needs at least 10 numbers", line_no);
            if (!read_doubles(tokens, 1, 10, v, line_no, err, err_size)) goto fail;
            Object *o = new_object();
            if (!o) FAIL("out of memory");
            o->kind = OBJ_PLANE;
            o->plane.normal = vec3_normalize(vec3(v[0], v[1], v[2]));
            o->plane.distance = v[3];
            o->material.albedo = vec3(v[4], v[5], v[6]);
            o->material.albedo2 = vec3(v[7], v[8], v[9]);
            o->material.checker = 1;
            if (!apply_reflect(tokens, count, 11, &o->material, line_no, err, err_size)) {
                free(o); goto fail;
            }
            scene_add_object(scene, o);
            object_count++;

        } else if (strcmp(kw, "box") == 0) {
            double v[9];
            if (count < 10) FAIL("line %d: box needs at least 9 numbers", line_no);
            if (!read_doubles(tokens, 1, 9, v, line_no, err, err_size)) goto fail;
            Object *o = new_object();
            if (!o) FAIL("out of memory");
            o->kind = OBJ_BOX;
            o->box.min = vec3(v[0], v[1], v[2]);
            o->box.max = vec3(v[3], v[4], v[5]);
            o->material.albedo = vec3(v[6], v[7], v[8]);
            o->material.checker = 0;
            if (!apply_reflect(tokens, count, 10, &o->material, line_no, err, err_size)) {
                free(o); goto fail;
            }
            scene_add_object(scene, o);
            object_count++;

        } else {
            FAIL("line %d: unknown directive '%s'", line_no, kw);
        }
    }

    fclose(f);

    if (object_count == 0) {
        snprintf(err, err_size, "scene '%s' contains no objects", path);
        return 0;
    }
    if (scene->light_count == 0) {
        /* A scene with no light renders black; warn but keep a default. */
        scene->lights[0].position = vec3(-10, 10, -5);
        scene->lights[0].color = vec3(1, 1, 1);
        scene->lights[0].intensity = 1.0;
        scene->light_count = 1;
    }
    return 1;

fail:
    fclose(f);
    return 0;
}
