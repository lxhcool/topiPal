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

#include <topi40_spine/Skeleton.h>
#include <topi40_spine/extension.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	SP_UPDATE_BONE,
	SP_UPDATE_IK_CONSTRAINT,
	SP_UPDATE_PATH_CONSTRAINT,
	SP_UPDATE_TRANSFORM_CONSTRAINT
} _topi40_spUpdateType;

typedef struct {
	_topi40_spUpdateType type;
	void *object;
} _topi40_spUpdate;

typedef struct {
	topi40_spSkeleton super;

	int updateCacheCount;
	int updateCacheCapacity;
	_topi40_spUpdate *updateCache;
} _topi40_spSkeleton;

topi40_spSkeleton *topi40_spSkeleton_create(topi40_spSkeletonData *data) {
	int i;
	int *childrenCounts;

	_topi40_spSkeleton *internal = NEW(_topi40_spSkeleton);
	topi40_spSkeleton *self = SUPER(internal);
	CONST_CAST(topi40_spSkeletonData *, self->data) = data;

	self->bonesCount = self->data->bonesCount;
	self->bones = MALLOC(topi40_spBone *, self->bonesCount);
	childrenCounts = CALLOC(int, self->bonesCount);

	for (i = 0; i < self->bonesCount; ++i) {
		topi40_spBoneData *boneData = self->data->bones[i];
		topi40_spBone *newBone;
		if (!boneData->parent)
			newBone = topi40_spBone_create(boneData, self, 0);
		else {
			topi40_spBone *parent = self->bones[boneData->parent->index];
			newBone = topi40_spBone_create(boneData, self, parent);
			++childrenCounts[boneData->parent->index];
		}
		self->bones[i] = newBone;
	}
	for (i = 0; i < self->bonesCount; ++i) {
		topi40_spBoneData *boneData = self->data->bones[i];
		topi40_spBone *bone = self->bones[i];
		CONST_CAST(topi40_spBone **, bone->children) = MALLOC(topi40_spBone *, childrenCounts[boneData->index]);
	}
	for (i = 0; i < self->bonesCount; ++i) {
		topi40_spBone *bone = self->bones[i];
		topi40_spBone *parent = bone->parent;
		if (parent)
			parent->children[parent->childrenCount++] = bone;
	}
	CONST_CAST(topi40_spBone *, self->root) = (self->bonesCount > 0 ? self->bones[0] : NULL);

	self->slotsCount = data->slotsCount;
	self->slots = MALLOC(topi40_spSlot *, self->slotsCount);
	for (i = 0; i < self->slotsCount; ++i) {
		topi40_spSlotData *slotData = data->slots[i];
		topi40_spBone *bone = self->bones[slotData->boneData->index];
		self->slots[i] = topi40_spSlot_create(slotData, bone);
	}

	self->drawOrder = MALLOC(topi40_spSlot *, self->slotsCount);
	memcpy(self->drawOrder, self->slots, sizeof(topi40_spSlot *) * self->slotsCount);

	self->ikConstraintsCount = data->ikConstraintsCount;
	self->ikConstraints = MALLOC(topi40_spIkConstraint *, self->ikConstraintsCount);
	for (i = 0; i < self->data->ikConstraintsCount; ++i)
		self->ikConstraints[i] = topi40_spIkConstraint_create(self->data->ikConstraints[i], self);

	self->transformConstraintsCount = data->transformConstraintsCount;
	self->transformConstraints = MALLOC(topi40_spTransformConstraint *, self->transformConstraintsCount);
	for (i = 0; i < self->data->transformConstraintsCount; ++i)
		self->transformConstraints[i] = topi40_spTransformConstraint_create(self->data->transformConstraints[i], self);

	self->pathConstraintsCount = data->pathConstraintsCount;
	self->pathConstraints = MALLOC(topi40_spPathConstraint *, self->pathConstraintsCount);
	for (i = 0; i < self->data->pathConstraintsCount; i++)
		self->pathConstraints[i] = topi40_spPathConstraint_create(self->data->pathConstraints[i], self);

	topi40_spColor_setFromFloats(&self->color, 1, 1, 1, 1);

	self->scaleX = 1;
	self->scaleY = 1;

	topi40_spSkeleton_updateCache(self);

	FREE(childrenCounts);

	return self;
}

void topi40_spSkeleton_dispose(topi40_spSkeleton *self) {
	int i;
	_topi40_spSkeleton *internal = SUB_CAST(_topi40_spSkeleton, self);

	FREE(internal->updateCache);

	for (i = 0; i < self->bonesCount; ++i)
		topi40_spBone_dispose(self->bones[i]);
	FREE(self->bones);

	for (i = 0; i < self->slotsCount; ++i)
		topi40_spSlot_dispose(self->slots[i]);
	FREE(self->slots);

	for (i = 0; i < self->ikConstraintsCount; ++i)
		topi40_spIkConstraint_dispose(self->ikConstraints[i]);
	FREE(self->ikConstraints);

	for (i = 0; i < self->transformConstraintsCount; ++i)
		topi40_spTransformConstraint_dispose(self->transformConstraints[i]);
	FREE(self->transformConstraints);

	for (i = 0; i < self->pathConstraintsCount; i++)
		topi40_spPathConstraint_dispose(self->pathConstraints[i]);
	FREE(self->pathConstraints);

	FREE(self->drawOrder);
	FREE(self);
}

static void _addToUpdateCache(_topi40_spSkeleton *const internal, _topi40_spUpdateType type, void *object) {
	_topi40_spUpdate *update;
	if (internal->updateCacheCount == internal->updateCacheCapacity) {
		internal->updateCacheCapacity *= 2;
		internal->updateCache = (_topi40_spUpdate *) REALLOC(internal->updateCache, _topi40_spUpdate, internal->updateCacheCapacity);
	}
	update = internal->updateCache + internal->updateCacheCount;
	update->type = type;
	update->object = object;
	++internal->updateCacheCount;
}

static void _sortBone(_topi40_spSkeleton *const internal, topi40_spBone *bone) {
	if (bone->sorted) return;
	if (bone->parent) _sortBone(internal, bone->parent);
	bone->sorted = 1;
	_addToUpdateCache(internal, SP_UPDATE_BONE, bone);
}

static void
_sortPathConstraintAttachmentBones(_topi40_spSkeleton *const internal, topi40_spAttachment *attachment, topi40_spBone *slotBone) {
	topi40_spPathAttachment *pathAttachment = (topi40_spPathAttachment *) attachment;
	int *pathBones;
	int pathBonesCount;
	if (pathAttachment->super.super.type != SP_ATTACHMENT_PATH) return;
	pathBones = pathAttachment->super.bones;
	pathBonesCount = pathAttachment->super.bonesCount;
	if (pathBones == 0)
		_sortBone(internal, slotBone);
	else {
		topi40_spBone **bones = internal->super.bones;
		int i = 0, n;

		for (i = 0, n = pathBonesCount; i < n;) {
			int nn = pathBones[i++];
			nn += i;
			while (i < nn)
				_sortBone(internal, bones[pathBones[i++]]);
		}
	}
}

static void _sortPathConstraintAttachment(_topi40_spSkeleton *const internal, topi40_spSkin *skin, int slotIndex, topi40_spBone *slotBone) {
	_topi40_Entry *entry = SUB_CAST(_topi40_spSkin, skin)->entries;
	while (entry) {
		if (entry->slotIndex == slotIndex) _sortPathConstraintAttachmentBones(internal, entry->attachment, slotBone);
		entry = entry->next;
	}
}

static void _sortReset(topi40_spBone **bones, int bonesCount) {
	int i;
	for (i = 0; i < bonesCount; ++i) {
		topi40_spBone *bone = bones[i];
		if (!bone->active) continue;
		if (bone->sorted) _sortReset(bone->children, bone->childrenCount);
		bone->sorted = 0;
	}
}

static void _sortIkConstraint(_topi40_spSkeleton *const internal, topi40_spIkConstraint *constraint) {
	topi40_spBone *target = constraint->target;
	topi40_spBone **constrained;
	topi40_spBone *parent;

	constraint->active = constraint->target->active && (!constraint->data->skinRequired || (internal->super.skin != 0 &&
																							topi40_spIkConstraintDataArray_contains(
																									internal->super.skin->ikConstraints,
																									constraint->data)));
	if (!constraint->active) return;

	_sortBone(internal, target);

	constrained = constraint->bones;
	parent = constrained[0];
	_sortBone(internal, parent);

	if (constraint->bonesCount == 1) {
		_addToUpdateCache(internal, SP_UPDATE_IK_CONSTRAINT, constraint);
		_sortReset(parent->children, parent->childrenCount);
	} else {
		topi40_spBone *child = constrained[constraint->bonesCount - 1];
		_sortBone(internal, child);

		_addToUpdateCache(internal, SP_UPDATE_IK_CONSTRAINT, constraint);

		_sortReset(parent->children, parent->childrenCount);
		child->sorted = 1;
	}
}

static void _sortPathConstraint(_topi40_spSkeleton *const internal, topi40_spPathConstraint *constraint) {
	topi40_spSlot *slot = constraint->target;
	int slotIndex = slot->data->index;
	topi40_spBone *slotBone = slot->bone;
	int i, n, boneCount;
	topi40_spAttachment *attachment;
	topi40_spBone **constrained;
	topi40_spSkeleton *skeleton = SUPER_CAST(topi40_spSkeleton, internal);

	constraint->active = constraint->target->bone->active && (!constraint->data->skinRequired ||
															  (internal->super.skin != 0 &&
															   topi40_spPathConstraintDataArray_contains(
																	   internal->super.skin->pathConstraints,
																	   constraint->data)));
	if (!constraint->active) return;

	if (skeleton->skin) _sortPathConstraintAttachment(internal, skeleton->skin, slotIndex, slotBone);
	if (skeleton->data->defaultSkin && skeleton->data->defaultSkin != skeleton->skin)
		_sortPathConstraintAttachment(internal, skeleton->data->defaultSkin, slotIndex, slotBone);
	for (i = 0, n = skeleton->data->skinsCount; i < n; i++)
		_sortPathConstraintAttachment(internal, skeleton->data->skins[i], slotIndex, slotBone);

	attachment = slot->attachment;
	if (attachment && attachment->type == SP_ATTACHMENT_PATH)
		_sortPathConstraintAttachmentBones(internal, attachment, slotBone);

	constrained = constraint->bones;
	boneCount = constraint->bonesCount;
	for (i = 0; i < boneCount; i++)
		_sortBone(internal, constrained[i]);

	_addToUpdateCache(internal, SP_UPDATE_PATH_CONSTRAINT, constraint);

	for (i = 0; i < boneCount; i++)
		_sortReset(constrained[i]->children, constrained[i]->childrenCount);
	for (i = 0; i < boneCount; i++)
		constrained[i]->sorted = 1;
}

static void _sortTransformConstraint(_topi40_spSkeleton *const internal, topi40_spTransformConstraint *constraint) {
	int i, boneCount;
	topi40_spBone **constrained;
	topi40_spBone *child;

	constraint->active = constraint->target->active && (!constraint->data->skinRequired || (internal->super.skin != 0 &&
																							topi40_spTransformConstraintDataArray_contains(
																									internal->super.skin->transformConstraints,
																									constraint->data)));
	if (!constraint->active) return;

	_sortBone(internal, constraint->target);

	constrained = constraint->bones;
	boneCount = constraint->bonesCount;
	if (constraint->data->local) {
		for (i = 0; i < boneCount; i++) {
			child = constrained[i];
			_sortBone(internal, child->parent);
			_sortBone(internal, child);
		}
	} else {
		for (i = 0; i < boneCount; i++)
			_sortBone(internal, constrained[i]);
	}

	_addToUpdateCache(internal, SP_UPDATE_TRANSFORM_CONSTRAINT, constraint);

	for (i = 0; i < boneCount; i++)
		_sortReset(constrained[i]->children, constrained[i]->childrenCount);
	for (i = 0; i < boneCount; i++)
		constrained[i]->sorted = 1;
}

void topi40_spSkeleton_updateCache(topi40_spSkeleton *self) {
	int i, ii;
	topi40_spBone **bones;
	topi40_spIkConstraint **ikConstraints;
	topi40_spPathConstraint **pathConstraints;
	topi40_spTransformConstraint **transformConstraints;
	int ikCount, transformCount, pathCount, constraintCount;
	_topi40_spSkeleton *internal = SUB_CAST(_topi40_spSkeleton, self);

	internal->updateCacheCapacity =
			self->bonesCount + self->ikConstraintsCount + self->transformConstraintsCount + self->pathConstraintsCount;
	FREE(internal->updateCache);
	internal->updateCache = MALLOC(_topi40_spUpdate, internal->updateCacheCapacity);
	internal->updateCacheCount = 0;

	bones = self->bones;
	for (i = 0; i < self->bonesCount; ++i) {
		topi40_spBone *bone = bones[i];
		bone->sorted = bone->data->skinRequired;
		bone->active = !bone->sorted;
	}

	if (self->skin) {
		topi40_spBoneDataArray *skinBones = self->skin->bones;
		for (i = 0; i < skinBones->size; i++) {
			topi40_spBone *bone = self->bones[skinBones->items[i]->index];
			do {
				bone->sorted = 0;
				bone->active = -1;
				bone = bone->parent;
			} while (bone != 0);
		}
	}

	/* IK first, lowest hierarchy depth first. */
	ikConstraints = self->ikConstraints;
	transformConstraints = self->transformConstraints;
	pathConstraints = self->pathConstraints;
	ikCount = self->ikConstraintsCount;
	transformCount = self->transformConstraintsCount;
	pathCount = self->pathConstraintsCount;
	constraintCount = ikCount + transformCount + pathCount;

	i = 0;
continue_outer:
	for (; i < constraintCount; i++) {
		for (ii = 0; ii < ikCount; ii++) {
			topi40_spIkConstraint *ikConstraint = ikConstraints[ii];
			if (ikConstraint->data->order == i) {
				_sortIkConstraint(internal, ikConstraint);
				i++;
				goto continue_outer;
			}
		}

		for (ii = 0; ii < transformCount; ii++) {
			topi40_spTransformConstraint *transformConstraint = transformConstraints[ii];
			if (transformConstraint->data->order == i) {
				_sortTransformConstraint(internal, transformConstraint);
				i++;
				goto continue_outer;
			}
		}

		for (ii = 0; ii < pathCount; ii++) {
			topi40_spPathConstraint *pathConstraint = pathConstraints[ii];
			if (pathConstraint->data->order == i) {
				_sortPathConstraint(internal, pathConstraint);
				i++;
				goto continue_outer;
			}
		}
	}

	for (i = 0; i < self->bonesCount; ++i)
		_sortBone(internal, self->bones[i]);
}

void topi40_spSkeleton_updateWorldTransform(const topi40_spSkeleton *self) {
	int i, n;
	_topi40_spSkeleton *internal = SUB_CAST(_topi40_spSkeleton, self);

	for (i = 0, n = self->bonesCount; i < n; i++) {
		topi40_spBone *bone = self->bones[i];
		bone->ax = bone->x;
		bone->ay = bone->y;
		bone->arotation = bone->rotation;
		bone->ascaleX = bone->scaleX;
		bone->ascaleY = bone->scaleY;
		bone->ashearX = bone->shearX;
		bone->ashearY = bone->shearY;
	}

	for (i = 0; i < internal->updateCacheCount; ++i) {
		_topi40_spUpdate *update = internal->updateCache + i;
		switch (update->type) {
			case SP_UPDATE_BONE:
				topi40_spBone_update((topi40_spBone *) update->object);
				break;
			case SP_UPDATE_IK_CONSTRAINT:
				topi40_spIkConstraint_update((topi40_spIkConstraint *) update->object);
				break;
			case SP_UPDATE_TRANSFORM_CONSTRAINT:
				topi40_spTransformConstraint_update((topi40_spTransformConstraint *) update->object);
				break;
			case SP_UPDATE_PATH_CONSTRAINT:
				topi40_spPathConstraint_update((topi40_spPathConstraint *) update->object);
				break;
		}
	}
}

void topi40_spSkeleton_updateWorldTransformWith(const topi40_spSkeleton *self, const topi40_spBone *parent) {
	/* Apply the parent bone transform to the root bone. The root bone always inherits scale, rotation and reflection. */
	int i;
	float rotationY, la, lb, lc, ld;
	_topi40_spUpdate *updateCache;
	_topi40_spSkeleton *internal = SUB_CAST(_topi40_spSkeleton, self);
	topi40_spBone *rootBone = self->root;
	float pa = parent->a, pb = parent->b, pc = parent->c, pd = parent->d;
	CONST_CAST(float, rootBone->worldX) = pa * self->x + pb * self->y + parent->worldX;
	CONST_CAST(float, rootBone->worldY) = pc * self->x + pd * self->y + parent->worldY;

	rotationY = rootBone->rotation + 90 + rootBone->shearY;
	la = COS_DEG(rootBone->rotation + rootBone->shearX) * rootBone->scaleX;
	lb = COS_DEG(rotationY) * rootBone->scaleY;
	lc = SIN_DEG(rootBone->rotation + rootBone->shearX) * rootBone->scaleX;
	ld = SIN_DEG(rotationY) * rootBone->scaleY;
	CONST_CAST(float, rootBone->a) = (pa * la + pb * lc) * self->scaleX;
	CONST_CAST(float, rootBone->b) = (pa * lb + pb * ld) * self->scaleX;
	CONST_CAST(float, rootBone->c) = (pc * la + pd * lc) * self->scaleY;
	CONST_CAST(float, rootBone->d) = (pc * lb + pd * ld) * self->scaleY;

	/* Update everything except root bone. */
	updateCache = internal->updateCache;
	for (i = 0; i < internal->updateCacheCount; ++i) {
		_topi40_spUpdate *update = internal->updateCache + i;
		switch (update->type) {
			case SP_UPDATE_BONE:
				if ((topi40_spBone *) update->object != rootBone) topi40_spBone_updateWorldTransform((topi40_spBone *) update->object);
				break;
			case SP_UPDATE_IK_CONSTRAINT:
				topi40_spIkConstraint_update((topi40_spIkConstraint *) update->object);
				break;
			case SP_UPDATE_TRANSFORM_CONSTRAINT:
				topi40_spTransformConstraint_update((topi40_spTransformConstraint *) update->object);
				break;
			case SP_UPDATE_PATH_CONSTRAINT:
				topi40_spPathConstraint_update((topi40_spPathConstraint *) update->object);
				break;
		}
	}
}

void topi40_spSkeleton_setToSetupPose(const topi40_spSkeleton *self) {
	topi40_spSkeleton_setBonesToSetupPose(self);
	topi40_spSkeleton_setSlotsToSetupPose(self);
}

void topi40_spSkeleton_setBonesToSetupPose(const topi40_spSkeleton *self) {
	int i;
	for (i = 0; i < self->bonesCount; ++i)
		topi40_spBone_setToSetupPose(self->bones[i]);

	for (i = 0; i < self->ikConstraintsCount; ++i) {
		topi40_spIkConstraint *ikConstraint = self->ikConstraints[i];
		ikConstraint->bendDirection = ikConstraint->data->bendDirection;
		ikConstraint->compress = ikConstraint->data->compress;
		ikConstraint->stretch = ikConstraint->data->stretch;
		ikConstraint->softness = ikConstraint->data->softness;
		ikConstraint->mix = ikConstraint->data->mix;
	}

	for (i = 0; i < self->transformConstraintsCount; ++i) {
		topi40_spTransformConstraint *constraint = self->transformConstraints[i];
		topi40_spTransformConstraintData *data = constraint->data;
		constraint->mixRotate = data->mixRotate;
		constraint->mixX = data->mixX;
		constraint->mixY = data->mixY;
		constraint->mixScaleX = data->mixScaleX;
		constraint->mixScaleY = data->mixScaleY;
		constraint->mixShearY = data->mixShearY;
	}

	for (i = 0; i < self->pathConstraintsCount; ++i) {
		topi40_spPathConstraint *constraint = self->pathConstraints[i];
		topi40_spPathConstraintData *data = constraint->data;
		constraint->position = data->position;
		constraint->topi40_spacing = data->topi40_spacing;
		constraint->mixRotate = data->mixRotate;
		constraint->mixX = data->mixX;
		constraint->mixY = data->mixY;
	}
}

void topi40_spSkeleton_setSlotsToSetupPose(const topi40_spSkeleton *self) {
	int i;
	memcpy(self->drawOrder, self->slots, self->slotsCount * sizeof(topi40_spSlot *));
	for (i = 0; i < self->slotsCount; ++i)
		topi40_spSlot_setToSetupPose(self->slots[i]);
}

topi40_spBone *topi40_spSkeleton_findBone(const topi40_spSkeleton *self, const char *boneName) {
	int i;
	for (i = 0; i < self->bonesCount; ++i)
		if (strcmp(self->data->bones[i]->name, boneName) == 0) return self->bones[i];
	return 0;
}

topi40_spSlot *topi40_spSkeleton_findSlot(const topi40_spSkeleton *self, const char *slotName) {
	int i;
	for (i = 0; i < self->slotsCount; ++i)
		if (strcmp(self->data->slots[i]->name, slotName) == 0) return self->slots[i];
	return 0;
}

int topi40_spSkeleton_setSkinByName(topi40_spSkeleton *self, const char *skinName) {
	topi40_spSkin *skin;
	if (!skinName) {
		topi40_spSkeleton_setSkin(self, 0);
		return 1;
	}
	skin = topi40_spSkeletonData_findSkin(self->data, skinName);
	if (!skin) return 0;
	topi40_spSkeleton_setSkin(self, skin);
	return 1;
}

void topi40_spSkeleton_setSkin(topi40_spSkeleton *self, topi40_spSkin *newSkin) {
	if (self->skin == newSkin) return;
	if (newSkin) {
		if (self->skin)
			topi40_spSkin_attachAll(newSkin, self, self->skin);
		else {
			/* No previous skin, attach setup pose attachments. */
			int i;
			for (i = 0; i < self->slotsCount; ++i) {
				topi40_spSlot *slot = self->slots[i];
				if (slot->data->attachmentName) {
					topi40_spAttachment *attachment = topi40_spSkin_getAttachment(newSkin, i, slot->data->attachmentName);
					if (attachment) topi40_spSlot_setAttachment(slot, attachment);
				}
			}
		}
	}
	CONST_CAST(topi40_spSkin *, self->skin) = newSkin;
	topi40_spSkeleton_updateCache(self);
}

topi40_spAttachment *
topi40_spSkeleton_getAttachmentForSlotName(const topi40_spSkeleton *self, const char *slotName, const char *attachmentName) {
	int slotIndex = topi40_spSkeletonData_findSlot(self->data, slotName)->index;
	return topi40_spSkeleton_getAttachmentForSlotIndex(self, slotIndex, attachmentName);
}

topi40_spAttachment *topi40_spSkeleton_getAttachmentForSlotIndex(const topi40_spSkeleton *self, int slotIndex, const char *attachmentName) {
	if (slotIndex == -1) return 0;
	if (self->skin) {
		topi40_spAttachment *attachment = topi40_spSkin_getAttachment(self->skin, slotIndex, attachmentName);
		if (attachment) return attachment;
	}
	if (self->data->defaultSkin) {
		topi40_spAttachment *attachment = topi40_spSkin_getAttachment(self->data->defaultSkin, slotIndex, attachmentName);
		if (attachment) return attachment;
	}
	return 0;
}

int topi40_spSkeleton_setAttachment(topi40_spSkeleton *self, const char *slotName, const char *attachmentName) {
	int i;
	for (i = 0; i < self->slotsCount; ++i) {
		topi40_spSlot *slot = self->slots[i];
		if (strcmp(slot->data->name, slotName) == 0) {
			if (!attachmentName)
				topi40_spSlot_setAttachment(slot, 0);
			else {
				topi40_spAttachment *attachment = topi40_spSkeleton_getAttachmentForSlotIndex(self, i, attachmentName);
				if (!attachment) return 0;
				topi40_spSlot_setAttachment(slot, attachment);
			}
			return 1;
		}
	}
	return 0;
}

topi40_spIkConstraint *topi40_spSkeleton_findIkConstraint(const topi40_spSkeleton *self, const char *constraintName) {
	int i;
	for (i = 0; i < self->ikConstraintsCount; ++i)
		if (strcmp(self->ikConstraints[i]->data->name, constraintName) == 0) return self->ikConstraints[i];
	return 0;
}

topi40_spTransformConstraint *topi40_spSkeleton_findTransformConstraint(const topi40_spSkeleton *self, const char *constraintName) {
	int i;
	for (i = 0; i < self->transformConstraintsCount; ++i)
		if (strcmp(self->transformConstraints[i]->data->name, constraintName) == 0)
			return self->transformConstraints[i];
	return 0;
}

topi40_spPathConstraint *topi40_spSkeleton_findPathConstraint(const topi40_spSkeleton *self, const char *constraintName) {
	int i;
	for (i = 0; i < self->pathConstraintsCount; ++i)
		if (strcmp(self->pathConstraints[i]->data->name, constraintName) == 0) return self->pathConstraints[i];
	return 0;
}

void topi40_spSkeleton_update(topi40_spSkeleton *self, float deltaTime) {
	self->time += deltaTime;
}
