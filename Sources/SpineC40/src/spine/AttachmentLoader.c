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

#include <topi40_spine/AttachmentLoader.h>
#include <topi40_spine/extension.h>
#include <stdio.h>

typedef struct _topi40_spAttachmentLoaderVtable {
	topi40_spAttachment *(*createAttachment)(topi40_spAttachmentLoader *self, topi40_spSkin *skin, topi40_spAttachmentType type, const char *name,
									  const char *path);

	void (*configureAttachment)(topi40_spAttachmentLoader *self, topi40_spAttachment *);

	void (*disposeAttachment)(topi40_spAttachmentLoader *self, topi40_spAttachment *);

	void (*dispose)(topi40_spAttachmentLoader *self);
} _topi40_spAttachmentLoaderVtable;

void _topi40_spAttachmentLoader_init(topi40_spAttachmentLoader *self,
							  void (*dispose)(topi40_spAttachmentLoader *self),
							  topi40_spAttachment *(*createAttachment)(topi40_spAttachmentLoader *self, topi40_spSkin *skin,
																topi40_spAttachmentType type, const char *name,
																const char *path),
							  void (*configureAttachment)(topi40_spAttachmentLoader *self, topi40_spAttachment *),
							  void (*disposeAttachment)(topi40_spAttachmentLoader *self, topi40_spAttachment *)) {
	CONST_CAST(_topi40_spAttachmentLoaderVtable *, self->vtable) = NEW(_topi40_spAttachmentLoaderVtable);
	VTABLE(topi40_spAttachmentLoader, self)->dispose = dispose;
	VTABLE(topi40_spAttachmentLoader, self)->createAttachment = createAttachment;
	VTABLE(topi40_spAttachmentLoader, self)->configureAttachment = configureAttachment;
	VTABLE(topi40_spAttachmentLoader, self)->disposeAttachment = disposeAttachment;
}

void _topi40_spAttachmentLoader_deinit(topi40_spAttachmentLoader *self) {
	FREE(self->vtable);
	FREE(self->error1);
	FREE(self->error2);
}

void topi40_spAttachmentLoader_dispose(topi40_spAttachmentLoader *self) {
	VTABLE(topi40_spAttachmentLoader, self)->dispose(self);
	FREE(self);
}

topi40_spAttachment *
topi40_spAttachmentLoader_createAttachment(topi40_spAttachmentLoader *self, topi40_spSkin *skin, topi40_spAttachmentType type, const char *name,
									const char *path) {
	FREE(self->error1);
	FREE(self->error2);
	self->error1 = 0;
	self->error2 = 0;
	return VTABLE(topi40_spAttachmentLoader, self)->createAttachment(self, skin, type, name, path);
}

void topi40_spAttachmentLoader_configureAttachment(topi40_spAttachmentLoader *self, topi40_spAttachment *attachment) {
	if (!VTABLE(topi40_spAttachmentLoader, self)->configureAttachment) return;
	VTABLE(topi40_spAttachmentLoader, self)->configureAttachment(self, attachment);
}

void topi40_spAttachmentLoader_disposeAttachment(topi40_spAttachmentLoader *self, topi40_spAttachment *attachment) {
	if (!VTABLE(topi40_spAttachmentLoader, self)->disposeAttachment) return;
	VTABLE(topi40_spAttachmentLoader, self)->disposeAttachment(self, attachment);
}

void _topi40_spAttachmentLoader_setError(topi40_spAttachmentLoader *self, const char *error1, const char *error2) {
	FREE(self->error1);
	FREE(self->error2);
	MALLOC_STR(self->error1, error1);
	MALLOC_STR(self->error2, error2);
}

void _topi40_spAttachmentLoader_setUnknownTypeError(topi40_spAttachmentLoader *self, topi40_spAttachmentType type) {
	char buffer[16];
	sprintf(buffer, "%d", type);
	_topi40_spAttachmentLoader_setError(self, "Unknown attachment type: ", buffer);
}
