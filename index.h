#ifndef _FLRL_INDEX_H_
#define _FLRL_INDEX_H_

#include "fs.h"

class Index {
public:
	Index();
	~Index();
	struct Item {
		const char *path;
		int size;
		char hash[33];
		const char *zipSource;
		const char *zipPath;
		int zipSize;
		int zipHash;
	};
	struct Item *items;
	int itemsCount;
	void load(char *data);
	bool loadFromFile(FSChar *path);
	bool saveToFile(const FSChar *path);
	void removeAll();
	void remove(int i);
	void compare(Index *in, Index *out);
private:
	int itemsSize;
	void allocate();
	bool itemAdd(const char *line);
	void itemAdd(const Index::Item *item);
	void sort();
};

#endif
