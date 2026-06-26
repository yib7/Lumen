#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "scene.h"
#include "parser.h"
#include "render.h"
#include "image.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#define LUMEN_VERSION "1.0.0"

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

/* Parses an integer option value, exiting with a message on bad input. */
static int parse_int_arg(const char *flag, const char *value) {
    char *end;
    long n = strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        fprintf(stderr, "error: %s expects an integer, got '%s'\n", flag, value);
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
            cfg.width = parse_int_arg(arg, argv[++i]);
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--height") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.height = parse_int_arg(arg, argv[++i]);
        } else if (strcmp(arg, "-a") == 0 || strcmp(arg, "--samples") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.samples = parse_int_arg(arg, argv[++i]);
        } else if (strcmp(arg, "-d") == 0 || strcmp(arg, "--depth") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.max_depth = parse_int_arg(arg, argv[++i]);
        } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--threads") == 0) {
            if (!needs_value) { fprintf(stderr, "error: %s needs a value\n", arg); return 1; }
            cfg.threads = parse_int_arg(arg, argv[++i]);
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

    clock_t start = clock();
    unsigned char *pixels = render_scene(&scene, &cfg);
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;

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
