#ifndef EMPARSER_H_
#define EMPARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

struct TaggedToken
{
	char* token;
	size_t index;
	size_t length; // without null terminator
	unsigned int bShouldCap;
};

struct TagParserResult
{
	struct TaggedToken* tokens;
	size_t tokenreserved, tokencount;
	size_t total_token_len;
};

//Sub list needs the replacement and the index of the token to replace
//in TagParserResult

struct TagSub
{
	char* string;
	size_t length;
};

struct TagParserSubList
{
	struct TagSub* subs;
	size_t subscount, subsreserved;
	size_t total_sub_len;
};

struct TagParser
{
	char* tokenlist;
	size_t tokenlistlen;
};

void TagParser_Init(struct TagParser* parser, const char* tlist);
void TagParser_Destroy(struct TagParser* parser);
void TagParser_Destroy(struct TagParser* parser);
void TagParser_Parse(struct TagParser* parser, const char* str, size_t len,
		struct TagParserResult* result);

void TagParserSubList_Init(struct TagParserSubList* sublist);
void TagParserSubList_Destroy(struct TagParserSubList* sublist);
struct TagSub* TagParserSubList_GetSubAt(struct TagParserSubList* sublist,
					size_t idx);
void TagParserSubList_AddSub(struct TagParserSubList* sublist,
			const char* str, size_t len);
char* TagParserResult_PerformSub(struct TagParserResult* result,
				struct TagParserSubList* subs,
				const char* str, size_t len);

void TagParserResult_Init(struct TagParserResult* result);
size_t TagParserResult_GetTokenCount(struct TagParserResult* result);
const char* TagParserResult_GetTokenAt(struct TagParserResult* result,
				size_t idx);
void TagParserResult_Destroy(struct TagParserResult* result);

#endif
