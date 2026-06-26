#include <stdio.h>
#include <string.h>
#include "image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

/* Case-insensitive check that `path` ends with `.png`. */
static int ends_with_png(const char *path) {
    size_t len = strlen(path);
    if (len < 4) {
        return 0;
    }
    const char *ext = path + len - 4;
    return (ext[0] == '.' &&
            (ext[1] == 'p' || ext[1] == 'P') &&
            (ext[2] == 'n' || ext[2] == 'N') &&
            (ext[3] == 'g' || ext[3] == 'G'));
}

static int write_ppm(const char *path, const unsigned char *pixels,
                     int width, int height) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return 0;
    }
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    size_t count = (size_t)width * height * 3;
    size_t written = fwrite(pixels, 1, count, f);
    fclose(f);
    return written == count;
}

int image_write(const char *path, const unsigned char *pixels,
                int width, int height) {
    if (ends_with_png(path)) {
        /* stride = width*3 for a tightly packed RGB buffer */
        return stbi_write_png(path, width, height, 3, pixels, width * 3) != 0;
    }
    return write_ppm(path, pixels, width, height);
}
