#include "stdafx.h"
#include "resource.h"
#include "CDevConsoleDialog.h"
#include "format_timestamp.h"
#include "config_devconsole.h"

static const GUID guid_cfg_window_placement_devconsole = { 0x8c99b65a, 0xe753, 0x41fb, { 0x9f, 0x66, 0x71, 0x2d, 0x97, 0xf3, 0x9b, 0xc } };
static cfg_window_placement cfg_window_placement_devconsole(guid_cfg_window_placement_devconsole);

static bool g_frozen = false;

static bool g_always_expanded = false;

//#define DEBUG_SPAM

dialog_resize_helper::param CDevConsoleDialog::m_resize_helper_table[] =
{
	{IDC_MSGLIST, dialog_resize_helper::XY_SIZE},
	{IDC_FREEZE,  dialog_resize_helper::Y_MOVE},
	{IDC_UNFREEZE,  dialog_resize_helper::Y_MOVE},
	{IDC_CLEAR,   dialog_resize_helper::XY_MOVE},
	{IDCANCEL,    dialog_resize_helper::XY_MOVE},
};

CDevConsoleDialog::CDevConsoleDialog():
	m_resize_helper(m_resize_helper_table, tabsize(m_resize_helper_table), 235, 94, 0, 0)
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

LRESULT CDevConsoleDialog::OnUpdateRequest(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SetMsgHandled(TRUE);
	UpdateMsgList();
	return 0;
}

LRESULT CDevConsoleDialog::OnCloseRequest(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SetMsgHandled(TRUE);
	DestroyWindow();
	return 0;
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
	HICON icon = static_api_ptr_t<ui_control>()->get_main_icon();
	SetIcon(icon);

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
	Invalidate();
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
	bExpanded = bExpanded || g_always_expanded;

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
		uDebugLog() << "GetItemHeight() computed " << pfc::format_int(lines) << " lines for item " << pfc::format_int(nItem));
#endif
		cyMessage = max(1, lines) * m_textmetric.tmHeight;
	}
	else
	{
		cyMessage = m_textmetric.tmHeight;
	}
	int cyFocusRect = min(1, GetSystemMetrics(SM_CYFOCUSBORDER));
	int height = cyHeader + cyMessage + 2 * cyFocusRect;
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb761348(v=vs.85).aspx
	return min(height, 255);
}

void CDevConsoleDialog::OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (nID != IDC_MSGLIST)
	{
		SetMsgHandled(FALSE);
		return;
	}

#ifdef DEBUG_SPAM
	uDebugLog() << "OnMeasureItem() with itemID = 0x" << pfc::format_hex(lpMeasureItemStruct->itemID, 8));
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
	bExpanded = bExpanded || g_always_expanded;

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
			CBrushHandle brushHeader = GetSysColorBrush(bSelected ? COLOR_HIGHLIGHT : COLOR_MENU);
			dc.FillRect(&rcHeader, brushHeader);
		}

		// timestamp and thread ID
		{
			CRect rcHeaderText = rcHeader;
			rcHeaderText.DeflateRect(2, 0);

			int baseline = rcHeaderText.top + m_textmetric.tmAscent;
		
			COLORREF colorHeaderText = GetSysColor(bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
			dc.SetTextColor(colorHeaderText);
			fmt.reset();
			fmt << format_timestamp(entry.m_timestamp) << " ";
			fmt << "thread " << pfc::format_uint(entry.m_nThreadId);
			if (entry.m_bMainThread)
				fmt << " (main thread)";
				
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
	uDebugLog() << "OnDrawItem() with itemID = 0x" << pfc::format_hex(lpDrawItemStruct->itemID, 8));
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
				DrawMsgListItem(item, dc, rcItem, is_selected, g_always_expanded || is_focused);

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
		{
			UpdateItemHeight(m_selected_item, false);
		}
		m_selected_item = selected_item;
		if (m_selected_item != -1)
		{
			UpdateItemHeight(m_selected_item, true);
		}
	}
	m_wndMsgList.SetCurSel(selected_item);
	m_wndMsgList.SetRedraw(TRUE);
	m_wndMsgList.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void CDevConsoleDialog::UpdateItemHeight(int nItem, bool bExpanded)
{
	UINT height = GetItemHeight(nItem, bExpanded);
	int rv = m_wndMsgList.SetItemHeight(nItem, height);
	if (rv == LB_ERR)
		uDebugLog() << "Failed to set height of item " << nItem << " to " << height;
}
