#include "downloader.h"

#include <string.h>
#include <curl/curl.h>

class Downloader::DownloaderPrivate {
public:
	CURL *curl;
};

extern "C" {
	struct downloader_progress_info {
		void (*progress)(void *data, int bytes, int total);
		void *data;
		FILE *file;
		char *buffer;
		int downloaded;
		int bufferSize;
		Downloader *downloader;
	};

	static int download_progress(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
		struct downloader_progress_info *i = (struct downloader_progress_info *)p;
		if (i->progress)
			i->progress(i->data, dlnow, dltotal);

		return 0;
	}

	static int download_write(void *data, size_t size, size_t nmemb, void *p) {
		struct downloader_progress_info *i = (struct downloader_progress_info *)p;
		int r = size * nmemb;
		if (i->downloader->isAborted())
			return -1;

		if (i->file) {
			r = fwrite(data, size, nmemb, i->file) * size;
		}
		if (i->buffer) {
			while (i->downloaded + r > i->bufferSize) {
				char *oldBuffer = i->buffer;
				i->bufferSize *= 2;
				i->buffer = new char[i->bufferSize];
				memcpy(i->buffer, oldBuffer, i->downloaded);
				delete[] oldBuffer;
			}
			memcpy(&i->buffer[i->downloaded], (char *)data, r);
		}
		i->downloaded += r;
		return r;
	}
}

bool Downloader::isAborted() {
	return this->aborted;
}

bool Downloader::download(const char *url, void progress(void *data, int bytes, int total), void *progress_data, FILE *file, char **buffer, int *downloaded) {
	downloader_progress_info i;
	CURLcode res;
	aborted = false;
	i.progress = progress;
	i.data = progress_data;
	i.file = file;
	i.downloaded = 0;
	i.downloader = this;
	if (buffer) {
		i.buffer = new char[1024];
		i.bufferSize = 1024;
	} else {
		i.buffer = NULL;
		i.bufferSize = 0;
	}
	curl_easy_reset(priv->curl);
	curl_easy_setopt(priv->curl, CURLOPT_URL, url);
	curl_easy_setopt(priv->curl, CURLOPT_USERAGENT, "FLRexuizLauncher");
	curl_easy_setopt(priv->curl, CURLOPT_XFERINFOFUNCTION, download_progress);
	curl_easy_setopt(priv->curl, CURLOPT_XFERINFODATA, (void *) &i);
	curl_easy_setopt(priv->curl, CURLOPT_WRITEFUNCTION, download_write);
	curl_easy_setopt(priv->curl, CURLOPT_WRITEDATA, (void *) &i);
	curl_easy_setopt(priv->curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(priv->curl, CURLOPT_SSL_VERIFYPEER, 0L);
	res = curl_easy_perform(priv->curl);
	long http_code = 0;
	if (res == CURLE_OK) {
		curl_easy_getinfo(priv->curl, CURLINFO_RESPONSE_CODE, &http_code);
	}
	if (buffer) {
		if (res == CURLE_OK && http_code == 200) {
			*buffer = i.buffer;
		} else {
			delete[] i.buffer;
			*buffer = NULL;
		}
	}
	if (downloaded)
		*downloaded = i.downloaded;

	return res == CURLE_OK && http_code == 200;
}

void Downloader::abort() {
	aborted = true;
}

Downloader::Downloader() {
	priv = new Downloader::DownloaderPrivate;
	priv->curl = curl_easy_init();
}

Downloader::~Downloader() {
	curl_easy_cleanup(priv->curl);
	delete priv;
}
