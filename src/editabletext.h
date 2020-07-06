#ifndef EDITABLETEXT_H_
#define EDTIABLETEXT_H_

#include <string>
#include <atomic>
#include "angelscript.h"

#include "as_refcountedobj.h"
#include "rwlockingobj.h"

class EditableText : public AS_RefCountedObj, public RWLockingObject
{
/* std strings as registered by AngelScript are quick for string literals and
 * are easily compatible with C++ functions which use std::strings. Their chief
 * limitation in AngelScript is that they are strictly value types and cannot
 * have any kind of references taken; this is a problem when using them with the
 * player editor.

 * The editor instance can be given a pointer to the std::string, but has no way
 * to no if the pointer will still be valid by the time it wishes to write the
 * changes to the string. Though the problem is simple, it is critical.

 * One possible solution was to only allow editing of a special object type and
 * its subclasses, to which a reference could be kept to keep it and the strings
 * it contained alive as long as needed by the editor. The limitations in
 * flexibility with this approach should be obvious, however.

 * The solution I have chosen to go with is this class, which provides a wrapper
 * for a std string. Not only can the wrapper ensure pointer validity, but it can
 * also serve as storage for editing options peculiar to each string: should the
 * string be limited to having only so many lines? Should it be indented? How wide
 * should the editor format the paragraphs?
 */
public:
	static EditableText* Factory(const std::string& str);
	static EditableText* Factory();
	EditableText();
	EditableText(const std::string& str);

	void assign(const char* str, size_t len);
	EditableText& operator=(const std::string& str);
	EditableText& operator+=(const std::string& str);
	const std::string& GetString() const;

	void SetMaxLines(unsigned int lines)
	{
		m_maxlines = lines;
	}

	void SetIndentationCount(unsigned int count)
	{
		m_indentation_amt = count;
	}

	void SetHyphenationEnabled(bool v)
	{
		m_bAllowHyphenation = v;
	}

	void SetLineWidth(unsigned int v)
	{
		m_line_width = v;
	}

	unsigned int GetMaxLines()
	{
		return m_maxlines;
	}

	unsigned int GetIndentationCount()
	{
		return m_indentation_amt;
	}

	bool GetHyphenationEnabled()
	{
		return m_bAllowHyphenation;
	}

	unsigned int GetLineWidth()
	{
		return m_line_width;
	}

	unsigned int GetMaxLength()
	{
		return m_maxlength;
	}

	void SetMaxLength(unsigned int v)
	{
		m_maxlength = v;
	}




private:
	std::string m_text;
	mutable std::atomic<bool> m_bCopyNeedsUpdate;
	mutable std::string m_cachedret;
	unsigned int m_maxlength;
	unsigned int m_maxlines;
	unsigned int m_indentation_amt;
	unsigned int m_line_width;
	bool m_bAllowHyphenation;
};

int RegisterEditableTextClass(asIScriptEngine* pEngine);

#endif
