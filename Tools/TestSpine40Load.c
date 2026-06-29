#include "SpineBridge40.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: TestSpine40Load atlas skel\n");
        return 2;
    }

    TopiSpine40Drawable *drawable = topi_spine40_create(argv[1], argv[2], 1.0f);
    if (!drawable) {
        return 1;
    }

    int count = topi_spine40_animation_count(drawable);
    printf("animations=%d\n", count);
    for (int i = 0; i < count; i++) {
        const char *name = topi_spine40_animation_name(drawable, i);
        printf("%s\n", name ? name : "");
    }
    topi_spine40_dispose(drawable);
    return 0;
}
