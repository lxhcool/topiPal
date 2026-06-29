#ifndef TOPI_SPINE_BRIDGE_H
#define TOPI_SPINE_BRIDGE_H

#include <stdint.h>
#include <topi40_spine/topi40_spine.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TopiSpine40Drawable TopiSpine40Drawable;

typedef struct TopiSpine40Vertex {
    float x;
    float y;
    float u;
    float v;
    float r;
    float g;
    float b;
    float a;
} TopiSpine40Vertex;

typedef struct TopiSpine40Command {
    int textureIndex;
    int blendMode;
    int vertexOffset;
    int vertexCount;
    int indexOffset;
    int indexCount;
} TopiSpine40Command;

TopiSpine40Drawable *topi_spine40_create(const char *atlasPath, const char *skeletonPath, float scale);
void topi_spine40_dispose(TopiSpine40Drawable *drawable);

int topi_spine40_animation_count(TopiSpine40Drawable *drawable);
const char *topi_spine40_animation_name(TopiSpine40Drawable *drawable, int index);
int topi_spine40_set_animation(TopiSpine40Drawable *drawable, const char *name, int loop);
void topi_spine40_update(TopiSpine40Drawable *drawable, float delta);

float topi_spine40_width(TopiSpine40Drawable *drawable);
float topi_spine40_height(TopiSpine40Drawable *drawable);

int topi_spine40_texture_count(TopiSpine40Drawable *drawable);
const char *topi_spine40_texture_name(TopiSpine40Drawable *drawable, int index);

const TopiSpine40Vertex *topi_spine40_vertices(TopiSpine40Drawable *drawable);
int topi_spine40_vertex_count(TopiSpine40Drawable *drawable);
const uint16_t *topi_spine40_indices(TopiSpine40Drawable *drawable);
int topi_spine40_index_count(TopiSpine40Drawable *drawable);
const TopiSpine40Command *topi_spine40_commands(TopiSpine40Drawable *drawable);
int topi_spine40_command_count(TopiSpine40Drawable *drawable);

#ifdef __cplusplus
}
#endif

#endif
