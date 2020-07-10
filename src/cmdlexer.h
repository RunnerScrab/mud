#ifndef CMDLEXER_H_
#define CMDLEXER_H_

#include <stddef.h>

struct LexerResult
{
	char** tokens;
	size_t token_count, token_reserved;

	size_t* token_starts;
	char* orig_str;

	unsigned char bParseSubcmds;

	char** subcommands;
	size_t subcmd_length, subcmd_reserved;

	char** subcmdmarkers; //Marking characters setting subcommand boundaries
	size_t subcmdmarker_length, subcmdmarker_reserved;
};

void LexerResult_AddSubcommandMarkers(struct LexerResult* lr, const char left, const char right);

void LexerResult_Prepare(struct LexerResult* lr, unsigned char bParseSubcommands);

void LexerResult_AddToken(struct LexerResult* lr, size_t start, const char* token,
			size_t tokenlen);

void LexerResult_Destroy(struct LexerResult* lr);
void LexerResult_Clear(struct LexerResult* lr);

size_t LexerResult_GetTokenCount(struct LexerResult* lr);
char* LexerResult_GetTokenAt(struct LexerResult* lr, size_t idx);

size_t LexerResult_GetSubcommandCount(struct LexerResult* lr);
char* LexerResult_GetSubcommandAt(struct LexerResult* lr, size_t idx);

//token = 0 gets the entire string (with subcommands parsed out)
char* LexerResult_GetStringAfterToken(struct LexerResult* lr, size_t token);
void LexerResult_LexString(struct LexerResult* lr, const char* str, size_t len, size_t max_tokens);

#endif
