#ifndef LUMEN_IMAGE_H
#define LUMEN_IMAGE_H

/* Writes an RGB8 buffer (width*height*3 bytes) to disk.
 * Returns 1 on success, 0 on failure. Format is chosen from the file
 * extension: ".png" uses stb_image_write, anything else writes binary PPM. */
int image_write(const char *path, const unsigned char *pixels,
                int width, int height);

#endif /* LUMEN_IMAGE_H */
