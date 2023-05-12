#ifndef _FLRL_DOWNLOADER_H_
#define _FLRL_DOWNLOADER_H_

#include "fs.h"

class Downloader {
public:
	bool download(const char *url, void progress(void *data, int bytes), void *progress_data, FILE *file, char **buffer, int *downloaded);
	Downloader();
	~Downloader();
private:
	class DownloaderPrivate;
	DownloaderPrivate *priv;
};

#endif
