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

#include <topi40_spine/Slot.h>
#include <topi40_spine/extension.h>

typedef struct {
	topi40_spSlot super;
	float attachmentTime;
} _topi40_spSlot;

topi40_spSlot *topi40_spSlot_create(topi40_spSlotData *data, topi40_spBone *bone) {
	topi40_spSlot *self = SUPER(NEW(_topi40_spSlot));
	CONST_CAST(topi40_spSlotData *, self->data) = data;
	CONST_CAST(topi40_spBone *, self->bone) = bone;
	topi40_spColor_setFromFloats(&self->color, 1, 1, 1, 1);
	self->darkColor = data->darkColor == 0 ? 0 : topi40_spColor_create();
	topi40_spSlot_setToSetupPose(self);
	return self;
}

void topi40_spSlot_dispose(topi40_spSlot *self) {
	FREE(self->deform);
	FREE(self->darkColor);
	FREE(self);
}

static int isVertexAttachment(topi40_spAttachment *attachment) {
	if (attachment == NULL) return 0;
	switch (attachment->type) {
		case SP_ATTACHMENT_BOUNDING_BOX:
		case SP_ATTACHMENT_CLIPPING:
		case SP_ATTACHMENT_MESH:
		case SP_ATTACHMENT_PATH:
			return -1;
		default:
			return 0;
	}
}

void topi40_spSlot_setAttachment(topi40_spSlot *self, topi40_spAttachment *attachment) {
	if (attachment == self->attachment) return;

	if (!isVertexAttachment(attachment) ||
		!isVertexAttachment(self->attachment) || (SUB_CAST(topi40_spVertexAttachment, attachment)->deformAttachment != SUB_CAST(topi40_spVertexAttachment, self->attachment)->deformAttachment)) {
		self->deformCount = 0;
	}

	CONST_CAST(topi40_spAttachment *, self->attachment) = attachment;
	SUB_CAST(_topi40_spSlot, self)->attachmentTime = self->bone->skeleton->time;
}

void topi40_spSlot_setAttachmentTime(topi40_spSlot *self, float time) {
	SUB_CAST(_topi40_spSlot, self)->attachmentTime = self->bone->skeleton->time - time;
}

float topi40_spSlot_getAttachmentTime(const topi40_spSlot *self) {
	return self->bone->skeleton->time - SUB_CAST(_topi40_spSlot, self)->attachmentTime;
}

void topi40_spSlot_setToSetupPose(topi40_spSlot *self) {
	topi40_spColor_setFromColor(&self->color, &self->data->color);
	if (self->darkColor) topi40_spColor_setFromColor(self->darkColor, self->data->darkColor);

	if (!self->data->attachmentName)
		topi40_spSlot_setAttachment(self, 0);
	else {
		topi40_spAttachment *attachment = topi40_spSkeleton_getAttachmentForSlotIndex(
				self->bone->skeleton, self->data->index, self->data->attachmentName);
		CONST_CAST(topi40_spAttachment *, self->attachment) = 0;
		topi40_spSlot_setAttachment(self, attachment);
	}
}
