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

#include <topi40_spine/Skin.h>
#include <topi40_spine/extension.h>
#include <stdio.h>

_SP_ARRAY_IMPLEMENT_TYPE(topi40_spBoneDataArray, topi40_spBoneData *)

_SP_ARRAY_IMPLEMENT_TYPE(topi40_spIkConstraintDataArray, topi40_spIkConstraintData *)

_SP_ARRAY_IMPLEMENT_TYPE(topi40_spTransformConstraintDataArray, topi40_spTransformConstraintData *)

_SP_ARRAY_IMPLEMENT_TYPE(topi40_spPathConstraintDataArray, topi40_spPathConstraintData *)

_topi40_Entry *topi40_Entry_create(int slotIndex, const char *name, topi40_spAttachment *attachment) {
	_topi40_Entry *self = NEW(_topi40_Entry);
	self->slotIndex = slotIndex;
	MALLOC_STR(self->name, name);
	self->attachment = attachment;
	return self;
}

void topi40_Entry_dispose(_topi40_Entry *self) {
	topi40_spAttachment_dispose(self->attachment);
	FREE(self->name);
	FREE(self);
}

static _topi40_SkinHashTableEntry *_topi40_SkinHashTableEntry_create(_topi40_Entry *entry) {
	_topi40_SkinHashTableEntry *self = NEW(_topi40_SkinHashTableEntry);
	self->entry = entry;
	return self;
}

static void _topi40_SkinHashTableEntry_dispose(_topi40_SkinHashTableEntry *self) {
	FREE(self);
}

/**/

topi40_spSkin *topi40_spSkin_create(const char *name) {
	topi40_spSkin *self = SUPER(NEW(_topi40_spSkin));
	MALLOC_STR(self->name, name);
	self->bones = topi40_spBoneDataArray_create(4);
	self->ikConstraints = topi40_spIkConstraintDataArray_create(4);
	self->transformConstraints = topi40_spTransformConstraintDataArray_create(4);
	self->pathConstraints = topi40_spPathConstraintDataArray_create(4);
	return self;
}

void topi40_spSkin_dispose(topi40_spSkin *self) {
	_topi40_Entry *entry = SUB_CAST(_topi40_spSkin, self)->entries;

	while (entry) {
		_topi40_Entry *nextEntry = entry->next;
		topi40_Entry_dispose(entry);
		entry = nextEntry;
	}

	{
		_topi40_SkinHashTableEntry **currentHashtableEntry = SUB_CAST(_topi40_spSkin, self)->entriesHashTable;
		int i;

		for (i = 0; i < SKIN_ENTRIES_HASH_TABLE_SIZE; ++i, ++currentHashtableEntry) {
			_topi40_SkinHashTableEntry *hashtableEntry = *currentHashtableEntry;

			while (hashtableEntry) {
				_topi40_SkinHashTableEntry *nextEntry = hashtableEntry->next;
				_topi40_SkinHashTableEntry_dispose(hashtableEntry);
				hashtableEntry = nextEntry;
			}
		}
	}

	topi40_spBoneDataArray_dispose(self->bones);
	topi40_spIkConstraintDataArray_dispose(self->ikConstraints);
	topi40_spTransformConstraintDataArray_dispose(self->transformConstraints);
	topi40_spPathConstraintDataArray_dispose(self->pathConstraints);
	FREE(self->name);
	FREE(self);
}

void topi40_spSkin_setAttachment(topi40_spSkin *self, int slotIndex, const char *name, topi40_spAttachment *attachment) {
	_topi40_SkinHashTableEntry *existingEntry = 0;
	_topi40_SkinHashTableEntry *hashEntry = SUB_CAST(_topi40_spSkin, self)->entriesHashTable[(unsigned int) slotIndex % SKIN_ENTRIES_HASH_TABLE_SIZE];
	while (hashEntry) {
		if (hashEntry->entry->slotIndex == slotIndex && strcmp(hashEntry->entry->name, name) == 0) {
			existingEntry = hashEntry;
			break;
		}
		hashEntry = hashEntry->next;
	}

	if (attachment) attachment->refCount++;

	if (existingEntry) {
		if (hashEntry->entry->attachment) topi40_spAttachment_dispose(hashEntry->entry->attachment);
		hashEntry->entry->attachment = attachment;
	} else {
		_topi40_Entry *newEntry = topi40_Entry_create(slotIndex, name, attachment);
		newEntry->next = SUB_CAST(_topi40_spSkin, self)->entries;
		SUB_CAST(_topi40_spSkin, self)->entries = newEntry;
		{
			unsigned int hashTableIndex = (unsigned int) slotIndex % SKIN_ENTRIES_HASH_TABLE_SIZE;
			_topi40_SkinHashTableEntry **hashTable = SUB_CAST(_topi40_spSkin, self)->entriesHashTable;

			_topi40_SkinHashTableEntry *newHashEntry = _topi40_SkinHashTableEntry_create(newEntry);
			newHashEntry->next = hashTable[hashTableIndex];
			SUB_CAST(_topi40_spSkin, self)->entriesHashTable[hashTableIndex] = newHashEntry;
		}
	}
}

topi40_spAttachment *topi40_spSkin_getAttachment(const topi40_spSkin *self, int slotIndex, const char *name) {
	const _topi40_SkinHashTableEntry *hashEntry = SUB_CAST(_topi40_spSkin, self)->entriesHashTable[(unsigned int) slotIndex % SKIN_ENTRIES_HASH_TABLE_SIZE];
	while (hashEntry) {
		if (hashEntry->entry->slotIndex == slotIndex && strcmp(hashEntry->entry->name, name) == 0)
			return hashEntry->entry->attachment;
		hashEntry = hashEntry->next;
	}
	return 0;
}

const char *topi40_spSkin_getAttachmentName(const topi40_spSkin *self, int slotIndex, int attachmentIndex) {
	const _topi40_Entry *entry = SUB_CAST(_topi40_spSkin, self)->entries;
	int i = 0;
	while (entry) {
		if (entry->slotIndex == slotIndex) {
			if (i == attachmentIndex) return entry->name;
			i++;
		}
		entry = entry->next;
	}
	return 0;
}

void topi40_spSkin_attachAll(const topi40_spSkin *self, topi40_spSkeleton *skeleton, const topi40_spSkin *oldSkin) {
	const _topi40_Entry *entry = SUB_CAST(_topi40_spSkin, oldSkin)->entries;
	while (entry) {
		topi40_spSlot *slot = skeleton->slots[entry->slotIndex];
		if (slot->attachment == entry->attachment) {
			topi40_spAttachment *attachment = topi40_spSkin_getAttachment(self, entry->slotIndex, entry->name);
			if (attachment) topi40_spSlot_setAttachment(slot, attachment);
		}
		entry = entry->next;
	}
}

void topi40_spSkin_addSkin(topi40_spSkin *self, const topi40_spSkin *other) {
	int i = 0;
	topi40_spSkinEntry *entry;

	for (i = 0; i < other->bones->size; i++) {
		if (!topi40_spBoneDataArray_contains(self->bones, other->bones->items[i]))
			topi40_spBoneDataArray_add(self->bones, other->bones->items[i]);
	}

	for (i = 0; i < other->ikConstraints->size; i++) {
		if (!topi40_spIkConstraintDataArray_contains(self->ikConstraints, other->ikConstraints->items[i]))
			topi40_spIkConstraintDataArray_add(self->ikConstraints, other->ikConstraints->items[i]);
	}

	for (i = 0; i < other->transformConstraints->size; i++) {
		if (!topi40_spTransformConstraintDataArray_contains(self->transformConstraints, other->transformConstraints->items[i]))
			topi40_spTransformConstraintDataArray_add(self->transformConstraints, other->transformConstraints->items[i]);
	}

	for (i = 0; i < other->pathConstraints->size; i++) {
		if (!topi40_spPathConstraintDataArray_contains(self->pathConstraints, other->pathConstraints->items[i]))
			topi40_spPathConstraintDataArray_add(self->pathConstraints, other->pathConstraints->items[i]);
	}

	entry = topi40_spSkin_getAttachments(other);
	while (entry) {
		topi40_spSkin_setAttachment(self, entry->slotIndex, entry->name, entry->attachment);
		entry = entry->next;
	}
}

void topi40_spSkin_copySkin(topi40_spSkin *self, const topi40_spSkin *other) {
	int i = 0;
	topi40_spSkinEntry *entry;

	for (i = 0; i < other->bones->size; i++) {
		if (!topi40_spBoneDataArray_contains(self->bones, other->bones->items[i]))
			topi40_spBoneDataArray_add(self->bones, other->bones->items[i]);
	}

	for (i = 0; i < other->ikConstraints->size; i++) {
		if (!topi40_spIkConstraintDataArray_contains(self->ikConstraints, other->ikConstraints->items[i]))
			topi40_spIkConstraintDataArray_add(self->ikConstraints, other->ikConstraints->items[i]);
	}

	for (i = 0; i < other->transformConstraints->size; i++) {
		if (!topi40_spTransformConstraintDataArray_contains(self->transformConstraints, other->transformConstraints->items[i]))
			topi40_spTransformConstraintDataArray_add(self->transformConstraints, other->transformConstraints->items[i]);
	}

	for (i = 0; i < other->pathConstraints->size; i++) {
		if (!topi40_spPathConstraintDataArray_contains(self->pathConstraints, other->pathConstraints->items[i]))
			topi40_spPathConstraintDataArray_add(self->pathConstraints, other->pathConstraints->items[i]);
	}

	entry = topi40_spSkin_getAttachments(other);
	while (entry) {
		if (entry->attachment->type == SP_ATTACHMENT_MESH) {
			topi40_spMeshAttachment *attachment = topi40_spMeshAttachment_newLinkedMesh(
					SUB_CAST(topi40_spMeshAttachment, entry->attachment));
			topi40_spSkin_setAttachment(self, entry->slotIndex, entry->name, SUPER(SUPER(attachment)));
		} else {
			topi40_spAttachment *attachment = entry->attachment ? topi40_spAttachment_copy(entry->attachment) : 0;
			topi40_spSkin_setAttachment(self, entry->slotIndex, entry->name, attachment);
		}
		entry = entry->next;
	}
}

topi40_spSkinEntry *topi40_spSkin_getAttachments(const topi40_spSkin *self) {
	return SUB_CAST(_topi40_spSkin, self)->entries;
}

void topi40_spSkin_clear(topi40_spSkin *self) {
	_topi40_Entry *entry = SUB_CAST(_topi40_spSkin, self)->entries;

	while (entry) {
		_topi40_Entry *nextEntry = entry->next;
		topi40_Entry_dispose(entry);
		entry = nextEntry;
	}

	SUB_CAST(_topi40_spSkin, self)->entries = 0;

	{
		_topi40_SkinHashTableEntry **currentHashtableEntry = SUB_CAST(_topi40_spSkin, self)->entriesHashTable;
		int i;

		for (i = 0; i < SKIN_ENTRIES_HASH_TABLE_SIZE; ++i, ++currentHashtableEntry) {
			_topi40_SkinHashTableEntry *hashtableEntry = *currentHashtableEntry;

			while (hashtableEntry) {
				_topi40_SkinHashTableEntry *nextEntry = hashtableEntry->next;
				_topi40_SkinHashTableEntry_dispose(hashtableEntry);
				hashtableEntry = nextEntry;
			}

			SUB_CAST(_topi40_spSkin, self)->entriesHashTable[i] = 0;
		}
	}

	topi40_spBoneDataArray_clear(self->bones);
	topi40_spIkConstraintDataArray_clear(self->ikConstraints);
	topi40_spTransformConstraintDataArray_clear(self->transformConstraints);
	topi40_spPathConstraintDataArray_clear(self->pathConstraints);
}
