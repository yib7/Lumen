#ifndef LIGHT_H
#define LIGHT_H

#include "ray.h"

COLOR_T illuminate(RAY_T ray, VP_T int_pt, SCENE_T scene, VP_T normal, OBJ_T closet_object);

#endif /* LIGHT_H */