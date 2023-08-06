#include "launcher.h"
#include "rexuiz.h"
#include "sign.h"
#include "unzip.h"
#include "translation_data.h"
#include "process.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
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
	char lang[3];
	#ifdef _WIN32
	TCHAR loc[80];
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, loc, 80*sizeof(TCHAR));
	printf("locale=%s\n", loc);
	lang[0] = tolower(loc[0]);
	lang[1] = tolower(loc[1]);
	#else
	#ifdef __APPLE__
	CFLocaleRef locale = CFLocaleCopyCurrent();
	CFStringRef value = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleLanguageCode);
	const char *locale_language = CFStringGetCStringPtr(value, kCFStringEncodingUTF8);
	lang[0] = locale_language[0];
	lang[1] = locale_language[1];
	CFRelease(locale);
	CFRelease(value);
	#else
	lang[0] = 0;
	const char *langenv = getenv("LANG");
	if (langenv) memcpy(lang, langenv, 2);
	#endif
	#endif
	lang[2] = 0;
	translation = new Translation((const char *)translation_data, translation_data_len, lang);
}

Launcher::~Launcher() {
	if (installPath)
		delete[] installPath;

	if (indexPath)
		delete[] indexPath;

	delete gui;
	delete translation;
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
	gui->setProgress(0);
	gui->setProgressSecondary(0);
	Index updateIndex;
	newIndex.compare(&currentIndex, &updateIndex);
	if (!updateIndex.itemsCount) {
		updateEmpty = true;
		newIndex.saveToFile(indexPath);
		return;
	}
	updateHappened = true;
	printf("Files to update: %i\n", updateIndex.itemsCount);
	gui->setInfo(translation->translate("Updating..."));
	gui->setProgress(0);
	gui->setProgressSecondary(0);
	gui->setInfoSecondary(translation->translate("Validating..."));
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
	snprintf(question, sizeof(question), translation->translate("Update size is %i.%iMiB, install it?"), totalSize / (1024 * 1024), (totalSize % (1024 * 1024)) / (103 * 1024));
	if (updateIndex.itemsCount && gui->askYesNo(question)) {
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
					gui->setInfoSecondary(translation->translate("Downloading..."));
					if (downloader.download(linkgz, launcher_downloader_progress, &d, f, NULL, NULL)) {
						fclose(f);
						f = NULL;
						struct launcher_uncompress_progress_data ud = {.gui = gui};
						gui->setInfoSecondary(translation->translate("Extracting..."));
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
			gui->setInfoSecondary(translation->translate("Downloading..."));
			if (f) fclose(f);
			if (!(f = FS::open(pathTmp, "wb"))) {
				char message[1024];
				snprintf(message, sizeof(message), "%s "
						#ifdef FS_CHAR_IS_16BIT
						"%ls"
						#else
						"%s"
						#endif
						, translation->translate("Cannot open file:")
						, pathTmp);
				error(message);
				goto finish;
			}
			if (downloader.download(link, launcher_downloader_progress, &d, f, NULL, NULL)) {
				fclose(f);
				f = NULL;
				gui->setInfoSecondary(translation->translate("Validating..."));
				if (FS::size(pathTmp) == updateIndex.items[i].size) {
					if (Sign::checkFileHash(pathTmp, updateIndex.items[i].hash, 32)) {
						if (!FS::move(pathTmp, path)) {
							error(translation->translate("File saving failed"));
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
				error(translation->translate("Update failed"));
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
	FSChar *repoPath = FS::pathConcat(installPath, "launcherrepos.txt");
	FILE *f;
	char *repoLine = NULL;
	int repoLineSize = 0;
	char **reposFromFile = NULL;
	int reposFromFileSize = 0;
	if ((f = FS::open(repoPath, "rb"))) {
		printf("Loading repos from launcherrepos.txt\n");
		while (FS::readLine(&repoLine, &repoLineSize, f) >= 0) {
			if (!repoLine[0]) continue;
			char **reposFromFileNew = new char *[reposFromFileSize + 2];
			memcpy(reposFromFileNew, reposFromFile, sizeof(char *) * (reposFromFileSize));
			if (reposFromFile) delete[] reposFromFile;
			reposFromFile = reposFromFileNew;
			reposFromFile[reposFromFileSize] = repoLine;
			reposFromFileSize++;
			repoLineSize = 0;
			repoLine = NULL;
		}
		if (reposFromFile) {
			reposFromFile[reposFromFileSize + 1] = NULL;
			repos = (const char **)reposFromFile;
		}
		if (repoLine)
			delete[] repoLine;

		fclose(f);
	}
	delete[] repoPath;
	int n = 0;
	for (p = repos; *p; p++) n++;
	gui->setInfo(translation->translate("Check repository..."));
	for (p = repos; *p && !repo; p++) {
		if (aborted) break;
		gui->setProgress((p - repos) * 100 / n);
		printf("checking repo: %s\n", *p);
		repoCheck = new char[strlen(*p) + 16];
		sprintf(repoCheck, "%s/%s", *p, "index.lst");
		gui->setInfoSecondary(translation->translate("Downloading..."));
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
	delete reposFromFile;
	gui->setProgress(100);
	if (repo && buffer) {
		printf("repo found: %s\n", repo);
		newIndex.load(buffer);
		delete[] buffer;
	} else
		printf("repo not found\n");
}

void launcher_execute_pipe_callback(long int fd, void *data) {
	GUI *gui = (GUI *)data;
	char buffer[1024];
	int n;
#ifdef _WIN32
	if ((n = _read(fd, buffer, sizeof(buffer))) > 0) {
#else
	if ((n = read(fd, buffer, sizeof(buffer))) > 0) {
#endif
		fwrite(buffer, 1, n, stdout);
	} else {
		gui->hide();
	}
}

void Launcher::execute() {
	int pipe_fileno = -1;
	gui->setInfo(translation->translate("Running"));
	gui->setInfoSecondary("");
	FSChar *executablePath = NULL;
#ifdef _WIN32
	void *process = NULL;
#else
	int pipes[2];
	pipes[0] = -1;
	pipes[1] = -1;
#endif
	if (aborted) return;
	if (updateHappened && !updateEmpty && Rexuiz::presentsInDirectory(installPath)) {
		if (!updateFailed) {
			if (installRequired) {
				if (!gui->askYesNo(translation->translate("Install finished. Run game?"))) return;
			} else {
				if (!gui->askYesNo(translation->translate("Update finished. Run game?"))) return;
			}
		} else {
			if (installRequired) {
				if (!gui->askYesNo(translation->translate("Install failed. Try run game anyway?"))) return;
			} else {
				if (!gui->askYesNo(translation->translate("Update failed. Try run game anyway?"))) return;
			}
		}
	}
	executablePath = FS::pathConcat(installPath, Rexuiz::binary());
#ifdef _WIN32
	int popenStringLength = 64;
	for (FSChar *c = executablePath; *c; c++) {
		popenStringLength++;
		if (*c == L' ') popenStringLength += 2;
		else if (*c == L'"') popenStringLength += 1;
	}
	const FSChar *args = GetCommandLineW();
	if (*args)
		popenStringLength += wcslen(args) + 1;

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
	if (*args) {
		*c2 = L' ';
		c2++;
		wcscpy(c2, args);
	} else
		*c2 = 0;

	swprintf(&c2[wcslen(c2)], 64, L" +set rexuizlauncher %li", (long int)version);
	if (!(process = Process::open(popenString))) goto finish;
	pipe_fileno = Process::fd(process);
#else
	int pid = 0, status;
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
		pipes[1] = -1;
	} else {
		char *argv2[argc + 4];
		char versionString[16];
		snprintf(versionString, sizeof(versionString), "%li", (long int)version);
		close(pipes[0]);
		close(1);
		dup2(pipes[1], 1);
		memcpy(&argv2[1], &argv[1], sizeof(char *) * (argc - 1));
		argv2[argc] = (char*)"+set";
		argv2[argc + 1] = (char*)"rexuizlauncher";
		argv2[argc + 2] = versionString;
		argv2[argc + 3] = NULL;
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
	if (process)
		Process::close(process);
#else
	if (pid > 0)
		waitpid(pid, &status, 0);

	if (pipes[0] >= 0)
		close(pipes[0]);

	if (pipes[1] >= 0)
		close(pipes[1]);
#endif
	if (executablePath)
		delete[] executablePath;
}

bool Launcher::checkNewVersion() {
	FSChar *path = FS::pathConcat(installPath, "launcherupdate.txt");
	FILE *file = FS::open(path, "rb");
	int lineLength = 0;
	char *line = NULL;
	long int version;
	bool r = false;
	if (!file) goto finish;
	if (FS::readLine(&line, &lineLength, file) < 0) goto finish;
	version = atol(line);
	if (version <= Launcher::version) goto finish;
	if (FS::readLine(&line, &lineLength, file) < 0) goto finish;
	if (!strchr(line, '\'') && !strchr(line, '"') && !strchr(line, '\\') && gui->askYesNo(translation->translate("New version of RexuizLauncher available. Open download page?"))) {
		char cmd[strlen(line) + 128];
		#ifdef __APPLE__
		sprintf(cmd, "nohup open -n '%s' > /dev/null 2> /dev/null &", line);
		#else
		#ifdef _WIN32
		sprintf(cmd, "start \"\" \"%s\"", line);
		#else
		sprintf(cmd, "nohup xdg-open '%s' > /dev/null 2> /dev/null &", line);
		#endif
		#endif
		system(cmd);
		r = true;
	}
finish:
	if (file) fclose(file);
	delete[] path;
	return r;
}

int Launcher::run() {
	int r = 1;
	long long int epoch, epochExit;
	gui->show();
	gui->setInfo(translation->translate("Preparing"));
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
	if (!installPath || !installPath[0] || !Rexuiz::presentsInDirectory(installPath)) {
		installPath = gui->selectDirectory(translation->translate("Please select install location"), startLocation);
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

	if (checkNewVersion()) goto finish;
	if (installRequired && updateFailed) goto finish;
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
		printf("Exited after %li seconds, next update forced\n", (long int)(epochExit - epoch));
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
