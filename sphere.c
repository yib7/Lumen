#include <stdio.h>
#include <math.h>
#include "vector.h"
#include "ray.h"
#include "sphere.h"

/* 
* Function, intersect_sphere, that takes a ray and checks if it intersects
* with an object sphere. If it intersects the sphere, then it creates an intersection 
* point vector and normal vector.
*/

int intersect_sphere(RAY_T ray, OBJ_T *s_obj, double *t, VP_T *int_pt, VP_T *normal) {
   double a = 1;

   double b = 2 * ((ray.dir.x * (ray.origin.x - s_obj->sphere.center.x)) +
                   (ray.dir.y * (ray.origin.y - s_obj->sphere.center.y)) +
                   (ray.dir.z * (ray.origin.z - s_obj->sphere.center.z)) );

   double c = (ray.origin.x - s_obj->sphere.center.x) * (ray.origin.x - s_obj->sphere.center.x) +
              (ray.origin.y - s_obj->sphere.center.y) * (ray.origin.y - s_obj->sphere.center.y) +
              (ray.origin.z - s_obj->sphere.center.z) * (ray.origin.z - s_obj->sphere.center.z) - 
              (s_obj->sphere.radius * s_obj->sphere.radius);

   double t1, t2, discriminant;
   discriminant = (b*b) - (4*a*c);

   if(discriminant > 0) {
      t1 = ((-b) + sqrt(discriminant))/(2*a);
      t2 = ((-b) - sqrt(discriminant))/(2*a);
      if((t1 >= 0) && (t2 >= 0)){
         if(t1 < t2) {
            *t = t1;
         }
         else {
            *t = t2;
         }
         int_pt->x = ray.origin.x + (*t) * ray.dir.x;
         int_pt->y = ray.origin.y + (*t) * ray.dir.y;
         int_pt->z = ray.origin.z + (*t) * ray.dir.z;

         normal->x = int_pt->x - s_obj->sphere.center.x;
         normal->y = int_pt->y - s_obj->sphere.center.y;
         normal->z = int_pt->z - s_obj->sphere.center.z;

         return 1;
      }
   }
   return 0;
}