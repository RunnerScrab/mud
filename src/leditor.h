#ifndef LEDITOR_H_
#define LEDITOR_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include "charvector.h"
#include "cmdlexer.h"
#include "textutil.h"
#ifdef __cplusplus
}
#endif

struct Client;

struct TextLine
{
	size_t start, length;
};

struct LineEditor
{
	cv_t buffer;
	struct LexerResult lexresult;

	struct TextLine* lines;
	size_t lines_reserved;
	size_t lines_count;
	unsigned char bSaveResult;

	int refcount;
	struct Client* client;
};

typedef enum {LEDITOR_OK = 0, LEDITOR_QUIT = 1, LEDITOR_SAVE = 2} LineEditorResult;

LineEditorResult LineEditor_ProcessInput(struct LineEditor* ple, const char* input, size_t len);
int LineEditor_Init(struct LineEditor* le);
void LineEditor_Destroy(struct LineEditor* le);

struct EditorCommand
{
	const char* name;
	const char* usage;
	const char* desc;
	LineEditorResult (*cmdfp)(struct LineEditor*, struct LexerResult*);
};

LineEditorResult EditorCmdFormat(struct LineEditor* pLE, struct LexerResult* plr);
LineEditorResult EditorCmdDelete(struct LineEditor* pLE, struct LexerResult* plr);
LineEditorResult EditorCmdInsert(struct LineEditor* pLE, struct LexerResult* plr);
LineEditorResult EditorCmdPrint(struct LineEditor* pLE, struct LexerResult* plr);
LineEditorResult EditorCmdQuit(struct LineEditor* pLE, struct LexerResult* plr);
LineEditorResult EditorCmdSave(struct LineEditor* pLE, struct LexerResult* plr);
LineEditorResult EditorCmdClear(struct LineEditor* pLE, struct LexerResult* plr);
LineEditorResult EditorCmdHelp(struct LineEditor* pLE, struct LexerResult* plr);

#ifdef __cplusplus
class asIScriptEngine;
int RegisterLineEditorClass(asIScriptEngine*);
#endif

#endif
