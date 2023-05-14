#include "sign.h"
#include "rexuiz_pub_key.h"

#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/md_internal.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform.h>
#include <string.h>

bool Sign::verifyHash(const unsigned char *hash, int hashSize, const char *sign, int signSize) {
	bool r = false;
	mbedtls_pk_context pk;
	mbedtls_pk_init(&pk);
	unsigned char pub_key_null_terminated[sizeof(rexuiz_pub_key) + 1];
	memcpy(pub_key_null_terminated, rexuiz_pub_key, sizeof(rexuiz_pub_key));
	pub_key_null_terminated[sizeof(rexuiz_pub_key)] = 0;
	if (mbedtls_pk_parse_public_key(&pk, pub_key_null_terminated, sizeof(rexuiz_pub_key) + 1)) {
		goto finish;
	}
	if (mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 0, (const unsigned char *)sign, signSize)) {
		goto finish;
	}
	r = true;
finish:
	mbedtls_pk_free(&pk);
	return r;
}

bool Sign::checkFileHash(FSChar *path, const char *hash, int hashSize) {
	bool r = false;
	if (hashSize != 32) return false;
	mbedtls_md_context_t md_ctx;
	mbedtls_md_init(&md_ctx);
	FILE *f = NULL;
	unsigned char checkhash_bin[32];
	unsigned char checkhash[32];
	const char *hex = "0123456789abcdef";
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
	if (mbedtls_md_setup(&md_ctx, md_info, 0))
		goto finish;

	if (mbedtls_md_starts(&md_ctx))
		goto finish;

	if (!(f = FS::open(path, "r")))
		goto finish;

	char buffer[4096];
	int n;
	while ((n = fread(buffer, 1, 4096, f)) > 0) {
		if (mbedtls_md_update(&md_ctx, (const unsigned char *)buffer, n))
			goto finish;
	}
	if (mbedtls_md_finish(&md_ctx, checkhash_bin))
		goto finish;

	for (int i = 0; i < 16; i++) {
		checkhash[2 * i] = hex[checkhash_bin[i] / 16];
		checkhash[2 * i + 1] = hex[checkhash_bin[i] % 16];
	}
	r = !memcmp(hash, checkhash, hashSize);
finish:
	if (f) fclose(f);
	mbedtls_md_free(&md_ctx);
	return r;
}

bool Sign::verify(const char *data, int dataSize, const char *sign, int signSize) {
	bool r = false;
	mbedtls_md_context_t md_ctx;
	mbedtls_md_init(&md_ctx);
	unsigned char hash[32];
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (mbedtls_md_setup(&md_ctx, md_info, 0))
		goto finish;

	if (mbedtls_md_starts(&md_ctx))
		goto finish;

	if (mbedtls_md_update(&md_ctx, (const unsigned char *)data, dataSize))
		goto finish;

	if (mbedtls_md_finish(&md_ctx, hash))
		goto finish;

	r = verifyHash(hash, sizeof(hash), sign, signSize);
finish:
	mbedtls_md_free(&md_ctx);
	return r;
}
