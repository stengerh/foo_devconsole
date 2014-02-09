#include "stdafx.h"
#include "resource.h"

#define COMPONENT_NAME    "DevConsole"
#ifndef ASYNC_DEVCONSOLE_UI
#define COMPONENT_VERSION "1.0.2"
#else
#define COMPONENT_VERSION "1.0.2async"
#endif
#define COMPONENT_ABOUT   "(C) 2005 Holger Stenger"

DECLARE_COMPONENT_VERSION(COMPONENT_NAME, COMPONENT_VERSION, COMPONENT_ABOUT)

struct console_entry
{
	t_filetimestamp m_timestamp;
	DWORD m_nThreadId;
	bool m_bMainThread;
	char * m_message;

	void init(const char * p_message, t_size p_message_length);
	void deinit();
};

static pfc::list_t<console_entry> g_message_list;
static critical_section g_lock;

static void g_entry_added_locked();

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

class console_receiver_impl : public console_receiver
{
public:
	virtual void print(const char * p_message, unsigned p_message_length);
};


/////////////////////////////////////////////////
// implementation

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

void console_receiver_impl::print(const char * p_message, unsigned p_message_length)
{
	console_entry entry;
	entry.init(p_message, p_message_length);
	insync(g_lock);
	g_message_list.add_item(entry);
	g_entry_added_locked();
}

static service_factory_single_t<console_receiver_impl> foo_console_receiver;

class CDevConsoleDialog :
	public CDialogImpl<CDevConsoleDialog>
{
public:
	enum {IDD = IDD_DEVCONSOLE};

	BEGIN_MSG_MAP(CDevConsoleDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_CLOSE(OnClose)
		MSG_WM_SIZE(OnSize)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDC_CLEAR, BN_CLICKED, OnClickedClear)
		COMMAND_HANDLER_EX(IDC_FREEZE, BN_CLICKED, OnClickedFreeze)
		COMMAND_HANDLER_EX(IDC_UNFREEZE, BN_CLICKED, OnClickedUnfreeze)
		MSG_WM_MEASUREITEM(OnMeasureItem)
		MSG_WM_DRAWITEM(OnDrawItem)
		COMMAND_HANDLER_EX(IDC_MSGLIST, LBN_SELCHANGE, OnListBoxSelChange)
		if (uMsg == WM_USER)
		{
			SetMsgHandled(TRUE);
			UpdateMsgList();
			lResult = TRUE;
			return TRUE;
		}
		if (uMsg == WM_USER+1)
		{
			SetMsgHandled(TRUE);
			DestroyWindow();
			lResult = TRUE;
			return TRUE;
		}
	END_MSG_MAP()

	CDevConsoleDialog();

private:
	virtual void OnFinalMessage(HWND hWnd);

	LRESULT OnInitDialog(HWND hWnd, LPARAM lParam);
	void OnDestroy();
	void OnClose();
	void OnSize(UINT nType, CSize newSize);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo);
	void OnCancel(UINT nCode, int nId, HWND hWnd);
	void OnClickedClear(UINT nCode, int nId, HWND hWnd);
	void OnClickedFreeze(UINT nCode, int nId, HWND hWnd);
	void OnClickedUnfreeze(UINT nCode, int nId, HWND hWnd);

	void OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	void OnDrawItem(UINT nID, LPDRAWITEMSTRUCT lpDrawItemStruct);

	void DrawMsgListItem(unsigned nIndex, CDCHandle dc, const RECT & rcItem, bool bSelected, bool bExpanded);

	void OnListBoxSelChange(UINT nCode, int nID, HWND hWnd);

	void UpdateMsgList();
	void UpdateTextMetric();

	int GetItemHeight(int nItem, bool bExpanded);

	WTL::CListBox m_wndMsgList;

	dialog_resize_helper m_resize_helper;
	static dialog_resize_helper::param m_resize_helper_table[];

	TEXTMETRIC m_textmetric;
	bool m_textmetric_valid;

	int m_selected_item;

	// number of items in g_message_list that are checked
	unsigned m_checked;

	// number of items in g_message_list that are displayed
	unsigned m_displayed;
};

static const GUID guid_cfg_window_placement_devconsole = { 0x8c99b65a, 0xe753, 0x41fb, { 0x9f, 0x66, 0x71, 0x2d, 0x97, 0xf3, 0x9b, 0xc } };
static cfg_window_placement cfg_window_placement_devconsole(guid_cfg_window_placement_devconsole);

static const GUID guid_cfg_show_devconsole = { 0xe976f86, 0x256f, 0x4421, { 0xa2, 0x86, 0xc1, 0x36, 0x24, 0xb1, 0xaf, 0xc0 } };
static cfg_bool cfg_show_devconsole(guid_cfg_show_devconsole, false);

static bool g_frozen = false;

static HWND g_hWnd = NULL;

dialog_resize_helper::param CDevConsoleDialog::m_resize_helper_table[] =
{
	{IDC_MSGLIST, dialog_resize_helper::XY_SIZE},
	{IDC_FREEZE,  dialog_resize_helper::Y_MOVE},
	{IDC_UNFREEZE,  dialog_resize_helper::Y_MOVE},
	{IDC_CLEAR,   dialog_resize_helper::XY_MOVE},
	{IDCANCEL,    dialog_resize_helper::XY_MOVE},
};

CDevConsoleDialog::CDevConsoleDialog() : m_resize_helper(m_resize_helper_table, tabsize(m_resize_helper_table), 235, 94, 0, 0)
{
	m_textmetric_valid = false;
	m_selected_item = -1;
	m_displayed = m_checked = 0;
}

void CDevConsoleDialog::OnFinalMessage(HWND hWnd)
{
#ifdef ASYNC_DEVCONSOLE_UI
	::PostQuitMessage(0);
#else
	delete this;
#endif
}

void CDevConsoleDialog::UpdateMsgList()
{
	insync(g_lock);
	unsigned count = g_message_list.get_count();
	unsigned n = m_checked;
	if (!g_frozen)
	{
		m_wndMsgList.SetRedraw(FALSE);
		if (n < count)
			m_wndMsgList.InitStorage(count - n, 0);
		for (; n < count; n++)
		{
			int index = m_wndMsgList.AddString(_T(""));
			if (index >= 0)
			{
				m_displayed++;
			}
		}
		m_checked = count;
		m_wndMsgList.SetRedraw(TRUE);
	}

	pfc::string8 caption;
	caption << "DevConsole - " << pfc::format_uint(m_displayed);
	if (m_displayed < count || g_frozen)
	{
		caption << " of " << pfc::format_uint(count) << " messages";
		if (g_frozen)
			caption << " (frozen)";
	}
	else
	{
		caption << " messages";
	}
	uSetWindowText(m_hWnd, caption);
}

void CDevConsoleDialog::UpdateTextMetric()
{
	if (!m_textmetric_valid)
	{
		CWindowDC dc(m_wndMsgList);
		if (!dc.GetTextMetrics(&m_textmetric))
		{
			m_textmetric.tmAscent = 10;
			m_textmetric.tmDescent = 3;
		}
		
		m_textmetric_valid = true;
	}
}

LRESULT CDevConsoleDialog::OnInitDialog(HWND hWnd, LPARAM lParam)
{
	m_wndMsgList.Attach(GetDlgItem(IDC_MSGLIST));

	m_resize_helper.process_message(m_hWnd, WM_INITDIALOG, (WPARAM)hWnd, lParam);
	m_resize_helper.add_sizegrip();

	if (g_frozen)
	{
		::EnableWindow(GetDlgItem(IDC_FREEZE), FALSE);
		::EnableWindow(GetDlgItem(IDC_UNFREEZE), TRUE);
	}
	else
	{
		::EnableWindow(GetDlgItem(IDC_FREEZE), TRUE);
		::EnableWindow(GetDlgItem(IDC_UNFREEZE), FALSE);
	}

	UpdateMsgList();

	cfg_window_placement_devconsole.on_window_creation(m_hWnd);

	{
		insync(g_lock);
		if (g_hWnd == NULL)
			g_hWnd = m_hWnd;
	}

	return TRUE;
}

void CDevConsoleDialog::OnDestroy()
{
	{
		insync(g_lock);
		if (g_hWnd == m_hWnd)
			g_hWnd = NULL;
	}

	m_resize_helper.process_message(m_hWnd, WM_DESTROY, 0, 0);

	cfg_window_placement_devconsole.on_window_destruction(m_hWnd);
}

void CDevConsoleDialog::OnClose()
{
	cfg_show_devconsole = false;
	DestroyWindow();
}

void CDevConsoleDialog::OnSize(UINT nType, CSize newSize)
{
	m_resize_helper.process_message(m_hWnd, WM_SIZE, nType, MAKELPARAM(newSize.cx, newSize.cy));
}

void CDevConsoleDialog::OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo)
{
	m_resize_helper.process_message(m_hWnd, WM_GETMINMAXINFO, 0, (LPARAM)lpMinMaxInfo);
}

void CDevConsoleDialog::OnCancel(UINT nCode, int nId, HWND hWnd)
{
	cfg_show_devconsole = false;
	DestroyWindow();
}

void CDevConsoleDialog::OnClickedClear(UINT nCode, int nId, HWND hWnd)
{
	m_wndMsgList.ResetContent();

	{
		insync(g_lock);
		unsigned count = g_message_list.get_count();
		for (unsigned n = 0; n < count; n++)
		{
			g_message_list[n].deinit();
		}
		g_message_list.remove_all();
	}

	g_frozen = false;
	m_checked = m_displayed = 0;

	UpdateMsgList();
}

void CDevConsoleDialog::OnClickedFreeze(UINT nCode, int nId, HWND hWnd)
{
	g_frozen = true;

	::EnableWindow(GetDlgItem(IDC_FREEZE), FALSE);
	::EnableWindow(GetDlgItem(IDC_UNFREEZE), TRUE);

	UpdateMsgList();
}

void CDevConsoleDialog::OnClickedUnfreeze(UINT nCode, int nId, HWND hWnd)
{
	g_frozen = false;

	::EnableWindow(GetDlgItem(IDC_FREEZE), TRUE);
	::EnableWindow(GetDlgItem(IDC_UNFREEZE), FALSE);

	UpdateMsgList();
}

int CDevConsoleDialog::GetItemHeight(int nItem, bool bExpanded)
{
	UpdateTextMetric();
	int cyHeader = m_textmetric.tmHeight;
	int cyMessage = 0;
	if (bExpanded)
	{
		pfc::string8 message;
		{
			insync(g_lock);
			if (nItem >= 0 && nItem < (int)g_message_list.get_count())
				message = g_message_list[nItem].m_message;
		}
		int lines = 0;
		int ptr = 0, lastline = 0;
		while (message[ptr] != 0)
		{
			if (message[ptr] == '\r' || message[ptr] == '\n')
			{
				if (message[ptr] == '\r' && message[ptr+1] == '\n')
					ptr++;
				ptr++;
				lastline = ptr;
				lines++;
			}
			else ptr++;
		}
		if (ptr > lastline)
			lines++;
#ifdef DEBUG_SPAM
		uOutputDebugString(string_formatter() << "GetItemHeight() computed " << format_int(lines) << " lines for item " << format_int(nItem) << ".\r\n");
#endif
		cyMessage = max(1, lines) * m_textmetric.tmHeight;
	}
	else
	{
		cyMessage = m_textmetric.tmHeight;
	}
	int cyFocusRect = min(1, GetSystemMetrics(SM_CYFOCUSBORDER));
	return cyHeader + cyMessage + 2 * cyFocusRect;
}

void CDevConsoleDialog::OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (nID != IDC_MSGLIST)
	{
		SetMsgHandled(FALSE);
		return;
	}

#ifdef DEBUG_SPAM
	uOutputDebugString(string_formatter() << "OnMeasureItem() with itemID = 0x" << format_hex(lpMeasureItemStruct->itemID, 8) << "\r\n");
#endif

	if (lpMeasureItemStruct->itemID == -1)
	{
		SetMsgHandled(FALSE);
		return;
	}

	int selected_item = m_wndMsgList.GetCurSel();
	bool has_focus = (selected_item != -1) && (lpMeasureItemStruct->itemID == selected_item);

	CRect rcClient;
	m_wndMsgList.GetClientRect(&rcClient);

	lpMeasureItemStruct->itemHeight = GetItemHeight(lpMeasureItemStruct->itemID, has_focus);
	lpMeasureItemStruct->itemWidth = rcClient.Width() + 10;
}

void CDevConsoleDialog::DrawMsgListItem(unsigned nIndex, CDCHandle dc, const RECT & rcItem, bool bSelected, bool bExpanded)
{
	CBrushHandle brushBackground = GetSysColorBrush(COLOR_WINDOW);
	int nBkgColorIndex = COLOR_WINDOW;

	dc.FrameRect(&rcItem, brushBackground);

	UINT textAlignOld = dc.SetTextAlign(TA_BASELINE | TA_LEFT | TA_NOUPDATECP);
	int bkModeOld = dc.SetBkMode(TRANSPARENT);

	insync(g_lock);
	if (nIndex < g_message_list.get_count())
	{
		const console_entry & entry = g_message_list[nIndex];

		int cxFocusRect = min(1, GetSystemMetrics(SM_CXFOCUSBORDER));
		int cyFocusRect = min(1, GetSystemMetrics(SM_CYFOCUSBORDER));
		pfc::string_formatter fmt;

		enum {cfg_show_message_header = 1};

		CRect rcHeader, rcBody;

		rcHeader.left  = rcBody.left  = rcItem.left  + cxFocusRect;
		rcHeader.right = rcBody.right = rcItem.right - cxFocusRect;

		if (cfg_show_message_header)
		{
			rcHeader.top    = rcItem.top   + cyFocusRect;
			rcHeader.bottom = rcHeader.top + m_textmetric.tmHeight;
		}
		else
		{
			rcHeader.top    = rcItem.top;
			rcHeader.bottom = rcHeader.top;
		}

		rcBody.top      = rcHeader.bottom;
		rcBody.bottom   = rcItem.bottom - cyFocusRect;

		// header background
		{
#if 0
			COLORREF colorHeaderLeft, colorHeaderRight;
			colorHeaderLeft = GetSysColor(bSelected ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
			colorHeaderRight = GetSysColor(bSelected ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);

			TRIVERTEX vert[2];
			GRADIENT_RECT gRect;

			vert[0].x     = rcHeader.left;
			vert[0].y     = rcHeader.top;
			vert[0].Red   = GetRValue(colorHeaderLeft) << 8;
			vert[0].Green = GetGValue(colorHeaderLeft) << 8;
			vert[0].Blue  = GetBValue(colorHeaderLeft) << 8;
			vert[0].Alpha = 0x0000;

			vert[1].x     = rcHeader.right;
			vert[1].y     = rcHeader.bottom;
			vert[1].Red   = GetRValue(colorHeaderRight) << 8;
			vert[1].Green = GetGValue(colorHeaderRight) << 8;
			vert[1].Blue  = GetBValue(colorHeaderRight) << 8;
			vert[1].Alpha = 0x0000;

			gRect.UpperLeft  = 0;
			gRect.LowerRight = 1;

			dc.GradientFill(vert, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
#else
			CBrushHandle brushHeader = GetSysColorBrush(bSelected ? COLOR_HIGHLIGHT : COLOR_MENU);
			dc.FillRect(&rcHeader, brushHeader);
#endif
		}

		// timestamp and thread ID
		{
			CRect rcHeaderText = rcHeader;
			rcHeaderText.DeflateRect(2, 0);

			int baseline = rcHeaderText.top + m_textmetric.tmAscent;
		
			COLORREF colorHeaderText;
#if 0
			colorHeaderText = GetSysColor(bSelected ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT);
#else
			colorHeaderText = GetSysColor(bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
#endif
			dc.SetTextColor(colorHeaderText);
			fmt.reset();
			fmt << format_timestamp(entry.m_timestamp) << " ";
			if (entry.m_bMainThread)
				fmt << "main thread";
			else
				fmt << "thread " << pfc::format_uint(entry.m_nThreadId);
			uExtTextOut(dc, rcHeaderText.left, baseline, ETO_CLIPPED, &rcHeaderText, fmt, fmt.length(), NULL);
		}
		
		// message background
		{
			dc.FillRect(&rcBody, brushBackground);
		}


		// message
		{
			CRect rcBodyText = rcBody;
			rcBodyText.DeflateRect(2, 0);

			int baseline = rcBodyText.top + m_textmetric.tmAscent;

			dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
			const char * message = entry.m_message;
			if (bExpanded)
			{
				int ptr = 0, lastline = 0;
				while (message[ptr] != 0)
				{
					if (message[ptr] == '\r' || message[ptr] == '\n')
					{
						uExtTextOut(dc, rcBodyText.left, baseline, ETO_CLIPPED, &rcBodyText,
							&message[lastline], ptr - lastline, NULL);
						baseline += m_textmetric.tmHeight;
						if (message[ptr] == '\r' && message[ptr+1] == '\n')
							ptr++;
						ptr++;
						lastline = ptr;
					}
					else ptr++;
				}
				if (ptr > lastline)
				{
					uExtTextOut(dc, rcBodyText.left, baseline, ETO_CLIPPED, &rcBodyText,
						&message[lastline], ptr - lastline, NULL);
				}
			}
			else
			{
				int ptr = 0, lastline = 0, firstline = -1;
				int lines = 0;
				while (message[ptr] != 0)
				{
					if (message[ptr] == '\r' || message[ptr] == '\n')
					{
						if (firstline == -1)
							firstline = ptr;
						lines++;
						if (message[ptr] == '\r' && message[ptr+1] == '\n')
							ptr++;
						ptr++;
						lastline = ptr;
					}
					else ptr++;
				}
				if (ptr > lastline)
				{
					if (firstline == -1)
						firstline = ptr;
					lines++;
				}
				if (firstline == -1)
					firstline = 0;
				if (lines > 1)
				{
					pfc::string_formatter fmt;
					fmt << "(" << pfc::format_int(lines - 1) << " more line";
					if (lines > 2) fmt << "s";
					fmt << ")";
					CSize size;
					if (uGetTextExtentPoint32(dc, fmt, fmt.length(), &size))
					{
						uExtTextOut(dc, rcBodyText.right - size.cx, baseline, ETO_CLIPPED, &rcBodyText,
							fmt, fmt.length(), NULL);
					}
					rcBodyText.DeflateRect(0, 0, size.cx, 0);
				}
				uExtTextOut(dc, rcBodyText.left, baseline, ETO_CLIPPED, &rcBodyText,
					&message[0], firstline, NULL);
			}
		}
	}

	dc.SetTextAlign(textAlignOld);
	dc.SetBkMode(bkModeOld);
}

void CDevConsoleDialog::OnDrawItem(UINT nID, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nID != IDC_MSGLIST)
	{
		SetMsgHandled(FALSE);
		return;
	}

#ifdef DEBUG_SPAM
	uOutputDebugString(string_formatter() << "OnDrawItem() with itemID = 0x" << format_hex(lpDrawItemStruct->itemID, 8) << "\r\n");
#endif

	CDCHandle dc = lpDrawItemStruct->hDC;
	UINT item = lpDrawItemStruct->itemID;
	const UINT itemState = lpDrawItemStruct->itemState;
	CRect rcItem = lpDrawItemStruct->rcItem;

	bool is_selected = (itemState & ODS_SELECTED) != 0;
	bool is_focused = (item != -1) && (item == m_wndMsgList.GetCurSel());
	COLORREF textColorOld = dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	COLORREF bkColorOld = dc.SetBkColor(GetSysColor(COLOR_WINDOW));
	if (item != (UINT)-1)
	{
		switch (lpDrawItemStruct->itemAction)
		{
		case ODA_DRAWENTIRE:
		case ODA_SELECT:
			{
				DrawMsgListItem(item, dc, rcItem, is_selected, is_focused);

				if ((itemState & ODS_FOCUS) != 0 && (itemState & ODS_NOFOCUSRECT) == 0)
					dc.DrawFocusRect(&lpDrawItemStruct->rcItem);
			}
			break;
		case ODA_FOCUS:
			{
				if ((itemState & ODS_NOFOCUSRECT) == 0)
					dc.DrawFocusRect(&lpDrawItemStruct->rcItem);
			}
			break;
		}
	}
	else
	{
		int nBkgColorIndex = (itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW;
		dc.FillRect(&lpDrawItemStruct->rcItem, nBkgColorIndex);
	}
	dc.SetTextColor(textColorOld);
	dc.SetBkColor(bkColorOld);
}

void CDevConsoleDialog::OnListBoxSelChange(UINT nCode, int nID, HWND hWnd)
{
	m_wndMsgList.SetRedraw(FALSE);
	int selected_item = m_wndMsgList.GetCurSel();
	if (selected_item != m_selected_item)
	{
		if (m_selected_item != -1)
			m_wndMsgList.SetItemHeight(m_selected_item, GetItemHeight(m_selected_item, false));
		m_selected_item = selected_item;
	}
	if (m_selected_item != -1)
		m_wndMsgList.SetItemHeight(m_selected_item, GetItemHeight(m_selected_item, true));
	m_wndMsgList.SetCurSel(selected_item);
	m_wndMsgList.SetRedraw(TRUE);
	m_wndMsgList.RedrawWindow(NULL, NULL, RDW_INVALIDATE);
}

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

static void g_entry_added_locked()
{
	HWND hWnd = g_hWnd;
	if (hWnd != NULL)
	{
		::PostMessage(hWnd, WM_USER, 0, 0);
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
		if (cfg_show_devconsole)
			g_activate(false);
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
			::SendMessage(hWnd, WM_USER+1, 0, 0);
#endif
	}
};

static initquit_factory_t<initquit_devconsole> foo_initquit;
