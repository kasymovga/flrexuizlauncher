#include "launcher.h"
#include "rexuiz.h"
#include "sign.h"
#include "unzip.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <processthreadsapi.h>
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#endif

Launcher::Launcher(int argc, char **argv) {
	this->argc = argc;
	this->argv = argv;
	installPath = NULL;
	installRequired = false;
	repo = NULL;
	indexPath = NULL;
	aborted = false;
	gui = new GUI(this);
	updateFailed = false;
	updateHappened = false;
	updateEmpty = false;
}

Launcher::~Launcher() {
	if (installPath)
		delete[] installPath;

	if (indexPath)
		delete[] indexPath;

	delete gui;
}

void Launcher::abort() {
	aborted = true;
	downloader.abort();
}

void Launcher::error(const char *message) {
	if (!aborted)
		gui->error(message);
}

void Launcher::validate() {
	bool validated = true;
	for (int i = 0; i < currentIndex.itemsCount && validated; ++i) {
		FSChar *path = FS::pathConcat(installPath, currentIndex.items[i].path);
		if (FS::size(path) != currentIndex.items[i].size) validated = false;
		delete[] path;
	}
	if (!validated) {
		printf("Current index is not valid\n");
		currentIndex.removeAll();
	}
}

struct launcher_downloader_progress_data {
	int expectedSize;
	GUI *gui;
	Downloader *downloader;
};

static void launcher_downloader_progress(void *data, int bytes, int total) {
	struct launcher_downloader_progress_data *d = (struct launcher_downloader_progress_data *)data;
	if (d->expectedSize && total && total != d->expectedSize)
		d->downloader->abort();

	if (d->expectedSize) {
		d->gui->setProgressSecondary(((long long int)bytes * 100) / d->expectedSize);
	} else if (total) {
		d->gui->setProgressSecondary(((long long int)bytes * 100) / total);
	} else {
		d->gui->frame();
	}
}

struct launcher_uncompress_progress_data {
	GUI *gui;
};

static void launcher_uncompress_progress(void *data, int bytes, int total) {
	struct launcher_uncompress_progress_data *d = (struct launcher_uncompress_progress_data *)data;
	if (total)
		d->gui->setProgressSecondary(((long long int)bytes * 100) / total);
}

void Launcher::update() {
	int totalSize = 0;;
	if (aborted) return;
	repoSearch();
	if (!repo) { //no working repo
		if (installRequired)
			error("Cannot get installation data");

		return;
	}
	const char *ext;
	FILE *f = NULL;
	FSChar *path = NULL, *pathTmp = NULL;
	for (int i = 0; i < newIndex.itemsCount; ++i) {
#ifndef _WIN32
		if ((ext = strrchr(newIndex.items[i].path, '.'))) {
			if (!strcmp(ext, ".exe") || !strcmp(ext, ".dll") || !strcmp(ext, ".cmd")) {
				newIndex.remove(i);
				--i;
				continue;
			}
		}
#endif
#ifndef __APPLE__
		if (strstr(newIndex.items[i].path, ".app/")) {
			newIndex.remove(i);
			--i;
			continue;
		}
#endif
#ifndef __linux__
		if (strstr(newIndex.items[i].path, "linux")) {
			newIndex.remove(i);
			--i;
			continue;
		}
#endif
	}
	updateHappened = true;
	gui->setProgress(0);
	gui->setProgressSecondary(0);
	Index updateIndex;
	newIndex.compare(&currentIndex, &updateIndex);
	if (!updateIndex.itemsCount) {
		updateEmpty = true;
		newIndex.saveToFile(indexPath);
		return;
	}
	printf("Files to update: %i\n", updateIndex.itemsCount);
	gui->setInfo("Updating...");
	gui->setProgress(0);
	gui->setProgressSecondary(0);
	gui->setInfoSecondary("Validating...");
	for (int i = 0; i < updateIndex.itemsCount; ++i) {
		FSChar *path = FS::pathConcat(installPath, updateIndex.items[i].path);
		gui->setProgress(i * 100 / updateIndex.itemsCount);
		if (FS::size(path) == updateIndex.items[i].size && Sign::checkFileHash(path, updateIndex.items[i].hash, 32)) {
			updateIndex.remove(i);
			--i;
			continue;
		}
		delete[] path;
		path = NULL;
	}
	gui->setProgress(100);
	for (int i = 0; i < updateIndex.itemsCount; ++i) {
		totalSize += updateIndex.items[i].size;
	}
	char question[256];
	updateFailed = true;
	snprintf(question, sizeof(question), "Update size is %i.%iMiB, install it?", totalSize / (1024 * 1024), (totalSize % (1024 * 1024)) / (103 * 1024));
	if (gui->askYesNo(question)) {
		for (int i = 0; i < updateIndex.itemsCount; ++i) {
			printf("Downloading %s\n", updateIndex.items[i].path);
			gui->setProgress(i * 100 / updateIndex.itemsCount);
			if (path) delete[] path;
			if (pathTmp) delete[] pathTmp;
			path = FS::pathConcat(installPath, updateIndex.items[i].path);
			pathTmp = FS::concat(path, ".tmp");
			FS::directoryMakeFor(pathTmp);
			char link[strlen(repo) + strlen(updateIndex.items[i].path) + 16];
			sprintf(link, "%s/%s", repo, updateIndex.items[i].path);
			gui->setProgress(i * 100 / updateIndex.itemsCount);
			if ((ext = strrchr(link, '.'))) ext++;
			if (ext && strcmp(ext, "pk3")) {
				bool success = false;
				char linkgz[strlen(link)];
				sprintf(linkgz, "%s.gz", link);
				FSChar *pathTmpGz = FS::concat(pathTmp, ".gz");
				if ((f = FS::open(pathTmpGz, "wb"))) {
					struct launcher_downloader_progress_data d = {.expectedSize = 0, .gui = gui, .downloader = &downloader};
					gui->setInfoSecondary("Downloading...");
					if (downloader.download(linkgz, launcher_downloader_progress, &d, f, NULL, NULL)) {
						fclose(f);
						f = NULL;
						struct launcher_uncompress_progress_data ud = {.gui = gui};
						gui->setInfoSecondary("Extracting...");
						if (UnZip::uncompressFile(pathTmpGz, pathTmp, launcher_uncompress_progress, &ud) && FS::size(pathTmp) == updateIndex.items[i].size && Sign::checkFileHash(pathTmp, updateIndex.items[i].hash, 32)) {
							FS::move(pathTmp, path);
							success = true;
						} else {
							printf("Uncompressing failed\n");
						}
					}
					if (f) fclose(f);
					f = NULL;
					FS::remove(pathTmpGz);
				}
				delete[] pathTmpGz;
				if (success) continue;
			}
			struct launcher_downloader_progress_data d = {.expectedSize = updateIndex.items[i].size, .gui = gui, .downloader = &downloader};
			gui->setInfoSecondary("Downloading...");
			if (f) fclose(f);
			if (!(f = FS::open(pathTmp, "wb"))) {
				char message[1024];
				snprintf(message, sizeof(message), "Cannot open file: "
						#ifdef FS_CHAR_IS_16BIT
						"%ls"
						#else
						"%s"
						#endif
						, pathTmp);
				error(message);
				goto finish;
			}
			if (downloader.download(link, launcher_downloader_progress, &d, f, NULL, NULL)) {
				fclose(f);
				f = NULL;
				gui->setInfoSecondary("Validating...");
				if (FS::size(pathTmp) == updateIndex.items[i].size) {
					if (Sign::checkFileHash(pathTmp, updateIndex.items[i].hash, 32)) {
						if (!FS::move(pathTmp, path)) {
							error("File saving failed");
							goto finish;
						}
					} else {
						error("Wrong checksum");
						goto finish;
					}
				} else {
					error("Wrong file size");
					goto finish;
				}
			} else {
				printf("Downloading of %s failed\n", link);
				error("Update failed");
				goto finish;
			}
		}
	}
	updateFailed = false;
finish:
	if (f) fclose(f);
	if (path) delete[] path;
	if (pathTmp) delete[] pathTmp;
	newIndex.saveToFile(indexPath);
	gui->setInfo("");
	gui->setInfoSecondary("");
	gui->setProgress(100);
	gui->setProgressSecondary(100);
	printf("update finished\n");
}

void Launcher::repoSearch() {
	char *buffer = NULL;
	int bufferLength = 0;
	char *bufferSig = NULL;
	int bufferSigLength = 0;
	char *repoCheck;
	const char **p;
	const char **repos = Rexuiz::repos();
	int n = 0;
	for (p = repos; *p; p++) n++;
	gui->setInfo("Check repository...");
	for (p = repos; *p && !repo; p++) {
		if (aborted) break;
		gui->setProgress((p - repos) * 100 / n);
		printf("checking repo: %s\n", *p);
		repoCheck = new char[strlen(*p) + 16];
		sprintf(repoCheck, "%s/%s", *p, "index.lst");
		gui->setInfoSecondary("Downloading...");
		struct launcher_downloader_progress_data d = {.expectedSize = 0, .gui = gui, .downloader = &downloader};
		if (downloader.download(repoCheck, launcher_downloader_progress, &d, NULL, &buffer, &bufferLength)) {
			gui->frame();
			sprintf(repoCheck, "%s/%s", *p, "index.lst.sig");
			if (downloader.download(repoCheck, launcher_downloader_progress, &d, NULL, &bufferSig, &bufferSigLength)) {
				if (Sign::verify(buffer, bufferLength, bufferSig, bufferSigLength)) {
					printf("sign verification successed\n");
					repo = *p;
				} else {
					printf("sign verification failed\n");
					delete[] buffer;
				}
				delete[] bufferSig;
			} else {
				delete[] buffer;
			}
		}
		delete[] repoCheck;
	}
	gui->setProgress(100);
	if (repo) {
		printf("repo found: %s\n", repo);
		newIndex.load(buffer);
		delete[] buffer;
	} else
		printf("repo not found\n");
}

void launcher_execute_pipe_callback(int fd, void *data) {
	GUI *gui = (GUI *)data;
#ifdef _WIN32
	char buffer[1024];
	int n;
	if ((n = _read(fd, buffer, sizeof(buffer)) - 1) >= 0) {
		fwrite(buffer, n, 1, stdout);
	} else
		gui->hide();
#else
	char buffer[1024];
	int n;
	if ((n = read(fd, buffer, sizeof(buffer)) - 1) >= 0) {
		fwrite(buffer, n, 1, stdout);
	} else
		gui->hide();
#endif
}

void Launcher::execute() {
	int pipe_fileno = -1;
	FSChar *executablePath = NULL;
#ifdef _WIN32
	FILE *pf = NULL;
#else
	int pipes[2];
	pipes[0] = -1;
#endif
	if (aborted) return;
	if (updateHappened && !updateEmpty) {
		if (!updateFailed) {
			if (installRequired) {
				if (!gui->askYesNo("Install finished. Run game?")) return;
			} else {
				if (!gui->askYesNo("Update finished. Run game?")) return;
			}
		} else if (Rexuiz::presentsInDirectory(installPath)) {
			if (installRequired) {
				if (!gui->askYesNo("Install failed. Try run game anyway?")) return;
			} else {
				if (!gui->askYesNo("Update failed. Try run game anyway?")) return;
			}
		}
	}
	executablePath = FS::pathConcat(installPath, Rexuiz::binary());
#ifdef _WIN32
	int popenStringLength = 0;
	for (FSChar *c = executablePath; *c; c++) {
		popenStringLength++;
		if (*c == L' ') popenStringLength += 2;
		else if (*c == L'"') popenStringLength += 1;
	}
	FSChar popenString[popenStringLength + 1];
	FSChar *c2 = popenString;
	for (FSChar *c = executablePath; *c; ++c, ++c2) {
		if (*c == L' ') {
			*c2 = L'"';
			c2++;
			*c2 = L' ';
			c2++;
			*c2 = L'"';
		} else if (*c == L'"') {
			*c2 = L'"';
			c2++;
			*c2 = L'"';
		} else
			*c2 = *c;
	}
	*c2 = 0;
	pf = _wpopen(popenString, L"rb");
	if (!pf) {
		gui->error("popen() failed");
		goto finish;
	}
	pipe_fileno = fileno(pf);
#else
	int pid, status;
	if (pipe(pipes)) {
		gui->error("pipe() failed");
		goto finish;;
	}
	pipe_fileno = pipes[0];
	if ((pid = fork())) {
		if (pid < 0) {
			gui->error("fork() failed");
			goto finish;
		}
		close(pipes[1]);
	} else {
		char *argv2[argc + 1];
		close(pipes[0]);
		close(1);
		dup2(pipes[1], 1);
		memcpy(&argv2[1], &argv[1], sizeof(char *) * (argc - 1));
		argv2[argc] = NULL;
		argv2[0] = executablePath;
		printf("Executing...\n");
		chmod(executablePath, 0755);
		execv(executablePath, argv2);
		printf("Exec of %s failed\n", executablePath);
		exit(1);
	}
#endif
	gui->watchfd(pipe_fileno, launcher_execute_pipe_callback, gui);
	while (gui->wait());
finish:
	if (pipe_fileno > 0)
		gui->unwatchfd(pipe_fileno);

#ifdef _WIN32
	if (pf) fclose(pf);
#else
	waitpid(pid, &status, 0);
	if (pipes[0] >= 0)
		close(pipes[0]);
#endif
	if (executablePath)
		delete[] executablePath;
}

int Launcher::run() {
	int r = 1;
	long long int epoch, epochExit;
	gui->show();
	gui->setInfo("Preparing");
	FSChar *startLocation = FS::getBinaryLocation(argv[0]);
	FS::stripToParent(startLocation);
	settings.load();
	if (Rexuiz::presentsInDirectory(startLocation)) {
		updater = true;
		installPath = FS::duplicate(startLocation);
	} else {
		updater = false;
		if (settings.installPath && settings.installPath[0]) {
			installPath = FS::duplicate(settings.installPath);
		}
	}
	if (!installPath || !installPath[0]) {
		installPath = gui->selectDirectory("Please select install location", startLocation);
	}
	if (!installPath || !installPath[0])
		goto finish;

	if (!Rexuiz::presentsInDirectory(installPath)) {
		installRequired = true;
	}
	settings.setInstallPath(installPath);
	indexPath = FS::pathConcat(installPath, "/index.lst");
	if (!currentIndex.loadFromFile(indexPath)) {
		installRequired = true;
	} else
		validate();

	#ifdef _WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	epoch = (((LONGLONG)ft.dwLowDateTime + ((LONGLONG)(ft.dwHighDateTime) << 32LL)) / 10000000);
	#else
	epoch = time(NULL);
	#endif
	if (installRequired || !currentIndex.itemsCount || epoch - settings.lastUpdate > 21600)
		update();

	execute();
	#ifdef _WIN32
	GetSystemTimeAsFileTime(&ft);
	epochExit = (((LONGLONG)ft.dwLowDateTime + ((LONGLONG)(ft.dwHighDateTime) << 32LL)) / 10000000);
	#else
	epochExit = time(NULL);
	#endif
	printf("Saving settings\n");
	if (epochExit - epoch > 30) {
		if (updateHappened) {
			if (updateFailed)
				settings.lastUpdate = 0;
			else
				settings.lastUpdate = epoch;
		}
	} else { //probably something wrong happened, force update next time
		printf("Exited after %lli seconds, next update forced\n", (long long int)(epochExit - epoch));
		settings.lastUpdate = 0;
	}
	settings.save();
	r = 0;
finish:
	gui->hide();
	if (startLocation)
		delete[] startLocation;

	return r;
}
