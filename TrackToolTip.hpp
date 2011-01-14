#ifndef _TRACKTOOLTIP_HPP
#define _TRACKTOOLTIP_HPP

/* This is a replacement for the wxToolTip class.
   While wxToolTip is very good at placing itself so it doesn't interfere with
   other windows, it doesn't let the application move it to follow the pointer.
   Microsoft has good sample code for this in the 2003 Platform SDK documentation,
   which I copied here.

   Adam Bailey     5/27/04
*/


class TrackToolTip //: public wxToolTip
{
public:
   TrackToolTip(wxWindow *owner, wxString text, bool bEnable = false);
   WXHWND GetToolTipCtrl();

   void Track(int x, int y);
   void Enable(BOOL bEnable);
   void SetText(wxString text);

protected:
   HWND m_hWnd;
   HWND m_hWndParent;
   //BOOL m_bIsVisible;
   wxString m_text;
};

#endif  // _TRACKTOOLTIP_HPP
