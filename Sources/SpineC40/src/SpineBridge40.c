#include "SpineBridge40.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <topi40_spine/extension.h>

typedef struct TopiAtlasContext {
    int nextPageIndex;
} TopiAtlasContext;

struct TopiSpine40Drawable {
    topi40_spAtlas *atlas;
    TopiAtlasContext atlasContext;
    topi40_spSkeletonData *skeletonData;
    topi40_spAnimationStateData *stateData;
    topi40_spAnimationState *state;
    topi40_spSkeleton *skeleton;
    topi40_spSkeletonClipping *clipper;
    topi40_spFloatArray *worldVertices;

    TopiSpine40Vertex *vertices;
    int vertexCount;
    int vertexCapacity;
    uint16_t *indices;
    int indexCount;
    int indexCapacity;
    TopiSpine40Command *commands;
    int commandCount;
    int commandCapacity;
};

static void ensureVertices(TopiSpine40Drawable *drawable, int count) {
    if (count <= drawable->vertexCapacity) return;
    int capacity = drawable->vertexCapacity == 0 ? 64 : drawable->vertexCapacity;
    while (capacity < count) capacity *= 2;
    drawable->vertices = (TopiSpine40Vertex *)realloc(drawable->vertices, sizeof(TopiSpine40Vertex) * capacity);
    drawable->vertexCapacity = capacity;
}

static void ensureIndices(TopiSpine40Drawable *drawable, int count) {
    if (count <= drawable->indexCapacity) return;
    int capacity = drawable->indexCapacity == 0 ? 96 : drawable->indexCapacity;
    while (capacity < count) capacity *= 2;
    drawable->indices = (uint16_t *)realloc(drawable->indices, sizeof(uint16_t) * capacity);
    drawable->indexCapacity = capacity;
}

static void ensureCommands(TopiSpine40Drawable *drawable, int count) {
    if (count <= drawable->commandCapacity) return;
    int capacity = drawable->commandCapacity == 0 ? 16 : drawable->commandCapacity;
    while (capacity < count) capacity *= 2;
    drawable->commands = (TopiSpine40Command *)realloc(drawable->commands, sizeof(TopiSpine40Command) * capacity);
    drawable->commandCapacity = capacity;
}

static int textureIndexForPage(topi40_spAtlasPage *page) {
    return (int)(intptr_t)page->rendererObject;
}

static void appendCommand(
    TopiSpine40Drawable *drawable,
    int textureIndex,
    int blendMode,
    float *vertices,
    int verticesCount,
    float *uvs,
    unsigned short *indices,
    int indicesCount,
    float r,
    float g,
    float b,
    float a
) {
    if (verticesCount <= 0 || indicesCount <= 0) return;

    int vertexOffset = drawable->vertexCount;
    int indexOffset = drawable->indexCount;
    ensureVertices(drawable, drawable->vertexCount + verticesCount);
    ensureIndices(drawable, drawable->indexCount + indicesCount);
    ensureCommands(drawable, drawable->commandCount + 1);

    for (int i = 0; i < verticesCount; i++) {
        TopiSpine40Vertex vertex;
        vertex.x = vertices[i * 2];
        vertex.y = vertices[i * 2 + 1];
        vertex.u = uvs[i * 2];
        vertex.v = uvs[i * 2 + 1];
        vertex.r = r;
        vertex.g = g;
        vertex.b = b;
        vertex.a = a;
        drawable->vertices[drawable->vertexCount++] = vertex;
    }

    for (int i = 0; i < indicesCount; i++) {
        drawable->indices[drawable->indexCount++] = (uint16_t)(vertexOffset + indices[i]);
    }

    TopiSpine40Command command;
    command.textureIndex = textureIndex;
    command.blendMode = blendMode;
    command.vertexOffset = vertexOffset;
    command.vertexCount = verticesCount;
    command.indexOffset = indexOffset;
    command.indexCount = indicesCount;
    drawable->commands[drawable->commandCount++] = command;
}

TopiSpine40Drawable *topi_spine40_create(const char *atlasPath, const char *skeletonPath, float scale) {
    topi40_spBone_setYDown(-1);

    TopiSpine40Drawable *drawable = (TopiSpine40Drawable *)calloc(1, sizeof(TopiSpine40Drawable));
    drawable->atlasContext.nextPageIndex = 0;
    drawable->atlas = topi40_spAtlas_createFromFile(atlasPath, &drawable->atlasContext);
    if (!drawable->atlas) {
        free(drawable);
        return NULL;
    }

    topi40_spSkeletonBinary *binary = topi40_spSkeletonBinary_create(drawable->atlas);
    binary->scale = scale;
    drawable->skeletonData = topi40_spSkeletonBinary_readSkeletonDataFile(binary, skeletonPath);
    if (!drawable->skeletonData && binary->error) {
        fprintf(stderr, "TopiSpine failed to read skeleton '%s': %s\n", skeletonPath, binary->error);
    }
    topi40_spSkeletonBinary_dispose(binary);
    if (!drawable->skeletonData) {
        topi40_spAtlas_dispose(drawable->atlas);
        free(drawable);
        return NULL;
    }

    drawable->skeleton = topi40_spSkeleton_create(drawable->skeletonData);
    drawable->stateData = topi40_spAnimationStateData_create(drawable->skeletonData);
    drawable->state = topi40_spAnimationState_create(drawable->stateData);
    drawable->clipper = topi40_spSkeletonClipping_create();
    drawable->worldVertices = topi40_spFloatArray_create(128);

    if (drawable->skeletonData->animationsCount > 0) {
        topi40_spAnimationState_setAnimation(drawable->state, 0, drawable->skeletonData->animations[0], 1);
    }

    topi40_spSkeleton_setToSetupPose(drawable->skeleton);
    topi40_spSkeleton_updateWorldTransform(drawable->skeleton);
    return drawable;
}

void topi_spine40_dispose(TopiSpine40Drawable *drawable) {
    if (!drawable) return;
    free(drawable->vertices);
    free(drawable->indices);
    free(drawable->commands);
    topi40_spFloatArray_dispose(drawable->worldVertices);
    topi40_spSkeletonClipping_dispose(drawable->clipper);
    topi40_spAnimationState_dispose(drawable->state);
    topi40_spAnimationStateData_dispose(drawable->stateData);
    topi40_spSkeleton_dispose(drawable->skeleton);
    topi40_spSkeletonData_dispose(drawable->skeletonData);
    topi40_spAtlas_dispose(drawable->atlas);
    free(drawable);
}

int topi_spine40_animation_count(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->skeletonData->animationsCount : 0;
}

const char *topi_spine40_animation_name(TopiSpine40Drawable *drawable, int index) {
    if (!drawable || index < 0 || index >= drawable->skeletonData->animationsCount) return NULL;
    return drawable->skeletonData->animations[index]->name;
}

int topi_spine40_set_animation(TopiSpine40Drawable *drawable, const char *name, int loop) {
    if (!drawable || !name) return 0;
    return topi40_spAnimationState_setAnimationByName(drawable->state, 0, name, loop) != NULL;
}

void topi_spine40_update(TopiSpine40Drawable *drawable, float delta) {
    if (!drawable) return;

    topi40_spAnimationState_update(drawable->state, delta);
    topi40_spAnimationState_apply(drawable->state, drawable->skeleton);
    topi40_spSkeleton_updateWorldTransform(drawable->skeleton);

    drawable->vertexCount = 0;
    drawable->indexCount = 0;
    drawable->commandCount = 0;

    static unsigned short quadIndices[] = {0, 1, 2, 2, 3, 0};
    topi40_spSkeleton *skeleton = drawable->skeleton;
    topi40_spSkeletonClipping *clipper = drawable->clipper;

    for (int i = 0; i < skeleton->slotsCount; ++i) {
        topi40_spSlot *slot = skeleton->drawOrder[i];
        topi40_spAttachment *attachment = slot->attachment;
        if (!attachment) {
            topi40_spSkeletonClipping_clipEnd(clipper, slot);
            continue;
        }

        if (slot->color.a == 0 || !slot->bone->active) {
            topi40_spSkeletonClipping_clipEnd(clipper, slot);
            continue;
        }

        topi40_spFloatArray *vertices = drawable->worldVertices;
        int verticesCount = 0;
        float *uvs = NULL;
        unsigned short *indices = NULL;
        int indicesCount = 0;
        topi40_spColor *attachmentColor = NULL;
        int textureIndex = 0;

        if (attachment->type == SP_ATTACHMENT_REGION) {
            topi40_spRegionAttachment *region = (topi40_spRegionAttachment *)attachment;
            attachmentColor = &region->color;
            if (attachmentColor->a == 0) {
                topi40_spSkeletonClipping_clipEnd(clipper, slot);
                continue;
            }

            topi40_spFloatArray_setSize(vertices, 8);
            topi40_spRegionAttachment_computeWorldVertices(region, slot->bone, vertices->items, 0, 2);
            verticesCount = 4;
            uvs = region->uvs;
            indices = quadIndices;
            indicesCount = 6;
            textureIndex = textureIndexForPage(((topi40_spAtlasRegion *)region->rendererObject)->page);
        } else if (attachment->type == SP_ATTACHMENT_MESH) {
            topi40_spMeshAttachment *mesh = (topi40_spMeshAttachment *)attachment;
            attachmentColor = &mesh->color;
            if (attachmentColor->a == 0) {
                topi40_spSkeletonClipping_clipEnd(clipper, slot);
                continue;
            }

            topi40_spFloatArray_setSize(vertices, mesh->super.worldVerticesLength);
            topi40_spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, vertices->items, 0, 2);
            verticesCount = mesh->super.worldVerticesLength >> 1;
            uvs = mesh->uvs;
            indices = mesh->triangles;
            indicesCount = mesh->trianglesCount;
            textureIndex = textureIndexForPage(((topi40_spAtlasRegion *)mesh->rendererObject)->page);
        } else if (attachment->type == SP_ATTACHMENT_CLIPPING) {
            topi40_spClippingAttachment *clip = (topi40_spClippingAttachment *)slot->attachment;
            topi40_spSkeletonClipping_clipStart(clipper, slot, clip);
            continue;
        } else {
            continue;
        }

        float r = skeleton->color.r * slot->color.r * attachmentColor->r;
        float g = skeleton->color.g * slot->color.g * attachmentColor->g;
        float b = skeleton->color.b * slot->color.b * attachmentColor->b;
        float a = skeleton->color.a * slot->color.a * attachmentColor->a;

        if (topi40_spSkeletonClipping_isClipping(clipper)) {
            topi40_spSkeletonClipping_clipTriangles(clipper, vertices->items, verticesCount << 1, indices, indicesCount, uvs, 2);
            vertices = clipper->clippedVertices;
            verticesCount = clipper->clippedVertices->size >> 1;
            uvs = clipper->clippedUVs->items;
            indices = clipper->clippedTriangles->items;
            indicesCount = clipper->clippedTriangles->size;
        }

        appendCommand(drawable, textureIndex, slot->data->blendMode, vertices->items, verticesCount, uvs, indices, indicesCount, r, g, b, a);
        topi40_spSkeletonClipping_clipEnd(clipper, slot);
    }
    topi40_spSkeletonClipping_clipEnd2(clipper);
}

float topi_spine40_width(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->skeletonData->width : 0;
}

float topi_spine40_height(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->skeletonData->height : 0;
}

int topi_spine40_texture_count(TopiSpine40Drawable *drawable) {
    if (!drawable) return 0;
    int count = 0;
    for (topi40_spAtlasPage *page = drawable->atlas->pages; page; page = page->next) count++;
    return count;
}

const char *topi_spine40_texture_name(TopiSpine40Drawable *drawable, int index) {
    if (!drawable || index < 0) return NULL;
    int current = 0;
    for (topi40_spAtlasPage *page = drawable->atlas->pages; page; page = page->next) {
        if (current == index) return page->name;
        current++;
    }
    return NULL;
}

const TopiSpine40Vertex *topi_spine40_vertices(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->vertices : NULL;
}

int topi_spine40_vertex_count(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->vertexCount : 0;
}

const uint16_t *topi_spine40_indices(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->indices : NULL;
}

int topi_spine40_index_count(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->indexCount : 0;
}

const TopiSpine40Command *topi_spine40_commands(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->commands : NULL;
}

int topi_spine40_command_count(TopiSpine40Drawable *drawable) {
    return drawable ? drawable->commandCount : 0;
}

void _topi40_spAtlasPage_createTexture(topi40_spAtlasPage *self, const char *path) {
    TopiAtlasContext *context = (TopiAtlasContext *)self->atlas->rendererObject;
    self->rendererObject = (void *)(intptr_t)context->nextPageIndex;
    context->nextPageIndex += 1;
}

void _topi40_spAtlasPage_disposeTexture(topi40_spAtlasPage *self) {
    self->rendererObject = NULL;
}

char *_topi40_spUtil_readFile(const char *path, int *length) {
    return _topi40_spReadFile(path, length);
}
