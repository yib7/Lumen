#include <stdio.h>
#include <math.h>
#include "vector.h"
#include "ray.h"
#include "light.h"

/*
* Function, shadow_test, that determines if an object's shadow should be considered
* when calculating the color of the pixel
*/

static int shadow_test(VP_T int_pt, SCENE_T scene, OBJ_T *closest_object) {
   RAY_T shadow_ray;
   shadow_ray.dir = vp_subtract(scene.light.loc, int_pt);
   normalize(&shadow_ray.dir);
   shadow_ray.origin.x = int_pt.x;
   shadow_ray.origin.y = int_pt.y;
   shadow_ray.origin.z = int_pt.z;

   for(OBJ_T *curr = scene.objs; curr != NULL; curr = curr->next){
      double dummyt;
      VP_T dummy_int_pt;
      VP_T dummy_normal;

      if(curr->type == 'p') {
         continue;
      }

      if(curr->intersect(shadow_ray, curr, &dummyt, &dummy_int_pt, &dummy_normal) == 1){
         return 1;
      }
   }

   return 0;
}

/*
* Function, illuminate, that determines the intensity of the color 
* specifics at a given pixel and returns it
*/

COLOR_T illuminate(RAY_T ray, VP_T int_pt, SCENE_T scene, VP_T normal, OBJ_T closest_object){
   // Ambient

   OBJ_T *object = &closest_object;

   if(object->checker == 1 && ((int) floor (int_pt.x) + (int) floor (int_pt.y) + (int) floor (int_pt.z) & 1)) {
      object->color1 = object->color2;         
   }
   
   COLOR_T color;
   color.r = 0.1 * object->color1.r;
   color.g = 0.1 * object->color1.g;
   color.b = 0.1 * object->color1.b;

   if(shadow_test(int_pt, scene, object) == 1) {
      return color;
   }

   // Diffuse

   VP_T light_vector;
   light_vector.x = scene.light.loc.x - int_pt.x;
   light_vector.y = scene.light.loc.y - int_pt.y;
   light_vector.z = scene.light.loc.z - int_pt.z;

   double dl = len(light_vector);
   double atten = 1/(.002 * (dl * dl) + .02 * dl + .2);

   normalize(&light_vector);
   normalize(&normal);
   double dp = dot(light_vector, normal);

   if(dp > 0) {
      color.r += dp * object->color1.r * atten;
      color.g += dp * object->color1.g * atten;
      color.b += dp * object->color1.b * atten;

      // Specular

      VP_T reflect_ray;
      reflect_ray.x = light_vector.x - normal.x * 2 * dp;
      reflect_ray.y = light_vector.y - normal.y * 2 * dp;
      reflect_ray.z = light_vector.z - normal.z * 2 * dp;
      normalize(&reflect_ray);
      double dp2 = dot(reflect_ray, ray.dir);
      
      if(dp2 > 0) {
         color.r += pow(dp2, 200) * atten;
         color.g += pow(dp2, 200) * atten;
         color.b += pow(dp2, 200) * atten;
      }
   }

   if(color.r > 1) {
      color.r = 1;
   }
   if(color.g > 1){
      color.g = 1;
   }
   if(color.b > 1){
      color.b = 1;
   }
   return color;
}

