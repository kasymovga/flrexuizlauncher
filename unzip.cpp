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

bool UnZip::uncompressFile(const FSChar *path, const FSChar *extractPath, void progress(void *data, int bytes, int total), void *progress_data) {
	mz_stream_s stream;
	FILE *f;
	FILE *fOut;
	unsigned char bufferIn[1024], bufferOut[1024];
	bool mz_inflateInitialized = false;
	bool r = false;
	int readed;
	int mzresult;
	int totalSize;
	int totalReaded = 0;
	memset(&stream, 0, sizeof(stream));
	totalSize = FS::size(path) - 10;
	if (mz_inflateInit2(&stream, -MZ_DEFAULT_WINDOW_BITS) != MZ_OK) goto finish;
	mz_inflateInitialized = true;
	if (!(f = FS::open(path, "rb"))) goto finish;
	if (fseek(f, 10, SEEK_SET)) goto finish;
	if (!(fOut = FS::open(extractPath, "wb"))) goto finish;
	stream.next_in = bufferIn;
	stream.avail_in = 0;
	for (;;) {
		//printf("decompress cycle\n");
		stream.next_out = bufferOut;
		stream.avail_out = sizeof(bufferOut);
		if (stream.avail_in && bufferIn != stream.next_in) {
			memmove(bufferIn, stream.next_in, stream.avail_in);
		}
		stream.next_in = bufferIn;
		if (stream.avail_in < sizeof(bufferIn)) {
			readed = fread(bufferIn + stream.avail_in, 1, sizeof(bufferIn) - stream.avail_in, f);
			if (readed <= 0) {
				if (feof(f))
					mzresult = mz_inflate(&stream, MZ_NO_FLUSH);
				else {
					printf("uncompress: read failed\n");
					goto finish;
				}
			} else {
				totalReaded += readed;
				stream.avail_in += readed;
				mzresult = mz_inflate(&stream, MZ_NO_FLUSH);
			}
		} else
			mzresult = mz_inflate(&stream, MZ_NO_FLUSH);

		if (progress)
			progress(progress_data, totalReaded, totalSize);

		if (stream.avail_out < sizeof(bufferOut)) {
			if (fwrite(bufferOut, 1, sizeof(bufferOut) - stream.avail_out, fOut) < 0) {
				printf("uncompress: write failed\n");
				goto finish;
			}
		}
		if (mzresult == MZ_STREAM_END) {
			printf("mz_inflate: stream finished\n");
			break;
		}
		if (mzresult != MZ_OK) {
			if (mzresult == MZ_DATA_ERROR)
				printf("mz_inflate: wrong data\n");
			else if (mzresult == MZ_BUF_ERROR)
				printf("mz_inflate: buffer error\n");

			goto finish;
		}
	};
	r = true;
finish:
	if (mz_inflateInitialized) mz_inflateEnd(&stream);
	if (f) fclose(f);
	if (fOut) fclose(fOut);
	if (!r) FS::remove(extractPath);
	return r;
}

bool UnZip::extractFile(const FSChar *path, const char *zipPath, const FSChar *extractPath) {
	bool r = false;
	bool zip_archive_opened = false;
	FILE *file = FS::open(path, "rb");
	FILE *extractFile = FS::open(extractPath, "rb");
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
