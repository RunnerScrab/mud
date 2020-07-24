#ifndef CMDTAGLEXER_H_
#define CMDTAGLEXER_H_

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

struct TagLexerResult
{
	struct TaggedToken* tokens;
	size_t tokenreserved, tokencount;
	size_t total_token_len;
};

//Sub list needs the replacement and the index of the token to replace
//in TagLexerResult

struct TagSub
{
	char* string;
	size_t length;
};

struct TagLexerSubList
{
	struct TagSub* subs;
	size_t subscount, subsreserved;
	size_t total_sub_len;
};

struct TagLexer
{
	char* tokenlist;
	size_t tokenlistlen;
};

#ifdef __cplusplus
extern "C" {
#endif

void TagLexer_Init(struct TagLexer* tlexer, const char* tlist);
void TagLexer_SetTagList(struct TagLexer* tlexer, const char* tlist);
void TagLexer_Destroy(struct TagLexer* tlexer);
void TagLexer_Parse(struct TagLexer* tlexer, const char* str, size_t len,
		struct TagLexerResult* result);

void TagLexerSubList_Init(struct TagLexerSubList* sublist);
void TagLexerSubList_Destroy(struct TagLexerSubList* sublist);
struct TagSub* TagLexerSubList_GetSubAt(struct TagLexerSubList* sublist,
					size_t idx);
void TagLexerSubList_AddSub(struct TagLexerSubList* sublist,
			const char* str, size_t len);
char* TagLexerResult_PerformSub(struct TagLexerResult* result,
				struct TagLexerSubList* subs,
				const char* str, size_t len); //Substitute tokens in str with words in subs

void TagLexerResult_Init(struct TagLexerResult* result);
size_t TagLexerResult_GetTokenCount(struct TagLexerResult* result);
const char* TagLexerResult_GetTokenAt(struct TagLexerResult* result,
				size_t idx);
void TagLexerResult_Destroy(struct TagLexerResult* result);

#ifdef __cplusplus
}
#endif


#endif
