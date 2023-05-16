#ifndef _FLRSL_SETTINGS_H_
#define _FLRSL_SETTINGS_H_

#include "fs.h"

class Settings {
public:
	Settings();
	~Settings();
	FSChar *installPath;
	bool save();
	bool load();
	void setInstallPath(const FSChar *path);
	long long unsigned int lastUpdate;
};

#endif
