#ifndef COMMANDLEXER_H_
#define COMMANDLEXER_H_

#include <string>
#include "as_refcountedobj.h"

struct Lexer;
struct LexerResult;
struct TagLexer;
struct TaggedToken;
class asIScriptEngine;

class CommandLexerResult : public AS_RefCountedObj
{
	friend class CommandLexer;
public:
	CommandLexerResult();
	~CommandLexerResult();

	size_t GetTokenCount();
	std::string GetTokenAt(size_t idx);

	size_t GetSubCmdCount();
	std::string GetSubCmdAt(size_t idx);
	std::string GetStringAfterToken(size_t tokenidx);

private:
	struct LexerResult* m_result;
};

//The Angelscript interface to the lexer functions
class CommandLexer
{
public:
	CommandLexer();
	~CommandLexer();
	CommandLexerResult* LexString(const std::string& str, size_t max_tokens, bool bParseSubCmds);
	void AddSubcommandMarkers(const std::string& left, const std::string& right);
private:
	struct Lexer* m_lexer;
};

int RegisterLexerClasses(asIScriptEngine* pengine);

#endif
