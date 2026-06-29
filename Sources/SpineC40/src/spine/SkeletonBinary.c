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

#include <topi40_spine/Animation.h>
#include <topi40_spine/Array.h>
#include <topi40_spine/AtlasAttachmentLoader.h>
#include <topi40_spine/SkeletonBinary.h>
#include <topi40_spine/extension.h>
#include <stdio.h>

typedef struct {
	const unsigned char *cursor;
	const unsigned char *end;
} _dataInput;

typedef struct {
	const char *parent;
	const char *skin;
	int slotIndex;
	topi40_spMeshAttachment *mesh;
	int inheritDeform;
} _topi40_spLinkedMesh;

typedef struct {
	topi40_spSkeletonBinary super;
	int ownsLoader;

	int linkedMeshCount;
	int linkedMeshCapacity;
	_topi40_spLinkedMesh *linkedMeshes;
} _topi40_spSkeletonBinary;

topi40_spSkeletonBinary *topi40_spSkeletonBinary_createWithLoader(topi40_spAttachmentLoader *attachmentLoader) {
	topi40_spSkeletonBinary *self = SUPER(NEW(_topi40_spSkeletonBinary));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

topi40_spSkeletonBinary *topi40_spSkeletonBinary_create(topi40_spAtlas *atlas) {
	topi40_spAtlasAttachmentLoader *attachmentLoader = topi40_spAtlasAttachmentLoader_create(atlas);
	topi40_spSkeletonBinary *self = topi40_spSkeletonBinary_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_topi40_spSkeletonBinary, self)->ownsLoader = 1;
	return self;
}

void topi40_spSkeletonBinary_dispose(topi40_spSkeletonBinary *self) {
	_topi40_spSkeletonBinary *internal = SUB_CAST(_topi40_spSkeletonBinary, self);
	if (internal->ownsLoader) topi40_spAttachmentLoader_dispose(self->attachmentLoader);
	FREE(internal->linkedMeshes);
	FREE(self->error);
	FREE(self);
}

void _topi40_spSkeletonBinary_setError(topi40_spSkeletonBinary *self, const char *value1, const char *value2) {
	char message[256];
	int length;
	FREE(self->error);
	strcpy(message, value1);
	length = (int) strlen(value1);
	if (value2) strncat(message + length, value2, 255 - length);
	MALLOC_STR(self->error, message);
}

static unsigned char readByte(_dataInput *input) {
	return *input->cursor++;
}

static signed char readSByte(_dataInput *input) {
	return (signed char) readByte(input);
}

static int readBoolean(_dataInput *input) {
	return readByte(input) != 0;
}

static int readInt(_dataInput *input) {
	uint32_t result = readByte(input);
	result <<= 8;
	result |= readByte(input);
	result <<= 8;
	result |= readByte(input);
	result <<= 8;
	result |= readByte(input);
	return (int) result;
}

static int readVarint(_dataInput *input, int /*bool*/ optimizePositive) {
	unsigned char b = readByte(input);
	uint32_t value = b & 0x7F;
	if (b & 0x80) {
		b = readByte(input);
		value |= (b & 0x7F) << 7;
		if (b & 0x80) {
			b = readByte(input);
			value |= (b & 0x7F) << 14;
			if (b & 0x80) {
				b = readByte(input);
				value |= (b & 0x7F) << 21;
				if (b & 0x80) value |= (uint32_t) (readByte(input) & 0x7F) << 28;
			}
		}
	}
	if (!optimizePositive)
		value = ((unsigned int) value >> 1) ^ (~(value & 1));
	return (int) value;
}

float topi40_readFloat(_dataInput *input) {
	union {
		int intValue;
		float floatValue;
	} intToFloat;
	intToFloat.intValue = readInt(input);
	return intToFloat.floatValue;
}

char *topi40_readString(_dataInput *input) {
	int length = readVarint(input, 1);
	char *string;
	if (length == 0) return NULL;
	string = MALLOC(char, length);
	memcpy(string, input->cursor, length - 1);
	input->cursor += length - 1;
	string[length - 1] = '\0';
	return string;
}

static char *readStringRef(_dataInput *input, topi40_spSkeletonData *skeletonData) {
	int index = readVarint(input, 1);
	return index == 0 ? 0 : skeletonData->strings[index - 1];
}

static void readColor(_dataInput *input, float *r, float *g, float *b, float *a) {
	*r = readByte(input) / 255.0f;
	*g = readByte(input) / 255.0f;
	*b = readByte(input) / 255.0f;
	*a = readByte(input) / 255.0f;
}

#define ATTACHMENT_REGION 0
#define ATTACHMENT_BOUNDING_BOX 1
#define ATTACHMENT_MESH 2
#define ATTACHMENT_LINKED_MESH 3
#define ATTACHMENT_PATH 4

#define BLEND_MODE_NORMAL 0
#define BLEND_MODE_ADDITIVE 1
#define BLEND_MODE_MULTIPLY 2
#define BLEND_MODE_SCREEN 3

#define BONE_ROTATE 0
#define BONE_TRANSLATE 1
#define BONE_TRANSLATEX 2
#define BONE_TRANSLATEY 3
#define BONE_SCALE 4
#define BONE_SCALEX 5
#define BONE_SCALEY 6
#define BONE_SHEAR 7
#define BONE_SHEARX 8
#define BONE_SHEARY 9

#define SLOT_ATTACHMENT 0
#define SLOT_RGBA 1
#define SLOT_RGB 2
#define SLOT_RGBA2 3
#define SLOT_RGB2 4
#define SLOT_ALPHA 5

#define PATH_POSITION 0
#define PATH_SPACING 1
#define PATH_MIX 2

#define CURVE_LINEAR 0
#define CURVE_STEPPED 1
#define CURVE_BEZIER 2

#define PATH_POSITION_FIXED 0
#define PATH_POSITION_PERCENT 1

#define PATH_SPACING_LENGTH 0
#define PATH_SPACING_FIXED 1
#define PATH_SPACING_PERCENT 2

#define PATH_ROTATE_TANGENT 0
#define PATH_ROTATE_CHAIN 1
#define PATH_ROTATE_CHAIN_SCALE 2

static void
setBezier(_dataInput *input, topi40_spTimeline *timeline, int bezier, int frame, int value, float time1, float time2,
		  float value1, float value2, float scale) {
	float cx1 = topi40_readFloat(input);
	float cy1 = topi40_readFloat(input);
	float cx2 = topi40_readFloat(input);
	float cy2 = topi40_readFloat(input);
	topi40_spTimeline_setBezier(timeline, bezier, frame, value, time1, value1, cx1, cy1 * scale, cx2, cy2 * scale, time2,
						 value2);
}

static topi40_spTimeline *readTimeline(_dataInput *input, topi40_spCurveTimeline1 *timeline, float scale) {
	int frame, bezier, frameLast;
	float time2, value2;
	float time = topi40_readFloat(input);
	float value = topi40_readFloat(input) * scale;
	for (frame = 0, bezier = 0, frameLast = timeline->super.frameCount - 1;; frame++) {
		topi40_spCurveTimeline1_setFrame(timeline, frame, time, value);
		if (frame == frameLast) break;
		time2 = topi40_readFloat(input);
		value2 = topi40_readFloat(input) * scale;
		switch (readSByte(input)) {
			case CURVE_STEPPED:
				topi40_spCurveTimeline_setStepped(timeline, frame);
				break;
			case CURVE_BEZIER:
				setBezier(input, SUPER(timeline), bezier++, frame, 0, time, time2, value, value2, scale);
		}
		time = time2;
		value = value2;
	}
	return SUPER(timeline);
}

static topi40_spTimeline *readTimeline2(_dataInput *input, topi40_spCurveTimeline2 *timeline, float scale) {
	int frame, bezier, frameLast;
	float time2, nvalue1, nvalue2;
	float time = topi40_readFloat(input);
	float value1 = topi40_readFloat(input) * scale;
	float value2 = topi40_readFloat(input) * scale;
	for (frame = 0, bezier = 0, frameLast = timeline->super.frameCount - 1;; frame++) {
		topi40_spCurveTimeline2_setFrame(timeline, frame, time, value1, value2);
		if (frame == frameLast) break;
		time2 = topi40_readFloat(input);
		nvalue1 = topi40_readFloat(input) * scale;
		nvalue2 = topi40_readFloat(input) * scale;
		switch (readSByte(input)) {
			case CURVE_STEPPED:
				topi40_spCurveTimeline_setStepped(timeline, frame);
				break;
			case CURVE_BEZIER:
				setBezier(input, SUPER(timeline), bezier++, frame, 0, time, time2, value1, nvalue1, scale);
				setBezier(input, SUPER(timeline), bezier++, frame, 1, time, time2, value2, nvalue2, scale);
		}
		time = time2;
		value1 = nvalue1;
		value2 = nvalue2;
	}
	return SUPER(timeline);
}

static void _topi40_spSkeletonBinary_addLinkedMesh(topi40_spSkeletonBinary *self, topi40_spMeshAttachment *mesh,
											const char *skin, int slotIndex, const char *parent, int inheritDeform) {
	_topi40_spLinkedMesh *linkedMesh;
	_topi40_spSkeletonBinary *internal = SUB_CAST(_topi40_spSkeletonBinary, self);

	if (internal->linkedMeshCount == internal->linkedMeshCapacity) {
		_topi40_spLinkedMesh *linkedMeshes;
		internal->linkedMeshCapacity *= 2;
		if (internal->linkedMeshCapacity < 8) internal->linkedMeshCapacity = 8;
		/* TODO Why not realloc? */
		linkedMeshes = MALLOC(_topi40_spLinkedMesh, internal->linkedMeshCapacity);
		memcpy(linkedMeshes, internal->linkedMeshes, sizeof(_topi40_spLinkedMesh) * internal->linkedMeshCount);
		FREE(internal->linkedMeshes);
		internal->linkedMeshes = linkedMeshes;
	}

	linkedMesh = internal->linkedMeshes + internal->linkedMeshCount++;
	linkedMesh->mesh = mesh;
	linkedMesh->skin = skin;
	linkedMesh->slotIndex = slotIndex;
	linkedMesh->parent = parent;
	linkedMesh->inheritDeform = inheritDeform;
}

static topi40_spAnimation *_topi40_spSkeletonBinary_readAnimation(topi40_spSkeletonBinary *self, const char *name,
													_dataInput *input, topi40_spSkeletonData *skeletonData) {
	topi40_spTimelineArray *timelines = topi40_spTimelineArray_create(18);
	float duration = 0;
	int i, n, ii, nn, iii, nnn;
	int frame, bezier;
	int drawOrderCount, eventCount;
	topi40_spAnimation *animation;
	float scale = self->scale;

	int numTimelines = readVarint(input, 1);
	UNUSED(numTimelines);

	/* Slot timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int slotIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			unsigned char timelineType = readByte(input);
			int frameCount = readVarint(input, 1);
			int frameLast = frameCount - 1;
			switch (timelineType) {
				case SLOT_ATTACHMENT: {
					topi40_spAttachmentTimeline *timeline = topi40_spAttachmentTimeline_create(frameCount, slotIndex);
					for (frame = 0; frame < frameCount; ++frame) {
						float time = topi40_readFloat(input);
						const char *attachmentName = readStringRef(input, skeletonData);
						topi40_spAttachmentTimeline_setFrame(timeline, frame, time, attachmentName);
					}
					topi40_spTimelineArray_add(timelines, SUPER(timeline));
					break;
				}
				case SLOT_RGBA: {
					int bezierCount = readVarint(input, 1);
					topi40_spRGBATimeline *timeline = topi40_spRGBATimeline_create(frameCount, bezierCount, slotIndex);

					float time = topi40_readFloat(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;
					float a = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, r2, g2, b2, a2;
						topi40_spRGBATimeline_setFrame(timeline, frame, time, r, g, b, a);
						if (frame == frameLast) break;

						time2 = topi40_readFloat(input);
						r2 = readByte(input) / 255.0;
						g2 = readByte(input) / 255.0;
						b2 = readByte(input) / 255.0;
						a2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, r2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, g2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, b2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, a, a2, 1);
						}
						time = time2;
						r = r2;
						g = g2;
						b = b2;
						a = a2;
					}
					topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_RGB: {
					int bezierCount = readVarint(input, 1);
					topi40_spRGBTimeline *timeline = topi40_spRGBTimeline_create(frameCount, bezierCount, slotIndex);

					float time = topi40_readFloat(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, r2, g2, b2;
						topi40_spRGBTimeline_setFrame(timeline, frame, time, r, g, b);
						if (frame == frameLast) break;

						time2 = topi40_readFloat(input);
						r2 = readByte(input) / 255.0;
						g2 = readByte(input) / 255.0;
						b2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, r2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, g2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, b2, 1);
						}
						time = time2;
						r = r2;
						g = g2;
						b = b2;
					}
					topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_RGBA2: {
					int bezierCount = readVarint(input, 1);
					topi40_spRGBA2Timeline *timeline = topi40_spRGBA2Timeline_create(frameCount, bezierCount, slotIndex);

					float time = topi40_readFloat(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;
					float a = readByte(input) / 255.0;
					float r2 = readByte(input) / 255.0;
					float g2 = readByte(input) / 255.0;
					float b2 = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, nr, ng, nb, na, nr2, ng2, nb2;
						topi40_spRGBA2Timeline_setFrame(timeline, frame, time, r, g, b, a, r2, g2, b2);
						if (frame == frameLast) break;
						time2 = topi40_readFloat(input);
						nr = readByte(input) / 255.0;
						ng = readByte(input) / 255.0;
						nb = readByte(input) / 255.0;
						na = readByte(input) / 255.0;
						nr2 = readByte(input) / 255.0;
						ng2 = readByte(input) / 255.0;
						nb2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, nr, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, ng, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, nb, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, a, na, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 4, time, time2, r2, nr2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 5, time, time2, g2, ng2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 6, time, time2, b2, nb2, 1);
						}
						time = time2;
						r = nr;
						g = ng;
						b = nb;
						a = na;
						r2 = nr2;
						g2 = ng2;
						b2 = nb2;
					}
					topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_RGB2: {
					int bezierCount = readVarint(input, 1);
					topi40_spRGB2Timeline *timeline = topi40_spRGB2Timeline_create(frameCount, bezierCount, slotIndex);

					float time = topi40_readFloat(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;
					float r2 = readByte(input) / 255.0;
					float g2 = readByte(input) / 255.0;
					float b2 = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, nr, ng, nb, nr2, ng2, nb2;
						topi40_spRGB2Timeline_setFrame(timeline, frame, time, r, g, b, r2, g2, b2);
						if (frame == frameLast) break;
						time2 = topi40_readFloat(input);
						nr = readByte(input) / 255.0;
						ng = readByte(input) / 255.0;
						nb = readByte(input) / 255.0;
						nr2 = readByte(input) / 255.0;
						ng2 = readByte(input) / 255.0;
						nb2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, nr, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, ng, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, nb, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, r2, nr2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 4, time, time2, g2, ng2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 5, time, time2, b2, nb2, 1);
						}
						time = time2;
						r = nr;
						g = ng;
						b = nb;
						r2 = nr2;
						g2 = ng2;
						b2 = nb2;
					}
					topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_ALPHA: {
					int bezierCount = readVarint(input, 1);
					topi40_spAlphaTimeline *timeline = topi40_spAlphaTimeline_create(frameCount, bezierCount, slotIndex);
					float time = topi40_readFloat(input);
					float a = readByte(input) / 255.0;
					for (frame = 0, bezier = 0;; frame++) {
						float time2, a2;
						topi40_spAlphaTimeline_setFrame(timeline, frame, time, a);
						if (frame == frameLast) break;
						time2 = topi40_readFloat(input);
						a2 = readByte(input) / 255;
						switch (readSByte(input)) {
							case CURVE_STEPPED:
								topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, a, a2, 1);
						}
						time = time2;
						a = a2;
					}
					topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				default: {
					return NULL;
				}
			}
		}
	}

	/* Bone timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int boneIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			unsigned char timelineType = readByte(input);
			int frameCount = readVarint(input, 1);
			int bezierCount = readVarint(input, 1);
			topi40_spTimeline *timeline = NULL;
			switch (timelineType) {
				case BONE_ROTATE:
					timeline = readTimeline(input, SUPER(topi40_spRotateTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_TRANSLATE:
					timeline = readTimeline2(input,
											 SUPER(topi40_spTranslateTimeline_create(frameCount, bezierCount, boneIndex)),
											 scale);
					break;
				case BONE_TRANSLATEX:
					timeline = readTimeline(input,
											SUPER(topi40_spTranslateXTimeline_create(frameCount, bezierCount, boneIndex)),
											scale);
					break;
				case BONE_TRANSLATEY:
					timeline = readTimeline(input,
											SUPER(topi40_spTranslateYTimeline_create(frameCount, bezierCount, boneIndex)),
											scale);
					break;
				case BONE_SCALE:
					timeline = readTimeline2(input, SUPER(topi40_spScaleTimeline_create(frameCount, bezierCount, boneIndex)),
											 1);
					break;
				case BONE_SCALEX:
					timeline = readTimeline(input, SUPER(topi40_spScaleXTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_SCALEY:
					timeline = readTimeline(input, SUPER(topi40_spScaleYTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_SHEAR:
					timeline = readTimeline2(input, SUPER(topi40_spShearTimeline_create(frameCount, bezierCount, boneIndex)),
											 1);
					break;
				case BONE_SHEARX:
					timeline = readTimeline(input, SUPER(topi40_spShearXTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_SHEARY:
					timeline = readTimeline(input, SUPER(topi40_spShearYTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				default: {
					for (iii = 0; iii < timelines->size; ++iii)
						topi40_spTimeline_dispose(timelines->items[iii]);
					topi40_spTimelineArray_dispose(timelines);
					_topi40_spSkeletonBinary_setError(self, "Invalid timeline type for a bone: ",
											   skeletonData->bones[boneIndex]->name);
					return NULL;
				}
			}
			topi40_spTimelineArray_add(timelines, timeline);
		}
	}

	/* IK constraint timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int index = readVarint(input, 1);
		int frameCount = readVarint(input, 1);
		int frameLast = frameCount - 1;
		int bezierCount = readVarint(input, 1);
		topi40_spIkConstraintTimeline *timeline = topi40_spIkConstraintTimeline_create(frameCount, bezierCount, index);
		float time = topi40_readFloat(input);
		float mix = topi40_readFloat(input);
		float softness = topi40_readFloat(input) * scale;
		for (frame = 0, bezier = 0;; frame++) {
			float time2, mix2, softness2;
			int bendDirection = readSByte(input);
			int compress = readBoolean(input);
			int stretch = readBoolean(input);
			topi40_spIkConstraintTimeline_setFrame(timeline, frame, time, mix, softness, bendDirection, compress, stretch);
			if (frame == frameLast) break;
			time2 = topi40_readFloat(input);
			mix2 = topi40_readFloat(input);
			softness2 = topi40_readFloat(input) * scale;
			switch (readSByte(input)) {
				case CURVE_STEPPED:
					topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
					break;
				case CURVE_BEZIER:
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, mix, mix2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, softness, softness2,
							  scale);
			}
			time = time2;
			mix = mix2;
			softness = softness2;
		}
		topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/* Transform constraint timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int index = readVarint(input, 1);
		int frameCount = readVarint(input, 1);
		int frameLast = frameCount - 1;
		int bezierCount = readVarint(input, 1);
		topi40_spTransformConstraintTimeline *timeline = topi40_spTransformConstraintTimeline_create(frameCount, bezierCount, index);
		float time = topi40_readFloat(input);
		float mixRotate = topi40_readFloat(input);
		float mixX = topi40_readFloat(input);
		float mixY = topi40_readFloat(input);
		float mixScaleX = topi40_readFloat(input);
		float mixScaleY = topi40_readFloat(input);
		float mixShearY = topi40_readFloat(input);
		for (frame = 0, bezier = 0;; frame++) {
			float time2, mixRotate2, mixX2, mixY2, mixScaleX2, mixScaleY2, mixShearY2;
			topi40_spTransformConstraintTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY, mixScaleX, mixScaleY,
												   mixShearY);
			if (frame == frameLast) break;
			time2 = topi40_readFloat(input);
			mixRotate2 = topi40_readFloat(input);
			mixX2 = topi40_readFloat(input);
			mixY2 = topi40_readFloat(input);
			mixScaleX2 = topi40_readFloat(input);
			mixScaleY2 = topi40_readFloat(input);
			mixShearY2 = topi40_readFloat(input);
			switch (readSByte(input)) {
				case CURVE_STEPPED:
					topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
					break;
				case CURVE_BEZIER:
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, mixRotate, mixRotate2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, mixX, mixX2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, mixY, mixY2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, mixScaleX, mixScaleX2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 4, time, time2, mixScaleY, mixScaleY2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 5, time, time2, mixShearY, mixShearY2, 1);
			}
			time = time2;
			mixRotate = mixRotate2;
			mixX = mixX2;
			mixY = mixY2;
			mixScaleX = mixScaleX2;
			mixScaleY = mixScaleY2;
			mixShearY = mixShearY2;
		}
		topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/* Path constraint timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int index = readVarint(input, 1);
		topi40_spPathConstraintData *data = skeletonData->pathConstraints[index];
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			int type = readSByte(input);
			int frameCount = readVarint(input, 1);
			int bezierCount = readVarint(input, 1);
			switch (type) {
				case PATH_POSITION: {
					topi40_spTimelineArray_add(timelines, readTimeline(input, SUPER(topi40_spPathConstraintPositionTimeline_create(frameCount, bezierCount, index)),
																data->positionMode == SP_POSITION_MODE_FIXED ? scale
																											 : 1));
					break;
				}
				case PATH_SPACING: {
					topi40_spTimelineArray_add(timelines, readTimeline(input,
																SUPER(topi40_spPathConstraintSpacingTimeline_create(frameCount,
																											 bezierCount,
																											 index)),
																data->topi40_spacingMode == SP_SPACING_MODE_LENGTH ||
																				data->topi40_spacingMode == SP_SPACING_MODE_FIXED
																		? scale
																		: 1));
					break;
				}
				case PATH_MIX: {
					float time, mixRotate, mixX, mixY;
					int frameLast;
					topi40_spPathConstraintMixTimeline *timeline = topi40_spPathConstraintMixTimeline_create(frameCount, bezierCount,
																							   index);
					time = topi40_readFloat(input);
					mixRotate = topi40_readFloat(input);
					mixX = topi40_readFloat(input);
					mixY = topi40_readFloat(input);
					for (frame = 0, bezier = 0, frameLast = timeline->super.super.frameCount - 1;; frame++) {
						float time2, mixRotate2, mixX2, mixY2;
						topi40_spPathConstraintMixTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY);
						if (frame == frameLast) break;
						time2 = topi40_readFloat(input);
						mixRotate2 = topi40_readFloat(input);
						mixX2 = topi40_readFloat(input);
						mixY2 = topi40_readFloat(input);
						switch (readSByte(input)) {
							case CURVE_STEPPED:
								topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, mixRotate,
										  mixRotate2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, mixX, mixX2,
										  1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, mixY, mixY2,
										  1);
						}
						time = time2;
						mixRotate = mixRotate2;
						mixX = mixX2;
						mixY = mixY2;
					}
					topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
				}
			}
		}
	}

	/* Deform timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		topi40_spSkin *skin = skeletonData->skins[readVarint(input, 1)];
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			int slotIndex = readVarint(input, 1);
			for (iii = 0, nnn = readVarint(input, 1); iii < nnn; ++iii) {
				float *tempDeform;
				topi40_spDeformTimeline *timeline;
				int weighted, deformLength;
				const char *attachmentName = readStringRef(input, skeletonData);
				int frameCount, frameLast, bezierCount;
				float time, time2;

				topi40_spVertexAttachment *attachment = SUB_CAST(topi40_spVertexAttachment,
														  topi40_spSkin_getAttachment(skin, slotIndex, attachmentName));
				if (!attachment) {
					for (i = 0; i < timelines->size; ++i)
						topi40_spTimeline_dispose(timelines->items[i]);
					topi40_spTimelineArray_dispose(timelines);
					_topi40_spSkeletonBinary_setError(self, "Attachment not found: ", attachmentName);
					return NULL;
				}

				weighted = attachment->bones != 0;
				deformLength = weighted ? attachment->verticesCount / 3 * 2 : attachment->verticesCount;
				tempDeform = MALLOC(float, deformLength);

				frameCount = readVarint(input, 1);
				frameLast = frameCount - 1;
				bezierCount = readVarint(input, 1);
				timeline = topi40_spDeformTimeline_create(frameCount, deformLength, bezierCount, slotIndex, attachment);

				time = topi40_readFloat(input);
				for (frame = 0, bezier = 0;; ++frame) {
					float *deform;
					int end = readVarint(input, 1);
					if (!end) {
						if (weighted) {
							deform = tempDeform;
							memset(deform, 0, sizeof(float) * deformLength);
						} else
							deform = attachment->vertices;
					} else {
						int v, start = readVarint(input, 1);
						deform = tempDeform;
						memset(deform, 0, sizeof(float) * start);
						end += start;
						if (self->scale == 1) {
							for (v = start; v < end; ++v)
								deform[v] = topi40_readFloat(input);
						} else {
							for (v = start; v < end; ++v)
								deform[v] = topi40_readFloat(input) * self->scale;
						}
						memset(deform + v, 0, sizeof(float) * (deformLength - v));
						if (!weighted) {
							float *vertices = attachment->vertices;
							for (v = 0; v < deformLength; ++v)
								deform[v] += vertices[v];
						}
					}
					topi40_spDeformTimeline_setFrame(timeline, frame, time, deform);
					if (frame == frameLast) break;
					time2 = topi40_readFloat(input);
					switch (readSByte(input)) {
						case CURVE_STEPPED:
							topi40_spCurveTimeline_setStepped(SUPER(timeline), frame);
							break;
						case CURVE_BEZIER:
							setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, 0, 1, 1);
					}
					time = time2;
				}
				FREE(tempDeform);

				topi40_spTimelineArray_add(timelines, (topi40_spTimeline *) timeline);
			}
		}
	}

	/* Draw order timeline. */
	drawOrderCount = readVarint(input, 1);
	if (drawOrderCount) {
		topi40_spDrawOrderTimeline *timeline = topi40_spDrawOrderTimeline_create(drawOrderCount, skeletonData->slotsCount);
		for (i = 0; i < drawOrderCount; ++i) {
			float time = topi40_readFloat(input);
			int offsetCount = readVarint(input, 1);
			int *drawOrder = MALLOC(int, skeletonData->slotsCount);
			int *unchanged = MALLOC(int, skeletonData->slotsCount - offsetCount);
			int originalIndex = 0, unchangedIndex = 0;
			memset(drawOrder, -1, sizeof(int) * skeletonData->slotsCount);
			for (ii = 0; ii < offsetCount; ++ii) {
				int slotIndex = readVarint(input, 1);
				/* Collect unchanged items. */
				while (originalIndex != slotIndex)
					unchanged[unchangedIndex++] = originalIndex++;
				/* Set changed items. */
				drawOrder[originalIndex + readVarint(input, 1)] = originalIndex;
				++originalIndex;
			}
			/* Collect remaining unchanged items. */
			while (originalIndex < skeletonData->slotsCount)
				unchanged[unchangedIndex++] = originalIndex++;
			/* Fill in unchanged items. */
			for (ii = skeletonData->slotsCount - 1; ii >= 0; ii--)
				if (drawOrder[ii] == -1) drawOrder[ii] = unchanged[--unchangedIndex];
			FREE(unchanged);
			/* TODO Avoid copying of drawOrder inside */
			topi40_spDrawOrderTimeline_setFrame(timeline, i, time, drawOrder);
			FREE(drawOrder);
		}
		topi40_spTimelineArray_add(timelines, (topi40_spTimeline *) timeline);
	}

	/* Event timeline. */
	eventCount = readVarint(input, 1);
	if (eventCount) {
		topi40_spEventTimeline *timeline = topi40_spEventTimeline_create(eventCount);
		for (i = 0; i < eventCount; ++i) {
			float time = topi40_readFloat(input);
			topi40_spEventData *eventData = skeletonData->events[readVarint(input, 1)];
			topi40_spEvent *event = topi40_spEvent_create(time, eventData);
			event->intValue = readVarint(input, 0);
			event->floatValue = topi40_readFloat(input);
			if (readBoolean(input))
				event->stringValue = topi40_readString(input);
			else
				MALLOC_STR(event->stringValue, eventData->stringValue);
			if (eventData->audioPath) {
				event->volume = topi40_readFloat(input);
				event->balance = topi40_readFloat(input);
			}
			topi40_spEventTimeline_setFrame(timeline, i, event);
		}
		topi40_spTimelineArray_add(timelines, (topi40_spTimeline *) timeline);
	}

	duration = 0;
	for (i = 0, n = timelines->size; i < n; i++) {
		duration = MAX(duration, topi40_spTimeline_getDuration(timelines->items[i]));
	}
	animation = topi40_spAnimation_create(name, timelines, duration);
	return animation;
}

static float *_readFloatArray(_dataInput *input, int n, float scale) {
	float *array = MALLOC(float, n);
	int i;
	if (scale == 1)
		for (i = 0; i < n; ++i)
			array[i] = topi40_readFloat(input);
	else
		for (i = 0; i < n; ++i)
			array[i] = topi40_readFloat(input) * scale;
	return array;
}

static short *_readShortArray(_dataInput *input, int *length) {
	int n = readVarint(input, 1);
	short *array = MALLOC(short, n);
	int i;
	*length = n;
	for (i = 0; i < n; ++i) {
		array[i] = readByte(input) << 8;
		array[i] |= readByte(input);
	}
	return array;
}

static void _readVertices(topi40_spSkeletonBinary *self, _dataInput *input, topi40_spVertexAttachment *attachment,
						  int vertexCount) {
	int i, ii;
	int verticesLength = vertexCount << 1;
	topi40_spFloatArray *weights = topi40_spFloatArray_create(8);
	topi40_spIntArray *bones = topi40_spIntArray_create(8);

	attachment->worldVerticesLength = verticesLength;

	if (!readBoolean(input)) {
		attachment->verticesCount = verticesLength;
		attachment->vertices = _readFloatArray(input, verticesLength, self->scale);
		attachment->bonesCount = 0;
		attachment->bones = 0;
		topi40_spFloatArray_dispose(weights);
		topi40_spIntArray_dispose(bones);
		return;
	}

	topi40_spFloatArray_ensureCapacity(weights, verticesLength * 3 * 3);
	topi40_spIntArray_ensureCapacity(bones, verticesLength * 3);

	for (i = 0; i < vertexCount; ++i) {
		int boneCount = readVarint(input, 1);
		topi40_spIntArray_add(bones, boneCount);
		for (ii = 0; ii < boneCount; ++ii) {
			topi40_spIntArray_add(bones, readVarint(input, 1));
			topi40_spFloatArray_add(weights, topi40_readFloat(input) * self->scale);
			topi40_spFloatArray_add(weights, topi40_readFloat(input) * self->scale);
			topi40_spFloatArray_add(weights, topi40_readFloat(input));
		}
	}

	attachment->verticesCount = weights->size;
	attachment->vertices = weights->items;
	FREE(weights);

	attachment->bonesCount = bones->size;
	attachment->bones = bones->items;
	FREE(bones);
}

topi40_spAttachment *topi40_spSkeletonBinary_readAttachment(topi40_spSkeletonBinary *self, _dataInput *input,
											  topi40_spSkin *skin, int slotIndex, const char *attachmentName,
											  topi40_spSkeletonData *skeletonData, int /*bool*/ nonessential) {
	int i;
	topi40_spAttachmentType type;
	const char *name = readStringRef(input, skeletonData);
	if (!name) name = attachmentName;

	type = (topi40_spAttachmentType) readByte(input);

	switch (type) {
		case SP_ATTACHMENT_REGION: {
			const char *path = readStringRef(input, skeletonData);
			topi40_spAttachment *attachment;
			topi40_spRegionAttachment *region;
			if (!path) MALLOC_STR(path, name);
			else {
				const char *tmp = 0;
				MALLOC_STR(tmp, path);
				path = tmp;
			}
			attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, path);
			region = SUB_CAST(topi40_spRegionAttachment, attachment);
			region->path = path;
			region->rotation = topi40_readFloat(input);
			region->x = topi40_readFloat(input) * self->scale;
			region->y = topi40_readFloat(input) * self->scale;
			region->scaleX = topi40_readFloat(input);
			region->scaleY = topi40_readFloat(input);
			region->width = topi40_readFloat(input) * self->scale;
			region->height = topi40_readFloat(input) * self->scale;
			readColor(input, &region->color.r, &region->color.g, &region->color.b, &region->color.a);
			topi40_spRegionAttachment_updateOffset(region);
			topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP_ATTACHMENT_BOUNDING_BOX: {
			int vertexCount = readVarint(input, 1);
			topi40_spAttachment *attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			_readVertices(self, input, SUB_CAST(topi40_spVertexAttachment, attachment), vertexCount);
			if (nonessential) {
				topi40_spBoundingBoxAttachment *bbox = SUB_CAST(topi40_spBoundingBoxAttachment, attachment);
				readColor(input, &bbox->color.r, &bbox->color.g, &bbox->color.b, &bbox->color.a);
			}
			topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP_ATTACHMENT_MESH: {
			int vertexCount;
			topi40_spAttachment *attachment;
			topi40_spMeshAttachment *mesh;
			const char *path = readStringRef(input, skeletonData);
			if (!path) MALLOC_STR(path, name);
			else {
				const char *tmp = 0;
				MALLOC_STR(tmp, path);
				path = tmp;
			}
			attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, path);
			mesh = SUB_CAST(topi40_spMeshAttachment, attachment);
			mesh->path = path;
			readColor(input, &mesh->color.r, &mesh->color.g, &mesh->color.b, &mesh->color.a);
			vertexCount = readVarint(input, 1);
			mesh->regionUVs = _readFloatArray(input, vertexCount << 1, 1);
			mesh->triangles = (unsigned short *) _readShortArray(input, &mesh->trianglesCount);
			_readVertices(self, input, SUPER(mesh), vertexCount);
			topi40_spMeshAttachment_updateUVs(mesh);
			mesh->hullLength = readVarint(input, 1) << 1;
			if (nonessential) {
				mesh->edges = (int *) _readShortArray(input, &mesh->edgesCount);
				mesh->width = topi40_readFloat(input) * self->scale;
				mesh->height = topi40_readFloat(input) * self->scale;
			} else {
				mesh->edges = 0;
				mesh->width = 0;
				mesh->height = 0;
			}
			topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP_ATTACHMENT_LINKED_MESH: {
			const char *skinName;
			const char *parent;
			topi40_spAttachment *attachment;
			topi40_spMeshAttachment *mesh;
			int inheritDeform;
			const char *path = readStringRef(input, skeletonData);
			if (!path) MALLOC_STR(path, name);
			else {
				const char *tmp = 0;
				MALLOC_STR(tmp, path);
				path = tmp;
			}
			attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, path);
			mesh = SUB_CAST(topi40_spMeshAttachment, attachment);
			mesh->path = path;
			readColor(input, &mesh->color.r, &mesh->color.g, &mesh->color.b, &mesh->color.a);
			skinName = readStringRef(input, skeletonData);
			parent = readStringRef(input, skeletonData);
			inheritDeform = readBoolean(input);
			if (nonessential) {
				mesh->width = topi40_readFloat(input) * self->scale;
				mesh->height = topi40_readFloat(input) * self->scale;
			}
			_topi40_spSkeletonBinary_addLinkedMesh(self, mesh, skinName, slotIndex, parent, inheritDeform);
			return attachment;
		}
		case SP_ATTACHMENT_PATH: {
			topi40_spAttachment *attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			topi40_spPathAttachment *path = SUB_CAST(topi40_spPathAttachment, attachment);
			int vertexCount = 0;
			path->closed = readBoolean(input);
			path->constantSpeed = readBoolean(input);
			vertexCount = readVarint(input, 1);
			_readVertices(self, input, SUPER(path), vertexCount);
			path->lengthsLength = vertexCount / 3;
			path->lengths = MALLOC(float, path->lengthsLength);
			for (i = 0; i < path->lengthsLength; ++i) {
				path->lengths[i] = topi40_readFloat(input) * self->scale;
			}
			if (nonessential) {
				readColor(input, &path->color.r, &path->color.g, &path->color.b, &path->color.a);
			}
			topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP_ATTACHMENT_POINT: {
			topi40_spAttachment *attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			topi40_spPointAttachment *point = SUB_CAST(topi40_spPointAttachment, attachment);
			point->rotation = topi40_readFloat(input);
			point->x = topi40_readFloat(input) * self->scale;
			point->y = topi40_readFloat(input) * self->scale;

			if (nonessential) {
				readColor(input, &point->color.r, &point->color.g, &point->color.b, &point->color.a);
			}
			topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP_ATTACHMENT_CLIPPING: {
			int endSlotIndex = readVarint(input, 1);
			int vertexCount = readVarint(input, 1);
			topi40_spAttachment *attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			topi40_spClippingAttachment *clip = SUB_CAST(topi40_spClippingAttachment, attachment);
			_readVertices(self, input, SUB_CAST(topi40_spVertexAttachment, attachment), vertexCount);
			if (nonessential) {
				readColor(input, &clip->color.r, &clip->color.g, &clip->color.b, &clip->color.a);
			}
			clip->endSlot = skeletonData->slots[endSlotIndex];
			topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
	}

	return NULL;
}

topi40_spSkin *topi40_spSkeletonBinary_readSkin(topi40_spSkeletonBinary *self, _dataInput *input, int /*bool*/ defaultSkin,
								  topi40_spSkeletonData *skeletonData, int /*bool*/ nonessential) {
	topi40_spSkin *skin;
	int i, n, ii, nn, slotCount;

	if (defaultSkin) {
		slotCount = readVarint(input, 1);
		if (slotCount == 0) return NULL;
		skin = topi40_spSkin_create("default");
	} else {
		skin = topi40_spSkin_create(readStringRef(input, skeletonData));
		for (i = 0, n = readVarint(input, 1); i < n; i++)
			topi40_spBoneDataArray_add(skin->bones, skeletonData->bones[readVarint(input, 1)]);

		for (i = 0, n = readVarint(input, 1); i < n; i++)
			topi40_spIkConstraintDataArray_add(skin->ikConstraints, skeletonData->ikConstraints[readVarint(input, 1)]);

		for (i = 0, n = readVarint(input, 1); i < n; i++)
			topi40_spTransformConstraintDataArray_add(skin->transformConstraints,
											   skeletonData->transformConstraints[readVarint(input, 1)]);

		for (i = 0, n = readVarint(input, 1); i < n; i++)
			topi40_spPathConstraintDataArray_add(skin->pathConstraints, skeletonData->pathConstraints[readVarint(input, 1)]);

		slotCount = readVarint(input, 1);
	}

	for (i = 0; i < slotCount; ++i) {
		int slotIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			const char *name = readStringRef(input, skeletonData);
			topi40_spAttachment *attachment = topi40_spSkeletonBinary_readAttachment(self, input, skin, slotIndex, name, skeletonData,
																	   nonessential);
			if (attachment) topi40_spSkin_setAttachment(skin, slotIndex, name, attachment);
		}
	}
	return skin;
}

topi40_spSkeletonData *topi40_spSkeletonBinary_readSkeletonDataFile(topi40_spSkeletonBinary *self, const char *path) {
	int length;
	topi40_spSkeletonData *skeletonData;
	const char *binary = _topi40_spUtil_readFile(path, &length);
	if (length == 0 || !binary) {
		_topi40_spSkeletonBinary_setError(self, "Unable to read skeleton file: ", path);
		return NULL;
	}
	skeletonData = topi40_spSkeletonBinary_readSkeletonData(self, (unsigned char *) binary, length);
	FREE(binary);
	return skeletonData;
}

topi40_spSkeletonData *topi40_spSkeletonBinary_readSkeletonData(topi40_spSkeletonBinary *self, const unsigned char *binary,
												  const int length) {
	int i, n, ii, nonessential;
	char buffer[32];
	int lowHash, highHash;
	topi40_spSkeletonData *skeletonData;
	_topi40_spSkeletonBinary *internal = SUB_CAST(_topi40_spSkeletonBinary, self);

	_dataInput *input = NEW(_dataInput);
	input->cursor = binary;
	input->end = binary + length;

	FREE(self->error);
	CONST_CAST(char *, self->error) = 0;
	internal->linkedMeshCount = 0;

	skeletonData = topi40_spSkeletonData_create();
	lowHash = readInt(input);
	highHash = readInt(input);
	sprintf(buffer, "%x%x", highHash, lowHash);
	buffer[31] = 0;
	MALLOC_STR(skeletonData->hash, buffer);

	skeletonData->version = topi40_readString(input);
	if (!strlen(skeletonData->version)) {
		FREE(skeletonData->version);
		skeletonData->version = 0;
	}

	skeletonData->x = topi40_readFloat(input);
	skeletonData->y = topi40_readFloat(input);
	skeletonData->width = topi40_readFloat(input);
	skeletonData->height = topi40_readFloat(input);

	nonessential = readBoolean(input);

	if (nonessential) {
		skeletonData->fps = topi40_readFloat(input);
		skeletonData->imagesPath = topi40_readString(input);
		if (!strlen(skeletonData->imagesPath)) {
			FREE(skeletonData->imagesPath);
			skeletonData->imagesPath = 0;
		}
		skeletonData->audioPath = topi40_readString(input);
		if (!strlen(skeletonData->audioPath)) {
			FREE(skeletonData->audioPath);
			skeletonData->audioPath = 0;
		}
	}

	skeletonData->stringsCount = n = readVarint(input, 1);
	skeletonData->strings = MALLOC(char *, skeletonData->stringsCount);
	for (i = 0; i < n; i++) {
		skeletonData->strings[i] = topi40_readString(input);
	}

	/* Bones. */
	skeletonData->bonesCount = readVarint(input, 1);
	skeletonData->bones = MALLOC(topi40_spBoneData *, skeletonData->bonesCount);
	for (i = 0; i < skeletonData->bonesCount; ++i) {
		topi40_spBoneData *data;
		int mode;
		const char *name = topi40_readString(input);
		topi40_spBoneData *parent = i == 0 ? 0 : skeletonData->bones[readVarint(input, 1)];
		/* TODO Avoid copying of name */
		data = topi40_spBoneData_create(i, name, parent);
		FREE(name);
		data->rotation = topi40_readFloat(input);
		data->x = topi40_readFloat(input) * self->scale;
		data->y = topi40_readFloat(input) * self->scale;
		data->scaleX = topi40_readFloat(input);
		data->scaleY = topi40_readFloat(input);
		data->shearX = topi40_readFloat(input);
		data->shearY = topi40_readFloat(input);
		data->length = topi40_readFloat(input) * self->scale;
		mode = readVarint(input, 1);
		switch (mode) {
			case 0:
				data->transformMode = SP_TRANSFORMMODE_NORMAL;
				break;
			case 1:
				data->transformMode = SP_TRANSFORMMODE_ONLYTRANSLATION;
				break;
			case 2:
				data->transformMode = SP_TRANSFORMMODE_NOROTATIONORREFLECTION;
				break;
			case 3:
				data->transformMode = SP_TRANSFORMMODE_NOSCALE;
				break;
			case 4:
				data->transformMode = SP_TRANSFORMMODE_NOSCALEORREFLECTION;
				break;
		}
		data->skinRequired = readBoolean(input);
		if (nonessential) {
			readColor(input, &data->color.r, &data->color.g, &data->color.b, &data->color.a);
		}
		skeletonData->bones[i] = data;
	}

	/* Slots. */
	skeletonData->slotsCount = readVarint(input, 1);
	skeletonData->slots = MALLOC(topi40_spSlotData *, skeletonData->slotsCount);
	for (i = 0; i < skeletonData->slotsCount; ++i) {
		int r, g, b, a;
		const char *attachmentName;
		const char *slotName = topi40_readString(input);
		topi40_spBoneData *boneData = skeletonData->bones[readVarint(input, 1)];
		/* TODO Avoid copying of slotName */
		topi40_spSlotData *slotData = topi40_spSlotData_create(i, slotName, boneData);
		FREE(slotName);
		readColor(input, &slotData->color.r, &slotData->color.g, &slotData->color.b, &slotData->color.a);
		a = readByte(input);
		r = readByte(input);
		g = readByte(input);
		b = readByte(input);
		if (!(r == 0xff && g == 0xff && b == 0xff && a == 0xff)) {
			slotData->darkColor = topi40_spColor_create();
			topi40_spColor_setFromFloats(slotData->darkColor, r / 255.0f, g / 255.0f, b / 255.0f, 1);
		}
		attachmentName = readStringRef(input, skeletonData);
		if (attachmentName) MALLOC_STR(slotData->attachmentName, attachmentName);
		else
			slotData->attachmentName = 0;
		slotData->blendMode = (topi40_spBlendMode) readVarint(input, 1);
		skeletonData->slots[i] = slotData;
	}

	/* IK constraints. */
	skeletonData->ikConstraintsCount = readVarint(input, 1);
	skeletonData->ikConstraints = MALLOC(topi40_spIkConstraintData *, skeletonData->ikConstraintsCount);
	for (i = 0; i < skeletonData->ikConstraintsCount; ++i) {
		const char *name = topi40_readString(input);
		/* TODO Avoid copying of name */
		topi40_spIkConstraintData *data = topi40_spIkConstraintData_create(name);
		data->order = readVarint(input, 1);
		data->skinRequired = readBoolean(input);
		FREE(name);
		data->bonesCount = readVarint(input, 1);
		data->bones = MALLOC(topi40_spBoneData *, data->bonesCount);
		for (ii = 0; ii < data->bonesCount; ++ii)
			data->bones[ii] = skeletonData->bones[readVarint(input, 1)];
		data->target = skeletonData->bones[readVarint(input, 1)];
		data->mix = topi40_readFloat(input);
		data->softness = topi40_readFloat(input);
		data->bendDirection = readSByte(input);
		data->compress = readBoolean(input);
		data->stretch = readBoolean(input);
		data->uniform = readBoolean(input);
		skeletonData->ikConstraints[i] = data;
	}

	/* Transform constraints. */
	skeletonData->transformConstraintsCount = readVarint(input, 1);
	skeletonData->transformConstraints = MALLOC(
			topi40_spTransformConstraintData *, skeletonData->transformConstraintsCount);
	for (i = 0; i < skeletonData->transformConstraintsCount; ++i) {
		const char *name = topi40_readString(input);
		/* TODO Avoid copying of name */
		topi40_spTransformConstraintData *data = topi40_spTransformConstraintData_create(name);
		data->order = readVarint(input, 1);
		data->skinRequired = readBoolean(input);
		FREE(name);
		data->bonesCount = readVarint(input, 1);
		CONST_CAST(topi40_spBoneData **, data->bones) = MALLOC(topi40_spBoneData *, data->bonesCount);
		for (ii = 0; ii < data->bonesCount; ++ii)
			data->bones[ii] = skeletonData->bones[readVarint(input, 1)];
		data->target = skeletonData->bones[readVarint(input, 1)];
		data->local = readBoolean(input);
		data->relative = readBoolean(input);
		data->offsetRotation = topi40_readFloat(input);
		data->offsetX = topi40_readFloat(input) * self->scale;
		data->offsetY = topi40_readFloat(input) * self->scale;
		data->offsetScaleX = topi40_readFloat(input);
		data->offsetScaleY = topi40_readFloat(input);
		data->offsetShearY = topi40_readFloat(input);
		data->mixRotate = topi40_readFloat(input);
		data->mixX = topi40_readFloat(input);
		data->mixY = topi40_readFloat(input);
		data->mixScaleX = topi40_readFloat(input);
		data->mixScaleY = topi40_readFloat(input);
		data->mixShearY = topi40_readFloat(input);
		skeletonData->transformConstraints[i] = data;
	}

	/* Path constraints */
	skeletonData->pathConstraintsCount = readVarint(input, 1);
	skeletonData->pathConstraints = MALLOC(topi40_spPathConstraintData *, skeletonData->pathConstraintsCount);
	for (i = 0; i < skeletonData->pathConstraintsCount; ++i) {
		const char *name = topi40_readString(input);
		/* TODO Avoid copying of name */
		topi40_spPathConstraintData *data = topi40_spPathConstraintData_create(name);
		data->order = readVarint(input, 1);
		data->skinRequired = readBoolean(input);
		FREE(name);
		data->bonesCount = readVarint(input, 1);
		CONST_CAST(topi40_spBoneData **, data->bones) = MALLOC(topi40_spBoneData *, data->bonesCount);
		for (ii = 0; ii < data->bonesCount; ++ii)
			data->bones[ii] = skeletonData->bones[readVarint(input, 1)];
		data->target = skeletonData->slots[readVarint(input, 1)];
		data->positionMode = (topi40_spPositionMode) readVarint(input, 1);
		data->topi40_spacingMode = (topi40_spSpacingMode) readVarint(input, 1);
		data->rotateMode = (topi40_spRotateMode) readVarint(input, 1);
		data->offsetRotation = topi40_readFloat(input);
		data->position = topi40_readFloat(input);
		if (data->positionMode == SP_POSITION_MODE_FIXED) data->position *= self->scale;
		data->topi40_spacing = topi40_readFloat(input);
		if (data->topi40_spacingMode == SP_SPACING_MODE_LENGTH || data->topi40_spacingMode == SP_SPACING_MODE_FIXED)
			data->topi40_spacing *= self->scale;
		data->mixRotate = topi40_readFloat(input);
		data->mixX = topi40_readFloat(input);
		data->mixY = topi40_readFloat(input);
		skeletonData->pathConstraints[i] = data;
	}

	/* Default skin. */
	skeletonData->defaultSkin = topi40_spSkeletonBinary_readSkin(self, input, -1, skeletonData, nonessential);
	skeletonData->skinsCount = readVarint(input, 1);

	if (skeletonData->defaultSkin)
		++skeletonData->skinsCount;

	skeletonData->skins = MALLOC(topi40_spSkin *, skeletonData->skinsCount);

	if (skeletonData->defaultSkin)
		skeletonData->skins[0] = skeletonData->defaultSkin;

	/* Skins. */
	for (i = skeletonData->defaultSkin ? 1 : 0; i < skeletonData->skinsCount; ++i) {
		skeletonData->skins[i] = topi40_spSkeletonBinary_readSkin(self, input, 0, skeletonData, nonessential);
	}

	/* Linked meshes. */
	for (i = 0; i < internal->linkedMeshCount; ++i) {
		_topi40_spLinkedMesh *linkedMesh = internal->linkedMeshes + i;
		topi40_spSkin *skin = !linkedMesh->skin ? skeletonData->defaultSkin : topi40_spSkeletonData_findSkin(skeletonData, linkedMesh->skin);
		topi40_spAttachment *parent;
		if (!skin) {
			FREE(input);
			topi40_spSkeletonData_dispose(skeletonData);
			_topi40_spSkeletonBinary_setError(self, "Skin not found: ", linkedMesh->skin);
			return NULL;
		}
		parent = topi40_spSkin_getAttachment(skin, linkedMesh->slotIndex, linkedMesh->parent);
		if (!parent) {
			FREE(input);
			topi40_spSkeletonData_dispose(skeletonData);
			_topi40_spSkeletonBinary_setError(self, "Parent mesh not found: ", linkedMesh->parent);
			return NULL;
		}
		linkedMesh->mesh->super.deformAttachment = linkedMesh->inheritDeform ? SUB_CAST(topi40_spVertexAttachment, parent)
																			 : SUB_CAST(topi40_spVertexAttachment,
																						linkedMesh->mesh);
		topi40_spMeshAttachment_setParentMesh(linkedMesh->mesh, SUB_CAST(topi40_spMeshAttachment, parent));
		topi40_spMeshAttachment_updateUVs(linkedMesh->mesh);
		topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, SUPER(SUPER(linkedMesh->mesh)));
	}

	/* Events. */
	skeletonData->eventsCount = readVarint(input, 1);
	skeletonData->events = MALLOC(topi40_spEventData *, skeletonData->eventsCount);
	for (i = 0; i < skeletonData->eventsCount; ++i) {
		const char *name = readStringRef(input, skeletonData);
		topi40_spEventData *eventData = topi40_spEventData_create(name);
		eventData->intValue = readVarint(input, 0);
		eventData->floatValue = topi40_readFloat(input);
		eventData->stringValue = topi40_readString(input);
		eventData->audioPath = topi40_readString(input);
		if (eventData->audioPath) {
			eventData->volume = topi40_readFloat(input);
			eventData->balance = topi40_readFloat(input);
		}
		skeletonData->events[i] = eventData;
	}

	/* Animations. */
	skeletonData->animationsCount = readVarint(input, 1);
	skeletonData->animations = MALLOC(topi40_spAnimation *, skeletonData->animationsCount);
	for (i = 0; i < skeletonData->animationsCount; ++i) {
		const char *name = topi40_readString(input);
		topi40_spAnimation *animation = _topi40_spSkeletonBinary_readAnimation(self, name, input, skeletonData);
		FREE(name);
		if (!animation) {
			FREE(input);
			topi40_spSkeletonData_dispose(skeletonData);
			return NULL;
		}
		skeletonData->animations[i] = animation;
	}

	FREE(input);
	return skeletonData;
}
