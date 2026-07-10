#include <stdio.h>
#include <stdlib.h>
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

/* Write `len` bytes to `path`, removing a partial file on any failure.
 * Returns 1 only if every byte was written AND fclose succeeded. A short
 * write (disk full / quota / I/O error) or a flush failure surfaced by
 * fclose is therefore always reported as failure, and no truncated file is
 * left behind for a caller to mistake for a valid image. */
static int write_all(const char *path, const unsigned char *bytes, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return 0;
    }
    size_t written = fwrite(bytes, 1, len, f);
    int close_ok = (fclose(f) == 0);
    if (written != len || !close_ok) {
        remove(path);
        return 0;
    }
    return 1;
}

/* Encode an RGB8 buffer to a PNG in memory, then write it through the
 * integrity-checked helper. This deliberately avoids stb's stdio wrapper
 * (stbi_write_png), which ignores the results of fwrite and fclose and so
 * can report success after a short write, leaving a truncated PNG on disk. */
static int write_png(const char *path, const unsigned char *pixels,
                     int width, int height) {
    int len = 0;
    unsigned char *png = stbi_write_png_to_mem(pixels, width * 3,
                                               width, height, 3, &len);
    if (!png) {
        return 0;
    }
    int ok = write_all(path, png, (size_t)len);
    STBIW_FREE(png);
    return ok;
}

/* Encode an RGB8 buffer as a binary PPM (P6) into memory, then write it
 * through the same integrity-checked helper as the PNG path so both formats
 * share one failure guarantee. The header ("P6\n%d %d\n255\n") and the
 * width*height*3 body are byte-for-byte identical to the previous streaming
 * implementation. */
static int write_ppm(const char *path, const unsigned char *pixels,
                     int width, int height) {
    char header[64];
    int header_len = snprintf(header, sizeof header, "P6\n%d %d\n255\n",
                              width, height);
    if (header_len < 0 || (size_t)header_len >= sizeof header) {
        return 0;
    }
    size_t body = (size_t)width * (size_t)height * 3;
    size_t total = (size_t)header_len + body;
    unsigned char *buf = malloc(total);
    if (!buf) {
        return 0;
    }
    memcpy(buf, header, (size_t)header_len);
    memcpy(buf + header_len, pixels, body);
    int ok = write_all(path, buf, total);
    free(buf);
    return ok;
}

int image_write(const char *path, const unsigned char *pixels,
                int width, int height) {
    if (ends_with_png(path)) {
        return write_png(path, pixels, width, height);
    }
    return write_ppm(path, pixels, width, height);
}
