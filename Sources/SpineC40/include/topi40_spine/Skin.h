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

#ifndef SPINE_SKIN_H_
#define SPINE_SKIN_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Attachment.h>
#include <topi40_spine/IkConstraintData.h>
#include <topi40_spine/TransformConstraintData.h>
#include <topi40_spine/PathConstraintData.h>
#include <topi40_spine/Array.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Size of hashtable used in skin structure for fast attachment lookup. */
#define SKIN_ENTRIES_HASH_TABLE_SIZE 100

struct topi40_spSkeleton;

_SP_ARRAY_DECLARE_TYPE(topi40_spBoneDataArray, topi40_spBoneData*)

_SP_ARRAY_DECLARE_TYPE(topi40_spIkConstraintDataArray, topi40_spIkConstraintData*)

_SP_ARRAY_DECLARE_TYPE(topi40_spTransformConstraintDataArray, topi40_spTransformConstraintData*)

_SP_ARRAY_DECLARE_TYPE(topi40_spPathConstraintDataArray, topi40_spPathConstraintData*)

typedef struct topi40_spSkin {
	const char *const name;

	topi40_spBoneDataArray *bones;
	topi40_spIkConstraintDataArray *ikConstraints;
	topi40_spTransformConstraintDataArray *transformConstraints;
	topi40_spPathConstraintDataArray *pathConstraints;
} topi40_spSkin;

/* Private structs, needed by Skeleton */
typedef struct _topi40_Entry _topi40_Entry;
typedef struct _topi40_Entry topi40_spSkinEntry;
struct _topi40_Entry {
	int slotIndex;
	const char *name;
	topi40_spAttachment *attachment;
	_topi40_Entry *next;
};

typedef struct _SkinHashTableEntry _SkinHashTableEntry;
struct _SkinHashTableEntry {
	_topi40_Entry *entry;
	_SkinHashTableEntry *next;
};

typedef struct {
	topi40_spSkin super;
	_topi40_Entry *entries; /* entries list stored for getting attachment name by attachment index */
	_SkinHashTableEntry *entriesHashTable[SKIN_ENTRIES_HASH_TABLE_SIZE]; /* hashtable for fast attachment lookup */
} _topi40_spSkin;

SP_API topi40_spSkin *topi40_spSkin_create(const char *name);

SP_API void topi40_spSkin_dispose(topi40_spSkin *self);

/* The Skin owns the attachment. */
SP_API void topi40_spSkin_setAttachment(topi40_spSkin *self, int slotIndex, const char *name, topi40_spAttachment *attachment);
/* Returns 0 if the attachment was not found. */
SP_API topi40_spAttachment *topi40_spSkin_getAttachment(const topi40_spSkin *self, int slotIndex, const char *name);

/* Returns 0 if the slot or attachment was not found. */
SP_API const char *topi40_spSkin_getAttachmentName(const topi40_spSkin *self, int slotIndex, int attachmentIndex);

/** Attach each attachment in this skin if the corresponding attachment in oldSkin is currently attached. */
SP_API void topi40_spSkin_attachAll(const topi40_spSkin *self, struct topi40_spSkeleton *skeleton, const topi40_spSkin *oldspSkin);

/** Adds all attachments, bones, and constraints from the topi40_specified skin to this skin. */
SP_API void topi40_spSkin_addSkin(topi40_spSkin *self, const topi40_spSkin *other);

/** Adds all attachments, bones, and constraints from the topi40_specified skin to this skin. Attachments are deep copied. */
SP_API void topi40_spSkin_copySkin(topi40_spSkin *self, const topi40_spSkin *other);

/** Returns all attachments in this skin. */
SP_API topi40_spSkinEntry *topi40_spSkin_getAttachments(const topi40_spSkin *self);

/** Clears all attachments, bones, and constraints. */
SP_API void topi40_spSkin_clear(topi40_spSkin *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SKIN_H_ */
