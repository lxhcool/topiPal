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

#ifndef SPINE_VERTEXEFFECT_H_
#define SPINE_VERTEXEFFECT_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Skeleton.h>
#include <topi40_spine/Color.h>

#ifdef __cplusplus
extern "C" {
#endif

struct topi40_spVertexEffect;

typedef void (*topi40_spVertexEffectBegin)(struct topi40_spVertexEffect *self, topi40_spSkeleton *skeleton);

typedef void (*topi40_spVertexEffectTransform)(struct topi40_spVertexEffect *self, float *x, float *y, float *u, float *v,
										topi40_spColor *light, topi40_spColor *dark);

typedef void (*topi40_spVertexEffectEnd)(struct topi40_spVertexEffect *self);

typedef struct topi40_spVertexEffect {
	topi40_spVertexEffectBegin begin;
	topi40_spVertexEffectTransform transform;
	topi40_spVertexEffectEnd end;
} topi40_spVertexEffect;

typedef struct topi40_spJitterVertexEffect {
	topi40_spVertexEffect super;
	float jitterX;
	float jitterY;
} topi40_spJitterVertexEffect;

typedef struct topi40_spSwirlVertexEffect {
	topi40_spVertexEffect super;
	float centerX;
	float centerY;
	float radius;
	float angle;
	float worldX;
	float worldY;
} topi40_spSwirlVertexEffect;

SP_API topi40_spJitterVertexEffect *topi40_spJitterVertexEffect_create(float jitterX, float jitterY);

SP_API void topi40_spJitterVertexEffect_dispose(topi40_spJitterVertexEffect *effect);

SP_API topi40_spSwirlVertexEffect *topi40_spSwirlVertexEffect_create(float radius);

SP_API void topi40_spSwirlVertexEffect_dispose(topi40_spSwirlVertexEffect *effect);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_VERTEX_EFFECT_H_ */
