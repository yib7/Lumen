#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
   double x;
   double y;
   double z;
} VP_T;

double len(VP_T vector);

void normalize(VP_T *vector);

double dot(VP_T vector_1, VP_T vector_2);

VP_T vp_subtract(VP_T vector_1, VP_T vector_2);

#endif /* VECTOR_H */