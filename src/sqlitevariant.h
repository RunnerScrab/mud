#ifndef SQLITEVARIANT_H_
#define SQLITEVARIANT_H_

#include <cstdio>
#include <string>
#include <vector>
#include <variant>
#include <cstring>
#include <sqlite3.h>

class SQLiteVariant
{
public:
	typedef enum {VARNONE = 0, VARINT, VARREAL, VARBLOB, VARTEXT} StoredType;
private:
	StoredType m_storedtype;
	size_t m_datalen;
	size_t m_reservedlen;

	union VARUNION
	{
		char* as_blob;
		unsigned long long as_uint64;
		long long as_int64;
		double as_double;
		unsigned int as_uint;
		int as_int;
	} m_data;

	void FreeBlobHeapMemIfAllocated()
	{
		if((VARBLOB == m_storedtype || VARTEXT == m_storedtype) && m_data.as_blob)
		{
			free(m_data.as_blob);
		}
	}
public:
	void ForceSetType(StoredType vartype)
	{
		//Only to be used as a placeholder in case the user tries to
		//create a table before providing values to deduce types
		m_storedtype = vartype;
	}

	bool HasType() const
	{
		return VARNONE != m_storedtype;
	}

	StoredType GetType() const
	{
		return m_storedtype;
	}

	std::string GetTypeAsString()
	{
		static const char* typestrs[] = {"NULL", "INT", "REAL", "BLOB", "TEXT"};
		return std::string(typestrs[static_cast<int>(m_storedtype)]);
	}

	size_t GetDataLength() const
	{
		return m_datalen;
	}


	void ClearValue()
	{
		//Don't clear the reserved len or release any heap memory
		m_datalen = 0;
		switch(m_storedtype)
		{
		case VARTEXT:
		case VARBLOB:
			memset(m_data.as_blob, 0, m_reservedlen);
			break;
		default:
			memset(&m_data, 0, sizeof(union VARUNION));
			break;
		}

	}

	SQLiteVariant()
	{
		m_storedtype = VARNONE;
		m_datalen = 0;
		m_reservedlen = 0;
		memset(&m_data, 0, sizeof(union VARUNION));
	}

	SQLiteVariant(const SQLiteVariant& other)
	{
		m_datalen = other.m_datalen;
		m_storedtype = other.m_storedtype;
		m_reservedlen = other.m_reservedlen;

		if(VARTEXT == m_storedtype || VARBLOB == m_storedtype)
		{
			m_data.as_blob = (char*) malloc(m_reservedlen);
			memcpy(m_data.as_blob, other.m_data.as_blob, m_reservedlen);
		}
		else
		{
			m_data = other.m_data;
		}
	}

	SQLiteVariant(SQLiteVariant&& other)
	{
		m_datalen = std::move(other.m_datalen);
		m_reservedlen = std::move(other.m_reservedlen);
		m_storedtype = std::move(other.m_storedtype);
		m_data.as_blob = std::move(other.m_data.as_blob);

		other.m_data.as_blob = 0;
		other.m_storedtype = VARNONE;
	}

	SQLiteVariant& operator=(const SQLiteVariant& other)
	{
		m_datalen = other.m_datalen;
		m_reservedlen = other.m_reservedlen;
		m_storedtype = other.m_storedtype;

		if(VARTEXT == m_storedtype || VARBLOB == m_storedtype)
		{
			m_data.as_blob = (char*) malloc(m_reservedlen);
			memcpy(m_data.as_blob, other.m_data.as_blob, m_reservedlen);
		}
		else
		{
			m_data = other.m_data;
		}

		return *this;
	}

	~SQLiteVariant()
	{
		if((VARBLOB == m_storedtype || VARTEXT == m_storedtype) && m_data.as_blob)
		{
			free(m_data.as_blob);
		}
	}

	void SetValue(int v)
	{
		FreeBlobHeapMemIfAllocated();
		m_data.as_int = v;
		m_datalen = sizeof(int);
		m_storedtype = VARINT;
	}

	void SetValue(long long v)
	{
		FreeBlobHeapMemIfAllocated();
		m_data.as_int64 = v;
		m_datalen = sizeof(long long);
		m_storedtype = VARINT;
	}

	void SetValue(unsigned int v)
	{
		FreeBlobHeapMemIfAllocated();
		m_data.as_uint = v;
		m_datalen = sizeof(unsigned int);
		m_storedtype = VARINT;
	}

	void SetValue(unsigned long long v)
	{
		FreeBlobHeapMemIfAllocated();
		m_data.as_uint64 = v;
		m_datalen = sizeof(unsigned long long);
		m_storedtype = VARINT;
	}

	void SetValue(float v)
	{
		FreeBlobHeapMemIfAllocated();
		m_data.as_double = static_cast<double>(v);
		m_datalen = sizeof(float);
		m_storedtype = VARREAL;
	}

	void SetValue(double v)
	{
		FreeBlobHeapMemIfAllocated();
		m_data.as_double = v;
		m_datalen = sizeof(double);
		m_storedtype = VARREAL;
	}

	void SetValue(const std::string& v)
	{
		if(VARBLOB == m_storedtype || VARTEXT == m_storedtype)
		{
			if(m_data.as_blob)
			{
				if((v.length() + 1) > m_reservedlen)
				{
					m_reservedlen = (v.length() + 1) << 1;
					m_data.as_blob = (char*) realloc(m_data.as_blob, m_reservedlen);
				}
			}
		}

		m_datalen = v.length() + 1;

		if(!m_data.as_blob)
		{
			m_data.as_blob = (char*) malloc(m_datalen);
			m_reservedlen = m_datalen;
		}
		memset(m_data.as_blob, 0, m_reservedlen);
		strncpy(m_data.as_blob, v.c_str(), m_datalen);
		m_storedtype = VARTEXT;
	}

	void SetValue(const char* data, size_t len)
	{
		if(VARBLOB == m_storedtype || VARTEXT == m_storedtype)
		{
			if(m_data.as_blob)
			{
				if(len > m_reservedlen)
				{
					m_reservedlen = len << 1;
					m_data.as_blob = (char*) realloc(m_data.as_blob, m_reservedlen);
				}
			}
		}

		m_datalen = len;

		if(!m_data.as_blob)
		{
			m_data.as_blob = (char*) malloc(m_datalen);
			m_reservedlen = m_datalen;
		}

		memset(m_data.as_blob, 0, m_reservedlen);
		memcpy(m_data.as_blob, data, m_datalen);
		m_storedtype = VARBLOB;
	}

	bool GetValue(int& v)
	{
		if(VARINT == m_storedtype && m_datalen == sizeof(int))
		{
			v = m_data.as_int;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetValue(long long& v)
	{
		if(VARINT == m_storedtype && m_datalen == sizeof(long long))
		{
			v = m_data.as_int64;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetValue(unsigned int& v)
	{
		if(VARINT == m_storedtype && m_datalen == sizeof(unsigned int))
		{
			v = m_data.as_uint;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetValue(unsigned long long& v)
	{
		if(VARINT == m_storedtype && m_datalen == sizeof(unsigned long long))
		{
			v = m_data.as_uint64;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetValue(float& v)
	{
		if(VARINT == m_storedtype && m_datalen == sizeof(float))
		{
			v = static_cast<float>(m_data.as_double);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetValue(double& v)
	{
		if(VARREAL == m_storedtype && m_datalen == sizeof(double))
		{
			v = m_data.as_double;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetValue(std::string& v)
	{
		if(VARTEXT == m_storedtype)
		{
			v = std::string(m_data.as_blob);
			return true;
		}
		else
		{
			return false;
		}
	}

	const char* GetValueBlobPtr() const
	{
		return m_data.as_blob;
	}

	bool GetValue(std::vector<char>& v)
	{
		if(VARBLOB == m_storedtype)
		{
			v.resize(m_datalen);
			memcpy(&v[0], m_data.as_blob, m_datalen);
			return true;
		}
		else
		{
			return false;
		}
	}
};

#endif
