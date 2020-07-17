#ifndef CMDLEXER_H_
#define CMDLEXER_H_

#include <stddef.h>

struct SubCommandIndex
{
	size_t index, length;
};

struct Lexer
{
	unsigned char bParseSubcmds;

	char** subcmdmarkers; //Marking characters setting subcommand boundaries
	size_t subcmdmarker_length, subcmdmarker_reserved;
	size_t max_tokens;
};

struct LexerResult
{
	unsigned char bFilled;
	char** tokens;
	size_t token_count, token_reserved;

	size_t* token_starts;
	char* orig_str;

	char** subcommands;
	size_t subcmd_length, subcmd_reserved;

	int refcount;
};

#ifdef __cplusplus
extern "C" {
#endif

void Lexer_Prepare(struct Lexer* lx, unsigned char bParseSubcommands, size_t max_tokens);
void Lexer_Destroy(struct Lexer* lx);
void Lexer_LexString(const struct Lexer* lx, const char* str, size_t len, struct LexerResult* lr);

void Lexer_AddSubcommandMarkers(struct Lexer* lr, const char left, const char right);

void LexerResult_Prepare(struct LexerResult* lr);
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

#ifdef __cplusplus
}
#endif


#endif
