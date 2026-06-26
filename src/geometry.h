#ifndef LUMEN_GEOMETRY_H
#define LUMEN_GEOMETRY_H

#include "scene.h"

/* Per-primitive intersection tests. Each returns 1 on a hit in front of the
 * ray (t > t_min) and writes the hit distance, point and outward normal. */

int intersect_sphere(const Sphere *s, Ray ray, double t_min,
                     double *t, Vec3 *point, Vec3 *normal);

int intersect_plane(const Plane *p, Ray ray, double t_min,
                    double *t, Vec3 *point, Vec3 *normal);

int intersect_box(const Box *b, Ray ray, double t_min,
                  double *t, Vec3 *point, Vec3 *normal);

#endif /* LUMEN_GEOMETRY_H */
