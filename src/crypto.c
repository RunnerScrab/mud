#include "crypto.h"

int CryptoManager_Init(struct CryptoManager *cmanager)
{
	return sodium_init();
}

void CryptoManager_Destroy(struct CryptoManager *cmanager)
{

}

int CryptoManager_HashPassword(const char *password, size_t passwordlen,
		cv_t *output)
{
	cv_resize(output, crypto_pwhash_STRBYTES);
	return crypto_pwhash_str(output->data, password, passwordlen,
	crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE);
}
