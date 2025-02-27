#ifndef PLANE_H
#define PLANE_H

#include "ray.h"

int intersect_plane(RAY_T ray, OBJ_T *p_obj, double *t, VP_T *int_pt, VP_T *normal);

#endif /* PLANE_H */