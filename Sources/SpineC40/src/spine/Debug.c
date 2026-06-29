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
#include <topi40_spine/Debug.h>

#include <stdio.h>

static const char *_topi40_spTimelineTypeNames[] = {
		"Attachment",
		"Alpha",
		"PathConstraintPosition",
		"PathConstraintSpace",
		"Rotate",
		"ScaleX",
		"ScaleY",
		"ShearX",
		"ShearY",
		"TranslateX",
		"TranslateY",
		"Scale",
		"Shear",
		"Translate",
		"Deform",
		"IkConstraint",
		"PathConstraintMix",
		"Rgb2",
		"Rgba2",
		"Rgba",
		"Rgb",
		"TransformConstraint",
		"DrawOrder",
		"Event"};

void topi40_spDebug_printSkeletonData(topi40_spSkeletonData *skeletonData) {
	int i, n;
	topi40_spDebug_printBoneDatas(skeletonData->bones, skeletonData->bonesCount);

	for (i = 0, n = skeletonData->animationsCount; i < n; i++) {
		topi40_spDebug_printAnimation(skeletonData->animations[i]);
	}
}

void _topi40_spDebug_printTimelineBase(topi40_spTimeline *timeline) {
	printf("   Timeline %s:\n", _topi40_spTimelineTypeNames[timeline->type]);
	printf("      frame count: %i\n", timeline->frameCount);
	printf("      frame entries: %i\n", timeline->frameEntries);
	printf("      frames: ");
	topi40_spDebug_printFloats(timeline->frames->items, timeline->frames->size);
	printf("\n");
}

void _topi40_spDebug_printCurveTimeline(topi40_spCurveTimeline *timeline) {
	_topi40_spDebug_printTimelineBase(&timeline->super);
	printf("      curves: ");
	topi40_spDebug_printFloats(timeline->curves->items, timeline->curves->size);
	printf("\n");
}

void topi40_spDebug_printTimeline(topi40_spTimeline *timeline) {
	switch (timeline->type) {
		case SP_TIMELINE_ATTACHMENT: {
			topi40_spAttachmentTimeline *t = (topi40_spAttachmentTimeline *) timeline;
			_topi40_spDebug_printTimelineBase(&t->super);
			break;
		}
		case SP_TIMELINE_ALPHA: {
			topi40_spAlphaTimeline *t = (topi40_spAlphaTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_PATHCONSTRAINTPOSITION: {
			topi40_spPathConstraintPositionTimeline *t = (topi40_spPathConstraintPositionTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_PATHCONSTRAINTSPACING: {
			topi40_spPathConstraintMixTimeline *t = (topi40_spPathConstraintMixTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_ROTATE: {
			topi40_spRotateTimeline *t = (topi40_spRotateTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_SCALEX: {
			topi40_spScaleXTimeline *t = (topi40_spScaleXTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_SCALEY: {
			topi40_spScaleYTimeline *t = (topi40_spScaleYTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_SHEARX: {
			topi40_spShearXTimeline *t = (topi40_spShearXTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_SHEARY: {
			topi40_spShearYTimeline *t = (topi40_spShearYTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_TRANSLATEX: {
			topi40_spTranslateXTimeline *t = (topi40_spTranslateXTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_TRANSLATEY: {
			topi40_spTranslateYTimeline *t = (topi40_spTranslateYTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_SCALE: {
			topi40_spScaleTimeline *t = (topi40_spScaleTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_SHEAR: {
			topi40_spShearTimeline *t = (topi40_spShearTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_TRANSLATE: {
			topi40_spTranslateTimeline *t = (topi40_spTranslateTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_DEFORM: {
			topi40_spDeformTimeline *t = (topi40_spDeformTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_IKCONSTRAINT: {
			topi40_spIkConstraintTimeline *t = (topi40_spIkConstraintTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_PATHCONSTRAINTMIX: {
			topi40_spPathConstraintMixTimeline *t = (topi40_spPathConstraintMixTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_RGB2: {
			topi40_spRGB2Timeline *t = (topi40_spRGB2Timeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_RGBA2: {
			topi40_spRGBA2Timeline *t = (topi40_spRGBA2Timeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_RGBA: {
			topi40_spRGBATimeline *t = (topi40_spRGBATimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_RGB: {
			topi40_spRGBTimeline *t = (topi40_spRGBTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_TRANSFORMCONSTRAINT: {
			topi40_spTransformConstraintTimeline *t = (topi40_spTransformConstraintTimeline *) timeline;
			_topi40_spDebug_printCurveTimeline(&t->super);
			break;
		}
		case SP_TIMELINE_DRAWORDER: {
			topi40_spDrawOrderTimeline *t = (topi40_spDrawOrderTimeline *) timeline;
			_topi40_spDebug_printTimelineBase(&t->super);
			break;
		}
		case SP_TIMELINE_EVENT: {
			topi40_spEventTimeline *t = (topi40_spEventTimeline *) timeline;
			_topi40_spDebug_printTimelineBase(&t->super);
			break;
		}
	}
}

void topi40_spDebug_printAnimation(topi40_spAnimation *animation) {
	int i, n;
	printf("Animation %s: %i timelines\n", animation->name, animation->timelines->size);

	for (i = 0, n = animation->timelines->size; i < n; i++) {
		topi40_spDebug_printTimeline(animation->timelines->items[i]);
	}
}

void topi40_spDebug_printBoneDatas(topi40_spBoneData **boneDatas, int numBoneDatas) {
	int i;
	for (i = 0; i < numBoneDatas; i++) {
		topi40_spDebug_printBoneData(boneDatas[i]);
	}
}

void topi40_spDebug_printBoneData(topi40_spBoneData *boneData) {
	printf("Bone data %s: %f, %f, %f, %f, %f, %f %f\n", boneData->name, boneData->rotation, boneData->scaleX,
		   boneData->scaleY, boneData->x, boneData->y, boneData->shearX, boneData->shearY);
}

void topi40_spDebug_printSkeleton(topi40_spSkeleton *skeleton) {
	topi40_spDebug_printBones(skeleton->bones, skeleton->bonesCount);
}

void topi40_spDebug_printBones(topi40_spBone **bones, int numBones) {
	int i;
	for (i = 0; i < numBones; i++) {
		topi40_spDebug_printBone(bones[i]);
	}
}

void topi40_spDebug_printBone(topi40_spBone *bone) {
	printf("Bone %s: %f, %f, %f, %f, %f, %f\n", bone->data->name, bone->a, bone->b, bone->c, bone->d, bone->worldX,
		   bone->worldY);
}

void topi40_spDebug_printFloats(float *values, int numFloats) {
	int i;
	printf("(%i) [", numFloats);
	for (i = 0; i < numFloats; i++) {
		printf("%f, ", values[i]);
	}
	printf("]");
}
