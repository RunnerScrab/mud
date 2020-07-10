#include "cmdlexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void LexerResult_Prepare(struct LexerResult* lr, unsigned char bParseSubcommands)
{
	lr->subcmdmarker_length = 0;
	lr->subcmdmarker_reserved = 4;
	lr->subcmdmarkers = bParseSubcommands ? malloc(lr->subcmdmarker_reserved * sizeof(char*)) : 0;

	lr->bParseSubcmds = bParseSubcommands;
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

	if(bParseSubcommands)
	{
		memset(lr->subcmdmarkers, 0, sizeof(char*) * lr->subcmdmarker_reserved);
	}
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
	//TODO: Freeing the elements of the array but not the array itself
	//is idiotic; do both or neither when you aren't half asleep
/*	unsigned char bParse = lr->bParseSubcmds;
	LexerResult_Destroy(lr);
	memset(lr, 0, sizeof(struct LexerResult));
	LexerResult_Prepare(lr, bParse);
	return;*/
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

	/*
	  Don't reset the subcmd markers as these are likely to be reused
	  for(idx = 0; idx < lr->subcmdmarker_length; ++idx)
	  {
	  free(lr->subcmdmarkers[idx]);
	  lr->subcmdmarkers[idx] = 0;
	  }

	  lr->subcmdmarker_length = 0;
	*/
}

void LexerResult_AddSubcommandMarkers(struct LexerResult* lr, const char left, const char right)
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

	for(idx = 0; idx < lr->subcmdmarker_length; ++idx)
	{
		free(lr->subcmdmarkers[idx]);
	}

	if(lr->subcmdmarkers)
	{
		free(lr->subcmdmarkers);
		lr->subcmdmarkers = 0;
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

struct SubCommandIndex
{
	size_t index, length;
};

int SubCommandIndexCmp(const void* a, const void* b)
{
	struct SubCommandIndex* pa = (struct SubCommandIndex*) a;
	struct SubCommandIndex* pb = (struct SubCommandIndex*) b;
	return pa->index - pb->index;
}

char* LexerResult_ExtractSubcommands(struct LexerResult* lexer, const char* str, size_t len)
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

	lexer->subcmd_length = 0;
	lexer->subcmd_reserved = 4;

	subcmdindices = malloc(sizeof(struct SubCommandIndex) * lexer->subcmd_reserved);
	memset(subcmdindices, 0, sizeof(struct SubCommandIndex) * lexer->subcmd_reserved);

	if(!lexer->subcommands)
	{
		lexer->subcommands = malloc(sizeof(char*) * lexer->subcmd_reserved);
		memset(subcmdindices, 0, sizeof(struct SubCommandIndex) * lexer->subcmd_reserved);
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
					if(lexer->subcmd_length >= lexer->subcmd_reserved)
					{
						lexer->subcmd_reserved <<= 1;
						lexer->subcommands = realloc(lexer->subcommands,
									sizeof(char*) * lexer->subcmd_reserved);
						subcmdindices = realloc(subcmdindices, sizeof(struct SubCommandIndex) *
									lexer->subcmd_reserved);
					}

					size_t subcmdlen = end - start + 1;
					subcmdindices[lexer->subcmd_length].index = start - str;
					subcmdindices[lexer->subcmd_length].length = subcmdlen;

					lexer->subcommands[lexer->subcmd_length] = malloc(subcmdlen + 1);
					memset(lexer->subcommands[lexer->subcmd_length], 0, subcmdlen);

					strncat(lexer->subcommands[lexer->subcmd_length], start, subcmdlen);

					++lexer->subcmd_length;

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

	qsort(subcmdindices, lexer->subcmd_length, sizeof(struct SubCommandIndex),
		SubCommandIndexCmp);

	size_t lastidx = 0;

	//Reconstitute string with the subcommands extracted
	for(idx = 0; idx < lexer->subcmd_length; ++idx)
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

void LexerResult_LexString(struct LexerResult* lr, const char* str, size_t len, size_t max_tokens)
{
	if(lr->orig_str)
	{
		//This function should never be called twice for the same result
		//struct
		return;
	}

	if(lr->bParseSubcmds)
	{
		lr->orig_str = LexerResult_ExtractSubcommands(lr, str, len);
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

			if(max_tokens && lr->token_count >= max_tokens)
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
