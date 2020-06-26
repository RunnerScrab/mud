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
};

int LineEditor_ProcessInput(struct LineEditor* ple, const char* input, size_t len);
int LineEditor_Init(struct LineEditor* le);
void LineEditor_Destroy(struct LineEditor* le);

struct EditorCommand
{
	const char* name;
	const char* usage;
	const char* desc;
	int (*cmdfp)(struct LineEditor*, struct LexerResult*);
};

int EditorCmdFormat(struct LineEditor* pLE, struct LexerResult* plr);
int EditorCmdDelete(struct LineEditor* pLE, struct LexerResult* plr);
int EditorCmdInsert(struct LineEditor* pLE, struct LexerResult* plr);
int EditorCmdPrint(struct LineEditor* pLE, struct LexerResult* plr);
int EditorCmdQuit(struct LineEditor* pLE, struct LexerResult* plr);
int EditorCmdSave(struct LineEditor* pLE, struct LexerResult* plr);
int EditorCmdClear(struct LineEditor* pLE, struct LexerResult* plr);
int EditorCmdHelp(struct LineEditor* pLE, struct LexerResult* plr);

#ifdef __cplusplus
class asIScriptEngine;
int RegisterLineEditorClass(asIScriptEngine*);
#endif

#endif
