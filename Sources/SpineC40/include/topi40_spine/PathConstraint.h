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

#ifndef SPINE_PATHCONSTRAINT_H_
#define SPINE_PATHCONSTRAINT_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/PathConstraintData.h>
#include <topi40_spine/Bone.h>
#include <topi40_spine/Slot.h>
#include "PathAttachment.h"

#ifdef __cplusplus
extern "C" {
#endif

struct topi40_spSkeleton;

typedef struct topi40_spPathConstraint {
	topi40_spPathConstraintData *const data;
	int bonesCount;
	topi40_spBone **const bones;
	topi40_spSlot *target;
	float position, topi40_spacing;
	float mixRotate, mixX, mixY;

	int topi40_spacesCount;
	float *topi40_spaces;

	int positionsCount;
	float *positions;

	int worldCount;
	float *world;

	int curvesCount;
	float *curves;

	int lengthsCount;
	float *lengths;

	float segments[10];

	int /*boolean*/ active;
} topi40_spPathConstraint;

#define SP_PATHCONSTRAINT_

SP_API topi40_spPathConstraint *topi40_spPathConstraint_create(topi40_spPathConstraintData *data, const struct topi40_spSkeleton *skeleton);

SP_API void topi40_spPathConstraint_dispose(topi40_spPathConstraint *self);

SP_API void topi40_spPathConstraint_update(topi40_spPathConstraint *self);

SP_API float *topi40_spPathConstraint_computeWorldPositions(topi40_spPathConstraint *self, topi40_spPathAttachment *path, int topi40_spacesCount,
													 int/*bool*/ tangents);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_PATHCONSTRAINT_H_ */
