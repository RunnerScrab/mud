#include "cmdlexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void LexerResult_Prepare(struct LexerResult* lr)
{
	lr->token_count = 0;
	lr->token_reserved = 2;
	lr->orig_str = 0;
	lr->tokens = (char**) malloc(sizeof(char*) * lr->token_reserved);
	lr->token_starts = (size_t*) malloc(sizeof(size_t) * lr->token_reserved);
	memset(lr->tokens, 0, sizeof(char*) * lr->token_reserved);
	memset(lr->token_starts, 0, sizeof(size_t) * lr->token_reserved);
}

void LexerResult_AddToken(struct LexerResult* lr, size_t start, const char* token,
		size_t tokenlen)
{
	if(lr->token_count >= lr->token_reserved)
	{
		lr->token_reserved <<= 1;
		lr->tokens = (char**) realloc(lr->tokens,
			sizeof(char*) * lr->token_reserved);
		lr->token_starts = (size_t*) realloc(lr->token_starts,
						sizeof(size_t) * lr->token_reserved);
	}
	lr->tokens[lr->token_count] = malloc(tokenlen + 1);
	strncpy(lr->tokens[lr->token_count], token, tokenlen + 1);
	lr->token_starts[lr->token_count] = start;
	++lr->token_count;
}

void LexerResult_Clear(struct LexerResult* lr)
{
	size_t idx = 0;
	for(; idx < lr->token_count; ++idx)
	{
		if(lr->tokens[idx])
		{
			free(lr->tokens[idx]);
			lr->tokens[idx] = 0;
		}
	}
	lr->token_count = 0;
	memset(lr->tokens, 0, sizeof(char*) * lr->token_reserved);
	memset(lr->token_starts, 0, sizeof(size_t) * lr->token_reserved);
	if(lr->orig_str)
		free(lr->orig_str);
}

void LexerResult_Destroy(struct LexerResult* lr)
{
	size_t idx = 0;
	for(; idx < lr->token_count; ++idx)
	{
		if(lr->tokens[idx])
		{
			free(lr->tokens[idx]);
			lr->tokens[idx] = 0;
		}
	}
	free(lr->tokens);
	free(lr->token_starts);
	if(lr->orig_str)
		free(lr->orig_str);
}

size_t LexerResult_GetTokenCount(struct LexerResult* lr)
{
	return lr->token_count;
}

char* LexerResult_GetTokenAt(struct LexerResult* lr, size_t idx)
{
	if(idx >= lr->token_count)
		return 0;
	return lr->tokens[idx];
}

char* LexerResult_GetStringAfterToken(struct LexerResult* lr, size_t token)
{
	return &lr->orig_str[lr->token_starts[token]];
}

void LexerResult_LexString(struct LexerResult* lr, const char* str, size_t len)
{
	lr->orig_str = malloc(len + 1);
	memcpy(lr->orig_str, str, len + 1);

	char* saveptr = 0;
	char* copy = malloc(len + 1);
	strncpy(copy, str, len + 1);

	char* result = 0;
	char* pstr = copy;
	do
	{
		result = strtok_r(pstr, " \n", &saveptr);
		if(result)
		{
			LexerResult_AddToken(lr, result - copy, result,
				strnlen(result, len + 1));
		}
		pstr = 0;

	}
	while(result);
	free(copy);
}
