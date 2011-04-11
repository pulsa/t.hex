#pragma once

#ifndef _HEXWND_H_
#define _HEXWND_H_

#include "hexdoc.h"
#include "settings.h"
#include "clipboard.h"

class HexWndSettings;
//class FatInfo;
//#include "FatInfo.h"

#include "undo.h"
#include "utils.h"

enum {GBR_VFILL = 1, GBR_NOHSCROLL = 2, GBR_RSPACE = 4, GBR_LSPACE = 8};
enum {JV_SHORTEST = 0x00, JV_AUTO = 0x01, JV_CENTER = 0x02};
enum {JH_SHORTEST = 0x00, JH_AUTO = 0x10, JH_CENTER = 0x20};
enum {J_SHORTEST  = 0x00, J_AUTO  = 0x11, J_CENTER  = 0x22};

class DisplayPane
{
public:
    DisplayPane() : id(0) {}
    void Init(HexWnd *hw, int id, int iCharWidth, int extra = 1);
    void InitNumeric(int iCharWidth, int iBytes, bool bSigned, bool bFloat, int iBase, int extra = 1);
    void InitVecMem(int iCharWidth);
    void Position(int left, int pad);

    //wxString name;
    //int m_start, m_width;
    int m_left, m_start1, m_width1, m_width2;
    int m_iColChars;
    int m_iColBytes;
    int m_iCharWidth;
    int m_iColGrouping; // columns in a group, padded by more space
    int m_iGroupSpace;
    int m_iCols;
    int id;
    int m_iColWidth; // m_iColChars * m_iCharWidth + 2
    int m_extra; // extra pixel around columns for fixed-width fonts

    // for NUMERIC type
    bool m_bSigned;
    int m_iBase;
    bool m_bFloat;
    THSIZE m_iMax;

    enum {ADDRESS = 1, BIN, DEC, OCT, HEX, ANSI, ID_UNICODE, NUMERIC, VECMEM};
    bool CanEdit() { return id != ADDRESS; }
    int GetWidth(int cols);
    int GetX(int col);
    int GetRect(int col, int count, int &width, uint32 flags); // returns the X-coordinate
    int HitTest(int x, int &digit, int &half);
    #ifdef _WINDOWS
    virtual void SetFont(wxFont &newFont, wxDC &dc, TEXTMETRIC &tm);
    #else
    //! not yet implemented.
    #endif

    inline int GetRight() { return m_left + m_width2; }

    bool IsHex() const
    {
        return id == NUMERIC &&
            m_iBase == 16 &&
            m_bSigned == false;
    }

    inline bool IsAddress() const { return id == ADDRESS; }

    wxBrush m_hbrBack;

    static HexWndSettings *s_pSettings; //! this is awful
};

class UnicodePane : public DisplayPane
{
public:
    UnicodePane();
    //virtual void SetFont(wxFont &newFont, wxDC &dc, TEXTMETRIC &tm);
    //! not yet implemented
};

class FileMap;
class thFrame;
class DataView;
class DataSource;

typedef struct {
    //sequence *doc;
    HexDoc *doc;
    Selection sel;
    THSIZE firstLine;
} DOC_INFO;

class HexWnd : public wxWindow
{
public:
    HexWnd(); //! do we need this?
    HexWnd(thFrame *frame, HexWndSettings *ps = NULL);
    ~HexWnd();
    //void SetData(Hex

    thFrame *frame;
    DataSource *m_pDS;
    HexDoc *doc;
    bool SetDoc(HexDoc *pDoc);
    bool SetDataSource(DataSource *pDS);
    //FatInfo fatInfo;  //! not yet implemented

    //sequence *doc;
    std::vector<DOC_INFO> docinfo;
    std::vector<HexDoc*>  m_docs; // list of documents from current data source
    int curdoc;

    DisplayPane m_pane[10];

    void thScrollWindowRaw(int dx, int dy, bool bUpdate = false);
    void thScrollWindow(int dx, THSIZE oldLine, bool bUpdate = false);
    int OnSmoothScroll(int dx, int dy);
    void FinishSmoothScroll();
    int m_iYOffset, m_iLastYScroll; // used for sub-line scrolling

    bool OpenFile(LPCTSTR filename, bool bReadOnly);
    bool OpenDrive(LPCTSTR path, bool bReadOnly);
    bool OpenProcess(DWORD pid, wxString procName, bool bReadOnly);
    bool OpenBlank();
    bool OpenLC1VectorMemory(wxString addr);
    bool OpenTestFile(LPCTSTR filename);
    bool OpenDataSource(DataSource *pDS);

    bool ToggleReadOnly(); // Only available with some data sources.

    void UpdateSettings(HexWndSettings &ns);

    Selection m_highlight;
    DataView *m_dvHighlight; // which view window controls the highlighting?
    void Highlight(THSIZE start, THSIZE size, DataView *dv = NULL);
    void SetCurrentView(DataView *dv)
    {
        Highlight(0, 0, NULL);
        m_dvHighlight = dv;
    }

    void Hyperlink(THSIZE offset, THSIZE size, THSIZE target);
    THSIZE m_linkStart, m_linkSize, m_linkTarget;

    bool Ok() { return m_ok; }
    bool DataOk() const;
    wxString error() { return m_error; }

    void SetFont(wxFont &newFont); // Use this if you have a wxFont already.
    #ifdef _WINDOWS
    void SetFont(LOGFONT *plf);    // Use this if you don't.
    #endif

    TEXTMETRIC GetTextMetric() { return m_tm; }

    void GetSelection(uint64 &iSelStart, uint64 &iSelEnd)
    {
        if (m_iSelStart >= m_iCurByte)
            iSelStart = m_iCurByte, iSelEnd = m_iSelStart;
        else
            iSelStart = m_iSelStart, iSelEnd = m_iCurByte;
    }

    void GetSelection(Selection &sel)
    {
        sel.Set(m_iSelStart, m_iCurByte, m_iCurPane, m_iCurDigit);
    }
    Selection GetSelection()
    {
        Selection sel;
        GetSelection(sel);
        return sel;
    }
    Selection GetSelectionOrAll()
    {
        if (m_iCurByte == m_iSelStart)
            return Selection(0, DocSize(), m_iCurPane, m_iCurDigit);
        else
            return GetSelection();
    }
    void SetSelection(Selection &sel)
    {
        SetSelection(sel.nStart, sel.nEnd, sel.iRegion, sel.iDigit);
        Update();
    }

    uint64 GetCursor() { return m_iCurByte; }
    uint64 GetCursorVirtual();

    enum {RGN_RULER = -1, RGN_ADDRESS = 0, RGN_NONE = -2};  // I think stuff depends on having RGN_ADDRESS==0.

    int HitTest(const wxPoint &pt, uint64 &mouseByte, uint64 &mouseLine, int &digit, int &half);
    void SelectionHitTest(const wxPoint &pt, uint64 &mouseLine, int &col, int &digit, int &half);

    uint64 LineColToByte(uint64 line, int col);
    uint64 ByteToLineCol(uint64 byte, int *pCol); // returns byte in *pCol

    THSIZE LineColToByte2(THSIZE line, int col); // uses real column
    THSIZE ByteToLineCol2(THSIZE byte, int *pCol); // returns real column in *pCol

    int LineToY(THSIZE line);
    THSIZE YToLine(int y);
    int GetPaneLeft(const DisplayPane &pane);

    THSIZE GetFirstVisibleByte() { return LineColToByte(m_iFirstLine, 0); }
    THSIZE GetLastVisibleByte();
    THSIZE GetLastVisibleLine();
    bool IsCursorVisible();

    bool ScrollToLine(uint64 line, int x = -1); // should not be called except by ScrollToRange()
    //bool MakeLineVisible(uint64 line); // like ScrollToLine, but moves the shortest distance to get line on the screen
    //void ScrollToCursor() { MakeLineVisible(ByteToLineCol(m_iCurByte, NULL)); }
    void ScrollToCursor() { ScrollToRange(m_iCurByte, m_iCurByte, J_AUTO); }
    //bool ScrollToSelection(bool lazy = false) { return ScrollToRange(m_iSelStart, m_iCurByte, lazy); }
    //bool ScrollToSelection(int jumpiness = 1) { return ScrollToRange(m_iSelStart, m_iCurByte, jumpiness); }
    bool ScrollToRange(THSIZE start, THSIZE end, int jumpiness, int pane = -1);
    //bool CenterLine(THSIZE line);
    //bool CenterByte(THSIZE byte) { return CenterLine(ByteToLineCol(byte, NULL)); }
    bool CenterByte(THSIZE byte) { return ScrollToRange(byte, byte, 2); }

    void MoveCursorIntoView();

    bool IsReadOnly() { return doc ? doc->IsReadOnly() : true; }
    bool Goto(uint64 offset, bool bExtendSel = false, int origin = 0); // uses virtual address
    uint64 GetFileOffset(uint64 address);

    HexWndSettings s, &m_settings; //! m_settings is reference to s
    //HexDoc doc;
    //UndoManager undo;

    void OnDataChange(THSIZE nAddress, THSIZE nOldSize, THSIZE nNewSize); // called by HexDoc
    bool SetModified(bool modified);
    void SetSelection(uint64 iSelStart, uint64 iSelEnd, int region = 0, int digit = 0);
    void SetSelection(THSIZE iCursorPos) { SetSelection(iCursorPos, iCursorPos); }
    int FormatAddress(THSIZE address, TCHAR *buf, int bufsize);

    // wxFrameManager calls p.window->Refresh(true), which used to make our background get redrawn.
    // Override Refresh() here to prevent that.  Refresh(true) is never used for this window.
    virtual void Refresh() { wxWindow::Refresh(false, NULL); }
    virtual void Refresh( bool WXUNUSED(eraseBackground), const wxRect *rect = (const wxRect *) NULL )
    {
        wxWindow::Refresh(false, rect);
    }

    wxString GetTitle();
    wxString GetShortTitle();
    THSIZE DocSize() const { return doc ? doc->GetSize() : 0; }

    THSIZE GetLines() const;
    uint32 GetVisibleLines(int height = -1) const;
    THSIZE GetLastDisplayAddress() const { return m_iLastDisplayAddress; }

    void RemoveMP3Metadata(); // Heh.  This should be fun.
    void NextOddMp3Frame();  // Seek to the next frame whose size is wrong.  For finding metadata, errors, and...?

protected:
    void Init();
    void ClearDocs();
    uint64 m_iFirstLine;
    uint64 m_iTotalLines;
    uint64 m_iMaxScroll;
    uint32 m_iVisibleLines;
    
    int m_iScrollX, m_iScrollMaxX, m_iScrollPageX;
    
    uint64 m_iMouseOverByte, m_iMouseOverLine;
    int m_iMouseOverRegion, m_iMouseOverCount;
    bool m_bMousePosValid;
    THSIZE m_iMouseDownByte;
    int m_iMouseDownDigit, m_iMouseDownHalf;
    uint64 m_iCurByte;
    int m_iCurDigit;
    int m_iCurPane;
    int m_iPartialScroll;
    int m_iByteDigits;
    int m_iAddrPaneWidth;
    int m_iDesiredFontSize;
    uint32 m_iAddressOffset; // 0 <= m_iAddressOffset < s.iLineBytes
    THSIZE m_iLastDisplayAddress;

    int m_iLineBytes; // local copy of HexWndSettings.iLineBytes;
    uint8 *m_lineBuffer;
    uint8 *m_pModified;
    int m_iLineBufferSize;
    int m_iSpaceCount, *m_pCharSpaces; // used by CenterTextOut

    // used to make variable width font look more like monospace
    #ifdef _WINDOWS
    void CenterTextOut(HDC hdc, int x, int y, int flags, RECT *pRC,
                       LPCTSTR text, int charCount, int cellWidth);
    #endif
    //! I hate to slow down painting so much, but it's either that or the project.
    void CenterTextOut(wxDC &dc, int x, int y, wxString text, int cellWidth);

    bool m_ok;
    wxString m_error;

    wxBrush m_hbrWndBack, m_hbrAdrBack;
    wxPen m_penGrid;
    TEXTMETRIC m_tm;
    #ifdef _WINDOWS
    LOGFONT m_lf;  // Yeah.  It's a perfectly good name.
    #endif
    int m_iCodePage, m_iDefaultChar;
    bool m_bFontCharsOnly; // mostly a shadow of s.bFontCharsOnly
    bool m_bFixedWidthFont;

    uint64 m_iSelStart; // m_iCurByte is always end of selection
    bool m_bMouseSelecting;
    wxTimer m_scrollTimer;

    void OnPaint(wxPaintEvent &event);
    void OnErase(wxEraseEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnScroll(wxScrollWinEvent &event); // calls either VScroll or HScroll
    void OnLButtonDown(wxMouseEvent &event);
    void OnLButtonUp(wxMouseEvent &event);
    void OnCaptureChange(wxMouseCaptureChangedEvent &event); //! Windows only.  Fired even if we called ReleaseCapture().
#if wxCHECK_VERSION(2, 7, 0)
    void OnCaptureLost(wxMouseCaptureLostEvent &event); //! Windows only.  Fired only by external event.
#endif
    void OnMouseMove(wxMouseEvent &event);
    void OnMouseEnter(wxMouseEvent &event);
    void OnMouseLeave(wxMouseEvent &event);
    void OnMButtonDown(wxMouseEvent &event);
    void OnMouseWheel(wxMouseEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void OnKey(wxKeyEvent &event); //! temporary
    void OnKeyUp(wxKeyEvent &event); //! temporary?
    //void OnNavigationKey(wxKeyEvent &event); //! temporary
    void OnChar(wxKeyEvent &event);
    void OnSetFocus(wxFocusEvent &event);
    void OnKillFocus(wxFocusEvent &event);
    void OnScrollTimer(wxTimerEvent &event);

    // events not called directly by wxWidgets
    void OnVScroll(wxScrollWinEvent &event);
    void OnHScroll(wxScrollWinEvent &event);
    void OnSelectionMouseMove(uint64 mouseLine, int col, int half);
    void OnFileChange();

    wxRect PaintRect(wxDC &memDC, const wxRect &rcPaint);
    void PaintPane(wxDC &dc, int nPane, const THSIZE &firstLine, const THSIZE &lastLine, const wxRect &rcPaint);
    void PaintLine(uint64 line, wxDC &dc, int start_x, int width, int top = 0);
    void PaintPaneLine(wxDC &dc, uint64 line, int nPane, size_t offset, int byteCount, int byteStart, THSIZE address, int top = 0);
    void PaintRuler(wxDC &dc, const wxRect &rcPaint);
    int m_nPaintLines;
    DWORD panesPainted;  // bitmask of panes updated; used by PaintRuler().
    void PaintSelection(wxDC &dc);
    void PaintPaneSelection(wxDC &dc, DisplayPane &pane, THSIZE firstLine, THSIZE lastLine, int firstByte, int lastByte);
    void MyTextOut(const wxDC &dc, int x, int y, int etoFlags, RECT *rcText,
                   TCHAR *buf, int charCount, DisplayPane &pane, bool center);

    void RepaintBytes(uint64 start_byte, uint64 end_byte, bool bUpdateNow = true);
    void RepaintLines(uint64 start_line, uint64 end_line, bool bUpdateNow = true);

public:
    void set_caret_pos();
protected:
    void ShowCaret(int flags);
    void HideCaret(int flags);
    int m_caretHideFlags; // caret will be visible if this is zero
    enum {CHF_FOCUS = 1, CHF_AUTOPAN = 2, CHF_OFFSCREEN = 4, CHF_SELECTING = 8};

    THSIZE GetScrollPos64(int nPos32);
    void SetScrollbar64(int orientation, THSIZE nPos64, int nPage, THSIZE nMax64, bool refresh = true);
    void CalcLineWidth();
    void UpdateViews(int flags);
    friend class FileMap;

#ifdef _DEBUG
    bool bPrintPaintMessages;
    void PrintPaintMessages(bool b) { bPrintPaintMessages = b; }
    bool PrintPaintMessages() { return bPrintPaintMessages; }
#else
    void PrintPaintMessages(bool WXUNUSED(b)) { }
    bool PrintPaintMessages() { return false; }
#endif

    int GetByteX(int col, int region);
    bool GetByteRect(uint64 address, int count, int region, wxRect &rc); // sets rectangle in client coords
    void GetColRect(THSIZE line, int col, int count, int region, wxRect &rc, uint32 flags);

    void AdjustForNewDataSize();
    void ResizeScrollBars();
    bool GetLineInfo(uint64 line, uint64 &address, int &count, int& start);
    void ScrollPartialLine(int delta, int wheelDelta);
    int GetByteDigits(int mouseRegion = -1);
    //int GetCursorRegion() { return m_bHexCursor ? 4 : 6; }
    bool DigitGranularity();

    int m_iLineWidth, m_iLineHeight, m_iCharWidth, m_iDigitWidth;
    int m_iRulerHeight, m_iPaintWidth, m_iFirstLineTop;
    int m_iHexStart, m_iASCIIStart;
    //int GetTextWidth(int chars) { return m_tm.tmMaxCharWidth * chars + m_tm.tmOverhang; }
    
    uint16 iLeftLead[65536], iCharWidths[65536], width_hex, width_all;
    uint16 charMap256[256]; // code point for each 8-bit character
    #ifdef _WINDOWS
    // used by SetFont()
    UINT GetCharWidths(HDC hdc, UINT first, UINT last, bool bUseDrawText = false);

    wxString m_sFileName;
    HANDLE m_hChange;
    LARGE_INTEGER m_liFileSize;
    FILETIME m_ftLastWriteTime;
    static void CALLBACK s_FileChange(HANDLE UNUSED(handle), DWORD_PTR dwUser)
    {
        ((HexWnd*)dwUser)->OnFileChange();
    }
    #endif

    THSIZE NextBytePos(int dir, THSIZE current, int *pDigit = NULL);  // dir -1 = left, +1 = right

    wxArrayString m_asGotoCluster;

public:
    // Commands
    void CmdSetSelection(THSIZE iSelStart, THSIZE iSelEnd,
                         int region = 0, int digit = 0, int jumpiness = J_AUTO);
    void CmdExtendSelection(THSIZE iSelEnd, int region = 0);
    void CmdMoveCursor(uint64 address, BOOL bExtendSel = FALSE,
       int region = 0, int digit = 0, int jumpiness = J_AUTO);
    void CmdMoveRelative(int64 lines, int64 bytes, BOOL bExtendSel = FALSE);
    void CmdCursorLeft();
    void CmdCursorRight();
    void CmdCursorUp();
    void CmdCursorDown();
    void CmdCursorStartOfLine(wxCommandEvent &event);
    void CmdCursorEndOfLine(wxCommandEvent &event);
    void CmdCursorStartOfFile(wxCommandEvent &event);
    void CmdCursorEndOfFile(wxCommandEvent &event);
    void CmdCursorPageUp(bool bCtrlDown = false);
    void CmdCursorPageDown(bool bCtrlDown = false);
    void CmdViewLineUp(wxCommandEvent &event);
    void CmdViewLineDown(wxCommandEvent &event);
    void CmdViewPageUp(wxCommandEvent &event);
    void CmdViewPageDown(wxCommandEvent &event);
    void CmdViewTop(wxCommandEvent &event);
    void CmdViewCenter(wxCommandEvent &event);
    void CmdViewBottom(wxCommandEvent &event);
    void CmdGotoAgain(wxCommandEvent &event);
    //void CmdSave(wxCommandEvent &event);
    //void CmdFindAgainForward(wxCommandEvent &event);
    //void CmdFindAgainBackward(wxCommandEvent &event);
    void CmdNextBlock(wxCommandEvent &event);
    void CmdPrevBlock(wxCommandEvent &event);
    void CmdSelectAll(wxCommandEvent &event);
    void CmdCopy(wxCommandEvent &event);
    void CmdPaste(wxCommandEvent &event);
    void CmdPasteAs(wxCommandEvent &event);
    void CmdCut(wxCommandEvent &event);
    void CmdDelete(wxCommandEvent &event);
    void CmdBackspace(wxCommandEvent &event);
    void CmdSelectModeNormal();
    void CmdSelectModeBlock();
    void CmdSelectModeToggle();
    void CmdNextView(wxCommandEvent &event);
    void CmdPrevView(wxCommandEvent &event);
    void CmdIncreaseFontSize(wxCommandEvent &event);
    void CmdDecreaseFontSize(wxCommandEvent &event);
    void CmdRegionNext(wxCommandEvent &event);
    void CmdRegionPrev(wxCommandEvent &event);
    void CmdUndo(wxCommandEvent &event);
    void CmdRedo(wxCommandEvent &event);
    bool CmdChar(char c);
    void CmdUndoAll(wxCommandEvent &event);
    void CmdRedoAll(wxCommandEvent &event);
    void CmdToggleEndianMode(wxCommandEvent &event);
    void CmdRelativeAddresses(wxCommandEvent &event);
    void CmdAbsoluteAddresses(wxCommandEvent &event);
    void CmdCopyAddress(wxCommandEvent &event);
    void CmdCopyCurrentAddress(wxCommandEvent &event);
    void CmdViewAdjustColumns(wxCommandEvent &event);
    void CmdOffsetLeft(wxCommandEvent &event);
    void CmdOffsetRight(wxCommandEvent &event);
    //void CmdNewFile(wxCommandEvent &event);
    void CmdViewRuler(wxCommandEvent &event);
    void CmdViewStickyAddr(wxCommandEvent &event);
    void CmdReadPalette(wxCommandEvent &event);
    //void CmdFocusHexWnd(wxCommandEvent &event);
    void CmdToggleHexDec(wxCommandEvent &event);
    void CmdViewPrevRegion(wxCommandEvent &event);
    void CmdViewNextRegion(wxCommandEvent &event);

    void CmdGotoCluster(wxCommandEvent &event);
    void CmdJumpToFromFat(wxCommandEvent &event);
    void CmdFirstCluster(wxCommandEvent &event);
    void CmdLastCluster(wxCommandEvent &event);
    void CmdGotoPath(wxCommandEvent &event);
    void CmdFatAutoSave(wxCommandEvent &event);
    void CmdCopyFontCodePoints(wxCommandEvent &WXUNUSED(event));
    void CmdFindDiffRec(wxCommandEvent &WXUNUSED(event));
    void CmdFindDiffRecBack(wxCommandEvent &WXUNUSED(event));
    void CmdCopySector(wxCommandEvent &event);
    void CmdCustomOp1(wxCommandEvent &event);

    bool DoFind(const uint8 *data, size_t searchLength, int flags = 0);
    void DoCopy(thCopyFormat fmt);
    bool DoPaste(thString data, SerialData *psData = NULL);
    void PasteWithFormat(int userFormat = -1);
    thString FormatPaste(const char* pData, size_t newSize, UINT srcFormat, int dstFormat);

    DECLARE_EVENT_TABLE()

public:
    //! Horrible.  Set and used only by DocList.
    int DocListSortOrder[4], DocListSortColumn;
};

//! Accelerators using the Shift key leave bit 0 set.  Damn annoying.
#ifdef _WINDOWS
inline bool ShiftDown() { return !!(GetKeyState(VK_SHIFT) >> 15); }
#else
//! TBDL (To Be Done in Linux)
inline bool ShiftDown() { return false; }
#endif

#endif // _HEXWND_H_
