#include "unzip.h"
#include "miniz.h"
#include "fs.h"
#include <stdio.h>

extern "C" {
	static size_t write_cb(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n) {
		FILE *file = (FILE *)pOpaque;
		return fwrite((const char*)pBuf, n, 1, file);
	}
}

bool UnZip::extractFile(const FSChar *path, const char *zipPath, const FSChar *extractPath) {
	bool r = false;
	bool zip_archive_opened = false;
	FILE *file = FS::open(path, "r");
	FILE *extractFile = FS::open(extractPath, "r");
	int size;
	if (!file || !extractFile) goto finish;
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (!(zip_archive_opened =
			mz_zip_reader_init_cfile(&zip_archive, file, size, 0)))
		goto finish;

	if (!mz_zip_reader_extract_file_to_callback(&zip_archive, zipPath, write_cb, (void *)&extractFile, 0))
		goto finish;

	r = true;
finish:
	if (zip_archive_opened)
		mz_zip_end(&zip_archive);

	if (extractFile)
		fclose(extractFile);

	if (file)
		fclose(file);

	return r;
}
