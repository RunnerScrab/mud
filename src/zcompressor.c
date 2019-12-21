#include "zcompressor.h"

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

int ZCompressor_CompressRawData(ZCompressor* pComp,
				const char* buf, size_t len, cv_t* out)
{
	cv_clear(out);
	size_t bound = deflateBound(&pComp->strm_out, len);
	cv_resize(out, bound);

	z_stream* strm = &pComp->strm_out;
	strm->avail_in = len;
	strm->next_in = (Bytef*) buf;
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


int ZCompressor_CompressData(ZCompressor* pComp, cv_t* in, cv_t* out)
{
	cv_clear(out);
	size_t bound = deflateBound(&pComp->strm_out, in->length);
	cv_resize(out, bound);

	z_stream* strm = &pComp->strm_out;
	strm->avail_in = in->length;
	strm->next_in = (Bytef*) in->data;
	strm->avail_out = bound;
	strm->next_out = (Bytef*) out->data;
	int result = 0;
	for(;strm->avail_in > 0 || strm->avail_out == 0;)
	{
		switch(result = deflate(strm, Z_SYNC_FLUSH))
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
