#include "SpineBridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <spine/extension.h>

typedef struct TopiAtlasContext {
    int nextPageIndex;
} TopiAtlasContext;

struct TopiSpineDrawable {
    spAtlas *atlas;
    TopiAtlasContext atlasContext;
    spSkeletonData *skeletonData;
    spAnimationStateData *stateData;
    spAnimationState *state;
    spSkeleton *skeleton;
    spSkeletonClipping *clipper;
    spFloatArray *worldVertices;

    TopiSpineVertex *vertices;
    int vertexCount;
    int vertexCapacity;
    uint16_t *indices;
    int indexCount;
    int indexCapacity;
    TopiSpineCommand *commands;
    int commandCount;
    int commandCapacity;
};

static void ensureVertices(TopiSpineDrawable *drawable, int count) {
    if (count <= drawable->vertexCapacity) return;
    int capacity = drawable->vertexCapacity == 0 ? 64 : drawable->vertexCapacity;
    while (capacity < count) capacity *= 2;
    drawable->vertices = (TopiSpineVertex *)realloc(drawable->vertices, sizeof(TopiSpineVertex) * capacity);
    drawable->vertexCapacity = capacity;
}

static void ensureIndices(TopiSpineDrawable *drawable, int count) {
    if (count <= drawable->indexCapacity) return;
    int capacity = drawable->indexCapacity == 0 ? 96 : drawable->indexCapacity;
    while (capacity < count) capacity *= 2;
    drawable->indices = (uint16_t *)realloc(drawable->indices, sizeof(uint16_t) * capacity);
    drawable->indexCapacity = capacity;
}

static void ensureCommands(TopiSpineDrawable *drawable, int count) {
    if (count <= drawable->commandCapacity) return;
    int capacity = drawable->commandCapacity == 0 ? 16 : drawable->commandCapacity;
    while (capacity < count) capacity *= 2;
    drawable->commands = (TopiSpineCommand *)realloc(drawable->commands, sizeof(TopiSpineCommand) * capacity);
    drawable->commandCapacity = capacity;
}

static int textureIndexForPage(spAtlasPage *page) {
    return (int)(intptr_t)page->rendererObject;
}

static void appendCommand(
    TopiSpineDrawable *drawable,
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
        TopiSpineVertex vertex;
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

    TopiSpineCommand command;
    command.textureIndex = textureIndex;
    command.blendMode = blendMode;
    command.vertexOffset = vertexOffset;
    command.vertexCount = verticesCount;
    command.indexOffset = indexOffset;
    command.indexCount = indicesCount;
    drawable->commands[drawable->commandCount++] = command;
}

TopiSpineDrawable *topi_spine_create(const char *atlasPath, const char *skeletonPath, float scale) {
    spBone_setYDown(-1);

    TopiSpineDrawable *drawable = (TopiSpineDrawable *)calloc(1, sizeof(TopiSpineDrawable));
    drawable->atlasContext.nextPageIndex = 0;
    drawable->atlas = spAtlas_createFromFile(atlasPath, &drawable->atlasContext);
    if (!drawable->atlas) {
        free(drawable);
        return NULL;
    }

    spSkeletonBinary *binary = spSkeletonBinary_create(drawable->atlas);
    binary->scale = scale;
    drawable->skeletonData = spSkeletonBinary_readSkeletonDataFile(binary, skeletonPath);
    if (!drawable->skeletonData && binary->error) {
        fprintf(stderr, "TopiSpine failed to read skeleton '%s': %s\n", skeletonPath, binary->error);
    }
    spSkeletonBinary_dispose(binary);
    if (!drawable->skeletonData) {
        spAtlas_dispose(drawable->atlas);
        free(drawable);
        return NULL;
    }

    drawable->skeleton = spSkeleton_create(drawable->skeletonData);
    drawable->stateData = spAnimationStateData_create(drawable->skeletonData);
    drawable->state = spAnimationState_create(drawable->stateData);
    drawable->clipper = spSkeletonClipping_create();
    drawable->worldVertices = spFloatArray_create(128);

    if (drawable->skeletonData->animationsCount > 0) {
        spAnimationState_setAnimation(drawable->state, 0, drawable->skeletonData->animations[0], 1);
    }

    spSkeleton_setToSetupPose(drawable->skeleton);
    spSkeleton_updateWorldTransform(drawable->skeleton);
    return drawable;
}

void topi_spine_dispose(TopiSpineDrawable *drawable) {
    if (!drawable) return;
    free(drawable->vertices);
    free(drawable->indices);
    free(drawable->commands);
    spFloatArray_dispose(drawable->worldVertices);
    spSkeletonClipping_dispose(drawable->clipper);
    spAnimationState_dispose(drawable->state);
    spAnimationStateData_dispose(drawable->stateData);
    spSkeleton_dispose(drawable->skeleton);
    spSkeletonData_dispose(drawable->skeletonData);
    spAtlas_dispose(drawable->atlas);
    free(drawable);
}

int topi_spine_animation_count(TopiSpineDrawable *drawable) {
    return drawable ? drawable->skeletonData->animationsCount : 0;
}

const char *topi_spine_animation_name(TopiSpineDrawable *drawable, int index) {
    if (!drawable || index < 0 || index >= drawable->skeletonData->animationsCount) return NULL;
    return drawable->skeletonData->animations[index]->name;
}

int topi_spine_set_animation(TopiSpineDrawable *drawable, const char *name, int loop) {
    if (!drawable || !name) return 0;
    return spAnimationState_setAnimationByName(drawable->state, 0, name, loop) != NULL;
}

void topi_spine_update(TopiSpineDrawable *drawable, float delta) {
    if (!drawable) return;

    spAnimationState_update(drawable->state, delta);
    spAnimationState_apply(drawable->state, drawable->skeleton);
    spSkeleton_updateWorldTransform(drawable->skeleton);

    drawable->vertexCount = 0;
    drawable->indexCount = 0;
    drawable->commandCount = 0;

    static unsigned short quadIndices[] = {0, 1, 2, 2, 3, 0};
    spSkeleton *skeleton = drawable->skeleton;
    spSkeletonClipping *clipper = drawable->clipper;

    for (int i = 0; i < skeleton->slotsCount; ++i) {
        spSlot *slot = skeleton->drawOrder[i];
        spAttachment *attachment = slot->attachment;
        if (!attachment) {
            spSkeletonClipping_clipEnd(clipper, slot);
            continue;
        }

        if (slot->color.a == 0 || !slot->bone->active) {
            spSkeletonClipping_clipEnd(clipper, slot);
            continue;
        }

        spFloatArray *vertices = drawable->worldVertices;
        int verticesCount = 0;
        float *uvs = NULL;
        unsigned short *indices = NULL;
        int indicesCount = 0;
        spColor *attachmentColor = NULL;
        int textureIndex = 0;

        if (attachment->type == SP_ATTACHMENT_REGION) {
            spRegionAttachment *region = (spRegionAttachment *)attachment;
            attachmentColor = &region->color;
            if (attachmentColor->a == 0) {
                spSkeletonClipping_clipEnd(clipper, slot);
                continue;
            }

            spFloatArray_setSize(vertices, 8);
            spRegionAttachment_computeWorldVertices(region, slot, vertices->items, 0, 2);
            verticesCount = 4;
            uvs = region->uvs;
            indices = quadIndices;
            indicesCount = 6;
            textureIndex = textureIndexForPage(((spAtlasRegion *)region->rendererObject)->page);
        } else if (attachment->type == SP_ATTACHMENT_MESH) {
            spMeshAttachment *mesh = (spMeshAttachment *)attachment;
            attachmentColor = &mesh->color;
            if (attachmentColor->a == 0) {
                spSkeletonClipping_clipEnd(clipper, slot);
                continue;
            }

            spFloatArray_setSize(vertices, mesh->super.worldVerticesLength);
            spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, vertices->items, 0, 2);
            verticesCount = mesh->super.worldVerticesLength >> 1;
            uvs = mesh->uvs;
            indices = mesh->triangles;
            indicesCount = mesh->trianglesCount;
            textureIndex = textureIndexForPage(((spAtlasRegion *)mesh->rendererObject)->page);
        } else if (attachment->type == SP_ATTACHMENT_CLIPPING) {
            spClippingAttachment *clip = (spClippingAttachment *)slot->attachment;
            spSkeletonClipping_clipStart(clipper, slot, clip);
            continue;
        } else {
            continue;
        }

        float r = skeleton->color.r * slot->color.r * attachmentColor->r;
        float g = skeleton->color.g * slot->color.g * attachmentColor->g;
        float b = skeleton->color.b * slot->color.b * attachmentColor->b;
        float a = skeleton->color.a * slot->color.a * attachmentColor->a;

        if (spSkeletonClipping_isClipping(clipper)) {
            spSkeletonClipping_clipTriangles(clipper, vertices->items, verticesCount << 1, indices, indicesCount, uvs, 2);
            vertices = clipper->clippedVertices;
            verticesCount = clipper->clippedVertices->size >> 1;
            uvs = clipper->clippedUVs->items;
            indices = clipper->clippedTriangles->items;
            indicesCount = clipper->clippedTriangles->size;
        }

        appendCommand(drawable, textureIndex, slot->data->blendMode, vertices->items, verticesCount, uvs, indices, indicesCount, r, g, b, a);
        spSkeletonClipping_clipEnd(clipper, slot);
    }
    spSkeletonClipping_clipEnd2(clipper);
}

float topi_spine_width(TopiSpineDrawable *drawable) {
    return drawable ? drawable->skeletonData->width : 0;
}

float topi_spine_height(TopiSpineDrawable *drawable) {
    return drawable ? drawable->skeletonData->height : 0;
}

int topi_spine_texture_count(TopiSpineDrawable *drawable) {
    if (!drawable) return 0;
    int count = 0;
    for (spAtlasPage *page = drawable->atlas->pages; page; page = page->next) count++;
    return count;
}

const char *topi_spine_texture_name(TopiSpineDrawable *drawable, int index) {
    if (!drawable || index < 0) return NULL;
    int current = 0;
    for (spAtlasPage *page = drawable->atlas->pages; page; page = page->next) {
        if (current == index) return page->name;
        current++;
    }
    return NULL;
}

const TopiSpineVertex *topi_spine_vertices(TopiSpineDrawable *drawable) {
    return drawable ? drawable->vertices : NULL;
}

int topi_spine_vertex_count(TopiSpineDrawable *drawable) {
    return drawable ? drawable->vertexCount : 0;
}

const uint16_t *topi_spine_indices(TopiSpineDrawable *drawable) {
    return drawable ? drawable->indices : NULL;
}

int topi_spine_index_count(TopiSpineDrawable *drawable) {
    return drawable ? drawable->indexCount : 0;
}

const TopiSpineCommand *topi_spine_commands(TopiSpineDrawable *drawable) {
    return drawable ? drawable->commands : NULL;
}

int topi_spine_command_count(TopiSpineDrawable *drawable) {
    return drawable ? drawable->commandCount : 0;
}

void _spAtlasPage_createTexture(spAtlasPage *self, const char *path) {
    TopiAtlasContext *context = (TopiAtlasContext *)self->atlas->rendererObject;
    self->rendererObject = (void *)(intptr_t)context->nextPageIndex;
    context->nextPageIndex += 1;
}

void _spAtlasPage_disposeTexture(spAtlasPage *self) {
    self->rendererObject = NULL;
}

char *_spUtil_readFile(const char *path, int *length) {
    return _spReadFile(path, length);
}
