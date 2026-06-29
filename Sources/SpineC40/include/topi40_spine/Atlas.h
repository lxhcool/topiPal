/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated January 1, 2020. Replaces all prior versions.
 *
 * Copyright (c) 2013-2020, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/topi40_spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef SPINE_ATLAS_H_
#define SPINE_ATLAS_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Array.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spAtlas topi40_spAtlas;

typedef enum {
	SP_ATLAS_UNKNOWN_FORMAT,
	SP_ATLAS_ALPHA,
	SP_ATLAS_INTENSITY,
	SP_ATLAS_LUMINANCE_ALPHA,
	SP_ATLAS_RGB565,
	SP_ATLAS_RGBA4444,
	SP_ATLAS_RGB888,
	SP_ATLAS_RGBA8888
} topi40_spAtlasFormat;

typedef enum {
	SP_ATLAS_UNKNOWN_FILTER,
	SP_ATLAS_NEAREST,
	SP_ATLAS_LINEAR,
	SP_ATLAS_MIPMAP,
	SP_ATLAS_MIPMAP_NEAREST_NEAREST,
	SP_ATLAS_MIPMAP_LINEAR_NEAREST,
	SP_ATLAS_MIPMAP_NEAREST_LINEAR,
	SP_ATLAS_MIPMAP_LINEAR_LINEAR
} topi40_spAtlasFilter;

typedef enum {
	SP_ATLAS_MIRROREDREPEAT,
	SP_ATLAS_CLAMPTOEDGE,
	SP_ATLAS_REPEAT
} topi40_spAtlasWrap;

typedef struct topi40_spAtlasPage topi40_spAtlasPage;
struct topi40_spAtlasPage {
	const topi40_spAtlas *atlas;
	const char *name;
	topi40_spAtlasFormat format;
	topi40_spAtlasFilter minFilter, magFilter;
	topi40_spAtlasWrap uWrap, vWrap;

	void *rendererObject;
	int width, height;
	int /*boolean*/ pma;

	topi40_spAtlasPage *next;
};

SP_API topi40_spAtlasPage *topi40_spAtlasPage_create(topi40_spAtlas *atlas, const char *name);

SP_API void topi40_spAtlasPage_dispose(topi40_spAtlasPage *self);

/**/
typedef struct topi40_spKeyValue {
	char *name;
	float values[5];
} topi40_spKeyValue;
_SP_ARRAY_DECLARE_TYPE(topi40_spKeyValueArray, topi40_spKeyValue)

/**/
typedef struct topi40_spAtlasRegion topi40_spAtlasRegion;
struct topi40_spAtlasRegion {
	const char *name;
	int x, y, width, height;
	float u, v, u2, v2;
	int offsetX, offsetY;
	int originalWidth, originalHeight;
	int index;
	int degrees;
	int *topi40_splits;
	int *pads;
	topi40_spKeyValueArray *keyValues;

	topi40_spAtlasPage *page;

	topi40_spAtlasRegion *next;
};

SP_API topi40_spAtlasRegion *topi40_spAtlasRegion_create();

SP_API void topi40_spAtlasRegion_dispose(topi40_spAtlasRegion *self);

/**/

struct topi40_spAtlas {
	topi40_spAtlasPage *pages;
	topi40_spAtlasRegion *regions;

	void *rendererObject;
};

/* Image files referenced in the atlas file will be prefixed with dir. */
SP_API topi40_spAtlas *topi40_spAtlas_create(const char *data, int length, const char *dir, void *rendererObject);
/* Image files referenced in the atlas file will be prefixed with the directory containing the atlas file. */
SP_API topi40_spAtlas *topi40_spAtlas_createFromFile(const char *path, void *rendererObject);

SP_API void topi40_spAtlas_dispose(topi40_spAtlas *atlas);

/* Returns 0 if the region was not found. */
SP_API topi40_spAtlasRegion *topi40_spAtlas_findRegion(const topi40_spAtlas *self, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_ATLAS_H_ */
