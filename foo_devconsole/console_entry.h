#ifndef console_entry_INCLUDED
#define console_entry_INCLUDED

struct console_entry
{
	t_filetimestamp m_timestamp;
	DWORD m_nThreadId;
	bool m_bMainThread;
	char * m_message;

	void init(const char * p_message, t_size p_message_length);
	void deinit();
};

#endif // console_entry_INCLUDED
