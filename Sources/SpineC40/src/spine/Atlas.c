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

#include <ctype.h>
#include <topi40_spine/Atlas.h>
#include <topi40_spine/extension.h>

topi40_spKeyValueArray *topi40_spKeyValueArray_create(int initialCapacity) {
	topi40_spKeyValueArray *array = ((topi40_spKeyValueArray *) _topi40_spCalloc(1, sizeof(topi40_spKeyValueArray), "_file_name_", 39));
	array->size = 0;
	array->capacity = initialCapacity;
	array->items = ((topi40_spKeyValue *) _topi40_spCalloc(initialCapacity, sizeof(topi40_spKeyValue), "_file_name_", 39));
	return array;
}

void topi40_spKeyValueArray_dispose(topi40_spKeyValueArray *self) {
	_topi40_spFree((void *) self->items);
	_topi40_spFree((void *) self);
}

void topi40_spKeyValueArray_clear(topi40_spKeyValueArray *self) { self->size = 0; }

topi40_spKeyValueArray *topi40_spKeyValueArray_setSize(topi40_spKeyValueArray *self, int newSize) {
	self->size = newSize;
	if (self->capacity < newSize) {
		self->capacity = ((8) > ((int) (self->size * 1.75f)) ? (8) : ((int) (self->size * 1.75f)));
		self->items = ((topi40_spKeyValue *) _topi40_spRealloc(self->items, sizeof(topi40_spKeyValue) * (self->capacity)));
	}
	return self;
}

void topi40_spKeyValueArray_ensureCapacity(topi40_spKeyValueArray *self, int newCapacity) {
	if (self->capacity >= newCapacity) return;
	self->capacity = newCapacity;
	self->items = ((topi40_spKeyValue *) _topi40_spRealloc(self->items, sizeof(topi40_spKeyValue) * (self->capacity)));
}

void topi40_spKeyValueArray_add(topi40_spKeyValueArray *self, topi40_spKeyValue value) {
	if (self->size == self->capacity) {
		self->capacity = ((8) > ((int) (self->size * 1.75f)) ? (8) : ((int) (self->size * 1.75f)));
		self->items = ((topi40_spKeyValue *) _topi40_spRealloc(self->items, sizeof(topi40_spKeyValue) * (self->capacity)));
	}
	self->items[self->size++] = value;
}

void topi40_spKeyValueArray_addAll(topi40_spKeyValueArray *self, topi40_spKeyValueArray *other) {
	int i = 0;
	for (; i < other->size; i++) { topi40_spKeyValueArray_add(self, other->items[i]); }
}

void topi40_spKeyValueArray_addAllValues(topi40_spKeyValueArray *self, topi40_spKeyValue *values, int offset, int count) {
	int i = offset, n = offset + count;
	for (; i < n; i++) { topi40_spKeyValueArray_add(self, values[i]); }
}

int topi40_spKeyValueArray_contains(topi40_spKeyValueArray *self, topi40_spKeyValue value) {
	topi40_spKeyValue *items = self->items;
	int i, n;
	for (i = 0, n = self->size; i < n; i++) {
		if (!strcmp(items[i].name, value.name)) return -1;
	}
	return 0;
}

topi40_spKeyValue topi40_spKeyValueArray_pop(topi40_spKeyValueArray *self) {
	topi40_spKeyValue item = self->items[--self->size];
	return item;
}

topi40_spKeyValue topi40_spKeyValueArray_peek(topi40_spKeyValueArray *self) { return self->items[self->size - 1]; }

topi40_spAtlasPage *topi40_spAtlasPage_create(topi40_spAtlas *atlas, const char *name) {
	topi40_spAtlasPage *self = NEW(topi40_spAtlasPage);
	CONST_CAST(topi40_spAtlas *, self->atlas) = atlas;
	MALLOC_STR(self->name, name);
	return self;
}

void topi40_spAtlasPage_dispose(topi40_spAtlasPage *self) {
	_topi40_spAtlasPage_disposeTexture(self);
	FREE(self->name);
	FREE(self);
}

/**/

topi40_spAtlasRegion *topi40_spAtlasRegion_create() {
	topi40_spAtlasRegion *region = NEW(topi40_spAtlasRegion);
	region->keyValues = topi40_spKeyValueArray_create(2);
	return region;
}

void topi40_spAtlasRegion_dispose(topi40_spAtlasRegion *self) {
	int i, n;
	FREE(self->name);
	FREE(self->topi40_splits);
	FREE(self->pads);
	for (i = 0, n = self->keyValues->size; i < n; i++) {
		FREE(self->keyValues->items[i].name);
	}
	topi40_spKeyValueArray_dispose(self->keyValues);
	FREE(self);
}

/**/

typedef struct SimpleString {
	char *start;
	char *end;
	int length;
} SimpleString;

static SimpleString *ss_trim(SimpleString *self) {
	while (isspace((unsigned char) *self->start) && self->start < self->end)
		self->start++;
	if (self->start == self->end) {
		self->length = self->end - self->start;
		return self;
	}
	self->end--;
	while (((unsigned char) *self->end == '\r') && self->end >= self->start)
		self->end--;
	self->end++;
	self->length = self->end - self->start;
	return self;
}

static int ss_indexOf(SimpleString *self, char needle) {
	char *c = self->start;
	while (c < self->end) {
		if (*c == needle) return c - self->start;
		c++;
	}
	return -1;
}

static int ss_indexOf2(SimpleString *self, char needle, int at) {
	char *c = self->start + at;
	while (c < self->end) {
		if (*c == needle) return c - self->start;
		c++;
	}
	return -1;
}

static SimpleString ss_substr(SimpleString *self, int s, int e) {
	SimpleString result;
	e = s + e;
	result.start = self->start + s;
	result.end = self->start + e;
	result.length = e - s;
	return result;
}

static SimpleString ss_substr2(SimpleString *self, int s) {
	SimpleString result;
	result.start = self->start + s;
	result.end = self->end;
	result.length = result.end - result.start;
	return result;
}

static int /*boolean*/ ss_equals(SimpleString *self, const char *str) {
	int i;
	int otherLen = strlen(str);
	if (self->length != otherLen) return 0;
	for (i = 0; i < self->length; i++) {
		if (self->start[i] != str[i]) return 0;
	}
	return -1;
}

static char *ss_copy(SimpleString *self) {
	char *string = CALLOC(char, self->length + 1);
	memcpy(string, self->start, self->length);
	string[self->length] = '\0';
	return string;
}

static int ss_toInt(SimpleString *self) {
	return (int) strtol(self->start, &self->end, 10);
}

typedef struct AtlasInput {
	const char *start;
	const char *end;
	char *index;
	int length;
	SimpleString line;
} AtlasInput;

static SimpleString *ai_readLine(AtlasInput *self) {
	if (self->index >= self->end) return 0;
	self->line.start = self->index;
	while (self->index < self->end && *self->index != '\n')
		self->index++;
	self->line.end = self->index;
	if (self->index != self->end) self->index++;
	self->line = *ss_trim(&self->line);
	self->line.length = self->line.end - self->line.start;
	return &self->line;
}

static int ai_readEntry(SimpleString entry[5], SimpleString *line) {
	int colon, i, lastMatch;
	SimpleString substr;
	if (line == NULL) return 0;
	ss_trim(line);
	if (line->length == 0) return 0;

	colon = ss_indexOf(line, ':');
	if (colon == -1) return 0;
	substr = ss_substr(line, 0, colon);
	entry[0] = *ss_trim(&substr);
	for (i = 1, lastMatch = colon + 1;; i++) {
		int comma = ss_indexOf2(line, ',', lastMatch);
		if (comma == -1) {
			substr = ss_substr2(line, lastMatch);
			entry[i] = *ss_trim(&substr);
			return i;
		}
		substr = ss_substr(line, lastMatch, comma - lastMatch);
		entry[i] = *ss_trim(&substr);
		lastMatch = comma + 1;
		if (i == 4) return 4;
	}
}

static const char *formatNames[] = {"", "Alpha", "Intensity", "LuminanceAlpha", "RGB565", "RGBA4444", "RGB888",
									"RGBA8888"};
static const char *textureFilterNames[] = {"", "Nearest", "Linear", "MipMap", "MipMapNearestNearest",
										   "MipMapLinearNearest",
										   "MipMapNearestLinear", "MipMapLinearLinear"};

int topi40_indexOf(const char **array, int count, SimpleString *str) {
	int i;
	for (i = 0; i < count; i++)
		if (ss_equals(str, array[i])) return i;
	return 0;
}

topi40_spAtlas *topi40_spAtlas_create(const char *begin, int length, const char *dir, void *rendererObject) {
	topi40_spAtlas *self;
	AtlasInput reader;
	SimpleString *line;
	SimpleString entry[5];
	topi40_spAtlasPage *page = NULL;
	topi40_spAtlasPage *lastPage = NULL;
	topi40_spAtlasRegion *lastRegion = NULL;

	int count;
	int dirLength = (int) strlen(dir);
	int needsSlash = dirLength > 0 && dir[dirLength - 1] != '/' && dir[dirLength - 1] != '\\';

	self = NEW(topi40_spAtlas);
	self->rendererObject = rendererObject;

	reader.start = begin;
	reader.end = begin + length;
	reader.index = (char *) begin;
	reader.length = length;

	line = ai_readLine(&reader);
	while (line != NULL && line->length == 0)
		line = ai_readLine(&reader);

	while (-1) {
		if (line == NULL || line->length == 0) break;
		if (ai_readEntry(entry, line) == 0) break;
		line = ai_readLine(&reader);
	}

	while (-1) {
		if (line == NULL) break;
		if (ss_trim(line)->length == 0) {
			page = NULL;
			line = ai_readLine(&reader);
		} else if (page == NULL) {
			char *name = ss_copy(line);
			char *path = CALLOC(char, dirLength + needsSlash + strlen(name) + 1);
			memcpy(path, dir, dirLength);
			if (needsSlash) path[dirLength] = '/';
			strcpy(path + dirLength + needsSlash, name);
			page = topi40_spAtlasPage_create(self, name);
			FREE(name);

			if (lastPage)
				lastPage->next = page;
			else
				self->pages = page;
			lastPage = page;

			while (-1) {
				line = ai_readLine(&reader);
				if (ai_readEntry(entry, line) == 0) break;
				if (ss_equals(&entry[0], "size")) {
					page->width = ss_toInt(&entry[1]);
					page->height = ss_toInt(&entry[2]);
				} else if (ss_equals(&entry[0], "format")) {
					page->format = (topi40_spAtlasFormat) topi40_indexOf(formatNames, 8, &entry[1]);
				} else if (ss_equals(&entry[0], "filter")) {
					page->minFilter = (topi40_spAtlasFilter) topi40_indexOf(textureFilterNames, 8, &entry[1]);
					page->magFilter = (topi40_spAtlasFilter) topi40_indexOf(textureFilterNames, 8, &entry[2]);
				} else if (ss_equals(&entry[0], "repeat")) {
					page->uWrap = SP_ATLAS_CLAMPTOEDGE;
					page->vWrap = SP_ATLAS_CLAMPTOEDGE;
					if (ss_indexOf(&entry[1], 'x') != -1) page->uWrap = SP_ATLAS_REPEAT;
					if (ss_indexOf(&entry[1], 'y') != -1) page->vWrap = SP_ATLAS_REPEAT;
				} else if (ss_equals(&entry[0], "pma")) {
					page->pma = ss_equals(&entry[1], "true");
				}
			}

			_topi40_spAtlasPage_createTexture(page, path);
			FREE(path);
		} else {
			topi40_spAtlasRegion *region = topi40_spAtlasRegion_create();
			if (lastRegion)
				lastRegion->next = region;
			else
				self->regions = region;
			lastRegion = region;
			region->page = page;
			region->name = ss_copy(line);
			while (-1) {
				line = ai_readLine(&reader);
				count = ai_readEntry(entry, line);
				if (count == 0) break;
				if (ss_equals(&entry[0], "xy")) {
					region->x = ss_toInt(&entry[1]);
					region->y = ss_toInt(&entry[2]);
				} else if (ss_equals(&entry[0], "size")) {
					region->width = ss_toInt(&entry[1]);
					region->height = ss_toInt(&entry[2]);
				} else if (ss_equals(&entry[0], "bounds")) {
					region->x = ss_toInt(&entry[1]);
					region->y = ss_toInt(&entry[2]);
					region->width = ss_toInt(&entry[3]);
					region->height = ss_toInt(&entry[4]);
				} else if (ss_equals(&entry[0], "offset")) {
					region->offsetX = ss_toInt(&entry[1]);
					region->offsetY = ss_toInt(&entry[2]);
				} else if (ss_equals(&entry[0], "orig")) {
					region->originalWidth = ss_toInt(&entry[1]);
					region->originalHeight = ss_toInt(&entry[2]);
				} else if (ss_equals(&entry[0], "offsets")) {
					region->offsetX = ss_toInt(&entry[1]);
					region->offsetY = ss_toInt(&entry[2]);
					region->originalWidth = ss_toInt(&entry[3]);
					region->originalHeight = ss_toInt(&entry[4]);
				} else if (ss_equals(&entry[0], "rotate")) {
					if (ss_equals(&entry[1], "true")) {
						region->degrees = 90;
					} else if (!ss_equals(&entry[1], "false")) {
						region->degrees = ss_toInt(&entry[1]);
					}
				} else if (ss_equals(&entry[0], "index")) {
					region->index = ss_toInt(&entry[1]);
				} else {
					int i = 0;
					topi40_spKeyValue keyValue;
					keyValue.name = ss_copy(&entry[0]);
					for (i = 0; i < count; i++) {
						keyValue.values[i] = ss_toInt(&entry[i + 1]);
					}
					topi40_spKeyValueArray_add(region->keyValues, keyValue);
				}
			}
			if (region->originalWidth == 0 && region->originalHeight == 0) {
				region->originalWidth = region->width;
				region->originalHeight = region->height;
			}

			region->u = (float) region->x / page->width;
			region->v = (float) region->y / page->height;
			if (region->degrees == 90) {
				region->u2 = (float) (region->x + region->height) / page->width;
				region->v2 = (float) (region->y + region->width) / page->height;
			} else {
				region->u2 = (float) (region->x + region->width) / page->width;
				region->v2 = (float) (region->y + region->height) / page->height;
			}
		}
	}

	return self;
}

topi40_spAtlas *topi40_spAtlas_createFromFile(const char *path, void *rendererObject) {
	int dirLength;
	char *dir;
	int length;
	const char *data;

	topi40_spAtlas *atlas = 0;

	/* Get directory from atlas path. */
	const char *lastForwardSlash = strrchr(path, '/');
	const char *lastBackwardSlash = strrchr(path, '\\');
	const char *lastSlash = lastForwardSlash > lastBackwardSlash ? lastForwardSlash : lastBackwardSlash;
	if (lastSlash == path) lastSlash++; /* Never drop starting slash. */
	dirLength = (int) (lastSlash ? lastSlash - path : 0);
	dir = MALLOC(char, dirLength + 1);
	memcpy(dir, path, dirLength);
	dir[dirLength] = '\0';

	data = _topi40_spUtil_readFile(path, &length);
	if (data) atlas = topi40_spAtlas_create(data, length, dir, rendererObject);

	FREE(data);
	FREE(dir);
	return atlas;
}

void topi40_spAtlas_dispose(topi40_spAtlas *self) {
	topi40_spAtlasRegion *region, *nextRegion;
	topi40_spAtlasPage *page = self->pages;
	while (page) {
		topi40_spAtlasPage *nextPage = page->next;
		topi40_spAtlasPage_dispose(page);
		page = nextPage;
	}

	region = self->regions;
	while (region) {
		nextRegion = region->next;
		topi40_spAtlasRegion_dispose(region);
		region = nextRegion;
	}

	FREE(self);
}

topi40_spAtlasRegion *topi40_spAtlas_findRegion(const topi40_spAtlas *self, const char *name) {
	topi40_spAtlasRegion *region = self->regions;
	while (region) {
		if (strcmp(region->name, name) == 0) return region;
		region = region->next;
	}
	return 0;
}
