#ifndef TOPI_LIVE2D_BRIDGE_H
#define TOPI_LIVE2D_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TopiLive2DModel TopiLive2DModel;

typedef struct TopiLive2DVertex {
    float x;
    float y;
    float u;
    float v;
    float r;
    float g;
    float b;
    float a;
} TopiLive2DVertex;

typedef struct TopiLive2DCommand {
    int32_t drawableIndex;
    int32_t textureIndex;
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
    int32_t blendMode;
    int32_t maskCount;
    int32_t maskIndices[8];
    int32_t invertedMask;
} TopiLive2DCommand;

typedef struct TopiLive2DCanvasInfo {
    float width;
    float height;
    float originX;
    float originY;
    float pixelsPerUnit;
} TopiLive2DCanvasInfo;

TopiLive2DModel *topi_live2d_create(const char *mocPath);
void topi_live2d_dispose(TopiLive2DModel *model);
void topi_live2d_update(TopiLive2DModel *model);

int32_t topi_live2d_parameter_count(TopiLive2DModel *model);
const char *topi_live2d_parameter_name(TopiLive2DModel *model, int32_t index);
int32_t topi_live2d_offscreen_count(TopiLive2DModel *model);
void topi_live2d_reset_parameters(TopiLive2DModel *model);
void topi_live2d_set_parameter(TopiLive2DModel *model, const char *parameterID, float value);

TopiLive2DCanvasInfo topi_live2d_canvas_info(TopiLive2DModel *model);
uint32_t topi_live2d_vertex_count(TopiLive2DModel *model);
uint32_t topi_live2d_index_count(TopiLive2DModel *model);
uint32_t topi_live2d_command_count(TopiLive2DModel *model);
const TopiLive2DVertex *topi_live2d_vertices(TopiLive2DModel *model);
const uint16_t *topi_live2d_indices(TopiLive2DModel *model);
const TopiLive2DCommand *topi_live2d_commands(TopiLive2DModel *model);

#ifdef __cplusplus
}
#endif

#endif
