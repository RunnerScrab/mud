#include "cmdtagparser.h"

int TaggedTokenCmp(const void* a, const void* b)
{
	struct TaggedToken* pa = (struct TaggedToken*) a;
	struct TaggedToken* pb = (struct TaggedToken*) b;
	return strcmp(pa->token, pb->token);
}

void TagParser_Init(struct TagParser* parser, const char* tlist)
{
	parser->tokenlistlen = strnlen(tlist, 128);
	parser->tokenlist = malloc(parser->tokenlistlen + 1);
	strncpy(parser->tokenlist, tlist, parser->tokenlistlen + 1);
}

void TagParserSubList_Init(struct TagParserSubList* sublist)
{
	sublist->total_sub_len = 0;
	sublist->subscount = 0;
	sublist->subsreserved = 4;
	sublist->subs = (struct TagSub*) malloc(sizeof(struct TagSub) *
						sublist->subsreserved);
}

void TagParserSubList_Destroy(struct TagParserSubList* sublist)
{
	size_t idx = 0;
	for(; idx < sublist->subscount; ++idx)
	{
		free(sublist->subs[idx].string);
	}
	free(sublist->subs);
}

struct TagSub* TagParserSubList_GetSubAt(struct TagParserSubList* sublist, size_t idx)
{
	return &sublist->subs[idx];
}

char* TagParserResult_PerformSub(struct TagParserResult* result, struct TagParserSubList* subs,
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

void TagParserSubList_AddSub(struct TagParserSubList* sublist, const char* str, size_t len)
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

void TagParserResult_Init(struct TagParserResult* result)
{
	result->total_token_len = 0;
	result->tokenreserved = 4;
	result->tokencount = 0;
	result->tokens = (struct TaggedToken*) malloc(sizeof(struct TaggedToken) *
						result->tokenreserved);
	memset(result->tokens, 0, sizeof(struct TaggedToken) * result->tokenreserved);
}

size_t TagParserResult_GetTokenCount(struct TagParserResult* result)
{
	return result->tokencount;
}

const char* TagParserResult_GetTokenAt(struct TagParserResult* result, size_t idx)
{
	return result->tokens[idx].token;
}

void TagParserResult_Destroy(struct TagParserResult* result)
{
	size_t idx = 0;
	for(; idx < result->tokencount; ++idx)
	{
		free(result->tokens[idx].token);
	}
	free(result->tokens);
}

void TagParser_Destroy(struct TagParser* parser)
{
	free(parser->tokenlist);
}

static void TagParser_AddTaggedToken(struct TagParserResult* result,
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

void TagParser_Parse(struct TagParser* parser, const char* str, size_t len,
		struct TagParserResult* result)
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
				TagParser_AddTaggedToken(result, &str[offset], offset, length,
							ShouldCapitalize(str, start));
			}
			idx = offset + length;
		}
	}
	while(start);

}
