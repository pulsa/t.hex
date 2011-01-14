/******************************************************************************
 *
 *   mccLogWindow2
 *   Adam Bailey      7/21/2005
 *
 *   This class displays lines of text as a message log, with an option to show
 *   only certain lines.
 *
 *   It was written to allow this selective display and solve the scrolling
 *   problems we experienced with wxTextCtrl using a RichEdit control.  It is
 *   also very fast and memory-efficient, and supports an unlimited number of
 *   lines with no noticeable slowdown.
 *
 *   To the user it behaves like a read-only text box, with normal scrolling
 *   and selection.  However it may be helpful for the programmer to think of
 *   it as a list box, since text is appended one line at a time, and there is
 *   no line wrapping.  I have tried to make its interface look as much as
 *   possible like wxTextCtrl regarding cursor movement and text retrieval.
 *   Maximum line length is 32767 bytes.  Longer lines will be truncated.
 *
 *   Possible future improvements include line wrapping, multiple fonts, and
 *   multiple text styles within a line (probably using info stored after the
 *   line of text).
 *
 *****************************************************************************/

#ifndef _MLOGWIN2_HPP_
#define _MLOGWIN2_HPP_

#include "mdefs.h"
#include <wx/fdrepdlg.h> // find/replace dialog

#define MLW_MOREINFOBYTES  0x80
#define MLW_SHOWALWAYS     0x40
#define MLW_STYLEMASK      0x3F

class MCCLIBEXPORT LineStore
{
public:
   LineStore();
   ~LineStore() { Clear(); }

   UINT AddLine(LPCSTR text, UINT len = 0, UCHAR info = 0);
   LPCSTR GetLine(int line, UINT *pLen = NULL);
   void AppendText(LPCSTR text, UINT len = 0, UCHAR info = 0);

   void Clear();
   long XYToPosition(long x, long y);
   bool PositionToXY(long pos, long *x, long *y);
   bool RemoveLine(int line);
   UINT GetNumberOfLines();

private:

   void CreateNew();

   class LineMem
   {
   public:
      LineMem(LineMem *prev);

      // Line numbers are passed in relative to the beginning of the block.
      UINT AddLine(LPCSTR text, UINT len, UCHAR info);
      LPCSTR GetLine(int line, UINT *pLen); // pLen may be null
      void AppendText(LPCSTR text, UINT len, UCHAR info);
      bool PositionToXY(long pos, long *x, long *y); // output line number is adjusted by block offset
      long XYToPosition(long x, long y);
      void RemoveLine(int line);
      UINT CountInfoBytes(const UCHAR *offset, UINT *pLastInfoStart = NULL);

      inline UINT SizeLeft()
      {
         return MEM_SIZE   // size of memory block
            - nUsedSize    // size of string data and info bytes
            - 4 * nLines;  // line offsets and lengths
      }

      inline bool HasRoomFor(int len)
      {
         return (int)SizeLeft() >= len + 4 + nInfoBytes;
      }

      int nUsedSize;
      int nLines;
      int nFirstLine;
      int nFirstChar;
      int nTotalChars;
      static const int nInfoBytes; // data bytes stored after each line of text

      enum { MEM_SIZE = 0x10000 };
      char data[MEM_SIZE];

      LineMem *pPrev, *pNext;
   };

   LineMem *pFirst;
   LineMem *pLast;
   LineMem *pCurrent;
   UINT totalLines;
};

enum {
   IDLW_COPY     = 200,
   IDLW_COPYALL  = 201,
   IDLW_PRINTALL = 202,
   IDLW_PRINTSEL = 203,
   IDLW_FILTER   = 204,
};

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(MCCLIBEXPORT, wxEVT_TEXT_SELECTED, 0) // value is not used in new macro
    //DECLARE_EXPORTED_EVENT_TYPE(MCCLIBEXPORT, wxEVT_TEXT_DBL_CLICK, 1)
END_DECLARE_EVENT_TYPES()

#define EVT_TEXT_SELECTED(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_TEXT_SELECTED, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) & fn, (wxObject *) NULL ),
//#define EVT_TEXT_DBL_CLICK(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_TEXT_DBL_CLICK, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) & fn, (wxObject *) NULL ),

#define MLW_DEFAULT        0
#define MLW_LINE_BREAK     1
#define MLW_LINE_NOBREAK   2

WX_DECLARE_USER_EXPORTED_OBJARRAY(wxTextAttr, wxArrayTextAttr, MCCLIBEXPORT);

class MCCLIBEXPORT mccLogWindow2 : public wxControl
{
public:
   mccLogWindow2(wxWindow *parent, wxWindowID id,
             const wxString& value = wxEmptyString,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = 0,
             const wxValidator& validator = wxDefaultValidator,
             const wxString& name = wxControlNameStr);

   virtual ~mccLogWindow2();

   bool Create(wxWindow *parent, wxWindowID id,
           const wxPoint& pos = wxDefaultPosition,
           const wxSize& size = wxDefaultSize,
           long style = 0,
           const wxValidator& validator = wxDefaultValidator,
           const wxString& name = wxControlNameStr);

   // The lineControl parameter to AppendText() gives the caller the option to
   // override its newline detection.  If the string you are appending does not
   // have '\n' and it should, set lineControl = MLW_LINE_BREAK.  If it does
   // and it shouldn't, set lineControl = MLW_LINE_NOBREAK.  Otherwise leave it
   // set to MLW_DEFAULT, or zero.
   // MLW_SHOWALWAYS may also be used here.
   virtual void AppendText(LPCSTR ptext, int lineControl = MLW_DEFAULT);

   virtual bool RemoveLine(int line);
   virtual void replaceLastLine(LPCSTR ptext);
   virtual void SetInsertionPointEnd() { MoveCursor(m_nLines - 1, -1); }
   virtual int SetDefaultStyle(const wxTextAttr& style);
   virtual void Clear(); // delete all text
   virtual void Copy();  // copy text to clipboard
   virtual void CopyAll(); // copy all text to clipboard
   virtual void ShowPrintDlg(bool bSelOnly);

   // set/retrieve the font for the window (SetFont() returns true if the
   // font really changed)
   virtual bool SetFont(const wxFont& font);

   UINT GetNumberOfLines() { return m_nLines; }

   virtual inline LPCSTR GetLine(int line, UINT *pLen = NULL)
   {
      return ls.GetLine(GetStoredLineNumber(line), pLen);
   }

   wxString GetLineText(int lineNo)
   {
      UINT len;
      LPCSTR text = GetLine(lineNo, &len);
      return wxString(text, len);
   }

   int GetLineLength(long lineNo)
   {
      UINT len;
      GetLine(lineNo, &len);
      return (int)len;
   }

   wxString GetRange(long from, long to);

   long XYToPosition(long x, long y);
   bool PositionToXY(long pos, long *x, long *y);

   long GetInsertionPoint() { return XYToPosition(m_cursor.x, m_cursor.y); }
   void GetInsertionPoint(wxPoint *point) { *point = m_cursor; }
   long GetLastPosition();

   virtual void GetSelection(long *from, long *to)
   {
      *from = XYToPosition(m_selStart.x, m_selStart.y);
      *to = XYToPosition(m_cursor.x, m_cursor.y);
   }

   bool HasSelection() { return m_cursor != m_selStart; }
   wxString GetSelectionText()
   {
      long from, to;
      GetSelection(&from, &to);
      return GetRange(from, to);
   }

   BOOL MouseToLineCol(const wxPoint &pos, int &line, int &col);
   int MoveCursor(int line, int col, bool bExtendSel = false);
   bool IsLineVisible(int line) { return (line >= yPos && line < yPos + yClient / yChar); }
   void ScrollTo(int new_xPos, int new_yPos); // ScrollTo() enforces valid positions

   virtual int SetErrorStyle(const wxTextAttr &style);
   virtual int Filter(bool ErrorsOnly);
   virtual bool IsFiltered() { return m_bFiltered; }

   enum ScrollStyle { SCROLL_ALWAYS, SCROLL_NEVER, SCROLL_AUTO };
   void SetScrollStyle(enum ScrollStyle style) { m_scrollStyle = style; }
   enum ScrollStyle GetScrollStyle() { return m_scrollStyle; }

   bool FindText(LPCSTR search, wxPoint *result, int start = 0, int flags = 0);
	bool DisplayFirstError();
	bool DisplayLastError();
	bool DisplayPreviousError();
	bool DisplayNextError();

   wxMenu *contextMenu; // give apps access to context menu

   DECLARE_EVENT_TABLE()
protected:

   void OnMouseDown(wxMouseEvent &event);
   void OnMouseUp(wxMouseEvent &event);
   void OnMouseMove(wxMouseEvent &event);
   void OnMouseWheel(wxMouseEvent &event);
   void OnKeyDown(wxKeyEvent &event);
   void OnKeyUp(wxKeyEvent &event); // same as OnMouseUp(), with different event
   void OnDoubleClick(wxMouseEvent &event);
   void OnRightClick(wxMouseEvent &event);
   void OnMenuCmd(wxCommandEvent &event);

   void OnPaint(wxPaintEvent &event);
   void OnSize(wxSizeEvent &event);

   void OnSetFocus(wxFocusEvent &event);
   void OnLoseFocus(wxFocusEvent &event);

   void OnFind(wxFindDialogEvent &event);       // Find (or Find Next) button was pressed in the dialog.
   void OnFindClose(wxFindDialogEvent &event);  // The dialog is being destroyed, any pointers to it cannot be used any longer.

   void DrawLinePart(wxDC &dc, int nStyle, LPCSTR text, int len, RECT &rc);
   void Redraw(wxDC &dc, int rcPaint_top, int rcPaint_bottom);
   virtual void Redraw();          // creates a wxClientDC

   virtual void AdjustScrollbars(int orientFlags);
   virtual int AdjustScrollPos(int event, int bar); // set and return new scrollbar position according to event
   virtual void CalcFontSizeInfo();
   virtual bool ShowFindDialog();

   int XYtoLineCol(int x, int y, int &col); // returns line

   long MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam); // intercept WM_GETDLGCODE, like wxTextCtrl does
   // scroll event (both horizontal and vertical)
   virtual bool MSWOnScroll(int orientation, WXWORD wParam,
                            WXWORD pos, WXHWND control);

   virtual bool HasFocus() { return ::GetFocus() == m_hWnd; }
   bool IsCursorAtEnd() { return m_bCursorAtEnd; }
   wxPoint m_cursor;    // use MoveCursor() to change this
   wxPoint m_selStart;  // use MoveCursor() to change this
   bool m_bCursorAtEnd; // use MoveCursor() to change this

   wxFindReplaceDialog *m_findDlg;
   wxFindReplaceData m_findData;
   virtual bool DoFind();

   int m_nLines;
   bool m_bStartNewLine;
	int m_nCurrentErrLine;

   LineStore ls;
   //ATimer timer;
   wxPoint m_caretPos; // window coordinates of caret

   virtual int GetStoredLineNumber(int line)
   {
      if (line < 0)
         return -1;
      else if (line >= m_nLines)
         return m_nLines - 1;
      else if (m_bFiltered)
         return filterLines[line];
      else
         return line;
   }

   virtual int GetDisplayedLineNumber(int storedLineNo);

   //wxArrayInt m_colors;
   wxArrayTextAttr m_styles;
   int m_defaultStyle;
   int m_errorStyle;
   int m_selectedStyle;       // currently selected into DC
   bool m_bFiltered;
   int *filterLines;
   enum ScrollStyle m_scrollStyle;

   // MS sample stuff
   // These variables are required to display text.
   int xClient;     // width of client area
   int yClient;     // height of client area
   int xClientMax;  // maximum width of client area

   int xChar;       // horizontal scrolling unit
   int yChar;       // vertical scrolling unit
   int xUpper;      // average width of uppercase letters

   int xPos;        // current horizontal scrolling position
   int yPos;        // current vertical scrolling position

   int m_old_xPos;  // last updated position of horizontal scrollbar
   int m_old_yPos;  // last updated position of vertical scrollbar

public: // debugging stuff

   int m_nPaintCount;
   static wxTextCtrl *s_txtDebug;
   void debug(LPCSTR fmt, ...);
};

/*
typedef struct {
   mccLogWindow2 *pLogWindow;
   UINT lineno, len;
   LPCSTR text;
   void *pUserData;
} LW2_FILTERINFO;

bool FilterLineProc(LW2_FILTERINFO *pfi)
{
   UCHAR *pInfo = (UCHAR*)pfi->text + pfi->len;

   if ((*pInfo & 0x3F) == (int)pUserData)
      return true;
   else
      return false;
}
*/

#endif // _MLOGWIN2_HPP_
