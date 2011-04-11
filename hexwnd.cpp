#include "precomp.h"
#include "hexwnd.h"
#include "hexdoc.h"
#include "atimer.h"
#include "resource.h"
#include "undo.h"
#include "scroll.h"
#include "thex.h"
#include "toolwnds.h"
#include "thFrame.h"
#include "settings.h"
#include "utils.h"
#include "datasource.h"
//#include "fatinfo.h"

#define new New

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

enum {ID_SCROLL_TIMER = 1};

struct T_VecMemDec
{
    TCHAR c[4];
};

extern T_VecMemDec DecodeVecMem(const uint8 *pRow, int offset);

BEGIN_EVENT_TABLE(HexWnd, wxWindow)
    EVT_PAINT(HexWnd::OnPaint)
    EVT_ERASE_BACKGROUND(HexWnd::OnErase)
    EVT_KEY_DOWN(HexWnd::OnKey)
    EVT_KEY_UP(HexWnd::OnKeyUp)
    //EVT_NAVIGATION_KEY(HexWnd::OnNavigationKey)
    EVT_CHAR(HexWnd::OnChar)
    EVT_LEFT_DOWN(HexWnd::OnLButtonDown)
    EVT_LEFT_UP(HexWnd::OnLButtonUp)
    EVT_MOUSE_CAPTURE_CHANGED(HexWnd::OnCaptureChange)
    EVT_MOUSE_CAPTURE_LOST(HexWnd::OnCaptureLost)
    EVT_MOTION(HexWnd::OnMouseMove)
    EVT_ENTER_WINDOW(HexWnd::OnMouseEnter)
    EVT_LEAVE_WINDOW(HexWnd::OnMouseLeave)
    EVT_MOUSEWHEEL(HexWnd::OnMouseWheel)
    //EVT_MIDDLE_DOWN(HexWnd::OnMButtonDown)
    EVT_SCROLLWIN(HexWnd::OnScroll)
    EVT_CONTEXT_MENU(HexWnd::OnContextMenu)
    EVT_SIZE(HexWnd::OnSize)
    EVT_SET_FOCUS(HexWnd::OnSetFocus)
    EVT_KILL_FOCUS(HexWnd::OnKillFocus)

    EVT_TIMER(ID_SCROLL_TIMER, HexWnd::OnScrollTimer)

    EVT_MENU(IDM_CursorStartOfLine, HexWnd::CmdCursorStartOfLine)
    EVT_MENU(IDM_CursorEndOfLine,   HexWnd::CmdCursorEndOfLine)
    EVT_MENU(IDM_CursorStartOfFile, HexWnd::CmdCursorStartOfFile)
    EVT_MENU(IDM_CursorEndOfFile,   HexWnd::CmdCursorEndOfFile)
    EVT_MENU(IDM_ViewLineUp,        HexWnd::CmdViewLineUp)
    EVT_MENU(IDM_ViewLineDown,      HexWnd::CmdViewLineDown)
    EVT_MENU(IDM_ViewPageUp,        HexWnd::CmdViewPageUp)
    EVT_MENU(IDM_ViewPageDown,      HexWnd::CmdViewPageDown)
    EVT_MENU(IDM_NextView,          HexWnd::CmdNextView)
    EVT_MENU(IDM_PrevView,          HexWnd::CmdPrevView)
    #ifdef TBDL
    EVT_MENU(IDM_ViewFontSizeUp,    HexWnd::CmdIncreaseFontSize)
    EVT_MENU(IDM_ViewFontSizeDown,  HexWnd::CmdDecreaseFontSize)
    #endif
    EVT_MENU(IDM_Cut,               HexWnd::CmdCut)
    EVT_MENU(IDM_Copy,              HexWnd::CmdCopy)
    //EVT_MENU(IDM_Paste,             HexWnd::CmdPaste)
    //EVT_MENU(IDM_PasteDlg,          HexWnd::CmdPasteAs)
    EVT_MENU(IDM_Delete,            HexWnd::CmdDelete)
    EVT_MENU(IDM_Undo,              HexWnd::CmdUndo)
    EVT_MENU(IDM_Redo,              HexWnd::CmdRedo)
    EVT_MENU(IDM_SelectAll,         HexWnd::CmdSelectAll)
    EVT_MENU(IDM_UndoAll,           HexWnd::CmdUndoAll)
    EVT_MENU(IDM_RedoAll,           HexWnd::CmdRedoAll)
    EVT_MENU(IDM_ToggleEndianMode,  HexWnd::CmdToggleEndianMode)
    EVT_MENU(IDM_ViewAbsoluteAddresses, HexWnd::CmdAbsoluteAddresses)
    EVT_MENU(IDM_ViewRelativeAddresses, HexWnd::CmdRelativeAddresses)
    EVT_MENU(IDM_CopyAddress,       HexWnd::CmdCopyAddress)
    EVT_MENU(IDM_CopyCurrentAddress,HexWnd::CmdCopyCurrentAddress)
    EVT_MENU(IDM_ViewAdjustColumns, HexWnd::CmdViewAdjustColumns)
    EVT_MENU(IDM_OffsetLeft,        HexWnd::CmdOffsetLeft)
    EVT_MENU(IDM_OffsetRight,       HexWnd::CmdOffsetRight)
    //EVT_MENU(IDM_FileNew,           HexWnd::CmdNewFile)
    EVT_MENU(IDM_ViewRuler,         HexWnd::CmdViewRuler)
    EVT_MENU(IDM_ViewStickyAddr,    HexWnd::CmdViewStickyAddr)
    EVT_MENU(IDM_ReadPalette,       HexWnd::CmdReadPalette)
    //EVT_MENU(IDM_CopyCodePoints,    HexWnd::CmdCopyFontCodePoints)
    EVT_MENU(IDM_FindDiffRec,       HexWnd::CmdFindDiffRec)
    EVT_MENU(IDM_FindDiffRecBack,   HexWnd::CmdFindDiffRecBack)
    EVT_MENU(IDM_ViewPrevRegion,    HexWnd::CmdViewPrevRegion)
    EVT_MENU(IDM_ViewNextRegion,    HexWnd::CmdViewNextRegion)

    //EVT_MENU(IDM_GotoCluster,       HexWnd::CmdGotoCluster)
    //EVT_MENU(IDM_JumpToFromFat,     HexWnd::CmdJumpToFromFat)
    //EVT_MENU(IDM_GotoPath,          HexWnd::CmdGotoPath)
    //EVT_MENU(IDM_NextBlock,         HexWnd::CmdNextBlock)
    //EVT_MENU(IDM_PrevBlock,         HexWnd::CmdPrevBlock)
    //EVT_MENU(IDM_NextBlockTrack,    HexWnd::CmdNextBlock)
    //EVT_MENU(IDM_PrevBlockTrack,    HexWnd::CmdPrevBlock)
    //EVT_MENU(IDM_FirstCluster,      HexWnd::CmdFirstCluster)
    //EVT_MENU(IDM_LastCluster,       HexWnd::CmdLastCluster)
    //EVT_MENU(IDM_FatAutoSave,       HexWnd::CmdFatAutoSave)
    //EVT_MENU(IDM_CopySector,        HexWnd::CmdCopySector)
    //EVT_MENU(IDM_OpsCustom1,        HexWnd::CmdCustomOp1)
END_EVENT_TABLE()


HexWnd::HexWnd(thFrame *frame, HexWndSettings *ps /*= NULL*/)
: m_scrollTimer(this, ID_SCROLL_TIMER), m_settings(s) //! fix names
{
    curdoc = -1;
    if (ps)
        s = *ps;
    this->frame = frame;
    m_ok = Create(frame, -1, wxDefaultPosition, wxSize(0, 0), wxWANTS_CHARS, _T("HexWnd"));
    Init();

    wxAcceleratorEntry entries[16];
    entries[0].Set(wxACCEL_NORMAL, WXK_HOME, IDM_CursorStartOfLine);
    entries[1].Set(wxACCEL_NORMAL, WXK_END, IDM_CursorEndOfLine);
    entries[2].Set(wxACCEL_CTRL, WXK_HOME, IDM_CursorStartOfFile);
    entries[3].Set(wxACCEL_CTRL, WXK_END, IDM_CursorEndOfFile);
    entries[4].Set(wxACCEL_NORMAL, WXK_TAB, IDM_NextView);
    entries[5].Set(wxACCEL_SHIFT, WXK_TAB, IDM_PrevView);
    entries[6].Set(wxACCEL_SHIFT, WXK_HOME, IDM_CursorStartOfLine);
    entries[7].Set(wxACCEL_SHIFT, WXK_END, IDM_CursorEndOfLine);
    entries[8].Set(wxACCEL_CTRL | wxACCEL_SHIFT, WXK_HOME, IDM_CursorStartOfFile);
    entries[9].Set(wxACCEL_CTRL | wxACCEL_SHIFT, WXK_END, IDM_CursorEndOfFile);
    entries[10].Set(wxACCEL_ALT | wxACCEL_SHIFT, WXK_LEFT, IDM_OffsetLeft);
    entries[11].Set(wxACCEL_ALT | wxACCEL_SHIFT, WXK_RIGHT, IDM_OffsetRight);
    entries[12].Set(wxACCEL_ALT, WXK_LEFT, IDM_PrevBlockTrack);
    entries[13].Set(wxACCEL_ALT, WXK_RIGHT, IDM_NextBlockTrack);
    entries[14].Set(wxACCEL_CTRL, WXK_LEFT, IDM_PrevBlock);
    entries[15].Set(wxACCEL_CTRL, WXK_RIGHT, IDM_NextBlock);
    //entries[16].Set(wxACCEL_NORMAL, WXK_ESCAPE, IDM_FocusHexWnd);
    SetAcceleratorTable(wxAcceleratorTable(DIM(entries), entries));
}

void HexWnd::Init()
{
    m_ok = false;
    m_iFirstLine = 0;
    m_iScrollX = 0;
    m_iCurByte = 0;
    m_iCurDigit = 0;
    m_bMousePosValid = false;
    m_iPartialScroll = 0;
    m_iCurPane = 1;
    m_iTotalLines = 0;
    m_iSelStart = 0;
    m_bMouseSelecting = false;
    m_iAddressOffset = 0;
    m_caretHideFlags = 0;
    m_dvHighlight = NULL;
    m_iSpaceCount = 0;
    m_pCharSpaces = NULL;

    m_pDS = NULL;
    doc = NULL;

    s = ghw_settings;
    DisplayPane::s_pSettings = &s; //!

    for (int i = 0; i < 4; i++)
        DocListSortOrder[i] = 1;
    DocListSortColumn = 1; // initially sorted by starting address

    m_lineBuffer = NULL;
    m_pModified = NULL;
    m_iLineBytes = 0;
    m_iLineBufferSize = 0;

    m_iYOffset = 0;
    m_iLastYScroll = 0;
    PrintPaintMessages(false);

    switch(s.iByteBase) {
        case  2: m_iByteDigits =  8; break;
        case  8: m_iByteDigits =  3; break;
        case 10: m_iByteDigits =  3; break;
        case 16: m_iByteDigits =  2; break;
    }

    SetBackgroundColour(s.GetBackgroundColour()); // wxWidgets still insists on drawing the background sometimes
    m_hbrWndBack = *wxTheBrushList->FindOrCreateBrush(s.GetBackgroundColour());
    m_hbrAdrBack = *wxTheBrushList->FindOrCreateBrush(s.clrAdrBack);
    m_penGrid = wxPen(s.clrGrid, 1, wxSOLID);

    m_iDesiredFontSize = -1;

    //SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));
    wxFont newFont;
    if (!newFont.SetNativeFontInfo(s.sFont))
        newFont = wxFont(14, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    SetFont(newFont);

    m_iCurByte = 0;
}

HexWnd::~HexWnd()
{
    ClearDocs();
    //UpdateViews(DataView::DV_CLOSE); // tell all listeners that this window is going away
    delete [] m_lineBuffer;
    delete [] m_pModified;
    delete [] m_pCharSpaces;
    //m_pDS->CleanUp();

    if (m_pDS)
       m_pDS->Release();

    ghw_settings = s; // set current window settings as new defaults
}

bool HexWnd::DataOk() const
{
   return m_pDS && m_pDS->IsOpen();
}

void HexWnd::ClearDocs()
{
    for (size_t n = 0; n < m_docs.size(); n++)
        delete m_docs[n];
    m_docs.clear();
}

void HexWnd::OnErase(wxEraseEvent &event)
{
}

void HexWnd::RepaintBytes(uint64 start_byte, uint64 end_byte, bool bUpdateNow /*= true*/)
{
    uint64 start_line = ByteToLineCol(start_byte, NULL), end_line;
    if (end_byte == start_byte || end_byte == (uint64)-1)
        end_line = start_line;
    else
        end_line = ByteToLineCol(end_byte, NULL);
    RepaintLines(start_line, end_line, bUpdateNow);
}

void HexWnd::RepaintLines(uint64 start_line, uint64 end_line, bool bUpdateNow /*= true*/)
{
    uint64 tmp = wxMin(start_line, end_line);
    end_line = wxMax(start_line, end_line);
    start_line = tmp;

    if (start_line > m_iFirstLine + m_iVisibleLines || end_line < m_iFirstLine)
        return;

    start_line = wxMax(start_line, m_iFirstLine);
    end_line = wxMin(end_line, m_iFirstLine + m_iVisibleLines);

    wxRect rc(
        0,
        LineToY(start_line),
        m_iLineWidth,
        (end_line - start_line + 1) * m_iLineHeight);
    Refresh(false, &rc);

    if (bUpdateNow)
        Update();
}

//! which is faster:  SetBkMode(OPAQUE) or Rectangle()?
//  SetBkMode(OPAQUE) is faster.
//  TextOut() is much faster than DrawText().
//  20x small rectangles take 20x longer than one big one.
//! which is faster if background is already set: SetBkMode(OPAQUE) or TRANSPARENT?
//! if WM_SETCURSOR and WM_MOUSEMOVE both need hit test, skip one.

int HexWnd::HitTest(const wxPoint &pt, uint64 &mouseByte, uint64 &mouseLine, int &digit, int &half)
{
    if (!GetClientRect().Contains(pt))
        return RGN_NONE; //! Happens in Init=>SetFont=>set_caret_pos.  //!fix this for selection mode

    if (s.bShowRuler && pt.y < m_iRulerHeight)
        return RGN_RULER; // I guess 2 is the ruler region now.

    mouseLine = YToLine(pt.y);
    int col, left = 0, right = s.iPanePad;
    int x = pt.x;

    //! todo: allow selection to start in empty space

    // check address column
    if (s.bStickyAddr && x < m_iAddrPaneWidth)
        return RGN_ADDRESS;

    x += m_iScrollX;

    for (int nPane = 0; nPane < 10; nPane++)
    {
        DisplayPane &pane = m_pane[nPane];
        if (!pane.id)
           continue;

        if (x <= pane.GetRight())
        {
            col = pane.HitTest(x, digit, half);
            mouseByte = LineColToByte(mouseLine, col * pane.m_iColBytes);
            return nPane;
        }
    }
    return RGN_NONE;
}

void HexWnd::SelectionHitTest(const wxPoint &pt, uint64 &mouseLine, int &col, int &digit, int &half)
{
    int x = pt.x + m_iScrollX;
    //int relativeLine;
    //if (pt.y < 0)
    //    relativeLine = (pt.y - m_iLineHeight + 1) / m_iLineHeight;
    //else
    //    relativeLine = pt.y / m_iLineHeight;

    //if (relativeLine < 0 && (uint64)(-relativeLine) > m_iFirstLine)
    //    mouseLine = 0;
    //else if (relativeLine < 0)
    //    mouseLine = m_iFirstLine - (uint64)(-relativeLine);
    //else
    //    mouseLine = wxMin(m_iFirstLine + (uint64)relativeLine, m_iTotalLines);
    mouseLine = YToLine(pt.y);
    if (mouseLine >= m_iTotalLines)
        mouseLine = m_iTotalLines - 1;

    col = m_pane[m_iCurPane].HitTest(x, digit, half);
}

// GetByteRect()
// Sets rectangle in client coords, if any part of it is visible.
// Returns false if the rectangle is completely outside the current view area.
bool HexWnd::GetByteRect(uint64 address, int count, int region, wxRect &rc)
{
    int col;
    uint64 line = ByteToLineCol(address, &col);
    if (line >= m_iFirstLine && line <= m_iFirstLine + m_iVisibleLines)
    {
        int start = col / m_pane[region].m_iColBytes;
        int end = DivideRoundUp(col + count, m_pane[region].m_iColBytes);
        GetColRect(line, start, end - start, region, rc, 0);
        // Rectangle is already adjusted for X and Y scrolling.
        //if (rc.x + rc.width <= 0 || rc.x >= GetClientSize().GetWidth())
        if (rc.x >= m_iPaintWidth)
            return false;
        if (s.bStickyAddr && rc.x + rc.width <= m_iAddrPaneWidth + 1)
            return false;
        return true;
    }
    else
       return false;
}

void HexWnd::GetColRect(THSIZE line, int col, int count, int region, wxRect &rc, uint32 flags)
{
    if (flags & GBR_VFILL)
    {
        rc.y = 0;
        rc.height = m_tm.tmHeight + s.iExtraLineSpace; //! leave out grid line at bottom
    }
    else
    {
        //rc.y = rel_line * m_iLineHeight + (s.iExtraLineSpace >> 1);
        rc.y = LineToY(line) + (s.iExtraLineSpace >> 1);
        rc.height = m_tm.tmHeight;
    }

    rc.x = m_pane[region].GetRect(col, count, rc.width, flags);

#ifdef _DEBUG
    //! is this ever called for the address pane?  what do we do about bStickAddr?
    if (region == 0)
        region = 0;  //! breakpoint
#endif

    if (!(flags & GBR_NOHSCROLL))
        rc.x -= m_iScrollX;
}

int HexWnd::GetByteX(int col, int region)
{
    if (region < 0 || region >= DIM(m_pane))
        return 0; //! shouldn't happen
    return m_pane[region].GetX(col);
}

void HexWnd::CalcLineWidth()
{

    m_iRulerHeight = (m_tm.tmHeight + s.iExtraLineSpace) * CountDigits(s.iLineBytes - 1, s.iAddressBase);
    m_iFirstLineTop = (s.bShowRuler ? m_iRulerHeight : 0);

    DisplayPane::s_pSettings = &s; //!

    // Put an extra pixel around columns, for fixed-width fonts only.
    // Proportional fonts tend to waste space.
    int extra = (m_bFixedWidthFont ? 1 : 0);

    // pane 0 is always the address column
    m_pane[0].Init(this, DisplayPane::ADDRESS, width_hex, 0);
    m_pane[0].Position(0, s.iPanePad);
    m_iAddrPaneWidth = m_pane[0].m_width2;
    int p = 1;

    if (0)
    {
        // tiny pixels for video memory, 2008-01-16
        int tgs = s.iGroupSpace;
        s.iGroupSpace = 0;
        m_pane[p].Init(this, DisplayPane::ANSI, m_iCharWidth, 0);
        m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
        s.iGroupSpace = tgs;
        m_iLineWidth = m_pane[p].GetRight() + 1;
        return;
    }

    //m_pane[4].Init(DisplayPane::HEX, width_hex);
    m_pane[p].InitNumeric(width_hex, 1, false, false, 16, 1/*extra*/);
    m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    p++;
    m_pane[p].Init(this, DisplayPane::ANSI, m_iCharWidth, extra);
    m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    p++;
    m_pane[p].Init(this, DisplayPane::ID_UNICODE, width_all, extra);
    m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    p++;
    //m_pane[p].Init(DisplayPane::BIN, m_iCharWidth);
    //m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    //p++;

    // 16-bit hex unsigned int
    //m_pane[p].InitNumeric(m_iCharWidth, 2, false, false, 16);
    //m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    //p++;

    // 16-bit dec signed int (great for CD audio!)
    //m_pane[p].InitNumeric(m_iCharWidth, 2, true, false, 10);
    //m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    //p++;

    // 32-bit float
    //m_pane[p].InitNumeric(m_iCharWidth, 4, false, true, 10);
    //m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    //p++;

    // 64-bit double
    //m_pane[p].InitNumeric(m_iCharWidth, 8, false, true, 10);
    //m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    //p++;

    //m_pane[p].InitVecMem(m_iCharWidth);
    //m_pane[p].Position(m_pane[p-1].GetRight() + 1, s.iPanePad);
    //p++;

    m_iLineWidth = m_pane[p-1].GetRight() + 1;
}

class Incrementer
{
public:
    Incrementer(int &i) { p = &i; i++; }
    ~Incrementer() { if (p) (*p)--; }
    int *p;
};

void HexWnd::OnSize(wxSizeEvent &event)
{
    static int InSize = 0, QueuedSize = 0;
    if (InSize)
    {
        // Busy!  Come back later.
        // There's got to be a better way to keep track of this...
        QueuedSize = 1;
        return;
    }
    Incrementer incr(InSize);

    PRINTF(_T("Resizing HexWnd to (%d x %d).\n"), event.GetSize().x, event.GetSize().y);
    if (!m_ok)
        return;
    int width, height;
    GetClientSize(&width, &height);

    // address of center line before resize
    THSIZE oldTop = LineColToByte(m_iFirstLine, 0);

    m_iTotalLines = GetLines(); // added for adjusting line bytes on the fly
    if (appSettings.bAdjustAddressDigits)
    {
        m_iLastDisplayAddress = LineColToByte(m_iTotalLines - 1, 0);
        if (s.bAbsoluteAddresses && doc)
           m_iLastDisplayAddress += doc->display_address;
        s.iAddressDigits = CountDigits(m_iLastDisplayAddress, s.iAddressBase);

        /*if (s.bPrettyAddress)
        {
           if (s.iAddressBase == 16) digits += (digits - 1) / 4;
           if (s.iAddressBase == 10) digits += (digits - 1) / 3;
           if (s.iAddressBase ==  2) digits += (digits - 1) / 8;
        }
        s.iAddressDigits = digits;*/
    }

    CalcLineWidth();

    if (s.bAdjustLineBytes)
    {
        //! this is broken for Unicode pane
        int groupWidth = 0;
        int singleWidth = 0;
        int paneCount = 0; //! 1 for address pane
        for (int nPane = 0; nPane < 10; nPane++)
        {
            DisplayPane &pane = m_pane[nPane];
            if (!pane.id || pane.id == DisplayPane::ADDRESS)
                continue;
            groupWidth += pane.GetWidth(s.iDigitGrouping) / pane.m_iColBytes;
            singleWidth += pane.GetWidth(1) / pane.m_iColBytes;
            paneCount++;
        }
        int avail = width - m_iAddrPaneWidth - (s.iPanePad * 2 + 1) * paneCount + 1; //! extra pixel for grid line at right edge
        int groups = avail / groupWidth;
        int extraBytes = (avail - (groups * groupWidth)) / singleWidth;
        s.iLineBytes = wxMax(1, groups * s.iDigitGrouping + extraBytes);

        PRINTF(_T("width %d group %d single %d avail %d groups %d extra %d line %d char %d\n"),
            width, groupWidth, singleWidth, avail, groups, extraBytes, s.iLineBytes, m_iCharWidth);

        // The above code isn't quite accurate because of grouping and multiple-byte columns.
        // Instead of fixing it, we're going to call it an estimate and do this instead:
        if (groups * groupWidth + extraBytes * singleWidth + (singleWidth / 2) < avail)
            s.iLineBytes++;
        CalcLineWidth();

        if (m_iLineWidth > GetClientSize().GetWidth() && s.iLineBytes > 1)
        {
           s.iLineBytes--;
           CalcLineWidth();
        }

        m_iTotalLines = GetLines();
    }
    //PRINTF("m_iLineWidth = %d\n", m_iLineWidth);

    if (s.bShowRuler)
        m_iVisibleLines = (height - m_iRulerHeight) / m_iLineHeight;
    else
        m_iVisibleLines = height / m_iLineHeight;

    if (m_iVisibleLines == 0)  // bad things happen
        m_iVisibleLines = 1;

    m_iFirstLine = ByteToLineCol(oldTop, NULL);

    // scroll bars are signed 32 bits, so we can only show 2G lines at once.  Big deal.
    // (Fixed with James Brown's 64-bit scroll bar idea.)

    if (m_iTotalLines >= m_iVisibleLines)
    {
        if (m_iFirstLine + m_iVisibleLines > m_iTotalLines)
            m_iFirstLine = m_iTotalLines - m_iVisibleLines;
    }
    else
        m_iFirstLine = 0;
    if (m_iScrollX >= m_iLineWidth - width)
        m_iScrollX = wxMax(m_iLineWidth - width, 0);

    // See if we can get rid of scroll bars.
    const int winHeight = event.GetSize().GetHeight();
    if (height < winHeight) // H-scroll bar shown?
    {
        int possibleLines;
        if (s.bShowRuler)
            possibleLines = (winHeight - m_iRulerHeight) / m_iLineHeight;
        else
            possibleLines = winHeight / m_iLineHeight;

        if (m_iLineWidth > width &&
            m_iLineWidth <= event.GetSize().GetWidth() &&
            m_iVisibleLines < m_iTotalLines &&
            possibleLines >= m_iTotalLines)
        {
            // We don't need scroll bars!  Now, what can we do about it...
            m_iVisibleLines = m_iTotalLines; // This should help.
        }
    }

    // Set scroll bars.  This may cause more WM_SIZE messages to be sent.
    SetScrollbar64(wxVERTICAL, m_iFirstLine, m_iVisibleLines, m_iTotalLines);

    // client area may have changed with addition of vertical scroll bar
    GetClientSize(&width, &height);

    //! Did I get this right?  Test.
    m_iScrollPageX = width;
    m_iScrollMaxX = m_iLineWidth - width;
    SetScrollbar(wxHORIZONTAL, m_iScrollX, width, m_iLineWidth);

    if (QueuedSize)
    {
        // NOW you may handle the event.
        QueuedSize = 0;
        incr.p = NULL;
        InSize = 0;
        wxSizeEvent event2(GetSize());
        OnSize(event2);
        return;
    }

    // same bitmap gets used for ruler and normal lines
    m_iPaintWidth = wxMin(width, m_iLineWidth);
    //if (s.bShowRuler)
    //    bmpLine.Create(m_iPaintWidth, wxMax(m_iLineHeight, m_iRulerHeight));
    //else
    //    bmpLine.Create(m_iPaintWidth, m_iLineHeight);

    set_caret_pos(); //! should this be part of repaint()?
    Refresh(false);  //! we may not need this here.  OK, we do if the font changes.
    //Update(); //! Without this, the background gets repainted with the default color.  Update() seems to fix it.

#ifdef TBDL
    UpdateViews(DataView::DV_WINDOWSIZE);
#endif
}

void HexWnd::OnScroll(wxScrollWinEvent &event)
{
    if (event.GetOrientation() == wxVERTICAL)
        OnVScroll(event);
    else
        OnHScroll(event);
}

void HexWnd::OnVScroll(wxScrollWinEvent &event)
{
    int cmd = event.GetEventType(); //! is this right?
    //SCROLLINFO si = { sizeof(si) };
    THSIZE oldLine = m_iFirstLine;

    if (cmd == wxEVT_SCROLLWIN_TOP)
        m_iFirstLine = 0;
    else if (cmd == wxEVT_SCROLLWIN_BOTTOM)
        m_iFirstLine = m_iTotalLines - m_iVisibleLines;
    else if (cmd == wxEVT_SCROLLWIN_LINEUP)
    {
        if (m_iFirstLine > 0)
            m_iFirstLine--;
    }
    else if (cmd == wxEVT_SCROLLWIN_LINEDOWN)
    {
        if (m_iFirstLine < m_iTotalLines - m_iVisibleLines)
            m_iFirstLine++;
    }
    else if (cmd == wxEVT_SCROLLWIN_PAGEUP)
    {
        m_iFirstLine = Subtract(m_iFirstLine, m_iVisibleLines);
    }
    else if (cmd == wxEVT_SCROLLWIN_PAGEDOWN)
    {
        m_iFirstLine = Add(m_iFirstLine, m_iVisibleLines, Subtract(m_iTotalLines, m_iVisibleLines));
    }
    else if (cmd == wxEVT_SCROLLWIN_THUMBTRACK)
    {
        //si.fMask = SIF_TRACKPOS;
        //GetScrollInfo(GetHwnd(), SB_VERT, &si);
        //m_iFirstLine = si.nTrackPos;
        m_iFirstLine = GetScrollPos64(event.GetPosition());
        PRINTF(_T("Scroll to %I64X\n"), m_iFirstLine * s.iLineBytes + doc->display_address);
    }
    else // I don't think we will ever hit this -- there are no more scroll bar commands.
        return;

    #ifdef TBDL
    thScrollWindow(m_iScrollX, oldLine);
    #else
    Refresh(false);
    #endif

    //SetScrollPos(wxVERTICAL, m_iFirstLine, cmd != SB_THUMBTRACK);
    SetScrollbar64(wxVERTICAL, m_iFirstLine, m_iVisibleLines, m_iTotalLines, cmd != wxEVT_SCROLLWIN_THUMBTRACK);
    set_caret_pos();
#ifdef TBDL
    UpdateViews(DataView::DV_SCROLL);
#endif
}

void HexWnd::OnHScroll(wxScrollWinEvent &event)
{
    int cmd = event.GetEventType(); //! is this right?
    int oldX = m_iScrollX;

    if (cmd == wxEVT_SCROLLWIN_TOP)
        m_iScrollX = 0;
    else if (cmd == wxEVT_SCROLLWIN_BOTTOM)
        m_iScrollX = m_iScrollMaxX - m_iScrollPageX;
    else if (cmd == wxEVT_SCROLLWIN_LINEUP)
    {
        if (m_iScrollX > 0)
            m_iScrollX--;
    }
    else if (cmd == wxEVT_SCROLLWIN_LINEDOWN)
    {
        if (m_iScrollX <= m_iScrollMaxX - m_iScrollPageX)
            m_iScrollX++;
    }
    else if (cmd == wxEVT_SCROLLWIN_PAGEUP)
    {
        m_iScrollX = wxMax(m_iScrollX - m_iScrollPageX, 0);
    }
    else if (cmd == wxEVT_SCROLLWIN_PAGEDOWN)
    {
        if (m_iScrollMaxX >= 2 * m_iScrollPageX && m_iScrollX <= m_iScrollMaxX - 2 * m_iScrollPageX)
            m_iScrollX += m_iScrollPageX;
        else
            m_iScrollX = m_iScrollMaxX - m_iScrollPageX;
    }
    else if (cmd == wxEVT_SCROLLWIN_THUMBTRACK)
    {
        m_iScrollX = event.GetPosition();  //! needs testing
    }
    else // I don't think we will ever hit this -- there are no more scroll bar commands.
        return;

    SetScrollPos(wxHORIZONTAL, m_iScrollX, true);
    set_caret_pos();

    #ifdef TBDL
    thScrollWindow(oldX, m_iFirstLine);
    #else
    Refresh(false);
    #endif
}

void HexWnd::OnLButtonDown(wxMouseEvent &event)
{
    // It looks like we always get WM_MOUSEMOVE to a certain position before WM_LBUTTONDOWN.
    // Aha -- not when we are not the active window, or have a menu up.
    THSIZE mouseByte;
    int /*byte,*/ digit, half;
    m_iMouseOverRegion = HitTest(event.GetPosition(), mouseByte, m_iMouseOverLine, digit, half);

    //! todo: add timer for drag/click action
    SetFocus();
    if (m_iMouseOverRegion < 0)
        return;

    if (m_iMouseOverRegion == 0) // click the address pane
    {
       wxCommandEvent cmd(wxEVT_COMMAND_MENU_SELECTED);
       CmdToggleHexDec(cmd);
       return;
    }

    m_iMouseOverCount = m_pane[m_iMouseOverRegion].m_iColBytes;

    m_bMouseSelecting = true;
    HideCaret(CHF_SELECTING);
    CaptureMouse();
    m_bMousePosValid = false;

    //SelectionHitTest(event.GetPosition(), m_iMouseOverLine, byte, digit, half);

    if (!DigitGranularity())
        digit = 0;

    if (ShiftDown()) // pretend we started dragging at the current selection start
    {
        m_iMouseDownByte = m_iSelStart;
        m_iMouseDownHalf = 0;
        m_iMouseDownDigit = 0;
        if (half > 0 && mouseByte < DocSize())
            mouseByte++;
    }
    else
    {
        m_iMouseDownByte = mouseByte;
        m_iMouseDownHalf = half;
        m_iMouseDownDigit = digit;
        PRINTF(_T("byte %X, half %d, digit %d\n"), (int)mouseByte, half, digit);
    }
    CmdMoveCursor(mouseByte, ShiftDown(), m_iMouseOverRegion, digit);
}

void HexWnd::OnLButtonUp(wxMouseEvent &event)
{
    ReleaseMouse(); // other processing is done in OnCaptureChange()
}

void HexWnd::OnCaptureChange(wxMouseCaptureChangedEvent &event)
{
    ShowCaret(CHF_SELECTING);
    if (m_bMouseSelecting)
    {
        m_bMouseSelecting = false;
        ReleaseMouse();
        m_scrollTimer.Stop(); // mouse may be outside window, so timer would still be running
    }
}

void HexWnd::OnCaptureLost(wxMouseCaptureLostEvent &event)
{
   //! wx 2.7.0 requires this event to be handled, even if we don't care.
}

void HexWnd::OnMouseMove(wxMouseEvent &event)
{
    if (m_bMouseSelecting)
    {
        uint64 mouseLine;
        int col, digit, half;
        SelectionHitTest(event.GetPosition(), mouseLine, col, digit, half);
        OnSelectionMouseMove(mouseLine, col, half);
        return;
    }

    //PRINTF("Mouse Move (%3d, %3d)\n", x, y);
    // We want to be notified when the mouse leaves our client area,
    // so we can erase the highlight over the current byte.
    // wxWidgets does this for us.  How nice.
    //TRACKMOUSEEVENT tme = { sizeof(tme) };
    //tme.dwFlags = TME_LEAVE;
    //tme.hwndTrack = GetHwnd();
    //TrackMouseEvent(&tme);

    if (frame->GetStatusBar())
        frame->SetStatusText(wxString::Format(_T("(%d, %d)   m_iScrollX=%d"), event.GetX(), event.GetY(), m_iScrollX)); //!

    uint64 mouseByte, mouseLine;
    int digit, half;
    m_iMouseOverRegion = HitTest(event.GetPosition(), mouseByte, mouseLine, digit, half);

    //int col;
    //ByteToLineCol(mouseByte, &col);
    //PRINTF("mouseover  R%d  L%I64X  B%X  d%d  h%d\n", m_iMouseOverRegion, mouseLine, col, digit, half);

    bool inPane = (m_iMouseOverRegion >= 1);

    if (inPane)
        SetCursor(wxCursor(wxCURSOR_IBEAM));
    else
        SetCursor(wxCursor(wxCURSOR_ARROW));

    if (inPane)
        m_iMouseOverCount = m_pane[m_iMouseOverRegion].m_iColBytes;
    else
        m_iMouseOverCount = 1;

    if (!inPane)
    {
        if (m_bMousePosValid == true)
        {
            m_bMousePosValid = false;
            RepaintLines(m_iMouseOverLine, m_iMouseOverLine);
        }
    }
    else // inPane
    {
        if (m_bMousePosValid && m_iMouseOverByte == mouseByte)
            return;
        if (m_bMousePosValid && mouseLine != m_iMouseOverLine)
        {
            m_bMousePosValid = false;
            RepaintLines(m_iMouseOverLine, m_iMouseOverLine);
        }
        m_iMouseOverByte = mouseByte;
        m_iMouseOverLine = mouseLine;
        m_bMousePosValid = true;
        RepaintLines(mouseLine, mouseLine);
    }
    m_iMouseOverLine = mouseLine;
}

void HexWnd::OnSelectionMouseMove(uint64 mouseLine, int col, int half)
{
    //if (mouseLine * s.iLineBytes + col * m_pane[m_iCurPane].m_iColBytes >= DocSize())
    //    col = col;

    uint64 mouseByte = LineColToByte(mouseLine, col * m_pane[m_iCurPane].m_iColBytes);
    if (mouseByte == m_iMouseDownByte && half == m_iMouseDownHalf)
    {
        CmdMoveCursor(mouseByte, ShiftDown(), 0, m_iMouseDownDigit);
        return;
    }

    if (half > 0 && mouseByte < DocSize())
        mouseByte++;
    //THSIZE iSelStart = m_iMouseDownByte;
    if (m_iMouseDownHalf > 0) // only happens once
    {
        //iSelStart++;
       m_iMouseDownByte++;
       SetSelection(m_iMouseDownByte, mouseByte);
       m_iMouseDownHalf = 0;
    }

    // Windows doesn't send WM_MOUSELEAVE if the mouse has been captured.
    // wxWidgets generates wxEVT_LEAVE_WINDOW in HandleMouseMove(), but only
    // when the mouse leaves the entire window region, not the client area.
    // Therefore, we do our wxEVT_LEAVE_WINDOW processing here if the mouse
    // has been captured (i.e. m_bMouseSelecting == true).
    // Since we check each message anyway, we will scroll when the mouse is not
    // on a line or column that is completely visible.
    //! todo: horizontal drag scrolling
    if (mouseLine < m_iFirstLine || mouseLine >= m_iFirstLine + m_iVisibleLines)
        m_scrollTimer.Start(100);

    //SetSelection(iSelStart, mouseByte);
    CmdExtendSelection(mouseByte);
}


void HexWnd::OnMouseEnter(wxMouseEvent &event)
{
    //! We don't get this event either when we have captured mouse input.  Bummer.
    m_scrollTimer.Stop();
}

void HexWnd::OnMouseLeave(wxMouseEvent &event)
{
    if (m_bMousePosValid)
    {
        m_bMousePosValid = false;
        RepaintLines(m_iMouseOverLine, m_iMouseOverLine);
    }
    return;
}

void HexWnd::OnScrollTimer(wxTimerEvent &event)
{
    wxPoint pt = ScreenToClient(wxGetMousePosition());
    PRINTF(_T("OnScrollTimer(%d, %d)\n"), pt.x, pt.y);
    uint64 mouseLine;
    int col, digit, half;
    SelectionHitTest(pt, mouseLine, col, digit, half);
    //!MakeLineVisible(mouseLine); //! no Update()

    THSIZE byte = LineColToByte(mouseLine, col);
    ScrollToRange(byte, byte, J_SHORTEST);

    OnSelectionMouseMove(mouseLine, col, half);
}

#ifdef TBDL
void HexWnd::OnMButtonDown(wxMouseEvent &event)
{
    CreateScrollOriginWnd(this, event.GetX(), event.GetY());
}
#endif

//void HexWnd::OnNavigationKey(wxNavigationKeyEvent &event)
//{
//    if (event.A
//}

//*****************************************************************************
//*****************************************************************************
// ByteIterator3
//*****************************************************************************
//*****************************************************************************

struct ByteIterator3
{
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef wxByte value_type;
    typedef wxFileOffset difference_type;
    typedef const value_type *pointer;
    typedef const value_type &reference;

    //enum {BUFFER_COUNT = 0x4000, BUFFER_MAX = 0x3FFF};
    enum {BUFFER_COUNT = 0x1000, BUFFER_MAX = 0x0FFF};

    THSIZE          pos;
    //wxFileOffset    linepos;
    HexDoc         *doc;
    uint8          *buffer;

    ByteIterator3() { buffer = new uint8[BUFFER_COUNT]; doc = NULL; }

    ~ByteIterator3() { delete [] buffer; }

    ByteIterator3(const ByteIterator3 &it)
    {
        this->operator =( it );
    }

    ByteIterator3(wxFileOffset pos0, HexDoc *doc0)
        :pos(pos0), doc(doc0)
    {
        buffer = new uint8[BUFFER_COUNT];
        Read();
    }

    ByteIterator3 & operator=(const ByteIterator3 & it)
    {
        bool read = true;
        if (doc != NULL && ((pos ^ it.pos) >> 12) == 0)
            read = false;
        pos = it.pos;
        doc = it.doc;
        if (read)
            Read();
        return *this;
    }

    const value_type operator*()
    {
        //wxASSERT(pos < doc->GetSize());
        return buffer[pos & BUFFER_MAX];
    }

    // pre-increment operator
    ByteIterator3 & operator++()
    {
        ++pos;
        if (((DWORD)pos & BUFFER_MAX) == 0)
            Read();

        return *this;
    }

    // pre-decrement operator
    ByteIterator3 & operator--()
    {
        wxASSERT(pos>0);
        --pos;
        if ((pos & BUFFER_MAX) == BUFFER_MAX)
            Read();

        return *this;
    }

    bool operator==(const ByteIterator3 & it) const
    {
        return (pos == it.pos);
    }

    bool operator!=(const ByteIterator3 & it) const
    {
        return ! (this->operator==(it)) ;
    }

    void Read();

    bool AtEnd() const;
};

void ByteIterator3::Read()
{
    int size = BUFFER_COUNT;
    THSIZE offset = pos & (THSIZE)~BUFFER_MAX;
    if (offset + size > doc->GetSize())
        size = doc->GetSize() - offset;
    doc->Read(offset, size, buffer);
}

bool ByteIterator3::AtEnd() const { return pos == doc->GetSize(); }

void TestReadSpeed(HexDoc *doc)
{
    ATimer timer;
    timer.StartWatch();
    int ffcount = 0;
    //uint8 b;

    ByteIterator3 iter(0, doc);
    while (!iter.AtEnd())
    {
        if (*iter == 0xFF)
            ffcount++;
        ++iter;
    }

    //for (THSIZE n = 0; n < doc->size; n++)
    //{
    //    if (doc->GetAt(n) == 0xFF)
    //        ffcount++;
    //}

    //doc->Seek(0);
    //for (THSIZE n = 0; n < doc->size; n++)
    //{
    //    if (doc->Next8() == 0xFF)
    //        ffcount++;
    //}

    timer.StopWatch();
    double MB = doc->size / 1048576.0;
    wxString msg;
    msg.Printf(_T("%0.2fMB in %0.3fs = %0.3fMB/s \n  count = %d"),
       MB, timer.GetSeconds(),
       MB / timer.GetSeconds(),
       ffcount);
    PRINTF(_T("%s\n"), msg);
    wxMessageBox(msg);
}

void HexWnd::OnKey(wxKeyEvent &event)
{
    // There are a few keys that it just doesn't make sense to remap.
    // Here we handle arrow keys and Page Up/Down.

    if (event.AltDown())
    {
        event.Skip();
        return;
    }

    int code = event.GetKeyCode();

    switch (code)
    {
    case WXK_TAB: // use for tab navigation
        if (event.ControlDown())
        {
            wxNavigationKeyEvent nke;
            nke.SetDirection(!event.ShiftDown());
            nke.SetWindowChange(true);
            nke.SetFromTab(true);
            nke.SetEventObject(this);
            GetParent()->GetEventHandler()->ProcessEvent(nke);
        }
        break;
    case WXK_LEFT:   CmdCursorLeft();    break;
    case WXK_RIGHT:  CmdCursorRight();   break;
    case WXK_UP:     CmdCursorUp();      break;
    case WXK_DOWN:   CmdCursorDown();    break;
    case WXK_F5:
        PRINTF(_T("Refreshing HexWnd area\n"));
        doc->InvalidateCache();
        Refresh(false);
        break;
#ifdef TBDL
    case WXK_F6: {
        THSIZE base = 0;
        for (Segment *s = doc->m_head; s; s = s->next)
        {
            PRINTF(_T("%8I64X: %p size=%8I64X next=%p\n"), base, s, s->size, s->next);
            base += s->size;
        }

        fatInfo.BuildBackwardFAT();
    } break;
    case WXK_F7:
    //    for (size_t n = 0; n < doc->modRanges.size(); n++)
    //    {
    //        ByteRange& range = doc->modRanges[n];
    //        PRINTF("%8I64X - %8I64X\n", range.start, range.end);
    //    }
        {
            THSIZE offset;
            int area = fatInfo.HitTest(GetCursor(), offset);
            if (area == FatInfo::AREA_ROOT)
               offset = 0; // screwy hack for root directory on Fat12/16
            else if (area != FatInfo::AREA_DATA)
               break;
            fatInfo.ReadDir(offset);
        }
        break;
#endif
    case WXK_F8:
        doc->DumpCache();
        break;
    case WXK_F9:
        //TestReadSpeed(doc);
        frame->CompareBuffers();
        break;
    // F10 works like Alt -- it sets keyboard focus to the menu bar.  Probably a Windows thing.
    case WXK_F10:  // see WM_KEYDOWN in MSDN docs.
        //RemoveMP3Metadata();
        NextOddMp3Frame();
        break;
    case WXK_PAGEUP: CmdCursorPageUp(/*event.ControlDown()*/); break;
    case WXK_PAGEDOWN: CmdCursorPageDown(/*event.ControlDown()*/); break;
    case WXK_RETURN:
        if (GetCursor() >= m_linkStart &&
            GetCursor() <  m_linkStart + m_linkSize)
        {
            Highlight(0, 0);
            CmdMoveCursor(m_linkTarget);
        }
        break;
    default:
        event.Skip(); // not processed
    }
}

void HexWnd::OnKeyUp(wxKeyEvent &event)
{
    switch (event.GetKeyCode())
    {
    case WXK_F10:
        return;
    default:
        event.Skip(); // not processed
    }
}

void HexWnd::OnChar(wxKeyEvent &event)
{
    if (event.HasModifiers()) // ignores shift key state
    {
        event.Skip();
        return;
    }

    int code = event.GetKeyCode();
    if (code == WXK_WINDOWS_LEFT || code == WXK_WINDOWS_RIGHT)
    { // why does wxWindow send us these?
        event.Skip();
        return;
    }

    if (IsReadOnly())
    {
        //MessageBeep((DWORD)-1);
        //return 0;
        event.Skip(); // not processed
        return;
    }

    if (!CmdChar(code))
        wxBell();
    ScrollToCursor();
}

void HexWnd::OnSetFocus(wxFocusEvent &event)
{
    //! todo: if in hex data and moving whole byte at a time, double cursor width?
    //CreateCaret(m_hWnd, NULL, m_iCharWidth, m_tm.tmHeight);

    set_caret_pos();
    ShowCaret(CHF_FOCUS);
}

void HexWnd::OnKillFocus(wxFocusEvent &event)
{
    HideCaret(CHF_FOCUS);
}

bool HexWnd::GetLineInfo(uint64 line, uint64 &address, int &count, int &start)
{
    if (line >= m_iTotalLines)
        return false;
    if (line == 0 && m_iAddressOffset)
    {
        start = s.iLineBytes - m_iAddressOffset;
        count = wxMin(m_iAddressOffset, DocSize());
        address = 0;
        return true;
    }

    start = 0;
    address = line * s.iLineBytes + m_iAddressOffset;
    if (m_iAddressOffset)
        address -= s.iLineBytes;

    if (address + s.iLineBytes <= DocSize())
        count = s.iLineBytes;
    else
        count = DocSize() - address;
    return true;
}

uint64 HexWnd::LineColToByte(uint64 line, int col)
{
    uint64 address;
    int byteCount, start;
    if (!GetLineInfo(line, address, byteCount, start))
        //return 0;
        return DocSize(); // assume the caller is asking for something beyond EOF

    if (line == 0 && m_iAddressOffset)
        col -= start;

    if (col < 0)
        col = 0;
    if (col > byteCount)
        col = byteCount;
    return address + col;
}

uint64 HexWnd::ByteToLineCol(uint64 byte, int *pCol)
{
    if (!doc) {
        if (pCol)
            *pCol = 0;
        return 0;
    }
    if (m_iAddressOffset)
        byte += s.iLineBytes - m_iAddressOffset;
    if (byte > DocSize())
        byte = DocSize();
    if (pCol)
        *pCol = byte % s.iLineBytes;
    return byte / s.iLineBytes;
}

THSIZE HexWnd::LineColToByte2(THSIZE line, int col)
{
    return LineColToByte(line, col * m_pane[m_iCurPane].m_iColBytes);
}

THSIZE HexWnd::ByteToLineCol2(THSIZE byte, int *pCol)
{
    THSIZE line = ByteToLineCol(byte, pCol);
    if (pCol)
        *pCol /= m_pane[m_iCurPane].m_iColBytes;
    return line;
}

bool HexWnd::ScrollToLine(uint64 line, int x /*= -1*/)
{
    THSIZE oldLine = m_iFirstLine;
    int oldX = m_iScrollX, dy = 0;

    //! could use ScrollWindow() here
    if (m_iTotalLines > m_iVisibleLines)
    {
        if (line > m_iTotalLines - m_iVisibleLines)
            line = m_iTotalLines - m_iVisibleLines;
    }
    else
        line = 0;

    if (x == -1)
        x = m_iScrollX;
    else if (x < 0)
        x = 0;  //! This shouldn't happen.
    int maxX = m_iLineWidth - GetClientSize().GetWidth();
    if (maxX > 0) {
        if (s.bStickyAddr)
            maxX += m_iAddrPaneWidth;
        if (x > maxX)
            x = maxX;  // clip to available width
    }
    else if (x > 0)
        x = 0;  // This happens when trying to center on the right half of the screen.

    if (m_iFirstLine == line && m_iScrollX == x) // already in the right spot
    {
        set_caret_pos();
        return false;
    }

    if (m_iFirstLine != line)
    {
        m_iFirstLine = line;
        //SetScrollPos(wxVERTICAL, m_iFirstLine, true); //! does this send any window messages?
        SetScrollbar64(wxVERTICAL, m_iFirstLine, m_iVisibleLines, m_iTotalLines, true);
    }

    if (m_iScrollX != x)
    {
        m_iScrollX = x;
        SetScrollPos(wxHORIZONTAL, x, true);
    }

#ifdef TBDL
    UpdateViews(DataView::DV_SCROLL);
#endif

    m_bMousePosValid = false;

    #ifdef TBDL
    thScrollWindow(oldX, oldLine);
    #else
    Refresh(false);
    #endif
    set_caret_pos();
    return true;
}

//bool HexWnd::MakeLineVisible(uint64 line)
//{
//    if (line >= m_iFirstLine + m_iVisibleLines)
//        return ScrollToLine(line - m_iVisibleLines + 1);
//    else if (line < m_iFirstLine)
//        return ScrollToLine(line);
//    set_caret_pos(); //! do we need this?  2006-09-11 // yeah, for CmdChar().
//    return false;
//}

// ScrollToRange()
// Scroll the document so that the ending point is visible, and the starting
// point if possible.
// Jumpiness parameter (0-2) determines how hard we try to center the selection.
bool HexWnd::ScrollToRange(THSIZE start, THSIZE end, int jumpiness, int pane /*= -1*/)
{
    int hjump = (jumpiness >> 4) & 0x0F, vjump = jumpiness & 0x0F;
    if (pane < 0)
        pane = m_iCurPane;

    int reqCol, optCol;
    if (start < end)
        end--; // go the the last byte of the selection, not the first byte outside.
    THSIZE reqLine = ByteToLineCol2(end, &reqCol);
    THSIZE optLine = ByteToLineCol2(start, &optCol);

    int reqL, optL, reqWidth, optWidth, optR, padL, padR, padWidth;
    reqL = m_pane[pane].GetRect(reqCol, 1, reqWidth, 0);
    if (reqLine == optLine)
    {
        optL = m_pane[pane].GetRect(optCol, 1, optWidth, 0);
        optR = wxMax(reqL + reqWidth, optL + optWidth);
        optL = wxMin(optL, reqL);
        optWidth = optR - optL;
    }
    else // selection spans multiple lines, so try to include entire horizontal area
    {
        optL = m_pane[pane].m_start1;
        optWidth = m_pane[pane].m_width1;
        optR = optL + optWidth;
    }
    padL = Subtract(optL, 2 * m_iCharWidth);  // allow some extra space
    padR = Add(optR, 2 * m_iCharWidth, m_iLineWidth);
    padWidth = padR - padL;

    // find new horizontal scroll position
    int width = GetClientSize().GetWidth();
    int x = m_iScrollX;
    if (s.bStickyAddr)
    {
        //width -= m_iAddrPaneWidth;
        width = Subtract(width, m_iAddrPaneWidth);
        padL = Subtract(padL, m_iAddrPaneWidth);
        padR = Subtract(padR, m_iAddrPaneWidth);
        optL = Subtract(optL, m_iAddrPaneWidth);
        optR = Subtract(optR, m_iAddrPaneWidth);
        reqL = Subtract(reqL, m_iAddrPaneWidth);
        //reqR = Subtract(reqR, m_iAddrPaneWidth);
    }

    // Special case for pressing the Home key in the left-most pane
    if (padR < width / 2)
    {
        x = 0;
        goto x_done;
    }

    // if jumpiness == 0, move shortest distance to get optWidth on screen.
    // Center on padded width.
    if (hjump > 0)
        x = Subtract(padL + padWidth / 2, width / 2);
    // Show as much of desired width as possible.
    if (x + width < optR)
        x = optR - width;
    if (x > optL)
        x = optL;
    // Show as much of required width as possible.
    if (x + width < reqL + reqWidth)
        x = reqL + reqWidth - width;
    if (x > reqL)
        x = reqL;
    //! todo: if (lazy), only worry about reqWidth?

    //if (s.bStickyAddr)
    //    x += m_iAddrPaneWidth;
    //    //x -= m_iAddrPaneWidth;

x_done:
    if (vjump == JV_AUTO || vjump == JV_CENTER) // center vertically if possible?
    {
        THSIZE tmp;
        if (reqLine >= optLine)
            tmp = reqLine - optLine + 1;    // count lines
        else
            tmp = optLine - reqLine + 1;    // count lines
        if (tmp <= m_iVisibleLines)         // fits on one page?
        {
            tmp = (reqLine + optLine) / 2;  // get midpoint
            // If jumpiness == 2, always center.
            // If jumpiness == 1, center only if the selection is currently off the screen.
            if (vjump == JV_CENTER ||
                tmp < m_iFirstLine ||
                tmp > m_iFirstLine + m_iVisibleLines
                )
                return ScrollToLine(Subtract(tmp, m_iVisibleLines / 2), x);
        }
    }

    if (m_iVisibleLines <= 1)  // extreme zoom
        return ScrollToLine(reqLine, x);
    else if (reqLine >= optLine)
    {
        if (reqLine - optLine > m_iVisibleLines || // too big for one page
            reqLine >= m_iFirstLine + m_iVisibleLines) // fits on a page and we move forward
            return ScrollToLine(reqLine - m_iVisibleLines + 1, x);
        else if (optLine < m_iFirstLine) // fits on a page and we move back
            return ScrollToLine(optLine, x);
    }
    else // reqLine < optLine
    {
        if (optLine - reqLine >= m_iVisibleLines || // too big for one page
            reqLine < m_iFirstLine) // fits on a page and we move back
            return ScrollToLine(reqLine, x);
        else if (optLine >= m_iFirstLine + m_iVisibleLines) // fits on a page and we move forward
            return ScrollToLine(optLine - m_iVisibleLines + 1, x);
    }

    // else: fits on a page and we don't move vertically.  Might move horizontally.
    return ScrollToLine(m_iFirstLine, x);
}

//bool HexWnd::CenterLine(THSIZE line)
//{
//    if (line < m_iVisibleLines / 2)
//        line = 0;
//    else
//        line -= m_iVisibleLines / 2;
//    return ScrollToLine(line);
//}

void HexWnd::set_caret_pos()
{
    //! todo: different cursor for insert/overwrite mode?
    wxRect rc;
    if (GetByteRect(m_iCurByte, 1, m_iCurPane, rc))
    {
        rc.x++; //! adjust for phantom pixel
        const DisplayPane& pane = m_pane[m_iCurPane];
        if (pane.m_iColChars > 1)
            rc.x += pane.m_iCharWidth * m_iCurDigit;
        rc.width = pane.m_iCharWidth;
        if (appSettings.bInsertMode && m_iCurDigit == 0)
            rc.SetWidth(2);

        if (s.bStickyAddr)
        {
            const int left = m_iAddrPaneWidth + 1;
            if (rc.GetRight() < left)
                goto no_caret;
            else if (rc.x < left)
            {
                rc.width -= (left - rc.x);
                rc.x = left;
            }
        }

        if (1)
        {
             wxCaret* caret = GetCaret();
             if (caret)
                 caret->SetSize(rc.GetSize());
             else
                 SetCaret(caret = new wxCaret(this, rc.width, rc.height));
             caret->Move(rc.x, rc.y);
             ShowCaret(CHF_OFFSCREEN);
             //PRINTF(" Caret %3d, %3d\n", rc.x, rc.y);
             return;
        }
    }

no_caret:  // Bad ass!  No caret for you!
    HideCaret(CHF_OFFSCREEN);
    //PRINTF(" Caret hidden\n");
}

void HexWnd::OnMouseWheel(wxMouseEvent &event)
{
    int delta = event.GetWheelRotation();
    #ifdef TBDL
    if (event.ControlDown())
    {
        wxCommandEvent dummy;
        if (delta > 0)
            CmdIncreaseFontSize(dummy);
        else if (delta < 0)
            CmdDecreaseFontSize(dummy);
    }
    else
    #endif // TBDL
    {
        UINT ucNumLines = 1;
        #ifdef WIN32 //! TBDL
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &ucNumLines, 0);
        #endif
        if (ucNumLines == (UINT)-1)
            ucNumLines = wxMax(m_iVisibleLines - 1, 1);
        ScrollPartialLine(delta * ucNumLines, event.GetWheelDelta());
    }
}

void HexWnd::ScrollPartialLine(int delta, int wheelDelta)
{
    m_iPartialScroll += delta;
    int lines = m_iPartialScroll / -wheelDelta;
    m_iPartialScroll %= wheelDelta;
    if (lines < 0)
    {
        if (m_iFirstLine > (uint32)-lines)
            ScrollToLine(m_iFirstLine + lines);
        else
            ScrollToLine(0);
    }
    else if (lines > 0)
        ScrollToLine(m_iFirstLine + lines);
}

bool HexWnd::SetDataSource(DataSource *pDS)
{
    ClearDocs();

    if (m_pDS)
    {
        //m_pDS->CleanUp(); // This fixes some of the horrible reference loops.
        m_pDS->Release();
        doc = NULL;
    }
    if (!pDS || !pDS->IsOpen())
        return false;

    // Don't do pDS->AddRef() here.  I don't get reference counting.
    m_pDS = pDS;
    //return SetDoc(pDS->GetRootView());
    m_docs = pDS->GetDocs();

    //! hack for searching all regions
    for (size_t i = 0; i < m_docs.size(); i++)
        m_docs[i]->hw = this;

    return SetDoc(m_docs[0]);
}

bool HexWnd::SetDoc(HexDoc *pDoc)
{
    pDoc->pSettings = &s; //! is this OK?  NO!!!
    int newdoc = -1;
    for (size_t n = 0; n < docinfo.size(); n++)
        if (docinfo[n].doc == pDoc)
        {
            newdoc = n;
            break;
        }
    if (newdoc == -1)
    {
        newdoc = docinfo.size();
        DOC_INFO di = {pDoc};
        docinfo.push_back(di);
    }
    if (newdoc != curdoc && curdoc >= 0) // save stuff for old document
    {
        GetSelection(docinfo[curdoc].sel);
        docinfo[curdoc].firstLine = m_iFirstLine;
    }

    this->doc = pDoc;
    doc->hw = this;

    //OnDataChange(0, 0, DocSize());
    m_iTotalLines = GetLines();
    AdjustForNewDataSize();
#ifdef TBDL
    UpdateViews(DataView::DV_DATA | DataView::DV_DATASIZE | DataView::DV_NEWDOC);
#endif

    SetSelection(docinfo[newdoc].sel);
    ScrollToLine(docinfo[newdoc].firstLine);
    SetModified(false); //! WRONG

#ifdef TBDL
    fatInfo.Init(this);
#endif

    curdoc = newdoc;
    return true;
}

wxString HexWnd::GetTitle()
{
    if (m_pDS)
        return m_pDS->GetTitle();
    else
        return _T("[No file]");
}

bool HexWnd::OpenFile(LPCTSTR filename, bool bReadOnly)
{
    DataSource *pDS = new FileDataSource(filename, bReadOnly);
    if (!pDS->IsOpen())
    {
        delete pDS;
        return false;
    }
    return SetDataSource(pDS);

    //m_sFileName = filename;
    //HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    //GetFileSizeEx(hFile, &m_liFileSize);
    //BY_HANDLE_FILE_INFORMATION bhfi;
    //GetFileInformationByHandle(hFile, &bhfi);
    //m_ftLastWriteTime = bhfi.ftLastWriteTime;
    //CloseHandle(hFile);

    //char path[MAX_PATH];
    //LPSTR filepart;
    //GetFullPathName(filename, MAX_PATH, path, &filepart);
    //filepart[0] = 0;

    //m_hChange = FindFirstChangeNotification(path, FALSE, FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);
    //if (m_hChange == INVALID_HANDLE_VALUE)
    //{
    //    wxString msg;
    //    msg.Printf("Couldn't watch changes for %s\nCode: %d", filename, GetLastError());
    //    fatal(msg);
    //}
    //else
    //!    g_wol.Add(m_hChange, s_FileChange, (DWORD_PTR)this);
}

#ifdef TBDL
bool HexWnd::OpenDrive(LPCTSTR path, bool bReadOnly)
{
    DataSource *pDS = new DiskDataSource(path, bReadOnly);
    if (!pDS->IsOpen())
    {
       delete pDS;
       return false;
    }
    SetDataSource(pDS);

    //! Temporary code for br0ken HD.  2007-10-18
    //uint8 fill = 0xAB;
    //doc->bWriteable = true;
    //doc->ReplaceAt(0x4C000, 0x34000, &fill, 1, 0x34000);
    //doc->ReplaceAt(0xC000C000, 0xffe00, &fill, 1, 0xffe00);
    //doc->bWriteable = false;
    return true;
}

bool HexWnd::OpenProcess(DWORD pid, wxString procName, bool bReadOnly)
{
    DataSource *pDS = new ProcMemDataSource(pid, procName, bReadOnly);
    if (!pDS->IsOpen())
    {
       delete pDS;
       return false;
    }
    SetDataSource(pDS);
    return true;
}

bool HexWnd::OpenLC1VectorMemory(wxString addr)
{
#ifdef LC1VECMEM
    wxIPV4address ipv4;
    ipv4.Hostname(addr);
    ipv4.Service(0x721F);

    DataSource *pDS = new VecMemDataSource(ipv4, true);
    if (!pDS->IsOpen())
    {
       delete pDS;
       return false;
    }
    SetDataSource(pDS);
    return true;
#else
    wxMessageBox(_T("LC-1 vector memory support disabled in this build."), _T("T. Hex"));
    return false;
#endif // LC1VECMEM
}

bool HexWnd::OpenTestFile(LPCTSTR filename)
{
    DataSource *pDS = new CachedFile(filename);
    if (!pDS->IsOpen())
    {
       delete pDS;
       return false;
    }
    SetDataSource(pDS);
    return true;
}
#endif // TBDL

bool HexWnd::OpenDataSource(DataSource *pDS)
{
    if (!pDS->IsOpen())
        return false;
    pDS->AddRef();
    return SetDataSource(pDS);
}

uint64 HexWnd::GetFileOffset(uint64 address)
{
    if (address < doc->display_address)
        return 0;
    if (address > doc->display_address + DocSize())
        return doc->display_address + DocSize();
    return address - doc->display_address;
}

bool HexWnd::Goto(uint64 offset, bool bExtendSel /*= false*/, int origin /*= 0*/)
{
    uint64 address;
    if (origin == 0) // absolute
        address = GetFileOffset(offset);
    else if (origin == 1) // forward from start of region
        address = offset;
    else if (origin ==2) // forward from cursor
        address = wxMin(offset + m_iCurByte, DocSize());
    else if (origin == 3) // backward from cursor
    {
        if (offset > m_iCurByte)
            address = 0;
        else
            address = m_iCurByte - offset;
    }
    else if (origin == 4) // backward from end of region
    {
        if (offset > DocSize())
            address = 0;
        else
            address = DocSize() - offset;
    }
    else
        return false; // unknown origin

    CmdMoveCursor(address, bExtendSel);
    return true;

    //! todo: if we allow overlapping regions, there may be multiple regions containing
    //  the desired location.  Allow user to pick one from a dialog box?
}

#ifdef WIN32
UINT HexWnd::GetCharWidths(HDC hdc, UINT first, UINT count, bool bUseDrawText /*= false*/)
{
    uint32 width = 0;
    if (count == 0)
        return 0; //! error
    UINT last = first + count - 1;

    if (m_tm.tmPitchAndFamily & TMPF_TRUETYPE) // TrueType
    {
        ABC *abcWidths = new ABC[count];
        if (!GetCharABCWidthsW(hdc, first, last, abcWidths))
            last = GetLastError();
        for (UINT i = 0; i < count; i++)
        {
            UINT tw = abcWidths[i].abcB;
            if (abcWidths[i].abcA >= 0)
                tw += (UINT)abcWidths[i].abcA;
            else
                iLeftLead[first + i] = -abcWidths[i].abcA;

            //if (abcWidths[i].abcC > 0)
            //    tw += abcWidths[i].abcC; //! not terribly useful, I think.

            //! This is necessary when the font renderer tries to synthesize characters
            //  that aren't in the font.
            if (bUseDrawText && !iCharWidths[first + i])
            {
                RECT rc = {0, 0, 0, 0};
                WCHAR str[1] = { first + i };
                ::DrawTextW(hdc, str, 1, &rc, DT_CALCRECT);
                tw = (UINT)rc.right;
            }

            iCharWidths[first + i] = tw;
            if (tw > width)
                width = tw;
        }
        delete [] abcWidths;
    }
    else // not TrueType
    {
         //! Should we even show Unicode pane with a non-TT font?  -Yes.  It still helps.
        int *widths = new int[count];
        if (!::GetCharWidth32W(hdc, first, last, widths))
        {
            widths[0] = GetLastError(); //! breakpoint
            memset(widths, 0, count * sizeof(widths[0]));
        }

        for (UINT i = 0; i < count; i++)
        {
            iCharWidths[first + i] = width = wxMax((int)width, widths[i]);
            iLeftLead[first + i] = 0;
        }
        delete [] widths;
    }
    return width;
}

int GetDefaultCharGlyph(int codePage = CP_ACP)
{
    // Find default character in font.
    // Unicode "Interpunct" (Middle dot)

    //if (hdc)  // Not helpful, since we don't use glyph indices.
    //{
    //    WORD glyph = 0;
    //    DWORD dw = GetGlyphIndicesW(hdc, L"\xB7", 1, &glyph, GGI_MARK_NONEXISTING_GLYPHS);
    //    if (dw != 1) {
    //        dw = GetLastError();  //! breakpoint
    //        glyph = '.';
    //    }
    //    return glyph;
    //}
    //else
    {
        char c;
        if (WideCharToMultiByte(codePage, 0, L"\xB7", 1, &c, 1, NULL, NULL))
            return (int)(uint8)c;
        int err = GetLastError(); //! breakpoint
        return '.';
    }
}

void HexWnd::SetFont(LOGFONT *plf)
{
    plf->lfQuality = m_settings.GetFontQuality();
    SetFont(wxCreateFontFromLogFont(plf));
}
#endif // WIN32

void HexWnd::SetFont(wxFont &newFont)
{
#ifdef TBDL
    // First tweak the font (if necessary) to set lfQuality.
    //LOGFONT lf;
    LOGFONT &lf = m_lf;
    GetObject((HFONT)newFont.GetHFONT(), sizeof(lf), &lf);
    BYTE desiredQuality = m_settings.GetFontQuality();
    if (lf.lfQuality != desiredQuality) // Set wrong?  Fix it.
    {
        lf.lfQuality = desiredQuality;  //! Ignored when comparing newFont == m_font.
        newFont = wxCreateFontFromLogFont(&lf); //! Why doesn't this set the right info?  (family)
    }
    if (lf.lfOutPrecision == OUT_STRING_PRECIS)
        m_bFontCharsOnly = true;  // GDI doesn't step in for raster fonts, so draw dots instead.
#endif // TBDL

    s.sFont = newFont.GetNativeFontInfoDesc();

    m_bFontCharsOnly = s.bFontCharsOnly;

    wxWindow::SetFont(newFont);  //! Ignores font quality setting.
    m_iDesiredFontSize = -1; //! weirdness
    m_bFixedWidthFont = newFont.IsFixedWidth();

    #ifdef WIN32
    GetCharacterSizes(newFont);
    #else  //! LINUX TESTING
    m_tm.tmHeight = 20;
    m_tm.tmInternalLeading = 2;
    m_tm.tmAveCharWidth = 10;
    m_tm.tmMaxCharWidth = 10;
    width_all = width_hex = m_iCharWidth = 10;
    m_iDefaultChar = ' ';
    #endif

    m_iLineHeight = m_tm.tmHeight + s.iExtraLineSpace;
    if (s.bGridLines)
        m_iLineHeight++;
    CalcLineWidth();

    PRINTF(_T("New font height=%d IntLead=%d AveWidth=%d MaxWidth=%d w256=%d w16=%d w.all=%d\n"),
        m_tm.tmHeight, m_tm.tmInternalLeading, m_tm.tmAveCharWidth, m_tm.tmMaxCharWidth, m_iCharWidth, width_hex, width_all);

    set_caret_pos();

    AdjustForNewDataSize();
}

#ifdef TBDL
// Set m_tm, iLeftLead, iCharWidths, width_all, width_hex,
//     charMap256, m_iCodePage, m_iDefaultChar
void HexWnd::GetCharacterSizes(const wxFont &newFont)
{
    HFONT hFont = (HFONT)newFont.GetHFONT();

    HDC hdc = GetDC(NULL);
    SelectObject(hdc, hFont);
    GetTextMetrics(hdc, &m_tm);
    ReleaseDC(NULL, hdc);

    // This would be nice, but it doesn't work.
    //m_iDefaultChar = m_tm.tmDefaultChar;

    //m_iCharWidth = m_tm.tmMaxCharWidth;
    //m_iCharWidth = m_tm.tmAveCharWidth;
    width_hex = 0;
    width_all = 0;
    uint32 width = 0;

    for (int i = 0; i < DIM(iLeftLead); i++)
        iLeftLead[i] = 0;

    memset(iCharWidths, 0, sizeof(iCharWidths));
    m_iCodePage = CP_ACP;

    m_iCharWidth = 1;
    //! try to find a reasonable character width
    if (1)
    {
        HDC hdc = GetDC(NULL);
        SelectObject(hdc, hFont);

        //! This is wrong, especially for TT fonts.  We shouldn't include control characters.
        //!m_iCharWidth = GetCharWidths(hdc, 0, 256);

        width_all = m_tm.tmMaxCharWidth; // starting point.  We will almost certainly find wider glyphs.
#if _WIN32_WINNT >= 0x0500
        //if (m_tm.tmPitchAndFamily & TMPF_TRUETYPE) // TrueType
        if (1) // GetFontUnicodeRanges() works for "MS Dialog Light"
        {
            ATimer timer; // time this operation
            timer.StartWatch();

            //! This code works, but it doesn't get complex script characters that are
            //  drawn even though they are not included in the font.
            GLYPHSET* pgs;
            DWORD cBytes = GetFontUnicodeRanges(hdc, NULL);
            pgs = (GLYPHSET*) new char[cBytes];
            GetFontUnicodeRanges(hdc, pgs);
            width = GetLastError();
            int codePoints = 0;
            for (DWORD range = 0; range < pgs->cRanges; range++)
            {
               width = GetCharWidths(hdc, pgs->ranges[range].wcLow, pgs->ranges[range].cGlyphs);
               width_all = wxMax(width, width_all);

               int first = pgs->ranges[range].wcLow, count = pgs->ranges[range].cGlyphs, last = first + count - 1;
               //PRINTF(_T("Character range: %04X - %04X (%d)\n"), first, last, count);
               codePoints += count;
            }
            PRINTF(_T("Total: %d code points in %d ranges.\n"), codePoints, pgs->cRanges);
            delete [] pgs; // was allocated as array of chars

            //GLYPHSET* pgs;
            //DWORD cBytes = GetFontUnicodeRanges(hdc, NULL);
            //pgs = (GLYPHSET*) new char[cBytes];
            //GetFontUnicodeRanges(hdc, pgs);
            //width = GetLastError();
            //int codePoints = 0;
            //for (int range = 0; range < pgs->cRanges; range++)
            //{
            //    int first = pgs->ranges[range].wcLow;
            //    for (int i = 0; i < pgs->ranges[range].cGlyphs; i++)
            //    {
            //        iCharWidths[first + i] = 1; // not zero
            //    }
            //}
            //delete [] pgs; //! was allocated as array of chars.  Is this OK?

            //width = GetCharWidths(hdc, 0, 0xD800, 0);
            //width_all = wxMax(width_all, width);

            //width = GetCharWidths(hdc, 0xE000, 0x2000, 0);
            //width_all = wxMax(width_all, width);

            timer.StopWatch();
            PRINTF(_T("GetCharWidths() took %0.6f seconds total.\n"), timer.GetSeconds());
            //m_iCharWidth = width_all;
        }
        else
#endif
        {
            //for (int i = 0; i < 256; i++)
            //    charMap256[i] = i;
            width_all = m_iCharWidth = GetCharWidths(hdc, 0, 256);
        }

        // Get maximum width of characters needed to display hexadecimal digits (hexits?)
        width = GetCharWidths(hdc, '0', 10);
        width_hex = wxMax(width_hex, width);
        width = GetCharWidths(hdc, 'A', 6);
        width_hex = wxMax(width_hex, width);
        //width = GetCharWidths(hdc, 'a', 6);
        //width_hex = wxMax(width_hex, width);

        charMap256[0] = 0;
        //int cp = 437; //GetTextCharset(hdc);
        int charset = GetTextCharset(hdc); //lf.lfCharSet;
        charset = m_tm.tmCharSet;
        if (charset == OEM_CHARSET)
        {
            m_iCodePage = CP_OEMCP;
            for (int i = 0; i < 256; i++)
                charMap256[i] = i;
            m_iDefaultChar = GetDefaultCharGlyph(CP_OEMCP);
        }
        else
        {
            CHARSETINFO csi;
            int rc2 = TranslateCharsetInfo((DWORD*)charset, &csi, TCI_SRCCHARSET);
            if (rc2 == 0)
                PRINTF(_T("TranslateCharsetInfo() failed.  Code %d\n"), rc2 = GetLastError());
            //int cp = m_iCodePage = csi.ciACP; //437;
            m_iDefaultChar = GetDefaultCharGlyph(csi.ciACP);
            if (s.iCodePage == 0)
                m_iCodePage = csi.ciACP;
            else
                m_iCodePage = s.iCodePage;
            for (int i = 0; i < 256; i++)
            {
                WCHAR wc[5];
                char c = (char)i;
                int rc = MultiByteToWideChar(m_iCodePage, MB_USEGLYPHCHARS | MB_ERR_INVALID_CHARS, &c, 1, wc, 5);
                if (rc == 1 && iCharWidths[wc[0]])
                    charMap256[i] = wc[0];
                else
                    charMap256[i] = m_iDefaultChar;
            }
        }
        if (s.bDrawNulAsSpace)
           charMap256[0] = ' ';
        for (int i = 0; i < 256; i++)
            m_iCharWidth = wxMax(m_iCharWidth, iCharWidths[charMap256[i]]);
        m_iCharWidth = wxMax(m_iCharWidth, width_hex);

        ReleaseDC(NULL, hdc);
    }
    else //! for testing font sizes, draw characters as if they were one big string
    {
        for (int i = 0; i < 255; i++)
            iLeftLead[i] = 0;
        m_iCharWidth = m_tm.tmAveCharWidth;
    }
}
#endif // TBDL

void HexWnd::SetSelection(uint64 iSelStart, uint64 iSelEnd, int region /*= 0*/, int digit /*= 0*/)
{
    // It's OK to use region=0 as the default because 0 is the address pane.
    //! (Unless you want the cursor in the address pane... maybe someday?)

    //! where do we do region checking?
    if (iSelStart == m_iSelStart &&
        iSelEnd == m_iCurByte &&
        (region == m_iCurPane || region == 0) &&
        digit == m_iCurDigit)
        return;

    Highlight(0, 0); // turn off highlighting

    uint64 iSelStartLine, iSelEndLine;

    //PRINTF("SetSelection(%x, %x)\n", (int)iSelStart, (int)iSelEnd);

    // first erase old selection
    iSelStartLine = ByteToLineCol(m_iSelStart, NULL);
    iSelEndLine   = ByteToLineCol(m_iCurByte, NULL);
    m_iSelStart = m_iCurByte;
    RepaintLines(iSelStartLine, iSelEndLine, false);

    m_iSelStart = iSelStart;
    m_iCurByte = iSelEnd;
    if (region > 0) //! do we use region 0?
        m_iCurPane = region;
    if (iSelStart != iSelEnd)
        m_iCurDigit = 0;
    else
        m_iCurDigit = minmax(digit, 0, GetByteDigits(m_iCurPane) - 1);

    // draw new selection
    iSelStartLine = ByteToLineCol(m_iSelStart, NULL);
    iSelEndLine   = ByteToLineCol(m_iCurByte, NULL);
    RepaintLines(iSelStartLine, iSelEndLine);

#ifdef TBDL
    UpdateViews(DataView::DV_SELECTION);
#endif
}

#ifdef WIN32
void HexWnd::thScrollWindowRaw(int dx, int dy, bool bUpdate /*= false*/)
{
    RECT rcScroll, rcUpdate;
    ::GetClientRect(GetHwnd(), &rcScroll);
    if (s.bShowRuler)
    {
        // Exclude ruler from scrolled region.
        rcScroll.top = m_iRulerHeight;

        if (dx) // Ruler has to move, too.
        {
            RECT rcTmp = {0, 0, m_iPaintWidth, m_iRulerHeight};
            if (s.bStickyAddr)
                rcTmp.left = m_iAddrPaneWidth;
            ScrollWindowEx(GetHwnd(), -dx, 0, &rcTmp, &rcTmp, NULL, &rcUpdate, SW_INVALIDATE);
        }
    }
    if (s.bStickyAddr && dx)
    {
        // Exclude address from scrolled region
        rcScroll.left = m_iAddrPaneWidth + 1;

        if (dy) // Address has to move, too.
        {
            RECT rcTmp = {0, rcScroll.top, rcScroll.left, rcScroll.bottom};
            ScrollWindowEx(GetHwnd(), 0, -dy, &rcTmp, &rcTmp, NULL, &rcUpdate, SW_INVALIDATE);
        }
    }
    ScrollWindowEx(GetHwnd(), -dx, -dy, &rcScroll, &rcScroll, NULL, &rcUpdate, SW_INVALIDATE);
    if (bUpdate)
        Update();
}

void HexWnd::thScrollWindow(int oldX, THSIZE oldLine, bool bUpdate /*= false*/)
{
    if ((m_iFirstLine >= oldLine && m_iFirstLine - oldLine < m_iVisibleLines) ||
        (m_iFirstLine <  oldLine && oldLine - m_iFirstLine < m_iVisibleLines) )
    {
        RepaintLines(m_iMouseOverLine, m_iMouseOverLine, false);
        thScrollWindowRaw(m_iScrollX - oldX, m_iLineHeight * (int)(m_iFirstLine - oldLine), bUpdate);
    }
    else
    {
        Refresh(false);
        if (bUpdate)
            Update();
    }
}

int HexWnd::OnSmoothScroll(int dx, int dy)
{
    bool updateVScroll = false, updateHScroll = false;

    if (dy)
       m_iLastYScroll = dy;

    if (dx > 0)
    {
        RECT rc;
        ::GetClientRect(GetHwnd(), &rc);
        dx = wxMin(dx, m_iLineWidth - rc.right - m_iScrollX);
    }
    if (dx < 0)
        dx = wxMax(dx, -m_iScrollX);
    m_iScrollX += dx;

    if (dx)
        updateHScroll = true;

    //int max_y_smooth = m_iVisibleLines * m_iLineHeight;
    int max_y_smooth = wxMin(3, m_iVisibleLines) * m_iLineHeight;
    if (dy > max_y_smooth)
    {
        m_iYOffset = 0;
        dy -= dy % max_y_smooth;
    }
    else if (-dy > max_y_smooth)
    {
        m_iYOffset = 0;
        dy += -dy % max_y_smooth;
    }

    if (dy < 0)
    {
        if (m_iFirstLine == 0 && m_iYOffset == 0)
            dy = 0;
        else if ((uint64)(abs(dy + m_iYOffset) / m_iLineHeight) >= m_iFirstLine)
            dy = -wxMin(-dy, (int)m_iFirstLine * m_iLineHeight + m_iYOffset);
    }
    else if (dy > 0)
    {
        if (m_iFirstLine + m_iVisibleLines >= m_iTotalLines)
            dy = 0;
        else if (m_iFirstLine + m_iVisibleLines + (m_iYOffset + dy) / m_iLineHeight >= m_iTotalLines)
            dy = wxMin((uint32)dy, (m_iTotalLines - m_iVisibleLines - m_iFirstLine) * m_iLineHeight - m_iYOffset);
    }
    m_iYOffset += dy;
    while (m_iYOffset >= m_iLineHeight)
    {
        if (m_iFirstLine + m_iVisibleLines < m_iTotalLines)
        {
            m_iFirstLine++;
            m_iYOffset -= m_iLineHeight;
        }
        else
            m_iYOffset = 0;
        updateVScroll = true;
    }
    //while (m_iYOffset <= -m_iLineHeight)
    while (m_iYOffset < 0)
    {
        if (m_iFirstLine > 0)
        {
            m_iFirstLine--;
            m_iYOffset += m_iLineHeight;
        }
        else
            m_iYOffset = 0;
        updateVScroll = true;
    }
    //ScrollWindow(-dx, -dy, NULL); //! this works if scroll origin window is not WS_CHILD

    thScrollWindowRaw(dx, dy, true);

    if (updateVScroll)
    {
        //SetScrollPos(wxVERTICAL, m_iFirstLine, true);
        SetScrollbar64(wxVERTICAL, m_iFirstLine, m_iVisibleLines, m_iTotalLines, true);
        #ifdef TBDL
        UpdateViews(DataView::DV_SCROLL);
        #endif
    }
    if (updateHScroll)
        SetScrollPos(wxHORIZONTAL, m_iScrollX, true);

    HideCaret(CHF_AUTOPAN);

    return 0;
}

void HexWnd::FinishSmoothScroll()
{
    //int yStep = SGN(m_iYOffset);
    int yStep = SGN(m_iLastYScroll);

    if (m_bMousePosValid)
    {
        m_bMousePosValid = false;
        RepaintLines(m_iMouseOverLine, m_iMouseOverLine, false);
    }

#if 0
    while (m_iYOffset)
    {
        OnSmoothScroll(0, yStep);
        Sleep(10);
    }
#else
    int dist = 0;
    if (yStep > 0 && m_iYOffset)
        dist = m_iLineHeight - m_iYOffset;
    else if (yStep < 0)
        dist = m_iYOffset;

    // Feel like a bit of math today?  How about a geometric series?
    double r = .95;  // magic number that makes the speed look right
    double a = dist * (1 - r) / (1 - pow(r, wxMin(dist, 30)));
    int d0 = dist;
    int s;

    for (s = 0; dist; s++, a *= r)
    {
        int d = a + .5;
        if (d == 0 || d0 < 30)
            d = 1;
        if (dist - d < 0)
            d = dist;
        OnSmoothScroll(0, yStep * d);
        Sleep(10);
        dist -= d;
    }

#endif

    //RepaintLines(m_iMouseOverLine, m_iMouseOverLine);
    // The mouse doesn't move, but the line under it does.
    wxMouseEvent me;
    wxPoint pt = ::wxGetMousePosition();
    ScreenToClient(pt);
    me.m_x = pt.x;
    me.m_y = pt.y;
    OnMouseMove(me);

    ShowCaret(CHF_AUTOPAN);
    set_caret_pos(); // may have been changed in OnSetFocus()
}

wxString FormatTime(FILETIME ft)
{
    SYSTEMTIME stUTC, stLocal;

    // Convert the last-write time to local time.
    FileTimeToSystemTime(&ft, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    // Build a string showing the date and time.
    wxString str;
    str.Printf(_T("%02d/%02d/%d  %02d:%02d"),
        stLocal.wMonth, stLocal.wDay, stLocal.wYear,
        stLocal.wHour, stLocal.wMinute);

    return str;
}

void HexWnd::OnFileChange()
{
    HANDLE hFile = CreateFile(m_sFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    LARGE_INTEGER liFileSize;
#if _WIN32_WINNT >= 0x0500
    GetFileSizeEx(hFile, &liFileSize);
#else
    liFileSize.LowPart = GetFileSize(hFile, &liFileSize.HighPart);
#endif
    BY_HANDLE_FILE_INFORMATION bhfi;
    GetFileInformationByHandle(hFile, &bhfi);
    CloseHandle(hFile);

    wxString msg, tstr;
    if (liFileSize.QuadPart != m_liFileSize.QuadPart) {
        tstr.Printf(_T("File size changed from %d to %d.\n"), m_liFileSize.LowPart, liFileSize.LowPart);
        msg += tstr;
    }

    if (CompareFileTime(&bhfi.ftLastWriteTime, &m_ftLastWriteTime)) {
        wxString time1 = FormatTime(m_ftLastWriteTime);
        wxString time2 = FormatTime(bhfi.ftLastWriteTime);
        tstr.Printf(_T("File time changed from %s to %s.\n"), time1.c_str(), time2.c_str());
        msg += tstr;
    }

    if (!msg.Len())
        msg = _T("Some other file was changed.");

    wxMessageBox(msg, _T("Ahed"), MB_OK, this);

    FindNextChangeNotification(m_hChange);
}
#endif // WIN32

void HexWnd::ShowCaret(int flags)
{
    m_caretHideFlags &= ~flags;
    wxCaret *caret = GetCaret();
    if (m_caretHideFlags == 0 && caret && !caret->IsVisible())
        caret->Show();
    //! todo: create caret here if necessary?
}

void HexWnd::HideCaret(int flags)
{
    m_caretHideFlags |= flags;
    wxCaret *caret = GetCaret();
    if (m_caretHideFlags && caret && caret->IsVisible())
        caret->Hide();
}

int HexWnd::GetByteDigits(int mouseRegion /*= -1*/)
{
    if (mouseRegion == -1)
        mouseRegion = m_iCurPane;
    return m_pane[mouseRegion].m_iColChars;
}

bool HexWnd::DigitGranularity()
{
    // allow per-digit editing only if we can overwrite in the current document
    return (doc->IsWriteable() && !appSettings.bInsertMode);
}

void HexWnd::AdjustForNewDataSize()
{
    wxSizeEvent dummy;
    OnSize(dummy);
    return;

    ////! this is broken
    ////! todo: split this into data and GUI stuff (much of it is both)
    //if (!m_ok)
    //    return;

    //m_iTotalLines = GetLines();

    ////! todo: if we don't need any scroll bars, remove them.

    //if (m_iTotalLines >= m_iVisibleLines)
    //{
    //    if (m_iFirstLine + m_iVisibleLines > m_iTotalLines)
    //        m_iFirstLine = m_iTotalLines - m_iVisibleLines;
    //}
    //else
    //    m_iFirstLine = 0;

    //ResizeScrollBars();

    //set_caret_pos(); //! should this be part of repaint()?
    //Refresh(false);  //! we may not need this here.  OK, we do if the font changes.
    //Update(); // Without this, the background gets repainted with the default color.  Update() seems to fix it.
}

void HexWnd::ResizeScrollBars()
{
    wxSize csize = GetClientSize();
    wxSize wsize = GetSize(); // window size, including non-client area

    THSIZE lines = GetVisibleLines(wsize.y);
    if (wsize.x >= m_iLineWidth && lines >= m_iTotalLines &&
        csize.x < m_iLineWidth && m_iVisibleLines < m_iTotalLines)
    {
        // Special case, when window size is big enough but client size isn't.
        // Remove both scrollbars.

        SetScrollbar(wxHORIZONTAL, 0, wsize.x, wsize.x); // cheat!
    }
    else
    {
        // set horizontal scroll bar
        SetScrollbar(wxHORIZONTAL, m_iScrollX, csize.x, m_iLineWidth);
    }

    csize = GetClientSize();
    m_iVisibleLines = GetVisibleLines(csize.y);

    // set vertical scroll bar
    SetScrollbar64(wxVERTICAL, m_iFirstLine, m_iVisibleLines, m_iTotalLines);

    // client area may have changed with addition of vertical scroll bar
    wxSize csize2 = GetClientSize();
    if (csize2 != csize)
        SetScrollbar(wxHORIZONTAL, m_iScrollX, csize2.x, m_iLineWidth);
}

void HexWnd::OnDataChange(THSIZE nAddress, THSIZE nOldSize, THSIZE nNewSize)
{
    int flags = DataView::DV_DATA;
    if (nNewSize == nOldSize)
    {
        RepaintBytes(nAddress, nAddress + nOldSize);
    }
    else
    {
        flags |= DataView::DV_DATASIZE;
        if (doc)
            m_iTotalLines = GetLines();
        AdjustForNewDataSize();
    }

#ifdef TBDL
    UpdateViews(flags);
#endif
}

bool HexWnd::SetModified(bool modified)
{
    //if (modified)
    //    frame->SetStatusText("Modified", SBF_);
    //else
    //    frame->SetStatusText("Not modified", 1);
    return modified;
}

int HexWnd::FormatAddress(THSIZE address, TCHAR *buffer, int bufsize)
{
    if (doc && s.bAbsoluteAddresses)
        address += doc->display_address;

    if (s.bPrettyAddress)
    {
        size_t len = FormatNumber(address, buffer, bufsize, s.iAddressBase, s.iAddressDigits);
        if (len)
            return len;
        else
        {
            #ifdef WIN32
            _tcscpy(buffer, _T("??"));
            #else
            buffer[0] = buffer[1] = '?';
            buffer[2] = 0;
            #endif
            return 2;
        }
    }
    else
    {
        if (buffer == NULL) // count digits, don't write anything
            return CountDigits(address, s.iAddressBase);
        if (s.iAddressDigits > bufsize)
            return 0;
        my_itoa(address, buffer, s.iAddressBase, s.iAddressDigits);
        buffer[s.iAddressDigits] = 0;
        return s.iAddressDigits;
    }
}

HexWndSettings *DisplayPane::s_pSettings; //! this is awful

void DisplayPane::Init(HexWnd *hw, int id, int iCharWidth, int extra /*= 1*/)
{
    this->id = id;
    this->m_iCharWidth = iCharWidth;
    this->m_extra = extra;

    if (id == ADDRESS)
    {
        m_iColGrouping = 1;
        m_iGroupSpace = 0;

        // It's not actually the last address + 1 that matters, but that's easier to use,
        // and doesn't change during auto-layout.
        m_iColChars = hw->FormatAddress(hw->DocSize(), NULL, 100);
        m_iColChars = hw->FormatAddress(hw->GetLastDisplayAddress(), NULL, 100);

        m_iCols = 1;
        m_iColBytes = 0;  //! will this work?
        m_hbrBack = *wxTheBrushList->FindOrCreateBrush(s_pSettings->clrAdrBack);
    }
    else if (id == HEX)
    {
        m_iColGrouping = s_pSettings->iDigitGrouping;
        m_iGroupSpace = s_pSettings->iGroupSpace;
        m_iColChars = 2;
        m_iCols = s_pSettings->iLineBytes;
        m_iColBytes = 1;
        m_hbrBack = *wxTheBrushList->FindOrCreateBrush(s_pSettings->clrTextBack);
    }
    else if (id == ANSI)
    {
        m_iColGrouping = 1;
        m_iGroupSpace = 0;
        m_iColChars = 1;
        m_iCols = s_pSettings->iLineBytes;
        m_iColBytes = 1;
        m_hbrBack = *wxTheBrushList->FindOrCreateBrush(s_pSettings->clrAnsiBack);
    }
    else if (id == ID_UNICODE)
    {
        m_iColGrouping = 1;
        m_iGroupSpace = 0;
        m_iColChars = 1;
        m_iCols = s_pSettings->iLineBytes / 2;
        m_iColBytes = 2;
        m_hbrBack = *wxTheBrushList->FindOrCreateBrush(s_pSettings->clrUnicodeBack);
    }
    else if (id == BIN)
    {
        m_iColGrouping = s_pSettings->iDigitGrouping;
        m_iGroupSpace = s_pSettings->iGroupSpace;
        m_iColChars = 8;
        m_iCols = s_pSettings->iLineBytes;
        m_iColBytes = 1;
        m_hbrBack = *wxTheBrushList->FindOrCreateBrush(s_pSettings->clrTextBack); //! need better names both places clrTextBack is used.
    }

    m_iColWidth = m_iColChars * m_iCharWidth + m_extra * 2; // extra pixel to each side of each column
    m_width1 = m_iCols * m_iColWidth +
        m_iGroupSpace * ((m_iCols - 1) / m_iColGrouping);
}

void DisplayPane::InitNumeric(int iCharWidth, int iBytes, bool bSigned, bool bFloat, int iBase, int extra /*= 1*/)
{
    // This pane should be able to display data in any standard C type.
    // signed/unsigned, char/short/int/long long, float/double
    // long double?  Sure, why not.

    this->id = NUMERIC;
    this->m_iCharWidth = iCharWidth;
    this->m_extra = extra;

    m_iGroupSpace = s_pSettings->iGroupSpace;
    m_iCols = s_pSettings->iLineBytes / iBytes;
    m_iColBytes = iBytes;
    m_hbrBack = *wxTheBrushList->FindOrCreateBrush(s_pSettings->clrTextBack);

    m_bSigned = bSigned;
    m_bFloat = bFloat;
    m_iBase = iBase;
    //m_iEndianness = endianness;

    //! todo: floating point formats
    // 0.001 123.4 (align decimal point?  yes/no)
    // exponential notation

    //! what do we do if iBytes is some weird value that won't work?
    if (bFloat)
    {
        bSigned = false;
        if (iBytes == 4)
            m_iColChars = 13; // -1.23456e+78
        else
            m_iColChars = 15; // -1.234567e+890
    }

    if (!bFloat)
    {
        if (iBytes == 8 && !bSigned)
            m_iMax = (uint64)-1;
        else
        {
            int bits = iBytes * 8;
            if (bSigned)
                bits--;
            m_iMax = ((uint64)1 << bits) - 1;
        }
        m_iColChars = ceil(log((double)m_iMax) / log((double)iBase));
        if (bSigned)
            m_iColChars++;
    }

    if (bSigned)
        m_iColGrouping = 1;
    else
        m_iColGrouping = wxMax(4 / iBytes, 1);

    //m_iColWidth = m_iColChars * m_iCharWidth + 2; // 1 pixel to each side of each column
    m_iColWidth = m_iColChars * m_iCharWidth + m_extra * 2; // extra pixel to each side of each column
    m_width1 = m_iCols * m_iColWidth +
        m_iGroupSpace * ((m_iCols - 1) / m_iColGrouping);
}

void DisplayPane::InitVecMem(int iCharWidth)
{
    this->id = VECMEM;
    this->m_iCharWidth = iCharWidth;
    this->m_extra = 0;

    m_iColGrouping = 1;
    m_iGroupSpace = 0;
    m_iColChars = 4;
    m_iCols = s_pSettings->iLineBytes;
    m_iColBytes = 1;
    m_hbrBack = *wxTheBrushList->FindOrCreateBrush(s_pSettings->clrTextBack);

    m_iColWidth = m_iColChars * m_iCharWidth; // no extra pixels, so position may be off
    m_width1 = m_iCols * m_iColWidth +
        m_iGroupSpace * ((m_iCols - 1) / m_iColGrouping);
}

void DisplayPane::Position(int start, int pad)
{
    m_left = start;
    m_start1 = start + pad;
    m_width2 = m_width1 + pad * 2;
}

int DisplayPane::GetX(int col)
{
    int group = col / m_iColGrouping;
    return m_start1 + m_extra +
        col * m_iColWidth + group * m_iGroupSpace;
}

int DisplayPane::HitTest(int x, int &digit, int &half)
{
    x -= m_start1;
    if (x < 0)
    {
        digit = -1;
        half = 0; // left
        return 0;
    }
    if (x >= m_width1)
    {
        digit = m_iColChars;
        half = 1; // right;
        return m_iCols - 1;
    }

    //! todo: snap toward right to nearest byte
    int groupWidth = m_iColWidth * m_iColGrouping + m_iGroupSpace;
    int group = x / groupWidth;
    x -= group * groupWidth;
    int col = x / m_iColWidth;
    x -= col * m_iColWidth;
    col += group * m_iColGrouping;
    digit = (x - 1) / m_iCharWidth;
    if (x > m_iColWidth / 2)
        half = 1;
    else
        half = 0;
    return col;
}

int DisplayPane::GetRect(int col, int count, int &width, uint32 flags)
{
    int x = GetX(col) - m_extra;
    if (count == 0)
        width = 0;
    else if (count == 1)
        width = m_iColWidth;
    else
        width = GetX(col + count - 1) - m_extra - x + m_iColWidth;
    if ((flags & GBR_LSPACE) && col > 0 && col % m_iColGrouping == 0)
    {
        x -= m_iGroupSpace;
        width += m_iGroupSpace;
    }
    if ((flags & GBR_RSPACE) && col < m_iCols - 1 && (col + count) % m_iColGrouping == 0)
    {
        width += m_iGroupSpace;
    }
    return x;
}

int DisplayPane::GetWidth(int cols)
{
    int groups = cols / m_iColGrouping;
    int width = 0;
    if (groups > 0)
        width += groups * m_iColWidth * m_iColGrouping + m_iGroupSpace;
    width += (cols % m_iColGrouping) * m_iColWidth;
    return width;
}

void HexWnd::UpdateViews(int flags)
{
    if (frame)
        frame->UpdateViews(this, flags);
}

void HexWnd::OnContextMenu(wxContextMenuEvent &event)
{
    wxPoint pt = event.GetPosition();
    int pane = 0;
    if (pt == wxPoint(-1, -1))
        pt = GetCaret()->GetPosition();
    else
    {
        pt = ScreenToClient(pt);
        THSIZE mouseLine, mouseByte;
        int digit, half;
        pane = HitTest(pt, mouseByte, mouseLine, digit, half);
    }
    wxMenu menu;
    if (pane == RGN_ADDRESS)
    {
        menu.Append(IDM_CopyAddress, _T("Copy Address"));
        menu.AppendSeparator();
        menu.AppendCheckItem(IDM_ViewAbsoluteAddresses, _T("Absolute addresses"));
        menu.AppendCheckItem(IDM_ViewRelativeAddresses, _T("Relative addresses"));
        if (s.bAbsoluteAddresses)
            menu.Check(IDM_ViewAbsoluteAddresses, true);
        else
            menu.Check(IDM_ViewRelativeAddresses, true);
    }
    else if (pane == RGN_RULER) // ruler
    {
        //if (s.bShowRuler)
        //    menu.Append(IDM_ViewRuler, "Hide &Ruler");
        //else
        //    menu.Append(IDM_ViewRuler, "Show &Ruler"); // pointless, since you can't click it when hidden
        menu.AppendCheckItem(IDM_ViewRuler, _T("Show &Ruler"));
        menu.Check(IDM_ViewRuler, s.bShowRuler);
    }
    else
        menu.Append(-1, _T("Nothing"));
    PopupMenu(&menu, pt);
}

// 64-bit scrolling from James Brown (http://www.catch22.net/tuts/scroll64.asp)

#define WIN16_SCROLLBAR_MAX 0x8000
#define WIN32_SCROLLBAR_MAX 0x7fffffff

void HexWnd::SetScrollbar64(int orientation, THSIZE nPos64, int nPage, THSIZE nMax64, bool refresh /*= true*/)
{
    int nPos32, nMax32;

    m_iMaxScroll = nMax64;

    // normal scroll range requires no adjustment
    if(nMax64 <= WIN32_SCROLLBAR_MAX)
    {
        nPos32 = (int)nPos64;
        nMax32 = (int)nMax64;
    }
    // scale the scrollrange down into allowed bounds
    else
    {
        nPos32 = (int)(nPos64 / (nMax64 / WIN16_SCROLLBAR_MAX));
        nMax32 = (int)WIN16_SCROLLBAR_MAX;
        // page size probably won't matter
    }

    SetScrollbar(orientation, nPos32, nPage, nMax32, refresh);
}

THSIZE HexWnd::GetScrollPos64(int nPos32)
{
    // normal scroll range requires no adjustment
    if(m_iMaxScroll <= WIN32_SCROLLBAR_MAX)
        return nPos32;

    // special-case: scroll position at the very end
    if(nPos32 == WIN16_SCROLLBAR_MAX - m_iVisibleLines)
        return m_iMaxScroll - m_iVisibleLines;

    // adjust the scroll position to be relative to maximum value
    return (THSIZE)nPos32 * (m_iMaxScroll / WIN16_SCROLLBAR_MAX);
}

THSIZE HexWnd::GetLastVisibleByte()
{
    if (m_iFirstLine + m_iVisibleLines >= m_iTotalLines)
        return DocSize();
    return LineColToByte(m_iFirstLine + m_iVisibleLines - 1, s.iLineBytes - 1);
}

THSIZE HexWnd::GetLastVisibleLine()
{
    if (m_iVisibleLines > m_iTotalLines)
        return m_iTotalLines;
    return m_iFirstLine + m_iVisibleLines;
}

void HexWnd::MoveCursorIntoView()
{
    int col;
    THSIZE cursorLine = ByteToLineCol(m_iCurByte, &col);
    if (cursorLine >= m_iFirstLine + m_iVisibleLines)
        SetSelection(LineColToByte(m_iFirstLine + m_iVisibleLines - 1, col));
    else if (cursorLine < m_iFirstLine)
        SetSelection(LineColToByte(m_iFirstLine, col));
    set_caret_pos();
}

void HexWnd::Highlight(THSIZE start, THSIZE size, DataView *dv /*= NULL*/)
{
    if (dv && m_dvHighlight && dv != m_dvHighlight)
        return;
    Selection old = m_highlight;
    if (m_highlight.nStart == start && m_highlight.nEnd == start + size)
        return; //! is there a better way to say this?
    m_highlight.Set(start, start + size);
    //Refresh(false);
    RepaintBytes(old.nStart, old.nEnd, false);
    RepaintBytes(start, start + size, false);
}

int HexWnd::LineToY(THSIZE line)
{
    if (line + 1 < m_iFirstLine) //! +1 to allow for partial lines when panning
        return -1;
    if (line > m_iFirstLine + m_iVisibleLines + 1) //! +1 to allow for partial lines when panning
        return -1;
    int y = (line - m_iFirstLine) * m_iLineHeight;
    if (s.bShowRuler)
        y += m_iRulerHeight;
    return y - m_iYOffset;
}

THSIZE HexWnd::YToLine(int y)
{
    if (s.bShowRuler)
        y -= m_iRulerHeight;
    y += m_iYOffset;
    if (y < 0)
    {
        int minusLines = -((y - m_iLineHeight + 1) / m_iLineHeight);
        if (m_iFirstLine >= minusLines)
            return m_iFirstLine - minusLines;
        return 0;
    }
    else
        //return Add(m_iFirstLine, y / m_iLineHeight, m_iTotalLines - 1); //! this breaks painting
        return Add(m_iFirstLine, y / m_iLineHeight);
}

int HexWnd::GetPaneLeft(const DisplayPane &pane)
{
    if (pane.id == DisplayPane::ADDRESS && s.bStickyAddr)
        return 0;
    return pane.m_left - m_iScrollX;
}

#ifdef TBD
void DisplayPane::SetFont(wxFont &newFont, wxDC &dc, TEXTMETRIC &tm)
{
    //! much to do here
}

void UnicodePane::SetFont(wxFont &newFont, wxDC &dc, TEXTMETRIC &tm)
{
    this->m_iCharWidth = tm.tmMaxCharWidth;
}
#endif // TBD

THSIZE HexWnd::GetLines() const
{ //! includes space at (size+1).  ok if !CanChangeSize?
    return (DocSize() + m_iAddressOffset) / s.iLineBytes + 1;
}

uint32 HexWnd::GetVisibleLines(int height /*= -1*/) const
{
    int width, lines;
    if (height == -1)
        GetClientSize(&width, &height);

    if (s.bShowRuler)
        lines = (height - m_iRulerHeight) / m_iLineHeight;
    else
        lines = height / m_iLineHeight;
    return wxMax(lines, 1);
}

bool HexWnd::OpenBlank()
{
    return SetDataSource(new UnsavedDataSource());
}

bool HexWnd::ToggleReadOnly()
{
    if (!m_pDS || !m_pDS->ToggleReadOnly())
        return false;
    doc->bWriteable = m_pDS->IsWriteable();
    return true;  //! what else do we need to do here?
}

void HexWnd::Hyperlink(THSIZE offset, THSIZE size, THSIZE target)
{
    Highlight(offset, size);
    m_linkStart = offset;
    m_linkSize = size;
    m_linkTarget = target;
}

#ifdef WIN32
// Force mono-spaced drawing of proportional font.  Test 2007-09-06
void HexWnd::CenterTextOut(HDC hdc, int x, int y, int flags, RECT *pRC,
                           LPCTSTR text, int charCount, int cellWidth)
{
    if (charCount == 0)
        return;
    if (charCount < 0)
        charCount = _tcslen(text);
    if (m_iSpaceCount < charCount)
    {
        delete [] m_pCharSpaces;
        m_iSpaceCount = wxMax(charCount, 64);
        m_pCharSpaces = new int[m_iSpaceCount];
    }

    int lead = (cellWidth - iCharWidths[text[0]]) / 2, last = lead;
    for (int i = 1; i < charCount; i++)
    {
        int next = (cellWidth - iCharWidths[text[i]]) / 2;
        m_pCharSpaces[i - 1] = cellWidth - last + next;
        last = next;
    }
    ExtTextOut(hdc, x + lead, y, flags, pRC, text, charCount, m_pCharSpaces);
}
#endif // WIN32

void HexWnd::CenterTextOut(wxDC &dc, int x, int y, wxString text, int cellWidth)
{
    int charCount = text.Len();

    for (int i = 0; i < charCount; i++)
    {
        int lead = (cellWidth - iCharWidths[text[i]]) / 2;
        dc.DrawText(text[i], x + lead, y);
        x += cellWidth;
    }
}

void HexWnd::RemoveMP3Metadata()
{
   int *score = new int[8192], *start = new int[8192];
   int maxscore = 0, index = 0;
   uint8 *data = new uint8[DocSize()];
   int size = DocSize();
   doc->Read(0, size, data);

   //! Currently only handles zeros every 8192 bytes.
   //for (int i = 0; i < 8192; i++)
   //{
   //   int run;
   //   int offset = i;
   //   score[i] = 0;
   //   start[i] = 0;
   //   bool lastZero = false;
   //   for (run = 0; offset < size; run++, offset += 8192)
   //   {
   //      if (data[offset] == 0)
   //      {
   //         if (!lastZero) {
   //            lastZero = true;
   //            start[i] = offset;
   //         }
   //         score[i]++;
   //         if (score[i] > maxscore) {
   //            maxscore = score[i];
   //            index = i;
   //         }
   //         offset++;
   //      }
   //      else
   //         lastZero = false;
   //   }
   //}

   for (int i = 0; i < size; i++)
   {
      if (data[i] == 0)
      {
         int run = 1;
         for (int j = i + 8193; j < size && data[j] == 0; j += 8193, run++)
            ;
         if (run > maxscore) {
            maxscore = run;
            index = i;
         }
      }
   }

   int id = wxMessageBox(wxString::Format(_T("Index %d (0x%X): count %d (%0.1f%% coverage)"), index, index, maxscore, maxscore * 100.0 * 8192 / size), _T("Delete?"), wxYES_NO);
   if (wxYES == id) //! wxMessageDialog doc says wxID_YES ?!?
   {
       for (int i = 0; i < maxscore; i++)
       {
          // was 8193, but we remove one byte each time.  Is this OK?
          if (data[index + i * 8193] == 0)
             doc->RemoveAt(index + i * 8192, 1);
          else
             wxMessageBox(_T("Error."));
       }
   }

   //for (int pos = start[index]; data[pos] == 0; pos += 8192


   delete [] score;
   delete [] start;
}

void HexWnd::NextOddMp3Frame()
{
    THSIZE start = m_iCurByte, end = DocSize();
    s.iEndianMode = BIGENDIAN_MODE;
    const int bitrates[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,-1};
    THSIZE expected = 0, laststart = 0, lastend;
    if (doc->FindHex((const wxByte*)"\xFF\xFB", 2, start, end) <= 0)
        return;
    while (start < DocSize() - 4)
    {
        if (expected > 0 && start != expected)
        {
            CmdSetSelection(laststart, start);
            SetCurrentView((DataView *)this); //! bad.
            if (start < expected)
                Highlight(start, expected - start, (DataView*)this); //! bad.
            else
                Highlight(expected, start - expected, (DataView*)this); //! bad.
            return;
        }
        uint32 hdr = doc->Read32(start);
        int br = bitrates[(hdr >> 12) & 0x0F];
        if (br <= 0)
        {
            CmdSetSelection(start, start + 4);
            wxMessageBox(_T("Bad MPEG-1 header."));
            return;
        }
        int fs = 144 * br * 1000 / 44100;
        if (hdr & 0x200)
            fs++;
        laststart = start;
        lastend = end;
        end = DocSize();
        expected = start + fs;
        if (doc->Read16(expected) == 0xFFFB)
            start = expected;
        else {
            start++;
            if (doc->FindHex((const wxByte*)"\xFF\xFB", 2, start, end) <= 0)
                break;
        }
        end = DocSize();
    }
    wxMessageBox(_T("End of file."));
}

void HexWnd::UpdateSettings(HexWndSettings &ns)
{
    HexWndSettings os = s;
    s = ns;
    #ifdef TBDL
    if (ns.sFont != os.sFont)
        SetFont(GetFont());  // hey, it works.
    #endif // TBDL
    AdjustForNewDataSize();  // And this should pretty much take care of the rest.
}

#ifdef LC1VECMEM
static const char ABES_V_Formats[17] = "XQLH01NP89ABCDEF";

static const char * opcode_table[] =
{
   "ADV", "MSSA", "DINT", "EINT", "RINT", "0x05", "SCAN", "0x07", "CRF0",  "SF0", "CRF1",  "SF1", "CRF2",  "SF2", "CRF3",  "SF3",  //0
  "HALT", "0x11", "0x12", "0x13", "0x14", "0x15", "0x16", "0x17", "RETI", "0x19", "0x1a", "0x1b",  "RPT", "0x1d", "0x1e", "0x1f",  //1
   "LI0", "0x21",  "LI1", "0x23",  "LI2", "0x25",  "LI3", "0x27", "0x28", "0x29", "0x2a", "0x2b", "0x2c", "0x2d", "0x2e", "0x2f",  //2
  "0x30", "0x31", "0x32", "0x33", "0x34", "0x35", "0x36", "0x37", "0x38", "0x39", "0x3a", "0x3b", "0x3c", "0x3d", "0x3e", "0x3f",  //3
  "JNX0",  "JX0", "JNX1",  "JX1", "JNX2",  "JX2", "JNX3",  "JX3",  "JF0", "JNF0",  "JF1", "JNF1",  "JF2", "JNF2",  "JF3", "JNF3",  //4
  "JNME",  "JME", "JUMP", "0x53", "0x54", "0x55", "0x56", "0x57", "JNI0",  "JI0", "JNI1",  "JI1", "JNI2",  "JI2", "JNI3",  "JI3",  //5
  "CNX0",  "CX0", "CNX1",  "CX1", "CNX2",  "CX2", "CNX3",  "CX3",  "CF0", "CNF0",  "CF1", "CNF1",  "CF2", "CNF2",  "CF3", "CNF3",  //6
  "CNME",  "CME", "CALL", "0x73", "0x74", "0x75", "0x76", "0x77", "CNI0",  "CI0", "CNI1",  "CI1", "CNI2",  "CI2", "CNI3",  "CI3",  //7
   "RX0", "RNX0",  "RX1", "RNX1",  "RX2", "RNX2",  "RX3", "RNX3", "RNF0",  "RF0", "RNF1",  "RF1", "RNF2",  "RF2", "RNF3",  "RF3",  //8
   "RME", "RNME", "0x92",  "RET", "0x94", "0x95", "0x96", "RSCN",  "RI0", "RNI0",  "RI1", "RNI1",  "RI2", "RNI2",  "RI3", "RNI3",  //9
  "0xa0", "0xa1", "0xa2", "0xa3", "0xa4", "0xa5", "0xa6", "0xa7", "0xa8", "0xa9", "0xaa", "0xab", "0xac", "0xad", "0xae", "0xaf",  //A
  "0xb0", "0xb1", "0xb2", "0xb3", "0xb4", "0xb5", "0xb6", "0xb7", "0xb8", "0xb9", "0xba", "0xbb", "0xbc", "0xbd", "0xbe", "0xbf",  //B
  "0xc0", "0xc1", "0xc2", "0xc3", "0xc4", "0xc5", "0xc6", "0xc7", "0xc8", "0xc9", "0xca", "0xcb", "0xcc", "0xcd", "0xce", "0xcf",  //C
  "0xd0", "0xd1", "0xd2", "0xd3", "0xd4", "0xd5", "0xd6", "0xd7", "0xd8", "0xd9", "0xda", "0xdb", "0xdc", "0xdd", "0xde", "0xdf",  //D
  "0xe0", "0xe1", "0xe2", "0xe3", "0xe4", "0xe5", "0xe6", "0xe7", "0xe8", "0xe9", "0xea", "0xeb", "0xec", "0xed", "0xee", "0xef",  //E
  "0xf0", "0xf1", "0xf2", "0xf3", "0xf4", "0xf5", "0xf6", "0xf7", "0xf8", "0xf9", "0xfa", "0xfb", "0xfc", "0xfd", "0xfe", "0xff"   //F
};

T_VecMemDec DecodeVecMem(const uint8 *pRow, int offset)
{
    T_VecMemDec ret;
    TCHAR *buf = ret.c;
    buf[0] = buf[1] = buf[2] = buf[3] = ' ';

    if (offset >= 80)
        return ret;
    if (offset >= 16)
    {
        buf[1] = ABES_V_Formats[pRow[offset] >> 4];
        buf[2] = ABES_V_Formats[pRow[offset] & 15];
        return ret;
    }
    switch (offset)
    {
    case 0: { // opcode
        //return *(T_VecMemDec*)opcode_table[pRow[0]];
            const char *op = opcode_table[pRow[0]];
            buf[0] = op[0];
            buf[1] = op[1];
            buf[2] = op[2];
            buf[3] = op[3];
        }
        break;
    case 1: case 2: case 3: case 4: // operand bytes
    case 11: // Sync + CS
        my_itoa((uint32)pRow[offset], buf + 1, 16, 2);
        break;
    case 5: // MISR
        my_itoa((uint32)pRow[5] >> 6, buf + 1, 2, 2);
        break;
    case 6: // flags, timing goes in byte 7
        my_itoa((uint32)pRow[6] >> 4, buf, 2, 4);
        break;
    case 7: // timing set from byte 6
        buf[1] = (pRow[6] & 7) + '0';
        break;
    case 8: // FG resync
        buf[1] = (pRow[8] & 1) + '0';
        break;
    case 9: // FCS toggle bits from byte 10
        if (pRow[10] & 0x80) buf[0] = 'D';
        if (pRow[10] & 0x40) buf[1] = 'C';
        if (pRow[10] & 0x20) buf[2] = 'B';
        if (pRow[10] & 0x10) buf[3] = 'A';
        break;
    case 10: // FCS flags
        buf[1] = ((pRow[10] >> 2) & 3) + 'A';
        buf[2] = (pRow[10] & 3) + 'A';
        break;
    }
    return ret;
}

#endif // LC1VECMEM
