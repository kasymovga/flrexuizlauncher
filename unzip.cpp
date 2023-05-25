#include "unzip.h"
#include "fs.h"
#include <stdio.h>
#include <zlib.h>
#ifdef _WIN32
#include <io.h>
#endif

bool UnZip::uncompressFile(const FSChar *path, const FSChar *extractPath, void progress(void *data, int bytes, int total), void *progress_data) {
	FILE *f = NULL;
	FILE *fOut = NULL;
	unsigned char buffer[1024];
	int fd = -1;
	int readed;
	gzFile gz = NULL;
	bool r = false;
	int size = FS::size(path);
	f = FS::open(path, "rb");
	if (!f) goto finish;
	#ifdef _WIN32
	fd = _dup(fileno(f));
	#else
	fd = dup(fileno(f));
	#endif
	if (fd < 0) goto finish;
	gz = gzdopen(fd, "rb");
	if (!gz) goto finish;
	fd = -1;
	fOut = FS::open(extractPath, "wb");
	if (!fOut) goto finish;
	while ((readed = gzread(gz, buffer, sizeof(buffer))) > 0) {
		progress(progress_data, gztell(gz), size);
		fwrite(buffer, readed, 1, fOut);
	}
	if (!gzeof(gz)) goto finish;
	r = true;
finish:
	if (f) fclose(f);
	if (fOut) fclose(fOut);
	if (fd >= 0)
	if (gz) gzclose(gz);
	#ifdef _WIN32
		_close(fd);
	#else
		close(fd);
	#endif

	if (!r) FS::remove(extractPath);
	return r;
}
