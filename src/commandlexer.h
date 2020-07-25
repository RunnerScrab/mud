#ifndef COMMANDLEXER_H_
#define COMMANDLEXER_H_

#include <string>
#include "as_refcountedobj.h"

struct Lexer;
struct LexerResult;
struct TagLexer;
struct TagLexerResult;
struct TaggedToken;

class asIScriptEngine;
class CScriptArray;

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

class CommandTagLexerResult : public AS_RefCountedObj
{
	friend class CommandLexer;
public:
	CommandTagLexerResult();
	~CommandTagLexerResult();
	size_t GetTokenCount();
	std::string GetTokenAt(size_t idx);
	std::string PerformSubs(CScriptArray* subs, const std::string& str);
private:
	struct TagLexerResult* m_result;
};

//The Angelscript interface to the lexer functions
class CommandLexer
{
	//This class is given to Angelscript as a singleton and is used to
	//group lexer functions and store their "global" settings all in one
	//place.

	//Their global settings include types of subcommand markers, e.g: [], ()
	//Command tag symbol list, e.g: ~!@#$%
public:
	CommandLexer();
	~CommandLexer();
	CommandLexerResult* LexString(const std::string& str, size_t max_tokens, bool bParseSubCmds);
	void AddSubcommandMarkers(const std::string& left, const std::string& right);

	void SetTagList(const std::string& tags);
	CommandTagLexerResult* LexStringTags(const std::string& str);
private:
	struct Lexer* m_lexer;
	struct TagLexer* m_taglexer;
};

int RegisterLexerClasses(asIScriptEngine* pengine);

#endif
