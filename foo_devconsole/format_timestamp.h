#ifndef format_timestamp_INCLUDED
#define format_timestamp_INCLUDED

#include "console_entry.h"

class format_timestamp
{
public:
	format_timestamp(t_filetimestamp p_timestamp, bool p_is_utc = true);
	format_timestamp(const format_timestamp & p_source) {*this = p_source;}
	inline const char * get_ptr() const {return m_buffer;}
	inline operator const char*() const {return m_buffer;}
private:
	pfc::string_formatter m_buffer;
};

pfc::string_base & operator<<(pfc::string_base & p_fmt, const console_entry & p_val);

#endif format_timestamp_INCLUDED
