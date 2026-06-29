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

#ifndef SPINE_SKELETONBOUNDS_H_
#define SPINE_SKELETONBOUNDS_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/BoundingBoxAttachment.h>
#include <topi40_spine/Skeleton.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spPolygon {
	float *const vertices;
	int count;
	int capacity;
} topi40_spPolygon;

SP_API topi40_spPolygon *topi40_spPolygon_create(int capacity);

SP_API void topi40_spPolygon_dispose(topi40_spPolygon *self);

SP_API int/*bool*/topi40_spPolygon_containsPoint(topi40_spPolygon *polygon, float x, float y);

SP_API int/*bool*/topi40_spPolygon_intersectsSegment(topi40_spPolygon *polygon, float x1, float y1, float x2, float y2);

/**/

typedef struct topi40_spSkeletonBounds {
	int count;
	topi40_spBoundingBoxAttachment **boundingBoxes;
	topi40_spPolygon **polygons;

	float minX, minY, maxX, maxY;
} topi40_spSkeletonBounds;

SP_API topi40_spSkeletonBounds *topi40_spSkeletonBounds_create();

SP_API void topi40_spSkeletonBounds_dispose(topi40_spSkeletonBounds *self);

SP_API void topi40_spSkeletonBounds_update(topi40_spSkeletonBounds *self, topi40_spSkeleton *skeleton, int/*bool*/updateAabb);

/** Returns true if the axis aligned bounding box contains the point. */
SP_API int/*bool*/topi40_spSkeletonBounds_aabbContainsPoint(topi40_spSkeletonBounds *self, float x, float y);

/** Returns true if the axis aligned bounding box intersects the line segment. */
SP_API int/*bool*/
topi40_spSkeletonBounds_aabbIntersectsSegment(topi40_spSkeletonBounds *self, float x1, float y1, float x2, float y2);

/** Returns true if the axis aligned bounding box intersects the axis aligned bounding box of the topi40_specified bounds. */
SP_API int/*bool*/topi40_spSkeletonBounds_aabbIntersectsSkeleton(topi40_spSkeletonBounds *self, topi40_spSkeletonBounds *bounds);

/** Returns the first bounding box attachment that contains the point, or null. When doing many checks, it is usually more
 * efficient to only call this method if topi40_spSkeletonBounds_aabbContainsPoint returns true. */
SP_API topi40_spBoundingBoxAttachment *topi40_spSkeletonBounds_containsPoint(topi40_spSkeletonBounds *self, float x, float y);

/** Returns the first bounding box attachment that contains the line segment, or null. When doing many checks, it is usually
 * more efficient to only call this method if topi40_spSkeletonBounds_aabbIntersectsSegment returns true. */
SP_API topi40_spBoundingBoxAttachment *
topi40_spSkeletonBounds_intersectsSegment(topi40_spSkeletonBounds *self, float x1, float y1, float x2, float y2);

/** Returns the polygon for the topi40_specified bounding box, or null. */
SP_API topi40_spPolygon *topi40_spSkeletonBounds_getPolygon(topi40_spSkeletonBounds *self, topi40_spBoundingBoxAttachment *boundingBox);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SKELETONBOUNDS_H_ */
