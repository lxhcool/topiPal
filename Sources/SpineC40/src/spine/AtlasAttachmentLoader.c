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

#include <topi40_spine/AtlasAttachmentLoader.h>
#include <topi40_spine/extension.h>

topi40_spAttachment *_topi40_spAtlasAttachmentLoader_createAttachment(topi40_spAttachmentLoader *loader, topi40_spSkin *skin, topi40_spAttachmentType type,
														const char *name, const char *path) {
	topi40_spAtlasAttachmentLoader *self = SUB_CAST(topi40_spAtlasAttachmentLoader, loader);
	switch (type) {
		case SP_ATTACHMENT_REGION: {
			topi40_spRegionAttachment *attachment;
			topi40_spAtlasRegion *region = topi40_spAtlas_findRegion(self->atlas, path);
			if (!region) {
				_topi40_spAttachmentLoader_setError(loader, "Region not found: ", path);
				return 0;
			}
			attachment = topi40_spRegionAttachment_create(name);
			attachment->rendererObject = region;
			topi40_spRegionAttachment_setUVs(attachment, region->u, region->v, region->u2, region->v2, region->degrees);
			attachment->regionOffsetX = region->offsetX;
			attachment->regionOffsetY = region->offsetY;
			attachment->regionWidth = region->width;
			attachment->regionHeight = region->height;
			attachment->regionOriginalWidth = region->originalWidth;
			attachment->regionOriginalHeight = region->originalHeight;
			return SUPER(attachment);
		}
		case SP_ATTACHMENT_MESH:
		case SP_ATTACHMENT_LINKED_MESH: {
			topi40_spMeshAttachment *attachment;
			topi40_spAtlasRegion *region = topi40_spAtlas_findRegion(self->atlas, path);
			if (!region) {
				_topi40_spAttachmentLoader_setError(loader, "Region not found: ", path);
				return 0;
			}
			attachment = topi40_spMeshAttachment_create(name);
			attachment->rendererObject = region;
			attachment->regionU = region->u;
			attachment->regionV = region->v;
			attachment->regionU2 = region->u2;
			attachment->regionV2 = region->v2;
			attachment->regionDegrees = region->degrees;
			attachment->regionOffsetX = region->offsetX;
			attachment->regionOffsetY = region->offsetY;
			attachment->regionWidth = region->width;
			attachment->regionHeight = region->height;
			attachment->regionOriginalWidth = region->originalWidth;
			attachment->regionOriginalHeight = region->originalHeight;
			return SUPER(SUPER(attachment));
		}
		case SP_ATTACHMENT_BOUNDING_BOX:
			return SUPER(SUPER(topi40_spBoundingBoxAttachment_create(name)));
		case SP_ATTACHMENT_PATH:
			return SUPER(SUPER(topi40_spPathAttachment_create(name)));
		case SP_ATTACHMENT_POINT:
			return SUPER(topi40_spPointAttachment_create(name));
		case SP_ATTACHMENT_CLIPPING:
			return SUPER(SUPER(topi40_spClippingAttachment_create(name)));
		default:
			_topi40_spAttachmentLoader_setUnknownTypeError(loader, type);
			return 0;
	}

	UNUSED(skin);
}

topi40_spAtlasAttachmentLoader *topi40_spAtlasAttachmentLoader_create(topi40_spAtlas *atlas) {
	topi40_spAtlasAttachmentLoader *self = NEW(topi40_spAtlasAttachmentLoader);
	_topi40_spAttachmentLoader_init(SUPER(self), _topi40_spAttachmentLoader_deinit, _topi40_spAtlasAttachmentLoader_createAttachment, 0, 0);
	self->atlas = atlas;
	return self;
}
