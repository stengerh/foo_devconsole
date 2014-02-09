#ifndef CDevConsoleDialog_INCLUDED
#define CDevConsoleDialog_INCLUDED

#include "resource.h"

class CDevConsoleDialog :
	public CDialogImpl<CDevConsoleDialog>
{
public:
	enum {IDD = IDD_DEVCONSOLE};
	enum {
		DCM_REQUEST_UPDATE = WM_USER,
		DCM_REQUEST_CLOSE = WM_USER + 1,
	};

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
		MESSAGE_HANDLER_EX(DCM_REQUEST_UPDATE, OnUpdateRequest)
		MESSAGE_HANDLER_EX(DCM_REQUEST_CLOSE, OnCloseRequest)
	END_MSG_MAP()

	CDevConsoleDialog();

	static void Activate();

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

	LRESULT OnUpdateRequest(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCloseRequest(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void UpdateMsgList();
	void UpdateTextMetric();

	int GetItemHeight(int nItem, bool bExpanded);
	void UpdateItemHeight(int nItem, bool bExpanded);

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

#endif // CDevConsoleDialog_INCLUDED
