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

#include <topi40_spine/MeshAttachment.h>
#include <topi40_spine/extension.h>
#include <stdio.h>

void _topi40_spMeshAttachment_dispose(topi40_spAttachment *attachment) {
	topi40_spMeshAttachment *self = SUB_CAST(topi40_spMeshAttachment, attachment);
	FREE(self->path);
	FREE(self->uvs);
	if (!self->parentMesh) {
		_topi40_spVertexAttachment_deinit(SUPER(self));
		FREE(self->regionUVs);
		FREE(self->triangles);
		FREE(self->edges);
	} else
		_topi40_spAttachment_deinit(attachment);
	FREE(self);
}

topi40_spAttachment *_topi40_spMeshAttachment_copy(topi40_spAttachment *attachment) {
	topi40_spMeshAttachment *copy;
	topi40_spMeshAttachment *self = SUB_CAST(topi40_spMeshAttachment, attachment);
	if (self->parentMesh)
		return SUPER(SUPER(topi40_spMeshAttachment_newLinkedMesh(self)));
	copy = topi40_spMeshAttachment_create(attachment->name);
	copy->rendererObject = self->rendererObject;
	copy->regionU = self->regionU;
	copy->regionV = self->regionV;
	copy->regionU2 = self->regionU2;
	copy->regionV2 = self->regionV2;
	copy->regionDegrees = self->regionDegrees;
	copy->regionOffsetX = self->regionOffsetX;
	copy->regionOffsetY = self->regionOffsetY;
	copy->regionWidth = self->regionWidth;
	copy->regionHeight = self->regionHeight;
	copy->regionOriginalWidth = self->regionOriginalWidth;
	copy->regionOriginalHeight = self->regionOriginalHeight;
	MALLOC_STR(copy->path, self->path);
	topi40_spColor_setFromColor(&copy->color, &self->color);

	topi40_spVertexAttachment_copyTo(SUPER(self), SUPER(copy));
	copy->regionUVs = MALLOC(float, SUPER(self)->worldVerticesLength);
	memcpy(copy->regionUVs, self->regionUVs, SUPER(self)->worldVerticesLength * sizeof(float));
	copy->uvs = MALLOC(float, SUPER(self)->worldVerticesLength);
	memcpy(copy->uvs, self->uvs, SUPER(self)->worldVerticesLength * sizeof(float));
	copy->trianglesCount = self->trianglesCount;
	copy->triangles = MALLOC(unsigned short, self->trianglesCount);
	memcpy(copy->triangles, self->triangles, self->trianglesCount * sizeof(short));
	copy->hullLength = self->hullLength;
	if (self->edgesCount > 0) {
		copy->edgesCount = self->edgesCount;
		copy->edges = MALLOC(int, self->edgesCount);
		memcpy(copy->edges, self->edges, self->edgesCount * sizeof(int));
	}
	copy->width = self->width;
	copy->height = self->height;

	return SUPER(SUPER(copy));
}

topi40_spMeshAttachment *topi40_spMeshAttachment_newLinkedMesh(topi40_spMeshAttachment *self) {
	topi40_spMeshAttachment *copy = topi40_spMeshAttachment_create(self->super.super.name);

	copy->rendererObject = self->rendererObject;
	copy->regionU = self->regionU;
	copy->regionV = self->regionV;
	copy->regionU2 = self->regionU2;
	copy->regionV2 = self->regionV2;
	copy->regionDegrees = self->regionDegrees;
	copy->regionOffsetX = self->regionOffsetX;
	copy->regionOffsetY = self->regionOffsetY;
	copy->regionWidth = self->regionWidth;
	copy->regionHeight = self->regionHeight;
	copy->regionOriginalWidth = self->regionOriginalWidth;
	copy->regionOriginalHeight = self->regionOriginalHeight;
	MALLOC_STR(copy->path, self->path);
	topi40_spColor_setFromColor(&copy->color, &self->color);
	copy->super.deformAttachment = self->super.deformAttachment;
	topi40_spMeshAttachment_setParentMesh(copy, self->parentMesh ? self->parentMesh : self);
	topi40_spMeshAttachment_updateUVs(copy);
	return copy;
}

topi40_spMeshAttachment *topi40_spMeshAttachment_create(const char *name) {
	topi40_spMeshAttachment *self = NEW(topi40_spMeshAttachment);
	_topi40_spVertexAttachment_init(SUPER(self));
	topi40_spColor_setFromFloats(&self->color, 1, 1, 1, 1);
	_topi40_spAttachment_init(SUPER(SUPER(self)), name, SP_ATTACHMENT_MESH, _topi40_spMeshAttachment_dispose, _topi40_spMeshAttachment_copy);
	return self;
}

void topi40_spMeshAttachment_updateUVs(topi40_spMeshAttachment *self) {
	int i, n;
	float *uvs;
	float u, v, width, height;
	int verticesLength = SUPER(self)->worldVerticesLength;
	FREE(self->uvs);
	uvs = self->uvs = MALLOC(float, verticesLength);
	n = verticesLength;
	u = self->regionU;
	v = self->regionV;

	switch (self->regionDegrees) {
		case 90: {
			float textureWidth = self->regionHeight / (self->regionU2 - self->regionU);
			float textureHeight = self->regionWidth / (self->regionV2 - self->regionV);
			u -= (self->regionOriginalHeight - self->regionOffsetY - self->regionHeight) / textureWidth;
			v -= (self->regionOriginalWidth - self->regionOffsetX - self->regionWidth) / textureHeight;
			width = self->regionOriginalHeight / textureWidth;
			height = self->regionOriginalWidth / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + self->regionUVs[i + 1] * width;
				uvs[i + 1] = v + (1 - self->regionUVs[i]) * height;
			}
			return;
		}
		case 180: {
			float textureWidth = self->regionWidth / (self->regionU2 - self->regionU);
			float textureHeight = self->regionHeight / (self->regionV2 - self->regionV);
			u -= (self->regionOriginalWidth - self->regionOffsetX - self->regionWidth) / textureWidth;
			v -= self->regionOffsetY / textureHeight;
			width = self->regionOriginalWidth / textureWidth;
			height = self->regionOriginalHeight / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + (1 - self->regionUVs[i]) * width;
				uvs[i + 1] = v + (1 - self->regionUVs[i + 1]) * height;
			}
			return;
		}
		case 270: {
			float textureHeight = self->regionHeight / (self->regionV2 - self->regionV);
			float textureWidth = self->regionWidth / (self->regionU2 - self->regionU);
			u -= self->regionOffsetY / textureWidth;
			v -= self->regionOffsetX / textureHeight;
			width = self->regionOriginalHeight / textureWidth;
			height = self->regionOriginalWidth / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + (1 - self->regionUVs[i + 1]) * width;
				uvs[i + 1] = v + self->regionUVs[i] * height;
			}
			return;
		}
		default: {
			float textureWidth = self->regionWidth / (self->regionU2 - self->regionU);
			float textureHeight = self->regionHeight / (self->regionV2 - self->regionV);
			u -= self->regionOffsetX / textureWidth;
			v -= (self->regionOriginalHeight - self->regionOffsetY - self->regionHeight) / textureHeight;
			width = self->regionOriginalWidth / textureWidth;
			height = self->regionOriginalHeight / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + self->regionUVs[i] * width;
				uvs[i + 1] = v + self->regionUVs[i + 1] * height;
			}
		}
	}
}

void topi40_spMeshAttachment_setParentMesh(topi40_spMeshAttachment *self, topi40_spMeshAttachment *parentMesh) {
	CONST_CAST(topi40_spMeshAttachment *, self->parentMesh) = parentMesh;
	if (parentMesh) {
		self->super.bones = parentMesh->super.bones;
		self->super.bonesCount = parentMesh->super.bonesCount;

		self->super.vertices = parentMesh->super.vertices;
		self->super.verticesCount = parentMesh->super.verticesCount;

		self->regionUVs = parentMesh->regionUVs;

		self->triangles = parentMesh->triangles;
		self->trianglesCount = parentMesh->trianglesCount;

		self->hullLength = parentMesh->hullLength;

		self->super.worldVerticesLength = parentMesh->super.worldVerticesLength;

		self->edges = parentMesh->edges;
		self->edgesCount = parentMesh->edgesCount;

		self->width = parentMesh->width;
		self->height = parentMesh->height;
	}
}
