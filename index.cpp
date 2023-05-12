#include "index.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void Index::allocate() {
	if (itemsCount >= itemsSize) {
		if (!itemsSize) itemsSize = 8;
		itemsSize *= 2;
		Index::Item *newItems = new Index::Item[itemsSize];
		memcpy(newItems, items, sizeof(Index::Item) * itemsCount);
		if (items)
			delete[] items;

		items = newItems;
	}
}

bool Index::itemAdd(const char *line) {
	bool r;
	const char *p = line;
	const char *sep;
	char *path = NULL;
	char hash[32];
	int size = 0;
	allocate();
	if (!(sep = strchr(p, '|')))
		goto finish;

	if (sep - p != 32) {
		goto finish;
	}
	memcpy(hash, p, 32);
	for (int i = 0; i < 32; ++i) {
		hash[i] = tolower(hash[i]);
	}
	p = sep + 1;
	if (!(sep = strchr(p, '|'))) {
		goto finish;
	}
	size = atoi(p);
	p = sep + 1;
	if (!(sep = strchr(p, '|')))
		sep = &p[strlen(p)];

	path = new char[sep - p + 1];
	path[sep - p] = 0;
	memcpy(path, p, sep - p);
	items[itemsCount];
	memcpy(items[itemsCount].hash, hash, 32);
	items[itemsCount].hash[32] = 0;
	items[itemsCount].path = path;
	items[itemsCount].size = size;
	++itemsCount;
	r = true;
finish:
	if (!r) {
		if (path)
			delete[] path;
	}
	return r;
}

void Index::itemAdd(const Index::Item *item) {
	allocate();
	memcpy(items[itemsCount].hash, item->hash, 32);
	items[itemsCount].size = item->size;
	int pathLen = strlen(item->path);
	char *path = new char[pathLen + 1];
	strcpy(path, item->path);
	items[itemsCount].path = path;
	++itemsCount;
}

extern "C" {
	static int index_cmp(const void *p1, const void *p2) {
		const Index::Item *item1 = (Index::Item *)p1;
		const Index::Item *item2 = (Index::Item *)p2;
		return strcmp(item1->path, item2->path);
	}
}

void Index::sort() {
	if (itemsCount)
		qsort(items, itemsCount, sizeof(Index::Item), index_cmp);
}

void Index::load(char *data) {
	const char *nl;
	const char *p = data;
	while (*p) {
		nl = strchr(p, '\n');
		if (!*nl) nl = &p[strlen(p)];
		char line[nl - p + 1];
		memcpy(line, p, nl - p);
		line[nl - p] = 0;
		itemAdd(line);
		if (!*nl) break;
		p = nl + 1;
	}
}

bool Index::loadFromFile(FSChar *path) {
	bool r = false;
	FILE *file = NULL;
	char *line = NULL;
	size_t lineLength;
	if (!(file = FS::open(path, "r"))) goto finish;
	while (getline(&line, &lineLength, file) > 0) {
		if (lineLength < 1) continue;
		if (line[lineLength - 1] == '\n')
			line[lineLength - 1] = 0;

		itemAdd(line);
	}
	if (ferror(file)) goto finish;
	r = true;
finish:
	if (line) free(line);
	if (file) fclose(file);
	return r;
}

bool Index::saveToFile(const FSChar *path) {
	FILE *file = NULL;
	bool r = false;
	FSChar *tmpPath = FS::concat(path, ".tmp");
	if (!(file = FS::open(tmpPath, "w"))) goto finish;
	for (int i = 0; i < itemsCount; ++i) {
		if (fprintf(file, "%s|%i|%s\n", items[i].hash, items[i].size, items[i].path) < 0) goto finish;
	}
	FS::move(tmpPath, path);
	r = true;
finish:
	if (tmpPath) delete[] tmpPath;
	if (file) fclose(file);
	return r;
}

Index::Index() {
	items = NULL;
	itemsCount = 0;
	itemsSize = 0;
}

Index::~Index() {
	removeAll();
	if (items)
		delete[] items;
}

void Index::removeAll() {
	for (int i = 0; i < itemsCount; ++i) {
		delete[] items[i].path;
	}
	itemsCount = 0;
}

void Index::remove(int i) {
	delete[] items[i].path;
	if (i + 1 < itemsCount) {
		memmove(&items[i], &items[i + 1], sizeof(Index::Item) * (itemsCount - i - 1));
	}
	--itemsCount;
}

void Index::compare(Index *in, Index *out) {
	sort();
	in->sort();
	int i = 0, j = 0;
	int diff;
	while (i < itemsCount && j < in->itemsCount) {
		diff = strcmp(items[i].path, in->items[j].path);
		if (!diff) {
			if (items[i].size == in->items[j].size && !memcmp(items[i].hash, in->items[j].hash, 32)) {
				++i;
				++j;
			} else {
				out->itemAdd(&items[i]);
			}
		} else if (diff > 0) {
			++j;
		} else if (diff < 0) {
			out->itemAdd(&items[i]);
			++i;
		}
	}
	for (;i < itemsCount; ++i) {
		out->itemAdd(&items[i]);
	}
}
