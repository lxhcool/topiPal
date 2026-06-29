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

#ifndef SPINE_SLOT_H_
#define SPINE_SLOT_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Bone.h>
#include <topi40_spine/Attachment.h>
#include <topi40_spine/SlotData.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spSlot {
	topi40_spSlotData *const data;
	topi40_spBone *const bone;
	topi40_spColor color;
	topi40_spColor *darkColor;
	topi40_spAttachment *attachment;
	int attachmentState;

	int deformCapacity;
	int deformCount;
	float *deform;
} topi40_spSlot;

SP_API topi40_spSlot *topi40_spSlot_create(topi40_spSlotData *data, topi40_spBone *bone);

SP_API void topi40_spSlot_dispose(topi40_spSlot *self);

/* @param attachment May be 0 to clear the attachment for the slot. */
SP_API void topi40_spSlot_setAttachment(topi40_spSlot *self, topi40_spAttachment *attachment);

SP_API void topi40_spSlot_setAttachmentTime(topi40_spSlot *self, float time);

SP_API float topi40_spSlot_getAttachmentTime(const topi40_spSlot *self);

SP_API void topi40_spSlot_setToSetupPose(topi40_spSlot *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SLOT_H_ */
