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

#ifndef SPINE_SKELETONCLIPPING_H
#define SPINE_SKELETONCLIPPING_H

#include <topi40_spine/dll.h>
#include <topi40_spine/Array.h>
#include <topi40_spine/ClippingAttachment.h>
#include <topi40_spine/Slot.h>
#include <topi40_spine/Triangulator.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spSkeletonClipping {
	topi40_spTriangulator *triangulator;
	topi40_spFloatArray *clippingPolygon;
	topi40_spFloatArray *clipOutput;
	topi40_spFloatArray *clippedVertices;
	topi40_spFloatArray *clippedUVs;
	topi40_spUnsignedShortArray *clippedTriangles;
	topi40_spFloatArray *scratch;
	topi40_spClippingAttachment *clipAttachment;
	topi40_spArrayFloatArray *clippingPolygons;
} topi40_spSkeletonClipping;

SP_API topi40_spSkeletonClipping *topi40_spSkeletonClipping_create();

SP_API int topi40_spSkeletonClipping_clipStart(topi40_spSkeletonClipping *self, topi40_spSlot *slot, topi40_spClippingAttachment *clip);

SP_API void topi40_spSkeletonClipping_clipEnd(topi40_spSkeletonClipping *self, topi40_spSlot *slot);

SP_API void topi40_spSkeletonClipping_clipEnd2(topi40_spSkeletonClipping *self);

SP_API int /*boolean*/ topi40_spSkeletonClipping_isClipping(topi40_spSkeletonClipping *self);

SP_API void topi40_spSkeletonClipping_clipTriangles(topi40_spSkeletonClipping *self, float *vertices, int verticesLength,
											 unsigned short *triangles, int trianglesLength, float *uvs, int stride);

SP_API void topi40_spSkeletonClipping_dispose(topi40_spSkeletonClipping *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SKELETONCLIPPING_H */
