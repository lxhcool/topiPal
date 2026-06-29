#include "Live2DBridge.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: TestLive2DInfo model.moc3\n");
        return 2;
    }

    TopiLive2DModel *model = topi_live2d_create(argv[1]);
    if (!model) {
        return 1;
    }

    printf("parameters=%d\n", topi_live2d_parameter_count(model));
    printf("offscreen=%d\n", topi_live2d_offscreen_count(model));
    printf("vertices=%u\n", topi_live2d_vertex_count(model));
    printf("commands=%u\n", topi_live2d_command_count(model));
    const TopiLive2DCommand *commands = topi_live2d_commands(model);
    const uint16_t *indices = topi_live2d_indices(model);
    unsigned int invalid = 0;
    for (unsigned int c = 0; c < topi_live2d_command_count(model); c++) {
        for (unsigned int i = 0; i < commands[c].indexCount; i++) {
            if (indices[commands[c].indexOffset + i] >= commands[c].vertexCount) {
                invalid++;
            }
        }
    }
    printf("invalidLocalIndices=%u\n", invalid);
    topi_live2d_dispose(model);
    return 0;
}
