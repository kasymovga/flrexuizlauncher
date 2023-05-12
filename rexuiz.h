#ifndef _FLRSL_REXUIZ_H_
#define _FLRSL_REXUIZ_H_

#include "fs.h"

class Rexuiz {
public:
	static const FSChar *binary();
	static const char **repos();
	static bool presentsInDirectory(FSChar *path);
};

#endif
