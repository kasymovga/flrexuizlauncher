#ifndef _FLRL_LAUNCHER_H_
#define _FLRL_LAUNCHER_H_
#include "gui.h"
#include "downloader.h"
#include "settings.h"
#include "fs.h"
#include "index.h"

class Launcher {
public:
	~Launcher();
	Launcher(int, char **);
	int run();
private:
	Index currentIndex;
	Index newIndex;
	bool updater;
	bool updateFailed;
	const char *repo;
	FSChar *installPath;
	FSChar *indexPath;
	GUI gui;
	Downloader downloader;
	Settings settings;
	int argc;
	char **argv;
	void repoSearch();
	bool installRequired;
	void validate();
	void update();
	void execute();
};
#endif
