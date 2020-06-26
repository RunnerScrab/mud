#ifndef CMDLEXER_H_
#define CMDLEXER_H_

#include <stddef.h>

struct LexerResult
{
	size_t token_count;
	size_t token_reserved;
	char** tokens;
	size_t* token_starts;
	char* orig_str;
};

void LexerResult_Prepare(struct LexerResult* lr);

void LexerResult_AddToken(struct LexerResult* lr, size_t start, const char* token,
			size_t tokenlen);

void LexerResult_Destroy(struct LexerResult* lr);
void LexerResult_Clear(struct LexerResult* lr);

size_t LexerResult_GetTokenCount(struct LexerResult* lr);
char* LexerResult_GetTokenAt(struct LexerResult* lr, size_t idx);
char* LexerResult_GetStringAfterToken(struct LexerResult* lr, size_t token);
void LexerResult_LexString(struct LexerResult* lr, const char* str, size_t len);

#endif
