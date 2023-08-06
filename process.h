#ifndef _FLRL_PROCESS_H_
#define _FLRL_PROCESS_H_

#include "fs.h"

#ifdef _WIN32
class Process {
public:
	static void* open(FSChar *path, FSChar *cmd);
	static void close(void *process);
	static long int fd(void *process);
};
#endif

#endif
