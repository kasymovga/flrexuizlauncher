#include "settings.h"

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <winreg.h>
#endif

static const FSChar* settings_location() {
#ifdef FS_CHAR_IS_8BIT
	const FSChar *home = getenv("HOME");
	if (!home) home = "/tmp"; //no idea what else can be done here
#else
	const FSChar *home = _wgetenv(L"APPDATA");
	if (!home) home = _wgetenv(L"USERPROFILE");
	if (!home) home = L"."; //no idea what else can be done here
#endif
	int path_size = FS::length(home) + 32;
#ifdef _WIN32
	FSChar *path = FS::pathConcat(home, "RexuizLauncher" FS_DELIMETER_STRING "flrl.cfg");;
#else
#ifdef __APPLE__
	FSChar *path = FS::pathConcat(home, "Library" FS_DELIMETER_STRING "Application Support" FS_DELIMETER_STRING "flrl.cfg");;
#else
	FSChar *path = FS::pathConcat(home, ".config" FS_DELIMETER_STRING "flrl.cfg");;
#endif
#endif
	return path;
}

void Settings::import() {
#ifdef _WIN32
	DWORD bufferSize = 1024;
	wchar_t buffer[bufferSize];
	if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\RexuizDev\\RexuizLauncher\\main", L"install_path", RRF_RT_REG_SZ, NULL, buffer, &bufferSize) == ERROR_SUCCESS) {
		if (buffer[0])
			installPath = FS::duplicate(buffer);
	}
#else
#ifdef __APPLE__
	FILE *f = popen("plutil -extract main\\\\.install_path xml1 ~/Library/Preferences/com.rexuizdev.RexuizLauncher.plist -o - | xmllint --xpath '//plist//string//text()'", "rb");
	if (f) {
		char buffer[2048];
		int n;
		if ((n = fread(buffer, sizeof(buffer) - 1, 1, f)) > 0) {
			buffer[n] = 0;
			installPath = FS::duplicate(buffer);
		}
		fclose(f);
	}
#else
	FILE *f = NULL;
	const FSChar *home = getenv("HOME");
	char *line = NULL;
	int lineLength = 0;
	int lineLengthActual;
	printf("Trying import settings from qrexuizlauncher\n");
	FSChar *path = FS::pathConcat(home, ".config" FS_DELIMETER_STRING "RexuizDev" FS_DELIMETER_STRING "RexuizLauncher.conf");;
	if (!(f = FS::open(path, "r"))) goto finish;
	printf("Parsing config file of qrexuizlauncher\n");
	while ((lineLengthActual = FS::readLine(&line, &lineLength, f)) >= 0) {
		if (!strncmp(line, "install_path=", 13)) {
			if (installPath) delete[] installPath;
			installPath = FS::fromUTF8(&line[13]);
			break;
		}
	}
finish:
	if (path) delete[] path;
	if (line) delete[] line;
	if (f) fclose(f);
#endif
#endif
}

bool Settings::save() {
	const FSChar *path = settings_location();
	FS::directoryMakeFor(path);
	bool r = false;
	FILE *f = FS::open(path, "wb");
	const char *path_utf8 = NULL;
	if (!f) goto finish;
	if (installPath) {
		path_utf8 = FS::toUTF8(this->installPath);
		fprintf(f, "install_path=%s\n", path_utf8);
		fprintf(f, "last_update=%lli\n", lastUpdate);
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
	FILE *f = FS::open(path, "rb");
	char *line = NULL;
	int lineLength = 0;
	int lineLengthActual;
	bool r = false;
	if (!f) goto finish;
	while ((lineLengthActual = FS::readLine(&line, &lineLength, f)) >= 0) {
		if (!strncmp(line, "install_path=", 13)) {
			if (installPath) delete[] installPath;
			installPath = FS::fromUTF8(&line[13]);
		}
		if (!strncmp(line, "last_update=", 12)) {
			lastUpdate = atol(&line[12]);
		}
	}
	r = true;
finish:
	if (f) fclose(f);
	if (!r) import();
	if (path) delete[] path;
	if (line) delete[] line;
	return r;
}

Settings::Settings() {
	installPath = NULL;
	lastUpdate = 0;
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
