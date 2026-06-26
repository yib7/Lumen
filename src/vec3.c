#include <math.h>
#include "vec3.h"

Vec3 vec3(double x, double y, double z) {
    Vec3 v = {x, y, z};
    return v;
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 vec3_scale(Vec3 a, double s) {
    return vec3(a.x * s, a.y * s, a.z * s);
}

Vec3 vec3_mul(Vec3 a, Vec3 b) {
    return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

double vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

double vec3_len(Vec3 a) {
    return sqrt(vec3_dot(a, a));
}

Vec3 vec3_normalize(Vec3 a) {
    double len = vec3_len(a);
    if (len == 0.0) {
        return a;
    }
    return vec3_scale(a, 1.0 / len);
}

/* Mirror reflection of an incident direction about a surface normal. */
Vec3 vec3_reflect(Vec3 incident, Vec3 normal) {
    return vec3_sub(incident, vec3_scale(normal, 2.0 * vec3_dot(incident, normal)));
}

Vec3 vec3_lerp(Vec3 a, Vec3 b, double t) {
    return vec3_add(vec3_scale(a, 1.0 - t), vec3_scale(b, t));
}
