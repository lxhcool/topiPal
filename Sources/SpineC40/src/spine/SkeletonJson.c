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

#include "Json.h"
#include <topi40_spine/Array.h>
#include <topi40_spine/AtlasAttachmentLoader.h>
#include <topi40_spine/SkeletonJson.h>
#include <topi40_spine/extension.h>
#include <stdio.h>

typedef struct {
	const char *parent;
	const char *skin;
	int slotIndex;
	topi40_spMeshAttachment *mesh;
	int inheritDeform;
} _topi40_spLinkedMesh;

typedef struct {
	topi40_spSkeletonJson super;
	int ownsLoader;

	int linkedMeshCount;
	int linkedMeshCapacity;
	_topi40_spLinkedMesh *linkedMeshes;
} _topi40_spSkeletonJson;

topi40_spSkeletonJson *topi40_spSkeletonJson_createWithLoader(topi40_spAttachmentLoader *attachmentLoader) {
	topi40_spSkeletonJson *self = SUPER(NEW(_topi40_spSkeletonJson));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

topi40_spSkeletonJson *topi40_spSkeletonJson_create(topi40_spAtlas *atlas) {
	topi40_spAtlasAttachmentLoader *attachmentLoader = topi40_spAtlasAttachmentLoader_create(atlas);
	topi40_spSkeletonJson *self = topi40_spSkeletonJson_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_topi40_spSkeletonJson, self)->ownsLoader = 1;
	return self;
}

void topi40_spSkeletonJson_dispose(topi40_spSkeletonJson *self) {
	_topi40_spSkeletonJson *internal = SUB_CAST(_topi40_spSkeletonJson, self);
	if (internal->ownsLoader) topi40_spAttachmentLoader_dispose(self->attachmentLoader);
	FREE(internal->linkedMeshes);
	FREE(self->error);
	FREE(self);
}

void _topi40_spSkeletonJson_setError(topi40_spSkeletonJson *self, Json *root, const char *value1, const char *value2) {
	char message[256];
	int length;
	FREE(self->error);
	strcpy(message, value1);
	length = (int) strlen(value1);
	if (value2) strncat(message + length, value2, 255 - length);
	MALLOC_STR(self->error, message);
	if (root) topi40_Json_dispose(root);
}

static float toColor(const char *value, int index) {
	char digits[3];
	char *error;
	int color;

	if ((size_t) index >= strlen(value) / 2) return -1;
	value += index * 2;

	digits[0] = *value;
	digits[1] = *(value + 1);
	digits[2] = '\0';
	color = (int) strtoul(digits, &error, 16);
	if (*error != 0) return -1;
	return color / (float) 255;
}

static void toColor2(topi40_spColor *color, const char *value, int /*bool*/ hasAlpha) {
	color->r = toColor(value, 0);
	color->g = toColor(value, 1);
	color->b = toColor(value, 2);
	if (hasAlpha) color->a = toColor(value, 3);
}

static void
setBezier(topi40_spCurveTimeline *timeline, int frame, int value, int bezier, float time1, float value1, float cx1, float cy1,
		  float cx2, float cy2, float time2, float value2) {
	topi40_spTimeline_setBezier(SUPER(timeline), bezier, frame, value, time1, value1, cx1, cy1, cx2, cy2, time2, value2);
}

static int readCurve(Json *curve, topi40_spCurveTimeline *timeline, int bezier, int frame, int value, float time1, float time2,
					 float value1, float value2, float scale) {
	float cx1, cy1, cx2, cy2;
	if (curve->type == topi40_Json_String && strcmp(curve->valueString, "stepped") == 0) {
		if (value != 0) topi40_spCurveTimeline_setStepped(timeline, frame);
		return bezier;
	}
	curve = topi40_Json_getItemAtIndex(curve, value << 2);
	cx1 = curve->valueFloat;
	curve = curve->next;
	cy1 = curve->valueFloat * scale;
	curve = curve->next;
	cx2 = curve->valueFloat;
	curve = curve->next;
	cy2 = curve->valueFloat * scale;
	setBezier(timeline, frame, value, bezier, time1, value1, cx1, cy1, cx2, cy2, time2, value2);
	return bezier + 1;
}

static topi40_spTimeline *readTimeline(Json *keyMap, topi40_spCurveTimeline1 *timeline, float defaultValue, float scale) {
	float time = topi40_Json_getFloat(keyMap, "time", 0);
	float value = topi40_Json_getFloat(keyMap, "value", defaultValue) * scale;
	int frame, bezier = 0;
	for (frame = 0;; ++frame) {
		Json *nextMap, *curve;
		float time2, value2;
		topi40_spCurveTimeline1_setFrame(timeline, frame, time, value);
		nextMap = keyMap->next;
		if (nextMap == NULL) break;
		time2 = topi40_Json_getFloat(nextMap, "time", 0);
		value2 = topi40_Json_getFloat(nextMap, "value", defaultValue) * scale;
		curve = topi40_Json_getItem(keyMap, "curve");
		if (curve != NULL) bezier = readCurve(curve, timeline, bezier, frame, 0, time, time2, value, value2, scale);
		time = time2;
		value = value2;
		keyMap = nextMap;
	}
	/* timeline.shrink(); // BOZO */
	return SUPER(timeline);
}

static topi40_spTimeline *
readTimeline2(Json *keyMap, topi40_spCurveTimeline2 *timeline, const char *name1, const char *name2, float defaultValue,
			  float scale) {
	float time = topi40_Json_getFloat(keyMap, "time", 0);
	float value1 = topi40_Json_getFloat(keyMap, name1, defaultValue) * scale;
	float value2 = topi40_Json_getFloat(keyMap, name2, defaultValue) * scale;
	int frame, bezier = 0;
	for (frame = 0;; ++frame) {
		Json *nextMap, *curve;
		float time2, nvalue1, nvalue2;
		topi40_spCurveTimeline2_setFrame(timeline, frame, time, value1, value2);
		nextMap = keyMap->next;
		if (nextMap == NULL) break;
		time2 = topi40_Json_getFloat(nextMap, "time", 0);
		nvalue1 = topi40_Json_getFloat(nextMap, name1, defaultValue) * scale;
		nvalue2 = topi40_Json_getFloat(nextMap, name2, defaultValue) * scale;
		curve = topi40_Json_getItem(keyMap, "curve");
		if (curve != NULL) {
			bezier = readCurve(curve, timeline, bezier, frame, 0, time, time2, value1, nvalue1, scale);
			bezier = readCurve(curve, timeline, bezier, frame, 1, time, time2, value2, nvalue2, scale);
		}
		time = time2;
		value1 = nvalue1;
		value2 = nvalue2;
		keyMap = nextMap;
	}
	/* timeline.shrink(); // BOZO */
	return SUPER(timeline);
}

static void _topi40_spSkeletonJson_addLinkedMesh(topi40_spSkeletonJson *self, topi40_spMeshAttachment *mesh, const char *skin, int slotIndex,
										  const char *parent, int inheritDeform) {
	_topi40_spLinkedMesh *linkedMesh;
	_topi40_spSkeletonJson *internal = SUB_CAST(_topi40_spSkeletonJson, self);

	if (internal->linkedMeshCount == internal->linkedMeshCapacity) {
		_topi40_spLinkedMesh *linkedMeshes;
		internal->linkedMeshCapacity *= 2;
		if (internal->linkedMeshCapacity < 8) internal->linkedMeshCapacity = 8;
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

static void cleanUpTimelines(topi40_spTimelineArray *timelines) {
	int i, n;
	for (i = 0, n = timelines->size; i < n; ++i)
		topi40_spTimeline_dispose(timelines->items[i]);
	topi40_spTimelineArray_dispose(timelines);
}

static int findSlotIndex(topi40_spSkeletonJson *json, const topi40_spSkeletonData *skeletonData, const char *slotName, topi40_spTimelineArray *timelines) {
	topi40_spSlotData *slot = topi40_spSkeletonData_findSlot(skeletonData, slotName);
	if (slot) return slot->index;
	cleanUpTimelines(timelines);
	_topi40_spSkeletonJson_setError(json, NULL, "Slot not found: ", slotName);
	return -1;
}

int topi40_findIkConstraintIndex(topi40_spSkeletonJson *json, const topi40_spSkeletonData *skeletonData, const topi40_spIkConstraintData *constraint, topi40_spTimelineArray *timelines) {
	if (constraint) {
		int i;
		for (i = 0; i < skeletonData->ikConstraintsCount; ++i)
			if (skeletonData->ikConstraints[i] == constraint) return i;
	}
	cleanUpTimelines(timelines);
	_topi40_spSkeletonJson_setError(json, NULL, "IK constraint not found: ", constraint->name);
	return -1;
}

int topi40_findTransformConstraintIndex(topi40_spSkeletonJson *json, const topi40_spSkeletonData *skeletonData, const topi40_spTransformConstraintData *constraint, topi40_spTimelineArray *timelines) {
	if (constraint) {
		int i;
		for (i = 0; i < skeletonData->transformConstraintsCount; ++i)
			if (skeletonData->transformConstraints[i] == constraint) return i;
	}
	cleanUpTimelines(timelines);
	_topi40_spSkeletonJson_setError(json, NULL, "Transform constraint not found: ", constraint->name);
	return -1;
}

int topi40_findPathConstraintIndex(topi40_spSkeletonJson *json, const topi40_spSkeletonData *skeletonData, const topi40_spPathConstraintData *constraint, topi40_spTimelineArray *timelines) {
	if (constraint) {
		int i;
		for (i = 0; i < skeletonData->pathConstraintsCount; ++i)
			if (skeletonData->pathConstraints[i] == constraint) return i;
	}
	cleanUpTimelines(timelines);
	_topi40_spSkeletonJson_setError(json, NULL, "Path constraint not found: ", constraint->name);
	return -1;
}

static topi40_spAnimation *_topi40_spSkeletonJson_readAnimation(topi40_spSkeletonJson *self, Json *root, topi40_spSkeletonData *skeletonData) {
	topi40_spTimelineArray *timelines = topi40_spTimelineArray_create(8);

	float scale = self->scale, duration;
	Json *bones = topi40_Json_getItem(root, "bones");
	Json *slots = topi40_Json_getItem(root, "slots");
	Json *ik = topi40_Json_getItem(root, "ik");
	Json *transform = topi40_Json_getItem(root, "transform");
	Json *paths = topi40_Json_getItem(root, "path");
	Json *deformJson = topi40_Json_getItem(root, "deform");
	Json *drawOrderJson = topi40_Json_getItem(root, "drawOrder");
	Json *events = topi40_Json_getItem(root, "events");
	Json *boneMap, *slotMap, *constraintMap, *keyMap, *nextMap, *curve, *timelineMap;
	int frame, bezier, i, n;
	topi40_spColor color, color2, newColor, newColor2;

	/* Slot timelines. */
	for (slotMap = slots ? slots->child : 0; slotMap; slotMap = slotMap->next) {
		int slotIndex = findSlotIndex(self, skeletonData, slotMap->name, timelines);
		if (slotIndex == -1) return NULL;

		for (timelineMap = slotMap->child; timelineMap; timelineMap = timelineMap->next) {
			int frames = timelineMap->size;
			if (strcmp(timelineMap->name, "attachment") == 0) {
				topi40_spAttachmentTimeline *timeline = topi40_spAttachmentTimeline_create(frames, slotIndex);
				for (keyMap = timelineMap->child, frame = 0; keyMap; keyMap = keyMap->next, ++frame) {
					topi40_spAttachmentTimeline_setFrame(timeline, frame, topi40_Json_getFloat(keyMap, "time", 0),
												  topi40_Json_getItem(keyMap, "name") ? topi40_Json_getItem(keyMap, "name")->valueString : NULL);
				}
				topi40_spTimelineArray_add(timelines, SUPER(timeline));

			} else if (strcmp(timelineMap->name, "rgba") == 0) {
				float time;
				topi40_spRGBATimeline *timeline = topi40_spRGBATimeline_create(frames, frames << 2, slotIndex);
				keyMap = timelineMap->child;
				time = topi40_Json_getFloat(keyMap, "time", 0);
				toColor2(&color, topi40_Json_getString(keyMap, "color", 0), 1);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					topi40_spRGBATimeline_setFrame(timeline, frame, time, color.r, color.g, color.b, color.a);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = topi40_Json_getFloat(nextMap, "time", 0);
					toColor2(&newColor, topi40_Json_getString(nextMap, "color", 0), 1);
					curve = topi40_Json_getItem(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, color.a, newColor.a,
										   1);
					}
					time = time2;
					color = newColor;
					keyMap = nextMap;
				}
				topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else if (strcmp(timelineMap->name, "rgb") == 0) {
				float time;
				topi40_spRGBTimeline *timeline = topi40_spRGBTimeline_create(frames, frames * 3, slotIndex);
				keyMap = timelineMap->child;
				time = topi40_Json_getFloat(keyMap, "time", 0);
				toColor2(&color, topi40_Json_getString(keyMap, "color", 0), 1);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					topi40_spRGBTimeline_setFrame(timeline, frame, time, color.r, color.g, color.b);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = topi40_Json_getFloat(nextMap, "time", 0);
					toColor2(&newColor, topi40_Json_getString(nextMap, "color", 0), 1);
					curve = topi40_Json_getItem(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
					}
					time = time2;
					color = newColor;
					keyMap = nextMap;
				}
				topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else if (strcmp(timelineMap->name, "alpha") == 0) {
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child,
															SUPER(topi40_spAlphaTimeline_create(frames,
																						 frames, slotIndex)),
															0, 1));
			} else if (strcmp(timelineMap->name, "rgba2") == 0) {
				float time;
				topi40_spRGBA2Timeline *timeline = topi40_spRGBA2Timeline_create(frames, frames * 7, slotIndex);
				keyMap = timelineMap->child;
				time = topi40_Json_getFloat(keyMap, "time", 0);
				toColor2(&color, topi40_Json_getString(keyMap, "light", 0), 1);
				toColor2(&color2, topi40_Json_getString(keyMap, "dark", 0), 0);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					topi40_spRGBA2Timeline_setFrame(timeline, frame, time, color.r, color.g, color.b, color.a, color2.g,
											 color2.g, color2.b);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = topi40_Json_getFloat(nextMap, "time", 0);
					toColor2(&newColor, topi40_Json_getString(nextMap, "light", 0), 1);
					toColor2(&newColor2, topi40_Json_getString(nextMap, "dark", 0), 0);
					curve = topi40_Json_getItem(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, color.a, newColor.a,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 4, time, time2, color2.r, newColor2.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 5, time, time2, color2.g, newColor2.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 6, time, time2, color2.b, newColor2.b,
										   1);
					}
					time = time2;
					color = newColor;
					color2 = newColor2;
					keyMap = nextMap;
				}
				topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else if (strcmp(timelineMap->name, "rgb2") == 0) {
				float time;
				topi40_spRGBA2Timeline *timeline = topi40_spRGBA2Timeline_create(frames, frames * 6, slotIndex);
				keyMap = timelineMap->child;
				time = topi40_Json_getFloat(keyMap, "time", 0);
				toColor2(&color, topi40_Json_getString(keyMap, "light", 0), 0);
				toColor2(&color2, topi40_Json_getString(keyMap, "dark", 0), 0);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					topi40_spRGBA2Timeline_setFrame(timeline, frame, time, color.r, color.g, color.b, color.a, color2.r,
											 color2.g, color2.b);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = topi40_Json_getFloat(nextMap, "time", 0);
					toColor2(&newColor, topi40_Json_getString(nextMap, "light", 0), 0);
					toColor2(&newColor2, topi40_Json_getString(nextMap, "dark", 0), 0);
					curve = topi40_Json_getItem(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, color2.r, newColor2.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 4, time, time2, color2.g, newColor2.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 5, time, time2, color2.b, newColor2.b,
										   1);
					}
					time = time2;
					color = newColor;
					color2 = newColor2;
					keyMap = nextMap;
				}
				topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else {
				cleanUpTimelines(timelines);
				_topi40_spSkeletonJson_setError(self, NULL, "Invalid timeline type for a slot: ", timelineMap->name);
				return NULL;
			}
		}
	}

	/* Bone timelines. */
	for (boneMap = bones ? bones->child : 0; boneMap; boneMap = boneMap->next) {
		int boneIndex = -1;
		for (i = 0; i < skeletonData->bonesCount; ++i) {
			if (strcmp(skeletonData->bones[i]->name, boneMap->name) == 0) {
				boneIndex = i;
				break;
			}
		}
		if (boneIndex == -1) {
			cleanUpTimelines(timelines);
			_topi40_spSkeletonJson_setError(self, NULL, "Bone not found: ", boneMap->name);
			return NULL;
		}

		for (timelineMap = boneMap->child; timelineMap; timelineMap = timelineMap->next) {
			int frames = timelineMap->size;
			if (frames == 0) continue;

			if (strcmp(timelineMap->name, "rotate") == 0) {
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child,
															SUPER(topi40_spRotateTimeline_create(frames,
																						  frames,
																						  boneIndex)),
															0, 1));
			} else if (strcmp(timelineMap->name, "translate") == 0) {
				topi40_spTranslateTimeline *timeline = topi40_spTranslateTimeline_create(frames, frames << 1,
																		   boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline2(timelineMap->child, SUPER(timeline), "x", "y", 0, scale));
			} else if (strcmp(timelineMap->name, "translatex") == 0) {
				topi40_spTranslateXTimeline *timeline = topi40_spTranslateXTimeline_create(frames, frames,
																			 boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, scale));
			} else if (strcmp(timelineMap->name, "translatey") == 0) {
				topi40_spTranslateYTimeline *timeline = topi40_spTranslateYTimeline_create(frames, frames,
																			 boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, scale));
			} else if (strcmp(timelineMap->name, "scale") == 0) {
				topi40_spScaleTimeline *timeline = topi40_spScaleTimeline_create(frames, frames << 1,
																   boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline2(timelineMap->child, SUPER(timeline), "x", "y", 1, 1));
			} else if (strcmp(timelineMap->name, "scalex") == 0) {
				topi40_spScaleXTimeline *timeline = topi40_spScaleXTimeline_create(frames, frames, boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 1, 1));
			} else if (strcmp(timelineMap->name, "scaley") == 0) {
				topi40_spScaleYTimeline *timeline = topi40_spScaleYTimeline_create(frames, frames, boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 1, 1));
			} else if (strcmp(timelineMap->name, "shear") == 0) {
				topi40_spShearTimeline *timeline = topi40_spShearTimeline_create(frames, frames << 1,
																   boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline2(timelineMap->child, SUPER(timeline), "x", "y", 0, 1));
			} else if (strcmp(timelineMap->name, "shearx") == 0) {
				topi40_spShearXTimeline *timeline = topi40_spShearXTimeline_create(frames, frames, boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, 1));
			} else if (strcmp(timelineMap->name, "sheary") == 0) {
				topi40_spShearYTimeline *timeline = topi40_spShearYTimeline_create(frames, frames, boneIndex);
				topi40_spTimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, 1));
			} else {
				cleanUpTimelines(timelines);
				_topi40_spSkeletonJson_setError(self, NULL, "Invalid timeline type for a bone: ", timelineMap->name);
				return NULL;
			}
		}
	}

	/* IK constraint timelines. */
	for (constraintMap = ik ? ik->child : 0; constraintMap; constraintMap = constraintMap->next) {
		topi40_spIkConstraintData *constraint;
		topi40_spIkConstraintTimeline *timeline;
		int constraintIndex;
		float time, mix, softness;
		keyMap = constraintMap->child;
		if (keyMap == NULL) continue;

		constraint = topi40_spSkeletonData_findIkConstraint(skeletonData, constraintMap->name);
		constraintIndex = topi40_findIkConstraintIndex(self, skeletonData, constraint, timelines);
		if (constraintIndex == -1) return NULL;
		timeline = topi40_spIkConstraintTimeline_create(constraintMap->size, constraintMap->size << 1, constraintIndex);

		time = topi40_Json_getFloat(keyMap, "time", 0);
		mix = topi40_Json_getFloat(keyMap, "mix", 1);
		softness = topi40_Json_getFloat(keyMap, "softness", 0) * scale;

		for (frame = 0, bezier = 0;; ++frame) {
			float time2, mix2, softness2;
			int bendDirection = topi40_Json_getInt(keyMap, "bendPositive", 1) ? 1 : -1;
			topi40_spIkConstraintTimeline_setFrame(timeline, frame, time, mix, softness, bendDirection,
											topi40_Json_getInt(keyMap, "compress", 0) ? 1 : 0,
											topi40_Json_getInt(keyMap, "stretch", 0) ? 1 : 0);
			nextMap = keyMap->next;
			if (!nextMap) {
				/* timeline.shrink(); // BOZO */
				break;
			}

			time2 = topi40_Json_getFloat(nextMap, "time", 0);
			mix2 = topi40_Json_getFloat(nextMap, "mix", 1);
			softness2 = topi40_Json_getFloat(nextMap, "softness", 0) * scale;
			curve = topi40_Json_getItem(keyMap, "curve");
			if (curve) {
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, mix, mix2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, softness, softness2, scale);
			}

			time = time2;
			mix = mix2;
			softness = softness2;
			keyMap = nextMap;
		}

		topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/* Transform constraint timelines. */
	for (constraintMap = transform ? transform->child : 0; constraintMap; constraintMap = constraintMap->next) {
		topi40_spTransformConstraintData *constraint;
		topi40_spTransformConstraintTimeline *timeline;
		int constraintIndex;
		float time, mixRotate, mixShearY, mixX, mixY, mixScaleX, mixScaleY;
		keyMap = constraintMap->child;
		if (keyMap == NULL) continue;

		constraint = topi40_spSkeletonData_findTransformConstraint(skeletonData, constraintMap->name);
		constraintIndex = topi40_findTransformConstraintIndex(self, skeletonData, constraint, timelines);
		if (constraintIndex == -1) return NULL;
		timeline = topi40_spTransformConstraintTimeline_create(constraintMap->size, constraintMap->size * 6, constraintIndex);

		time = topi40_Json_getFloat(keyMap, "time", 0);
		mixRotate = topi40_Json_getFloat(keyMap, "mixRotate", 1);
		mixShearY = topi40_Json_getFloat(keyMap, "mixShearY", 1);
		mixX = topi40_Json_getFloat(keyMap, "mixX", 1);
		mixY = topi40_Json_getFloat(keyMap, "mixY", mixX);
		mixScaleX = topi40_Json_getFloat(keyMap, "mixScaleX", 1);
		mixScaleY = topi40_Json_getFloat(keyMap, "mixScaleY", mixScaleX);

		for (frame = 0, bezier = 0;; ++frame) {
			float time2, mixRotate2, mixShearY2, mixX2, mixY2, mixScaleX2, mixScaleY2;
			topi40_spTransformConstraintTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY, mixScaleX, mixScaleY,
												   mixShearY);
			nextMap = keyMap->next;
			if (!nextMap) {
				/* timeline.shrink(); // BOZO */
				break;
			}

			time2 = topi40_Json_getFloat(nextMap, "time", 0);
			mixRotate2 = topi40_Json_getFloat(nextMap, "mixRotate", 1);
			mixShearY2 = topi40_Json_getFloat(nextMap, "mixShearY", 1);
			mixX2 = topi40_Json_getFloat(nextMap, "mixX", 1);
			mixY2 = topi40_Json_getFloat(nextMap, "mixY", mixX2);
			mixScaleX2 = topi40_Json_getFloat(nextMap, "mixScaleX", 1);
			mixScaleY2 = topi40_Json_getFloat(nextMap, "mixScaleY", mixScaleX2);
			curve = topi40_Json_getItem(keyMap, "curve");
			if (curve) {
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, mixRotate, mixRotate2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, mixX, mixX2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, mixY, mixY2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, mixScaleX, mixScaleX2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 4, time, time2, mixScaleY, mixScaleY2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 5, time, time2, mixShearY, mixShearY2, 1);
			}

			time = time2;
			mixRotate = mixRotate2;
			mixX = mixX2;
			mixY = mixY2;
			mixScaleX = mixScaleX2;
			mixScaleY = mixScaleY2;
			mixScaleX = mixScaleX2;
			keyMap = nextMap;
		}

		topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/** Path constraint timelines. */
	for (constraintMap = paths ? paths->child : 0; constraintMap; constraintMap = constraintMap->next) {
		topi40_spPathConstraintData *constraint = topi40_spSkeletonData_findPathConstraint(skeletonData, constraintMap->name);
		int constraintIndex = topi40_findPathConstraintIndex(self, skeletonData, constraint, timelines);
		if (constraintIndex == -1) return NULL;
		for (timelineMap = constraintMap->child; timelineMap; timelineMap = timelineMap->next) {
			const char *timelineName;
			int frames;
			keyMap = timelineMap->child;
			if (keyMap == NULL) continue;
			frames = timelineMap->size;
			timelineName = timelineMap->name;
			if (strcmp(timelineName, "position") == 0) {
				topi40_spPathConstraintPositionTimeline *timeline = topi40_spPathConstraintPositionTimeline_create(frames,
																									 frames,
																									 constraintIndex);
				topi40_spTimelineArray_add(timelines, readTimeline(keyMap, SUPER(timeline), 0,
															constraint->positionMode == SP_POSITION_MODE_FIXED ? scale : 1));
			} else if (strcmp(timelineName, "topi40_spacing") == 0) {
				topi40_spCurveTimeline1 *timeline = SUPER(
						topi40_spPathConstraintSpacingTimeline_create(frames, frames, constraintIndex));
				topi40_spTimelineArray_add(timelines, readTimeline(keyMap, timeline, 0,
															constraint->topi40_spacingMode == SP_SPACING_MODE_LENGTH ||
																			constraint->topi40_spacingMode == SP_SPACING_MODE_FIXED
																	? scale
																	: 1));
			} else if (strcmp(timelineName, "mix") == 0) {
				topi40_spPathConstraintMixTimeline *timeline = topi40_spPathConstraintMixTimeline_create(frames,
																						   frames * 3,
																						   constraintIndex);
				float time = topi40_Json_getFloat(keyMap, "time", 0);
				float mixRotate = topi40_Json_getFloat(keyMap, "mixRotate", 1);
				float mixX = topi40_Json_getFloat(keyMap, "mixX", 1);
				float mixY = topi40_Json_getFloat(keyMap, "mixY", mixX);
				for (frame = 0, bezier = 0;; ++frame) {
					float time2, mixRotate2, mixX2, mixY2;
					topi40_spPathConstraintMixTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}

					time2 = topi40_Json_getFloat(nextMap, "time", 0);
					mixRotate2 = topi40_Json_getFloat(nextMap, "mixRotate", 1);
					mixX2 = topi40_Json_getFloat(nextMap, "mixX", 1);
					mixY2 = topi40_Json_getFloat(nextMap, "mixY", mixX2);
					curve = topi40_Json_getItem(keyMap, "curve");
					if (curve != NULL) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, mixRotate, mixRotate2,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, mixX, mixX2, 1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, mixY, mixY2, 1);
					}
					time = time2;
					mixRotate = mixRotate2;
					mixX = mixX2;
					mixY = mixY2;
					keyMap = nextMap;
				}
				topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
			}
		}
	}

	/* Deform timelines. */
	for (constraintMap = deformJson ? deformJson->child : 0; constraintMap; constraintMap = constraintMap->next) {
		topi40_spSkin *skin = topi40_spSkeletonData_findSkin(skeletonData, constraintMap->name);
		for (slotMap = constraintMap->child; slotMap; slotMap = slotMap->next) {
			int slotIndex = findSlotIndex(self, skeletonData, slotMap->name, timelines);
			if (slotIndex == -1) return NULL;

			for (timelineMap = slotMap->child; timelineMap; timelineMap = timelineMap->next) {
				float *tempDeform;
				topi40_spVertexAttachment *attachment;
				int weighted, deformLength;
				topi40_spDeformTimeline *timeline;
				float time;
				keyMap = timelineMap->child;
				if (keyMap == NULL) continue;

				attachment = SUB_CAST(topi40_spVertexAttachment, topi40_spSkin_getAttachment(skin, slotIndex, timelineMap->name));
				if (!attachment) {
					cleanUpTimelines(timelines);
					_topi40_spSkeletonJson_setError(self, 0, "Attachment not found: ", timelineMap->name);
					return NULL;
				}
				weighted = attachment->bones != 0;
				deformLength = weighted ? attachment->verticesCount / 3 * 2 : attachment->verticesCount;
				tempDeform = MALLOC(float, deformLength);
				timeline = topi40_spDeformTimeline_create(timelineMap->size, deformLength, timelineMap->size, slotIndex,
												   attachment);

				time = topi40_Json_getFloat(keyMap, "time", 0);
				for (frame = 0, bezier = 0;; ++frame) {
					Json *vertices = topi40_Json_getItem(keyMap, "vertices");
					float *deform;
					float time2;

					if (!vertices) {
						if (weighted) {
							deform = tempDeform;
							memset(deform, 0, sizeof(float) * deformLength);
						} else
							deform = attachment->vertices;
					} else {
						int v, start = topi40_Json_getInt(keyMap, "offset", 0);
						Json *vertex;
						deform = tempDeform;
						memset(deform, 0, sizeof(float) * start);
						if (self->scale == 1) {
							for (vertex = vertices->child, v = start; vertex; vertex = vertex->next, ++v)
								deform[v] = vertex->valueFloat;
						} else {
							for (vertex = vertices->child, v = start; vertex; vertex = vertex->next, ++v)
								deform[v] = vertex->valueFloat * self->scale;
						}
						memset(deform + v, 0, sizeof(float) * (deformLength - v));
						if (!weighted) {
							float *verticesValues = attachment->vertices;
							for (v = 0; v < deformLength; ++v)
								deform[v] += verticesValues[v];
						}
					}
					topi40_spDeformTimeline_setFrame(timeline, frame, time, deform);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = topi40_Json_getFloat(nextMap, "time", 0);
					curve = topi40_Json_getItem(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, 0, 1, 1);
					}
					time = time2;
					keyMap = nextMap;
				}
				FREE(tempDeform);

				topi40_spTimelineArray_add(timelines, SUPER(SUPER(timeline)));
			}
		}
	}

	/* Draw order timeline. */
	if (drawOrderJson) {
		topi40_spDrawOrderTimeline *timeline = topi40_spDrawOrderTimeline_create(drawOrderJson->size, skeletonData->slotsCount);
		for (keyMap = drawOrderJson->child, frame = 0; keyMap; keyMap = keyMap->next, ++frame) {
			int ii;
			int *drawOrder = 0;
			Json *offsets = topi40_Json_getItem(keyMap, "offsets");
			if (offsets) {
				Json *offsetMap;
				int *unchanged = MALLOC(int, skeletonData->slotsCount - offsets->size);
				int originalIndex = 0, unchangedIndex = 0;

				drawOrder = MALLOC(int, skeletonData->slotsCount);
				for (ii = skeletonData->slotsCount - 1; ii >= 0; --ii)
					drawOrder[ii] = -1;

				for (offsetMap = offsets->child; offsetMap; offsetMap = offsetMap->next) {
					int slotIndex = findSlotIndex(self, skeletonData, topi40_Json_getString(offsetMap, "slot", 0), timelines);
					if (slotIndex == -1) return NULL;

					/* Collect unchanged items. */
					while (originalIndex != slotIndex)
						unchanged[unchangedIndex++] = originalIndex++;
					/* Set changed items. */
					drawOrder[originalIndex + topi40_Json_getInt(offsetMap, "offset", 0)] = originalIndex;
					originalIndex++;
				}
				/* Collect remaining unchanged items. */
				while (originalIndex < skeletonData->slotsCount)
					unchanged[unchangedIndex++] = originalIndex++;
				/* Fill in unchanged items. */
				for (ii = skeletonData->slotsCount - 1; ii >= 0; ii--)
					if (drawOrder[ii] == -1) drawOrder[ii] = unchanged[--unchangedIndex];
				FREE(unchanged);
			}
			topi40_spDrawOrderTimeline_setFrame(timeline, frame, topi40_Json_getFloat(keyMap, "time", 0), drawOrder);
			FREE(drawOrder);
		}

		topi40_spTimelineArray_add(timelines, SUPER(timeline));
	}

	/* Event timeline. */
	if (events) {
		topi40_spEventTimeline *timeline = topi40_spEventTimeline_create(events->size);
		for (keyMap = events->child, frame = 0; keyMap; keyMap = keyMap->next, ++frame) {
			topi40_spEvent *event;
			const char *stringValue;
			topi40_spEventData *eventData = topi40_spSkeletonData_findEvent(skeletonData, topi40_Json_getString(keyMap, "name", 0));
			if (!eventData) {
				cleanUpTimelines(timelines);
				_topi40_spSkeletonJson_setError(self, 0, "Event not found: ", topi40_Json_getString(keyMap, "name", 0));
				return NULL;
			}
			event = topi40_spEvent_create(topi40_Json_getFloat(keyMap, "time", 0), eventData);
			event->intValue = topi40_Json_getInt(keyMap, "int", eventData->intValue);
			event->floatValue = topi40_Json_getFloat(keyMap, "float", eventData->floatValue);
			stringValue = topi40_Json_getString(keyMap, "string", eventData->stringValue);
			if (stringValue) MALLOC_STR(event->stringValue, stringValue);
			if (eventData->audioPath) {
				event->volume = topi40_Json_getFloat(keyMap, "volume", 1);
				event->balance = topi40_Json_getFloat(keyMap, "volume", 0);
			}
			topi40_spEventTimeline_setFrame(timeline, frame, event);
		}
		topi40_spTimelineArray_add(timelines, SUPER(timeline));
	}

	duration = 0;
	for (i = 0, n = timelines->size; i < n; ++i)
		duration = MAX(duration, topi40_spTimeline_getDuration(timelines->items[i]));
	return topi40_spAnimation_create(root->name, timelines, duration);
}

static void
_readVertices(topi40_spSkeletonJson *self, Json *attachmentMap, topi40_spVertexAttachment *attachment, int verticesLength) {
	Json *entry;
	float *vertices;
	int i, n, nn, entrySize;
	topi40_spFloatArray *weights;
	topi40_spIntArray *bones;

	attachment->worldVerticesLength = verticesLength;

	entry = topi40_Json_getItem(attachmentMap, "vertices");
	entrySize = entry->size;
	vertices = MALLOC(float, entrySize);
	for (entry = entry->child, i = 0; entry; entry = entry->next, ++i)
		vertices[i] = entry->valueFloat;

	if (verticesLength == entrySize) {
		if (self->scale != 1)
			for (i = 0; i < entrySize; ++i)
				vertices[i] *= self->scale;
		attachment->verticesCount = verticesLength;
		attachment->vertices = vertices;

		attachment->bonesCount = 0;
		attachment->bones = 0;
		return;
	}

	weights = topi40_spFloatArray_create(verticesLength * 3 * 3);
	bones = topi40_spIntArray_create(verticesLength * 3);

	for (i = 0, n = entrySize; i < n;) {
		int boneCount = (int) vertices[i++];
		topi40_spIntArray_add(bones, boneCount);
		for (nn = i + boneCount * 4; i < nn; i += 4) {
			topi40_spIntArray_add(bones, (int) vertices[i]);
			topi40_spFloatArray_add(weights, vertices[i + 1] * self->scale);
			topi40_spFloatArray_add(weights, vertices[i + 2] * self->scale);
			topi40_spFloatArray_add(weights, vertices[i + 3]);
		}
	}

	attachment->verticesCount = weights->size;
	attachment->vertices = weights->items;
	FREE(weights);
	attachment->bonesCount = bones->size;
	attachment->bones = bones->items;
	FREE(bones);

	FREE(vertices);
}

topi40_spSkeletonData *topi40_spSkeletonJson_readSkeletonDataFile(topi40_spSkeletonJson *self, const char *path) {
	int length;
	topi40_spSkeletonData *skeletonData;
	const char *json = _topi40_spUtil_readFile(path, &length);
	if (length == 0 || !json) {
		_topi40_spSkeletonJson_setError(self, 0, "Unable to read skeleton file: ", path);
		return NULL;
	}
	skeletonData = topi40_spSkeletonJson_readSkeletonData(self, json);
	FREE(json);
	return skeletonData;
}

topi40_spSkeletonData *topi40_spSkeletonJson_readSkeletonData(topi40_spSkeletonJson *self, const char *json) {
	int i, ii;
	topi40_spSkeletonData *skeletonData;
	Json *root, *skeleton, *bones, *boneMap, *ik, *transform, *pathJson, *slots, *skins, *animations, *events;
	_topi40_spSkeletonJson *internal = SUB_CAST(_topi40_spSkeletonJson, self);

	FREE(self->error);
	CONST_CAST(char *, self->error) = 0;
	internal->linkedMeshCount = 0;

	root = topi40_Json_create(json);
	if (!root) {
		_topi40_spSkeletonJson_setError(self, 0, "Invalid skeleton JSON: ", topi40_Json_getError());
		return NULL;
	}

	skeletonData = topi40_spSkeletonData_create();

	skeleton = topi40_Json_getItem(root, "skeleton");
	if (skeleton) {
		MALLOC_STR(skeletonData->hash, topi40_Json_getString(skeleton, "hash", 0));
		MALLOC_STR(skeletonData->version, topi40_Json_getString(skeleton, "topi40_spine", 0));
		skeletonData->x = topi40_Json_getFloat(skeleton, "x", 0);
		skeletonData->y = topi40_Json_getFloat(skeleton, "y", 0);
		skeletonData->width = topi40_Json_getFloat(skeleton, "width", 0);
		skeletonData->height = topi40_Json_getFloat(skeleton, "height", 0);
		skeletonData->fps = topi40_Json_getFloat(skeleton, "fps", 30);
		skeletonData->imagesPath = topi40_Json_getString(skeleton, "images", 0);
		if (skeletonData->imagesPath) {
			char *tmp = NULL;
			MALLOC_STR(tmp, skeletonData->imagesPath);
			skeletonData->imagesPath = tmp;
		}
		skeletonData->audioPath = topi40_Json_getString(skeleton, "audio", 0);
		if (skeletonData->audioPath) {
			char *tmp = NULL;
			MALLOC_STR(tmp, skeletonData->audioPath);
			skeletonData->audioPath = tmp;
		}
	}

	/* Bones. */
	bones = topi40_Json_getItem(root, "bones");
	skeletonData->bones = MALLOC(topi40_spBoneData *, bones->size);
	for (boneMap = bones->child, i = 0; boneMap; boneMap = boneMap->next, ++i) {
		topi40_spBoneData *data;
		const char *transformMode;
		const char *color;

		topi40_spBoneData *parent = 0;
		const char *parentName = topi40_Json_getString(boneMap, "parent", 0);
		if (parentName) {
			parent = topi40_spSkeletonData_findBone(skeletonData, parentName);
			if (!parent) {
				topi40_spSkeletonData_dispose(skeletonData);
				_topi40_spSkeletonJson_setError(self, root, "Parent bone not found: ", parentName);
				return NULL;
			}
		}

		data = topi40_spBoneData_create(skeletonData->bonesCount, topi40_Json_getString(boneMap, "name", 0), parent);
		data->length = topi40_Json_getFloat(boneMap, "length", 0) * self->scale;
		data->x = topi40_Json_getFloat(boneMap, "x", 0) * self->scale;
		data->y = topi40_Json_getFloat(boneMap, "y", 0) * self->scale;
		data->rotation = topi40_Json_getFloat(boneMap, "rotation", 0);
		data->scaleX = topi40_Json_getFloat(boneMap, "scaleX", 1);
		data->scaleY = topi40_Json_getFloat(boneMap, "scaleY", 1);
		data->shearX = topi40_Json_getFloat(boneMap, "shearX", 0);
		data->shearY = topi40_Json_getFloat(boneMap, "shearY", 0);
		transformMode = topi40_Json_getString(boneMap, "transform", "normal");
		data->transformMode = SP_TRANSFORMMODE_NORMAL;
		if (strcmp(transformMode, "normal") == 0) data->transformMode = SP_TRANSFORMMODE_NORMAL;
		else if (strcmp(transformMode, "onlyTranslation") == 0)
			data->transformMode = SP_TRANSFORMMODE_ONLYTRANSLATION;
		else if (strcmp(transformMode, "noRotationOrReflection") == 0)
			data->transformMode = SP_TRANSFORMMODE_NOROTATIONORREFLECTION;
		else if (strcmp(transformMode, "noScale") == 0)
			data->transformMode = SP_TRANSFORMMODE_NOSCALE;
		else if (strcmp(transformMode, "noScaleOrReflection") == 0)
			data->transformMode = SP_TRANSFORMMODE_NOSCALEORREFLECTION;
		data->skinRequired = topi40_Json_getInt(boneMap, "skin", 0) ? 1 : 0;

		color = topi40_Json_getString(boneMap, "color", 0);
		if (color) toColor2(&data->color, color, -1);

		skeletonData->bones[i] = data;
		skeletonData->bonesCount++;
	}

	/* Slots. */
	slots = topi40_Json_getItem(root, "slots");
	if (slots) {
		Json *slotMap;
		skeletonData->slotsCount = slots->size;
		skeletonData->slots = MALLOC(topi40_spSlotData *, slots->size);
		for (slotMap = slots->child, i = 0; slotMap; slotMap = slotMap->next, ++i) {
			topi40_spSlotData *data;
			const char *color;
			const char *dark;
			Json *item;

			const char *boneName = topi40_Json_getString(slotMap, "bone", 0);
			topi40_spBoneData *boneData = topi40_spSkeletonData_findBone(skeletonData, boneName);
			if (!boneData) {
				topi40_spSkeletonData_dispose(skeletonData);
				_topi40_spSkeletonJson_setError(self, root, "Slot bone not found: ", boneName);
				return NULL;
			}

			data = topi40_spSlotData_create(i, topi40_Json_getString(slotMap, "name", 0), boneData);

			color = topi40_Json_getString(slotMap, "color", 0);
			if (color) {
				topi40_spColor_setFromFloats(&data->color,
									  toColor(color, 0),
									  toColor(color, 1),
									  toColor(color, 2),
									  toColor(color, 3));
			}

			dark = topi40_Json_getString(slotMap, "dark", 0);
			if (dark) {
				data->darkColor = topi40_spColor_create();
				topi40_spColor_setFromFloats(data->darkColor,
									  toColor(dark, 0),
									  toColor(dark, 1),
									  toColor(dark, 2),
									  toColor(dark, 3));
			}

			item = topi40_Json_getItem(slotMap, "attachment");
			if (item) topi40_spSlotData_setAttachmentName(data, item->valueString);

			item = topi40_Json_getItem(slotMap, "blend");
			if (item) {
				if (strcmp(item->valueString, "additive") == 0)
					data->blendMode = SP_BLEND_MODE_ADDITIVE;
				else if (strcmp(item->valueString, "multiply") == 0)
					data->blendMode = SP_BLEND_MODE_MULTIPLY;
				else if (strcmp(item->valueString, "screen") == 0)
					data->blendMode = SP_BLEND_MODE_SCREEN;
			}

			skeletonData->slots[i] = data;
		}
	}

	/* IK constraints. */
	ik = topi40_Json_getItem(root, "ik");
	if (ik) {
		Json *constraintMap;
		skeletonData->ikConstraintsCount = ik->size;
		skeletonData->ikConstraints = MALLOC(topi40_spIkConstraintData *, ik->size);
		for (constraintMap = ik->child, i = 0; constraintMap; constraintMap = constraintMap->next, ++i) {
			const char *targetName;

			topi40_spIkConstraintData *data = topi40_spIkConstraintData_create(topi40_Json_getString(constraintMap, "name", 0));
			data->order = topi40_Json_getInt(constraintMap, "order", 0);
			data->skinRequired = topi40_Json_getInt(constraintMap, "skin", 0) ? 1 : 0;

			boneMap = topi40_Json_getItem(constraintMap, "bones");
			data->bonesCount = boneMap->size;
			data->bones = MALLOC(topi40_spBoneData *, boneMap->size);
			for (boneMap = boneMap->child, ii = 0; boneMap; boneMap = boneMap->next, ++ii) {
				data->bones[ii] = topi40_spSkeletonData_findBone(skeletonData, boneMap->valueString);
				if (!data->bones[ii]) {
					topi40_spSkeletonData_dispose(skeletonData);
					_topi40_spSkeletonJson_setError(self, root, "IK bone not found: ", boneMap->valueString);
					return NULL;
				}
			}

			targetName = topi40_Json_getString(constraintMap, "target", 0);
			data->target = topi40_spSkeletonData_findBone(skeletonData, targetName);
			if (!data->target) {
				topi40_spSkeletonData_dispose(skeletonData);
				_topi40_spSkeletonJson_setError(self, root, "Target bone not found: ", targetName);
				return NULL;
			}

			data->bendDirection = topi40_Json_getInt(constraintMap, "bendPositive", 1) ? 1 : -1;
			data->compress = topi40_Json_getInt(constraintMap, "compress", 0) ? 1 : 0;
			data->stretch = topi40_Json_getInt(constraintMap, "stretch", 0) ? 1 : 0;
			data->uniform = topi40_Json_getInt(constraintMap, "uniform", 0) ? 1 : 0;
			data->mix = topi40_Json_getFloat(constraintMap, "mix", 1);
			data->softness = topi40_Json_getFloat(constraintMap, "softness", 0) * self->scale;

			skeletonData->ikConstraints[i] = data;
		}
	}

	/* Transform constraints. */
	transform = topi40_Json_getItem(root, "transform");
	if (transform) {
		Json *constraintMap;
		skeletonData->transformConstraintsCount = transform->size;
		skeletonData->transformConstraints = MALLOC(topi40_spTransformConstraintData *, transform->size);
		for (constraintMap = transform->child, i = 0; constraintMap; constraintMap = constraintMap->next, ++i) {
			const char *name;

			topi40_spTransformConstraintData *data = topi40_spTransformConstraintData_create(
					topi40_Json_getString(constraintMap, "name", 0));
			data->order = topi40_Json_getInt(constraintMap, "order", 0);
			data->skinRequired = topi40_Json_getInt(constraintMap, "skin", 0) ? 1 : 0;

			boneMap = topi40_Json_getItem(constraintMap, "bones");
			data->bonesCount = boneMap->size;
			CONST_CAST(topi40_spBoneData **, data->bones) = MALLOC(topi40_spBoneData *, boneMap->size);
			for (boneMap = boneMap->child, ii = 0; boneMap; boneMap = boneMap->next, ++ii) {
				data->bones[ii] = topi40_spSkeletonData_findBone(skeletonData, boneMap->valueString);
				if (!data->bones[ii]) {
					topi40_spSkeletonData_dispose(skeletonData);
					_topi40_spSkeletonJson_setError(self, root, "Transform bone not found: ", boneMap->valueString);
					return NULL;
				}
			}

			name = topi40_Json_getString(constraintMap, "target", 0);
			data->target = topi40_spSkeletonData_findBone(skeletonData, name);
			if (!data->target) {
				topi40_spSkeletonData_dispose(skeletonData);
				_topi40_spSkeletonJson_setError(self, root, "Target bone not found: ", name);
				return NULL;
			}

			data->local = topi40_Json_getInt(constraintMap, "local", 0);
			data->relative = topi40_Json_getInt(constraintMap, "relative", 0);
			data->offsetRotation = topi40_Json_getFloat(constraintMap, "rotation", 0);
			data->offsetX = topi40_Json_getFloat(constraintMap, "x", 0) * self->scale;
			data->offsetY = topi40_Json_getFloat(constraintMap, "y", 0) * self->scale;
			data->offsetScaleX = topi40_Json_getFloat(constraintMap, "scaleX", 0);
			data->offsetScaleY = topi40_Json_getFloat(constraintMap, "scaleY", 0);
			data->offsetShearY = topi40_Json_getFloat(constraintMap, "shearY", 0);

			data->mixRotate = topi40_Json_getFloat(constraintMap, "mixRotate", 1);
			data->mixX = topi40_Json_getFloat(constraintMap, "mixX", 1);
			data->mixY = topi40_Json_getFloat(constraintMap, "mixY", data->mixX);
			data->mixScaleX = topi40_Json_getFloat(constraintMap, "mixScaleX", 1);
			data->mixScaleY = topi40_Json_getFloat(constraintMap, "mixScaleY", data->mixScaleX);
			data->mixShearY = topi40_Json_getFloat(constraintMap, "mixShearY", 1);

			skeletonData->transformConstraints[i] = data;
		}
	}

	/* Path constraints */
	pathJson = topi40_Json_getItem(root, "path");
	if (pathJson) {
		Json *constraintMap;
		skeletonData->pathConstraintsCount = pathJson->size;
		skeletonData->pathConstraints = MALLOC(topi40_spPathConstraintData *, pathJson->size);
		for (constraintMap = pathJson->child, i = 0; constraintMap; constraintMap = constraintMap->next, ++i) {
			const char *name;
			const char *item;

			topi40_spPathConstraintData *data = topi40_spPathConstraintData_create(topi40_Json_getString(constraintMap, "name", 0));
			data->order = topi40_Json_getInt(constraintMap, "order", 0);
			data->skinRequired = topi40_Json_getInt(constraintMap, "skin", 0) ? 1 : 0;

			boneMap = topi40_Json_getItem(constraintMap, "bones");
			data->bonesCount = boneMap->size;
			CONST_CAST(topi40_spBoneData **, data->bones) = MALLOC(topi40_spBoneData *, boneMap->size);
			for (boneMap = boneMap->child, ii = 0; boneMap; boneMap = boneMap->next, ++ii) {
				data->bones[ii] = topi40_spSkeletonData_findBone(skeletonData, boneMap->valueString);
				if (!data->bones[ii]) {
					topi40_spSkeletonData_dispose(skeletonData);
					_topi40_spSkeletonJson_setError(self, root, "Path bone not found: ", boneMap->valueString);
					return NULL;
				}
			}

			name = topi40_Json_getString(constraintMap, "target", 0);
			data->target = topi40_spSkeletonData_findSlot(skeletonData, name);
			if (!data->target) {
				topi40_spSkeletonData_dispose(skeletonData);
				_topi40_spSkeletonJson_setError(self, root, "Target slot not found: ", name);
				return NULL;
			}

			item = topi40_Json_getString(constraintMap, "positionMode", "percent");
			if (strcmp(item, "fixed") == 0) data->positionMode = SP_POSITION_MODE_FIXED;
			else if (strcmp(item, "percent") == 0)
				data->positionMode = SP_POSITION_MODE_PERCENT;

			item = topi40_Json_getString(constraintMap, "topi40_spacingMode", "length");
			if (strcmp(item, "length") == 0) data->topi40_spacingMode = SP_SPACING_MODE_LENGTH;
			else if (strcmp(item, "fixed") == 0)
				data->topi40_spacingMode = SP_SPACING_MODE_FIXED;
			else if (strcmp(item, "percent") == 0)
				data->topi40_spacingMode = SP_SPACING_MODE_PERCENT;
			else
				data->topi40_spacingMode = SP_SPACING_MODE_PROPORTIONAL;

			item = topi40_Json_getString(constraintMap, "rotateMode", "tangent");
			if (strcmp(item, "tangent") == 0) data->rotateMode = SP_ROTATE_MODE_TANGENT;
			else if (strcmp(item, "chain") == 0)
				data->rotateMode = SP_ROTATE_MODE_CHAIN;
			else if (strcmp(item, "chainScale") == 0)
				data->rotateMode = SP_ROTATE_MODE_CHAIN_SCALE;

			data->offsetRotation = topi40_Json_getFloat(constraintMap, "rotation", 0);
			data->position = topi40_Json_getFloat(constraintMap, "position", 0);
			if (data->positionMode == SP_POSITION_MODE_FIXED) data->position *= self->scale;
			data->topi40_spacing = topi40_Json_getFloat(constraintMap, "topi40_spacing", 0);
			if (data->topi40_spacingMode == SP_SPACING_MODE_LENGTH || data->topi40_spacingMode == SP_SPACING_MODE_FIXED)
				data->topi40_spacing *= self->scale;
			data->mixRotate = topi40_Json_getFloat(constraintMap, "mixRotate", 1);
			data->mixX = topi40_Json_getFloat(constraintMap, "mixX", 1);
			data->mixY = topi40_Json_getFloat(constraintMap, "mixY", data->mixX);

			skeletonData->pathConstraints[i] = data;
		}
	}

	/* Skins. */
	skins = topi40_Json_getItem(root, "skins");
	if (skins) {
		Json *skinMap;
		skeletonData->skins = MALLOC(topi40_spSkin *, skins->size);
		for (skinMap = skins->child, i = 0; skinMap; skinMap = skinMap->next, ++i) {
			Json *attachmentsMap;
			Json *curves;
			Json *skinPart;
			topi40_spSkin *skin = topi40_spSkin_create(topi40_Json_getString(skinMap, "name", ""));

			skinPart = topi40_Json_getItem(skinMap, "bones");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					topi40_spBoneData *bone = topi40_spSkeletonData_findBone(skeletonData, skinPart->valueString);
					if (!bone) {
						topi40_spSkeletonData_dispose(skeletonData);
						_topi40_spSkeletonJson_setError(self, root, "Skin bone constraint not found: ", skinPart->valueString);
						return NULL;
					}
					topi40_spBoneDataArray_add(skin->bones, bone);
				}
			}

			skinPart = topi40_Json_getItem(skinMap, "ik");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					topi40_spIkConstraintData *constraint = topi40_spSkeletonData_findIkConstraint(skeletonData,
																					 skinPart->valueString);
					if (!constraint) {
						topi40_spSkeletonData_dispose(skeletonData);
						_topi40_spSkeletonJson_setError(self, root, "Skin IK constraint not found: ", skinPart->valueString);
						return NULL;
					}
					topi40_spIkConstraintDataArray_add(skin->ikConstraints, constraint);
				}
			}

			skinPart = topi40_Json_getItem(skinMap, "path");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					topi40_spPathConstraintData *constraint = topi40_spSkeletonData_findPathConstraint(skeletonData,
																						 skinPart->valueString);
					if (!constraint) {
						topi40_spSkeletonData_dispose(skeletonData);
						_topi40_spSkeletonJson_setError(self, root, "Skin path constraint not found: ", skinPart->valueString);
						return NULL;
					}
					topi40_spPathConstraintDataArray_add(skin->pathConstraints, constraint);
				}
			}

			skinPart = topi40_Json_getItem(skinMap, "transform");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					topi40_spTransformConstraintData *constraint = topi40_spSkeletonData_findTransformConstraint(skeletonData,
																								   skinPart->valueString);
					if (!constraint) {
						topi40_spSkeletonData_dispose(skeletonData);
						_topi40_spSkeletonJson_setError(self, root, "Skin transform constraint not found: ",
												 skinPart->valueString);
						return NULL;
					}
					topi40_spTransformConstraintDataArray_add(skin->transformConstraints, constraint);
				}
			}

			skeletonData->skins[skeletonData->skinsCount++] = skin;
			if (strcmp(skin->name, "default") == 0) skeletonData->defaultSkin = skin;

			for (attachmentsMap = topi40_Json_getItem(skinMap,
											   "attachments")
										  ->child;
				 attachmentsMap; attachmentsMap = attachmentsMap->next) {
				topi40_spSlotData *slot = topi40_spSkeletonData_findSlot(skeletonData, attachmentsMap->name);
				Json *attachmentMap;

				for (attachmentMap = attachmentsMap->child; attachmentMap; attachmentMap = attachmentMap->next) {
					topi40_spAttachment *attachment;
					const char *skinAttachmentName = attachmentMap->name;
					const char *attachmentName = topi40_Json_getString(attachmentMap, "name", skinAttachmentName);
					const char *path = topi40_Json_getString(attachmentMap, "path", attachmentName);
					const char *color;
					Json *entry;

					const char *typeString = topi40_Json_getString(attachmentMap, "type", "region");
					topi40_spAttachmentType type;
					if (strcmp(typeString, "region") == 0) type = SP_ATTACHMENT_REGION;
					else if (strcmp(typeString, "mesh") == 0)
						type = SP_ATTACHMENT_MESH;
					else if (strcmp(typeString, "linkedmesh") == 0)
						type = SP_ATTACHMENT_LINKED_MESH;
					else if (strcmp(typeString, "boundingbox") == 0)
						type = SP_ATTACHMENT_BOUNDING_BOX;
					else if (strcmp(typeString, "path") == 0)
						type = SP_ATTACHMENT_PATH;
					else if (strcmp(typeString, "clipping") == 0)
						type = SP_ATTACHMENT_CLIPPING;
					else if (strcmp(typeString, "point") == 0)
						type = SP_ATTACHMENT_POINT;
					else {
						topi40_spSkeletonData_dispose(skeletonData);
						_topi40_spSkeletonJson_setError(self, root, "Unknown attachment type: ", typeString);
						return NULL;
					}

					attachment = topi40_spAttachmentLoader_createAttachment(self->attachmentLoader, skin, type, attachmentName,
																	 path);
					if (!attachment) {
						if (self->attachmentLoader->error1) {
							topi40_spSkeletonData_dispose(skeletonData);
							_topi40_spSkeletonJson_setError(self, root, self->attachmentLoader->error1,
													 self->attachmentLoader->error2);
							return NULL;
						}
						continue;
					}

					switch (attachment->type) {
						case SP_ATTACHMENT_REGION: {
							topi40_spRegionAttachment *region = SUB_CAST(topi40_spRegionAttachment, attachment);
							if (path) MALLOC_STR(region->path, path);
							region->x = topi40_Json_getFloat(attachmentMap, "x", 0) * self->scale;
							region->y = topi40_Json_getFloat(attachmentMap, "y", 0) * self->scale;
							region->scaleX = topi40_Json_getFloat(attachmentMap, "scaleX", 1);
							region->scaleY = topi40_Json_getFloat(attachmentMap, "scaleY", 1);
							region->rotation = topi40_Json_getFloat(attachmentMap, "rotation", 0);
							region->width = topi40_Json_getFloat(attachmentMap, "width", 32) * self->scale;
							region->height = topi40_Json_getFloat(attachmentMap, "height", 32) * self->scale;

							color = topi40_Json_getString(attachmentMap, "color", 0);
							if (color) {
								topi40_spColor_setFromFloats(&region->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}

							topi40_spRegionAttachment_updateOffset(region);

							topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							break;
						}
						case SP_ATTACHMENT_MESH:
						case SP_ATTACHMENT_LINKED_MESH: {
							topi40_spMeshAttachment *mesh = SUB_CAST(topi40_spMeshAttachment, attachment);

							MALLOC_STR(mesh->path, path);

							color = topi40_Json_getString(attachmentMap, "color", 0);
							if (color) {
								topi40_spColor_setFromFloats(&mesh->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}

							mesh->width = topi40_Json_getFloat(attachmentMap, "width", 32) * self->scale;
							mesh->height = topi40_Json_getFloat(attachmentMap, "height", 32) * self->scale;

							entry = topi40_Json_getItem(attachmentMap, "parent");
							if (!entry) {
								int verticesLength;
								entry = topi40_Json_getItem(attachmentMap, "triangles");
								mesh->trianglesCount = entry->size;
								mesh->triangles = MALLOC(unsigned short, entry->size);
								for (entry = entry->child, ii = 0; entry; entry = entry->next, ++ii)
									mesh->triangles[ii] = (unsigned short) entry->valueInt;

								entry = topi40_Json_getItem(attachmentMap, "uvs");
								verticesLength = entry->size;
								mesh->regionUVs = MALLOC(float, verticesLength);
								for (entry = entry->child, ii = 0; entry; entry = entry->next, ++ii)
									mesh->regionUVs[ii] = entry->valueFloat;

								_readVertices(self, attachmentMap, SUPER(mesh), verticesLength);

								topi40_spMeshAttachment_updateUVs(mesh);

								mesh->hullLength = topi40_Json_getInt(attachmentMap, "hull", 0);

								entry = topi40_Json_getItem(attachmentMap, "edges");
								if (entry) {
									mesh->edgesCount = entry->size;
									mesh->edges = MALLOC(int, entry->size);
									for (entry = entry->child, ii = 0; entry; entry = entry->next, ++ii)
										mesh->edges[ii] = entry->valueInt;
								}

								topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							} else {
								int inheritDeform = topi40_Json_getInt(attachmentMap, "deform", 1);
								_topi40_spSkeletonJson_addLinkedMesh(self, SUB_CAST(topi40_spMeshAttachment, attachment),
															  topi40_Json_getString(attachmentMap, "skin", 0), slot->index,
															  entry->valueString, inheritDeform);
							}
							break;
						}
						case SP_ATTACHMENT_BOUNDING_BOX: {
							topi40_spBoundingBoxAttachment *box = SUB_CAST(topi40_spBoundingBoxAttachment, attachment);
							int vertexCount = topi40_Json_getInt(attachmentMap, "vertexCount", 0) << 1;
							_readVertices(self, attachmentMap, SUPER(box), vertexCount);
							box->super.verticesCount = vertexCount;
							color = topi40_Json_getString(attachmentMap, "color", 0);
							if (color) {
								topi40_spColor_setFromFloats(&box->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							break;
						}
						case SP_ATTACHMENT_PATH: {
							topi40_spPathAttachment *pathAttachment = SUB_CAST(topi40_spPathAttachment, attachment);
							int vertexCount = 0;
							pathAttachment->closed = topi40_Json_getInt(attachmentMap, "closed", 0);
							pathAttachment->constantSpeed = topi40_Json_getInt(attachmentMap, "constantSpeed", 1);
							vertexCount = topi40_Json_getInt(attachmentMap, "vertexCount", 0);
							_readVertices(self, attachmentMap, SUPER(pathAttachment), vertexCount << 1);

							pathAttachment->lengthsLength = vertexCount / 3;
							pathAttachment->lengths = MALLOC(float, pathAttachment->lengthsLength);

							curves = topi40_Json_getItem(attachmentMap, "lengths");
							for (curves = curves->child, ii = 0; curves; curves = curves->next, ++ii)
								pathAttachment->lengths[ii] = curves->valueFloat * self->scale;
							color = topi40_Json_getString(attachmentMap, "color", 0);
							if (color) {
								topi40_spColor_setFromFloats(&pathAttachment->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							break;
						}
						case SP_ATTACHMENT_POINT: {
							topi40_spPointAttachment *point = SUB_CAST(topi40_spPointAttachment, attachment);
							point->x = topi40_Json_getFloat(attachmentMap, "x", 0) * self->scale;
							point->y = topi40_Json_getFloat(attachmentMap, "y", 0) * self->scale;
							point->rotation = topi40_Json_getFloat(attachmentMap, "rotation", 0);

							color = topi40_Json_getString(attachmentMap, "color", 0);
							if (color) {
								topi40_spColor_setFromFloats(&point->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							break;
						}
						case SP_ATTACHMENT_CLIPPING: {
							topi40_spClippingAttachment *clip = SUB_CAST(topi40_spClippingAttachment, attachment);
							int vertexCount = 0;
							const char *end = topi40_Json_getString(attachmentMap, "end", 0);
							if (end) {
								topi40_spSlotData *endSlot = topi40_spSkeletonData_findSlot(skeletonData, end);
								clip->endSlot = endSlot;
							}
							vertexCount = topi40_Json_getInt(attachmentMap, "vertexCount", 0) << 1;
							_readVertices(self, attachmentMap, SUPER(clip), vertexCount);
							color = topi40_Json_getString(attachmentMap, "color", 0);
							if (color) {
								topi40_spColor_setFromFloats(&clip->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							topi40_spAttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							break;
						}
					}

					topi40_spSkin_setAttachment(skin, slot->index, skinAttachmentName, attachment);
				}
			}
		}
	}

	/* Linked meshes. */
	for (i = 0; i < internal->linkedMeshCount; ++i) {
		topi40_spAttachment *parent;
		_topi40_spLinkedMesh *linkedMesh = internal->linkedMeshes + i;
		topi40_spSkin *skin = !linkedMesh->skin ? skeletonData->defaultSkin : topi40_spSkeletonData_findSkin(skeletonData, linkedMesh->skin);
		if (!skin) {
			topi40_spSkeletonData_dispose(skeletonData);
			_topi40_spSkeletonJson_setError(self, 0, "Skin not found: ", linkedMesh->skin);
			return NULL;
		}
		parent = topi40_spSkin_getAttachment(skin, linkedMesh->slotIndex, linkedMesh->parent);
		if (!parent) {
			topi40_spSkeletonData_dispose(skeletonData);
			_topi40_spSkeletonJson_setError(self, 0, "Parent mesh not found: ", linkedMesh->parent);
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
	events = topi40_Json_getItem(root, "events");
	if (events) {
		Json *eventMap;
		const char *stringValue;
		const char *audioPath;
		skeletonData->eventsCount = events->size;
		skeletonData->events = MALLOC(topi40_spEventData *, events->size);
		for (eventMap = events->child, i = 0; eventMap; eventMap = eventMap->next, ++i) {
			topi40_spEventData *eventData = topi40_spEventData_create(eventMap->name);
			eventData->intValue = topi40_Json_getInt(eventMap, "int", 0);
			eventData->floatValue = topi40_Json_getFloat(eventMap, "float", 0);
			stringValue = topi40_Json_getString(eventMap, "string", 0);
			if (stringValue) MALLOC_STR(eventData->stringValue, stringValue);
			audioPath = topi40_Json_getString(eventMap, "audio", 0);
			if (audioPath) {
				MALLOC_STR(eventData->audioPath, audioPath);
				eventData->volume = topi40_Json_getFloat(eventMap, "volume", 1);
				eventData->balance = topi40_Json_getFloat(eventMap, "balance", 0);
			}
			skeletonData->events[i] = eventData;
		}
	}

	/* Animations. */
	animations = topi40_Json_getItem(root, "animations");
	if (animations) {
		Json *animationMap;
		skeletonData->animations = MALLOC(topi40_spAnimation *, animations->size);
		for (animationMap = animations->child; animationMap; animationMap = animationMap->next) {
			topi40_spAnimation *animation = _topi40_spSkeletonJson_readAnimation(self, animationMap, skeletonData);
			if (!animation) {
				topi40_spSkeletonData_dispose(skeletonData);
				return NULL;
			}
			skeletonData->animations[skeletonData->animationsCount++] = animation;
		}
	}

	topi40_Json_dispose(root);
	return skeletonData;
}
