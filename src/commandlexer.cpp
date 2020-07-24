#include "commandlexer.h"

#include "angelscript.h"
#include "lexer.h"
#include "cmdtaglexer.h"
#include "utils.h"

int RegisterLexerClasses(asIScriptEngine* pengine)
{
	int result = 0;
	result = pengine->RegisterObjectType("CommandLexer", 0, asOBJ_REF | asOBJ_NOCOUNT);
	RETURNFAIL_IF(result < 0);


	result = pengine->RegisterObjectType("CommandTagLexerResult", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectBehaviour("CommandTagLexerResult", asBEHAVE_ADDREF, "void f()",
						  asMETHOD(CommandTagLexerResult, AddRef), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectBehaviour("CommandTagLexerResult", asBEHAVE_RELEASE, "void f()",
						  asMETHOD(CommandTagLexerResult, Release), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandTagLexerResult", "uint GetTokenCount()",
					       asMETHOD(CommandTagLexerResult, GetTokenCount), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandTagLexerResult", "string GetTokenAt(uint idx)",
					       asMETHOD(CommandTagLexerResult, GetTokenAt), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectType("CommandLexerResult", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectBehaviour("CommandLexerResult", asBEHAVE_ADDREF,
					     "void f()", asMETHOD(CommandLexerResult, AddRef),
					     asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectBehaviour("CommandLexerResult", asBEHAVE_RELEASE,
					     "void f()", asMETHOD(CommandLexerResult, Release),
					     asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexerResult",
					       "uint GetTokenCount()", asMETHOD(CommandLexerResult, GetTokenCount),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexerResult",
					       "string GetTokenAt(uint idx)", asMETHOD(CommandLexerResult, GetTokenAt),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexerResult",
					       "uint GetSubCmdCount()", asMETHOD(CommandLexerResult, GetSubCmdCount),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexerResult",
					       "string GetSubCmdAt(uint idx)", asMETHOD(CommandLexerResult, GetSubCmdAt),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexerResult",
					       "string GetStringAfterToken(uint idx)", asMETHOD(CommandLexerResult, GetStringAfterToken),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexer",
					       "CommandLexerResult@ LexString(const string& in, uint32 max_tokens, bool bParseSubCmds)",
					       asMETHODPR(CommandLexer, LexString, (const std::string&, size_t, bool), CommandLexerResult*),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexer",
					       "void AddSubcommandMarkers(const string& in, const string& in)",
					       asMETHODPR(CommandLexer, AddSubcommandMarkers, (const std::string&, const std::string&), void),
					       asCALL_THISCALL);

	result = pengine->RegisterObjectMethod("CommandLexer",
					       "void SetTagList(const string& in)",
					       asMETHOD(CommandLexer, SetTagList), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("CommandLexer",
					       "CommandTagLexerResult@ LexStringTags(const string& in)",
					       asMETHOD(CommandLexer, LexStringTags), asCALL_THISCALL);
	return result;
}

CommandLexerResult::CommandLexerResult()
{
	m_result = (struct LexerResult*) malloc(sizeof(struct LexerResult));
	LexerResult_Prepare(m_result);
}

CommandLexerResult::~CommandLexerResult()
{
	LexerResult_Destroy(m_result);
	free(m_result);
}

CommandTagLexerResult::CommandTagLexerResult()
{
	m_result = (struct TagLexerResult*) malloc(sizeof(struct TagLexerResult));
	TagLexerResult_Init(m_result);
}

CommandTagLexerResult::~CommandTagLexerResult()
{
	TagLexerResult_Destroy(m_result);
	free(m_result);
}

size_t CommandLexerResult::GetTokenCount()
{
	return m_result->token_count;
}

std::string CommandLexerResult::GetTokenAt(size_t idx)
{
	return std::string(LexerResult_GetTokenAt(m_result, idx));
}

size_t CommandLexerResult::GetSubCmdCount()
{
	return m_result->subcmd_length;
}

std::string CommandLexerResult::GetSubCmdAt(size_t idx)
{
	return std::string(LexerResult_GetSubcommandAt(m_result, idx));
}

std::string CommandLexerResult::GetStringAfterToken(size_t tokenidx)
{
	return std::string(LexerResult_GetStringAfterToken(m_result, tokenidx));
}

CommandLexer::CommandLexer()
{
	m_taglexer = (struct TagLexer*) malloc(sizeof(struct TagLexer));
	TagLexer_Init(m_taglexer, "");

	m_lexer = (struct Lexer*) malloc(sizeof(struct Lexer));
	memset(m_lexer, 0, sizeof(struct Lexer));
	Lexer_Prepare(m_lexer);
}

CommandLexer::~CommandLexer()
{
	if(m_taglexer)
	{
		TagLexer_Destroy(m_taglexer);
		free(m_taglexer);
		m_taglexer = 0;
	}
	Lexer_Destroy(m_lexer);
	free(m_lexer);
}

CommandLexerResult* CommandLexer::LexString(const std::string& str, size_t max_tokens, bool bParseSubCmds)
{
	CommandLexerResult* result = new CommandLexerResult();
	Lexer_LexString(m_lexer, str.c_str(), str.length(), max_tokens, bParseSubCmds,
			result->m_result);
	return result;
}

void CommandLexer::AddSubcommandMarkers(const std::string& left, const std::string& right)
{
	Lexer_AddSubcommandMarkers(m_lexer, left[0], right[0]);
}

void CommandLexer::SetTagList(const std::string& tags)
{
	TagLexer_SetTagList(m_taglexer, tags.c_str());
}

CommandTagLexerResult* CommandLexer::LexStringTags(const std::string& str)
{
	CommandTagLexerResult* result = new CommandTagLexerResult();
	TagLexer_Parse(m_taglexer, str.c_str(), str.length(), result->m_result);
	return result;
}

size_t CommandTagLexerResult::GetTokenCount()
{
	return m_result->tokencount;
}

std::string CommandTagLexerResult::GetTokenAt(size_t idx)
{
	std::string result = TagLexerResult_GetTokenAt(m_result, idx);
	return result;
}
