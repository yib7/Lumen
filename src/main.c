#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include "scene.h"
#include "parser.h"
#include "render.h"
#include "image.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#define LUMEN_VERSION "2.0.2"

/* CLI upper bounds. Without them --samples/--depth/--threads accept up to
 * INT_MAX and blow up downstream:
 *   MAX_SAMPLES  256 rays/px each way -> 65536 rays/pixel is already absurd,
 *                and samples*samples stays well inside a signed int.
 *   MAX_DEPTH    trace() recurses once per bounce; a huge depth overflows the
 *                stack on a mirror scene.
 *   MAX_THREADS  1024 is far past any real core count; more just asks OpenMP
 *                to spawn thousands of real threads. */
#define MAX_SAMPLES 256
#define MAX_DEPTH   64
#define MAX_THREADS 1024

/* Wall-clock seconds. OpenMP's timer measures elapsed real time on every
 * platform; clock() measures per-process CPU time, which sums every thread and
 * so over-reports a multithreaded render on Linux/macOS. */
static double now_seconds(void) {
#ifdef _OPENMP
    return omp_get_wtime();
#else
    return (double)clock() / CLOCKS_PER_SEC;
#endif
}

static void print_usage(const char *prog) {
    printf(
        "Lumen %s - a CPU raytracer in C\n\n"
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  -s, --scene PATH     scene file to render (default: scenes/solar.scene)\n"
        "  -o, --output PATH    output image; .png or .ppm by extension (default: out.png)\n"
        "  -w, --width N        image width in pixels (default: 800)\n"
        "  -h, --height N       image height in pixels (default: 600)\n"
        "  -a, --samples N      anti-aliasing grid; N*N rays per pixel (default: 2)\n"
        "  -d, --depth N        reflection bounce limit (default: 4)\n"
        "  -t, --threads N      worker threads, 0 = auto (default: 0)\n"
        "      --help           show this message\n",
        LUMEN_VERSION, prog);
}

/* Parses an integer option value, exiting with a message on bad input.
 * Rejects non-numeric input and values outside [min_val, max_val] (including
 * strtol overflow), so a huge dimension can never wrap into a small or
 * negative buffer size downstream, and samples/depth/threads can never grow
 * past a sane ceiling. `min_val` is 1 for dimensions/samples/depth and 0 for
 * --threads (0 = auto); `max_val` is INT_MAX for dimensions and the MAX_*
 * caps for samples/depth/threads. */
static int parse_int_arg(const char *flag, const char *value, int min_val, int max_val) {
    char *end;
    errno = 0;
    long n = strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        fprintf(stderr, "error: %s expects an integer, got '%s'\n", flag, value);
        exit(1);
    }
    if (errno == ERANGE || n < (long)min_val || n > (long)max_val) {
        fprintf(stderr, "error: %s must be between %d and %d, got '%s'\n",
                flag, min_val, max_val, value);
        exit(1);
    }
    return (int)n;
}

int main(int argc, char *argv[]) {
    const char *scene_path = "scenes/solar.scene";
    const char *output_path = "out.png";
    RenderConfig cfg = {
        .width = 800,
        .height = 600,
        .samples = 2,
        .max_depth = 4,
        .threads = 0
    };

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        int needs_value = (i + 1 < argc);

        if (strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--scene") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            scene_path = argv[++i];
        } else if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            output_path = argv[++i];
        } else if (strcmp(arg, "-w") == 0 || strcmp(arg, "--width") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.width = parse_int_arg(arg, argv[++i], 1, INT_MAX);
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--height") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.height = parse_int_arg(arg, argv[++i], 1, INT_MAX);
        } else if (strcmp(arg, "-a") == 0 || strcmp(arg, "--samples") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.samples = parse_int_arg(arg, argv[++i], 1, MAX_SAMPLES);
        } else if (strcmp(arg, "-d") == 0 || strcmp(arg, "--depth") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.max_depth = parse_int_arg(arg, argv[++i], 1, MAX_DEPTH);
        } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--threads") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.threads = parse_int_arg(arg, argv[++i], 0, MAX_THREADS);
        } else {
            fprintf(stderr, "error: unknown option '%s'\n", arg);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (cfg.width < 1 || cfg.height < 1) {
        fprintf(stderr, "error: width and height must be positive\n");
        return 1;
    }

    /* Cap the total pixel count. parse_int_arg already blocks per-dimension
     * overflow, and render.c now allocates in size_t so the buffer size can't
     * wrap; this bound additionally rejects requests that are technically
     * valid but absurd (e.g. 50000x50000 = 7.5 GB) up front, cleanly, instead
     * of attempting a multi-gigabyte allocation and a runaway render.
     *
     * The test is written in division form (width > MAX_PIXELS / height) rather
     * than as a product: the whole point of this guard is to catch an overflow,
     * so the check itself must not be able to overflow. On a 32-bit size_t,
     * width*height for 65536x65536 wraps to 0 and would sail past a product
     * comparison; the division can't wrap. The height>=1 guaranteed by the
     * check immediately above rules out divide-by-zero. */
    #define MAX_PIXELS ((size_t)100 * 1000 * 1000)   /* 100 MP (~300 MB RGB8) */
    if ((size_t)cfg.width > MAX_PIXELS / (size_t)cfg.height) {
        fprintf(stderr,
                "error: image too large (%dx%d = %zu pixels, max %zu)\n",
                cfg.width, cfg.height,
                (size_t)cfg.width * (size_t)cfg.height, (size_t)MAX_PIXELS);
        return 1;
    }
    #undef MAX_PIXELS

    Scene scene;
    scene_init(&scene);

    char err[256];
    if (!parse_scene_file(scene_path, &scene, err, sizeof(err))) {
        fprintf(stderr, "scene error: %s\n", err);
        scene_free(&scene);
        return 1;
    }

    printf("Lumen %s\n", LUMEN_VERSION);
    printf("scene:   %s\n", scene_path);
    printf("output:  %s\n", output_path);
    printf("size:    %dx%d, %d sample(s)/pixel, depth %d\n",
           cfg.width, cfg.height, cfg.samples * cfg.samples, cfg.max_depth);
#ifdef _OPENMP
    int reported = cfg.threads > 0 ? cfg.threads : omp_get_max_threads();
    printf("threads: %d (OpenMP)\n", reported);
#else
    printf("threads: 1 (single-threaded build)\n");
#endif

    double start = now_seconds();
    unsigned char *pixels = render_scene(&scene, &cfg);
    double elapsed = now_seconds() - start;

    if (!pixels) {
        fprintf(stderr, "error: failed to allocate the image buffer\n");
        scene_free(&scene);
        return 1;
    }

    if (!image_write(output_path, pixels, cfg.width, cfg.height)) {
        fprintf(stderr, "error: failed to write '%s'\n", output_path);
        free(pixels);
        scene_free(&scene);
        return 1;
    }

    printf("rendered in %.2fs -> %s\n", elapsed, output_path);

    free(pixels);
    scene_free(&scene);
    return 0;
}
