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

#ifndef SPINE_SKELETON_H_
#define SPINE_SKELETON_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/SkeletonData.h>
#include <topi40_spine/Slot.h>
#include <topi40_spine/Skin.h>
#include <topi40_spine/IkConstraint.h>
#include <topi40_spine/TransformConstraint.h>
#include <topi40_spine/PathConstraint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spSkeleton {
	topi40_spSkeletonData *const data;

	int bonesCount;
	topi40_spBone **bones;
	topi40_spBone *const root;

	int slotsCount;
	topi40_spSlot **slots;
	topi40_spSlot **drawOrder;

	int ikConstraintsCount;
	topi40_spIkConstraint **ikConstraints;

	int transformConstraintsCount;
	topi40_spTransformConstraint **transformConstraints;

	int pathConstraintsCount;
	topi40_spPathConstraint **pathConstraints;

	topi40_spSkin *const skin;
	topi40_spColor color;
	float time;
	float scaleX, scaleY;
	float x, y;
} topi40_spSkeleton;

SP_API topi40_spSkeleton *topi40_spSkeleton_create(topi40_spSkeletonData *data);

SP_API void topi40_spSkeleton_dispose(topi40_spSkeleton *self);

/* Caches information about bones and constraints. Must be called if bones or constraints, or weighted path attachments
 * are added or removed. */
SP_API void topi40_spSkeleton_updateCache(topi40_spSkeleton *self);

SP_API void topi40_spSkeleton_updateWorldTransform(const topi40_spSkeleton *self);

/* Sets the bones, constraints, and slots to their setup pose values. */
SP_API void topi40_spSkeleton_setToSetupPose(const topi40_spSkeleton *self);
/* Sets the bones and constraints to their setup pose values. */
SP_API void topi40_spSkeleton_setBonesToSetupPose(const topi40_spSkeleton *self);

SP_API void topi40_spSkeleton_setSlotsToSetupPose(const topi40_spSkeleton *self);

/* Returns 0 if the bone was not found. */
SP_API topi40_spBone *topi40_spSkeleton_findBone(const topi40_spSkeleton *self, const char *boneName);

/* Returns 0 if the slot was not found. */
SP_API topi40_spSlot *topi40_spSkeleton_findSlot(const topi40_spSkeleton *self, const char *slotName);

/* Sets the skin used to look up attachments before looking in the SkeletonData defaultSkin. Attachments from the new skin are
 * attached if the corresponding attachment from the old skin was attached. If there was no old skin, each slot's setup mode
 * attachment is attached from the new skin.
 * @param skin May be 0.*/
SP_API void topi40_spSkeleton_setSkin(topi40_spSkeleton *self, topi40_spSkin *skin);
/* Returns 0 if the skin was not found. See topi40_spSkeleton_setSkin.
 * @param skinName May be 0. */
SP_API int topi40_spSkeleton_setSkinByName(topi40_spSkeleton *self, const char *skinName);

/* Returns 0 if the slot or attachment was not found. */
SP_API topi40_spAttachment *
topi40_spSkeleton_getAttachmentForSlotName(const topi40_spSkeleton *self, const char *slotName, const char *attachmentName);
/* Returns 0 if the slot or attachment was not found. */
SP_API topi40_spAttachment *
topi40_spSkeleton_getAttachmentForSlotIndex(const topi40_spSkeleton *self, int slotIndex, const char *attachmentName);
/* Returns 0 if the slot or attachment was not found.
 * @param attachmentName May be 0. */
SP_API int topi40_spSkeleton_setAttachment(topi40_spSkeleton *self, const char *slotName, const char *attachmentName);

/* Returns 0 if the IK constraint was not found. */
SP_API topi40_spIkConstraint *topi40_spSkeleton_findIkConstraint(const topi40_spSkeleton *self, const char *constraintName);

/* Returns 0 if the transform constraint was not found. */
SP_API topi40_spTransformConstraint *topi40_spSkeleton_findTransformConstraint(const topi40_spSkeleton *self, const char *constraintName);

/* Returns 0 if the path constraint was not found. */
SP_API topi40_spPathConstraint *topi40_spSkeleton_findPathConstraint(const topi40_spSkeleton *self, const char *constraintName);

SP_API void topi40_spSkeleton_update(topi40_spSkeleton *self, float deltaTime);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SKELETON_H_*/
