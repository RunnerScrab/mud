#include <angelscript.h>
#include "./angelscriptsdk/add_on/scriptstdstring/scriptstdstring.h"
#include "./angelscriptsdk/add_on/scriptarray/scriptarray.h"
//#include "./angelscriptsdk/angelscript/AngelScript-JIT-Compiler/as_jit.h"
#include <as_jit.h>
#include <cstdio>
#include <cstdlib>

void as_MessageCallback(const asSMessageInfo* msg, void* param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING )
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION )
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);

}

void as_print(std::string& in)
{
	printf(in.c_str());
}

int main(void)
{
	asIScriptEngine* engine = asCreateScriptEngine();
	asCJITCompiler* jit = new asCJITCompiler(0);

	engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
	engine->SetJITCompiler(jit);

	if(engine == 0)
	{
		printf("Failed to initialize AS engine.\n");
		return -1;
	}

	RegisterStdString(engine);
	RegisterScriptArray(engine, true);
	engine->RegisterGlobalFunction("void print(string& in)",
				asFUNCTION(as_print), asCALL_CDECL);
	engine->SetMessageCallback(asFUNCTION(as_MessageCallback), 0, asCALL_CDECL);

	std::string script;

	FILE* fp = fopen("test.as", "rb");
	if(!fp)
	{
		printf("Could not open script file.\n");
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	script.resize(len);

	if(fread(&script[0], len, 1, fp) <= 0)
	{
		printf("Error reading script file.\n");
		return -1;
	}
	fclose(fp);

	asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	if(mod->AddScriptSection("script", &script[0], len) < 0)
	{
		printf("AddScriptSection() failed.\n");
		return -1;
	}

	if(mod->Build() < 0)
	{
		printf("Build() failed.\n");
		return -1;
	}

	jit->finalizePages();

	asIScriptFunction* func = engine->GetModule(0)->GetFunctionByDecl(
		"void run()");

	if(0 == func)
	{
		printf("Couldn't load function run()\n");
		return -1;
	}
	asIScriptContext* ctx = engine->CreateContext();
	ctx->Prepare(func);
	ctx->Execute();


	ctx->Release();
	engine->Release();
	delete jit;
	return 0;
}
