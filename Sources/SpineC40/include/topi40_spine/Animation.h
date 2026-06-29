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

#ifndef SPINE_ANIMATION_H_
#define SPINE_ANIMATION_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Event.h>
#include <topi40_spine/Attachment.h>
#include <topi40_spine/VertexAttachment.h>
#include <topi40_spine/Array.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spTimeline topi40_spTimeline;
struct topi40_spSkeleton;
typedef uint64_t topi40_spPropertyId;

_SP_ARRAY_DECLARE_TYPE(topi40_spPropertyIdArray, topi40_spPropertyId)

_SP_ARRAY_DECLARE_TYPE(topi40_spTimelineArray, topi40_spTimeline*)

typedef struct topi40_spAnimation {
	const char *const name;
	float duration;

	topi40_spTimelineArray *timelines;
	topi40_spPropertyIdArray *timelineIds;
} topi40_spAnimation;

typedef enum {
	SP_MIX_BLEND_SETUP,
	SP_MIX_BLEND_FIRST,
	SP_MIX_BLEND_REPLACE,
	SP_MIX_BLEND_ADD
} topi40_spMixBlend;

typedef enum {
	SP_MIX_DIRECTION_IN,
	SP_MIX_DIRECTION_OUT
} topi40_spMixDirection;

SP_API topi40_spAnimation *topi40_spAnimation_create(const char *name, topi40_spTimelineArray *timelines, float duration);

SP_API void topi40_spAnimation_dispose(topi40_spAnimation *self);

SP_API int /*bool*/ topi40_spAnimation_hasTimeline(topi40_spAnimation *self, topi40_spPropertyId *ids, int idsCount);

/** Poses the skeleton at the topi40_specified time for this animation.
 * @param lastTime The last time the animation was applied.
 * @param events Any triggered events are added. May be null.*/
SP_API void
topi40_spAnimation_apply(const topi40_spAnimation *self, struct topi40_spSkeleton *skeleton, float lastTime, float time, int loop,
				  topi40_spEvent **events, int *eventsCount, float alpha, topi40_spMixBlend blend, topi40_spMixDirection direction);

/**/
typedef enum {
	SP_TIMELINE_ATTACHMENT,
	SP_TIMELINE_ALPHA,
	SP_TIMELINE_PATHCONSTRAINTPOSITION,
	SP_TIMELINE_PATHCONSTRAINTSPACING,
	SP_TIMELINE_ROTATE,
	SP_TIMELINE_SCALEX,
	SP_TIMELINE_SCALEY,
	SP_TIMELINE_SHEARX,
	SP_TIMELINE_SHEARY,
	SP_TIMELINE_TRANSLATEX,
	SP_TIMELINE_TRANSLATEY,
	SP_TIMELINE_SCALE,
	SP_TIMELINE_SHEAR,
	SP_TIMELINE_TRANSLATE,
	SP_TIMELINE_DEFORM,
	SP_TIMELINE_IKCONSTRAINT,
	SP_TIMELINE_PATHCONSTRAINTMIX,
	SP_TIMELINE_RGB2,
	SP_TIMELINE_RGBA2,
	SP_TIMELINE_RGBA,
	SP_TIMELINE_RGB,
	SP_TIMELINE_TRANSFORMCONSTRAINT,
	SP_TIMELINE_DRAWORDER,
	SP_TIMELINE_EVENT
} topi40_spTimelineType;

/**/

typedef enum {
	SP_PROPERTY_ROTATE = 1 << 0,
	SP_PROPERTY_X = 1 << 1,
	SP_PROPERTY_Y = 1 << 2,
	SP_PROPERTY_SCALEX = 1 << 3,
	SP_PROPERTY_SCALEY = 1 << 4,
	SP_PROPERTY_SHEARX = 1 << 5,
	SP_PROPERTY_SHEARY = 1 << 6,
	SP_PROPERTY_RGB = 1 << 7,
	SP_PROPERTY_ALPHA = 1 << 8,
	SP_PROPERTY_RGB2 = 1 << 9,
	SP_PROPERTY_ATTACHMENT = 1 << 10,
	SP_PROPERTY_DEFORM = 1 << 11,
	SP_PROPERTY_EVENT = 1 << 12,
	SP_PROPERTY_DRAWORDER = 1 << 13,
	SP_PROPERTY_IKCONSTRAINT = 1 << 14,
	SP_PROPERTY_TRANSFORMCONSTRAINT = 1 << 15,
	SP_PROPERTY_PATHCONSTRAINT_POSITION = 1 << 16,
	SP_PROPERTY_PATHCONSTRAINT_SPACING = 1 << 17,
	SP_PROPERTY_PATHCONSTRAINT_MIX = 1 << 18
} topi40_spProperty;

#define SP_MAX_PROPERTY_IDS 3

typedef struct _topi40_spTimelineVtable {
	void (*apply)(topi40_spTimeline *self, struct topi40_spSkeleton *skeleton, float lastTime, float time, topi40_spEvent **firedEvents,
				  int *eventsCount, float alpha, topi40_spMixBlend blend, topi40_spMixDirection direction);

	void (*dispose)(topi40_spTimeline *self);

	void
	(*setBezier)(topi40_spTimeline *self, int bezier, int frame, float value, float time1, float value1, float cx1, float cy1,
				 float cx2, float cy2, float time2, float value2);
} _topi40_spTimelineVtable;

struct topi40_spTimeline {
	_topi40_spTimelineVtable vtable;
	topi40_spPropertyId propertyIds[SP_MAX_PROPERTY_IDS];
	int propertyIdsCount;
	topi40_spFloatArray *frames;
	int frameCount;
	int frameEntries;
	topi40_spTimelineType type;
};

SP_API void topi40_spTimeline_dispose(topi40_spTimeline *self);

SP_API void
topi40_spTimeline_apply(topi40_spTimeline *self, struct topi40_spSkeleton *skeleton, float lastTime, float time, topi40_spEvent **firedEvents,
				 int *eventsCount, float alpha, topi40_spMixBlend blend, topi40_spMixDirection direction);

SP_API void
topi40_spTimeline_setBezier(topi40_spTimeline *self, int bezier, int frame, float value, float time1, float value1, float cx1,
					 float cy1, float cx2, float cy2, float time2, float value2);

SP_API float topi40_spTimeline_getDuration(const topi40_spTimeline *self);

/**/

typedef struct topi40_spCurveTimeline {
	topi40_spTimeline super;
	topi40_spFloatArray *curves; /* type, x, y, ... */
} topi40_spCurveTimeline;

SP_API void topi40_spCurveTimeline_setLinear(topi40_spCurveTimeline *self, int frameIndex);

SP_API void topi40_spCurveTimeline_setStepped(topi40_spCurveTimeline *self, int frameIndex);

/* Sets the control handle positions for an interpolation bezier curve used to transition from this keyframe to the next.
 * cx1 and cx2 are from 0 to 1, representing the percent of time between the two keyframes. cy1 and cy2 are the percent of
 * the difference between the keyframe's values. */
SP_API void topi40_spCurveTimeline_setCurve(topi40_spCurveTimeline *self, int frameIndex, float cx1, float cy1, float cx2, float cy2);

SP_API float topi40_spCurveTimeline_getCurvePercent(const topi40_spCurveTimeline *self, int frameIndex, float percent);

typedef struct topi40_spCurveTimeline topi40_spCurveTimeline1;

SP_API void topi40_spCurveTimeline1_setFrame(topi40_spCurveTimeline1 *self, int frame, float time, float value);

SP_API float topi40_spCurveTimeline1_getCurveValue(topi40_spCurveTimeline1 *self, float time);

typedef struct topi40_spCurveTimeline topi40_spCurveTimeline2;

SP_API void topi40_spCurveTimeline2_setFrame(topi40_spCurveTimeline1 *self, int frame, float time, float value1, float value2);

/**/

typedef struct topi40_spRotateTimeline {
	topi40_spCurveTimeline1 super;
	int boneIndex;
} topi40_spRotateTimeline;

SP_API topi40_spRotateTimeline *topi40_spRotateTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spRotateTimeline_setFrame(topi40_spRotateTimeline *self, int frameIndex, float time, float angle);

/**/

typedef struct topi40_spTranslateTimeline {
	topi40_spCurveTimeline2 super;
	int boneIndex;
} topi40_spTranslateTimeline;

SP_API topi40_spTranslateTimeline *topi40_spTranslateTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spTranslateTimeline_setFrame(topi40_spTranslateTimeline *self, int frameIndex, float time, float x, float y);

/**/

typedef struct topi40_spTranslateXTimeline {
	topi40_spCurveTimeline1 super;
	int boneIndex;
} topi40_spTranslateXTimeline;

SP_API topi40_spTranslateXTimeline *topi40_spTranslateXTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spTranslateXTimeline_setFrame(topi40_spTranslateXTimeline *self, int frame, float time, float x);

/**/

typedef struct topi40_spTranslateYTimeline {
	topi40_spCurveTimeline1 super;
	int boneIndex;
} topi40_spTranslateYTimeline;

SP_API topi40_spTranslateYTimeline *topi40_spTranslateYTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spTranslateYTimeline_setFrame(topi40_spTranslateYTimeline *self, int frame, float time, float y);

/**/

typedef struct topi40_spScaleTimeline {
	topi40_spCurveTimeline2 super;
	int boneIndex;
} topi40_spScaleTimeline;

SP_API topi40_spScaleTimeline *topi40_spScaleTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spScaleTimeline_setFrame(topi40_spScaleTimeline *self, int frameIndex, float time, float x, float y);

/**/

typedef struct topi40_spScaleXTimeline {
	topi40_spCurveTimeline1 super;
	int boneIndex;
} topi40_spScaleXTimeline;

SP_API topi40_spScaleXTimeline *topi40_spScaleXTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spScaleXTimeline_setFrame(topi40_spScaleXTimeline *self, int frame, float time, float x);

/**/

typedef struct topi40_spScaleYTimeline {
	topi40_spCurveTimeline1 super;
	int boneIndex;
} topi40_spScaleYTimeline;

SP_API topi40_spScaleYTimeline *topi40_spScaleYTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spScaleYTimeline_setFrame(topi40_spScaleYTimeline *self, int frame, float time, float y);

/**/

typedef struct topi40_spShearTimeline {
	topi40_spCurveTimeline2 super;
	int boneIndex;
} topi40_spShearTimeline;

SP_API topi40_spShearTimeline *topi40_spShearTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spShearTimeline_setFrame(topi40_spShearTimeline *self, int frameIndex, float time, float x, float y);

/**/

typedef struct topi40_spShearXTimeline {
	topi40_spCurveTimeline1 super;
	int boneIndex;
} topi40_spShearXTimeline;

SP_API topi40_spShearXTimeline *topi40_spShearXTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spShearXTimeline_setFrame(topi40_spShearXTimeline *self, int frame, float time, float x);

/**/

typedef struct topi40_spShearYTimeline {
	topi40_spCurveTimeline1 super;
	int boneIndex;
} topi40_spShearYTimeline;

SP_API topi40_spShearYTimeline *topi40_spShearYTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP_API void topi40_spShearYTimeline_setFrame(topi40_spShearYTimeline *self, int frame, float time, float x);

/**/

typedef struct topi40_spRGBATimeline {
	topi40_spCurveTimeline2 super;
	int slotIndex;
} topi40_spRGBATimeline;

SP_API topi40_spRGBATimeline *topi40_spRGBATimeline_create(int framesCount, int bezierCount, int slotIndex);

SP_API void
topi40_spRGBATimeline_setFrame(topi40_spRGBATimeline *self, int frameIndex, float time, float r, float g, float b, float a);

/**/

typedef struct topi40_spRGBTimeline {
	topi40_spCurveTimeline2 super;
	int slotIndex;
} topi40_spRGBTimeline;

SP_API topi40_spRGBTimeline *topi40_spRGBTimeline_create(int framesCount, int bezierCount, int slotIndex);

SP_API void topi40_spRGBTimeline_setFrame(topi40_spRGBTimeline *self, int frameIndex, float time, float r, float g, float b);

/**/

typedef struct topi40_spAlphaTimeline {
	topi40_spCurveTimeline1 super;
	int slotIndex;
} topi40_spAlphaTimeline;

SP_API topi40_spAlphaTimeline *topi40_spAlphaTimeline_create(int frameCount, int bezierCount, int slotIndex);

SP_API void topi40_spAlphaTimeline_setFrame(topi40_spAlphaTimeline *self, int frame, float time, float x);

/**/

typedef struct topi40_spRGBA2Timeline {
	topi40_spCurveTimeline super;
	int slotIndex;
} topi40_spRGBA2Timeline;

SP_API topi40_spRGBA2Timeline *topi40_spRGBA2Timeline_create(int framesCount, int bezierCount, int slotIndex);

SP_API void
topi40_spRGBA2Timeline_setFrame(topi40_spRGBA2Timeline *self, int frameIndex, float time, float r, float g, float b, float a,
						 float r2, float g2, float b2);

/**/

typedef struct topi40_spRGB2Timeline {
	topi40_spCurveTimeline super;
	int slotIndex;
} topi40_spRGB2Timeline;

SP_API topi40_spRGB2Timeline *topi40_spRGB2Timeline_create(int framesCount, int bezierCount, int slotIndex);

SP_API void
topi40_spRGB2Timeline_setFrame(topi40_spRGB2Timeline *self, int frameIndex, float time, float r, float g, float b, float r2, float g2,
						float b2);

/**/

typedef struct topi40_spAttachmentTimeline {
	topi40_spTimeline super;
	int slotIndex;
	const char **const attachmentNames;
} topi40_spAttachmentTimeline;

SP_API topi40_spAttachmentTimeline *topi40_spAttachmentTimeline_create(int framesCount, int SlotIndex);

/* @param attachmentName May be 0. */
SP_API void
topi40_spAttachmentTimeline_setFrame(topi40_spAttachmentTimeline *self, int frameIndex, float time, const char *attachmentName);

/**/

typedef struct topi40_spDeformTimeline {
	topi40_spCurveTimeline super;
	int const frameVerticesCount;
	const float **const frameVertices;
	int slotIndex;
	topi40_spAttachment *attachment;
} topi40_spDeformTimeline;

SP_API topi40_spDeformTimeline *
topi40_spDeformTimeline_create(int framesCount, int frameVerticesCount, int bezierCount, int slotIndex,
						topi40_spVertexAttachment *attachment);

SP_API void topi40_spDeformTimeline_setFrame(topi40_spDeformTimeline *self, int frameIndex, float time, float *vertices);

/**/

typedef struct topi40_spEventTimeline {
	topi40_spTimeline super;
	topi40_spEvent **const events;
} topi40_spEventTimeline;

SP_API topi40_spEventTimeline *topi40_spEventTimeline_create(int framesCount);

SP_API void topi40_spEventTimeline_setFrame(topi40_spEventTimeline *self, int frameIndex, topi40_spEvent *event);

/**/

typedef struct topi40_spDrawOrderTimeline {
	topi40_spTimeline super;
	const int **const drawOrders;
	int const slotsCount;
} topi40_spDrawOrderTimeline;

SP_API topi40_spDrawOrderTimeline *topi40_spDrawOrderTimeline_create(int framesCount, int slotsCount);

SP_API void topi40_spDrawOrderTimeline_setFrame(topi40_spDrawOrderTimeline *self, int frameIndex, float time, const int *drawOrder);

/**/

typedef struct topi40_spIkConstraintTimeline {
	topi40_spCurveTimeline super;
	int ikConstraintIndex;
} topi40_spIkConstraintTimeline;

SP_API topi40_spIkConstraintTimeline *
topi40_spIkConstraintTimeline_create(int framesCount, int bezierCount, int transformConstraintIndex);

SP_API void
topi40_spIkConstraintTimeline_setFrame(topi40_spIkConstraintTimeline *self, int frameIndex, float time, float mix, float softness,
								int bendDirection, int /*boolean*/ compress, int /**boolean**/ stretch);

/**/

typedef struct topi40_spTransformConstraintTimeline {
	topi40_spCurveTimeline super;
	int transformConstraintIndex;
} topi40_spTransformConstraintTimeline;

SP_API topi40_spTransformConstraintTimeline *
topi40_spTransformConstraintTimeline_create(int framesCount, int bezierCount, int transformConstraintIndex);

SP_API void
topi40_spTransformConstraintTimeline_setFrame(topi40_spTransformConstraintTimeline *self, int frameIndex, float time, float mixRotate,
									   float mixX, float mixY, float mixScaleX, float mixScaleY, float mixShearY);

/**/

typedef struct topi40_spPathConstraintPositionTimeline {
	topi40_spCurveTimeline super;
	int pathConstraintIndex;
} topi40_spPathConstraintPositionTimeline;

SP_API topi40_spPathConstraintPositionTimeline *
topi40_spPathConstraintPositionTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex);

SP_API void
topi40_spPathConstraintPositionTimeline_setFrame(topi40_spPathConstraintPositionTimeline *self, int frameIndex, float time,
										  float value);

/**/

typedef struct topi40_spPathConstraintSpacingTimeline {
	topi40_spCurveTimeline super;
	int pathConstraintIndex;
} topi40_spPathConstraintSpacingTimeline;

SP_API topi40_spPathConstraintSpacingTimeline *
topi40_spPathConstraintSpacingTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex);

SP_API void topi40_spPathConstraintSpacingTimeline_setFrame(topi40_spPathConstraintSpacingTimeline *self, int frameIndex, float time,
													 float value);

/**/

typedef struct topi40_spPathConstraintMixTimeline {
	topi40_spCurveTimeline super;
	int pathConstraintIndex;
} topi40_spPathConstraintMixTimeline;

SP_API topi40_spPathConstraintMixTimeline *
topi40_spPathConstraintMixTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex);

SP_API void
topi40_spPathConstraintMixTimeline_setFrame(topi40_spPathConstraintMixTimeline *self, int frameIndex, float time, float mixRotate,
									 float mixX, float mixY);

/**/

#ifdef __cplusplus
}
#endif

#endif /* SPINE_ANIMATION_H_ */
