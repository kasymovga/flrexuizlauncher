#include "translation.h"
#include <string.h>
#include <stdio.h>

class Translation::Link {
public:
	const char *key;
	const char *translation;
	Translation::Link *next;
};

static int translation_hash(const char *str, int hash_size) {
	int hash = 0;
	for (const char *c = str; *c; c++) {
		hash = (hash + *c + 1) % hash_size;
	}
	return hash;
}

Translation::Translation(const char *translationData, int translationDataLength, const char *lang) {
	const char *c = translationData;
	const char *colon, *end;
	char *key = NULL;
	char *translation;
	Link *link;
	Link *chain = NULL;
	int translations = 0;
	int hash;
	while (*c && c < translationData + translationDataLength) {
		colon = strchr(c, ':');
		end = strchr(c, '\n');
		if (!end) end = &translationData[translationDataLength];
		if (colon && colon < end) {
			if (colon == c) {
				if (key) delete[] key;
				key = new char[end - colon];
				memcpy(key, &colon[1], end - colon - 1);
				key[end - colon - 1] = 0;
			} else if (key) {
				if (!strncmp(c, lang, colon - c)) {
					link = new Link;
					link->key = key;
					key = NULL;
					translation = new char[end - colon];
					memcpy(translation, &colon[1], end - colon - 1);
					translation[end - colon - 1] = 0;
					link->translation = translation;
					link->next = chain;
					chain = link;
					translations++;
				}
			}
		}
		if (!*end) break;
		c = &end[1];
	}
	hashTableSize = translations / 16;
	if (hashTableSize < 16) hashTableSize = 16;
	if (hashTableSize > 256) hashTableSize = 256;
	hashTable = new Link*[hashTableSize];
	for (int i = 0; i < hashTableSize; i++) {
		hashTable[i] = NULL;
	}
	while ((link = chain)) {
		chain = link->next;
		hash = translation_hash(link->key, hashTableSize);
		link->next = hashTable[hash];
		hashTable[hash] = link;
	}
	//for (int i = 0; i < hashTableSize; i++) {
	//	chain = hashTable[i];
	//	for (link = chain; link; link = link->next) {
	//		printf("%i '%s' -> '%s'\n", i, link->key, link->translation);
	//	}
	//}
}

Translation::~Translation() {
	Link *link, *chain;
	for (int i = 0; i < hashTableSize; i++) {
		chain = hashTable[i];
		while ((link = chain)) {
			chain = link->next;
			delete[] link->key;
			delete[] link->translation;
			delete link;
		}
	}
	delete[] hashTable;
}

const char *Translation::translate(const char *text) {
	int hash = translation_hash(text, hashTableSize);
	Link *link = hashTable[hash];
	for (; link; link = link->next) {
		if (!strcmp(link->key, text))
			return link->translation;
	}
	return text;
}
