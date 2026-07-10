#include <stdlib.h>
#include "scene.h"
#include "geometry.h"

void scene_init(Scene *scene) {
    scene->objects = NULL;
    scene->light_count = 0;
    scene->sky_top = vec3(0.45, 0.62, 0.95);
    scene->sky_bottom = vec3(0.85, 0.90, 1.0);
    scene->camera.position = vec3(0, 0, 0);
    scene->camera.fov_scale = 0.5;
    scene->camera.target = vec3(0, 0, 0);
    scene->camera.has_look = 0;
}

/* Prepend to keep insertion O(1); draw order does not matter for a raytracer. */
void scene_add_object(Scene *scene, Object *obj) {
    obj->next = scene->objects;
    scene->objects = obj;
}

void scene_free(Scene *scene) {
    Object *curr = scene->objects;
    while (curr != NULL) {
        Object *next = curr->next;
        free(curr);
        curr = next;
    }
    scene->objects = NULL;
}

/* Dispatch one object's intersection test based on its kind. */
static int object_intersect(const Object *obj, Ray ray, double t_min,
                            double *t, Vec3 *point, Vec3 *normal) {
    switch (obj->kind) {
        case OBJ_SPHERE:
            return intersect_sphere(&obj->sphere, ray, t_min, t, point, normal);
        case OBJ_PLANE:
            return intersect_plane(&obj->plane, ray, t_min, t, point, normal);
        case OBJ_BOX:
            return intersect_box(&obj->box, ray, t_min, t, point, normal);
        default:
            return 0;
    }
}

int scene_intersect(const Scene *scene, Ray ray, double t_min, Hit *hit) {
    int found = 0;
    double closest = 1e30;

    for (Object *curr = scene->objects; curr != NULL; curr = curr->next) {
        double t;
        Vec3 point, normal;
        if (object_intersect(curr, ray, t_min, &t, &point, &normal)) {
            if (t < closest) {
                closest = t;
                hit->t = t;
                hit->point = point;
                hit->normal = normal;
                hit->object = curr;
                found = 1;
            }
        }
    }
    return found;
}

int scene_intersect_any(const Scene *scene, Ray ray, double t_min, double t_max) {
    for (Object *curr = scene->objects; curr != NULL; curr = curr->next) {
        double t;
        Vec3 point, normal;
        if (object_intersect(curr, ray, t_min, &t, &point, &normal) && t < t_max) {
            return 1;   /* first blocker before t_max is enough */
        }
    }
    return 0;
}
