#include "rexuiz.h"

#include <cstddef>

const FSChar *Rexuiz::binary() {
#ifdef __linux__
#ifdef __x86_64__
	return "rexuiz-linux-sdl-x86_64";
#endif
#ifdef __i386__
	return "rexuiz-linux-sdl-i686";
#endif
#ifdef __aarch64__
	return "rexuiz-linux-sdl-aarch64";
#endif
#endif
#ifdef _WIN64
	return L"rexuiz-sdl-x86_64.exe";
#else
#ifdef _WIN32
	return L"rexuiz-sdl-i686.exe";
#endif
#endif
#ifdef __APPLE__
#ifdef __aarch64__
	return "Rexuiz-arm64.app/Contents/MacOS/rexuiz-dprm-sdl";
#else
	return "Rexuiz.app/Contents/MacOS/rexuiz-dprm-sdl";
#endif
#endif
#ifdef FS_CHAR_IS_8BIT
	return "unknown";
#else
	return L"unknown";
#endif
}

const char **Rexuiz::repos() {
	static const char *repos[6] = {
		"http://rexuiz.com/maps/RexuizLauncher2",
		"https://kasymovga.github.io/rexdlc-web/rl",
		"http://bnse.rexuiz.com/RexuizLauncher2",
		"http://nexuiz.mooo.com/RexuizLauncher2",
		"http://108.61.164.188/RexuizLauncher2",
		NULL
	};
	return repos;
}

bool Rexuiz::presentsInDirectory(FSChar *path) {
	const FSChar *binarySubPath = binary();
	FSChar *fullBinaryPath = FS::pathConcat(path, binarySubPath);
	FILE *file = FS::open(fullBinaryPath, "r");
	if (file)
		fclose(file);

	delete[] fullBinaryPath;
	return file != NULL; //just bool indicator
}
