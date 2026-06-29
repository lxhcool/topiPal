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

#include <topi40_spine/VertexEffect.h>
#include <topi40_spine/extension.h>

void _topi40_spJitterVertexEffect_begin(topi40_spVertexEffect *self, topi40_spSkeleton *skeleton) {
	UNUSED(self);
	UNUSED(skeleton);
}

void _topi40_spJitterVertexEffect_transform(topi40_spVertexEffect *self, float *x, float *y, float *u, float *v, topi40_spColor *light,
									 topi40_spColor *dark) {
	topi40_spJitterVertexEffect *internal = (topi40_spJitterVertexEffect *) self;
	float jitterX = internal->jitterX;
	float jitterY = internal->jitterY;
	(*x) += _topi40_spMath_randomTriangular(-jitterX, jitterY);
	(*y) += _topi40_spMath_randomTriangular(-jitterX, jitterY);
	UNUSED(u);
	UNUSED(v);
	UNUSED(light);
	UNUSED(dark);
}

void _topi40_spJitterVertexEffect_end(topi40_spVertexEffect *self) {
	UNUSED(self);
}

topi40_spJitterVertexEffect *topi40_spJitterVertexEffect_create(float jitterX, float jitterY) {
	topi40_spJitterVertexEffect *effect = CALLOC(topi40_spJitterVertexEffect, 1);
	effect->super.begin = _topi40_spJitterVertexEffect_begin;
	effect->super.transform = _topi40_spJitterVertexEffect_transform;
	effect->super.end = _topi40_spJitterVertexEffect_end;
	effect->jitterX = jitterX;
	effect->jitterY = jitterY;
	return effect;
}

void topi40_spJitterVertexEffect_dispose(topi40_spJitterVertexEffect *effect) {
	FREE(effect);
}

void _topi40_spSwirlVertexEffect_begin(topi40_spVertexEffect *self, topi40_spSkeleton *skeleton) {
	topi40_spSwirlVertexEffect *internal = (topi40_spSwirlVertexEffect *) self;
	internal->worldX = skeleton->x + internal->centerX;
	internal->worldY = skeleton->y + internal->centerY;
}

void _topi40_spSwirlVertexEffect_transform(topi40_spVertexEffect *self, float *positionX, float *positionY, float *u, float *v,
									topi40_spColor *light, topi40_spColor *dark) {
	topi40_spSwirlVertexEffect *internal = (topi40_spSwirlVertexEffect *) self;
	float radAngle = internal->angle * DEG_RAD;
	float x = *positionX - internal->worldX;
	float y = *positionY - internal->worldY;
	float dist = SQRT(x * x + y * y);
	if (dist < internal->radius) {
		float theta = _topi40_spMath_interpolate(_topi40_spMath_pow2_apply, 0, radAngle,
										  (internal->radius - dist) / internal->radius);
		float cosine = COS(theta);
		float sine = SIN(theta);
		(*positionX) = cosine * x - sine * y + internal->worldX;
		(*positionY) = sine * x + cosine * y + internal->worldY;
	}
	UNUSED(self);
	UNUSED(u);
	UNUSED(v);
	UNUSED(light);
	UNUSED(dark);
}

void _topi40_spSwirlVertexEffect_end(topi40_spVertexEffect *self) {
	UNUSED(self);
}

topi40_spSwirlVertexEffect *topi40_spSwirlVertexEffect_create(float radius) {
	topi40_spSwirlVertexEffect *effect = CALLOC(topi40_spSwirlVertexEffect, 1);
	effect->super.begin = _topi40_spSwirlVertexEffect_begin;
	effect->super.transform = _topi40_spSwirlVertexEffect_transform;
	effect->super.end = _topi40_spSwirlVertexEffect_end;
	effect->radius = radius;
	return effect;
}

void topi40_spSwirlVertexEffect_dispose(topi40_spSwirlVertexEffect *effect) {
	FREE(effect);
}
