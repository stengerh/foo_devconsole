#include "stdafx.h"
#include "console_receiver_impl.h"
#include "console_entry.h"
#include "config_devconsole.h"

void g_entry_added_locked();

void console_receiver_impl::print(const char * p_message, unsigned p_message_length)
{
	console_entry entry;
	entry.init(p_message, p_message_length);
	insync(g_lock);
	g_message_list.add_item(entry);
	g_entry_added_locked();
}

static service_factory_single_t<console_receiver_impl> foo_console_receiver;
