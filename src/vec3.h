#ifndef LUMEN_VEC3_H
#define LUMEN_VEC3_H

/*
 * 3D vector math used everywhere in the renderer: positions, directions,
 * normals and RGB colors are all stored as Vec3. Functions are small and
 * pass/return by value so they inline cleanly under -O2.
 */

typedef struct {
    double x;
    double y;
    double z;
} Vec3;

Vec3   vec3(double x, double y, double z);
Vec3   vec3_add(Vec3 a, Vec3 b);
Vec3   vec3_sub(Vec3 a, Vec3 b);
Vec3   vec3_scale(Vec3 a, double s);
Vec3   vec3_mul(Vec3 a, Vec3 b);        /* component-wise, used for color */
double vec3_dot(Vec3 a, Vec3 b);
double vec3_len(Vec3 a);
Vec3   vec3_normalize(Vec3 a);
Vec3   vec3_reflect(Vec3 incident, Vec3 normal);
Vec3   vec3_cross(Vec3 a, Vec3 b);
Vec3   vec3_lerp(Vec3 a, Vec3 b, double t);

#endif /* LUMEN_VEC3_H */
