#include "stdafx.h"
#include "config_devconsole.h"

static const GUID guid_cfg_show_devconsole = { 0xe976f86, 0x256f, 0x4421, { 0xa2, 0x86, 0xc1, 0x36, 0x24, 0xb1, 0xaf, 0xc0 } };
cfg_bool cfg_show_devconsole(guid_cfg_show_devconsole, false);

HWND g_hWnd = NULL;

pfc::list_t<console_entry> g_message_list;
critical_section g_lock;
