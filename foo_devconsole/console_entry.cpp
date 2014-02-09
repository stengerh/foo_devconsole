#include "stdafx.h"
#include "console_entry.h"

void console_entry::init(const char * p_message, unsigned p_message_length)
{
	FILETIME message_time;
	GetSystemTimeAsFileTime(&message_time);
	m_timestamp = *(t_filetimestamp*)&message_time;

	m_nThreadId = GetCurrentThreadId();
	m_bMainThread = core_api::is_main_thread();

	m_message = pfc::strdup_n(p_message, p_message_length);
}

void console_entry::deinit()
{
	free(m_message);
}
