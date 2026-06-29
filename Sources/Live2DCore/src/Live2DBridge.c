#include "Live2DBridge.h"
#include "Live2DCubismCore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TopiLive2DModel {
    void *mocMemory;
    unsigned int mocSize;
    csmMoc *moc;
    void *modelMemory;
    unsigned int modelSize;
    csmModel *model;
    TopiLive2DVertex *vertices;
    uint32_t vertexCount;
    uint32_t vertexCapacity;
    uint16_t *indices;
    uint32_t indexCount;
    uint32_t indexCapacity;
    TopiLive2DCommand *commands;
    uint32_t commandCount;
    uint32_t commandCapacity;
};

static void *topi_aligned_alloc(size_t alignment, size_t size) {
    void *memory = NULL;
    if (posix_memalign(&memory, alignment, size) != 0) {
        return NULL;
    }
    memset(memory, 0, size);
    return memory;
}

static void *topi_read_file_aligned(const char *path, size_t alignment, unsigned int *outSize) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length <= 0) {
        fclose(file);
        return NULL;
    }

    void *memory = topi_aligned_alloc(alignment, (size_t)length);
    if (!memory) {
        fclose(file);
        return NULL;
    }

    size_t readLength = fread(memory, 1, (size_t)length, file);
    fclose(file);

    if (readLength != (size_t)length) {
        free(memory);
        return NULL;
    }

    *outSize = (unsigned int)length;
    return memory;
}

static int topi_reserve_vertices(TopiLive2DModel *model, uint32_t count) {
    if (count <= model->vertexCapacity) {
        return 1;
    }
    TopiLive2DVertex *next = (TopiLive2DVertex *)realloc(model->vertices, sizeof(TopiLive2DVertex) * count);
    if (!next) {
        return 0;
    }
    model->vertices = next;
    model->vertexCapacity = count;
    return 1;
}

static int topi_reserve_indices(TopiLive2DModel *model, uint32_t count) {
    if (count <= model->indexCapacity) {
        return 1;
    }
    uint16_t *next = (uint16_t *)realloc(model->indices, sizeof(uint16_t) * count);
    if (!next) {
        return 0;
    }
    model->indices = next;
    model->indexCapacity = count;
    return 1;
}

static int topi_reserve_commands(TopiLive2DModel *model, uint32_t count) {
    if (count <= model->commandCapacity) {
        return 1;
    }
    TopiLive2DCommand *next = (TopiLive2DCommand *)realloc(model->commands, sizeof(TopiLive2DCommand) * count);
    if (!next) {
        return 0;
    }
    model->commands = next;
    model->commandCapacity = count;
    return 1;
}

static void topi_rebuild_draw_buffers(TopiLive2DModel *model) {
    int drawableCount = csmGetDrawableCount(model->model);
    if (drawableCount <= 0) {
        model->vertexCount = 0;
        model->indexCount = 0;
        model->commandCount = 0;
        return;
    }

    const int *renderOrders = csmGetRenderOrders(model->model);
    const csmFlags *dynamicFlags = csmGetDrawableDynamicFlags(model->model);
    const int *textureIndices = csmGetDrawableTextureIndices(model->model);
    const int *blendModes = csmGetDrawableBlendModes(model->model);
    const csmFlags *constantFlags = csmGetDrawableConstantFlags(model->model);
    const float *opacities = csmGetDrawableOpacities(model->model);
    const int *maskCounts = csmGetDrawableMaskCounts(model->model);
    const int **masks = csmGetDrawableMasks(model->model);
    const int *vertexCounts = csmGetDrawableVertexCounts(model->model);
    const csmVector2 **positions = csmGetDrawableVertexPositions(model->model);
    const csmVector2 **uvs = csmGetDrawableVertexUvs(model->model);
    const int *indexCounts = csmGetDrawableIndexCounts(model->model);
    const unsigned short **drawableIndices = csmGetDrawableIndices(model->model);

    int *order = (int *)malloc(sizeof(int) * (size_t)drawableCount);
    if (!order) {
        return;
    }
    for (int i = 0; i < drawableCount; i++) {
        order[i] = i;
    }
    for (int i = 1; i < drawableCount; i++) {
        int key = order[i];
        int j = i - 1;
        while (j >= 0 && renderOrders[order[j]] > renderOrders[key]) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;
    uint32_t totalCommands = 0;

    for (int sorted = 0; sorted < drawableCount; sorted++) {
        int drawable = order[sorted];
        if ((dynamicFlags[drawable] & csmIsVisible) == 0) {
            continue;
        }
        totalVertices += (uint32_t)vertexCounts[drawable];
        totalIndices += (uint32_t)indexCounts[drawable];
        totalCommands += 1;
    }

    if (!topi_reserve_vertices(model, totalVertices) ||
        !topi_reserve_indices(model, totalIndices) ||
        !topi_reserve_commands(model, totalCommands)) {
        free(order);
        return;
    }

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t commandOffset = 0;

    for (int sorted = 0; sorted < drawableCount; sorted++) {
        int drawable = order[sorted];
        if ((dynamicFlags[drawable] & csmIsVisible) == 0) {
            continue;
        }

        int vCount = vertexCounts[drawable];
        int iCount = indexCounts[drawable];
        float opacity = opacities[drawable];

        for (int vertex = 0; vertex < vCount; vertex++) {
            TopiLive2DVertex *outVertex = &model->vertices[vertexOffset + (uint32_t)vertex];
            outVertex->x = positions[drawable][vertex].X;
            outVertex->y = positions[drawable][vertex].Y;
            outVertex->u = uvs[drawable][vertex].X;
            outVertex->v = 1.0f - uvs[drawable][vertex].Y;
            outVertex->r = 1.0f;
            outVertex->g = 1.0f;
            outVertex->b = 1.0f;
            outVertex->a = opacity;
        }

        for (int index = 0; index < iCount; index++) {
            model->indices[indexOffset + (uint32_t)index] = drawableIndices[drawable][index];
        }

        TopiLive2DCommand *command = &model->commands[commandOffset];
        command->drawableIndex = drawable;
        command->textureIndex = textureIndices[drawable];
        command->vertexOffset = vertexOffset;
        command->vertexCount = (uint32_t)vCount;
        command->indexOffset = indexOffset;
        command->indexCount = (uint32_t)iCount;
        command->blendMode = blendModes[drawable];
        command->maskCount = maskCounts[drawable] > 8 ? 8 : maskCounts[drawable];
        command->invertedMask = (constantFlags[drawable] & csmIsInvertedMask) != 0;
        for (int mask = 0; mask < 8; mask++) {
            command->maskIndices[mask] = mask < command->maskCount ? masks[drawable][mask] : -1;
        }

        vertexOffset += (uint32_t)vCount;
        indexOffset += (uint32_t)iCount;
        commandOffset += 1;
    }

    model->vertexCount = vertexOffset;
    model->indexCount = indexOffset;
    model->commandCount = commandOffset;
    free(order);
}

TopiLive2DModel *topi_live2d_create(const char *mocPath) {
    if (!mocPath) {
        return NULL;
    }

    TopiLive2DModel *model = (TopiLive2DModel *)calloc(1, sizeof(TopiLive2DModel));
    if (!model) {
        return NULL;
    }

    model->mocMemory = topi_read_file_aligned(mocPath, csmAlignofMoc, &model->mocSize);
    if (!model->mocMemory) {
        topi_live2d_dispose(model);
        return NULL;
    }

    if (!csmHasMocConsistency(model->mocMemory, model->mocSize)) {
        topi_live2d_dispose(model);
        return NULL;
    }

    model->moc = csmReviveMocInPlace(model->mocMemory, model->mocSize);
    if (!model->moc) {
        topi_live2d_dispose(model);
        return NULL;
    }

    model->modelSize = csmGetSizeofModel(model->moc);
    model->modelMemory = topi_aligned_alloc(csmAlignofModel, model->modelSize);
    if (!model->modelMemory) {
        topi_live2d_dispose(model);
        return NULL;
    }

    model->model = csmInitializeModelInPlace(model->moc, model->modelMemory, model->modelSize);
    if (!model->model) {
        topi_live2d_dispose(model);
        return NULL;
    }

    topi_live2d_update(model);
    return model;
}

void topi_live2d_dispose(TopiLive2DModel *model) {
    if (!model) {
        return;
    }
    free(model->vertices);
    free(model->indices);
    free(model->commands);
    free(model->modelMemory);
    free(model->mocMemory);
    free(model);
}

void topi_live2d_update(TopiLive2DModel *model) {
    if (!model || !model->model) {
        return;
    }
    csmUpdateModel(model->model);
    topi_rebuild_draw_buffers(model);
}

int32_t topi_live2d_parameter_count(TopiLive2DModel *model) {
    if (!model || !model->model) {
        return 0;
    }
    return csmGetParameterCount(model->model);
}

const char *topi_live2d_parameter_name(TopiLive2DModel *model, int32_t index) {
    if (!model || !model->model || index < 0) {
        return NULL;
    }
    int count = csmGetParameterCount(model->model);
    if (index >= count) {
        return NULL;
    }
    const char **ids = csmGetParameterIds(model->model);
    return ids[index];
}

int32_t topi_live2d_offscreen_count(TopiLive2DModel *model) {
    if (!model || !model->model) {
        return 0;
    }
    return csmGetOffscreenCount(model->model);
}

void topi_live2d_reset_parameters(TopiLive2DModel *model) {
    if (!model || !model->model) {
        return;
    }
    int count = csmGetParameterCount(model->model);
    float *values = csmGetParameterValues(model->model);
    const float *defaults = csmGetParameterDefaultValues(model->model);
    for (int index = 0; index < count; index++) {
        values[index] = defaults[index];
    }
}

void topi_live2d_set_parameter(TopiLive2DModel *model, const char *parameterID, float value) {
    if (!model || !model->model || !parameterID) {
        return;
    }
    int count = csmGetParameterCount(model->model);
    const char **ids = csmGetParameterIds(model->model);
    float *values = csmGetParameterValues(model->model);
    const float *minimums = csmGetParameterMinimumValues(model->model);
    const float *maximums = csmGetParameterMaximumValues(model->model);

    for (int index = 0; index < count; index++) {
        if (strcmp(ids[index], parameterID) == 0) {
            float clamped = value;
            if (clamped < minimums[index]) clamped = minimums[index];
            if (clamped > maximums[index]) clamped = maximums[index];
            values[index] = clamped;
            return;
        }
    }
}

TopiLive2DCanvasInfo topi_live2d_canvas_info(TopiLive2DModel *model) {
    TopiLive2DCanvasInfo info = {0, 0, 0, 0, 1};
    if (!model || !model->model) {
        return info;
    }

    csmVector2 size = {0, 0};
    csmVector2 origin = {0, 0};
    float pixelsPerUnit = 1.0f;
    csmReadCanvasInfo(model->model, &size, &origin, &pixelsPerUnit);
    info.width = size.X;
    info.height = size.Y;
    info.originX = origin.X;
    info.originY = origin.Y;
    info.pixelsPerUnit = pixelsPerUnit;
    return info;
}

uint32_t topi_live2d_vertex_count(TopiLive2DModel *model) {
    return model ? model->vertexCount : 0;
}

uint32_t topi_live2d_index_count(TopiLive2DModel *model) {
    return model ? model->indexCount : 0;
}

uint32_t topi_live2d_command_count(TopiLive2DModel *model) {
    return model ? model->commandCount : 0;
}

const TopiLive2DVertex *topi_live2d_vertices(TopiLive2DModel *model) {
    return model ? model->vertices : NULL;
}

const uint16_t *topi_live2d_indices(TopiLive2DModel *model) {
    return model ? model->indices : NULL;
}

const TopiLive2DCommand *topi_live2d_commands(TopiLive2DModel *model) {
    return model ? model->commands : NULL;
}
