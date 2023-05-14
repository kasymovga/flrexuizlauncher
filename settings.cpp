#include "settings.h"

#include <stdlib.h>
#include <string.h>

static const FSChar* settings_location() {
#ifdef FS_CHAR_IS_8BIT
	const FSChar *home = getenv("HOME");
	if (!home) home = "/tmp"; //no idea what else can be done here
#else
	const FSChar *home = _wgetenv(L"USERPROFILE");
	if (!home) home = L"."; //no idea what else can be done here
#endif
	int path_size = FS::length(home) + 32;
#ifdef _WIN32
	FSChar *path = FS::pathConcat(home, "AppData" FS_DELIMETER_STRING "flrl.cfg");;
#else
#ifdef __APPLE__
	FSChar *path = FS::pathConcat(home, "Library" FS_DELIMETER_STRING "Preferences" FS_DELIMETER_STRING "flrl.cfg");;
#else
	FSChar *path = FS::pathConcat(home, ".config" FS_DELIMETER_STRING "flrl.cfg");;
#endif
#endif
	return path;
}

bool Settings::save() {
	const FSChar *path = settings_location();
	FS::directoryMakeFor(path);
	bool r = false;
	FILE *f = FS::open(path, "w");
	const char *path_utf8 = NULL;
	if (!f) goto finish;
	if (installPath) {
		path_utf8 = FS::toUTF8(this->installPath);
		fprintf(f, "install_path=%s\n", path_utf8);
	}
	r = true;
finish:
	if (f) fclose(f);
	if (path) delete[] path;
	if (path_utf8) delete[] path_utf8;
	return r;
}


bool Settings::load() {
	const FSChar *path = settings_location();
	FILE *f = FS::open(path, "r");
	char *line = NULL;
	size_t lineLength = 0;
	ssize_t lineLengthActual;
	bool r = false;
	if (!f) goto finish;;
	#ifdef _WIN32
	lineLength = 4096;
	line = new char[lineLength];
	while (fgets(line, lineLength, f)) {
		lineLengthActual = strlen(line);
	#else
	while ((lineLengthActual = getline(&line, &lineLength, f)) > 0) {
	#endif
		if (!line[0]) continue;
		if (line[lineLengthActual - 1] == '\n') {
			line[lineLengthActual - 1] = 0;
		}
		if (!strncmp(line, "install_path=", 13)) {
			if (installPath) delete[] installPath;
			installPath = FS::fromUTF8(&line[13]);
		}
	}
	r = true;
finish:
	if (f) fclose(f);
	if (path) delete[] path;
	if (line) free(line);
	return true;
}

Settings::Settings() {
	installPath = NULL;
}

Settings::~Settings() {
	if (installPath)
		delete[] installPath;
}

void Settings::setInstallPath(const FSChar *path) {
	if (installPath) {
		delete[] installPath;
		installPath = NULL;
	}
	if (path)
		installPath = FS::duplicate(path);
}
