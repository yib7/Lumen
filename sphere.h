#ifndef SPHERE_H
#define SPHERE_H

#include "ray.h"

int intersect_sphere(RAY_T ray, OBJ_T *s_obj, double *t, VP_T *int_pt, VP_T *normal);

#endif /* SPHERE_H */