#include "editabletext.h"
#include "utils.h"

int RegisterEditableTextClass(asIScriptEngine* pEngine)
{
	int result = 0;

	result = pEngine->RegisterObjectType("EditableText", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectBehaviour("EditableText", asBEHAVE_FACTORY,
						  "EditableText@ f()",
						  asFUNCTIONPR(EditableText::Factory, (void), EditableText*),
						  asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectBehaviour("EditableText", asBEHAVE_FACTORY,
						  "EditableText@ f(const string& in)",
						  asFUNCTIONPR(EditableText::Factory, (const std::string&), EditableText*),
						  asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectBehaviour("EditableText", asBEHAVE_ADDREF,
						  "void f()", asMETHOD(EditableText, AddRef),
						  asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectBehaviour("EditableText", asBEHAVE_RELEASE,
						  "void f()", asMETHOD(EditableText, Release),
						  asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectBehaviour("EditableText", asBEHAVE_GET_WEAKREF_FLAG,
						  "int &f()",
						  asMETHOD(EditableText, GetWeakRefFlag),
						  asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "EditableText& opAssign(const string& in)",
					       asMETHODPR(EditableText, operator=,
							  (const std::string&), EditableText&),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "EditableText& opAddAssign(const string& in)",
					       asMETHODPR(EditableText, operator+=,
							  (const std::string&), EditableText&),
					       asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "string& GetString()",
					       asMETHOD(EditableText, GetString), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "void SetMaxLines(int lines)",
					       asMETHOD(EditableText, SetMaxLines), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "void SetIndentationCount(int count)",
					       asMETHOD(EditableText, SetIndentationCount), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "void SetHyphenationEnabled(bool v)",
					       asMETHOD(EditableText, SetHyphenationEnabled), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "uint32 GetMaxLines()",
					       asMETHOD(EditableText, GetMaxLines), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "uint32 GetIndentationCount()",
					       asMETHOD(EditableText, GetIndentationCount), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "bool GetHyphenationEnabled()",
					       asMETHOD(EditableText, GetHyphenationEnabled), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "string& opImplConv()",
					       asMETHOD(EditableText, GetConstString), asCALL_THISCALL);
	return result;
}

EditableText::EditableText() :
	m_maxlines(0), m_indentation_amt(0),
	m_bAllowHyphenation(false)
{

}

EditableText::EditableText(const std::string& str) : EditableText()
{
	m_text = str;
}

EditableText* EditableText::Factory()
{
	EditableText* retval = new EditableText();
	retval->AddRef();
	return retval;
}

EditableText* EditableText::Factory(const std::string& str)
{
	EditableText* retval = new EditableText(str);
	retval->AddRef();
	return retval;
}

EditableText& EditableText::operator=(const std::string& str)
{
	m_text = str;
	return *this;
}

EditableText& EditableText::operator+=(const std::string& str)
{
	m_text += str;
	return *this;
}

std::string& EditableText::GetString()
{
	return m_text;
}

const std::string& EditableText::GetConstString()
{
	return m_text;
}
