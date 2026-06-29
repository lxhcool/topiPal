#ifndef TOPI_SPINE_BRIDGE_H
#define TOPI_SPINE_BRIDGE_H

#include <stdint.h>
#include <spine/spine.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TopiSpineDrawable TopiSpineDrawable;

typedef struct TopiSpineVertex {
    float x;
    float y;
    float u;
    float v;
    float r;
    float g;
    float b;
    float a;
} TopiSpineVertex;

typedef struct TopiSpineCommand {
    int textureIndex;
    int blendMode;
    int vertexOffset;
    int vertexCount;
    int indexOffset;
    int indexCount;
} TopiSpineCommand;

TopiSpineDrawable *topi_spine_create(const char *atlasPath, const char *skeletonPath, float scale);
void topi_spine_dispose(TopiSpineDrawable *drawable);

int topi_spine_animation_count(TopiSpineDrawable *drawable);
const char *topi_spine_animation_name(TopiSpineDrawable *drawable, int index);
int topi_spine_set_animation(TopiSpineDrawable *drawable, const char *name, int loop);
void topi_spine_update(TopiSpineDrawable *drawable, float delta);

float topi_spine_width(TopiSpineDrawable *drawable);
float topi_spine_height(TopiSpineDrawable *drawable);

int topi_spine_texture_count(TopiSpineDrawable *drawable);
const char *topi_spine_texture_name(TopiSpineDrawable *drawable, int index);

const TopiSpineVertex *topi_spine_vertices(TopiSpineDrawable *drawable);
int topi_spine_vertex_count(TopiSpineDrawable *drawable);
const uint16_t *topi_spine_indices(TopiSpineDrawable *drawable);
int topi_spine_index_count(TopiSpineDrawable *drawable);
const TopiSpineCommand *topi_spine_commands(TopiSpineDrawable *drawable);
int topi_spine_command_count(TopiSpineDrawable *drawable);

#ifdef __cplusplus
}
#endif

#endif
