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

#include <topi40_spine/BoundingBoxAttachment.h>
#include <topi40_spine/extension.h>

void _topi40_spBoundingBoxAttachment_dispose(topi40_spAttachment *attachment) {
	topi40_spBoundingBoxAttachment *self = SUB_CAST(topi40_spBoundingBoxAttachment, attachment);

	_topi40_spVertexAttachment_deinit(SUPER(self));

	FREE(self);
}

topi40_spAttachment *_topi40_spBoundingBoxAttachment_copy(topi40_spAttachment *attachment) {
	topi40_spBoundingBoxAttachment *copy = topi40_spBoundingBoxAttachment_create(attachment->name);
	topi40_spBoundingBoxAttachment *self = SUB_CAST(topi40_spBoundingBoxAttachment, attachment);
	topi40_spVertexAttachment_copyTo(SUPER(self), SUPER(copy));
	return SUPER(SUPER(copy));
}

topi40_spBoundingBoxAttachment *topi40_spBoundingBoxAttachment_create(const char *name) {
	topi40_spBoundingBoxAttachment *self = NEW(topi40_spBoundingBoxAttachment);
	_topi40_spVertexAttachment_init(SUPER(self));
	_topi40_spAttachment_init(SUPER(SUPER(self)), name, SP_ATTACHMENT_BOUNDING_BOX, _topi40_spBoundingBoxAttachment_dispose,
					   _topi40_spBoundingBoxAttachment_copy);
	return self;
}
