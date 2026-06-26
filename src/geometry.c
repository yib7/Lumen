#include <math.h>
#include "geometry.h"

/*
 * Ray/sphere intersection. With a normalized ray direction the quadratic
 * coefficient a is 1, so we solve t^2 + b t + c = 0 and take the nearest
 * root that is in front of the ray.
 */
int intersect_sphere(const Sphere *s, Ray ray, double t_min,
                     double *t, Vec3 *point, Vec3 *normal) {
    Vec3 oc = vec3_sub(ray.origin, s->center);
    double b = 2.0 * vec3_dot(ray.dir, oc);
    double c = vec3_dot(oc, oc) - s->radius * s->radius;
    double disc = b * b - 4.0 * c;

    if (disc < 0.0) {
        return 0;
    }

    double sqrt_disc = sqrt(disc);
    double t_near = (-b - sqrt_disc) * 0.5;
    double t_far  = (-b + sqrt_disc) * 0.5;

    double hit_t = t_near;
    if (hit_t <= t_min) {
        hit_t = t_far;       /* inside the sphere: use the exit point */
    }
    if (hit_t <= t_min) {
        return 0;
    }

    *t = hit_t;
    *point = vec3_add(ray.origin, vec3_scale(ray.dir, hit_t));
    *normal = vec3_normalize(vec3_sub(*point, s->center));
    return 1;
}

/*
 * Ray/plane intersection. The plane is the set of points where
 * dot(normal, P) + distance = 0.
 */
int intersect_plane(const Plane *p, Ray ray, double t_min,
                    double *t, Vec3 *point, Vec3 *normal) {
    double denom = vec3_dot(p->normal, ray.dir);
    if (fabs(denom) < 1e-9) {
        return 0;            /* ray parallel to the plane */
    }

    double hit_t = -(vec3_dot(p->normal, ray.origin) + p->distance) / denom;
    if (hit_t <= t_min) {
        return 0;
    }

    *t = hit_t;
    *point = vec3_add(ray.origin, vec3_scale(ray.dir, hit_t));
    /* Face the normal back toward the ray so shading is consistent. */
    *normal = (denom < 0.0) ? p->normal : vec3_scale(p->normal, -1.0);
    return 1;
}

/*
 * Ray/axis-aligned-box intersection using the slab method. The outward
 * normal is derived from which slab produced the entry point.
 */
int intersect_box(const Box *b, Ray ray, double t_min,
                  double *t, Vec3 *point, Vec3 *normal) {
    double tmin = -INFINITY;
    double tmax = INFINITY;
    int axis_min = 0;

    const double origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
    const double dir[3]    = {ray.dir.x, ray.dir.y, ray.dir.z};
    const double lo[3]     = {b->min.x, b->min.y, b->min.z};
    const double hi[3]     = {b->max.x, b->max.y, b->max.z};

    for (int axis = 0; axis < 3; axis++) {
        if (fabs(dir[axis]) < 1e-9) {
            if (origin[axis] < lo[axis] || origin[axis] > hi[axis]) {
                return 0;
            }
            continue;
        }
        double inv = 1.0 / dir[axis];
        double t1 = (lo[axis] - origin[axis]) * inv;
        double t2 = (hi[axis] - origin[axis]) * inv;
        if (t1 > t2) {
            double tmp = t1; t1 = t2; t2 = tmp;
        }
        if (t1 > tmin) {
            tmin = t1;
            axis_min = axis;
        }
        if (t2 < tmax) {
            tmax = t2;
        }
        if (tmin > tmax) {
            return 0;
        }
    }

    double hit_t = tmin;
    if (hit_t <= t_min) {
        hit_t = tmax;        /* ray starts inside the box */
    }
    if (hit_t <= t_min) {
        return 0;
    }

    *t = hit_t;
    *point = vec3_add(ray.origin, vec3_scale(ray.dir, hit_t));

    Vec3 n = {0, 0, 0};
    double sign = (dir[axis_min] < 0.0) ? 1.0 : -1.0;
    if (axis_min == 0)      n.x = sign;
    else if (axis_min == 1) n.y = sign;
    else                    n.z = sign;
    *normal = n;
    return 1;
}
