#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "charvector.h"
#include <string.h>

typedef struct
{
	z_stream strm_in, strm_out;
} ZCompressor;

int ZCompressor_Init(ZCompressor* comp)
{
	comp->strm_in.zfree = Z_NULL;
	comp->strm_in.opaque = Z_NULL;
	comp->strm_in.avail_in = 0;
	comp->strm_in.next_in = Z_NULL;
	comp->strm_in.zalloc = Z_NULL;

	comp->strm_out.zfree = Z_NULL;
	comp->strm_out.opaque = Z_NULL;
	comp->strm_out.avail_in = 0;
	comp->strm_out.next_in = Z_NULL;
	comp->strm_out.zalloc = Z_NULL;

	return
		(inflateInit(&comp->strm_in) == Z_OK &&
			deflateInit(&comp->strm_out, Z_DEFAULT_COMPRESSION) == Z_OK) ? 0 : -1;
}

int ZCompressor_CompressData(ZCompressor* pComp, cv_t* in, cv_t* out)
{
	cv_clear(out);
	size_t bound = deflateBound(&pComp->strm_out, in->length);
	cv_resize(out, bound);
	//the compressed result will rarely ever be the same size as the source data
	//
	z_stream* strm = &pComp->strm_out;
	strm->avail_in = in->length;
	strm->next_in = (Bytef*) in->data;
	strm->avail_out = bound;
	strm->next_out = (Bytef*) out->data;
	int result = 0;
	for(;strm->avail_in > 0 || strm->avail_out == 0;)
	{
		switch(result = deflate(strm, Z_PARTIAL_FLUSH))
		{
		case Z_NEED_DICT:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			deflateEnd(strm);
			return -1;
		}
		out->length += bound - strm->avail_out;
	}

	return result == Z_OK || result == Z_STREAM_END ? 0 : -1;
}

int ZCompressor_DecompressData(ZCompressor* pComp, cv_t* in, cv_t* out)
{
	cv_resize(out, 512);
	char buffer[1024] = {0};
	z_stream* strm = &pComp->strm_in;
	strm->avail_in = in->length;
	strm->next_in = (Bytef*) in->data;
	strm->avail_out = 1024;
	strm->next_out = (Bytef*) buffer;

	int result = 0;
	for(;strm->avail_in > 0 || strm->avail_out == 0;)
	{
		switch(result = inflate(strm, Z_PARTIAL_FLUSH))
		{
		case Z_NEED_DICT:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			inflateEnd(strm);
			return -1;
		}
		cv_append(out, buffer, 1024 - strm->avail_out);
	}

	return result == Z_OK || result == Z_STREAM_END ? 0 : -1;

}

int ZCompressor_Reset(ZCompressor* pComp)
{
	return inflateReset(&pComp->strm_in) == Z_OK &&
		deflateReset(&pComp->strm_out) == Z_OK ? 0 : -1;
}

void ZCompressor_StopAndRelease(ZCompressor* pComp)
{
	inflateEnd(&pComp->strm_in);
	deflateEnd(&pComp->strm_out);
}

int main(void)
{
	printf("z_stream is %u bytes.\n", sizeof(z_stream));
	ZCompressor compressor;
	ZCompressor_Init(&compressor);

	if(ZCompressor_Init(&compressor))
	{
		printf("Zlib failed to initialize compression.\n");
		return -1;
	}
	else
	{
		printf("Zlib initialized compression.\n");
	}

	char* demostr = "Before the call of deflate(), the application should ensure that at least one of the actions is possible, by providing more input and/or consuming more output, and updating avail_in or avail_out accordingly; avail_out should never be zero before the call. The application can consume the compressed output when it wants, for example when the output buffer is full (avail_out == 0), or after each call of deflate(). If deflate returns Z_OK and with zero avail_out, it must be called again after making room in the buffer because there might be more output pending. See deflatePending(), which can be used if desired to determine whether or not there is more ouput in that case.";
	cv_t test_in, compressed, decompressed;
	cv_init(&test_in, 256);
	cv_init(&compressed, 256);
	cv_init(&decompressed, 256);
	cv_append(&test_in, demostr, strlen(demostr));

	ZCompressor_CompressData(&compressor, &test_in, &compressed);
	printf("Original size: %u Compressed size: %u\n", test_in.length, compressed.length);


	ZCompressor_DecompressData(&compressor, &compressed, &decompressed);
	printf("Decompressed size: %u\n", decompressed.length);
	cv_push(&decompressed, 0);
	printf("%s\n", decompressed.data);

	ZCompressor_StopAndRelease(&compressor);
	cv_destroy(&test_in);
	cv_destroy(&compressed);
	cv_destroy(&decompressed);
	return 0;
}
