#ifndef _FLRL_DOWNLOADER_H_
#define _FLRL_DOWNLOADER_H_

#include "fs.h"

class Downloader {
public:
	bool download(const char *url, void progress(void *data, int bytes, int total), void *progress_data, FILE *file, char **buffer, int *downloaded);
	Downloader();
	~Downloader();
	void abort();
	bool isAborted();
private:
	bool aborted;
	class DownloaderPrivate;
	DownloaderPrivate *priv;
};

#endif
