#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <string>

#include "leditor.h"
#include "talloc.h"
#include "player.h"
#include "angelscript.h"
#include "utils.h"

#include "client.h"
#include "editabletext.h"

struct EditorCommand g_editor_commands[] =
{
	{".c", ".c", "Clear buffer", EditorCmdClear},
	{".d", ".d <line #>", "Delete line #", EditorCmdDelete},
	{".f", ".f", "Format buffer", EditorCmdFormat},
	{".h", ".h", "Show this help", EditorCmdHelp},
	{".i", ".i <line #> <text>", "Insert text before line #",EditorCmdInsert},
	{".p", ".p [r]", "Show buffer; r flag shows raw color tags", EditorCmdPrint},
	{".q", ".q", "Quit without saving", EditorCmdQuit},
	{".s", ".s", "Save buffer and quit", EditorCmdSave}
};

void LineEditor_AddRef(struct LineEditor* le)
{
	asAtomicInc(le->refcount);
}

void LineEditor_Release(struct LineEditor* le)
{
	asAtomicDec(le->refcount);
	if(!le->refcount)
	{
		if(le->pEditTarget)
		{
			le->pEditTarget->Release();
			le->pEditTarget = 0;
		}

		LineEditor_Destroy(le);
		tfree(le);
	}
}

struct LineEditor* LineEditor_Factory()
{
	struct LineEditor* retval = (struct LineEditor*) talloc(sizeof(struct LineEditor));
	LineEditor_Init(retval);
	retval->refcount = 1;
	return retval;
}

void LineEditor_SetPlayerConnection(struct LineEditor* ple, PlayerConnection* playerobj)
{
	ple->client = playerobj->m_pClient;
	playerobj->Release(); //we are not storing any reference to the playerobj
}

void LineEditor_SetEditTarget(struct LineEditor* ple, EditableText* target)
{
	ple->pEditTarget = target;
	const std::string& str = target->GetString();

	ple->max_length = target->GetMaxLength();
	ple->max_lines = target->GetMaxLines();
	ple->rfparams.num_indent_spaces = target->GetIndentationCount();
	ple->rfparams.line_width = target->GetLineWidth();
	ple->rfparams.bAllowHyphenation = target->GetHyphenationEnabled() ? 1 : 0;
	LineEditor_Append(ple, str.c_str(), str.size());
}

void LineEditor_ProcessInputString(struct LineEditor* ple, const std::string& str)
{
	printf("Received: '%s'\n", str.c_str());
	LineEditor_ProcessInput(ple, str.c_str(), str.length());
}

int RegisterLineEditorClass(asIScriptEngine* pengine)
{
	int result = 0;

	result = pengine->RegisterEnum("LineEditorResult");
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterEnumValue("LineEditorResult", "LEDITOR_OK", LEDITOR_OK);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterEnumValue("LineEditorResult", "LEDITOR_SAVE", LEDITOR_SAVE);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterEnumValue("LineEditorResult", "LEDITOR_QUIT", LEDITOR_QUIT);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectType("LineEditor", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectBehaviour("LineEditor", asBEHAVE_FACTORY, "LineEditor@ f()",
						  asFUNCTION(LineEditor_Factory), asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectBehaviour("LineEditor", asBEHAVE_ADDREF, "void f()",
						  asFUNCTION(LineEditor_AddRef), asCALL_CDECL_OBJFIRST);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectBehaviour("LineEditor", asBEHAVE_RELEASE, "void f()",
						  asFUNCTION(LineEditor_Release), asCALL_CDECL_OBJFIRST);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("LineEditor", "void SetPlayerConnection(PlayerConnection@ conn)",
					       asFUNCTION(LineEditor_SetPlayerConnection), asCALL_CDECL_OBJFIRST);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("LineEditor", "void SetEditTarget(EditableText@ conn)",
					       asFUNCTION(LineEditor_SetEditTarget), asCALL_CDECL_OBJFIRST);
	RETURNFAIL_IF(result < 0);

	result = pengine->RegisterObjectMethod("LineEditor", "LineEditorResult ProcessInput(const string& in)",
					       asFUNCTION(LineEditor_ProcessInputString), asCALL_CDECL_OBJFIRST);
	RETURNFAIL_IF(result < 0);

	return result;
}

int LineEditor_Init(struct LineEditor* le)
{
	Lexer_Prepare(&le->lexer);
	LexerResult_Prepare(&le->lexresult);

	le->bSaveResult = 0;
	cv_init(&le->buffer, 512);
	le->lines_reserved = 128;
	le->lines = (struct TextLine*) talloc(sizeof(struct TextLine) * le->lines_reserved);
	memset(le->lines, 0, sizeof(struct TextLine) * le->lines_reserved);
	le->lines_count = 0;
	le->client = 0;
	le->pEditTarget = 0;
	le->max_lines = 0;
	le->max_length = 0;

	le->rfparams.num_indent_spaces = 0;
	le->rfparams.line_width = 80;
	le->rfparams.bAllowHyphenation = 0;

	return 0;
}

void LineEditor_Destroy(struct LineEditor* le)
{
	tfree(le->lines);
	cv_destroy(&le->buffer);

	LexerResult_Destroy(&le->lexresult);
	Lexer_Destroy(&le->lexer);
}

void LineEditor_RebuildLineIndices(struct LineEditor* le, size_t from_idx);

void LineEditor_Append(struct LineEditor* le, const char* data, size_t datalen)
{
	size_t nlcount = CountNewlines(data, datalen);
	if(le->max_length && datalen + le->buffer.length > le->max_length)
	{
		Client_Sendf(le->client, "Would exceed the maximum length of this text (%lu)\r\n",
			     le->max_length);
	}
	else if(le->max_lines && le->lines_count + nlcount > le->max_lines)
	{
		Client_Sendf(le->client, "Would exceed the maximum number of lines (%lu).\r\n",
			le->max_lines);
	}
	else
	{
		cv_appendstr(&le->buffer, data, datalen);
		LineEditor_RebuildLineIndices(le, (le->lines_count > 1) ? (le->lines_count - 1) : 0);
	}
}

void LineEditor_RebuildLineIndices(struct LineEditor* le, size_t from_idx)
{
	const char* buf = le->buffer.data;
	size_t len = le->buffer.length;

	const char* p = buf;


	if(from_idx > le->lines_count)
	{
		return;
	}

	size_t offset = le->lines[from_idx].start;
	le->lines_count = from_idx;

	while(p && offset < len)
	{
		p = (const char*) memchr(buf + offset, '\n', len - offset);
		if(p)
		{
			if(le->lines_count >= le->lines_reserved)
			{
				size_t oldsize = le->lines_reserved;
				le->lines_reserved <<= 1;
				le->lines = (struct TextLine*) trealloc(le->lines, sizeof(struct TextLine) * le->lines_reserved);
				memset(&le->lines[oldsize], 0, sizeof(struct TextLine) * (le->lines_reserved - oldsize));
			}

			le->lines[le->lines_count].start = (0 == le->lines_count) ? 0 :
				(le->lines[le->lines_count - 1].start + le->lines[le->lines_count - 1].length + 1);
			le->lines[le->lines_count].length = (p - buf) - offset;

			++le->lines_count;
			offset = (p - buf) + 1;
		}
		else if(!le->lines_count)
		{
			//If we found no newlines in the entire buffer

			le->lines[le->lines_count].start = 0;
			le->lines[le->lines_count].length = len;
			++le->lines_count;
		}

	}
}

void LineEditor_InsertAt(struct LineEditor* le, size_t line_idx, const char* data, size_t datalen)
{
	if(line_idx > le->lines_count)
	{
		LineEditor_Append(le, data, datalen);
		return;
	}

	size_t startidx = le->lines[line_idx].start;
	el_t* copystart = &le->buffer.data[startidx];
	size_t copylen = le->buffer.length - startidx;
	el_t* temp = (el_t*) talloc(copylen);

	memcpy(temp, copystart, copylen);
	memset(copystart, 0, copylen);
	LineEditor_Append(le, data, datalen);
	LineEditor_Append(le, temp, copylen);
	LineEditor_RebuildLineIndices(le, line_idx);

	tfree(temp);
}

void LineEditor_DeleteLine(struct LineEditor* le, size_t line_idx)
{
	if(line_idx > le->lines_count)
	{
		return;
	}

	size_t startidx = le->lines[line_idx].start + le->lines[line_idx].length + 1;

	el_t* copystart = &le->buffer.data[startidx];
	size_t copylen = le->buffer.length - startidx;
	el_t* temp = (el_t*) talloc(copylen);

	memcpy(temp, copystart, copylen);

	memset(copystart, 0, copylen);
	memcpy(&le->buffer.data[le->lines[line_idx].start], temp, copylen);

	LineEditor_RebuildLineIndices(le, line_idx);

	tfree(temp);
}

LineEditorResult EditorCmdPrint(struct LineEditor* pLE, struct LexerResult* plr)
{
	if(pLE->buffer.length)
	{
		unsigned char bHideColor = (LexerResult_GetTokenCount(plr) > 1
					    && LexerResult_GetTokenAt(plr, 1)[0] == 'r');
		void (*SendFn)(struct Client*, const char* fmt, ...)
			= bHideColor ? Client_SendfShowColorTags : Client_Sendf;

		size_t idx = 0;
		size_t linebuf_reserved = 512;
		char* linebuf = (char*) talloc(linebuf_reserved);
		memset(linebuf, 0, linebuf_reserved);

		for(; idx < pLE->lines_count; ++idx)
		{
			size_t linelen = pLE->lines[idx].length;
			if(linebuf_reserved < linelen)
			{
				linebuf_reserved <<= 1;
				linebuf = (char*) trealloc(linebuf, linebuf_reserved);
			}
			linebuf[linelen] = 0;
			memcpy(linebuf, &pLE->buffer.data[pLE->lines[idx].start], linelen);
			SendFn(pLE->client, "%02lu] %s\n", idx, linebuf);
		}
		tfree(linebuf);
	}
	else
	{
		Client_Sendf(pLE->client, "Buffer is empty.\n");
	}

	return LEDITOR_OK;
}

LineEditorResult EditorCmdFormat(struct LineEditor* pLE, struct LexerResult* plr)
{
	cv_t out;

	cv_init(&out, pLE->buffer.length);
	ReflowText(pLE->buffer.data, pLE->buffer.length, &out, &pLE->rfparams);

	cv_swap(&pLE->buffer, &out);

	LineEditor_RebuildLineIndices(pLE, 0);
	cv_destroy(&out);
	return LEDITOR_OK;
}

LineEditorResult EditorCmdDelete(struct LineEditor* pLE, struct LexerResult* plr)
{
	size_t tokencount = LexerResult_GetTokenCount(plr);
	if(tokencount < 2)
	{
		Client_Sendf(pLE->client, "*Too few arguments.\n");
		return LEDITOR_OK;
	}

	char* pos_str = LexerResult_GetTokenAt(plr, 1);
	size_t delete_idx = atoi(pos_str);

	LineEditor_DeleteLine(pLE, delete_idx);
	return LEDITOR_OK;
}

LineEditorResult EditorCmdInsert(struct LineEditor* pLE, struct LexerResult* plr)
{
	size_t tokencount = LexerResult_GetTokenCount(plr);
	if(tokencount < 2)
	{
		Client_Sendf(pLE->client, "*Too few arguments.\n");
		return LEDITOR_OK;
	}

	char* pos_str = LexerResult_GetTokenAt(plr, 1);
	size_t insert_idx = atoi(pos_str);

	if(tokencount > 2)
	{
		char* insertstr = LexerResult_GetStringAfterToken(plr, 2);
		size_t insertstr_len = strlen(insertstr);
		char* temp = (char*) talloc(insertstr_len + 2);
		memset(temp, 0, insertstr_len + 2);
		snprintf(temp, insertstr_len + 2, "%s\n", insertstr);
		LineEditor_InsertAt(pLE, insert_idx, temp, insertstr_len + 2);
		tfree(temp);
	}
	return LEDITOR_OK;
}

LineEditorResult EditorCmdQuit(struct LineEditor* pLE, struct LexerResult* plr)
{
	pLE->bSaveResult = 0;
	return LEDITOR_QUIT;
}

LineEditorResult EditorCmdSave(struct LineEditor* pLE, struct LexerResult* plr)
{
	printf("Trying to run editor save command.\n");
	pLE->bSaveResult = 1;
	if(pLE->pEditTarget)
	{
		printf("Editor save command running.\n");
		pLE->pEditTarget->assign(pLE->buffer.data, pLE->buffer.length);
	}
	return LEDITOR_SAVE;
}

LineEditorResult EditorCmdClear(struct LineEditor* pLE, struct LexerResult* plr)
{
	pLE->bSaveResult = 0;
	cv_clear(&pLE->buffer);
	LineEditor_RebuildLineIndices(pLE, 0);
	return LEDITOR_OK;
}

LineEditorResult EditorCmdHelp(struct LineEditor* pLE, struct LexerResult* plr)
{
	size_t idx = 0;
	size_t len = 8;
	Client_Sendf(pLE->client, "%20s   %-20s\n", "Command/Usage", "Description");
	Client_Sendf(pLE->client, "-----------------------------------------------------\n");
	for(; idx < len; ++idx)
	{
		Client_Sendf(pLE->client, "%20s   %-20s\n", g_editor_commands[idx].usage,
			     g_editor_commands[idx].desc);
	}
	return LEDITOR_OK;
}

int EditorCmdCmp(const void* pA, const void* pB)
{
	struct EditorCommand* pCmd = ((struct EditorCommand*) pB);
	return strcmp((const char*) pA, pCmd->name);
}

LineEditorResult LineEditor_ProcessInput(struct LineEditor* ple, const char* input, size_t len)
{
	if('.' == input[0])
	{
		LexerResult_Clear(&ple->lexresult);
		Lexer_LexString(&ple->lexer, input, len, 2, 0, &ple->lexresult);

		char* cmdstr = LexerResult_GetTokenAt(&ple->lexresult, 0);

		struct EditorCommand* pCmd = (struct EditorCommand*)
			bsearch(cmdstr, g_editor_commands,
				8,
				sizeof(struct EditorCommand),
				EditorCmdCmp);
		if(pCmd)
		{
			return pCmd->cmdfp(ple, &ple->lexresult);
		}
		else
		{
			Client_Sendf(ple->client, "*Unrecognized command.\n");
			return LEDITOR_OK;
		}
	}
	else
	{
		LineEditor_Append(ple, input, len);
		return LEDITOR_OK;
	}
	return LEDITOR_OK;
}
