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

#ifndef SPINE_SKELETONDATA_H_
#define SPINE_SKELETONDATA_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/BoneData.h>
#include <topi40_spine/SlotData.h>
#include <topi40_spine/Skin.h>
#include <topi40_spine/EventData.h>
#include <topi40_spine/Animation.h>
#include <topi40_spine/IkConstraintData.h>
#include <topi40_spine/TransformConstraintData.h>
#include <topi40_spine/PathConstraintData.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spSkeletonData {
	const char *version;
	const char *hash;
	float x, y, width, height;
	float fps;
	const char *imagesPath;
	const char *audioPath;

	int stringsCount;
	char **strings;

	int bonesCount;
	topi40_spBoneData **bones;

	int slotsCount;
	topi40_spSlotData **slots;

	int skinsCount;
	topi40_spSkin **skins;
	topi40_spSkin *defaultSkin;

	int eventsCount;
	topi40_spEventData **events;

	int animationsCount;
	topi40_spAnimation **animations;

	int ikConstraintsCount;
	topi40_spIkConstraintData **ikConstraints;

	int transformConstraintsCount;
	topi40_spTransformConstraintData **transformConstraints;

	int pathConstraintsCount;
	topi40_spPathConstraintData **pathConstraints;
} topi40_spSkeletonData;

SP_API topi40_spSkeletonData *topi40_spSkeletonData_create();

SP_API void topi40_spSkeletonData_dispose(topi40_spSkeletonData *self);

SP_API topi40_spBoneData *topi40_spSkeletonData_findBone(const topi40_spSkeletonData *self, const char *boneName);

SP_API topi40_spSlotData *topi40_spSkeletonData_findSlot(const topi40_spSkeletonData *self, const char *slotName);

SP_API topi40_spSkin *topi40_spSkeletonData_findSkin(const topi40_spSkeletonData *self, const char *skinName);

SP_API topi40_spEventData *topi40_spSkeletonData_findEvent(const topi40_spSkeletonData *self, const char *eventName);

SP_API topi40_spAnimation *topi40_spSkeletonData_findAnimation(const topi40_spSkeletonData *self, const char *animationName);

SP_API topi40_spIkConstraintData *topi40_spSkeletonData_findIkConstraint(const topi40_spSkeletonData *self, const char *constraintName);

SP_API topi40_spTransformConstraintData *
topi40_spSkeletonData_findTransformConstraint(const topi40_spSkeletonData *self, const char *constraintName);

SP_API topi40_spPathConstraintData *topi40_spSkeletonData_findPathConstraint(const topi40_spSkeletonData *self, const char *constraintName);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SKELETONDATA_H_ */
