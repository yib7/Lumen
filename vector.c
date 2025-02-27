#include <stdio.h>
#include <math.h>
#include "vector.h"

// Function, len, that determines the magnitude of a vector 

double len(VP_T vector) {
   double length = (vector.x * vector.x) + 
                   (vector.y * vector.y) +
                   (vector.z * vector.z);
   length = sqrt(length);
   return length;
}

// Function, normalize, that normalizes a vector 

void normalize(VP_T *vector) {
   double length = len(*vector);
   vector->x /= length;
   vector->y /= length;
   vector->z /= length;
}

// Function, dot, that computes the dot product of two vectors 

double dot(VP_T vector_1, VP_T vector_2) {
   double dp;
   dp = (vector_1.x * vector_2.x) +
        (vector_1.y * vector_2.y) +
        (vector_1.z * vector_2.z);
   return dp;    
}

// Function, vp_subtract, that computes the difference of two vectors

VP_T vp_subtract(VP_T vector_1, VP_T vector_2) {
   VP_T difference;

   difference.x = vector_1.x - vector_2.x;
   difference.y = vector_1.y - vector_2.y;
   difference.z = vector_1.z - vector_2.z;

   return difference;
   
}