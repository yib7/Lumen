#include <stdio.h>
#include <math.h>
#include "vector.h"
#include "ray.h"
#include "plane.h"

/* 
* Function, intersect_plane, that takes a ray and checks if it intersects
* with an object plane. If it intersects the sphere, then it creates an intersection 
* point vector and normal vector and t value.
*/

int intersect_plane(RAY_T ray, OBJ_T *p_obj, double *t, VP_T *int_pt, VP_T *normal){
   double dp = dot(p_obj->plane.normal, ray.dir);
   if(dp == 0) {
      return 0;
   }
   else {
      *t = -(dot(p_obj->plane.normal, ray.origin) + p_obj->plane.distance)/ dp;
      if(*t <= 0) {
         return 0;
      }
      else {
         int_pt->x = ray.origin.x + (*t) * ray.dir.x;
         int_pt->y = ray.origin.y + (*t) * ray.dir.y;
         int_pt->z = ray.origin.z + (*t) * ray.dir.z;

         normal->x = p_obj->plane.normal.x;
         normal->y = p_obj->plane.normal.y;
         normal->z = p_obj->plane.normal.z;

         return 1;
      }
   }
}
