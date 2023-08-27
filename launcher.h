#ifndef _FLRL_LAUNCHER_H_
#define _FLRL_LAUNCHER_H_
#include "gui.h"
#include "downloader.h"
#include "settings.h"
#include "fs.h"
#include "index.h"
#include "translation.h"

class GUI;
class Launcher {
public:
	~Launcher();
	Launcher(int, char **);
	int run();
	void abort();
	static const long long int version = 20230827;
	Translation *translation;
private:
	Index currentIndex;
	Index newIndex;
	bool updater;
	bool updateFailed;
	bool updateHappened;
	bool updateEmpty;
	const char *repo;
	FSChar *installPath;
	FSChar *indexPath;
	GUI *gui;
	Downloader downloader;
	Settings settings;
	int argc;
	char **argv;
	void repoSearch();
	bool installRequired;
	void validate();
	void update();
	void execute();
	bool aborted;
	void error(const char *message);
	bool checkNewVersion();
};
#endif
