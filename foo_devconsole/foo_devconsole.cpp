#include "stdafx.h"
#include "console_entry.h"
#include "CDevConsoleDialog.h"
#include "config_devconsole.h"

#define COMPONENT_NAME    "DevConsole"
#ifndef ASYNC_DEVCONSOLE_UI
#define COMPONENT_VERSION "1.0.2"
#else
#define COMPONENT_VERSION "1.0.2async"
#endif
#define COMPONENT_ABOUT   "(C) 2005 Holger Stenger"

DECLARE_COMPONENT_VERSION(COMPONENT_NAME, COMPONENT_VERSION, COMPONENT_ABOUT)

/////////////////////////////////////////////////
// implementation

#ifdef ASYNC_DEVCONSOLE_UI
static DWORD WINAPI DevConsoleThreadProc(LPVOID lpParam)
{
#ifdef _DEBUG
	console::print("DevConsole UI thread starting");
#endif

	HANDLE hThread = GetCurrentThread();

	CDevConsoleDialog dlg;
	HWND hWnd = dlg.Create(HWND_DESKTOP);
	if (hWnd != NULL)
		::ShowWindow(hWnd, SW_SHOW);

	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			if (dlg.IsWindow())
				dlg.DestroyWindow();
			break;
		}
		else
		{
			if (hWnd == NULL || !IsDialogMessage(hWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

#ifdef _DEBUG
	console::print("DevConsole UI thread terminating");
#endif

	return msg.wParam;
}
#endif

void g_entry_added_locked()
{
	HWND hWnd = g_hWnd;
	if (hWnd != NULL)
	{
		::PostMessage(hWnd, CDevConsoleDialog::DCM_REQUEST_UPDATE, 0, 0);
	}
}

static void g_activate(bool p_take_focus = true)
{
	if (!core_api::assert_main_thread()) return;

	cfg_show_devconsole = true;
#ifdef ASYNC_DEVCONSOLE_UI
	HWND hWnd = NULL;
	{
		insync(g_lock);
		hWnd = g_hWnd;
	}
	if (hWnd == NULL)
	{
		DWORD dwThreadId = 0;
		HANDLE hThread = ::CreateThread(NULL, 0, DevConsoleThreadProc, NULL, CREATE_SUSPENDED, &dwThreadId);
		if (hThread != NULL)
		{
			::SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
			::ResumeThread(hThread);
			::CloseHandle(hThread);
		}
	}
	else
	{
		::ShowWindowAsync(hWnd, SW_SHOW);
		::SetForegroundWindow(hWnd);
	}
#else
	if (g_hWnd == NULL)
	{
		CDevConsoleDialog * dlg = new CDevConsoleDialog();
		dlg->Create(core_api::get_main_window());
	}
	if (g_hWnd != NULL)
	{
		::ShowWindow(g_hWnd, SW_SHOW);
		::SetForegroundWindow(g_hWnd);
	}
#endif
}

static const GUID guid_main_devconsole = { 0x9b2096d4, 0xec68, 0x493c, { 0x93, 0xd4, 0xf8, 0x8d, 0x35, 0x8c, 0xad, 0x78 } };

class mainmenu_commands_devconsole : public mainmenu_commands
{
public:
	virtual t_uint32 get_command_count()
	{
		return 1;
	}

	virtual GUID get_command(t_uint32 p_index)
	{
		if (p_index == 0)
			return guid_main_devconsole;
		return pfc::guid_null;
	}

	virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
	{
		if (p_index == 0)
			p_out = "DevConsole";
	}

	virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
	{
		return false;
	}

	virtual GUID get_parent()
	{
		return mainmenu_groups::view;
	}

	virtual t_uint32 get_sort_priority()
	{
		return sort_priority_dontcare;
	}

	virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
	{
		if (p_index == 0)
				g_activate();
	}
};

static mainmenu_commands_factory_t<mainmenu_commands_devconsole> foo_mainmenu_commands;

class initquit_devconsole : public initquit
{
public:
	virtual void FB2KAPI on_init()
	{
		if (!core_api::is_quiet_mode_enabled())
		{
			if (cfg_show_devconsole)
			{
				g_activate(false);
			}
		}
	}

	virtual void FB2KAPI on_quit()
	{
#ifdef ASYNC_DEVCONSOLE_UI
		HWND hWnd;
		{
			insync(g_lock);
			hWnd = g_hWnd;
		}
		if (hWnd != 0)
		{
			::SendMessage(hWnd, CDevConsoleDialog::DCM_REQUEST_CLOSE, 0, 0);
		}
#endif
	}
};

static initquit_factory_t<initquit_devconsole> foo_initquit;
