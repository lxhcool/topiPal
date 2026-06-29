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

#ifndef SPINE_VERTEXATTACHMENT_H_
#define SPINE_VERTEXATTACHMENT_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Attachment.h>
#include <topi40_spine/Slot.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spVertexAttachment topi40_spVertexAttachment;
struct topi40_spVertexAttachment {
	topi40_spAttachment super;

	int bonesCount;
	int *bones;

	int verticesCount;
	float *vertices;

	int worldVerticesLength;

	topi40_spVertexAttachment *deformAttachment;

	int id;
};

SP_API void topi40_spVertexAttachment_computeWorldVertices(topi40_spVertexAttachment *self, topi40_spSlot *slot, int start, int count,
													float *worldVertices, int offset, int stride);

void topi40_spVertexAttachment_copyTo(topi40_spVertexAttachment *self, topi40_spVertexAttachment *other);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_VERTEXATTACHMENT_H_ */
