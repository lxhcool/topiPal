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

#ifndef SPINE_ANIMATIONSTATE_H_
#define SPINE_ANIMATIONSTATE_H_

#include <topi40_spine/dll.h>
#include <topi40_spine/Animation.h>
#include <topi40_spine/AnimationStateData.h>
#include <topi40_spine/Event.h>
#include <topi40_spine/Array.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SP_ANIMATION_START,
	SP_ANIMATION_INTERRUPT,
	SP_ANIMATION_END,
	SP_ANIMATION_COMPLETE,
	SP_ANIMATION_DISPOSE,
	SP_ANIMATION_EVENT
} topi40_spEventType;

typedef struct topi40_spAnimationState topi40_spAnimationState;
typedef struct topi40_spTrackEntry topi40_spTrackEntry;

typedef void (*topi40_spAnimationStateListener)(topi40_spAnimationState *state, topi40_spEventType type, topi40_spTrackEntry *entry,
										 topi40_spEvent *event);

_SP_ARRAY_DECLARE_TYPE(topi40_spTrackEntryArray, topi40_spTrackEntry*)

struct topi40_spTrackEntry {
	topi40_spAnimation *animation;
	topi40_spTrackEntry *previous;
	topi40_spTrackEntry *next;
	topi40_spTrackEntry *mixingFrom;
	topi40_spTrackEntry *mixingTo;
	topi40_spAnimationStateListener listener;
	int trackIndex;
	int /*boolean*/ loop;
	int /*boolean*/ holdPrevious;
	int /*boolean*/ reverse;
	float eventThreshold, attachmentThreshold, drawOrderThreshold;
	float animationStart, animationEnd, animationLast, nextAnimationLast;
	float delay, trackTime, trackLast, nextTrackLast, trackEnd, timeScale;
	float alpha, mixTime, mixDuration, interruptAlpha, totalAlpha;
	topi40_spMixBlend mixBlend;
	topi40_spIntArray *timelineMode;
	topi40_spTrackEntryArray *timelineHoldMix;
	float *timelinesRotation;
	int timelinesRotationCount;
	void *rendererObject;
	void *userData;
};

struct topi40_spAnimationState {
	topi40_spAnimationStateData *const data;

	int tracksCount;
	topi40_spTrackEntry **tracks;

	topi40_spAnimationStateListener listener;

	float timeScale;

	void *rendererObject;
	void *userData;

	int unkeyedState;
};

/* @param data May be 0 for no mixing. */
SP_API topi40_spAnimationState *topi40_spAnimationState_create(topi40_spAnimationStateData *data);

SP_API void topi40_spAnimationState_dispose(topi40_spAnimationState *self);

SP_API void topi40_spAnimationState_update(topi40_spAnimationState *self, float delta);

SP_API int /**bool**/ topi40_spAnimationState_apply(topi40_spAnimationState *self, struct topi40_spSkeleton *skeleton);

SP_API void topi40_spAnimationState_clearTracks(topi40_spAnimationState *self);

SP_API void topi40_spAnimationState_clearTrack(topi40_spAnimationState *self, int trackIndex);

/** Set the current animation. Any queued animations are cleared. */
SP_API topi40_spTrackEntry *
topi40_spAnimationState_setAnimationByName(topi40_spAnimationState *self, int trackIndex, const char *animationName,
									int/*bool*/loop);

SP_API topi40_spTrackEntry *
topi40_spAnimationState_setAnimation(topi40_spAnimationState *self, int trackIndex, topi40_spAnimation *animation, int/*bool*/loop);

/** Adds an animation to be played delay seconds after the current or last queued animation, taking into account any mix
 * duration. */
SP_API topi40_spTrackEntry *
topi40_spAnimationState_addAnimationByName(topi40_spAnimationState *self, int trackIndex, const char *animationName,
									int/*bool*/loop, float delay);

SP_API topi40_spTrackEntry *
topi40_spAnimationState_addAnimation(topi40_spAnimationState *self, int trackIndex, topi40_spAnimation *animation, int/*bool*/loop,
							  float delay);

SP_API topi40_spTrackEntry *topi40_spAnimationState_setEmptyAnimation(topi40_spAnimationState *self, int trackIndex, float mixDuration);

SP_API topi40_spTrackEntry *
topi40_spAnimationState_addEmptyAnimation(topi40_spAnimationState *self, int trackIndex, float mixDuration, float delay);

SP_API void topi40_spAnimationState_setEmptyAnimations(topi40_spAnimationState *self, float mixDuration);

SP_API topi40_spTrackEntry *topi40_spAnimationState_getCurrent(topi40_spAnimationState *self, int trackIndex);

SP_API void topi40_spAnimationState_clearListenerNotifications(topi40_spAnimationState *self);

SP_API float topi40_spTrackEntry_getAnimationTime(topi40_spTrackEntry *entry);

SP_API float topi40_spTrackEntry_getTrackComplete(topi40_spTrackEntry *entry);

SP_API void topi40_spAnimationState_clearNext(topi40_spAnimationState *self, topi40_spTrackEntry *entry);

/** Use this to dispose static memory before your app exits to appease your memory leak detector*/
SP_API void topi40_spAnimationState_disposeStatics();

#ifdef __cplusplus
}
#endif

#endif /* SPINE_ANIMATIONSTATE_H_ */
