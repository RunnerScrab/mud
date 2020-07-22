#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>

struct SubCommandIndex
{
	size_t index, length;
};

struct Lexer
{
	char** subcmdmarkers; //Marking characters setting subcommand boundaries
	size_t subcmdmarker_length, subcmdmarker_reserved;
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

};

#ifdef __cplusplus
extern "C" {
#endif

void Lexer_Prepare(struct Lexer* lx);
void Lexer_Destroy(struct Lexer* lx);
void Lexer_LexString(const struct Lexer* lx, const char* str, size_t len,
			size_t max_tokens, unsigned char bParseSubCommands,
			struct LexerResult* lr);

void Lexer_AddSubcommandMarkers(struct Lexer* lr, const char left, const char right);

void LexerResult_Prepare(struct LexerResult* lr);
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
