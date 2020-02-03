#ifndef CRYPTO_H_
#define CRYPTO_H_
#include "charvector.h"
#include "libsodium/src/libsodium/include/sodium.h"

struct CryptoManager
{

};


int CryptoManager_Init(struct CryptoManager* cmanager);
void CryptoManager_Destroy(struct CryptoManager* cmanager);
int CryptoManager_HashPassword(const char* password,
			size_t passwordlen,
			cv_t* output);

#endif
