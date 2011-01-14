#include "precomp.h"
#include "mlogwin2.hpp"
#include <wx/wx.h>
#include "wx/msw/private.h"
#include <wx/caret.h>
#include <wx/fdrepdlg.h> // find/replace dialog
#include <wx/clipbrd.h>
#include <wx/textfile.h>
#include <wx/arrimpl.cpp>
#include <wx/datetime.h>

//#include "mccprint.hpp"

#include "mydebug.h"

/* The way this ended up, all three classes need to know how the info bytes are
   stored.  They go after the text in each line.  If the most significant bit
   of an info byte is set, it is followed by a 16-bit, little-endian word
   containing the number of characters is applies to, and then another info
   byte.  If that bit is clear, the info byte applies to the rest of the
   characters on the line.  So in most cases, there is only one info byte.
   Bit 6 of the first info byte means this line is always displayed when other
   lines are filtered.  I realize now that there may have been a better way to
   do this, but it works so I'll leave it alone.
   Adam Bailey  10/14/05
*/

const int LineStore::LineMem::nInfoBytes = 1; // data bytes stored after each line of text

LineStore::LineStore()
{
   pFirst = pLast = pCurrent = NULL;
   totalLines = 0;
}

void LineStore::CreateNew()
{
   LineMem *pMem = new LineMem(pLast);
   if (pLast)
      pLast->pNext = pMem;
   pLast = pMem;
   if (!pFirst)
      pFirst = pMem;
}

void LineStore::Clear()
{
   pCurrent = pFirst;
   while (pCurrent)
   {
      LineMem *pTmp = pCurrent;
      pCurrent = pCurrent->pNext;
      delete pTmp;
   }
   pFirst = pLast = pCurrent = NULL;
   totalLines = 0;
}

long LineStore::XYToPosition(long x, long y)
{
   if (!pFirst)
      return 0;

   UINT len;
   LPCSTR text = GetLine(y, &len);

   if (pCurrent && text)
      return pCurrent->XYToPosition(x, y - pCurrent->nFirstLine);
   else // past last line
      return pLast->nFirstChar + pLast->nTotalChars;
}

bool LineStore::PositionToXY(long pos, long *x, long *y)
{
   // Look through blocks of memory for the requested position.
   //! to do: smarter search
   for(pCurrent = pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext)
   {
      if (pos >= pCurrent->nFirstChar &&
          pos <= pCurrent->nFirstChar + pCurrent->nTotalChars + pCurrent->nLines)
         return pCurrent->PositionToXY(pos, x, y);
   }
   *x = *y = 0;
   return false;
}

UINT LineStore::GetNumberOfLines()
{
   UINT count = 0;

   for(pCurrent = pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext)
   {
      count += pCurrent->nLines;
   }
   return count;
}

LineStore::LineMem::LineMem(LineMem *prev)
{
   nLines = 0;
   nUsedSize = 0;
   nFirstLine = 0;
   nFirstChar = 0;
   nTotalChars = 0;

   if (prev)
   {
      nFirstLine = prev->nFirstLine + prev->nLines;
      nFirstChar = prev->nFirstChar + prev->nTotalChars;
   }

   UINT *pOffsets = (UINT*)(data + MEM_SIZE) - 1;
   pOffsets[0] = 0;

   memset(data, 0, sizeof(data)); // This isn't necessary, but helps debugging.

   this->pPrev = prev;
   this->pNext = NULL;
}

UINT LineStore::AddLine(LPCSTR text, UINT len /*= 0*/, UCHAR info /*= 0*/)
{
   if (len == 0)
      len = (UINT)strlen(text);
   if (len > 0x7FFF)
      len = 0x7FFF;

   if (!pLast || !pLast->HasRoomFor(len))
      CreateNew();

   int count = pLast->AddLine(text, len, info);
   totalLines += count;
   return count;
}

LPCSTR LineStore::GetLine(int line, UINT *pLen /*= NULL*/)
{
   // Look through blocks of memory for the requested line.
   if (pCurrent &&
       line >= pCurrent->nFirstLine &&
       line <  pCurrent->nFirstLine + pCurrent->nLines)
   {
      // If the line is in this block, we're all set.  Just fall through.
   }
   else if (pLast != pCurrent &&
            line >= pLast->nFirstLine &&
            line <  pLast->nFirstLine + pLast->nLines)
   {
      pCurrent = pLast;
   }
   else
   {
      //! to do: smarter search
      for(pCurrent = pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext)
      {
         if (line >= pCurrent->nFirstLine &&
             line <  pCurrent->nFirstLine + pCurrent->nLines)
            break;
      }
   }

   if (pCurrent)
      return pCurrent->GetLine(line - pCurrent->nFirstLine, pLen);
   else
   {
      if (pLen)
         *pLen = 0;
      return "";
   }
}

void LineStore::AppendText(LPCSTR text, UINT len /*= 0*/, UCHAR info /*= 0*/)
{
   if (len == 0)
      len = (UINT)strlen(text);
   if (len > 0x7FFF)
      len = 0x7FFF;

   if (!pLast || !pLast->HasRoomFor(len))
   {
      if (pLast)
      {
         LineMem *pOldLast = pLast;
         UINT oldlen;
         LPCSTR oldtext = pLast->GetLine(pLast->nLines - 1, &oldlen);
         UINT cbInfo = pLast->CountInfoBytes((UCHAR*)oldtext + oldlen);
         CreateNew();
         pLast->AddLine(oldtext, oldlen, oldtext[oldlen]);
         if (cbInfo > 1)
            memmove(pLast->data + oldlen, oldtext + oldlen, cbInfo);
         RemoveLine(pOldLast->nFirstLine + pOldLast->nLines - 1);
      }
      else
      {
         CreateNew();
         pLast->AddLine("", 0, info); // add a dummy line for LineMem::AppendText()
      }
   }
   pLast->AppendText(text, len, info);
}

bool LineStore::RemoveLine(int line)
{
   UINT len;
   GetLine(line, &len);
   if (!pCurrent)
      return false;
   pCurrent->RemoveLine(line - pCurrent->nFirstLine);

   for (LineMem *pNext = pCurrent->pNext; pNext; pNext = pNext->pNext)
   {
      pNext->nFirstLine--;
      pNext->nFirstChar -= len;
   }
   return true;
}

UINT LineStore::LineMem::AddLine(LPCSTR text, UINT len, UCHAR info)
{
   memcpy(data + nUsedSize, text, len);
   data[nUsedSize + len] = info;

   UINT *pOffsets = (UINT*)(data + MEM_SIZE) - 1;
   pOffsets[-nLines] = MAKELONG(nUsedSize, len); // LOWORD = offset, HIWORD = length
   nTotalChars += len;
   nUsedSize += len + 1;
   nLines++;
   return 1;
}

LPCSTR LineStore::LineMem::GetLine(int line, UINT *pLen /*= NULL*/)
{
   UINT *pOffsets = (UINT*)(data + MEM_SIZE) - 1;
   int len = HIWORD(pOffsets[-line]);
   LPCSTR text = data + LOWORD(pOffsets[-line]);

   if (pLen)
      *pLen = len;

   return text;
}

void LineStore::LineMem::AppendText(LPCSTR text, UINT len, UCHAR info)
{
   // This function requires that a line exists, even if it is empty.
   UINT *pOffsets = (UINT*)(data + MEM_SIZE) - 1;
   int line = this->nLines - 1;
   UINT linelen = HIWORD(pOffsets[-line]);
   int offset = LOWORD(pOffsets[-line]);

   UINT lastInfoStart;
   UINT cbInfo = CountInfoBytes((UCHAR*)data + offset + linelen, &lastInfoStart);

   len = wxMin(len, 0x7FFF - linelen);
   memmove(data + offset + linelen + len, data + offset + linelen, cbInfo);
   memmove(data + offset + linelen, text, len);
   UCHAR *oldinfo = (UCHAR*)data + offset + linelen + len + cbInfo - 1;
   if (info != *oldinfo)
   {
      *oldinfo |= MLW_MOREINFOBYTES; // set continuation flag
      *(USHORT*)(oldinfo + 1) = linelen - lastInfoStart; // write length of old last section
      oldinfo[3] = info; // store new info byte
      cbInfo += 3;
   }
   linelen += len;
   pOffsets[-line] = MAKELONG(offset, linelen);
   nTotalChars += len;
   nUsedSize += len + cbInfo;
}

UINT LineStore::LineMem::CountInfoBytes(const UCHAR *offset, UINT *pLastInfoStart /*= NULL*/)
{
   UINT count = 0;
   UINT start = 0;
   while (offset[count] & MLW_MOREINFOBYTES)
   {
      start += *(USHORT*)(offset + count + 1);
      count += 3;
   }
   if (pLastInfoStart)
      *pLastInfoStart = start;
   return count + 1;
}

bool LineStore::LineMem::PositionToXY(long pos, long *x, long *y)
{
   UINT *pOffsets = (UINT*)(data + MEM_SIZE) - 1;

   int tmp = this->nFirstChar;
   for (int i = 0; i < this->nLines; i++)
   {
      int len = HIWORD(pOffsets[-i]) + 1; // include virtual LF
      if (tmp + len > pos)
      {
         *x = pos - tmp;
         *y = this->nFirstLine + i;
         return true;
      }
      tmp += len;
   }
   return false; // LineStore::PositionToXY() says this should never happen
}

long LineStore::LineMem::XYToPosition(long x, long y)
{
   UINT *pOffsets = (UINT*)(data + MEM_SIZE) - 1;
   UINT len = HIWORD(pOffsets[-y]), offset = 0;

   for (int i = 0; i < y; i++)
      offset += HIWORD(pOffsets[-i]);

   return offset                 // offset from beginning of block
        + y                      // 1 LF char per line in this block
        + nFirstChar             // sum of all previous blocks
        + wxMin(x, (int)len);    // column inside line
}

void LineStore::LineMem::RemoveLine(int line)
{
   UINT *pOffsets = (UINT*)(data + MEM_SIZE) - 1;
   int len = HIWORD(pOffsets[-line]);
   LPSTR text = data + LOWORD(pOffsets[-line]);

   if (line < this->nLines - 1)
   {
      UINT nextOffset = LOWORD(pOffsets[-(line+1)]);
      memmove(text, data + nextOffset, this->nUsedSize - nextOffset);
      memmove(pOffsets - nLines + 1, pOffsets - nLines + 2, nLines - line);
   }

   this->nLines--;
   this->nTotalChars -= len;
}

wxTextCtrl *mccLogWindow2::s_txtDebug = NULL;

WX_DEFINE_OBJARRAY(wxArrayTextAttr);

DEFINE_EVENT_TYPE(wxEVT_TEXT_SELECTED)
//DEFINE_EVENT_TYPE(wxEVT_TEXT_DBL_CLICK)

BEGIN_EVENT_TABLE(mccLogWindow2, wxControl)
   EVT_LEFT_DOWN(OnMouseDown)
   EVT_LEFT_UP(OnMouseUp)
   EVT_MOTION(OnMouseMove)
   EVT_MOUSEWHEEL(OnMouseWheel)
   //EVT_MOUSE_CAPTURE_CHANGED(OnCaptureChange)
   //EVT_LEFT_DCLICK(OnDoubleClick)

   EVT_RIGHT_UP(OnRightClick)
   EVT_MENU(-1, OnMenuCmd) // handle all menu events

   EVT_KEY_DOWN(OnKeyDown)
   EVT_KEY_UP(OnKeyUp)

   EVT_PAINT(OnPaint)
   EVT_SIZE(OnSize)

   EVT_SET_FOCUS(OnSetFocus)
   EVT_KILL_FOCUS(OnLoseFocus)

   EVT_FIND(-1, OnFind)
   EVT_FIND_NEXT(-1, OnFind)
   EVT_FIND_CLOSE(-1, OnFindClose)

END_EVENT_TABLE()

mccLogWindow2::mccLogWindow2(wxWindow *parent, wxWindowID id,
             const wxString& value,
             const wxPoint& pos,
             const wxSize& size,
             long style,
             const wxValidator& validator,
             const wxString& name)
: wxControl(parent, id, pos, size,
            style | wxVSCROLL | wxHSCROLL | wxWANTS_CHARS,
            validator, name)
{
   m_nPaintCount = 0;
   m_defaultStyle = 0;
   m_errorStyle = 0;
   m_bCursorAtEnd = true;
   m_bFiltered = false;
   filterLines = NULL;
   m_findData.SetFlags(wxFR_DOWN);
   m_findDlg = NULL;
   m_nLines = 0;
   xChar = yChar = 1;
   xClient = yClient = 1;
   xPos = yPos = 0;
   m_old_xPos = m_old_yPos = 0;
   m_cursor.x = m_cursor.y = 0;
   m_scrollStyle = SCROLL_AUTO;
   m_bStartNewLine = true;
	m_nCurrentErrLine = 0;

   //Create(parent, id, pos, size,
   //       style | wxVSCROLL | wxHSCROLL | wxWANTS_CHARS,
   //       validator, name);

#if wxCHECK_VERSION(2, 6, 0)
   // Documentation incorrectly states that wxBG_STYLE_CUSTOM has no effect on Win32.
   SetBackgroundStyle(wxBG_STYLE_CUSTOM); // We draw our own background.
#endif
   SetBackgroundColour(GetSysColor(COLOR_WINDOW));

   // Add text styles for normal and selected text.
   m_styles.Add(wxTextAttr());
   m_styles.Add(wxTextAttr(GetSysColor(COLOR_HIGHLIGHTTEXT), GetSysColor(COLOR_HIGHLIGHT)));

   SetCaret(new wxCaret(this, 2, 16)); // size is changed in CalcFontSizeInfo()
   SetFont(*wxNORMAL_FONT);
   CalcFontSizeInfo();
   m_caret->Show();
   SetInitialSize(size);

   AppendText(value);

   contextMenu = new wxMenu();
   contextMenu->Append(IDLW_COPY,     "&Copy");
   contextMenu->Append(IDLW_COPYALL,  "Copy &all");
   contextMenu->AppendCheckItem(IDLW_FILTER,   "&Show errors only");
   contextMenu->AppendSeparator();
   contextMenu->Append(IDLW_PRINTSEL, "Print selected &lines...");
   contextMenu->Append(IDLW_PRINTALL, "&Print...");

}

mccLogWindow2::~mccLogWindow2()
{
   delete contextMenu;
}

#if 0
bool mccLogWindow2::Create(wxWindow *parent, wxWindowID id,
                       const wxPoint& pos, const wxSize& size, long style,
                       const wxValidator& validator, const wxString& name)
{
   // Create() and MSWCreate() are not virtual, and I couldn't find a
   // straight-forward way to register and use a custom window class.
   // So I did this instead.
   // We register a window class, then hijack the pointer to the name of
   // the class used by wxWindow::MSWCreate().

   extern WXDLLIMPEXP_CORE const wxChar *wxCanvasClassName;
   extern LRESULT WXDLLEXPORT APIENTRY _EXPORT
      wxWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

   const wxChar *tmpClassName = wxCanvasClassName;

   bool bRegisterClassTried = false;
   bool bClassRegistered = false;

   if (!bRegisterClassTried)
   {
      bRegisterClassTried = true;

      WinStruct<WNDCLASSEX> wcex;
      wcex.lpszClassName = "LogWndClassNameNR";
      wcex.lpfnWndProc   = (WNDPROC)wxWndProc;
      wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); //! use system color
      wcex.hInstance     = wxhInstance;
      wcex.style         = CS_DBLCLKS;

      if (RegisterClassEx(&wcex))
      {
         bClassRegistered = true;
      }
      else
      {
         int err = GetLastError();
      }
   }

   if (bClassRegistered)
      wxCanvasClassName = "LogWndClassName";

   bool rc = wxControl::Create(parent, id, pos, size, style, validator, name);
   wxCanvasClassName = tmpClassName;
   return rc;
}
#endif

WXLRESULT mccLogWindow2::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    WXLRESULT lRc = wxControl::MSWWindowProc(nMsg, wParam, lParam);

    if ( nMsg == WM_GETDLGCODE )
    {
        // we always want the chars and the arrows: the arrows for navigation
        // and the chars because we want Ctrl-C to work even in a read only
        // control
        lRc = DLGC_WANTCHARS | DLGC_WANTARROWS;
    }

    return lRc;
}

void mccLogWindow2::OnMouseDown(wxMouseEvent &event)
{
   SetFocus();
   int line, col;
   MouseToLineCol(event.GetPosition(), line, col);
   MoveCursor(line, col, event.ShiftDown());

   CaptureMouse();
}

void mccLogWindow2::OnMouseMove(wxMouseEvent &event)
{
   // We could just call HasCapture() here, but this is faster.
   if (::GetCapture() != (HWND)m_hWnd)
      return;

   int line, col;
   MouseToLineCol(event.GetPosition(), line, col);
   MoveCursor(line, col, true);
}

void mccLogWindow2::OnMouseUp(wxMouseEvent &event)
{
   if (HasCapture())
      ReleaseMouse();
   wxCommandEvent cmd(wxEVT_TEXT_SELECTED, m_windowId);
   InitCommandEvent(cmd);
   GetParent()->GetEventHandler()->ProcessEvent(cmd);
   event.Skip();
}

/*void mccLogWindow2::OnDoubleClick(wxMouseEvent &event)
{
   wxCommandEvent cmd(wxEVT_TEXT_DBL_CLICK, m_windowId);
   InitCommandEvent(cmd);
   GetParent()->GetEventHandler()->ProcessEvent(cmd);
   event.Skip();
}*/

void mccLogWindow2::OnRightClick(wxMouseEvent &event)
{
   contextMenu->Enable(IDLW_COPY, HasSelection());
   //contextMenu->Enable(IDLW_PRINTSEL, HasSelection());
   contextMenu->Enable(IDLW_PRINTSEL, false); // temporarily disable until printing works
   contextMenu->Enable(IDLW_PRINTALL, false); // temporarily disable until printing works
   contextMenu->Check(IDLW_FILTER, IsFiltered());

   wxWindow::PopupMenu(contextMenu, event.GetPosition());
}

void mccLogWindow2::OnMenuCmd(wxCommandEvent &event)
{
   switch (event.GetId())
   {
   case IDLW_COPY:
      Copy();
      break;
   case IDLW_COPYALL:
      CopyAll();
      break;
   case IDLW_PRINTSEL:
      ShowPrintDlg(true);
      break;
   case IDLW_PRINTALL:
      ShowPrintDlg(false);
      break;
   case IDLW_FILTER:
      Filter(!m_bFiltered);
      break;
   default:
      event.Skip();
   }
}

void mccLogWindow2::OnMouseWheel(wxMouseEvent &event)
{
   ScrollTo(xPos, yPos - 2 * event.GetWheelRotation() / event.GetWheelDelta());
}

void mccLogWindow2::OnKeyUp(wxKeyEvent &event)
{
   wxCommandEvent cmd(wxEVT_TEXT_SELECTED, m_windowId);
   InitCommandEvent(cmd);
   GetParent()->GetEventHandler()->ProcessEvent(cmd);
   event.Skip();
}

void mccLogWindow2::OnKeyDown(wxKeyEvent &event)
{
   int code = event.GetKeyCode();

   UINT len;
   GetLine(m_cursor.y, &len);
   int line = m_cursor.y, col = m_cursor.x;
   switch (code)
   {
   case WXK_UP:
      if (line > 0)
         line--;
      if (event.ControlDown())
         ScrollTo(xPos, yPos - 1);
      break;
   case WXK_DOWN:
      if (line < m_nLines - 1)
         line++;
      if (event.ControlDown())
         ScrollTo(xPos, yPos + 1);
      break;
   case WXK_LEFT:
      if (col == 0 && line > 0) // at beginning of any line except first
      {
         line--;
         col--;
      }
      else if (col > 0)
         col--;
      break;
   case WXK_RIGHT:
      if (col >= (int)len && line < m_nLines - 1) // end of any line except last
      {
         col = 0;
         line++;
      }
      else if (col < (int)len)
         col++;
      break;
   case WXK_HOME:
      col = 0;
      if (event.ControlDown())
         line = 0;
      break;
   case WXK_END:
      if (event.ControlDown())
      {
         line = m_nLines - 1;
         col = -1;
      }
      else
         col = len;
      break;
#if wxCHECK_VERSION(2, 8, 0)
   case WXK_PAGEUP:
#else
   case WXK_PRIOR:
#endif
      ScrollTo(xPos, yPos - yClient / yChar);
      line -= yClient / yChar;
      break;
#if wxCHECK_VERSION(2, 8, 0)
   case WXK_PAGEDOWN:
#else
   case WXK_NEXT:
#endif
      ScrollTo(xPos, yPos + yClient / yChar);
      line += yClient / yChar;
      break;

   case 'A': // Ctrl-A = Select all
      if (event.ControlDown()) {
         MoveCursor(0, 0);
         MoveCursor(m_nLines - 1, -1, true);
      }
      return;

   case 'C': // Ctrl-C = Copy
      if (event.ControlDown())
         Copy();
      return;

   case 'E': // Ctrl-E = Errors only
      if (event.ControlDown())
         Filter(!IsFiltered());
      return; // don't call MoveCursor() again

   case 'F': // Ctrl-F = Find
      if (event.ControlDown())
         ShowFindDialog();
      return;

   case WXK_F3: // find next
      if (!m_findData.GetFindString())
         ShowFindDialog();
      else {
         if (event.ShiftDown())
            m_findData.SetFlags(m_findData.GetFlags() & ~wxFR_DOWN);
         else
            m_findData.SetFlags(m_findData.GetFlags() | wxFR_DOWN);
         if (!DoFind())
            wxBell();
      }
      return;

   // this code copied from /src/wx2/src/msw/textctrl.cpp
   // ... and later made redundant by overriding MSWWindowProc()
   /*case WXK_TAB:
      {
      wxNavigationKeyEvent eventNav;
      eventNav.SetDirection(!event.ShiftDown());
      eventNav.SetWindowChange(event.ControlDown());
      eventNav.SetEventObject(this);

      if ( GetParent()->GetEventHandler()->ProcessEvent(eventNav) )
          return;
      }
      break;*/

   case WXK_RETURN:
   case WXK_NUMPAD_ENTER:
      {
      wxCommandEvent cmdEvent(wxEVT_COMMAND_TEXT_ENTER, m_windowId);
      InitCommandEvent(cmdEvent);
      //event.SetString(GetValue()); // if the app needs a string, it can get it from us
      if ( !GetEventHandler()->ProcessEvent(cmdEvent) )
         event.Skip();
      }
      return;

   default:
      //debug("Key code: %d\n", code);
      event.Skip();
      return;
   }

   MoveCursor(line, col, event.ShiftDown());
}

void mccLogWindow2::OnSize(wxSizeEvent &event)
{
   // Retrieve the dimensions of the client area.
   RECT rc;
   ::GetClientRect((HWND)m_hWnd, &rc);
   yClient = rc.bottom - rc.top;
   xClient = rc.right - rc.left;

   int old_xPos = xPos, old_yPos = yPos;
   AdjustScrollbars(wxHORIZONTAL | wxVERTICAL);
   xPos = GetScrollPos(wxHORIZONTAL);
   yPos = GetScrollPos(wxVERTICAL);

   if (xPos != old_xPos || yPos != old_yPos)
      Redraw();

   event.Skip();
}

void mccLogWindow2::OnSetFocus(wxFocusEvent &event)
{
   if (m_caret)
      m_caret->Move(m_caretPos);
   event.Skip();
}

void mccLogWindow2::OnLoseFocus(wxFocusEvent &event)
{
   event.Skip();
}

int mccLogWindow2::SetDefaultStyle(const wxTextAttr& style)
{
   if (style.HasFont()) // not yet supported
      return -1;

   if (m_styles.GetCount() >= 64)
      return -1; // we only have 6 bits to store style numbers

   for (UINT i = 0; i < m_styles.GetCount(); i++)
   {
      if (style.HasTextColour() == m_styles[i].HasTextColour() &&
          style.GetTextColour() == m_styles[i].GetTextColour() &&
          style.HasBackgroundColour() == m_styles[i].HasBackgroundColour() &&
          style.GetBackgroundColour() == m_styles[i].GetBackgroundColour() )
         return m_defaultStyle = i;
   }

   m_styles.Add(style);

   return m_defaultStyle = m_styles.GetCount() - 1;
}

int mccLogWindow2::SetErrorStyle(const wxTextAttr &style)
{
   int oldDefault = m_defaultStyle;
   int oldError = m_errorStyle;
   SetDefaultStyle(style);
   m_errorStyle = m_defaultStyle;
   m_defaultStyle = oldDefault;
   return oldError;
}

void mccLogWindow2::AppendText(LPCSTR ptext, int lineControl /*= MLW_DEFAULT*/)
{
   // keep track of number of lines added, and scroll by that much

   if (m_bFiltered)
      Filter(false);

   if (ptext == NULL || ptext[0] == 0)
      return;

   // find lengths of lines added
   while (1)
   {
      int cb = 0;
      while (ptext[cb] && ptext[cb] != '\n')
         cb++;

      int len = cb;
      if (cb && ptext[cb-1] == '\r')
         len--;

      if (m_bStartNewLine)
      {
         ls.AddLine(ptext, len, m_defaultStyle | (lineControl & MLW_SHOWALWAYS));
         m_nLines++;
      }
      else if (len)
         ls.AppendText(ptext, len, m_defaultStyle | (lineControl & MLW_SHOWALWAYS));

      if (ptext[cb] == '\n')
      {
         m_bStartNewLine = true;
         cb++;
      }
      else
         m_bStartNewLine = false;

      if (ptext[cb] == 0)
         break;

      ptext += cb;
   }

   if (lineControl == MLW_LINE_BREAK)
      m_bStartNewLine = true;
   else if (lineControl == MLW_LINE_NOBREAK)
      m_bStartNewLine = false;

   int old_xPos = xPos;
   int old_yPos = yPos;
   int old_xClientMax = xClientMax;

   if (m_scrollStyle == SCROLL_ALWAYS ||
       (m_scrollStyle == SCROLL_AUTO && IsCursorAtEnd()) )
   {
      yPos = m_nLines - (yClient / yChar);
      if (yPos < 0)
         yPos = 0;
      xPos = 0;
      AdjustScrollbars(wxVERTICAL);
      if (xPos != old_xPos)
         AdjustScrollbars(wxHORIZONTAL);
   }
   else
      AdjustScrollbars(wxVERTICAL);

   bool showCaret;
   if (m_caret && m_caret->IsVisible())
   {
      showCaret = true;
      m_caret->Hide();
   }
   else
      showCaret = false;

   yPos = GetScrollPos(wxVERTICAL);
   if (yPos == old_yPos)
   {
      if (m_nLines <= yClient / yChar)
      {
         // no scrolling yet, so just add a bottom line of text
         wxClientDC dc(this);
         Redraw(dc, (m_nLines - 1) * yChar, m_nLines * yChar);
      }
      else // appending to an existing line
      {
         wxClientDC dc(this);
         Redraw(dc, (m_nLines - yPos - 1) * yChar, (m_nLines - yPos) * yChar);
      }
   }
   else
   {
      if (yPos - old_yPos == 1)
      {
         // invalidate where the old bottom row would have been, which was empty
         RECT rc = { 0, yClient - (yClient % yChar), xClient, yClient };
         InvalidateRect((HWND)m_hWnd, &rc, FALSE);
      }
      POINT pos;
      BOOL rc = GetCaretPos(&pos);
      ::ScrollWindow((HWND)m_hWnd, 0, yChar * (old_yPos - yPos), NULL, NULL);
      rc = GetCaretPos(&pos);
      UpdateWindow((HWND)m_hWnd);
   }

   //MoveCursor(m_nLines - 1, 0);
   if (showCaret)
   {
      if (m_bCursorAtEnd)
      {
         m_cursor.x = 0;
         m_cursor.y = m_nLines - 1;
         m_selStart = m_cursor;
         if (::GetFocus() == (HWND)m_hWnd)
            m_caret->Move(1, (m_nLines - yPos - 1) * yChar);
      }
      m_caret->Show();
   }

   if (xClientMax > old_xClientMax)
      AdjustScrollbars(wxHORIZONTAL);
}

void mccLogWindow2::Redraw()
{
   xPos = GetScrollPos(wxHORIZONTAL);
   yPos = GetScrollPos(wxVERTICAL);

   InvalidateRect((HWND)m_hWnd, NULL, FALSE);
   UpdateWindow((HWND)m_hWnd);
}

void mccLogWindow2::DrawLinePart(wxDC &dc, int nStyle, LPCSTR text, int len, RECT &rc)
{
   HDC hdc = (HDC)dc.GetHDC();

   // Setting text colors in a DC doesn't take effect until you call dc.DrawText(),
   // which I don't want to do because it takes a wxString.

   if (nStyle != m_selectedStyle)
   {
      const wxTextAttr &ta = m_styles[nStyle];
      m_selectedStyle = nStyle;
      if (ta.HasTextColour())
         SetTextColor(hdc, ta.GetTextColour().GetPixel());
      else
         SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

      if (ta.HasBackgroundColour())
      {
         SetBkColor(hdc, ta.GetBackgroundColour().GetPixel());
         SetBkMode(hdc, OPAQUE);
      }
      else
         SetBkMode(hdc, TRANSPARENT);
   }

   if (len > 0) {
      DrawTextA(hdc, text, len, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
      DrawTextA(hdc, text, len, &rc, DT_SINGLELINE | DT_NOPREFIX);
      //rc.left = rc.right;
   }
}

void mccLogWindow2::Redraw(wxDC &dc, int rcPaint_top, int rcPaint_bottom)
{
   // Find painting limits
   int FirstLine = wxMax (0, yPos + rcPaint_top / yChar);
   int LastLine = wxMin ((int)m_nLines - 1, yPos + rcPaint_bottom / yChar);

   dc.SetFont(m_font);
   m_selectedStyle = -1;

   //m_nPaintCount++;

   // GetDC() takes about 35us
   // ReleaseDC() takes about 2us
   // InvalidateRect() is about 4us, I think.  May depend on size and bErase.
   // UpdateWindow() is maybe about 130us, including DC ops but no painting
   // ScrollWindow() -- ?

   // The control has to be blazingly fast when appending text.
   // This is why we calculate the maximum line length as we draw each line;
   // when appending, we don't want to call GetDC() more than once, so while we
   // have the DC, we get the line length.
   // Calling DrawText() with DT_CALCRECT doesn't take that long anyway.

   // Draw each line separately.
   // This is a little slower than drawing them all at once, but much easier.

   int x = 1 + xChar * (-xPos);
   int y = yChar * (FirstLine - yPos);

   int ssl = -1, sel = -1; // selection starting and ending lines
   int ssc = -1, sec = -1; // selection starting and ending columns
   //! this might be easier if m_cursor and m_selStart were plain integers.
   // Of course, then we'd have to call PositionToXY() twice.
   if (m_selStart != m_cursor)
   {
      if (m_selStart.y == m_cursor.y)
      {
         ssl = sel = m_selStart.y;
         ssc = wxMin(m_selStart.x, m_cursor.x);
         sec = wxMax(m_selStart.x, m_cursor.x);
      }
      else if (m_selStart.y > m_cursor.y)
      {
         ssl = m_cursor.y;
         ssc = m_cursor.x;
         sel = m_selStart.y;
         sec = m_selStart.x;
      }
      else if (m_selStart.y < m_cursor.y)
      {
         ssl = m_selStart.y;
         ssc = m_selStart.x;
         sel = m_cursor.y;
         sec = m_cursor.x;
      }
   }

   for (int i = FirstLine; i <= LastLine; i++)
   {
      RECT rc = { x, y, x + xClientMax, y + yChar };
      y += yChar;

      UINT len;
      int blocklen, done;
      LPCSTR line = GetLine(i, &len);
      UCHAR *info = (UCHAR*)line + len;
      if (!len)
         continue;

      if (i >= ssl && i <= sel)
      {
         int ls = (i == ssl ? ssc : 0);
         int le = (i == sel ? sec : len);
         if (i == ssl && ls > 0) // draw leading unselected part of line
         {
            // this part handles multiple colors
            for (done = 0; done < ssc; done += blocklen, info += 3)
            {
               if (*info & MLW_MOREINFOBYTES)
                  blocklen = *(USHORT*)(info + 1);
               else
                  blocklen = len - done;
               if (blocklen + done > ssc)
                  blocklen = ssc - done;
               DrawLinePart(dc, *info & MLW_STYLEMASK, line + done, blocklen, rc);
               rc.left = rc.right;
            }
         }

         if (le > ls)
         {
            // draw selected part of line
            DrawLinePart(dc, 1, line + ls, le - ls, rc);
            rc.left = rc.right;
         }

         if (i == sel) // draw trailing unselected part of line
         {
            int skip = 0;
            info = (UCHAR*)line + len;
            while (*info & MLW_MOREINFOBYTES)
            {
               skip += *(USHORT*)(info + 1);
               if (skip > le)
               {
                  skip = le;
                  break;
               }
               info += 3;
            }

            // this part handles multiple colors
            for (done = le; done < (int)len; done += blocklen, info += 3)
            {
               if (*info & MLW_MOREINFOBYTES)
               {
                  blocklen = *(USHORT*)(info + 1) - skip;
                  skip = 0;
               }
               else
                  blocklen = len - done;
               DrawLinePart(dc, *info & MLW_STYLEMASK, line + done, blocklen, rc);
               rc.left = rc.right;
            }
         }
      }
      else
      {
         // this part handles multiple colors
         for (done = 0; done < (int)len; done += blocklen, info += 3)
         {
            if (*info & MLW_MOREINFOBYTES)
               blocklen = *(USHORT*)(info + 1);
            else
               blocklen = len - done;

            DrawLinePart(dc, *info & MLW_STYLEMASK, line + done, blocklen, rc);
            int pixels = rc.right - rc.left;
            rc.left = rc.right;
            xClientMax = wxMax(xClientMax, pixels);
         }
      }
   }
}

void mccLogWindow2::OnPaint(wxPaintEvent &WXUNUSED(event))
{
   wxPaintDC dc(this);
   HDC hdc = (HDC)dc.GetHDC();

   wxRegionIterator upd(GetUpdateRegion()); // get the update rect list
   wxBrush *brush = wxTheBrushList->FindOrCreateBrush(GetBackgroundColour(), wxSOLID);
   HBRUSH hbrush = (HBRUSH)brush->GetResourceHandle();
   RECT rc;

   while (upd)
   {
      rc.left   =           upd.GetX();
      rc.top    =           upd.GetY();
      rc.right  = rc.left + upd.GetW();
      rc.bottom = rc.top  + upd.GetH();

      // Repaint this rectangle
      ::FillRect(hdc, &rc, hbrush);
      Redraw(dc, rc.top, rc.bottom);
      upd ++ ;
   }

}

void mccLogWindow2::AdjustScrollbars(int orientFlags)
{
   SCROLLINFO si;

   if (orientFlags & wxVERTICAL)
   {
      // Set the vertical scrolling range and page size
      si.cbSize = sizeof(si);
      si.fMask  = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL | SIF_POS;
      si.nMin   = 0;
      si.nMax   = m_nLines - 1;
      si.nPage  = yClient / yChar;
      si.nPos   = yPos;
      BOOL bRedraw = 1;
      if (si.nMax > (int)si.nPage)
      {
         // If scroll box position and height don't change, we don't have to redraw the scrollbar.
         int height = yClient - 2 * GetSystemMetrics(SM_CYVSCROLL);  // height of scroll bar, minus arrows
         int sbh = (height * si.nPage + (si.nMax >> 1)) / m_nLines; // calculate ratio, rounding up
         if (yPos == m_old_yPos + 1 && sbh < 8) // 8 is the minimum size Win2K draws.  I don't know about XP.
            bRedraw = 0;
         //debug("sbh: %d\n", sbh);
      }
      m_old_yPos = yPos;
      SetScrollInfo((HWND)m_hWnd, SB_VERT, &si, bRedraw);
   }

   if (orientFlags & wxHORIZONTAL)
   {
      // Set the horizontal scrolling range and page size.
      si.cbSize = sizeof(si);
      si.fMask  = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL | SIF_POS;
      si.nMin   = 0;
      si.nMax   = 2 + xClientMax / xChar;
      si.nPage  = xClient / xChar;
      si.nPos   = xPos;
      SetScrollInfo((HWND)m_hWnd, SB_HORZ, &si, TRUE);
   }
}

void mccLogWindow2::CalcFontSizeInfo()
{
   TEXTMETRIC tm;

   // Get the handle to the client area's device context.
   wxClientDC dc(this);
   dc.SetFont(m_font);
   HDC hdc = (HDC)dc.GetHDC();

   // Extract font dimensions from the text metrics.
   GetTextMetrics (hdc, &tm);
   xChar = tm.tmAveCharWidth;
   xUpper = (tm.tmPitchAndFamily & 1 ? 3 : 2) * xChar/2;
   yChar = tm.tmHeight + tm.tmExternalLeading;

   // Set an arbitrary maximum width for client area.
   // (xClientMax is the sum of the widths of 48 average
   // lowercase letters and 12 uppercase letters.)
   //xClientMax = 48 * xChar + 12 * xUpper;
   xClientMax = xChar; // we adjust this in AppendText() to accommodate the longest line

   // set caret size
   if (m_caret)
      m_caret->SetSize(2, yChar);
}

bool mccLogWindow2::MSWOnScroll(int orientation, WXWORD wParam, WXWORD pos, WXHWND control)
{
   if ( control && control != m_hWnd ) // Prevent infinite recursion
   {
      // This was copied from wxWindowMSW.  Do we need it?  Probably not...
      wxWindow *child = wxFindWinFromHandle(control);
      if ( child )
         return child->MSWOnScroll(orientation, wParam, pos, control);
   }

   if (orientation == wxVERTICAL)
   {
      int new_pos = AdjustScrollPos(LOWORD(wParam), SB_VERT);
      // If the position has changed, scroll window and update it
      if (new_pos != yPos)
      {
         ::ScrollWindow((HWND)m_hWnd, 0, yChar * (yPos - new_pos), NULL, NULL);
         yPos = new_pos;
         UpdateWindow ((HWND)m_hWnd);
      }
   }
   else
   {
      int new_pos = AdjustScrollPos(LOWORD(wParam), SB_HORZ);
      // If the position has changed, scroll window and update it
      if (new_pos != xPos)
      {
         ::ScrollWindow((HWND)m_hWnd, xChar * (xPos - new_pos), 0, NULL, NULL);
         xPos = new_pos;
         UpdateWindow ((HWND)m_hWnd);
      }
   }
   return true;
}

int mccLogWindow2::AdjustScrollPos(int event, int bar)
{
   SCROLLINFO si;
   si.cbSize = sizeof (si);
   si.fMask  = SIF_ALL;

   // Get all the scroll bar information
   GetScrollInfo ((HWND)m_hWnd, bar, &si);
   // Save the position for comparison later on
   //pos = si.nPos;

   switch (event)
   {
      // user clicked the HOME keyboard key
   case SB_TOP:
      si.nPos = si.nMin;
      break;

      // user clicked the END keyboard key
   case SB_BOTTOM:
      si.nPos = si.nMax;
      break;

      // user clicked the top arrow
   case SB_LINEUP:
      si.nPos -= 1;
      break;

      // user clicked the bottom arrow
   case SB_LINEDOWN:
      si.nPos += 1;
      break;

      // user clicked the shaft above the scroll box
   case SB_PAGEUP:
      si.nPos -= si.nPage - 1; // leave a line of overlap
      break;

      // user clicked the shaft below the scroll box
   case SB_PAGEDOWN:
      si.nPos += si.nPage - 1; // leave a line of overlap
      break;

      // user dragged the scroll box
   case SB_THUMBTRACK:
      si.nPos = si.nTrackPos;
      break;

   default:
      break;
   }

   // Set the position and then retrieve it.  Due to adjustments
   //   by Windows it may not be the same as the value set.
   si.fMask = SIF_POS;
   SetScrollInfo ((HWND)m_hWnd, bar, &si, TRUE);
   GetScrollInfo ((HWND)m_hWnd, bar, &si);

   return si.nPos;
}

bool mccLogWindow2::SetFont(const wxFont& font)
{
   bool rc = wxControl::SetFont(font);
   CalcFontSizeInfo();
   Redraw();
   return rc;
}

void mccLogWindow2::Clear()
{
   ls.Clear();
   m_nLines = 0;
	m_nCurrentErrLine = 0;
   xClientMax = xChar;
   xPos = yPos = 0;
   MoveCursor(0, 0);
   AdjustScrollbars(wxHORIZONTAL | wxVERTICAL);
   Redraw();
}

BOOL mccLogWindow2::MouseToLineCol(const wxPoint &pos, int &line, int &col)
{
   int x = pos.x + xPos * xChar;
   line = yPos + (pos.y / yChar);

   if (line >= m_nLines)
      line = m_nLines - 1;

   if (x)
      x--;   // text is drawn one pixel from the left edge, but x==-1 confuses GetTextExtentExPoint()

   UINT len;
   LPCSTR text = GetLine(line, &len);
   SIZE size;
   wxClientDC dc(this);
   dc.SetFont(m_font);
   HDC hdc = (HDC)dc.GetHDC();
   BOOL rc = GetTextExtentExPoint(hdc, text, len, x, &col, NULL, &size);

   if (rc && col < (int)len)
   {
      // see if we are closer to the next character
      int len1, len2;
      GetTextExtentPoint32(hdc, text, col, &size);
      len1 = size.cx;
      GetTextExtentPoint32(hdc, text, col+1, &size);
      len2 = size.cx;
      if (len2 - x < x - len1)
         col++;
   }

   return rc;
}

int mccLogWindow2::MoveCursor(int line, int col, bool bExtendSel /*= false*/)
{
   if (line < 0)
      line = 0;
   else if (line >= m_nLines)
      line = wxMax(0, m_nLines - 1);

   // check later for col past end of line

   UINT len;
   LPCSTR text = GetLine(line, &len);
   SIZE size;
   wxClientDC dc(this);
   dc.SetFont(m_font);

   if (col == -1 || col > (int)len) // (don't need to share this bit)
      col = len;
   else if (col < 0)
      col = 0;

   GetTextExtentPoint32((HDC)dc.GetHDC(), text, col, &size);

   size.cx += 1;
   size.cy = line * yChar;
   // now "size" contains the new virtual coordinates of the cursor

   int ssl, sel;
   if (bExtendSel)
   {
      ssl = wxMin(m_cursor.y, line);
      sel = wxMax(m_cursor.y, line);
   }
   else
   {
      ssl = wxMin(m_cursor.y, m_selStart.y);
      sel = wxMax(m_cursor.y, m_selStart.y);
   }

   if (m_cursor != m_selStart || bExtendSel)
   {
      // schedule selection for repainting
      RECT rc = { 0, (ssl - yPos) * yChar, xClient, (sel + 1 - yPos) * yChar };
      InvalidateRect((HWND)m_hWnd, &rc, FALSE);
   }

   m_cursor.x = col;
   m_cursor.y = line;

   if (!bExtendSel)
      m_selStart = m_cursor;

   // If we're on the last line, then len will be correct.
   m_bCursorAtEnd = (m_cursor.y >= (int)m_nLines - 1 && m_cursor.x >= (int)len);

   // The rest of this function scrolls the window to make the cursor visible.

   int new_xPos = xPos, new_yPos = yPos;

   //! Can we tell whether we're moving the cursor toward a window edge with the keyboard,
   //  so we can show farther ahead?

   if (line < yPos)
      new_yPos = line;
   else if (line >= yPos + (yClient / yChar))
      new_yPos = line + 1 - (yClient / yChar);

   if (size.cx < xPos * xChar)
      new_xPos = size.cx / xChar - 4;
   else if (size.cx - xClient > xPos * xChar)
      new_xPos = (size.cx - xClient) / xChar + 4;

   ScrollTo(new_xPos, new_yPos);

   m_caretPos = wxPoint(size.cx - xPos * xChar, size.cy - yPos * yChar);
   if (m_caret && ::GetFocus() == (HWND)m_hWnd)
      m_caret->Move(m_caretPos.x, m_caretPos.y);

   return 1;
}

void mccLogWindow2::ScrollTo(int new_xPos, int new_yPos)
{
   SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };

   //debug("Requested (%d, %d)\n", new_xPos, new_yPos);

   GetScrollInfo((HWND)m_hWnd, SB_HORZ, &si);
   new_xPos = wxMax(new_xPos, si.nMin);
   new_xPos = wxMin(new_xPos, si.nMax + 1 - (int)si.nPage);

   GetScrollInfo((HWND)m_hWnd, SB_VERT, &si);
   new_yPos = wxMax(new_yPos, si.nMin);
   new_yPos = wxMin(new_yPos, si.nMax + 1 - (int)si.nPage);

   if (new_xPos == xPos && new_yPos == yPos)
      return;

   //debug("Got (%d, %d)\n", new_xPos, new_yPos);

   int dx = (xPos - new_xPos) * xChar;
   int dy = (yPos - new_yPos) * yChar;

   xPos = new_xPos;
   yPos = new_yPos;

   AdjustScrollbars(wxVERTICAL | wxHORIZONTAL);

   ::ScrollWindow((HWND)m_hWnd, dx, dy, NULL, NULL);
   UpdateWindow((HWND)m_hWnd);
}

int mccLogWindow2::Filter(bool ErrorsOnly)
{
   if (ErrorsOnly == m_bFiltered)
      return m_nLines;

   int newCursorLine = 0;

   if (ErrorsOnly)
   {
      int i, filterCount = 0;
      UCHAR *info;
      UINT len;

      // count the number of error lines
      for (i = 0; i < m_nLines; i++)
      {
         info = (UCHAR*)ls.GetLine(i, &len);
         info += len;
         while (1) {
            if ((*info & MLW_STYLEMASK) == m_errorStyle || (*info & MLW_SHOWALWAYS)) {
               filterCount++;
               break;
            }
            if (*info & MLW_MOREINFOBYTES)
               info += 3;
            else
               break;
         }
      }

      filterLines = new int[filterCount]; // allocate some memory for error line indexes
      filterCount = 0;

      for (i = 0; i < m_nLines; i++)
      {
         // move cursor to same line in other view
         if (i == m_cursor.y)
            newCursorLine = filterCount;

         info = (UCHAR*)ls.GetLine(i, &len);
         info += len;
         while (1) {
            if ((*info & MLW_STYLEMASK) == m_errorStyle || (*info & MLW_SHOWALWAYS)) {
               filterLines[filterCount++] = i;
               break;
            }
            if (*info & MLW_MOREINFOBYTES)
               info += 3;
            else
               break;
         }
      }

      m_nLines = filterCount;
   }
   else
   {
      // move cursor to same line in other view
      newCursorLine = this->GetStoredLineNumber(m_cursor.y);

      // free the list of error lines and restore normal operation
      delete [] filterLines;
      m_nLines = ls.GetNumberOfLines();
   }
   m_bFiltered = ErrorsOnly;

   AdjustScrollbars(wxVERTICAL | wxHORIZONTAL);
   MoveCursor(newCursorLine, m_cursor.x, false);
   Redraw();

   return m_nLines;
}

bool mccLogWindow2::FindText(LPCSTR search, wxPoint *result, int start /*= 0*/, int flags /*= 0*/)
{
   long sl, sc; // line and column to start search
   PositionToXY(start, &sc, &sl);

   // if wxFR_MATCHCASE was not specified, convert the search string to upper case
   wxString wx_search(search);
   int srclen = (int)wx_search.Len();
   if ((flags & wxFR_MATCHCASE) == 0)
      wx_search.MakeUpper();
   search = wx_search.c_str();

   if (flags & wxFR_DOWN)
   {
      for (int line = sl; line < m_nLines; line++)
      {
         int len, lineStart, rc;
         LPCSTR text = GetLine(line, (UINT*)&len);

         if (line == sl)
            lineStart = sc; // start at specified column of first line
         else
            lineStart = 0;  // start at column zero

         // Since line is not null-terminated, we can't use strstr().
         // Anyway, we have to allow for case-insensitive searches.
         for ( ; lineStart < len - srclen; lineStart++)
         {
            if (flags & wxFR_MATCHCASE)
               rc = strncmp(text + lineStart, search, srclen);
            else
               rc = _strnicmp(text + lineStart, search, srclen);

            if (!rc)
            {
               result->y = line;
               result->x = lineStart;
               return true;
            }
         }
      } // end line loop
   }
   else // search up
   {
      for (int line = sl; line > 0; line--)
      {
         int len, lineStart, rc;
         LPCSTR text = GetLine(line, (UINT*)&len);

         if (line == sl)
            lineStart = sc; // start at specified column of starting line
         else
            lineStart = len;  // start at end of line

         for ( ; lineStart >= srclen; lineStart--)
         {
            if (flags & wxFR_MATCHCASE)
               rc = strncmp(text + lineStart - srclen - 1, search, srclen);
            else
               rc = _strnicmp(text + lineStart - srclen - 1, search, srclen);

            if (!rc)
            {
               result->y = line;
               result->x = lineStart - srclen;
               return true;
            }
         }
      } // end line loop
   } // end if (flags & wxFR_DOWN)

   return false; // not found
}

bool mccLogWindow2::ShowFindDialog()
{
   if (!m_findDlg)
      m_findDlg = new wxFindReplaceDialog(this, &m_findData, "Find Text", wxFR_NOWHOLEWORD);
   return m_findDlg->IsShown() || m_findDlg->Show();
}

void mccLogWindow2::OnFind(wxFindDialogEvent &WXUNUSED(event))
{
   if (!DoFind())
      wxBell();
}

bool mccLogWindow2::DoFind()
{
   // start forward searches from the end, and backward searches from the beginning of selection
   long from, to;
   GetSelection(&from, &to);
   int start = from;
   if (m_findData.GetFlags() & wxFR_DOWN)
      start = to;

   wxPoint pos;
   if (FindText(m_findData.GetFindString(), &pos, start, m_findData.GetFlags()))
   {
      MoveCursor(pos.y, pos.x);
      MoveCursor(pos.y, pos.x + (int)m_findData.GetFindString().Len(), true);
      return true;
   }
   else
      return false;
}

void mccLogWindow2::OnFindClose(wxFindDialogEvent &WXUNUSED(event))
{
   // We can't use the old dialog object anymore.  A new one will be created if needed.
   delete m_findDlg;
   m_findDlg = NULL;
}

void mccLogWindow2::Copy()
{
   if (!HasSelection())
      return;

   if (wxTheClipboard->Open())
   {
      // wxTextFile::Translate() is needed to transform all '\n' into "\r\n"
      wxString text = wxTextFile::Translate(GetSelectionText());
      wxTextDataObject *data = new wxTextDataObject(text);
      wxTheClipboard->SetData(data);
   }
}

void mccLogWindow2::CopyAll()
{
   if (wxTheClipboard->Open())
   {
      wxString text = GetRange(0, GetLastPosition());
      // wxTextFile::Translate() is needed to transform all '\n' into "\r\n"
      text = wxTextFile::Translate(text);
      wxTextDataObject *data = new wxTextDataObject(text);
      wxTheClipboard->SetData(data);
   }
}

wxString mccLogWindow2::GetRange(long from, long to)
{
   if (from > to)  // swap endpoints so "from" comes before "to"
   {
      long tmp = from;
      from = to;
      to = tmp;
   }

   wxString str;
   char *buf = str.GetWriteBuf(to - from);
   int buflen = 0; // number of characters written so far
   long sl, el; // starting and ending lines
   long sc, ec; // starting and ending columns
   PositionToXY(from, &sc, &sl);
   PositionToXY(to, &ec, &el);

   int len;
   LPCSTR text;
   for (int line = sl; line <= el; line++)
   {
      text = GetLine(line, (UINT*)&len);
      if (line == el)
         len = ec;
      if (line == sl)
      {
         text += sc;
         len -= sc;
      }
      if (len > 0)
      {
         memcpy(buf + buflen, text, len);
         buflen += len;
      }
      if (line < el)
         buf[buflen++] = '\n';
   }

   str.UngetWriteBuf(buflen);
   return str;
}

long mccLogWindow2::XYToPosition(long x, long y)
{
   return ls.XYToPosition(x, GetStoredLineNumber(y));
}

bool mccLogWindow2::PositionToXY(long pos, long *x, long *y)
{
   bool rc = ls.PositionToXY(pos, x, y);
   *y = GetDisplayedLineNumber(*y);
   return rc;
}

long mccLogWindow2::GetLastPosition()
{
   if (!m_nLines)
      return 0;

   UINT len;
   GetLine(m_nLines - 1, &len);
   return XYToPosition(len, m_nLines - 1);
}

int mccLogWindow2::GetDisplayedLineNumber(int storedLineNo)
{
   if (!IsFiltered())
      return storedLineNo;

   // binary search through filterLines
   int lo = 0, hi = m_nLines - 1, mid;
   while (lo < hi)
   {
      mid = (lo + hi) >> 1;
      if (filterLines[mid] == storedLineNo)
         return mid;
      else if (filterLines[mid] < storedLineNo)
         hi = mid;
      else if (filterLines[mid] > storedLineNo)
         lo = mid;
   }
   return -1; // not found
}

void mccLogWindow2::debug(LPCSTR fmt, ...)
{
   if (!s_txtDebug)
      return;

   va_list args;
   va_start(args, fmt);
   s_txtDebug->AppendText(wxString::FormatV(fmt, args));
}

void mccLogWindow2::ShowPrintDlg(bool WXUNUSED(bSelOnly))
{
//   MCC_PRINT printer;
//   if (printer.mccGetError().Len())
//   {
//      wxMessageBox(printer.mccGetError(), "Printing Error", wxOK | wxICON_ERROR, this);
//      return;
//   }
//
//   printer.mccStartDoc(wxString::Format("MCC Log output " + wxDateTime::UNow().Format()));
//   printer.mccSetFont(GetFont());
//   printer.mccPageInit();
//
//   long beginLine, endLine, line;
//   if (bSelOnly)
//   {
//      long from, to;
//      GetSelection(&from, &to);
//      PositionToXY(from, NULL, &beginLine);
//      PositionToXY(to, NULL, &endLine);
//   }
//   else
//   {
//      beginLine = 0;
//      endLine = m_nLines - 1;
//   }
//   for (line = beginLine; line <= endLine; line++)
//   {
//      printer.mccTextOut(GetLine(line));
//   }
}

bool mccLogWindow2::RemoveLine(int line)
{
   if (ls.RemoveLine(line))
   {
      m_nLines--;
      return true;
   }
   else
      return false;
}

void mccLogWindow2::replaceLastLine(LPCSTR ptext)
{
   RemoveLine(GetNumberOfLines() - 1);
   AppendText(ptext);
}



bool mccLogWindow2::DisplayFirstError()
{
//	wxPoint *result, int start /*= 0*/, int flags /*= 0*/ 
   int len;
//   long sl, sc; // line and column
//   PositionToXY(start, &sc, &sl);

   for (int line = 1; line < m_nLines; line++)
   {
      LPCSTR text = GetLine(line, (UINT*)&len);
//		if('E' == *text)
		if('E' == text[0])
		{
//			PositionToXY(line, &sc, &sl);
			ScrollTo(0, line);
			m_nCurrentErrLine = line;
			return true;
		}

   } // end line loop

   return false; // not found
}

bool mccLogWindow2::DisplayLastError()
{
   int len;
//   long sl, sc; // line and column

   for (int line = m_nLines; line > 0; line--)
   {
      LPCSTR text = GetLine(line, (UINT*)&len);
		if('E' == text[0])
		{
			ScrollTo(0, line);
			m_nCurrentErrLine = line;
			return true;
		}
   } // end line loop

   return false; // not found
}

bool mccLogWindow2::DisplayPreviousError()
{
   int len;
//   long sl, sc; // line and column

	if( 0 == m_nCurrentErrLine)
		m_nCurrentErrLine = m_nLines;	//  Use last line 
   for (int line = (m_nCurrentErrLine - 1); line > 0; line--)
   {
      LPCSTR text = GetLine(line, (UINT*)&len);
		if('E' == text[0])
		{
			ScrollTo(0, line);
			m_nCurrentErrLine = line;
			return true;
		}
   } // end line loop

   return false; // not found
}

bool mccLogWindow2::DisplayNextError()
{
   int len;
//   long sl, sc; // line and column

	if( 0 == m_nCurrentErrLine)
		m_nCurrentErrLine = m_nLines;	//  Use last line 
   for (int line = (m_nCurrentErrLine + 1); line < m_nLines; line++)
   {
      LPCSTR text = GetLine(line, (UINT*)&len);
		if('E' == text[0])
		{
			ScrollTo(0, line);
			m_nCurrentErrLine = line;
			return true;
		}
   } // end line loop

   return false; // not found
}
