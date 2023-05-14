#include "fs.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <libloaderapi.h>
#include <windows.h>
#include <shlwapi.h>
#include <io.h>
#else
#include <sys/stat.h>
#endif

char* FS::toUTF8(const FSChar *path) {
#ifdef FS_CHAR_IS_8BIT
	char *r = new char[strlen(path) + 1];
	strcpy(r, path);
	return r;
#else
	int n = wcstombs(NULL, path, 0) + 1;
	char *r = new char[n];
	wcstombs(r, path, n);
	return r;
#endif
}

FSChar* FS::fromUTF8(const char *u8) {
#ifdef FS_CHAR_IS_8BIT
	FSChar *r = new FSChar[strlen(u8) + 1];
	strcpy(r, u8);
#else
	int n = strlen(u8);
	FSChar *r = new FSChar[n + 1];
	n = mbstowcs(r, u8, n);
	r[n] = 0;
#endif
	for (FSChar *c = r; *c; ++c) {
		if (*c == '/') *c = FS_DELIMETER;
	}
	return r;
}

FSChar* FS::getBinaryLocation(const char *u8) {
	FSChar *r;
#ifdef _WIN32
	int s = 1024;
	int n;
	for (;;) {
		r = new FSChar[s];
		n = GetModuleFileNameW(NULL, r, s);
		if (!n) return 0;
		if (n < s)
			break;

		s *= 2;
		delete[] r;
	}
	return r;
#else
	char *tmp = realpath(u8, NULL);
	if (!tmp) return NULL;
	r = new FSChar[
			#ifdef FS_CHAR_IS_16BIT
			wcslen
			#else
			strlen
			#endif
			(tmp)
			];
	strcpy(r, tmp);
	free(tmp);
	return r;
#endif
}

void FS::stripToParent(FSChar* path) {
	FSChar *last_delimeter =
#ifdef FS_CHAR_IS_8BIT
			strrchr
#else
			wcsrchr
#endif
			(path, FS_DELIMETER);
	if (last_delimeter) {
		last_delimeter[0] = 0;
	} else {
		path[0] = 0;
	}
}

FILE *FS::open(const FSChar* path, const char *mode) {
#ifdef FS_CHAR_IS_8BIT
	return fopen(path, mode);
#else
	int n = strlen(mode);
	wchar_t wmode[n + 1];
	mbstowcs(wmode, mode, n + 1);
	return _wfopen(path, wmode);
#endif
}

FSChar *FS::concat(const FSChar *path1, const FSChar *path2) {
#ifdef FS_CHAR_IS_8BIT
	int len = FS::length(path1) + FS::length(path2) + 1;
	FSChar *out = new FSChar[len];
	snprintf(out, len, "%s%s", path1, path2);
#else
	int len = FS::length(path1) + FS::length(path2) + 1;
	FSChar *out = new FSChar[len];
	swprintf(out, len, L"%s%s", path1, path2);
#endif
	return out;
}

FSChar *FS::pathConcat(const FSChar *path1, const FSChar *path2) {
#ifdef FS_CHAR_IS_8BIT
	int len = FS::length(path1) + FS::length(path2) + 2;
	FSChar *out = new FSChar[len];
	if (path1[FS::length(path1) - 1] == FS_DELIMETER)
		snprintf(out, len, "%s%s", path1, path2);
	else
		snprintf(out, len, "%s%s%s", path1, FS_DELIMETER_STRING, path2);
#else
	int len = FS::length(path1) + FS::length(path2) + 2;
	FSChar *out = new FSChar[len];
	if (path1[FS::length(path1) - 1] == FS_DELIMETER)
		swprintf(out, len, L"%s%s", path1, path2);
	else
		swprintf(out, len, L"%s%s%s", path1, FS_DELIMETER_STRING, path2);
#endif
	return out;
}

#ifdef FS_CHAR_IS_16BIT
FSChar *FS::pathConcat(const FSChar *path1, const char *path2) {
	FSChar *path2fs = FS::fromUTF8(path2);
	FSChar *r = FS::pathConcat(path1, path2fs);
	delete[] path2fs;
	return r;
}

FSChar *FS::concat(const FSChar *path1, const char *path2) {
	FSChar *path2fs = FS::fromUTF8(path2);
	FSChar *r = FS::concat(path1, path2fs);
	delete[] path2fs;
	return r;
}
#endif

bool FS::directoryExists(const FSChar *path) {
#ifdef FS_CHAR_IS_8BIT
	struct stat s;
	stat(path, &s);
	return S_ISDIR(s.st_mode);
#else
	return PathIsDirectoryW(path);
#endif
}

bool FS::directoryMake(const FSChar *path) {
	int n = FS::length(path);
#ifdef _WIN32
	//drive, no need to create
	if (!wcschr(path, FS_DELIMETER)) {
		FSChar *colon = wcschr(path, L':');
		if (*colon && !colon[1])
			return true;
	}
#endif
	if (FS::directoryExists(path))
		return true;

	FSChar pathCopy[n + 1];
	FS::copy(pathCopy, path, n);
	pathCopy[n] = 0;
	FS::stripToParent(pathCopy);
	n = FS::length(pathCopy) - 1;
	while (n && pathCopy[n] == FS_DELIMETER) {
		pathCopy[n] = 0;
		--n;
	}
	if (pathCopy[0])
		if (!FS::directoryMake(pathCopy)) {
			return false;
		}
#ifdef FS_CHAR_IS_8BIT
	return !mkdir(path, 0755);
#else
	return CreateDirectoryW(path, NULL);
#endif
}

bool FS::directoryMakeFor(const FSChar *path) {
	int n = FS::length(path);
	FSChar pathCopy[n + 1];
	FS::copy(pathCopy, path, n);
	pathCopy[n] = 0;
	FS::stripToParent(pathCopy);
	if (pathCopy[0])
		return FS::directoryMake(pathCopy);

	return true;
}

void FS::copy(FSChar *dst, const FSChar *src, int n) {
#ifdef FS_CHAR_IS_8BIT
	strncpy(dst, src, n);
#else
	wcsncpy(dst, src, n);
#endif
}

int FS::length(const FSChar *path) {
#ifdef FS_CHAR_IS_8BIT
	return strlen(path);
#else
	return wcslen(path);
#endif
}

FSChar *FS::duplicate(const FSChar *path) {
	int n = FS::length(path);
	FSChar *duplicat = new FSChar[n + 1];
	FS::copy(duplicat, path, n);
	duplicat[n] = 0;
	return duplicat;
}

int FS::size(const FSChar *path) {
	int r;
	FILE *file = FS::open(path, "r");
	if (!file) return -1;
	fseek(file, 0, SEEK_END);
	r = ftell(file);
	fseek(file, 0, SEEK_SET);
	fclose(file);
	return r;
}

bool FS::move(const FSChar *source, const FSChar *destination) {
#ifdef FS_CHAR_IS_8BIT
	return !rename(source, destination);
#else
	return !_wrename(source, destination);
#endif
}

bool FS::remove(const FSChar *path) {
#ifdef FS_CHAR_IS_8BIT
	return !::remove(path);
#else
	return !_wremove(path);
#endif
}
