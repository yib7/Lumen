#ifndef LUMEN_PARSER_H
#define LUMEN_PARSER_H

#include "scene.h"

/* Loads a scene description file into `scene` (already scene_init'd).
 * On success returns 1. On failure returns 0 and writes a message that
 * names the line number and what went wrong into `err` (size `err_size`). */
int parse_scene_file(const char *path, Scene *scene,
                     char *err, int err_size);

#endif /* LUMEN_PARSER_H */
