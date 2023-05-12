#ifndef _FLRL_SIGN_H_
#define _FLRL_SIGN_H_

#include "fs.h"

class Sign
{
	static bool verifyHash(const unsigned char *hash, int hashSize, const char *sign, int signSize);
public:
	static bool verify(const char *data, int dataSize, const char *sign, int signSize);
	static bool checkFileHash(FSChar *path, const char *hash, int hashSize);
};

#endif // SIGN_H
