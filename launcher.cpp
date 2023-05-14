#include "launcher.h"
#include "rexuiz.h"
#include "sign.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <processthreadsapi.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

Launcher::Launcher(int argc, char **argv) {
	this->argc = argc;
	this->argv = argv;
	installPath = NULL;
	installRequired = false;
	repo = NULL;
	indexPath = NULL;
}

Launcher::~Launcher() {
	if (installPath)
		delete[] installPath;

	if (indexPath)
		delete[] indexPath;
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
};

static void launcher_downloader_progress(void *data, int bytes) {
	struct launcher_downloader_progress_data *d = (struct launcher_downloader_progress_data *)data;
	if (d->expectedSize) {
		d->gui->setProgressSecondary(bytes * 100 / d->expectedSize);
	} else
		d->gui->frame();
}

void Launcher::update() {
	int totalSize = 0;;
	repoSearch();
	if (!repo) { //no working repo
		gui.error("Update not available");
		return;
	}
#ifndef _WIN32
	const char *ext;
#endif
	for (int i = 0; i < newIndex.itemsCount; ++i) {
#ifndef _WIN32
		if ((ext = strrchr(newIndex.items[i].path, '.'))) {
			if (!strcmp(ext, ".exe") || !strcmp(ext, ".dll")) {
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
	gui.setProgress(0);
	gui.setProgressSecondary(0);
	Index updateIndex;
	newIndex.compare(&currentIndex, &updateIndex);
	if (!updateIndex.itemsCount) return;
	printf("Files to update: %i\n", updateIndex.itemsCount);
	gui.setInfo("Updating...");
	gui.setInfoSecondary("Validating...");
	for (int i = 0; i < updateIndex.itemsCount; ++i) {
		FSChar *path = FS::pathConcat(installPath, updateIndex.items[i].path);
		gui.setProgress(i * 100 / updateIndex.itemsCount);
		if (FS::size(path) == updateIndex.items[i].size && Sign::checkFileHash(path, updateIndex.items[i].hash, 32)) {
			updateIndex.remove(i);
			--i;
			continue;
		}
		delete[] path;
	}
	gui.setProgress(100);
	if (!updateIndex.itemsCount) {
		newIndex.saveToFile(indexPath);
		goto finish;
	}
	for (int i = 0; i < updateIndex.itemsCount; ++i) {
		totalSize += updateIndex.items[i].size;
	}
	char question[256];
	snprintf(question, sizeof(question), "Update size is %i.%iMiB, install it?", totalSize / (1024 * 1024), (totalSize % (1024 * 1024)) / 103);
	if (gui.askYesNo(question)) {
		for (int i = 0; i < updateIndex.itemsCount; ++i) {
			gui.setProgress(i * 100 / updateIndex.itemsCount);
			FSChar *path = FS::pathConcat(installPath, updateIndex.items[i].path);
			FSChar *pathTmp = FS::concat(path, ".tmp");
			FS::directoryMakeFor(pathTmp);
			FILE *f = FS::open(pathTmp, "w");
			if (!f) {
				i = updateIndex.itemsCount;
				gui.error("Open file faild");
			} else {
				char link[strlen(repo) + strlen(updateIndex.items[i].path) + 1];
				sprintf(link, "%s/%s", repo, updateIndex.items[i].path);
				gui.setProgress(i * 100 / updateIndex.itemsCount);
				struct launcher_downloader_progress_data d = {.expectedSize = updateIndex.items[i].size, .gui = &gui};
				gui.setInfoSecondary("Downloading...");
				if (downloader.download(link, launcher_downloader_progress, &d, f, NULL, NULL)) {
					fclose(f);
					f = NULL;
					gui.setInfoSecondary("Validating...");
					if (FS::size(pathTmp) == updateIndex.items[i].size && Sign::checkFileHash(pathTmp, updateIndex.items[i].hash, 32)) {
						if (FS::move(pathTmp, path)) {
							i = updateIndex.itemsCount;
							gui.error("File saving failed");
						}
					} else {
						i = updateIndex.itemsCount;
						FS::remove(pathTmp);
						gui.error("Wrong checksum");
					}
				} else {
					printf("Downloading of %s failed\n", link);
					i = updateIndex.itemsCount;
					gui.error("Update failed");
					//fail
				}
			}
			if (f) fclose(f);
			delete[] path;
			delete[] pathTmp;
		}
	}
	newIndex.saveToFile(indexPath);
finish:
	gui.setInfo("");
	gui.setInfoSecondary("");
	gui.setProgress(100);
	gui.setProgressSecondary(100);
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
	gui.setInfo("Check repository...");
	for (p = repos; *p && !repo; p++) {
		gui.setProgress((p - repos) * 100 / n);
		printf("checking repo: %s\n", *p);
		repoCheck = new char[strlen(*p) + 16];
		sprintf(repoCheck, "%s/%s", *p, "index.lst");
		gui.setInfoSecondary("Downloading...");
		struct launcher_downloader_progress_data d = {.expectedSize = 0, .gui = &gui};
		if (downloader.download(repoCheck, launcher_downloader_progress, &d, NULL, &buffer, &bufferLength)) {
			gui.frame();
			sprintf(repoCheck, "%s/%s", *p, "index.lst.sig");
			if (downloader.download(repoCheck, NULL, NULL, NULL, &bufferSig, &bufferSigLength)) {
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
	gui.setProgress(100);
	if (repo) {
		printf("repo found: %s\n", repo);
		newIndex.load(buffer);
		delete[] buffer;
	} else
		printf("repo not found\n");
}

void Launcher::execute() {
	FSChar *executablePath = FS::pathConcat(installPath, Rexuiz::binary());
#ifdef _WIN32
	PROCESS_INFORMATION pi;
	FSChar *args = FS::fromUTF8("rexuiz.exe");
	FSChar *argsNew;
	for (int i = 1; i < argc; ++i) {
		argsNew = FS::concat(args, " ");
		delete[] args;
		args = argsNew;
		argsNew = FS::concat(args, argv[i]);
		delete[] args;
		args = argsNew;
	}
	BOOL bSuccess = CreateProcessW(executablePath,
			args,     // command line
			NULL,          // process security attributes
			NULL,          // primary thread security attributes
			TRUE,          // handles are inherited
			0,             // creation flags
			NULL,          // use parent's environment
			NULL,          // use parent's current directory
			NULL,  // STARTUPINFO pointer
			&pi);  // receives PROCESS_INFORMATION
	// If an error occurs, exit the application.
	if (bSuccess) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
#else
	int pid;
	gui.hide();
	if ((pid = fork())) {
		int status;
		waitpid(pid, &status, 0);
	} else {
		char *argv2[argc + 1];
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
	delete[] executablePath;
}

int Launcher::run() {
	int r = 1;
	gui.show();
	gui.setInfo("Preparing");
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
		installPath = gui.selectDirectory("Please select install location", startLocation);
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

	if (installRequired || !currentIndex.itemsCount)
		update();

	execute();
	printf("Saving settings\n");
	settings.save();
	r = 0;
finish:
	gui.hide();
	if (startLocation)
		delete[] startLocation;

	return r;
}
