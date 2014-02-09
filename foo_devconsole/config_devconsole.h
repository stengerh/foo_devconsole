#ifndef config_devconsole_INCLUDED
#define config_devconsole_INCLUDED

#include "foobar2000/SDK/foobar2000.h"
#include "console_entry.h"

extern cfg_bool cfg_show_devconsole;

extern HWND g_hWnd;
extern pfc::list_t<console_entry> g_message_list;
extern critical_section g_lock;

#endif // config_devconsole_INCLUDED
