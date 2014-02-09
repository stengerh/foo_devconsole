#include "stdafx.h"
#include "format_timestamp.h"

format_timestamp::format_timestamp(t_filetimestamp p_timestamp, bool p_is_utc)
{
	FILETIME filetime = *(FILETIME*)&p_timestamp;
	if (p_is_utc)
	{
		FILETIME localfiletime;
		if (FileTimeToLocalFileTime(&filetime, &localfiletime))
			filetime = localfiletime;
	}
	SYSTEMTIME systemtime;
	if (FileTimeToSystemTime(&filetime, &systemtime))
	{
		m_buffer
			<< pfc::format_uint(systemtime.wYear, 4) << "-"
			<< pfc::format_uint(systemtime.wMonth, 2) << "-"
			<< pfc::format_uint(systemtime.wDay, 2) << " "
			<< pfc::format_uint(systemtime.wHour, 2) << ":"
			<< pfc::format_uint(systemtime.wMinute, 2) << ":"
			<< pfc::format_uint(systemtime.wSecond, 2) << "."
			<< pfc::format_uint(systemtime.wMilliseconds, 3);
	}
	else
	{
		m_buffer << "<invalid time>";
	}
}

pfc::string_base & operator<<(pfc::string_base & p_fmt, const console_entry & p_val)
{
	p_fmt << format_timestamp(p_val.m_timestamp) << " ";
	if (p_val.m_bMainThread)
		p_fmt << "main thread";
	else
		p_fmt << "thread " << pfc::format_uint(p_val.m_nThreadId);
	p_fmt << " " << p_val.m_message;
	return p_fmt;
}
