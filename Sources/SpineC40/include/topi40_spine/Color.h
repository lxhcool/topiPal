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

#ifndef SPINE_COLOR_H_
#define SPINE_COLOR_H_

#include <topi40_spine/dll.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topi40_spColor {
	float r, g, b, a;
} topi40_spColor;

/* @param attachmentName May be 0 for no setup pose attachment. */
SP_API topi40_spColor *topi40_spColor_create();

SP_API void topi40_spColor_dispose(topi40_spColor *self);

SP_API void topi40_spColor_setFromFloats(topi40_spColor *color, float r, float g, float b, float a);

SP_API void topi40_spColor_setFromFloats3(topi40_spColor *self, float r, float g, float b);

SP_API void topi40_spColor_setFromColor(topi40_spColor *color, topi40_spColor *otherColor);

SP_API void topi40_spColor_setFromColor3(topi40_spColor *self, topi40_spColor *otherColor);

SP_API void topi40_spColor_addFloats(topi40_spColor *color, float r, float g, float b, float a);

SP_API void topi40_spColor_addFloats3(topi40_spColor *color, float r, float g, float b);

SP_API void topi40_spColor_addColor(topi40_spColor *color, topi40_spColor *otherColor);

SP_API void topi40_spColor_clamp(topi40_spColor *color);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_COLOR_H_ */
