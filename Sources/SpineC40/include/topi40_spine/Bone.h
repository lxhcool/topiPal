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

#ifndef SPINE_BONE_H_
#define SPINE_BONE_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/BoneData.h>

#ifdef __cplusplus
extern "C" {
#endif

struct topi40_spSkeleton;

typedef struct topi40_spBone topi40_spBone;
struct topi40_spBone {
	topi40_spBoneData *const data;
	struct topi40_spSkeleton *const skeleton;
	topi40_spBone *const parent;
	int childrenCount;
	topi40_spBone **const children;
	float x, y, rotation, scaleX, scaleY, shearX, shearY;
	float ax, ay, arotation, ascaleX, ascaleY, ashearX, ashearY;

	float const a, b, worldX;
	float const c, d, worldY;

	int/*bool*/ sorted;
	int/*bool*/ active;
};

SP_API void topi40_spBone_setYDown(int/*bool*/yDown);

SP_API int/*bool*/topi40_spBone_isYDown();

/* @param parent May be 0. */
SP_API topi40_spBone *topi40_spBone_create(topi40_spBoneData *data, struct topi40_spSkeleton *skeleton, topi40_spBone *parent);

SP_API void topi40_spBone_dispose(topi40_spBone *self);

SP_API void topi40_spBone_setToSetupPose(topi40_spBone *self);

SP_API void topi40_spBone_update(topi40_spBone *self);

SP_API void topi40_spBone_updateWorldTransform(topi40_spBone *self);

SP_API void topi40_spBone_updateWorldTransformWith(topi40_spBone *self, float x, float y, float rotation, float scaleX, float scaleY,
											float shearX, float shearY);

SP_API float topi40_spBone_getWorldRotationX(topi40_spBone *self);

SP_API float topi40_spBone_getWorldRotationY(topi40_spBone *self);

SP_API float topi40_spBone_getWorldScaleX(topi40_spBone *self);

SP_API float topi40_spBone_getWorldScaleY(topi40_spBone *self);

SP_API void topi40_spBone_updateAppliedTransform(topi40_spBone *self);

SP_API void topi40_spBone_worldToLocal(topi40_spBone *self, float worldX, float worldY, float *localX, float *localY);

SP_API void topi40_spBone_localToWorld(topi40_spBone *self, float localX, float localY, float *worldX, float *worldY);

SP_API float topi40_spBone_worldToLocalRotation(topi40_spBone *self, float worldRotation);

SP_API float topi40_spBone_localToWorldRotation(topi40_spBone *self, float localRotation);

SP_API void topi40_spBone_rotateWorld(topi40_spBone *self, float degrees);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_BONE_H_ */
