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

#ifndef SPINE_SKELETONBINARY_H_
#define SPINE_SKELETONBINARY_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Attachment.h>
#include <topi40_spine/AttachmentLoader.h>
#include <topi40_spine/SkeletonData.h>
#include <topi40_spine/Atlas.h>

#ifdef __cplusplus
extern "C" {
#endif

struct topi40_spAtlasAttachmentLoader;

typedef struct topi40_spSkeletonBinary {
	float scale;
	topi40_spAttachmentLoader *attachmentLoader;
	const char *const error;
} topi40_spSkeletonBinary;

SP_API topi40_spSkeletonBinary *topi40_spSkeletonBinary_createWithLoader(topi40_spAttachmentLoader *attachmentLoader);

SP_API topi40_spSkeletonBinary *topi40_spSkeletonBinary_create(topi40_spAtlas *atlas);

SP_API void topi40_spSkeletonBinary_dispose(topi40_spSkeletonBinary *self);

SP_API topi40_spSkeletonData *
topi40_spSkeletonBinary_readSkeletonData(topi40_spSkeletonBinary *self, const unsigned char *binary, const int length);

SP_API topi40_spSkeletonData *topi40_spSkeletonBinary_readSkeletonDataFile(topi40_spSkeletonBinary *self, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SKELETONBINARY_H_ */
