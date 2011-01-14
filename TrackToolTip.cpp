#include "precomp.h"
#include "TrackToolTip.hpp"
#include <commctrl.h>

TrackToolTip::TrackToolTip(wxWindow *owner, wxString text, bool bEnable /*= false*/)
{
    INITCOMMONCONTROLSEX icex;
    HWND        hwndTT;
    TOOLINFO    ti;
    // Load the ToolTip class from the DLL.
    icex.dwSize = sizeof(icex);
    icex.dwICC  = ICC_BAR_CLASSES;

    //m_bIsVisible = false;

    if(!InitCommonControlsEx(&icex))
       return;
	   
    // Create the ToolTip control.
    hwndTT = CreateWindow(TOOLTIPS_CLASS, TEXT(""),
                          WS_POPUP,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL, (HMENU)NULL, GetModuleHandle(NULL),
                          NULL);

    m_hWndParent = (HWND)owner->GetHandle();
    m_text = text;

    // Prepare TOOLINFO structure for use as tracking ToolTip.
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd   = m_hWndParent;
    ti.uId    = (UINT)ti.hwnd;
    ti.hinst  = GetModuleHandle(NULL);
    ti.lpszText  = (LPSTR)m_text.c_str();
    ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0; 

    // Add the tool to the control, displaying an error if needed.
    if(!SendMessage(hwndTT,TTM_ADDTOOL,0,(LPARAM)&ti)){
       wxLogDebug(_T("Failed to create the tooltip '%s'"), m_text.c_str());
       return;
    }

    Enable(bEnable);

    m_hWnd = hwndTT;
}

void TrackToolTip::Track(int x, int y)
{
   SendMessage(m_hWnd, TTM_TRACKPOSITION, 0, MAKELPARAM(x, y));
}

void TrackToolTip::Enable(BOOL bEnable)
{
   TOOLINFO ti;
   ti.cbSize = sizeof(TOOLINFO);
   ti.uFlags = TTF_IDISHWND;
   ti.hwnd   = m_hWndParent;
   ti.uId    = (UINT)ti.hwnd;

   SendMessage(m_hWnd, TTM_TRACKACTIVATE, bEnable, (LPARAM)&ti);
}

void TrackToolTip::SetText(wxString text)
{
   m_text = text;
   TOOLINFO ti;
   ti.cbSize = sizeof(TOOLINFO);
   ti.uFlags = TTF_IDISHWND;
   ti.hwnd   = m_hWndParent;
   ti.uId    = (UINT)ti.hwnd;
   ti.hinst  = GetModuleHandle(NULL);
   ti.lpszText = (LPSTR)m_text.c_str();
   SendMessage(m_hWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
}
