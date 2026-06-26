#ifndef LUMEN_SCENE_H
#define LUMEN_SCENE_H

#include "vec3.h"

/* RGB color in linear [0,1] space, stored as a vector. */
typedef Vec3 Color;

typedef struct {
    Vec3 origin;
    Vec3 dir;       /* expected to be normalized */
} Ray;

/* Surface shading model for an object. */
typedef enum {
    MAT_DIFFUSE,     /* matte: ambient + diffuse + specular highlight */
    MAT_REFLECTIVE   /* diffuse base plus a mirror reflection term */
} MaterialKind;

typedef struct {
    MaterialKind kind;
    Color albedo;        /* base color */
    Color albedo2;       /* second color for checkerboards (planes only) */
    int   checker;       /* 1 = use a checkerboard of albedo/albedo2 */
    double reflectivity; /* 0..1 blend toward the reflected color */
    double shininess;    /* specular exponent */
} Material;

typedef enum {
    OBJ_SPHERE,
    OBJ_PLANE,
    OBJ_BOX
} ObjectKind;

typedef struct {
    Vec3   center;
    double radius;
} Sphere;

typedef struct {
    Vec3   normal;   /* unit normal */
    double distance; /* signed distance from origin along the normal */
} Plane;

typedef struct {
    Vec3 min;        /* axis-aligned bounding corners */
    Vec3 max;
} Box;

typedef struct Object {
    ObjectKind kind;
    Material   material;
    union {
        Sphere sphere;
        Plane  plane;
        Box    box;
    };
    struct Object *next;
} Object;

typedef struct {
    Vec3   position;
    Color  color;
    double intensity;
} Light;

/* Camera describes where rays originate and the image plane geometry. */
typedef struct {
    Vec3   position;
    double fov_scale;   /* half-height of the image plane at z = 1 */
} Camera;

#define MAX_LIGHTS 8

typedef struct {
    Object *objects;
    Light   lights[MAX_LIGHTS];
    int     light_count;
    Color   sky_top;     /* gradient background, top of frame */
    Color   sky_bottom;  /* gradient background, bottom of frame */
    Camera  camera;
} Scene;

/* Records the nearest surface a ray hit. */
typedef struct {
    double  t;
    Vec3    point;
    Vec3    normal;
    Object *object;
} Hit;

void scene_init(Scene *scene);
void scene_add_object(Scene *scene, Object *obj);
void scene_free(Scene *scene);

/* Returns 1 and fills `hit` with the closest intersection, else 0.
 * `t_min` lets shadow/reflection rays skip the surface they start on. */
int scene_intersect(const Scene *scene, Ray ray, double t_min, Hit *hit);

#endif /* LUMEN_SCENE_H */
