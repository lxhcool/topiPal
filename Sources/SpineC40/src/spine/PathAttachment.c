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

#include <topi40_spine/PathAttachment.h>
#include <topi40_spine/extension.h>

void _topi40_spPathAttachment_dispose(topi40_spAttachment *attachment) {
	topi40_spPathAttachment *self = SUB_CAST(topi40_spPathAttachment, attachment);

	_topi40_spVertexAttachment_deinit(SUPER(self));

	FREE(self->lengths);
	FREE(self);
}

topi40_spAttachment *_topi40_spPathAttachment_copy(topi40_spAttachment *attachment) {
	topi40_spPathAttachment *copy = topi40_spPathAttachment_create(attachment->name);
	topi40_spPathAttachment *self = SUB_CAST(topi40_spPathAttachment, attachment);
	topi40_spVertexAttachment_copyTo(SUPER(self), SUPER(copy));
	copy->lengthsLength = self->lengthsLength;
	copy->lengths = MALLOC(float, self->lengthsLength);
	memcpy(copy->lengths, self->lengths, self->lengthsLength * sizeof(float));
	copy->closed = self->closed;
	copy->constantSpeed = self->constantSpeed;
	return SUPER(SUPER(copy));
}

topi40_spPathAttachment *topi40_spPathAttachment_create(const char *name) {
	topi40_spPathAttachment *self = NEW(topi40_spPathAttachment);
	_topi40_spVertexAttachment_init(SUPER(self));
	_topi40_spAttachment_init(SUPER(SUPER(self)), name, SP_ATTACHMENT_PATH, _topi40_spPathAttachment_dispose, _topi40_spPathAttachment_copy);
	return self;
}
