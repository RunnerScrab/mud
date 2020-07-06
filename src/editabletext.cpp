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

	result = pEngine->RegisterObjectMethod("EditableText", "const string& GetString()",
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

	result = pEngine->RegisterObjectMethod("EditableText", "void SetLineWidth(uint32 v)",
					       asMETHOD(EditableText, SetLineWidth), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "void SetMaxLength(uint32 v)",
					       asMETHOD(EditableText, SetMaxLength), asCALL_THISCALL);
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

	result = pEngine->RegisterObjectMethod("EditableText", "uint32 GetLineWidth()",
					       asMETHOD(EditableText, GetLineWidth), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "uint32 GetMaxLength()",
					       asMETHOD(EditableText, GetMaxLength), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = pEngine->RegisterObjectMethod("EditableText", "const string& opImplConv()",
					       asMETHOD(EditableText, GetString), asCALL_THISCALL);
	return result;
}

EditableText::EditableText()
{
	m_maxlines = 0;
	m_maxlength = 0;
	m_indentation_amt = 0;
	m_line_width = 80;
	m_bAllowHyphenation = false;
	m_bCopyNeedsUpdate = false;
}

EditableText::EditableText(const std::string& str) : EditableText()
{
	m_text = str;
}

EditableText* EditableText::Factory()
{
	EditableText* retval = new EditableText();
	return retval;
}

EditableText* EditableText::Factory(const std::string& str)
{
	EditableText* retval = new EditableText(str);
	return retval;
}

EditableText& EditableText::operator=(const std::string& str)
{
	WriteLock();
	m_text = str;
	printf("Op Assigning string!\n");
	m_bCopyNeedsUpdate = true;
	Unlock();
	return *this;
}

void EditableText::assign(const char* str, size_t len)
{
	WriteLock();
	printf("Assigning string!\n");
	m_text.assign(str, len);
	m_bCopyNeedsUpdate = true;
	Unlock();
}

EditableText& EditableText::operator+=(const std::string& str)
{
	m_text += str;
	return *this;
}

const std::string& EditableText::GetString()
{
	if(true == m_bCopyNeedsUpdate)
	{
		ReadLock();
		m_cachedret = m_text;
		printf("Cached string:\n%s\n", m_cachedret.c_str());
		m_bCopyNeedsUpdate = false;
		Unlock();
	}
	return m_cachedret;
}
