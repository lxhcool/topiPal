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

#include <topi40_spine/Attachment.h>
#include <topi40_spine/Slot.h>
#include <topi40_spine/extension.h>

typedef struct _topi40_spAttachmentVtable {
	void (*dispose)(topi40_spAttachment *self);

	topi40_spAttachment *(*copy)(topi40_spAttachment *self);
} _topi40_spAttachmentVtable;

void _topi40_spAttachment_init(topi40_spAttachment *self, const char *name, topi40_spAttachmentType type, /**/
						void (*dispose)(topi40_spAttachment *self), topi40_spAttachment *(*copy)(topi40_spAttachment *self)) {

	CONST_CAST(_topi40_spAttachmentVtable *, self->vtable) = NEW(_topi40_spAttachmentVtable);
	VTABLE(topi40_spAttachment, self)->dispose = dispose;
	VTABLE(topi40_spAttachment, self)->copy = copy;

	MALLOC_STR(self->name, name);
	CONST_CAST(topi40_spAttachmentType, self->type) = type;
}

void _topi40_spAttachment_deinit(topi40_spAttachment *self) {
	if (self->attachmentLoader) topi40_spAttachmentLoader_disposeAttachment(self->attachmentLoader, self);
	FREE(self->vtable);
	FREE(self->name);
}

topi40_spAttachment *topi40_spAttachment_copy(topi40_spAttachment *self) {
	return VTABLE(topi40_spAttachment, self)->copy(self);
}

void topi40_spAttachment_dispose(topi40_spAttachment *self) {
	self->refCount--;
	if (self->refCount <= 0)
		VTABLE(topi40_spAttachment, self)->dispose(self);
}
