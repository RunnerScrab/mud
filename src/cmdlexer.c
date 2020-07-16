#include "cmdlexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Lexer_Prepare(struct Lexer* lx, unsigned char bParseSubcommands, size_t max_tokens)
{
	lx->bParseSubcmds = bParseSubcommands;
	lx->max_tokens = max_tokens;

	lx->subcmdmarker_length = 0;
	lx->subcmdmarker_reserved = 4;
	lx->subcmdmarkers = bParseSubcommands ? malloc(lx->subcmdmarker_reserved * sizeof(char*)) : 0;
	if(bParseSubcommands)
	{
		memset(lx->subcmdmarkers, 0, sizeof(char*) * lx->subcmdmarker_reserved);
	}

}

void Lexer_Destroy(struct Lexer* lx)
{
	size_t idx = 0;
	for(; idx < lx->subcmdmarker_length; ++idx)
	{
		free(lx->subcmdmarkers[idx]);
	}

	if(lx->subcmdmarkers)
	{
		free(lx->subcmdmarkers);
		lx->subcmdmarkers = 0;
	}
}

void LexerResult_Prepare(struct LexerResult* lr)
{
	lr->bFilled = 0;
	lr->token_count = 0;
	lr->token_reserved = 2;
	lr->orig_str = 0;
	lr->subcommands = 0;
	lr->subcmd_length = 0;
	lr->subcmd_reserved = 4;
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
	lr->bFilled = 0;

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
	{
		free(lr->orig_str);
		lr->orig_str = 0;
	}

	for(idx = 0; idx < lr->subcmd_length; ++idx)
	{
		free(lr->subcommands[idx]);
		lr->subcommands[idx] = 0;
	}

	lr->subcmd_length = 0;
}

void Lexer_AddSubcommandMarkers(struct Lexer* lr, const char left, const char right)
{
	if(lr->subcmdmarker_length >= lr->subcmdmarker_reserved)
	{
		lr->subcmdmarker_reserved <<= 1;
		lr->subcmdmarkers = realloc(lr->subcmdmarkers, sizeof(char*) * lr->subcmdmarker_reserved);
	}

	lr->subcmdmarkers[lr->subcmdmarker_length] = malloc(2);
	lr->subcmdmarkers[lr->subcmdmarker_length][0] = left;
	lr->subcmdmarkers[lr->subcmdmarker_length][1] = right;
	++lr->subcmdmarker_length;
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
	lr->tokens = 0;

	free(lr->token_starts);
	lr->token_starts = 0;

	if(lr->orig_str)
	{
		free(lr->orig_str);
		lr->orig_str = 0;
	}

	for(idx = 0; idx < lr->subcmd_length; ++idx)
	{
		free(lr->subcommands[idx]);
	}

	if(lr->subcommands)
	{
		free(lr->subcommands);
		lr->subcommands = 0;
	}
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

static int SubCommandIndexCmp(const void* a, const void* b)
{
	struct SubCommandIndex* pa = (struct SubCommandIndex*) a;
	struct SubCommandIndex* pb = (struct SubCommandIndex*) b;
	return pa->index - pb->index;
}

char* Lexer_ExtractSubcommands(const struct Lexer* lexer, struct LexerResult* lxresult, const char* str, size_t len)
{
	//TODO: This function is probably pretty slow; optimize
	struct SubCommandIndex* subcmdindices = 0;
	char* parsedstr = malloc(len + 1);

	if(!parsedstr)
	{
		return 0;
	}

	memset(parsedstr, 0, len + 1);

	size_t idx = 0;
	char* start = 0, *end = 0;

	lxresult->subcmd_length = 0;
	lxresult->subcmd_reserved = 4;

	subcmdindices = malloc(sizeof(struct SubCommandIndex) * lxresult->subcmd_reserved);
	memset(subcmdindices, 0, sizeof(struct SubCommandIndex) * lxresult->subcmd_reserved);

	if(!lxresult->subcommands)
	{
		lxresult->subcommands = malloc(sizeof(char*) * lxresult->subcmd_reserved);
		memset(subcmdindices, 0, sizeof(struct SubCommandIndex) * lxresult->subcmd_reserved);
	}

	size_t markeridx = 0;
	for(; markeridx < lexer->subcmdmarker_length; ++markeridx)
	{
		char left = lexer->subcmdmarkers[markeridx][0];
		char right = lexer->subcmdmarkers[markeridx][1];
		idx = 0;
		do
		{
			start = memchr(&str[idx], left, len - idx);
			if(start)
			{
				end = memchr(&str[idx], right, len - idx);

				if(end && end > start)
				{
					if(lxresult->subcmd_length >= lxresult->subcmd_reserved)
					{
						lxresult->subcmd_reserved <<= 1;
						lxresult->subcommands = realloc(lxresult->subcommands,
									sizeof(char*) * lxresult->subcmd_reserved);
						subcmdindices = realloc(subcmdindices, sizeof(struct SubCommandIndex) *
									lxresult->subcmd_reserved);
					}

					size_t subcmdlen = end - start + 1;
					subcmdindices[lxresult->subcmd_length].index = start - str;
					subcmdindices[lxresult->subcmd_length].length = subcmdlen;

					lxresult->subcommands[lxresult->subcmd_length] = malloc(subcmdlen + 1);
					memset(lxresult->subcommands[lxresult->subcmd_length], 0, subcmdlen);

					strncat(lxresult->subcommands[lxresult->subcmd_length], start, subcmdlen);

					++lxresult->subcmd_length;

					idx = end - str + 1;
				}
				else
				{
					break;
				}
			}
		}
		while(start);
	}

	qsort(subcmdindices, lxresult->subcmd_length, sizeof(struct SubCommandIndex),
		SubCommandIndexCmp);

	size_t lastidx = 0;

	//Reconstitute string with the subcommands extracted
	for(idx = 0; idx < lxresult->subcmd_length; ++idx)
	{
		size_t scmdidx = subcmdindices[idx].index;
		size_t space =
			((scmdidx - 1) >= 0 && (scmdidx - 1) < len &&
				' ' == str[scmdidx - 1]) ? 1 : 0;
		int n = (scmdidx - (lastidx + space));

		if(n < 0)
		{
			break;
		}

		strncat(parsedstr, &str[lastidx], n);
		lastidx = scmdidx + subcmdindices[idx].length;
	}

	strncat(parsedstr, &str[lastidx], (len - lastidx) + 1);

	free(subcmdindices);
	subcmdindices = 0;
	return parsedstr;
}

void Lexer_LexString(const struct Lexer* lx, const char* str, size_t len, struct LexerResult* lr)
{
	if(lr->orig_str)
	{
		//This function should never be called twice for the same result
		//struct
		return;
	}

	if(lx->bParseSubcmds)
	{
		lr->orig_str = Lexer_ExtractSubcommands(lx, lr, str, len);
	}
	else
	{
		lr->orig_str = malloc(len + 1);
		strncpy(lr->orig_str, str, len + 1);
	}

	char* saveptr = 0;
	char* copy = malloc(len + 1);
	strncpy(copy, lr->orig_str, len + 1);

	char* result = 0;
	char* pstr = copy;
	do
	{
		result = strtok_r(pstr, " \n", &saveptr);
		if(result)
		{
			LexerResult_AddToken(lr, result - copy, result,
					strnlen(result, len + 1));

			if(lx->max_tokens && lr->token_count >= lx->max_tokens)
			{
				free(copy);
				return;
			}
		}
		pstr = 0;

	}
	while(result);
	free(copy);
}

size_t LexerResult_GetSubcommandCount(struct LexerResult* lr)
{
	return lr->subcmd_length;
}

char* LexerResult_GetSubcommandAt(struct LexerResult* lr, size_t idx)
{
	return lr->subcommands[idx];
}
