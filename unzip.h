#ifndef _FLRL_UNZIP_H_
#define _FLRL_UNZIP_H_

#include "fs.h"

class UnZip {
public:
	static bool extractFile(const FSChar *path, const char *zipPath, const FSChar *extractPath);
	static bool uncompressFile(const FSChar *path, const FSChar *extractPath, void progress(void *data, int bytes, int total), void *progress_data);
};

#endif // UNZIP_H
