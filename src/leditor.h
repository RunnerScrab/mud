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

typedef struct EditableText EditableText;

struct LineEditor
{
	cv_t buffer;
	struct Lexer lexer;
	struct LexerResult lexresult;

	struct TextLine* lines;
	size_t lines_reserved;
	size_t lines_count;
	unsigned char bSaveResult;

	int refcount;
	struct Client* client;

	size_t max_lines; // 0 means there is no limitation
	size_t max_length; // 0 means there is no limitation

	struct ReflowParameters rfparams;
	EditableText* pEditTarget;
};

typedef enum {LEDITOR_OK = 0, LEDITOR_QUIT = 1, LEDITOR_SAVE = 2} LineEditorResult;

LineEditorResult LineEditor_ProcessInput(struct LineEditor* ple, const char* input, size_t len);
int LineEditor_Init(struct LineEditor* le);
void LineEditor_Destroy(struct LineEditor* le);
void LineEditor_Append(struct LineEditor* le, const char* data, size_t datalen);

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
