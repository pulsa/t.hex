#ifndef _BARVIEWCTRL_H_
#define _BARVIEWCTRL_H_

// custom wxCommandEvent-derived event fired when user clicks the bar view
DECLARE_EVENT_TYPE(wxEVT_BAR_SELECTED, -1)

class thBarViewCtrl : public wxWindow
{
public:
    thBarViewCtrl(wxWindow *parent, int id = -1,
        wxPoint pos = wxDefaultPosition,
        wxSize size = wxDefaultSize,
        wxString title = _T("thBarViewCtrl"));

    virtual ~thBarViewCtrl();

    virtual void UpdateView(bool bUpdate = true);
    virtual void Mark(int bar);
    virtual int HitTest(wxPoint pt);
    virtual int GetBarArea(int sel, int &width); // returns left edge
    virtual int GetBarMid(int left, int width) { return left + width - 1 - (width / 2); }
    virtual int GetBarHeight(int sel);
    virtual int GetBarCount();
    virtual wxColour GetBarColour(int sel);
    virtual wxPen GetBarPen(int sel) { return wxPen(GetBarColour(sel)); }
    virtual wxBrush GetBarBrush(int sel) { return wxBrush(GetBarColour(sel)); }
    virtual bool IsBarSelectable(int WXUNUSED(sel)) { return false; }

    virtual bool BeginDraw() { return true; }
    virtual void Draw(wxDC &dc);

    virtual wxString GetTitle() { return m_title; }
    void SetSelection(int sel) { Mark(sel); m_sel = sel; }
    int GetSelection() { return m_sel; }

protected:
    void Paint(wxDC &dc, int sel = -1);
    void Mark(wxDC &dc, int bar);
    void Erase(wxDC &dc);
    void SendSelectedEvent();

    virtual wxRect GetBorderRect() { wxRect rc = m_rcClient; rc.Deflate(6); return rc; }
    
    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnMouseDown(wxMouseEvent &event);
    void OnMouseLeave(wxMouseEvent &event);
    void OnErase(wxEraseEvent &event);
    DECLARE_EVENT_TABLE()

protected:
    DblBufWindow dbw;
    int m_mark, m_sel, m_mouseOverItem;
    wxRect m_rcClient, m_rcBorder;
    wxString m_title;
};

#endif // _BARVIEWCTRL_H_
