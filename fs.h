#ifndef _FLRL_FS_H_
#define _FLRL_FS_H_

#ifdef _WIN32
#define FS_CHAR_IS_16BIT
#define FS_DELIMETER L'\\'
#define FS_DELIMETER_STRING L"\\"
typedef wchar_t FSChar;
#else
#define FS_CHAR_IS_8BIT
#define FS_DELIMETER '/'
#define FS_DELIMETER_STRING "/"
typedef char FSChar;
#endif
#include <stdio.h>
class FS {
public:
	static char *toUTF8(const FSChar *path);
	static FSChar *fromUTF8(const char *u8);
	static FSChar *getBinaryLocation(const char *path);
	static void stripToParent(FSChar *path);
	static FSChar *concat(const FSChar *path1, const FSChar *path2);
	static FSChar *pathConcat(const FSChar *path1, const FSChar *path2);
	#ifdef FS_CHAR_IS_16BIT
	static FSChar *pathConcat(const FSChar *path1, const char *path2);
	static FSChar *concat(const FSChar *path1, const char *path2);
	#endif
	static FILE *open(const FSChar *path, const char *mode);
	static bool directoryMake(const FSChar *path);
	static bool directoryMakeFor(const FSChar *path);
	static int length(const FSChar *path);
	static void copy(FSChar *dst, const FSChar *src, int n);
	static bool directoryExists(const FSChar *path);
	static FSChar *duplicate(const FSChar *path);
	static int size(const FSChar *path);
	static bool move(const FSChar *source, const FSChar *destination);
	static bool remove(const FSChar *path);
	static int readLine(char **line, int *lineSize, FILE *file);
};

#endif
