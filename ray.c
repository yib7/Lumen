/*
* A program that reads a .txt file of spheres, plane, and light objects
* and creates a ppm file that displays a raytraced visual of the objects in
* a 640 x 480 aspect ratio.
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ray.h"
#include "vector.h"
#include "sphere.h"
#include "light.h"
#include "plane.h"

int init(SCENE_T *scene) {
   FILE *file = fopen("scene.txt", "r");
    if (!file) {
        fprintf(stderr, "Error opening file.\n");
        return 0;
    }

   OBJ_T *node;
   scene->objs = NULL;

   char type;

   while (fscanf(file, " %c", &type) != EOF) {
      if (type == 's') {
         node = (OBJ_T *) malloc (sizeof(OBJ_T));
         node->type = type;
         node->sphere = (SPHERE_T){{0, 0, 0}, 0};
         node->color1 = (COLOR_T){0, 0, 0};
         node->checker = 0;
         node->intersect = intersect_sphere;

         fscanf(file, "%lf %lf %lf", &node->sphere.center.x, &node->sphere.center.y, &node->sphere.center.z);
         fscanf(file, "%lf", &node->sphere.radius);
         fscanf(file, "%lf %lf %lf", &node->color1.r, &node->color1.g, &node->color1.b);

         node->next = scene->objs;
         scene->objs = node;
      }
      else if (type == 'p') {
         node = (OBJ_T *) malloc (sizeof(OBJ_T));
         node->type = type;
         node->plane = (PLANE_T){{0, 0, 0}, 0};
         node->color1 = (COLOR_T){0, 0, 0};
         node->color2 = (COLOR_T){0, 0, 0};
         node->checker = 1;
         node->intersect = intersect_plane;

         fscanf(file, "%lf %lf %lf", &node->plane.normal.x, &node->plane.normal.y, &node->plane.normal.z);
         fscanf(file, "%lf", &node->plane.distance);
         fscanf(file, "%lf %lf %lf", &node->color1.r, &node->color1.g, &node->color1.b);
         fscanf(file, "%lf %lf %lf", &node->color2.r, &node->color2.g, &node->color2.b);

         node->next = scene->objs;
         scene->objs = node;
      }
      else if(type == 'l'){
         fscanf(file, "%lf %lf %lf", &scene->light.loc.x, &scene->light.loc.y, &scene->light.loc.z);
      }
   }

   fclose(file);

   return 1;
}

/*
* Function, trace, that takes the scene and checks if a ray intersects through 
* each object in a list. If so, runs the illuminate function to determine the color of the pixel. 
* If not, returns a light blue color for the pixel to represent the background.
*/

COLOR_T trace(RAY_T ray, SCENE_T scene) {
   COLOR_T color;
   double closest_t;
   VP_T closest_int_pt;
   VP_T closest_normal;
   OBJ_T *closest_object;
   
   int index = 0;

   closest_t = 100000000;

   for(OBJ_T *curr = scene.objs; curr != NULL; curr = curr->next){
      double t;
      VP_T int_pt;
      VP_T normal;
      if(curr->intersect(ray, curr, &t, &int_pt, &normal) == 1) {
         if(t < closest_t){
            closest_t = t;
            closest_int_pt = int_pt;
            closest_normal = normal;
            closest_object = curr;
         }
      }
      index++;
   }

   if(closest_t != 100000000){
      color = illuminate(ray, closest_int_pt, scene, closest_normal, *closest_object);
   }

   else {
      color.r = 0.3;
      color.g = 0.3;
      color.b = 0.5;
   }

   return color;
}

/*
* Initalizes the scene and its necessary attributes and the header for the 640 x 480 
* ppm file. Runs through a loop where each iteration calls trace to determine 
* color of the relevant pixel and writes it to the ppm file to create 
* a raytraced image.
*/

int main(void) 
{
   SCENE_T scene;

   int result = init(&scene);

   if (result == 0) {
        fprintf(stderr, "Failed to initialize scene.\n");
        return 0;
    }

   int width = 640;
   int height = 480;

   FILE *file = fopen("img.ppm", "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 0;
   }
   
   fprintf(file, "P6\n%d %d\n255\n", width, height);

   scene.pix_size = 1/480.0;
   scene.start_y = 0.5;
   scene.start_x = -(640/480.0)/2;

   for(int y = 0; y < height; y++) {
      for(int x = 0; x < width; x++) {
         RAY_T ray = {{0, 0, 0}, {scene.start_x + x * scene.pix_size, scene.start_y - y * scene.pix_size, 1}};
         normalize(&ray.dir);
         COLOR_T pixel = trace(ray, scene);
         fprintf(file, "%c%c%c", (unsigned char)(pixel.r * 255), 
                                 (unsigned char)(pixel.g * 255), 
                                 (unsigned char)(pixel.b * 255));
      }
   }
   fclose(file);
   return 0;
}