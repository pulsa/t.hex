
#include "precomp.h"
#include "utils.h"
#include "BarViewCtrl.h"

//****************************************************************************
//****************************************************************************
// thBarViewCtrl
//****************************************************************************
//****************************************************************************

//class thBar
//{
//public:
//    thBar(wxColour Clr, int Height = -1, bool Selectable = true)
//        : clr(Clr), height(Height), selectable(Selectable)
//    { }
//
//    wxColour clr;
//    int height;
//    bool selectable;
//};


DEFINE_EVENT_TYPE(wxEVT_BAR_SELECTED)

BEGIN_EVENT_TABLE(thBarViewCtrl, wxWindow)
    EVT_PAINT(OnPaint)
    EVT_SIZE(OnSize)
    EVT_MOTION(OnMouseMove)
    EVT_LEFT_DOWN(OnMouseDown)
    EVT_LEAVE_WINDOW(OnMouseLeave)
    EVT_ERASE_BACKGROUND(OnErase)
END_EVENT_TABLE()

void thBarViewCtrl::OnErase(wxEraseEvent &event)
{
}

thBarViewCtrl::thBarViewCtrl(
    wxWindow *parent,
    int id /*= -1*/,
    wxPoint pos /*= wxDefaultPosition*/,
    wxSize size /*= wxDefaultSize*/,
    wxString title /*= "thBarViewCtrl"*/)
: wxWindow(parent, id, pos, size/*, wxFULL_REPAINT_ON_RESIZE*/)
{
    SetCursor(*wxCROSS_CURSOR);
}

thBarViewCtrl::~thBarViewCtrl()
{
}

void thBarViewCtrl::UpdateView(bool bUpdate /*= true*/)
{
    m_rcClient = GetClientRect();
    m_rcBorder = GetBorderRect();
    dbw.UpdateBitmapSize(m_rcClient.GetSize());

    // Draw bars to off-screen bitmap.
    wxMemoryDC dc;
    dc.SelectObject(dbw.m_bmp);

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    if (BeginDraw())
    {
        Draw(dc);
    }

    m_mark = m_sel = m_mouseOverItem = -1;

    // update the screen
    dc.SelectObject(wxNullBitmap); //! why is this here?
    Refresh(bUpdate);
    //if (bUpdate)
    //    Update();
}

void thBarViewCtrl::Draw(wxDC &dc)
{
    //int width = wxMax(m_rcBorder.width, 256); //! todo: put other bars somewhere else
    int bottom = m_rcBorder.y + m_rcBorder.height;

    int bar_left, bar_width, bar_height;
    for (int i = 0; i < GetBarCount(); i++)
    {
        bar_height = GetBarHeight(i);
        if (bar_height)
        {
            dc.SetBrush(GetBarBrush(i));
            bar_left = GetBarArea(i, bar_width);
            dc.DrawRectangle(bar_left, bottom - bar_height, bar_width, bar_height);
        }
    }
}

void thBarViewCtrl::OnSize(wxSizeEvent &event)
{
    UpdateView(false);
}

void thBarViewCtrl::OnPaint(wxPaintEvent &event)
{
    wxPaintDC paintDC(this);
    wxMemoryDC memDC;
    memDC.SelectObject(dbw.m_bmp);
    paintDC.Blit(0, 0, m_rcClient.width, m_rcClient.height, &memDC, 0, 0);
}

void thBarViewCtrl::OnMouseMove(wxMouseEvent &event)
{
    m_mouseOverItem = HitTest(event.GetPosition());
    Mark(m_mouseOverItem);

    // if mouse button down, fire event to select item in list view
    if (event.LeftIsDown() && m_mouseOverItem != m_sel)
    {
        m_sel = m_mouseOverItem;
        SendSelectedEvent();
    }
}

void thBarViewCtrl::OnMouseDown(wxMouseEvent &event)
{
    if (m_sel != m_mouseOverItem)
    {
        m_sel = m_mouseOverItem;
        SendSelectedEvent();
    }
}

void thBarViewCtrl::SendSelectedEvent()
{
    wxCommandEvent cmd(wxEVT_BAR_SELECTED, GetId());
    cmd.SetEventObject(this);
    cmd.SetInt(m_mouseOverItem);
    wxWindow *parent = GetParent();
    if (parent)
       parent->GetEventHandler()->ProcessEvent(cmd);
}

void thBarViewCtrl::OnMouseLeave(wxMouseEvent &event)
{
    Mark(GetSelection());
}

void thBarViewCtrl::Mark(int bar)
{
    if (bar == m_mark)
        return;

    //PRINTF("Mark(%d), m_mark = %d\n", bar, m_mark);

    // add the old bar and the new bar to the update region
    wxRect rc = m_rcClient;
    if (m_mark >= 0)
    {
        rc.x = GetBarArea(m_mark, rc.width);
        rc.x -= 4; rc.width += 8;
        RefreshRect(rc, false);
    }

    if (bar >= 0)
    {
        rc.x = GetBarArea(bar, rc.width);
        rc.x -= 4; rc.width += 8;
        RefreshRect(rc, false);
    }

    wxMemoryDC memDC;
    memDC.SelectObject(dbw.m_bmp);
    Erase(memDC); // sets m_mark = -1
    Mark(memDC, bar); // sets m_mark = bar
    memDC.SelectObject(wxNullBitmap);
    //Update();
}

void thBarViewCtrl::Erase(wxDC &dc)
{
    if (m_mark < 0)
        return;

    int bar_width, bar_left = GetBarArea(m_mark, bar_width);
    int bar_mid = bar_left + bar_width / 2;
    int bottom = m_rcBorder.y + m_rcBorder.height;

    dc.SetBrush(wxBrush(GetBackgroundColour()));
    dc.SetPen(*wxTRANSPARENT_PEN);

    // erase the top marker
    dc.DrawRectangle(bar_mid - 4, 0, 8, 6);

    // erase the bottom marker
    dc.DrawRectangle(bar_mid - 4, bottom + 1, 8, 4);

    // erase the empty space above the bar
    int bar_height = GetBarHeight(m_mark);
    dc.DrawRectangle(bar_left, m_rcBorder.y, bar_width, m_rcBorder.height - bar_height);

    // redraw the bar
    dc.SetBrush(GetBarBrush(m_mark));
    dc.DrawRectangle(bar_left, bottom - bar_height, bar_width, bar_height);

    m_mark = -1;
}

void thBarViewCtrl::Mark(wxDC &dc, int bar)
{
    if (bar < 0)
        return;
    m_mark = bar;

    int bar_width, bar_left = GetBarArea(bar, bar_width);
    int bar_mid = bar_left + bar_width / 2;
    int bottom = m_rcBorder.y + m_rcBorder.height;

    dc.SetPen(GetBarPen(bar));
    dc.SetBrush(GetBarBrush(bar));

    wxPoint pts[3];
    pts[0] = wxPoint(-3, 3);
    pts[1] = wxPoint(0, 0);
    pts[2] = wxPoint(3, 3);
    dc.DrawPolygon(3, pts, bar_mid, bottom + 1);

    pts[0] = wxPoint(-3, -3);
    pts[2] = wxPoint(3, -3);
    dc.DrawPolygon(3, pts, bar_mid, m_rcBorder.y - 2);

    dc.SetPen(*wxTRANSPARENT_PEN);

    //! This looks bad with cross cursor.  But it's still useful.
    RECT rcInvert = {bar_left, m_rcBorder.y, bar_left + bar_width, bottom};
    InvertRect((HDC)dc.GetHDC(), &rcInvert);
}

int thBarViewCtrl::HitTest(wxPoint pt)
{
    int width = wxMax(m_rcBorder.width, GetBarCount());
    if (pt.x < m_rcBorder.x || pt.x >= m_rcBorder.x + m_rcBorder.width)
        return -1;
    int bar = (pt.x - m_rcBorder.x) * GetBarCount() / width;
    // Bar may be one too low because of truncation, so check and adjust.
    int left = m_rcBorder.x + (bar + 1) * width / GetBarCount();
    if (left <= pt.x)
        bar++;
    return bar;
}

int thBarViewCtrl::GetBarArea(int sel, int &bar_width) // returns left edge
{
    int count = GetBarCount();
    int width = wxMax(m_rcBorder.width, count);
    if (sel < 0 || sel >= GetBarCount())
    {
        bar_width = 0;
        return 0;
    }

    int left = sel * width / count;
    bar_width = ((sel + 1) * width / count) - left;
    return m_rcBorder.x + left;
}

int thBarViewCtrl::GetBarCount()
{
    return 256;
}

wxColour thBarViewCtrl::GetBarColour(int sel)
{
    return wxColour(sel, sel, sel);
}

int thBarViewCtrl::GetBarHeight(int sel)
{
    return 1 + (sel * (m_rcBorder.height - 1) / GetBarCount());
}
