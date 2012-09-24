#include "precomp.h"
#include "toolwnds.h"
#include "hexwnd.h"
#include "hexdoc.h"
#include "resource.h"
#include "settings.h"
#include "utils.h"
#include <FlexLexer.h>
#include <fstream>
#include "wx/treelistctrl.h"
#include "datasource.h"

//#include <GL/gl.h>
//#include <GL/glu.h>

#define new New

DataView::DataView(wxWindow *win)
: win(win)
{
    m_hw = NULL;
    m_iSelStart = m_iSelSize = 0xFFFFFFFFFFFFFFFFL;
    m_iChangeIndex = 0xFFFFFFFFL;
    bSkipUpdate = false;

    //if (win)
    //{
    //   win->Connect(wxEVT_CHAR, wxCharEventHandler(DataView::OnEsc));
    //}
}

DataView::~DataView()
{
    //! todo: remove from thFrame::m_views?
}

void DataView::OnSetFocus(wxFocusEvent &event)
{
    if (m_hw)
        m_hw->SetCurrentView(this);
}

void DataView::OnKillFocus(wxFocusEvent &event)
{
    //if (m_hw)
    //    m_hw->SetCurrentView(NULL);
}


//****************************************************************************
//****************************************************************************
// FileMap
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(FileMap, wxWindow)
    EVT_PAINT(OnPaint)
    EVT_SIZE(OnSize)
    EVT_LEFT_DOWN(OnMouseDown)
    EVT_MOTION(OnMouseMove)
    EVT_LEFT_UP(OnMouseUp)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_COMMAND_RANGE(100, 200, wxEVT_COMMAND_MENU_SELECTED, OnCommand)
#if wxCHECK_VERSION(2, 7, 0)
    EVT_MOUSE_CAPTURE_LOST(OnCaptureLost)
#endif
END_EVENT_TABLE()

FileMap::FileMap(wxWindow *parent, wxSize size /*= wxDefaultSize*/)
: DataView(this),
wxWindow(parent, -1, wxDefaultPosition, (size == wxDefaultSize) ? wxSize(38, -1) : size)
{
    forceOrient = 0;
    m_hw = NULL;

    brMem = wxBrush(wxColour(255, 128, 128));
    brFile = wxBrush(wxColour(128, 255, 128));
    //brFill = wxBrush(wxColour(128, 128, 255));
    brForward = wxBrush(wxColour(255, 255, 128));
    brBackward = wxBrush(wxColour(128, 128, 255));
    //HBRUSH black = CreateSolidBrush(RGB(40, 40, 40));
    brBack = *wxWHITE_BRUSH;
    brBorder = *wxBLACK_BRUSH;
    borderPen = *wxBLACK_PEN;

    popup = new wxMenu();
    popup->Append(IDM_SELECTBLOCK, _T("Select Block"));
    popup->AppendSeparator();
    popup->Append(IDM_BLOCKSTART, _T("Start of Block"));
    popup->Append(IDM_BLOCKEND, _T("End of Block"));
    popup->Append(IDM_SELSTART, _T("Start of Selection"));
    popup->Append(IDM_SELEND, _T("End of Selection"));
    popup->AppendSeparator();
    popup->AppendCheckItem(IDM_VERTICAL, _T("Vertical Alignment"));
    popup->AppendCheckItem(IDM_HORIZONTAL, _T("Horizontal Alignment"));
    popup->AppendCheckItem(IDM_AUTO, _T("Automatic Alignment"));
    popup->AppendSeparator();
    popup->AppendCheckItem(IDM_CLOSE, _T("Close"));
}

FileMap::~FileMap()
{
    delete popup;
}

void FileMap::OnSize(wxSizeEvent &event)
{
    UpdateBitmapSize(event.GetSize());
    memDC.SelectObject(m_bmp);

    UpdateView(m_hw, DV_VIEWSIZE);
}

void FileMap::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    dc.Blit(0, 0, m_bmp.GetWidth(), m_bmp.GetHeight(), &memDC, 0, 0);
}

void FileMap::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
    this->m_hw = hw;
    if (!hw)
        return;

    wxRect rc;
    int width = m_bmp.GetWidth(), height = m_bmp.GetHeight();

    if (flags & DV_VIEWSIZE)
    {
        if (forceOrient)
            orient = forceOrient;
        else if (width > height)
            orient = wxHORIZONTAL;
        else
            orient = wxVERTICAL;

        if (orient == wxHORIZONTAL)
            m_majorSize = width;
        else
            m_majorSize = height;
    }

    int *major1, *major2;
    int *minor1, *minor2;
    int minorSize;
    if (orient == wxHORIZONTAL)
    {
        major1 = &rc.x;
        major2 = &rc.width;
        minor1 = &rc.y;
        minor2 = &rc.height;
        minorSize = height;
    }
    else
    {
        major1 = &rc.y;
        major2 = &rc.height;
        minor1 = &rc.x;
        minor2 = &rc.width;
        minorSize = width;
    }

    memDC.SetBrush(brBack);
    memDC.SetPen(*wxTRANSPARENT_PEN);
    memDC.DrawRectangle(0, 0, width, height);

    if (hw->DocSize() == 0)
    {
        // no data, don't draw anything
        Refresh(false);
        Update();
        return;
    }

    //int availHeight = height - (regions + 1) * 4;
    THSIZE docSize = hw->doc->GetSize();
    THSIZE topByte = hw->GetFirstVisibleByte();
    THSIZE bottomByte = hw->GetLastVisibleByte();
    THSIZE selTop, selBottom;
    hw->GetSelection(selTop, selBottom);

    const DataSource *pDS = hw->m_pDS;
    auto& segments = hw->doc->GetSegments();
    wxBrush *mainBrush = GetBrush(*segments.begin(), pDS, hw->doc->display_address);

    int blockstart = 3;
    int blockend = m_majorSize - 3;
    int blocksize = blockend - blockstart;
    double scale = (double)blocksize / (double)hw->doc->GetSize();

    *major1 = blockstart - 1;
    *major2 = blocksize + 2;
    *minor1 = 4;
    *minor2 = minorSize - 8;
    memDC.SetBrush(*mainBrush);
    memDC.SetPen(borderPen);
    memDC.DrawRectangle(rc);
    m_rcInside = rc;
    m_rcInside.Deflate(1, 1); // m_rcInside excludes the border
    memDC.SetPen(*wxTRANSPARENT_PEN);
    *minor1 += 1;
    *minor2 -= 2;

    //int type = Segment::FILE;
    THSIZE lastSegmentStart = 0;
    THSIZE segmentStart = hw->doc->display_address;
    wxBrush *brush, *curBrush = mainBrush;
    //for (size_t n = 0; n < hw->doc->segments.size(); n++)
    for (auto iter = segments.begin(); iter != segments.end(); iter++)
    {
        const Segment& s = *iter;
        //Segment* s = hw->doc->segments[n];
        //segmentStart = hw->doc->bases[n];

        brush = GetBrush(s, pDS, segmentStart);

        if (*brush != *curBrush)
        { //  only draw rectangle when we have read several ranges, and type changes

            if (*curBrush != *mainBrush) // skip regions that match pDS -- they've already been drawn
            {
                *major1 = blockstart + (int)(lastSegmentStart * scale);
                *major2 = (int)((segmentStart - lastSegmentStart) * scale);
                if (*major2 == 0)
                    *major2 = 1;
                memDC.SetBrush(*curBrush);
                memDC.DrawRectangle(rc);
            }
            lastSegmentStart = segmentStart;
            curBrush = brush;
        }
        segmentStart += s.size;
    }

    //if (type != Segment::FILE)
    if (*curBrush != *mainBrush)
    {
        *major1 = blockstart + (int)(lastSegmentStart * scale);
        *major2 = (int)((hw->doc->GetSize() - lastSegmentStart) * scale);
        if (*major2 == 0)
            *major2 = 1;
        memDC.SetBrush(*curBrush);
        memDC.DrawRectangle(rc);
    }

    //! should we do this with a bitmap?
    memDC.SetPen(borderPen);
    memDC.SetBrush(brBorder);
    wxPoint pt[3];

    if (orient == wxHORIZONTAL)
    {
        // draw selection triangles
        pt[0] = wxPoint(-3, 0);
        pt[1] = wxPoint(0, 0);
        pt[2] = wxPoint(0, 3);
        memDC.DrawPolygon(3, pt, GetCoord(selTop), 0);
        pt[0] = wxPoint(3, 0);
        memDC.DrawPolygon(3, pt, GetCoord(selBottom), 0);

        //! I'm not sure if we need to duplicate what the scroll bar shows, but it's kinda handy.
        // draw window triangles
        pt[0] = wxPoint(0, -4);
        pt[1] = wxPoint(0, -1);
        pt[2] = wxPoint(-3, -1);
        memDC.DrawPolygon(3, pt, GetCoord(topByte), height);

        pt[2] = wxPoint(3, -1);
        memDC.DrawPolygon(3, pt, GetCoord(bottomByte), height);
    }
    else
    {
        // draw selection triangles
        pt[0] = wxPoint(0, -3);
        pt[1] = wxPoint(0, 0);
        pt[2] = wxPoint(3, 0);
        memDC.DrawPolygon(3, pt, 0, GetCoord(selTop));
        pt[0] = wxPoint(0, 3);
        memDC.DrawPolygon(3, pt, 0, GetCoord(selBottom));

        // draw window triangles
        pt[0] = wxPoint(-4, 0);
        pt[1] = wxPoint(-1, 0);
        pt[2] = wxPoint(-1, -3);
        memDC.DrawPolygon(3, pt, width, GetCoord(topByte));

        pt[2] = wxPoint(-1, 3);
        memDC.DrawPolygon(3, pt, width, GetCoord(bottomByte));
    }

    //PRINTF("Updating FileMap\n");

    Refresh(false);
    Update();
}

wxBrush *FileMap::GetBrush(const Segment& s, const DataSource *pDS, THSIZE offset)
{
    if (s.pDS == pDS)
    {
        if (offset < s.stored_offset)
            return &brBackward;
        if (offset > s.stored_offset)
            return &brForward;
        return &brFile;
    }

    return &brMem;  // this segment does not contain data from the original file
}

int FileMap::GetCoord(THSIZE pos)
{
    int blockstart = 3;
    int blockend = m_majorSize - 4;
    int blocksize = blockend - blockstart;
    double scale = (double)blocksize / (double)m_hw->DocSize();
    return blockstart + (int)(scale * pos);
}

void FileMap::OnMouse(const wxPoint &pt)
{
    //wxPoint newPoint = MoveInside(m_rcInside, pt, 100);
    // I decided not to call MoveInside() here becuase HitTest() checks for
    // clicks on the selection triangles.  There may be a better way.
    wxPoint newPoint = pt;

    THSIZE pos;
    int region = HitTest(newPoint, pos);
    if (region)
        //m_hw->CenterLine(m_hw->ByteToLineCol(pos, NULL));
        m_hw->CenterByte(pos);
}

void FileMap::OnMouseDown(wxMouseEvent &event)
{
    //wxPoint pt = ScreenToClient(event.GetPosition());
    OnMouse(event.GetPosition());

    CaptureMouse();
}

void FileMap::OnMouseMove(wxMouseEvent &event)
{
    if (HasCapture() && event.LeftIsDown())
        OnMouse(event.GetPosition());
}

void FileMap::OnMouseUp(wxMouseEvent &event)
{
    ReleaseCapture();
}

int FileMap::HitTest(const wxPoint pt, THSIZE &pos)
{
    if (m_hw == NULL)
        return 0;

    if (m_rcInside.InsideContains(pt))
    {
        double d;
        if (orient == wxHORIZONTAL)
            d = (double)(pt.x - m_rcInside.x) / m_rcInside.width;
        else
            d = (double)(pt.y - m_rcInside.y) / m_rcInside.height;
        pos = d * m_hw->doc->GetSize();
        return 1;
    }
    else // test for top or bottom of selection
    {
        wxRect rc(0, 0, 4, 4);
        wxCoord *coord;
        if (orient == wxHORIZONTAL)
            coord = &rc.x;
        else
            coord = &rc.y;

        THSIZE selTop, selBottom;
        m_hw->GetSelection(selTop, selBottom);
        *coord = GetCoord(selTop) - 4;
        if (rc.InsideContains(pt))
        {
            pos = selTop;
            return 2;
        }

        *coord = GetCoord(selBottom);
        if (rc.InsideContains(pt))
        {
            pos = selBottom;
            return 3;
        }
    }

    return 0;
}

void FileMap::OnContextMenu(wxContextMenuEvent &event)
{
    m_mousePos = ScreenToClient(event.GetPosition());
    popup->Check(IDM_AUTO, forceOrient == 0);
    popup->Check(IDM_VERTICAL, forceOrient == wxVERTICAL);
    popup->Check(IDM_HORIZONTAL, forceOrient == wxHORIZONTAL);
    PopupMenu(popup, m_mousePos);
}

void FileMap::OnCommand(wxCommandEvent &event)
{
    THSIZE pos, base;

    switch (event.GetId())
    {
    case IDM_HORIZONTAL:
        this->forceOrient = wxHORIZONTAL;
        UpdateView(m_hw, -1);
        break;
    case IDM_VERTICAL:
        this->forceOrient = wxVERTICAL;
        UpdateView(m_hw, -1);
        break;
    case IDM_AUTO:
        this->forceOrient = 0;
        UpdateView(m_hw, -1);
        break;
    case IDM_SELECTBLOCK:
        if (HitTest(m_mousePos, pos))
        {
            const Segment *ts = m_hw->doc->GetSegment(pos, &base);
            if (ts)
            {
                m_hw->CmdSetSelection(base, base + ts->size);
            }
        }
        break;
    case IDM_SELSTART:
        m_hw->CenterByte(m_hw->GetSelection().GetFirst());
        break;
    case IDM_SELEND:
        m_hw->CenterByte(m_hw->GetSelection().GetLast());
        break;
    case IDM_BLOCKSTART:
        if (HitTest(m_mousePos, pos))
        {
            const Segment *ts = m_hw->doc->GetSegment(pos, &base);
            if (ts)
                m_hw->CenterByte(base);
        }
        break;
    case IDM_BLOCKEND:
        if (HitTest(m_mousePos, pos))
        {
            const Segment *ts = m_hw->doc->GetSegment(pos, &base);
            if (ts)
                m_hw->CenterByte(base + ts->size - 1);
        }
        break;
    case IDM_CLOSE: {
        wxCommandEvent cmd(wxEVT_COMMAND_MENU_SELECTED);
        cmd.SetId(IDM_ViewFileMap);
        GetParent()->GetEventHandler()->AddPendingEvent(cmd);
        }
        break;
    }
}


//****************************************************************************
//****************************************************************************
// DocList
//****************************************************************************
//****************************************************************************

int wxCALLBACK wxListCompareFunction(long item1, long item2, wxIntPtr data)
{
    DocList *docList = (DocList*)data;
    wxListCtrl *list = docList->list;
    HexDoc *r1 = (HexDoc*)item1;
    HexDoc *r2 = (HexDoc*)item2;

    static int cmpCount = 0;

    //PRINTF("%35s %35s %d\n", r1->info.Right(35).c_str(), r2->info.Right(35).c_str(), cmpCount++);

    int result = 0;
    switch (docList->sortColumn)
    {
    case 0: // Item name
       result = r1->info.CmpNoCase(r2->info);
       break;
    case 1: // Range
       if (r1->display_address > r2->display_address)
          result = 1;
       else if (r1->display_address < r2->display_address)
          result = -1;
       else if (r1->GetSize() > r2->GetSize())
          result = -1; // backward on purpose
       else if (r1->GetSize() < r2->GetSize())
          result = 1; // backward on purpose
       else
          result = 0;
       break;
    case 2: // Size
       if (r1->GetSize() > r2->GetSize())
          result = 1;
       else if (r1->GetSize() < r2->GetSize())
          result = -1;
       else
          result = 0;
       break;
    case 3: // Access (R or RW)
       //result = r1->IsWriteable() - r2->IsWriteable();
       result = r1->dwFlags - r2->dwFlags;
       break;
    }
    return result * docList->sortOrder[docList->sortColumn];
}



BEGIN_EVENT_TABLE(DocList, wxPanel)
    EVT_LIST_ITEM_SELECTED(IDC_DOC_LIST, OnSelChange)
    EVT_LIST_ITEM_ACTIVATED(IDC_DOC_LIST, OnActivate)
    EVT_LIST_COL_CLICK(IDC_DOC_LIST, OnColumnClick)
    EVT_LIST_COL_RIGHT_CLICK(IDC_DOC_LIST, OnColumnRightClick)
    EVT_LIST_ITEM_RIGHT_CLICK(IDC_DOC_LIST, OnItemRightClick)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_MENU(IDM_ActiveDoc, CmdActiveDoc)
END_EVENT_TABLE()

DocList::DocList(wxWindow *parent)
: DataView(this),
wxPanel(parent, -1)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    list = new wxListCtrl(this, IDC_DOC_LIST, wxDefaultPosition, wxSize(-1, 50), wxLC_REPORT /*| wxLC_NO_HEADER*/);
    list->InsertColumn(0, _T("Item"));
    list->InsertColumn(1, _T("Offset"));
    list->InsertColumn(2, _T("Size"));
    list->InsertColumn(3, _T("Access"));
    sizer->Add(list, 1, wxGROW);
    SetSizerAndFit(sizer);

    m_menuSource = NONE;
    m_bListFrozen = false;

    //for (int i = 0; i < 4; i++)
    //    sortOrder[i] = 1;
    //sortColumn = 1; // initially sorted by starting address
}

wxString ProtectFlagsAsString(DWORD flags)
{
    wxString ret;

    // The lower 8 bits are mutually exclusive.
    if (bitcount(flags & 0xFF) != 1)
        return _T("???");

    if (flags & PAGE_EXECUTE)           ret = _T("execute only");
    if (flags & PAGE_EXECUTE_READ)      ret = _T("read & execute");
    if (flags & PAGE_EXECUTE_READWRITE) ret = _T("read/write & execute");
    if (flags & PAGE_EXECUTE_WRITECOPY) ret = _T("copy on write & execute");
    if (flags & PAGE_NOACCESS)          ret = _T("no access");
    if (flags & PAGE_READONLY)          ret = _T("read only");
    if (flags & PAGE_READWRITE)         ret = _T("read/write");
    if (flags & PAGE_WRITECOPY)         ret = _T("copy on write");

    if (flags & PAGE_GUARD)             ret += _T(", guard");
    if (flags & PAGE_NOCACHE)           ret += _T(", no cache");
    if (flags & PAGE_WRITECOMBINE)      ret += _T(", write combined");
    return ret;
}

void DocList::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
    if (bSkipUpdate)
        return;
    int selected;
    if (hw != NULL && hw == m_hw && FLAGS(flags, DV_NEWDOC) && flags != -1)
    {   // Update the selection because somebody else called HexWnd::SetDoc().
        SelectDoc(hw->doc);
        return;
    }

    if (hw == m_hw)
        return;
    if (!FLAGS(flags, DV_ALL))
        return;
    m_hw = hw;

    if (!m_bListFrozen)
        list->Freeze();
    list->DeleteAllItems();

    int item = 0;
    //for (HexDoc *r = hw->m_pRootRegion; r != NULL; r = r->next, item++)
    int offset_digits = 1, size_digits = 0;
    if (hw)
    {
        auto docs = hw->GetDocList();
        const size_t nDocs = docs.size();
        for (size_t n = 0; n < nDocs; n++)
        {
            const HexDoc *r = docs[n];
            offset_digits = wxMax(offset_digits, CountDigits(r->display_address, 16));
            size_digits = wxMax(size_digits, CountDigits(r->GetSize(), 16));
        }

        for (size_t n = 0; n < nDocs; n++, item++)
        {
            const HexDoc *r = docs[n];
            wxListItem li;
            li.SetId(item);
            li.SetText(r->info);
            li.SetData((wxUIntPtr)r);
            list->InsertItem(li);
            list->SetItem(item, 1, FormatNumber(r->display_address, 16, offset_digits));
            list->SetItem(item, 2, FormatNumber(r->GetSize(), 16, size_digits));
            list->SetItem(item, 3, ProtectFlagsAsString(r->dwFlags));
        }
        //list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
        //list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
        //list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
        //list->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
        m_bListFrozen = true;

        sortColumn = hw->DocListSortColumn;
        for (size_t i = 0; i < 4; i++)
            sortOrder[i] = hw->DocListSortOrder[i];

        selected = list->FindItem(-1, (wxUIntPtr)hw->doc);
        if (selected) {
            list->EnsureVisible(selected);
            list->SetItemState(selected, selstate, selstate);
        }
    }
    list->SortItems(wxListCompareFunction, (long)this);

    //list->Thaw();
}

void DocList::ProcessUpdates()
{
    if (m_bListFrozen)
    {
        m_bListFrozen = false;
        list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
        list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
        list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
        list->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
        list->Thaw();
    }
}

void DocList::SelectDoc(HexDoc *doc)
{
    ClearWxListSelections(list);
    int selected = list->FindItem(-1, (wxUIntPtr)doc);
    if (selected >= 0) {
        list->EnsureVisible(selected);
        list->SetItemState(selected, selstate, selstate);
    }
}

void DocList::OnSelChange(wxListEvent &event)
{
    int sel = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0)
        return;

    HexDoc* r = (HexDoc*)list->GetItemData(sel);
    uint64 start = 0;
    //for (HexDoc *tr = m_hw->m_pRootRegion; tr != NULL; tr = tr->next)
    //{
    //    if (tr == r) {
    //        m_hw->SetDoc(r);
    //        return;
    //    }
    //    start += tr->size;
    //}
}

void DocList::OnActivate(wxListEvent &event)
{
   HexDoc* r = (HexDoc*)list->GetItemData(event.GetIndex());
   bSkipUpdate = true;
   m_hw->SetDoc(r);
   m_hw->SetFocus();
   bSkipUpdate = false;
}

void DocList::OnColumnClick(wxListEvent &event)
{
    int col = event.GetColumn();
    if (col == sortColumn)
        m_hw->DocListSortOrder[col] = sortOrder[col] *= -1;
    else
        m_hw->DocListSortColumn = sortColumn = col;
    
    list->SortItems(wxListCompareFunction, (long)this);

    if (m_hw) {
        int item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
        if (item >= 0)
            list->EnsureVisible(item);
    }
}

void DocList::OnColumnRightClick(wxListEvent &event)
{
    // I don't see how to prevent WM_CONTEXTMENU, so just work with it.
    m_menuSource = HEADER;
    m_menuSourceItem = event.GetColumn();
}

void DocList::OnItemRightClick(wxListEvent &event)
{
    m_menuSource = ITEM;
    m_menuSourceItem = event.GetIndex();
}

void DocList::OnContextMenu(wxContextMenuEvent &event)
{
    // WM_CONTEXTMENU gives us the mouse position in screen coordinates,
    // or (-1, -1) if the context menu key was pressed.
    wxPoint pt = event.GetPosition();
    int sel = m_menuSourceItem;

    if (m_menuSource == HEADER)
    {
        wxMenu menu;
        menu.AppendSeparator();
        menu.AppendSeparator();
        PopupMenu(&menu, ScreenToClient(pt));
        m_menuSource = NONE;
        return;
    }

    if (m_menuSource == NONE) // message came from context menu key (VK_APPS)
    {
        sel = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

        // All uses of pt inside this block are in screen coordinates.
        pt = ::wxGetMousePosition();
        wxRect rc(list->ClientToScreen(wxPoint(0, 0)), list->GetSize());
        if (!rc.InsideContains(pt))
        {
           if (list->GetItemRect(sel, rc))
              pt = list->ClientToScreen(rc.GetPosition());
        }
    }

    if (sel < 0 && list->GetItemCount() <= 1)
       return;

    wxMenu menu;
    //menu.AppendSeparator();
    menu.Append(IDM_ActiveDoc, _T("&Active document"));
    
    PopupMenu(&menu, ScreenToClient(pt));
    m_menuSource = NONE;
}

void DocList::CmdActiveDoc(wxCommandEvent &event)
{
    SelectDoc(m_hw->doc);
}


//****************************************************************************
//****************************************************************************
// StringView
//****************************************************************************
//****************************************************************************

#define EVT_TEXT_SELCHANGE(winid, func) wx__DECLARE_EVT1(wxEVT_COMMAND_TEXT_SELCHANGE, winid, wxCommandEventHandler(func))

BEGIN_EVENT_TABLE(StringView, wxTextCtrl)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_MENU(IDM_FontDlg,       OnFont)
    EVT_MENU(IDM_ViewAuto,      OnEncoding)
    EVT_MENU(IDM_ViewASCII,     OnEncoding)
    EVT_MENU(IDM_ViewUTF8,      OnEncoding)
    EVT_MENU(IDM_ViewUTF16,     OnEncoding)
    EVT_MENU(IDM_ViewUTF32,     OnEncoding)
    EVT_MENU(IDM_WordWrap,      OnWordWrap)
#ifdef WX_AEB_MOD
    EVT_TEXT_SELCHANGE(-1, OnSelChange)
#endif
    EVT_SET_FOCUS(StringView::OnSetFocus)
    EVT_KILL_FOCUS(OnKillFocus)
END_EVENT_TABLE()

StringView::StringView(wxWindow *parent)
: DataView(this),
wxTextCtrl(parent, -1, ZSTR, wxDefaultPosition, wxDefaultSize,
           wxTE_RICH | wxTE_READONLY | wxTE_MULTILINE
           | wxTE_DONTWRAP
           )
{
    text = this;
    text->SetMinSize(wxSize(100, 20));
    text->SetInitialSize(wxSize(200, 300));
    m_encoding = IDM_ViewAuto;
    bWordWrap = false;
}

StringView::~StringView()
{
}

void StringView::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
    if (hw != m_hw)
    {
        m_hw = hw;
        m_queuedUpdates = -1;
    }
    m_queuedUpdates |= flags;
}

void StringView::ProcessUpdates()
{
    if (!(m_queuedUpdates & (DV_DATA | DV_SELECTION | DV_NEWDOC)))
        return;

    if (!m_hw)
    {
       Clear();
       return;
    }

    bool bUnicode = false;
    wxString str;

    THSIZE addr, selSize, size8 = 0, size16 = 0, docSize = m_hw->doc->GetSize();
    m_hw->GetSelection().Get(addr, selSize);
    int charBytes = 0; // 0 = not set, 1 or 2

    if (selSize > 0xFFFF)
        selSize = 0xFFFF;

    //! todo: define how to auto-detect a string, then do that.
    if (selSize == 0) // look for beginning and end of string
    {
        while (addr + size8 < docSize && size8 < 0xFFFF)
        {
            char c = m_hw->doc->GetAt(addr + size8);
            if (my_isprint[(uint8)c])
                size8++;
            else
                break;
        }
        if (size8 > 0)
        {
            while (addr > 0 && size8 < 0xFFFF)
            {
                char c = m_hw->doc->GetAt(addr - 1);
                if (my_isprint[(uint8)c])
                    addr--, size8++;
                else
                    break;
            }
        }
    }

    if (selSize == 0 && size8 <= 1) // look for beginning and end of UTF-16 string //! (is it really UTF-16?)
    {
        THSIZE addr16 = addr - addr % 2;
        while (addr16 + size16 < docSize && size16 < 0xFFFF)
        {
            uint16 c = m_hw->doc->Read16(addr16 + size16);
            if (c <= 127 && my_isprint[(uint8)c])
                size16 += 2;
            else
                break;
        }
        if (size16 > 0)
        {
            while (addr16 > 0 && size16 < 0xFFFF)
            {
                uint16 c = m_hw->doc->Read16(addr16 - 2);
                if (c <= 127 && my_isprint[(uint8)c])
                    addr16 -= 2, size16 += 2;
                else
                    break;
            }
            addr = addr16;
        }
    }

    //if (m_style == IDM_ViewASCII)
    //    size16 = 0;
    //else if (m_style == IDM_ViewUnicode)
    //    size8 = 0;

    if (selSize == 0)
    {
        if (size16 > size8)
            charBytes = 2, selSize = size16;
        else
            charBytes = 1, selSize = size8;
    }
    else if (selSize == 1)
        charBytes = 1;
    else
    {
        // If every second byte is zero, display as UTF-16.
        charBytes = 2;
        for (THSIZE tmp = addr | 1; tmp < addr + selSize; tmp += 2)
        {
            if (m_hw->doc->GetAt(tmp) != 0)
            {
                charBytes = 1;
                break;
            }
        }
    }

    if (!(m_queuedUpdates & (DV_NEWDOC | DV_DATA)) &&
        addr == m_iSelStart &&
        selSize == m_iSelSize &&
        m_hw->doc->GetChange() == m_iChangeIndex)
        return;

    if (charBytes == 2 && selSize)
    {
        if (!(m_queuedUpdates & DV_NEWDOC) &&
            addr == m_iSelStart &&
            selSize == m_iSelSize &&
            m_hw->doc->GetChange() == m_iChangeIndex)
            return;

        str = m_hw->doc->ReadStringW(addr, selSize / 2, true);
        if (str.Len() == 0)
            charBytes = 1; // try as MB string
    }
    
    if (charBytes == 1 && selSize)
        str = m_hw->doc->ReadString(addr, selSize, true);

    m_iSelStart = addr;
    m_iSelSize = selSize;
    m_iChangeIndex = m_hw->doc->GetChange();

    //! todo: if selection exists and is Unicode, display as Unicode?

    // When using SetValue() with a single CR or LF, the rich text control inserts
    // instead of replacing.  Using Clear() should fix that.
    //text->Freeze();
    //text->Clear();
    //text->SetValue(str);
    //text->Thaw();

    //! Yes, it fixes the text problem but makes the scroll bars dance.
    // This might work better.  It's still weird.
    if (str.Len() <= 2)
        text->Clear();
    m_bHighlight = false;
    text->SetValue(str);
    m_bHighlight = true;
}

void StringView::OnContextMenu(wxContextMenuEvent &event)
{
    wxMenu menu;
    menu.AppendCheckItem(IDM_ViewAuto,  _T("A&uto"));
    menu.AppendCheckItem(IDM_ViewASCII, _T("&ASCII"));
    menu.AppendCheckItem(IDM_ViewUTF8,  _T("UTF-&8"));
    menu.AppendCheckItem(IDM_ViewUTF16, _T("UTF-16 (Windows &Unicode)"));
    menu.AppendCheckItem(IDM_ViewUTF32, _T("UTF-&32"));
    menu.AppendSeparator();

    menu.Check(IDM_ViewAuto , m_encoding == IDM_ViewAuto);
    menu.Check(IDM_ViewASCII, m_encoding == IDM_ViewASCII);
    menu.Check(IDM_ViewUTF8 , m_encoding == IDM_ViewUTF8);
    menu.Check(IDM_ViewUTF16, m_encoding == IDM_ViewUTF16);
    menu.Check(IDM_ViewUTF32, m_encoding == IDM_ViewUTF32);

    menu.Append(IDM_FontDlg, _T("&Font..."));
    //menu.AppendCheckItem(IDM_WordWrap, "&Word wrap");
    //menu.Check(IDM_WordWrap, bWordWrap);
    PopupMenu(&menu, ScreenToClient(event.GetPosition()));    
}

void StringView::OnFont(wxCommandEvent &WXUNUSED(event))
{
    wxFont font = ::wxGetFontFromUser(this, GetFont());
    if (font.IsOk())
        SetFont(font);
}

void StringView::OnEncoding(wxCommandEvent &event)
{
    m_encoding = event.GetId();
    UpdateView(m_hw, -1);
}

void StringView::OnWordWrap(wxCommandEvent &event)
{
    //! Wrapping styles can't be changed after control creation on MSW.
    bWordWrap = !bWordWrap;
    long flags = GetWindowStyle();
    if (bWordWrap)
        SetWindowStyle(flags & ~wxTE_DONTWRAP);
    else
        SetWindowStyle(flags | wxTE_DONTWRAP);
}

void StringView::OnSelChange(wxCommandEvent &event)
{
   if (!m_hw)
      return;
   if (!m_bHighlight) // OnSelChange() gets called when the text is changed
      return;
   THSIZE start = m_iSelStart + event.GetInt();
   long size = event.GetExtraLong() - event.GetInt();
   m_hw->ScrollToRange(start, start + size, 0);
   m_hw->Highlight(start, wxMax(size, 1));
}

//****************************************************************************
//****************************************************************************
// NumberView
//****************************************************************************
//****************************************************************************

//NumberView::NumberView(wxWindow *parent)
//: wxPanel(parent)
//{
//    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
//    text = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_RICH | wxTE_READONLY | wxTE_MULTILINE | wxHSCROLL);
//    sizer->Add(text, 1, wxGROW);
//    text->SetMinSize(wxSize(100, 20));
//    text->SetBestFittingSize(wxSize(200, 300));
//    text->SetFont(wxFont(GetFont().GetPointSize(), wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
//    SetSizerAndFit(sizer);
//}

NumberView::NumberView(wxWindow *parent)
: DataView(this),
wxTextCtrl(parent, -1, ZSTR, wxDefaultPosition, wxDefaultSize, wxTE_RICH | wxTE_READONLY | wxTE_MULTILINE | wxHSCROLL)
{
    //! Should use some other control, and possibly allow editing.
    //  Combine with structure view as "basic types"?
    text = this;
    text->SetMinSize(wxSize(100, 20));
    text->SetInitialSize(wxSize(200, 300));
    text->SetFont(wxFont(GetFont().GetPointSize(), wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
}

void NumberView::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
    bool bForceUpdate = false;
    if (hw != m_hw)
    {
        m_hw = hw;
        bForceUpdate = true;
    }
    QueueUpdates(flags);
}

void NumberView::ProcessUpdates()
{
    if (!(m_queuedUpdates & (DV_SELECTION | DV_DATA)))
        return;

    if (!m_hw)
    {
       Clear();
       return;
    }

    THSIZE addr, selSize;
    m_hw->GetSelection().Get(addr, selSize);
    if (selSize > 8)
        selSize = 8;
    
    if (!(m_queuedUpdates & DV_DATA) &&
        addr == m_iSelStart &&
        selSize == m_iSelSize)
        return;

    m_iSelStart = addr;
    m_iSelSize = selSize;

    uint8 i8 = m_hw->doc->GetAt(addr);
    uint16 i16 = m_hw->doc->Read16(addr);
    uint32 i32 = m_hw->doc->Read32(addr);
    uint64 i64 = m_hw->doc->Read64(addr);
    float f = m_hw->doc->ReadFloat(addr);
    double d = m_hw->doc->ReadDouble(addr);

    wxString txt;
    txt  = wxString::Format(_T("s 8: %d\r\n"), (signed char)i8);
    txt += wxString::Format(_T("u 8: %u\r\n"), i8);
    txt += wxString::Format(_T("s16: %d\r\n"), (signed short)i16);
    txt += wxString::Format(_T("u16: %u\r\n"), i16);
    txt += wxString::Format(_T("s32: %d\r\n"), (int32)i32);
    txt += wxString::Format(_T("u32: %u\r\n"), i32);
    txt += wxString::Format(_T("s64: %I64d\r\n"), (int64)i64);
    txt += wxString::Format(_T("u64: %I64u\r\n"), i64);
    txt += wxString::Format(_T("float: %hg\r\n"), f);
    txt += wxString::Format(_T("double: %g\r\n"), d);

    ibm2ieee(&f, &i32, 1);
    txt += wxString::Format(_T("IBM float: %hg"), f); // don't include \r\n on last line
    text->SetValue(txt);
}


//****************************************************************************
//****************************************************************************
// StructureView
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(StructureView, wxPanel)
    EVT_CHOICE(IDC_STRUCT, OnSetStruct)
    //EVT_LIST_ITEM_SELECTED(IDC_LIST, OnSelectMember)
    //EVT_LIST_ITEM_ACTIVATED(IDC_LIST, OnActivateMember)
    EVT_TREE_SEL_CHANGED(IDC_LIST, OnSelectMemberTree)
    EVT_TREE_ITEM_ACTIVATED(IDC_LIST, OnActivateMemberTree) 
    EVT_BUTTON(IDC_NEXT, OnNextStruct)
    EVT_BUTTON(IDC_PREV, OnPrevStruct)
    EVT_SET_FOCUS(OnSetFocus)
    EVT_KILL_FOCUS(OnKillFocus)
END_EVENT_TABLE()

StructureView::StructureView(wxWindow *parent)
: DataView(this),
wxPanel(parent, -1)
{
    btnNext = new wxButton(this, IDC_NEXT, _T("&Next"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxBU_EXACTFIT);
    btnPrev = new wxButton(this, IDC_PREV, _T("&Prev"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxBU_EXACTFIT);
    spinAddress = new wxSpinCtrl(this, IDC_ADDRESS);
    cmbStruct = new wxChoice(this, IDC_STRUCT);
    //list = new wxListCtrl(this, IDC_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    //list->InsertColumn(0, "Name");
    //list->InsertColumn(1, "Offset");
    //list->InsertColumn(2, "Type");
    //list->InsertColumn(3, "Value");

    list = new wxTreeListCtrl(this, IDC_LIST, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_EDIT_LABELS | wxTR_HIDE_ROOT | wxTR_VRULE);
    //! This is stupid.  You shouldn't have to insert columns backwards.
    list->AddColumn(_T("Value"));
    list->InsertColumn(0, _T("Type"));
    list->InsertColumn(0, _T("Offset"));
    list->InsertColumn(0, _T("Name"));

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    hSizer->Add(btnNext);
    hSizer->Add(btnPrev);
    hSizer->Add(spinAddress);
    sizer->Add(hSizer);

    hSizer = new wxBoxSizer(wxHORIZONTAL);
    hSizer->Add(new wxStaticText(this, -1, _T("&Struct")), 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    hSizer->Add(cmbStruct);
    sizer->Add(hSizer);

    sizer->Add(list, 1, wxGROW);

    //text = new wxTextCtrl(this, -1, ZSTR, wxDefaultPosition, wxDefaultSize, wxTE_RICH | wxTE_READONLY | wxTE_MULTILINE);
    //sizer->Add(text, 1, wxGROW);

    SetSizerAndFit(sizer);

    // Get name of configuration file.
    wxFileName fn(wxGetApp().argv[0]);
    fn.SetName(_T("structures")); // structures.ini in app directory
    fn.SetExt(_T("ini"));

    std::ifstream istr((LPCTSTR)fn.GetFullPath());
    if (istr.is_open())
    {
        yyFlexLexer flex(&istr);
        int err = yyparse(0, &flex, &m_structs);
    }

    for (size_t i = 0; i < m_structs.size(); i++)
    {
        cmbStruct->Append(m_structs[i].name);
    }

    m_nIndex = -1;
    if (m_structs.size())
    {
        cmbStruct->SetSelection(0);
        SetStruct(0);
    }
    else
        SetStruct(-1);

    //list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
    //list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
    //list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
}

void StructureView::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
    m_hw = hw;
    QueueUpdates(flags);
}

void StructureView::ProcessUpdates()
{
    if (!(m_queuedUpdates & (DV_SELECTION | DV_DATASIZE |DV_DATA)))
        return;
    //text->Clear();

    if (!m_hw || !m_hw->doc)
        return;

    if (m_nStruct < 0)
        return;

    StructInfo &si = m_structs[m_nStruct];
    Selection sel = m_hw->GetSelection();
    int readSize = wxMin(si.GetSize() + 100, m_hw->doc->GetSize() - sel.nStart);
    uint8 *pData = new uint8[readSize];

    if (!m_hw->doc->Read(sel.nStart, readSize, pData))
    {
        delete [] pData;
        return;
    }

    spinAddress->SetValue(sel.nStart);
    spinAddress->SetRange(0, m_hw->doc->GetSize());

    uint8 *tmpData = pData;

    for (size_t i = 0; i < si.vars.size(); i++)
    {
        VarInfo &var = si.vars[i];
        //text->AppendText(var.name + "\t" + var.Format(tmpData) + "\r\n");
        //list->SetItem(i, 3, var.Format(tmpData));
        list->SetItemText(m_treeIDs[i], 3, var.Format(tmpData));
        tmpData += var.GetSize();

        if (m_nIndex == i)
            m_hw->Highlight(sel.nStart + var.offset, var.GetSize(), this);
    }
    if (m_nIndex < 0)
        m_hw->Highlight(0, 0, this);

    delete [] pData;
}

void StructureView::OnSetStruct(wxCommandEvent &event)
{
    SetStruct(event.GetInt());
}

void StructureView::SetStruct(int nStruct)
{
    //list->DeleteAllItems();
    list->DeleteRoot();
    m_treeIDs.clear();
    this->m_nStruct = nStruct;
    this->m_nIndex = -1;

    if (nStruct < 0)
        return;

    StructInfo &si = m_structs[nStruct];

    wxFont underlined = list->GetFont();
    underlined.SetUnderlined(true);

    wxTreeItemId rootID = list->AddRoot(wxEmptyString);

    for (size_t i = 0; i < si.vars.size(); i++)
    {
        VarInfo &var = si.vars[i];
        //list->InsertItem(i, var.name);
        //list->SetItem(i, 1, wxString::Format("%d", var.offset));
        //list->SetItem(i, 2, var.FormatType());

        //int *pData = new int; //! stupid, stupid!
        //*pData = i;
        int *pData = NULL;
        wxTreeItemId id = list->AppendItem(rootID, var.name, -1, -1,
            (wxTreeItemData*)pData);
        list->SetItemText(id, 1, wxString::Format(_T("%d"), var.offset));
        list->SetItemText(id, 2, var.FormatType());

#if wxCHECK_VERSION(2, 7, 0)
        if (var.pointerType)
            //list->SetItemFont(i, underlined); // SetItemFont() not available in wx 2.6.1 
            list->SetItemFont(id, underlined); // SetItemFont() not available in wx 2.6.1 
#endif
        m_treeIDs.push_back(id);
    }
    list->ExpandAll(rootID);
    list->Refresh();

    if (m_hw)
        UpdateView(m_hw, -1);
}

void StructureView::OnSelectMember(wxListEvent &event)
{
    m_nIndex = event.GetIndex();
    if (!m_hw)
        return;
    if (m_nStruct < 0)
        return;
    VarInfo &var = m_structs[m_nStruct].vars[event.GetIndex()];
    Selection sel = m_hw->GetSelection();
    m_hw->Highlight(sel.GetFirst() + var.offset, var.GetSize(), this);
}

void StructureView::OnActivateMember(wxListEvent &event)
{
    if (!m_hw)
        return;
    if (m_nStruct < 0)
        return;
    VarInfo &var = m_structs[m_nStruct].vars[event.GetIndex()];
    if (var.pointerType == VarInfo::PT_NONE)
        return;

    Selection sel = m_hw->GetSelection();
    THSIZE target = var.GetPointer(m_hw, sel.GetFirst());
    if (target > m_hw->doc->GetSize())
        wxMessageBox(_T("Target is beyond the end of this file."), _T("T. Hex"));
    else
    {
        m_hw->CmdMoveCursor(target);
        m_hw->Highlight(target, 1, this);
        m_hw->SetFocus();
    }
}

int StructureView::FindTreeItem(wxTreeItemId item)
{
    //std::vector<wxTreeItemId>::const_iterator iter =
    //    std::find(m_treeIDs.begin(), m_treeIDs.end(), item);
    //if (iter == m_treeIDs.end())
    //    return -1;
    //return *iter; // wrong.  Need index, not contents.

    for (size_t i = 0; i < m_treeIDs.size(); i++)
    {
        if (m_treeIDs[i] == item)
            return i;
    }
    return -1;
}

void StructureView::OnSelectMemberTree(wxTreeEvent &event)
{
    //int *pData = (int*)list->GetItemData(event.GetItem());
    //if (!pData) return;
    //m_nIndex = *pData;

    m_nIndex = FindTreeItem(event.GetItem());
    if (m_nIndex == -1)
        return;

    if (!m_hw)
        return;
    if (m_nStruct < 0)
        return;
    VarInfo &var = m_structs[m_nStruct].vars[m_nIndex];
    Selection sel = m_hw->GetSelection();
    m_hw->Highlight(sel.GetFirst() + var.offset, var.GetSize());
}

void StructureView::OnActivateMemberTree(wxTreeEvent &event)
{
    m_nIndex = FindTreeItem(event.GetItem());
    if (m_nIndex == -1)
        return;
    if (!m_hw)
        return;
    if (m_nStruct < 0)
        return;

    VarInfo &var = m_structs[m_nStruct].vars[m_nIndex];
    if (var.pointerType == VarInfo::PT_NONE)
        return;

    Selection sel = m_hw->GetSelection();
    THSIZE target = var.GetPointer(m_hw, sel.GetFirst());
    if (target > m_hw->DocSize())
        wxMessageBox(_T("Target is beyond the end of this file."), _T("T. Hex"));
    else
    {
        m_hw->CmdMoveCursor(target);
        m_hw->Highlight(target, 1);
        m_hw->SetFocus();
    }
}

void StructureView::OnNextStruct(wxCommandEvent &event)
{
    m_hw->CmdMoveRelative(0, m_structs[m_nStruct].GetSize());
}

void StructureView::OnPrevStruct(wxCommandEvent &event)
{
    m_hw->CmdMoveRelative(0, -(int64)m_structs[m_nStruct].GetSize());
}

//****************************************************************************
//****************************************************************************
// ProfilerView
//****************************************************************************
//****************************************************************************

HDC   ProfilerView::ghDC = 0;
HGLRC ProfilerView::ghRC = 0;

ProfilerView *ProfilerView::g_profView = NULL;

BEGIN_EVENT_TABLE(ProfilerView, wxPanel)
   EVT_PAINT(OnPaint)
   EVT_SIZE(OnSize)
END_EVENT_TABLE()

ProfilerView::ProfilerView(wxWindow *parent, wxSize size /*= wxDefaultSize*/)
: DataView(this),
wxPanel(parent, -1, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE)
{
    ProfilerView::g_profView = this;

#ifdef PROFILE
   //ghDC = GetDC((HWND)m_hWnd);
   //ghRC = wglCreateContext(ghDC);
   //wglMakeCurrent(ghDC, ghRC);
   //wxRect rect = GetClientRect();
   //initializeGL(rect.width, rect.height);
   Prof_set_report_mode(Prof_HIERARCHICAL_TIME);
#endif

   SetBackgroundColour(wxColour(0, 0, 0));
}

ProfilerView::~ProfilerView()
{
   //wglDeleteContext(ghRC);
}

void ProfilerView::gl_printText(float x, float y, char *str)
{
   //printf("%s\n", str);
   RECT rc = {x, y, 9999, 9999};
   SetBkMode(ghDC, TRANSPARENT);
   DrawTextA(ghDC, str, -1, &rc, 0);
}

float ProfilerView::gl_textWidth(char *str)
{
   RECT rc = {0, 0, 9999, 0};
   DrawTextA(ghDC, str, -1, &rc, DT_CALCRECT);
   return rc.right;
}

void ProfilerView::OnSize(wxSizeEvent &event)
{
    UpdateBitmapSize(event.GetSize());
}

void ProfilerView::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
   Refresh(false);
}

extern "C" void Prof_draw_GDI(float sx, float sy,
                         float width, float height,
                         float line_spacing,
                         int precision, HDC hdc);

void ProfilerView::OnPaint(wxPaintEvent &event)
{
   wxPaintDC paintDC(this);

#ifdef PROFILE
   wxMemoryDC dc;
   dc.SelectObject(m_bmp);
   HDC hdc = (HDC)dc.GetHDC();
   HFONT hFont = (HFONT)GetFont().GetHFONT();
   SelectObject(hdc, hFont);

   // get line height
   //RECT rc = {0, 0, 9999, 9999};
   //DrawText(hdc, "0123456789jiqpgAZ", -1, &rc, DT_CALCRECT);
   //int height = rc.bottom;

   TEXTMETRIC tm;
   GetTextMetrics(hdc, &tm);
   int height = tm.tmHeight;

   //Prof_update(0);
   //ATimer timer;
   //timer.StartWatch();
   wxRect rcClient = GetClientRect();
   //Prof_draw_gl(0, 0, rcClient.width, rcClient.height, 18, 2, gl_printText, gl_textWidth);
   Prof_draw_GDI(0, 0, rcClient.width, rcClient.height, height, 2, hdc);

   paintDC.Blit(0, 0, rcClient.width, rcClient.height, &dc, 0, 0);

   //timer.StopWatch();
   {
       //Prof(ProfilerView);
       //timer.Wait();
   }
   //Prof_update(1);
#endif // PROFILE
}


//****************************************************************************
//****************************************************************************
// DisasmView
//****************************************************************************
//****************************************************************************
#ifdef INCLUDE_LIBDISASM

DisasmView::DisasmView(wxWindow* parent)
: DataView(this),
wxTextCtrl(parent, -1, ZSTR, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP)
{
#ifdef LIBDISASM_OLD
    disassemble_init(appSettings.DisasmOptions, INTEL_SYNTAX);
#else  // new libdisasm
    //x86_init(opt_ignore_nulls, 0, 0);  //! why doesn't this have options?  2009-02-17
    x86_init((enum x86_options)appSettings.DisasmOptions, 0, 0);
#endif
    SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));
}

DisasmView::~DisasmView()
{
#ifdef LIBDISASM_OLD
    disassemble_cleanup();
#else  // new libdisasm
    x86_cleanup();
#endif
}

extern "C" struct EXT__ARCH ext_arch;

void DisasmView::UpdateView(HexWnd* hw, int flags /*= -1*/)
{
#ifdef LIBDISASM_OLD
    if (ext_arch.options != appSettings.DisasmOptions)
    {
        disassemble_cleanup();
        disassemble_init(appSettings.DisasmOptions, INTEL_SYNTAX);
       flags = -1; // force update
    }
#else  // new libdisasm
//        x86_cleanup();
//        x86_init();
#endif

    if (hw != m_hw)
    {
        m_hw = hw;
        flags = -1;
    }
    QueueUpdates(flags);
}

#include "qstring.h"
#ifdef LIBDISASM_NEW
extern "C" x86_op_t * x86_operand_new( x86_insn_t *insn );
#endif

void DisasmView::ProcessUpdates()
{
    if (!(m_queuedUpdates & (DV_DATA | DV_SELECTION | DV_NEWDOC)))
        return;

    Clear();
    if (!m_hw || !m_hw->doc) return;

    int inst_len = 0;
    const int STD_BUF = 200;
    char instr[STD_BUF];
    TCHAR line[STD_BUF];
    Selection sel = m_hw->GetSelection();
    if (sel.GetSize() > 0xFFFF)
        sel.SetLastFromFirst(0xFFFF);
    THSIZE start = sel.GetFirst();
    size_t readsize = Add(sel.GetSize(), 10, m_hw->DocSize() - start);
    //if (readsize < 10)
    //    readsize = wxMin(10, m_hw->doc->size - sel.nStart);
    const uint8 *data = m_hw->doc->Load(sel.GetFirst(), readsize);
    if (!data)
    {
        //! error
        AppendText(_T("Couldn't read from document."));
        return;
    }

    //!Freeze();

    QString strdata;
    strdata.Grow(10 * sel.GetSize() * sizeof(TCHAR));  // rough estimate based on disassembling this code.
#ifdef LIBDISASM_NEW
    x86_insn_t insn;
    insn.operands = 0;  // My libdisasm change assumes that operands is either 0 or a valid list.
#endif

    for (size_t i = 0; i < sel.GetSize() || i == 0; i += inst_len)
    {
#ifdef LIBDISASM_OLD
        inst_len = sprint_address(instr, STD_BUF, (char*)data + i);
#else  // new libdisasm
        /* disassemble address */
        inst_len = x86_disasm((unsigned char*)data, readsize, start + m_hw->doc->display_address, i, &insn);
        int instr_len = 0;
        if ( inst_len ) {
            /* print instruction */
            instr_len = x86_format_insn(&insn, instr, STD_BUF, intel_syntax);
        }
        //x86_oplist_free(&insn);  // This seems like bad design to me.
        x86_oplist_empty(&insn);
#endif
        if (!inst_len)  // infinite loop == bad
        {
            //QString tmp;
            //tmp.Printf("inst_len == 0, i = 0x%X, address %X\n", i, start + i);
            //strdata.Append(tmp);
            break;
        }
        if (i + inst_len > readsize)  // instruction extends beyond buffer?
            break;
        for (int k = 0; instr[k]; k++)
        {
            if (instr[k] == '\t')
            {
                if (instr[k + 1] == 0)
                {
                    instr[k] = 0;
                    instr_len = k;
                    break;
                }
                else
                    instr[k] = ' ';
            }
        }

        //memset(line, ' ', STD_BUF - 1);
        for (int k = 0; k < STD_BUF; k++)
            line[k] = ' ';
        line[STD_BUF - 1] = 0;

        int len = m_hw->FormatAddress(start + i, line, STD_BUF);
        line[len] = ' ';
        len += 2;
        for (int k = 0; k < inst_len && k < 3; k++)
        {
            my_itoa((uint32)data[i + k], line + len, 16, 2);
            len += 3;
        }
        if (inst_len < 3) // pad with spaces up to 3 bytes
            len += (3 - inst_len) * 3;
        if (inst_len > 3) // put in ".." or one more space
        {
            line[len - 1] = '.';
            line[len] = '.';
        }
        len++;
        //len += _sntprintf(line + len, STD_BUF - len, _T("%hs"), instr);
        for (int k = 0; k < instr_len && len < STD_BUF - 2; k++)
            line[len++] = instr[k];
        line[len++] = '\n';
        line[len] = 0;
        //!AppendText(wxString(line, len+1));
        strdata.Append((char*)line, len * sizeof(TCHAR));
    }
    //!Thaw();
#ifdef LIBDISASM_NEW
    x86_oplist_free(&insn);
#endif
    SetValue(wxString((TCHAR*)strdata.data, strdata.length / sizeof(TCHAR)));
}


//****************************************************************************
//****************************************************************************
// DocHistoryView
//****************************************************************************
//****************************************************************************

DocHistoryView::DocHistoryView(wxWindow *parent)
: DataView(this),
  wxListBox(parent, -1)
{
}

DocHistoryView::~DocHistoryView()
{
}

void DocHistoryView::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
    if (hw != m_hw)
    {
        m_hw = hw;
        m_queuedUpdates = -1;
    }
    m_queuedUpdates |= flags;
}

void DocHistoryView::ProcessUpdates()
{
    if (!(m_queuedUpdates & (DV_DATA | DV_SELECTION | DV_NEWDOC)))
        return;

    if (!m_hw)
    {
       Clear();
       return;
    }

    Clear();

    UndoManager *undo = m_hw->doc->undo;
    if (!undo)
        return;
    for (UndoAction *ua = undo->GetFirst(); ua; ua = ua->next)
    {
        Append(ua->GetDescription());
        if (ua == undo->GetCurrent())
            SetSelection(GetCount() - 1);
    }
}

#endif // INCLUDE_LIBDISASM



//****************************************************************************
//****************************************************************************
// ExportView
// see http://win32assembly.online.fr/pe-tut7.html
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(ExportView, wxPanel)
    EVT_LIST_ITEM_ACTIVATED(IDC_DOC_LIST, OnActivate)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_MENU(IDM_GotoExportsTable, OnGotoExportsTable)
    EVT_MENU(IDM_GotoName, OnGotoName)
END_EVENT_TABLE()

ExportView::ExportView(wxWindow *parent)
: DataView(this),
wxPanel(parent, -1)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    list = new wxListCtrl(this, IDC_DOC_LIST, wxDefaultPosition, wxSize(-1, 50), wxLC_REPORT /*| wxLC_NO_HEADER*/);
    list->InsertColumn(0, _T("Name"));
    list->InsertColumn(1, _T("Ord."));
    list->InsertColumn(2, _T("RVA"));
    list->InsertColumn(3, _T("Offset"));
    //list->InsertColumn(4, _T("Size"));
    sizer->Add(list, 1, wxGROW);
    SetSizerAndFit(sizer);

    //m_menuSource = NONE;
}

void ExportView::ProcessUpdates()
{
    if (!(m_queuedUpdates & DV_DATA))
        return;

    list->DeleteAllItems();  // This is a big waste... if you don't like it, close the view window.

    if (!m_hw || !m_hw->doc)
        return;
    HexDoc *doc = m_hw->doc;

    // If the DLL has been loaded into memory, the RVA is valid so we can skip the section offsets.
    if (!doc->m_pDS->IsFile() && !doc->m_pDS->IsLoadedDLL())
        return;

    //! This won't work on big-endian machines.
    IMAGE_DOS_HEADER *doshdr = (IMAGE_DOS_HEADER*)doc->Load(0, sizeof(IMAGE_DOS_HEADER));
    if (!doshdr)
        return;

    if (doshdr->e_magic != IMAGE_DOS_SIGNATURE)  // MZ
        return;

    DWORD offset = doshdr->e_lfanew;
    if (doc->Read32(offset) != IMAGE_NT_SIGNATURE)
        return;

    // read PE header
    offset += 4;
    IMAGE_FILE_HEADER *fhdr = (IMAGE_FILE_HEADER*)doc->Load(offset, sizeof(IMAGE_FILE_HEADER));
    if (!fhdr)
        return;
    DWORD nSections = fhdr->NumberOfSections;

    offset += sizeof(IMAGE_FILE_HEADER);
    IMAGE_OPTIONAL_HEADER32 *hdr = (IMAGE_OPTIONAL_HEADER32*)doc->Load(offset, sizeof(IMAGE_OPTIONAL_HEADER32));
    if (!hdr)
        return;

    export_rva  = hdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    export_size = hdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    if (!export_rva || !export_size)
        return;

    IMAGE_SECTION_HEADER textsec, exportsec;
    if (doc->m_pDS->IsFile())
    {
        // find the section that has the export table
        DWORD iSection;
        exportsec.SizeOfRawData = 0;
        offset += sizeof(IMAGE_OPTIONAL_HEADER32);
        for (iSection = 0; iSection < nSections; iSection++)
        {
            IMAGE_SECTION_HEADER *sec = (IMAGE_SECTION_HEADER*)doc->Load(offset, sizeof(IMAGE_SECTION_HEADER));
            if (!sec)
                return;
            offset += sizeof(IMAGE_SECTION_HEADER);

            if (!memcmp(sec->Name, ".text", 5))
                textsec = *sec;

            if (sec->VirtualAddress <= export_rva &&
                sec->VirtualAddress + sec->SizeOfRawData >= export_rva + export_size)
                exportsec = *sec;  // This is the section we want.
        }
        if (exportsec.SizeOfRawData == 0)  // section containing export table not found.
            return;

        export_offset = export_rva - exportsec.VirtualAddress + exportsec.PointerToRawData;
    }
    else // if (doc->m_pDS->IsLoadedDLL())
    {
        export_offset = export_rva;
        ZeroMemory(&textsec, sizeof(textsec));
    }

    IMAGE_EXPORT_DIRECTORY *exports = (IMAGE_EXPORT_DIRECTORY *)new uint8[export_size];
    if (!doc->Read(export_offset, export_size, (uint8*)exports))
        goto done;

    DWORD addresses_offset;
    if (doc->m_pDS->IsFile())
    {
        names_offset = exports->AddressOfNames - exportsec.VirtualAddress + exportsec.PointerToRawData;
        addresses_offset = exports->AddressOfFunctions - exportsec.VirtualAddress + exportsec.PointerToRawData;
    }
    else
    {
        names_offset = exports->AddressOfNames;
        addresses_offset = exports->AddressOfFunctions;
    }

    TCHAR buf[100];
    for (DWORD n = 0; n < exports->NumberOfFunctions; n++)
    {
        wxString name;
        if (n < exports->NumberOfNames)
        {
            DWORD name_rva = doc->Read32(names_offset + n * 4);  // this should be inside the export table we already read.
            if (name_rva < export_rva || name_rva >= export_rva + export_size)  // minimal error checking
                continue;
            uint8 *pname = (uint8*)exports + name_rva - export_rva;
            name = wxString((const char*)pname, wxConvLibc);
        }

        wxListItem li;
        li.SetId(n);
        li.SetText(name);
        int item = list->InsertItem(li);

        // name, ordinal, RVA, file offset
        list->SetItem(item, 1, FormatNumber(n + exports->Base, 10));

        DWORD RVA = doc->Read32(addresses_offset + n * 4);
        m_hw->FormatAddress(RVA, buf, 100);
        list->SetItem(item, 2, buf);

        //! ugly, temporary hack: Assume functions are in the .text section.
        offset = RVA - textsec.VirtualAddress + textsec.PointerToRawData;
        m_hw->FormatAddress(offset, buf, 100);
        list->SetItem(item, 3, buf);

        list->SetItemData(item, (long)offset);  // store the file offset
    }
done:
    delete [] exports;

    for (int i = 0; i < 4; i++)
        list->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
}

void ExportView::OnActivate(wxListEvent &event)
{
    if (!m_hw)
        return;
    m_hw->CmdMoveCursor((DWORD)event.GetData());
    m_hw->SetFocus();
}

void ExportView::OnContextMenu(wxContextMenuEvent &event)
{
    wxPoint pt = event.GetPosition();
    wxMenu menu;
    //menu.AppendSeparator();
    menu.Append(IDM_GotoExportsTable, _T("Go to exports table"));
    //menu.Append(IDM_GotoRVA, _T("Go to selected RVA"));
    menu.Append(IDM_GotoName, _T("Go to selected name"));
    PopupMenu(&menu, ScreenToClient(pt));
}

void ExportView::OnGotoExportsTable(wxCommandEvent &event)
{
    if (!m_hw || !export_offset)
        return;

    m_hw->CmdSetSelection(export_offset + export_size, export_offset);
    m_hw->SetFocus();
}

void ExportView::OnGotoName(wxCommandEvent &event)
{
    if (!m_hw || !export_offset)
        return;

    int n = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (n < 0)
        return;
    DWORD name_rva = m_hw->doc->Read32(names_offset + n * 4);  // this should be inside the export table we already read.
    if (name_rva < export_rva || name_rva >= export_rva + export_size)  // minimal error checking
        return;

    m_hw->CmdSetSelection(export_offset + name_rva - export_rva,
                          export_offset + name_rva - export_rva + list->GetItemText(n).Len());
    m_hw->SetFocus();
}
