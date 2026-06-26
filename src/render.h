#ifndef LUMEN_RENDER_H
#define LUMEN_RENDER_H

#include "scene.h"

typedef struct {
    int width;
    int height;
    int samples;    /* supersampling: NxN rays per pixel (samples^2 total) */
    int max_depth;  /* reflection recursion limit */
    int threads;    /* worker threads; 0 = use the OpenMP default */
} RenderConfig;

/* Renders the scene into an RGB8 buffer (width*height*3 bytes, row-major,
 * top-left origin). Caller owns the returned buffer and must free it.
 * Returns NULL on allocation failure. */
unsigned char *render_scene(const Scene *scene, const RenderConfig *cfg);

#endif /* LUMEN_RENDER_H */
