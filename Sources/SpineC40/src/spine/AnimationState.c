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

#include <limits.h>
#include <topi40_spine/AnimationState.h>
#include <topi40_spine/extension.h>

#define SUBSEQUENT 0
#define FIRST 1
#define HOLD_SUBSEQUENT 2
#define HOLD_FIRST 3
#define HOLD_MIX 4

#define SETUP 1
#define CURRENT 2

_SP_ARRAY_IMPLEMENT_TYPE(topi40_spTrackEntryArray, topi40_spTrackEntry *)

static topi40_spAnimation *SP_EMPTY_ANIMATION = 0;

void topi40_spAnimationState_disposeStatics() {
	if (SP_EMPTY_ANIMATION) topi40_spAnimation_dispose(SP_EMPTY_ANIMATION);
	SP_EMPTY_ANIMATION = 0;
}

/* Forward declaration of some "private" functions so we can keep
 the same function order in C as we have method order in Java. */
void _topi40_spAnimationState_disposeTrackEntry(topi40_spTrackEntry *entry);

void _topi40_spAnimationState_disposeTrackEntries(topi40_spAnimationState *state, topi40_spTrackEntry *entry);

int /*boolean*/ _topi40_spAnimationState_updateMixingFrom(topi40_spAnimationState *self, topi40_spTrackEntry *entry, float delta);

float _topi40_spAnimationState_applyMixingFrom(topi40_spAnimationState *self, topi40_spTrackEntry *entry, topi40_spSkeleton *skeleton,
										topi40_spMixBlend currentBlend);

void _topi40_spAnimationState_applyRotateTimeline(topi40_spAnimationState *self, topi40_spTimeline *timeline, topi40_spSkeleton *skeleton, float time,
										   float alpha, topi40_spMixBlend blend, float *timelinesRotation, int i,
										   int /*boolean*/ firstFrame);

void _topi40_spAnimationState_applyAttachmentTimeline(topi40_spAnimationState *self, topi40_spTimeline *timeline, topi40_spSkeleton *skeleton,
											   float animationTime, topi40_spMixBlend blend, int /*bool*/ firstFrame);

void _topi40_spAnimationState_queueEvents(topi40_spAnimationState *self, topi40_spTrackEntry *entry, float animationTime);

void _topi40_spAnimationState_setCurrent(topi40_spAnimationState *self, int index, topi40_spTrackEntry *current, int /*boolean*/ interrupt);

topi40_spTrackEntry *_topi40_spAnimationState_expandToIndex(topi40_spAnimationState *self, int index);

topi40_spTrackEntry *
_topi40_spAnimationState_trackEntry(topi40_spAnimationState *self, int trackIndex, topi40_spAnimation *animation, int /*boolean*/ loop,
							 topi40_spTrackEntry *last);

void _topi40_spAnimationState_animationsChanged(topi40_spAnimationState *self);

float *_topi40_spAnimationState_resizeTimelinesRotation(topi40_spTrackEntry *entry, int newSize);

void _topi40_spAnimationState_ensureCapacityPropertyIDs(topi40_spAnimationState *self, int capacity);

int _topi40_spAnimationState_addPropertyID(topi40_spAnimationState *self, topi40_spPropertyId id);

void _topi40_spTrackEntry_computeHold(topi40_spTrackEntry *self, topi40_spAnimationState *state);

_topi40_spEventQueue *_topi40_spEventQueue_create(_topi40_spAnimationState *state) {
	_topi40_spEventQueue *self = CALLOC(_topi40_spEventQueue, 1);
	self->state = state;
	self->objectsCount = 0;
	self->objectsCapacity = 16;
	self->objects = CALLOC(_topi40_spEventQueueItem, self->objectsCapacity);
	self->drainDisabled = 0;
	return self;
}

void _topi40_spEventQueue_free(_topi40_spEventQueue *self) {
	FREE(self->objects);
	FREE(self);
}

void _topi40_spEventQueue_ensureCapacity(_topi40_spEventQueue *self, int newElements) {
	if (self->objectsCount + newElements > self->objectsCapacity) {
		_topi40_spEventQueueItem *newObjects;
		self->objectsCapacity <<= 1;
		newObjects = CALLOC(_topi40_spEventQueueItem, self->objectsCapacity);
		memcpy(newObjects, self->objects, sizeof(_topi40_spEventQueueItem) * self->objectsCount);
		FREE(self->objects);
		self->objects = newObjects;
	}
}

void _topi40_spEventQueue_addType(_topi40_spEventQueue *self, topi40_spEventType type) {
	_topi40_spEventQueue_ensureCapacity(self, 1);
	self->objects[self->objectsCount++].type = type;
}

void _topi40_spEventQueue_addEntry(_topi40_spEventQueue *self, topi40_spTrackEntry *entry) {
	_topi40_spEventQueue_ensureCapacity(self, 1);
	self->objects[self->objectsCount++].entry = entry;
}

void _topi40_spEventQueue_addEvent(_topi40_spEventQueue *self, topi40_spEvent *event) {
	_topi40_spEventQueue_ensureCapacity(self, 1);
	self->objects[self->objectsCount++].event = event;
}

void _topi40_spEventQueue_start(_topi40_spEventQueue *self, topi40_spTrackEntry *entry) {
	_topi40_spEventQueue_addType(self, SP_ANIMATION_START);
	_topi40_spEventQueue_addEntry(self, entry);
	self->state->animationsChanged = 1;
}

void _topi40_spEventQueue_interrupt(_topi40_spEventQueue *self, topi40_spTrackEntry *entry) {
	_topi40_spEventQueue_addType(self, SP_ANIMATION_INTERRUPT);
	_topi40_spEventQueue_addEntry(self, entry);
}

void _topi40_spEventQueue_end(_topi40_spEventQueue *self, topi40_spTrackEntry *entry) {
	_topi40_spEventQueue_addType(self, SP_ANIMATION_END);
	_topi40_spEventQueue_addEntry(self, entry);
	self->state->animationsChanged = 1;
}

void _topi40_spEventQueue_dispose(_topi40_spEventQueue *self, topi40_spTrackEntry *entry) {
	_topi40_spEventQueue_addType(self, SP_ANIMATION_DISPOSE);
	_topi40_spEventQueue_addEntry(self, entry);
}

void _topi40_spEventQueue_complete(_topi40_spEventQueue *self, topi40_spTrackEntry *entry) {
	_topi40_spEventQueue_addType(self, SP_ANIMATION_COMPLETE);
	_topi40_spEventQueue_addEntry(self, entry);
}

void _topi40_spEventQueue_event(_topi40_spEventQueue *self, topi40_spTrackEntry *entry, topi40_spEvent *event) {
	_topi40_spEventQueue_addType(self, SP_ANIMATION_EVENT);
	_topi40_spEventQueue_addEntry(self, entry);
	_topi40_spEventQueue_addEvent(self, event);
}

void _topi40_spEventQueue_clear(_topi40_spEventQueue *self) {
	self->objectsCount = 0;
}

void _topi40_spEventQueue_drain(_topi40_spEventQueue *self) {
	int i;
	if (self->drainDisabled) return;
	self->drainDisabled = 1;
	for (i = 0; i < self->objectsCount; i += 2) {
		topi40_spEventType type = (topi40_spEventType) self->objects[i].type;
		topi40_spTrackEntry *entry = self->objects[i + 1].entry;
		topi40_spEvent *event;
		switch (type) {
			case SP_ANIMATION_START:
			case SP_ANIMATION_INTERRUPT:
			case SP_ANIMATION_COMPLETE:
				if (entry->listener) entry->listener(SUPER(self->state), type, entry, 0);
				if (self->state->super.listener) self->state->super.listener(SUPER(self->state), type, entry, 0);
				break;
			case SP_ANIMATION_END:
				if (entry->listener) entry->listener(SUPER(self->state), type, entry, 0);
				if (self->state->super.listener) self->state->super.listener(SUPER(self->state), type, entry, 0);
				/* Fall through. */
			case SP_ANIMATION_DISPOSE:
				if (entry->listener) entry->listener(SUPER(self->state), SP_ANIMATION_DISPOSE, entry, 0);
				if (self->state->super.listener)
					self->state->super.listener(SUPER(self->state), SP_ANIMATION_DISPOSE, entry, 0);
				_topi40_spAnimationState_disposeTrackEntry(entry);
				break;
			case SP_ANIMATION_EVENT:
				event = self->objects[i + 2].event;
				if (entry->listener) entry->listener(SUPER(self->state), type, entry, event);
				if (self->state->super.listener) self->state->super.listener(SUPER(self->state), type, entry, event);
				i++;
				break;
		}
	}
	_topi40_spEventQueue_clear(self);

	self->drainDisabled = 0;
}

/* These two functions are needed in the UE4 runtime, see #1037 */
void _topi40_spAnimationState_enableQueue(topi40_spAnimationState *self) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	internal->queue->drainDisabled = 0;
}

void _topi40_spAnimationState_disableQueue(topi40_spAnimationState *self) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	internal->queue->drainDisabled = 1;
}

void _topi40_spAnimationState_disposeTrackEntry(topi40_spTrackEntry *entry) {
	topi40_spIntArray_dispose(entry->timelineMode);
	topi40_spTrackEntryArray_dispose(entry->timelineHoldMix);
	FREE(entry->timelinesRotation);
	FREE(entry);
}

void _topi40_spAnimationState_disposeTrackEntries(topi40_spAnimationState *state, topi40_spTrackEntry *entry) {
	while (entry) {
		topi40_spTrackEntry *next = entry->next;
		topi40_spTrackEntry *from = entry->mixingFrom;
		while (from) {
			topi40_spTrackEntry *nextFrom = from->mixingFrom;
			if (entry->listener) entry->listener(state, SP_ANIMATION_DISPOSE, from, 0);
			if (state->listener) state->listener(state, SP_ANIMATION_DISPOSE, from, 0);
			_topi40_spAnimationState_disposeTrackEntry(from);
			from = nextFrom;
		}
		if (entry->listener) entry->listener(state, SP_ANIMATION_DISPOSE, entry, 0);
		if (state->listener) state->listener(state, SP_ANIMATION_DISPOSE, entry, 0);
		_topi40_spAnimationState_disposeTrackEntry(entry);
		entry = next;
	}
}

topi40_spAnimationState *topi40_spAnimationState_create(topi40_spAnimationStateData *data) {
	_topi40_spAnimationState *internal;
	topi40_spAnimationState *self;

	if (!SP_EMPTY_ANIMATION) {
		SP_EMPTY_ANIMATION = (topi40_spAnimation *) 1; /* dirty trick so we can recursively call topi40_spAnimation_create */
		SP_EMPTY_ANIMATION = topi40_spAnimation_create("<empty>", NULL, 0);
	}

	internal = NEW(_topi40_spAnimationState);
	self = SUPER(internal);

	CONST_CAST(topi40_spAnimationStateData *, self->data) = data;
	self->timeScale = 1;

	internal->queue = _topi40_spEventQueue_create(internal);
	internal->events = CALLOC(topi40_spEvent *, 128);

	internal->propertyIDs = CALLOC(topi40_spPropertyId, 128);
	internal->propertyIDsCapacity = 128;

	return self;
}

void topi40_spAnimationState_dispose(topi40_spAnimationState *self) {
	int i;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	for (i = 0; i < self->tracksCount; i++)
		_topi40_spAnimationState_disposeTrackEntries(self, self->tracks[i]);
	FREE(self->tracks);
	_topi40_spEventQueue_free(internal->queue);
	FREE(internal->events);
	FREE(internal->propertyIDs);
	FREE(internal);
}

void topi40_spAnimationState_update(topi40_spAnimationState *self, float delta) {
	int i, n;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	delta *= self->timeScale;
	for (i = 0, n = self->tracksCount; i < n; i++) {
		float currentDelta;
		topi40_spTrackEntry *current = self->tracks[i];
		topi40_spTrackEntry *next;
		if (!current) continue;

		current->animationLast = current->nextAnimationLast;
		current->trackLast = current->nextTrackLast;

		currentDelta = delta * current->timeScale;

		if (current->delay > 0) {
			current->delay -= currentDelta;
			if (current->delay > 0) continue;
			currentDelta = -current->delay;
			current->delay = 0;
		}

		next = current->next;
		if (next) {
			/* When the next entry's delay is passed, change to the next entry, preserving leftover time. */
			float nextTime = current->trackLast - next->delay;
			if (nextTime >= 0) {
				next->delay = 0;
				next->trackTime +=
						current->timeScale == 0 ? 0 : (nextTime / current->timeScale + delta) * next->timeScale;
				current->trackTime += currentDelta;
				_topi40_spAnimationState_setCurrent(self, i, next, 1);
				while (next->mixingFrom) {
					next->mixTime += delta;
					next = next->mixingFrom;
				}
				continue;
			}
		} else {
			/* Clear the track when there is no next entry, the track end time is reached, and there is no mixingFrom. */
			if (current->trackLast >= current->trackEnd && current->mixingFrom == 0) {
				self->tracks[i] = 0;
				_topi40_spEventQueue_end(internal->queue, current);
				topi40_spAnimationState_clearNext(self, current);
				continue;
			}
		}
		if (current->mixingFrom != 0 && _topi40_spAnimationState_updateMixingFrom(self, current, delta)) {
			/* End mixing from entries once all have completed. */
			topi40_spTrackEntry *from = current->mixingFrom;
			current->mixingFrom = 0;
			if (from != 0) from->mixingTo = 0;
			while (from != 0) {
				_topi40_spEventQueue_end(internal->queue, from);
				from = from->mixingFrom;
			}
		}

		current->trackTime += currentDelta;
	}

	_topi40_spEventQueue_drain(internal->queue);
}

int /*boolean*/ _topi40_spAnimationState_updateMixingFrom(topi40_spAnimationState *self, topi40_spTrackEntry *to, float delta) {
	topi40_spTrackEntry *from = to->mixingFrom;
	int finished;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	if (!from) return -1;

	finished = _topi40_spAnimationState_updateMixingFrom(self, from, delta);

	from->animationLast = from->nextAnimationLast;
	from->trackLast = from->nextTrackLast;

	/* Require mixTime > 0 to ensure the mixing from entry was applied at least once. */
	if (to->mixTime > 0 && to->mixTime >= to->mixDuration) {
		/* Require totalAlpha == 0 to ensure mixing is complete, unless mixDuration == 0 (the transition is a single frame). */
		if (from->totalAlpha == 0 || to->mixDuration == 0) {
			to->mixingFrom = from->mixingFrom;
			if (from->mixingFrom != 0) from->mixingFrom->mixingTo = to;
			to->interruptAlpha = from->interruptAlpha;
			_topi40_spEventQueue_end(internal->queue, from);
		}
		return finished;
	}

	from->trackTime += delta * from->timeScale;
	to->mixTime += delta;
	return 0;
}

int topi40_spAnimationState_apply(topi40_spAnimationState *self, topi40_spSkeleton *skeleton) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	topi40_spTrackEntry *current;
	int i, ii, n;
	float animationLast, animationTime;
	int timelineCount;
	topi40_spTimeline **timelines;
	int /*boolean*/ firstFrame;
	float *timelinesRotation;
	topi40_spTimeline *timeline;
	int applied = 0;
	topi40_spMixBlend blend;
	topi40_spMixBlend timelineBlend;
	int setupState = 0;
	topi40_spSlot **slots = NULL;
	topi40_spSlot *slot = NULL;
	const char *attachmentName = NULL;
	topi40_spEvent **applyEvents = NULL;
	float applyTime;

	if (internal->animationsChanged) _topi40_spAnimationState_animationsChanged(self);

	for (i = 0, n = self->tracksCount; i < n; i++) {
		float mix;
		current = self->tracks[i];
		if (!current || current->delay > 0) continue;
		applied = -1;
		blend = i == 0 ? SP_MIX_BLEND_FIRST : current->mixBlend;

		/* Apply mixing from entries first. */
		mix = current->alpha;
		if (current->mixingFrom)
			mix *= _topi40_spAnimationState_applyMixingFrom(self, current, skeleton, blend);
		else if (current->trackTime >= current->trackEnd && current->next == 0)
			mix = 0;

		/* Apply current entry. */
		animationLast = current->animationLast;
		animationTime = topi40_spTrackEntry_getAnimationTime(current);
		timelineCount = current->animation->timelines->size;
		applyEvents = internal->events;
		applyTime = animationTime;
		if (current->reverse) {
			applyTime = current->animation->duration - applyTime;
			applyEvents = NULL;
		}
		timelines = current->animation->timelines->items;
		if ((i == 0 && mix == 1) || blend == SP_MIX_BLEND_ADD) {
			for (ii = 0; ii < timelineCount; ii++) {
				timeline = timelines[ii];
				if (timeline->type == SP_TIMELINE_ATTACHMENT) {
					_topi40_spAnimationState_applyAttachmentTimeline(self, timeline, skeleton, applyTime, blend, -1);
				} else {
					topi40_spTimeline_apply(timelines[ii], skeleton, animationLast, applyTime, applyEvents,
									 &internal->eventsCount, mix, blend, SP_MIX_DIRECTION_IN);
				}
			}
		} else {
			topi40_spIntArray *timelineMode = current->timelineMode;

			firstFrame = current->timelinesRotationCount != timelineCount << 1;
			if (firstFrame) _topi40_spAnimationState_resizeTimelinesRotation(current, timelineCount << 1);
			timelinesRotation = current->timelinesRotation;

			for (ii = 0; ii < timelineCount; ii++) {
				timeline = timelines[ii];
				timelineBlend = timelineMode->items[ii] == SUBSEQUENT ? blend : SP_MIX_BLEND_SETUP;
				if (timeline->propertyIds[0] == SP_PROPERTY_ROTATE)
					_topi40_spAnimationState_applyRotateTimeline(self, timeline, skeleton, applyTime, mix, timelineBlend,
														  timelinesRotation, ii << 1, firstFrame);
				else if (timeline->type == SP_TIMELINE_ATTACHMENT)
					_topi40_spAnimationState_applyAttachmentTimeline(self, timeline, skeleton, applyTime, timelineBlend, -1);
				else
					topi40_spTimeline_apply(timeline, skeleton, animationLast, applyTime, applyEvents, &internal->eventsCount,
									 mix, timelineBlend, SP_MIX_DIRECTION_IN);
			}
		}
		_topi40_spAnimationState_queueEvents(self, current, animationTime);
		internal->eventsCount = 0;
		current->nextAnimationLast = animationTime;
		current->nextTrackLast = current->trackTime;
	}

	setupState = self->unkeyedState + SETUP;
	slots = skeleton->slots;
	for (i = 0, n = skeleton->slotsCount; i < n; i++) {
		slot = slots[i];
		if (slot->attachmentState == setupState) {
			attachmentName = slot->data->attachmentName;
			topi40_spSlot_setAttachment(slot, attachmentName == NULL ? NULL : topi40_spSkeleton_getAttachmentForSlotIndex(skeleton, slot->data->index, attachmentName));
		}
	}
	self->unkeyedState += 2;

	_topi40_spEventQueue_drain(internal->queue);
	return applied;
}

float _topi40_spAnimationState_applyMixingFrom(topi40_spAnimationState *self, topi40_spTrackEntry *to, topi40_spSkeleton *skeleton, topi40_spMixBlend blend) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	float mix;
	topi40_spEvent **events;
	int /*boolean*/ attachments;
	int /*boolean*/ drawOrder;
	float animationLast;
	float animationTime;
	int timelineCount;
	topi40_spTimeline **timelines;
	topi40_spIntArray *timelineMode;
	topi40_spTrackEntryArray *timelineHoldMix;
	topi40_spMixBlend timelineBlend;
	float alphaHold;
	float alphaMix;
	float alpha;
	int /*boolean*/ firstFrame;
	float *timelinesRotation;
	int i;
	topi40_spTrackEntry *holdMix;
	float applyTime;

	topi40_spTrackEntry *from = to->mixingFrom;
	if (from->mixingFrom) _topi40_spAnimationState_applyMixingFrom(self, from, skeleton, blend);

	if (to->mixDuration == 0) { /* Single frame mix to undo mixingFrom changes. */
		mix = 1;
		if (blend == SP_MIX_BLEND_FIRST) blend = SP_MIX_BLEND_SETUP;
	} else {
		mix = to->mixTime / to->mixDuration;
		if (mix > 1) mix = 1;
		if (blend != SP_MIX_BLEND_FIRST) blend = from->mixBlend;
	}

	attachments = mix < from->attachmentThreshold;
	drawOrder = mix < from->drawOrderThreshold;
	timelineCount = from->animation->timelines->size;
	timelines = from->animation->timelines->items;
	alphaHold = from->alpha * to->interruptAlpha;
	alphaMix = alphaHold * (1 - mix);
	animationLast = from->animationLast;
	animationTime = topi40_spTrackEntry_getAnimationTime(from);
	applyTime = animationTime;
	events = NULL;
	if (from->reverse) {
		applyTime = from->animation->duration - applyTime;
	} else {
		if (mix < from->eventThreshold) events = internal->events;
	}

	if (blend == SP_MIX_BLEND_ADD) {
		for (i = 0; i < timelineCount; i++) {
			topi40_spTimeline *timeline = timelines[i];
			topi40_spTimeline_apply(timeline, skeleton, animationLast, applyTime, events, &internal->eventsCount, alphaMix,
							 blend, SP_MIX_DIRECTION_OUT);
		}
	} else {
		timelineMode = from->timelineMode;
		timelineHoldMix = from->timelineHoldMix;

		firstFrame = from->timelinesRotationCount != timelineCount << 1;
		if (firstFrame) _topi40_spAnimationState_resizeTimelinesRotation(from, timelineCount << 1);
		timelinesRotation = from->timelinesRotation;

		from->totalAlpha = 0;
		for (i = 0; i < timelineCount; i++) {
			topi40_spMixDirection direction = SP_MIX_DIRECTION_OUT;
			topi40_spTimeline *timeline = timelines[i];

			switch (timelineMode->items[i]) {
				case SUBSEQUENT:
					if (!drawOrder && timeline->type == SP_TIMELINE_DRAWORDER) continue;
					timelineBlend = blend;
					alpha = alphaMix;
					break;
				case FIRST:
					timelineBlend = SP_MIX_BLEND_SETUP;
					alpha = alphaMix;
					break;
				case HOLD_SUBSEQUENT:
					timelineBlend = blend;
					alpha = alphaHold;
					break;
				case HOLD_FIRST:
					timelineBlend = SP_MIX_BLEND_SETUP;
					alpha = alphaHold;
					break;
				default:
					timelineBlend = SP_MIX_BLEND_SETUP;
					holdMix = timelineHoldMix->items[i];
					alpha = alphaHold * MAX(0, 1 - holdMix->mixTime / holdMix->mixDuration);
					break;
			}
			from->totalAlpha += alpha;
			if (timeline->type == SP_TIMELINE_ROTATE)
				_topi40_spAnimationState_applyRotateTimeline(self, timeline, skeleton, applyTime, alpha, timelineBlend,
													  timelinesRotation, i << 1, firstFrame);
			else if (timeline->type == SP_TIMELINE_ATTACHMENT)
				_topi40_spAnimationState_applyAttachmentTimeline(self, timeline, skeleton, applyTime, timelineBlend,
														  attachments);
			else {
				if (drawOrder && timeline->type == SP_TIMELINE_DRAWORDER &&
					timelineBlend == SP_MIX_BLEND_SETUP)
					direction = SP_MIX_DIRECTION_IN;
				topi40_spTimeline_apply(timeline, skeleton, animationLast, applyTime, events, &internal->eventsCount,
								 alpha, timelineBlend, direction);
			}
		}
	}


	if (to->mixDuration > 0) _topi40_spAnimationState_queueEvents(self, from, animationTime);
	internal->eventsCount = 0;
	from->nextAnimationLast = animationTime;
	from->nextTrackLast = from->trackTime;

	return mix;
}

static void
_topi40_spAnimationState_setAttachment(topi40_spAnimationState *self, topi40_spSkeleton *skeleton, topi40_spSlot *slot, const char *attachmentName,
								int /*bool*/ attachments) {
	topi40_spSlot_setAttachment(slot, attachmentName == NULL ? NULL : topi40_spSkeleton_getAttachmentForSlotIndex(skeleton, slot->data->index, attachmentName));
	if (attachments) slot->attachmentState = self->unkeyedState + CURRENT;
}

/* @param target After the first and before the last entry. */
static int binarySearch1(float *values, int valuesLength, float target) {
	int i;
	for (i = 1; i < valuesLength; i++) {
		if (values[i] > target) return (int) (i - 1);
	}
	return (int) valuesLength - 1;
}

void _topi40_spAnimationState_applyAttachmentTimeline(topi40_spAnimationState *self, topi40_spTimeline *timeline, topi40_spSkeleton *skeleton,
											   float time, topi40_spMixBlend blend, int /*bool*/ attachments) {
	topi40_spAttachmentTimeline *attachmentTimeline;
	topi40_spSlot *slot;
	float *frames;

	attachmentTimeline = SUB_CAST(topi40_spAttachmentTimeline, timeline);
	slot = skeleton->slots[attachmentTimeline->slotIndex];
	if (!slot->bone->active) return;

	frames = attachmentTimeline->super.frames->items;
	if (time < frames[0]) {
		if (blend == SP_MIX_BLEND_SETUP || blend == SP_MIX_BLEND_FIRST)
			_topi40_spAnimationState_setAttachment(self, skeleton, slot, slot->data->attachmentName, attachments);
	} else {
		_topi40_spAnimationState_setAttachment(self, skeleton, slot, attachmentTimeline->attachmentNames[binarySearch1(frames, attachmentTimeline->super.frames->size, time)],
										attachments);
	}

	/* If an attachment wasn't set (ie before the first frame or attachments is false), set the setup attachment later.*/
	if (slot->attachmentState <= self->unkeyedState) slot->attachmentState = self->unkeyedState + SETUP;
}

void _topi40_spAnimationState_applyRotateTimeline(topi40_spAnimationState *self, topi40_spTimeline *timeline, topi40_spSkeleton *skeleton, float time,
										   float alpha, topi40_spMixBlend blend, float *timelinesRotation, int i,
										   int /*boolean*/ firstFrame) {
	topi40_spRotateTimeline *rotateTimeline;
	float *frames;
	topi40_spBone *bone;
	float r1, r2;
	float total, diff;
	int /*boolean*/ current, dir;
	UNUSED(self);

	if (firstFrame) timelinesRotation[i] = 0;

	if (alpha == 1) {
		topi40_spTimeline_apply(timeline, skeleton, 0, time, 0, 0, 1, blend, SP_MIX_DIRECTION_IN);
		return;
	}

	rotateTimeline = SUB_CAST(topi40_spRotateTimeline, timeline);
	frames = rotateTimeline->super.super.frames->items;
	bone = skeleton->bones[rotateTimeline->boneIndex];
	if (!bone->active) return;
	if (time < frames[0]) {
		switch (blend) {
			case SP_MIX_BLEND_SETUP:
				bone->rotation = bone->data->rotation;
			default:
				return;
			case SP_MIX_BLEND_FIRST:
				r1 = bone->rotation;
				r2 = bone->data->rotation;
		}
	} else {
		r1 = blend == SP_MIX_BLEND_SETUP ? bone->data->rotation : bone->rotation;
		r2 = bone->data->rotation + topi40_spCurveTimeline1_getCurveValue(&rotateTimeline->super, time);
	}

	/* Mix between rotations using the direction of the shortest route on the first frame while detecting crosses. */
	diff = r2 - r1;
	diff -= (16384 - (int) (16384.499999999996 - diff / 360)) * 360;
	if (diff == 0) {
		total = timelinesRotation[i];
	} else {
		float lastTotal, lastDiff;
		if (firstFrame) {
			lastTotal = 0;
			lastDiff = diff;
		} else {
			lastTotal = timelinesRotation[i];    /* Angle and direction of mix, including loops. */
			lastDiff = timelinesRotation[i + 1]; /* Difference between bones. */
		}
		current = diff > 0;
		dir = lastTotal >= 0;
		/* Detect cross at 0 (not 180). */
		if (SIGNUM(lastDiff) != SIGNUM(diff) && ABS(lastDiff) <= 90) {
			/* A cross after a 360 rotation is a loop. */
			if (ABS(lastTotal) > 180) lastTotal += 360 * SIGNUM(lastTotal);
			dir = current;
		}
		total = diff + lastTotal - FMOD(lastTotal, 360); /* Store loops as part of lastTotal. */
		if (dir != current) total += 360 * SIGNUM(lastTotal);
		timelinesRotation[i] = total;
	}
	timelinesRotation[i + 1] = diff;
	bone->rotation = r1 + total * alpha;
}

void _topi40_spAnimationState_queueEvents(topi40_spAnimationState *self, topi40_spTrackEntry *entry, float animationTime) {
	topi40_spEvent **events;
	topi40_spEvent *event;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	int i, n, complete;
	float animationStart = entry->animationStart, animationEnd = entry->animationEnd;
	float duration = animationEnd - animationStart;
	float trackLastWrapped = FMOD(entry->trackLast, duration);

	/* Queue events before complete. */
	events = internal->events;
	for (i = 0, n = internal->eventsCount; i < n; i++) {
		event = events[i];
		if (event->time < trackLastWrapped) break;
		if (event->time > animationEnd) continue; /* Discard events outside animation start/end. */
		_topi40_spEventQueue_event(internal->queue, entry, event);
	}

	/* Queue complete if completed a loop iteration or the animation. */
	if (entry->loop)
		complete = duration == 0 || (trackLastWrapped > FMOD(entry->trackTime, duration));
	else
		complete = (animationTime >= animationEnd && entry->animationLast < animationEnd);
	if (complete) _topi40_spEventQueue_complete(internal->queue, entry);

	/* Queue events after complete. */
	for (; i < n; i++) {
		event = events[i];
		if (event->time < animationStart) continue; /* Discard events outside animation start/end. */
		_topi40_spEventQueue_event(internal->queue, entry, event);
	}
}

void topi40_spAnimationState_clearTracks(topi40_spAnimationState *self) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	int i, n, oldDrainDisabled;
	oldDrainDisabled = internal->queue->drainDisabled;
	internal->queue->drainDisabled = 1;
	for (i = 0, n = self->tracksCount; i < n; i++)
		topi40_spAnimationState_clearTrack(self, i);
	self->tracksCount = 0;
	internal->queue->drainDisabled = oldDrainDisabled;
	_topi40_spEventQueue_drain(internal->queue);
}

void topi40_spAnimationState_clearTrack(topi40_spAnimationState *self, int trackIndex) {
	topi40_spTrackEntry *current;
	topi40_spTrackEntry *entry;
	topi40_spTrackEntry *from;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);

	if (trackIndex >= self->tracksCount) return;
	current = self->tracks[trackIndex];
	if (!current) return;

	_topi40_spEventQueue_end(internal->queue, current);

	topi40_spAnimationState_clearNext(self, current);

	entry = current;
	while (1) {
		from = entry->mixingFrom;
		if (!from) break;
		_topi40_spEventQueue_end(internal->queue, from);
		entry->mixingFrom = 0;
		entry->mixingTo = 0;
		entry = from;
	}

	self->tracks[current->trackIndex] = 0;
	_topi40_spEventQueue_drain(internal->queue);
}

void _topi40_spAnimationState_setCurrent(topi40_spAnimationState *self, int index, topi40_spTrackEntry *current, int /*boolean*/ interrupt) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	topi40_spTrackEntry *from = _topi40_spAnimationState_expandToIndex(self, index);
	self->tracks[index] = current;
	current->previous = NULL;

	if (from) {
		if (interrupt) _topi40_spEventQueue_interrupt(internal->queue, from);
		current->mixingFrom = from;
		from->mixingTo = current;
		current->mixTime = 0;

		/* Store the interrupted mix percentage. */
		if (from->mixingFrom != 0 && from->mixDuration > 0)
			current->interruptAlpha *= MIN(1, from->mixTime / from->mixDuration);

		from->timelinesRotationCount = 0;
	}

	_topi40_spEventQueue_start(internal->queue, current);
}

/** Set the current animation. Any queued animations are cleared. */
topi40_spTrackEntry *topi40_spAnimationState_setAnimationByName(topi40_spAnimationState *self, int trackIndex, const char *animationName,
												  int /*bool*/ loop) {
	topi40_spAnimation *animation = topi40_spSkeletonData_findAnimation(self->data->skeletonData, animationName);
	return topi40_spAnimationState_setAnimation(self, trackIndex, animation, loop);
}

topi40_spTrackEntry *
topi40_spAnimationState_setAnimation(topi40_spAnimationState *self, int trackIndex, topi40_spAnimation *animation, int /*bool*/ loop) {
	topi40_spTrackEntry *entry;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	int interrupt = 1;
	topi40_spTrackEntry *current = _topi40_spAnimationState_expandToIndex(self, trackIndex);
	if (current) {
		if (current->nextTrackLast == -1) {
			/* Don't mix from an entry that was never applied. */
			self->tracks[trackIndex] = current->mixingFrom;
			_topi40_spEventQueue_interrupt(internal->queue, current);
			_topi40_spEventQueue_end(internal->queue, current);
			topi40_spAnimationState_clearNext(self, current);
			current = current->mixingFrom;
			interrupt = 0;
		} else
			topi40_spAnimationState_clearNext(self, current);
	}
	entry = _topi40_spAnimationState_trackEntry(self, trackIndex, animation, loop, current);
	_topi40_spAnimationState_setCurrent(self, trackIndex, entry, interrupt);
	_topi40_spEventQueue_drain(internal->queue);
	return entry;
}

/** Adds an animation to be played delay seconds after the current or last queued animation, taking into account any mix
 * duration. */
topi40_spTrackEntry *topi40_spAnimationState_addAnimationByName(topi40_spAnimationState *self, int trackIndex, const char *animationName,
												  int /*bool*/ loop, float delay) {
	topi40_spAnimation *animation = topi40_spSkeletonData_findAnimation(self->data->skeletonData, animationName);
	return topi40_spAnimationState_addAnimation(self, trackIndex, animation, loop, delay);
}

topi40_spTrackEntry *
topi40_spAnimationState_addAnimation(topi40_spAnimationState *self, int trackIndex, topi40_spAnimation *animation, int /*bool*/ loop,
							  float delay) {
	topi40_spTrackEntry *entry;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	topi40_spTrackEntry *last = _topi40_spAnimationState_expandToIndex(self, trackIndex);
	if (last) {
		while (last->next)
			last = last->next;
	}

	entry = _topi40_spAnimationState_trackEntry(self, trackIndex, animation, loop, last);

	if (!last) {
		_topi40_spAnimationState_setCurrent(self, trackIndex, entry, 1);
		_topi40_spEventQueue_drain(internal->queue);
	} else {
		last->next = entry;
		entry->previous = last;
		if (delay <= 0) delay += topi40_spTrackEntry_getTrackComplete(last) - entry->mixDuration;
	}

	entry->delay = delay;
	return entry;
}

topi40_spTrackEntry *topi40_spAnimationState_setEmptyAnimation(topi40_spAnimationState *self, int trackIndex, float mixDuration) {
	topi40_spTrackEntry *entry = topi40_spAnimationState_setAnimation(self, trackIndex, SP_EMPTY_ANIMATION, 0);
	entry->mixDuration = mixDuration;
	entry->trackEnd = mixDuration;
	return entry;
}

topi40_spTrackEntry *
topi40_spAnimationState_addEmptyAnimation(topi40_spAnimationState *self, int trackIndex, float mixDuration, float delay) {
	topi40_spTrackEntry *entry = topi40_spAnimationState_addAnimation(self, trackIndex, SP_EMPTY_ANIMATION, 0, delay);
	if (delay <= 0) entry->delay += entry->mixDuration - mixDuration;
	entry->mixDuration = mixDuration;
	entry->trackEnd = mixDuration;
	return entry;
}

void topi40_spAnimationState_setEmptyAnimations(topi40_spAnimationState *self, float mixDuration) {
	int i, n, oldDrainDisabled;
	topi40_spTrackEntry *current;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	oldDrainDisabled = internal->queue->drainDisabled;
	internal->queue->drainDisabled = 1;
	for (i = 0, n = self->tracksCount; i < n; i++) {
		current = self->tracks[i];
		if (current) topi40_spAnimationState_setEmptyAnimation(self, current->trackIndex, mixDuration);
	}
	internal->queue->drainDisabled = oldDrainDisabled;
	_topi40_spEventQueue_drain(internal->queue);
}

topi40_spTrackEntry *_topi40_spAnimationState_expandToIndex(topi40_spAnimationState *self, int index) {
	topi40_spTrackEntry **newTracks;
	if (index < self->tracksCount) return self->tracks[index];
	newTracks = CALLOC(topi40_spTrackEntry *, index + 1);
	memcpy(newTracks, self->tracks, self->tracksCount * sizeof(topi40_spTrackEntry *));
	FREE(self->tracks);
	self->tracks = newTracks;
	self->tracksCount = index + 1;
	return 0;
}

topi40_spTrackEntry *
_topi40_spAnimationState_trackEntry(topi40_spAnimationState *self, int trackIndex, topi40_spAnimation *animation, int /*boolean*/ loop,
							 topi40_spTrackEntry *last) {
	topi40_spTrackEntry *entry = NEW(topi40_spTrackEntry);
	entry->trackIndex = trackIndex;
	entry->animation = animation;
	entry->loop = loop;
	entry->holdPrevious = 0;
	entry->reverse = 0;
	entry->previous = 0;
	entry->next = 0;

	entry->eventThreshold = 0;
	entry->attachmentThreshold = 0;
	entry->drawOrderThreshold = 0;

	entry->animationStart = 0;
	entry->animationEnd = animation->duration;
	entry->animationLast = -1;
	entry->nextAnimationLast = -1;

	entry->delay = 0;
	entry->trackTime = 0;
	entry->trackLast = -1;
	entry->nextTrackLast = -1;
	entry->trackEnd = (float) INT_MAX;
	entry->timeScale = 1;

	entry->alpha = 1;
	entry->interruptAlpha = 1;
	entry->mixTime = 0;
	entry->mixDuration = !last ? 0 : topi40_spAnimationStateData_getMix(self->data, last->animation, animation);
	entry->mixBlend = SP_MIX_BLEND_REPLACE;

	entry->timelineMode = topi40_spIntArray_create(16);
	entry->timelineHoldMix = topi40_spTrackEntryArray_create(16);

	return entry;
}

void topi40_spAnimationState_clearNext(topi40_spAnimationState *self, topi40_spTrackEntry *entry) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	topi40_spTrackEntry *next = entry->next;
	while (next) {
		_topi40_spEventQueue_dispose(internal->queue, next);
		next = next->next;
	}
	entry->next = 0;
}

void _topi40_spAnimationState_animationsChanged(topi40_spAnimationState *self) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	int i, n;
	topi40_spTrackEntry *entry;
	internal->animationsChanged = 0;

	internal->propertyIDsCount = 0;
	i = 0;
	n = self->tracksCount;

	for (; i < n; i++) {
		entry = self->tracks[i];
		if (!entry) continue;
		while (entry->mixingFrom != 0)
			entry = entry->mixingFrom;
		do {
			if (entry->mixingTo == 0 || entry->mixBlend != SP_MIX_BLEND_ADD) _topi40_spTrackEntry_computeHold(entry, self);
			entry = entry->mixingTo;
		} while (entry != 0);
	}
}

float *_topi40_spAnimationState_resizeTimelinesRotation(topi40_spTrackEntry *entry, int newSize) {
	if (entry->timelinesRotationCount != newSize) {
		float *newTimelinesRotation = CALLOC(float, newSize);
		FREE(entry->timelinesRotation);
		entry->timelinesRotation = newTimelinesRotation;
		entry->timelinesRotationCount = newSize;
	}
	return entry->timelinesRotation;
}

void _topi40_spAnimationState_ensureCapacityPropertyIDs(topi40_spAnimationState *self, int capacity) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	if (internal->propertyIDsCapacity < capacity) {
		topi40_spPropertyId *newPropertyIDs = CALLOC(topi40_spPropertyId, capacity << 1);
		memcpy(newPropertyIDs, internal->propertyIDs, sizeof(topi40_spPropertyId) * internal->propertyIDsCount);
		FREE(internal->propertyIDs);
		internal->propertyIDs = newPropertyIDs;
		internal->propertyIDsCapacity = capacity << 1;
	}
}

int _topi40_spAnimationState_addPropertyID(topi40_spAnimationState *self, topi40_spPropertyId id) {
	int i, n;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);

	for (i = 0, n = internal->propertyIDsCount; i < n; i++) {
		if (internal->propertyIDs[i] == id) return 0;
	}

	_topi40_spAnimationState_ensureCapacityPropertyIDs(self, internal->propertyIDsCount + 1);
	internal->propertyIDs[internal->propertyIDsCount] = id;
	internal->propertyIDsCount++;
	return 1;
}

int _topi40_spAnimationState_addPropertyIDs(topi40_spAnimationState *self, topi40_spPropertyId *ids, int numIds) {
	int i, n;
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	int oldSize = internal->propertyIDsCount;

	for (i = 0, n = numIds; i < n; i++) {
		_topi40_spAnimationState_addPropertyID(self, ids[i]);
	}

	return internal->propertyIDsCount != oldSize;
}

topi40_spTrackEntry *topi40_spAnimationState_getCurrent(topi40_spAnimationState *self, int trackIndex) {
	if (trackIndex >= self->tracksCount) return 0;
	return self->tracks[trackIndex];
}

void topi40_spAnimationState_clearListenerNotifications(topi40_spAnimationState *self) {
	_topi40_spAnimationState *internal = SUB_CAST(_topi40_spAnimationState, self);
	_topi40_spEventQueue_clear(internal->queue);
}

float topi40_spTrackEntry_getAnimationTime(topi40_spTrackEntry *entry) {
	if (entry->loop) {
		float duration = entry->animationEnd - entry->animationStart;
		if (duration == 0) return entry->animationStart;
		return FMOD(entry->trackTime, duration) + entry->animationStart;
	}
	return MIN(entry->trackTime + entry->animationStart, entry->animationEnd);
}

float topi40_spTrackEntry_getTrackComplete(topi40_spTrackEntry *entry) {
	float duration = entry->animationEnd - entry->animationStart;
	if (duration != 0) {
		if (entry->loop) return duration * (1 + (int) (entry->trackTime / duration)); /* Completion of next loop. */
		if (entry->trackTime < duration) return duration;                             /* Before duration. */
	}
	return entry->trackTime; /* Next update. */
}

void _topi40_spTrackEntry_computeHold(topi40_spTrackEntry *entry, topi40_spAnimationState *state) {
	topi40_spTrackEntry *to;
	topi40_spTimeline **timelines;
	int timelinesCount;
	int *timelineMode;
	topi40_spTrackEntry **timelineHoldMix;
	topi40_spTrackEntry *next;
	int i;

	to = entry->mixingTo;
	timelines = entry->animation->timelines->items;
	timelinesCount = entry->animation->timelines->size;
	timelineMode = topi40_spIntArray_setSize(entry->timelineMode, timelinesCount)->items;
	topi40_spTrackEntryArray_clear(entry->timelineHoldMix);
	timelineHoldMix = topi40_spTrackEntryArray_setSize(entry->timelineHoldMix, timelinesCount)->items;

	if (to != 0 && to->holdPrevious) {
		for (i = 0; i < timelinesCount; i++) {
			topi40_spPropertyId *ids = timelines[i]->propertyIds;
			int numIds = timelines[i]->propertyIdsCount;
			timelineMode[i] = _topi40_spAnimationState_addPropertyIDs(state, ids, numIds) ? HOLD_FIRST : HOLD_SUBSEQUENT;
		}
		return;
	}

	i = 0;
continue_outer:
	for (; i < timelinesCount; i++) {
		topi40_spTimeline *timeline = timelines[i];
		topi40_spPropertyId *ids = timeline->propertyIds;
		int numIds = timeline->propertyIdsCount;
		if (!_topi40_spAnimationState_addPropertyIDs(state, ids, numIds))
			timelineMode[i] = SUBSEQUENT;
		else if (to == 0 || timeline->type == SP_TIMELINE_ATTACHMENT ||
				 timeline->type == SP_TIMELINE_DRAWORDER ||
				 timeline->type == SP_TIMELINE_EVENT ||
				 !topi40_spAnimation_hasTimeline(to->animation, ids, numIds)) {
			timelineMode[i] = FIRST;
		} else {
			for (next = to->mixingTo; next != 0; next = next->mixingTo) {
				if (topi40_spAnimation_hasTimeline(next->animation, ids, numIds)) continue;
				if (next->mixDuration > 0) {
					timelineMode[i] = HOLD_MIX;
					timelineHoldMix[i] = next;
					i++;
					goto continue_outer;
				}
				break;
			}
			timelineMode[i] = HOLD_FIRST;
		}
	}
}
