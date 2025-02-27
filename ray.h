#ifndef RAY_H
#define RAY_H
#include "vector.h"

#define NUM_OBJS 3

typedef struct {
   double r;
   double g;
   double b;
} COLOR_T;

typedef struct {
   VP_T center;
   double radius;
} SPHERE_T;

typedef struct {
   VP_T origin;
   VP_T dir;
} RAY_T;

typedef struct {
   VP_T loc;
} LIGHT_T;

typedef struct {
   VP_T normal;
   double distance;
} PLANE_T;

typedef struct OBJ {
   union {
      SPHERE_T sphere;
      PLANE_T plane;
   };
   struct OBJ *next;
   char type;
   COLOR_T color1;
   int checker;
   COLOR_T color2;
   int (*intersect) (RAY_T ray, struct OBJ *obj, double *t, VP_T *int_pt, VP_T *normal);
} OBJ_T;

typedef struct {
   OBJ_T *objs;
   LIGHT_T light; 
   double start_x;       
   double start_y;     
   double pix_size;   
} SCENE_T;

#endif /* RAY.H */