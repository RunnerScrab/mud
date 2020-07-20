#include "cmdtaglexer.h"

int TaggedTokenCmp(const void* a, const void* b)
{
	struct TaggedToken* pa = (struct TaggedToken*) a;
	struct TaggedToken* pb = (struct TaggedToken*) b;
	return strcmp(pa->token, pb->token);
}

void TagLexer_Init(struct TagLexer* parser, const char* tlist)
{
	parser->tokenlistlen = strnlen(tlist, 128);
	parser->tokenlist = malloc(parser->tokenlistlen + 1);
	strncpy(parser->tokenlist, tlist, parser->tokenlistlen + 1);
}

void TagLexerSubList_Init(struct TagLexerSubList* sublist)
{
	sublist->total_sub_len = 0;
	sublist->subscount = 0;
	sublist->subsreserved = 4;
	sublist->subs = (struct TagSub*) malloc(sizeof(struct TagSub) *
						sublist->subsreserved);
}

void TagLexerSubList_Destroy(struct TagLexerSubList* sublist)
{
	size_t idx = 0;
	for(; idx < sublist->subscount; ++idx)
	{
		free(sublist->subs[idx].string);
	}
	free(sublist->subs);
}

struct TagSub* TagLexerSubList_GetSubAt(struct TagLexerSubList* sublist, size_t idx)
{
	return &sublist->subs[idx];
}

char* TagLexerResult_PerformSub(struct TagLexerResult* result, struct TagLexerSubList* subs,
				const char* str, size_t len)
{
	if(result->tokencount != subs->subscount)
	{
		return 0;
	}

	size_t required = (len - result->total_token_len) + subs->total_sub_len + 1;
	char* subbedstr = (char*) malloc(required);
	memset(subbedstr, 0, required);
	size_t idx = 0;
	size_t strpos = 0;

	for(; idx < result->tokencount; ++idx)
	{
		strncat(subbedstr, &str[strpos], result->tokens[idx].index - strpos);
		char* psubstr = subs->subs[idx].string;
		size_t sublen = subs->subs[idx].length;

		if(result->tokens[idx].bShouldCap)
		{
			psubstr[0] = toupper(psubstr[0]);
		}
		strncat(subbedstr, psubstr, sublen);

		strpos += result->tokens[idx].index - strpos
			+ result->tokens[idx].length;
	}
	strncat(subbedstr, &str[strpos], len - strpos + 1);
	return subbedstr;
}

void TagLexerSubList_AddSub(struct TagLexerSubList* sublist, const char* str, size_t len)
{
	if(sublist->subscount >= sublist->subsreserved)
	{
		sublist->subsreserved <<= 1;
		sublist->subs = realloc(sublist->subs, sizeof(struct TagSub)
					* sublist->subsreserved);
	}

	struct TagSub* pTagSub = &sublist->subs[sublist->subscount];
	pTagSub->string = malloc(len + 1);
	pTagSub->length = len;

	strncpy(pTagSub->string, str, len + 1);
	++sublist->subscount;
	sublist->total_sub_len += len;
}

void TagLexerResult_Init(struct TagLexerResult* result)
{
	result->total_token_len = 0;
	result->tokenreserved = 4;
	result->tokencount = 0;
	result->tokens = (struct TaggedToken*) malloc(sizeof(struct TaggedToken) *
						result->tokenreserved);
	memset(result->tokens, 0, sizeof(struct TaggedToken) * result->tokenreserved);
}

size_t TagLexerResult_GetTokenCount(struct TagLexerResult* result)
{
	return result->tokencount;
}

const char* TagLexerResult_GetTokenAt(struct TagLexerResult* result, size_t idx)
{
	return result->tokens[idx].token;
}

void TagLexerResult_Destroy(struct TagLexerResult* result)
{
	size_t idx = 0;
	for(; idx < result->tokencount; ++idx)
	{
		free(result->tokens[idx].token);
	}
	free(result->tokens);
}

void TagLexer_Destroy(struct TagLexer* parser)
{
	free(parser->tokenlist);
}

static void TagLexer_AddTaggedToken(struct TagLexerResult* result,
				const char* str,
			size_t index, size_t len, unsigned char bCap)
{
	if(result->tokencount >= result->tokenreserved)
	{
		result->tokenreserved <<= 1;
		result->tokens = (struct TaggedToken*) realloc(result->tokens, sizeof(struct TaggedToken) *
							result->tokenreserved);
		memset(&result->tokens[result->tokencount], 0,
			sizeof(struct TaggedToken) * (result->tokenreserved - result->tokencount));
	}
	struct TaggedToken* newtoken = &(result->tokens[result->tokencount]);
	newtoken->token = (char*) malloc(len + 1);
	newtoken->index = index;
	newtoken->length = len;
	newtoken->bShouldCap = bCap;
	//We're copying a substring from a longer string, and the byte at
	//[length] may not be a null
	strncpy(newtoken->token, str, len);
	newtoken->token[len] = 0;
	++result->tokencount;
	result->total_token_len += len;
}

static unsigned char ShouldCapitalize(const char* str, const char* start)
{
	int idx = start - str;
	for(; idx >= 0; --idx)
	{
		if(isalpha(str[idx]))
		{
			return 0;
		}
		else if(str[idx] == '.')
		{
			return 1;
		}
	}
	return 1;
}

void TagLexer_Parse(struct TagLexer* parser, const char* str, size_t len,
		struct TagLexerResult* result)
{
	char* start = 0;
	size_t idx = 0;
	do
	{
		start = strpbrk(&str[idx], parser->tokenlist);
		if(start)
		{
			size_t offset = start - str;
			size_t length = 1;
			for(; (length + offset) < len; ++length)
			{
				if(!isalnum(str[offset + length]))
				{
					break;
				}
			}

			if(length > 1)
			{
				TagLexer_AddTaggedToken(result, &str[offset], offset, length,
							ShouldCapitalize(str, start));
			}
			idx = offset + length;
		}
	}
	while(start);

}
