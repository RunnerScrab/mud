#ifndef ZCOMPRESSOR_H_
#define ZCOMPRESSOR_H_
#include <zlib.h>
#include "charvector.h"

typedef struct
{
	//These may have to exist per connection, depending on flush settings
	z_stream strm_in, strm_out;
} ZCompressor;

int ZCompressor_Init(ZCompressor *comp);
int ZCompressor_CompressData(ZCompressor *pComp, cv_t *in, cv_t *out);
int ZCompressor_CompressRawData(ZCompressor *pComp, const char *buf, size_t len,
		cv_t *out);
int ZCompressor_DecompressData(ZCompressor *pComp, cv_t *in, cv_t *out);
int ZCompressor_Reset(ZCompressor *pComp);
void ZCompressor_StopAndRelease(ZCompressor *pComp);
#endif
