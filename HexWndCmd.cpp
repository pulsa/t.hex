#include "precomp.h"
#include "hexwnd.h"
#include "hexdoc.h"
#include "atimer.h"
#include "undo.h"
#include <wx/msw/private.h>
#include <wx/numdlg.h>
#include "thFrame.h"
#include "settings.h"
#include "resource.h"
#include "dialogs.h"
#include "utils.h"
#include "toolwnds.h"
#include "datahelp.h"

#define new New

void HexWnd::CmdSetSelection(THSIZE iSelStart, THSIZE iSelEnd,
                             int region /*= -1*/, int digit /*= 0*/,
                             int jumpiness /*= J_AUTO*/)
{
    if (iSelStart != m_iSelStart ||
        iSelEnd != m_iCurByte ||
        (region != m_iCurPane && region != -1) ||
        digit != m_iCurDigit)
    {

        //UndoAction *u = doc->undo->GetCurrent();
        //doc->undo->Truncate();
        //if (!u)
        //    u = doc->undo->AllocDataChange(doc);

        SetSelection(iSelStart, iSelEnd, region, digit);

        //if (u)
        //    GetSelection(u->newSel);
        //if (u) u->Print();
    }

    //ScrollToSelection();
    ScrollToRange(iSelStart, iSelEnd, jumpiness);
}

void HexWnd::CmdExtendSelection(THSIZE iSelEnd, int region /*= 0*/)
{
    //uint64 iSelStart = m_iSelStart;
    //if (m_iSelStart == m_iCurByte && iSelEnd <= m_iCurByte && m_iCurDigit > 0)
    //    iSelStart++;
    SetSelection(m_iSelStart, iSelEnd, region);
    ScrollToCursor();
    //!set_caret_pos(); //! blah.  Why is everything so hard?
}

void HexWnd::CmdMoveCursor(uint64 address, BOOL bExtendSel /*= FALSE*/,
                           int region /*= 0*/, int digit /*= 0*/, int jumpiness /*= J_AUTO*/)
{
    if (address > DocSize())
        return; //! should beep or return false
    if (region == 0)
        region = m_iCurPane;
    if (bExtendSel)
        CmdExtendSelection(address, region);
    else
        CmdSetSelection(address, address, region, digit, jumpiness);

    //!ScrollToCursor(); // already done in both helper functions
}

void HexWnd::CmdMoveRelative(int64 lines, int64 bytes, BOOL bExtendSel /* = FALSE*/)
{
    // Treat lines and bytes separately.  This correctly handles an up arrow at byte 2.
    int col;
    THSIZE line = ByteToLineCol(m_iCurByte, &col);
    if (lines < 0)
        line = Subtract(line, -lines);
    else
        line = Add(line, lines, m_iTotalLines);

    THSIZE byte = LineColToByte(line, col);
    if (bytes < 0)
        byte = Subtract(byte, -bytes);
    else
        byte = Add(byte, bytes, DocSize());

    CmdMoveCursor(byte, bExtendSel);
}

THSIZE HexWnd::NextBytePos(int dir, THSIZE current, int *pDigit /*= NULL*/)
{
    int col;
    THSIZE line = ByteToLineCol2(current, &col);

    if (dir > 0 && current < DocSize())
    {
        if (pDigit && *pDigit < m_pane[m_iCurPane].m_iColChars - 1)
        {
            (*pDigit)++;
            goto done;
        }
        if (pDigit)
            *pDigit = 0;

        col++;
        if (col >= m_pane[m_iCurPane].m_iCols)
        {
            col = 0;
            line++;
        }
    }
    else if (dir < 0)
    {
        if (pDigit && *pDigit > 0)
        {
            (*pDigit)--;
            goto done;
        }
        if (pDigit)
            *pDigit = m_pane[m_iCurPane].m_iColChars - 1;

        if (col > 0)
            col--;
        else if (line > 0)
        {
            line--;
            col = m_pane[m_iCurPane].m_iCols - 1;
        }
    }
done:
    return LineColToByte2(line, col);
}

void HexWnd::CmdCursorLeft()
{
    uint64 byte = m_iCurByte;
    int digit = m_iCurDigit;
    // Move one whole byte if region is read-only.
    //! Is this the Right Thing to do?
    if (ShiftDown() && digit > 0)
    {
        CmdSetSelection(byte + 1, byte);
        return;
    }

    if (!ShiftDown() && m_iSelStart != m_iCurByte) {  // behavior from other respectable editors
        CmdMoveCursor(wxMin(m_iSelStart, m_iCurByte), false);
        return;
    }

    if (DigitGranularity() && !ShiftDown())
        byte = NextBytePos(-1, m_iCurByte, &digit);
    else
    {
        digit = 0;
        byte = NextBytePos(-1, m_iCurByte);
    }
    CmdMoveCursor(byte, ShiftDown(), 0, digit);
}

void HexWnd::CmdCursorRight()
{
    uint64 byte = m_iCurByte;
    int digit = m_iCurDigit;

    if (!ShiftDown() && m_iSelStart != m_iCurByte) {  // behavior from other respectable editors
        CmdMoveCursor(wxMax(m_iSelStart, m_iCurByte), false);
        return;
    }

    if (byte == DocSize())
        return;

    // Move one whole byte if region is read-only.
    //! Is this the Right Thing to do?
    if (DigitGranularity() && !ShiftDown())
        byte = NextBytePos(1, m_iCurByte, &digit);
    else
    {
        digit = 0;
        byte = NextBytePos(1, m_iCurByte);
    }

    CmdMoveCursor(byte, ShiftDown(), 0, digit);
}

void HexWnd::CmdCursorUp()
{
    uint64 byte = Subtract(m_iCurByte, s.iLineBytes);
    CmdMoveCursor(byte, ShiftDown(), 0, DigitGranularity() ? m_iCurDigit : 0);
}

void HexWnd::CmdCursorDown()
{
    uint64 byte = Add(m_iCurByte, s.iLineBytes, DocSize());
    CmdMoveCursor(byte, ShiftDown(), 0, DigitGranularity() ? m_iCurDigit : 0);
}

void HexWnd::CmdCursorStartOfLine(wxCommandEvent &WXUNUSED(event))
{
    uint64 line = ByteToLineCol(m_iCurByte, NULL);
    CmdMoveCursor(LineColToByte(line, 0), ShiftDown());
}

void HexWnd::CmdCursorEndOfLine(wxCommandEvent &WXUNUSED(event))
{
    uint64 line = ByteToLineCol(m_iCurByte, NULL);
    int digit = 0;
    //if (DigitGranularity())
    //    digit = GetByteDigits() - 1;
    CmdMoveCursor(LineColToByte(line, s.iLineBytes - 1), ShiftDown(), 0, digit);
}

void HexWnd::CmdCursorStartOfFile(wxCommandEvent &WXUNUSED(event))
{
    CmdMoveCursor(0, ShiftDown());
}

void HexWnd::CmdCursorEndOfFile(wxCommandEvent &WXUNUSED(event))
{
    CmdMoveCursor(DocSize(), ShiftDown());
}

void HexWnd::CmdCursorPageUp(bool bCtrlDown /*= false*/)
{
    int col;
    uint64 cursorLine = ByteToLineCol(m_iCurByte, &col);

    if (bCtrlDown)
        cursorLine = m_iFirstLine;
    else {
        // is cursor line visible?
        if (cursorLine >= m_iFirstLine && cursorLine < m_iFirstLine + m_iVisibleLines)
        { // if so, scroll display one page up
            uint64 topLine = m_iFirstLine;
            if (topLine > m_iVisibleLines)
                topLine -= m_iVisibleLines;
            else
                topLine = 0;
            ScrollToLine(topLine);
        } // else: CmdMoveCursor will scroll display

        if (cursorLine > m_iVisibleLines)
            cursorLine -= wxMax(m_iVisibleLines, 1);
        else
            cursorLine = 0;
    }
    CmdMoveCursor(LineColToByte(cursorLine, col), ShiftDown());
}

void HexWnd::CmdCursorPageDown(bool bCtrlDown /*= false*/)
{
    int col;
    uint64 cursorLine = ByteToLineCol(m_iCurByte, &col);

    if (bCtrlDown)
        cursorLine = Add(m_iFirstLine, m_iVisibleLines, m_iTotalLines);
    else {
        // is cursor line visible?
        if (cursorLine >= m_iFirstLine && cursorLine < m_iFirstLine + m_iVisibleLines)
        { // if so, scroll display one page down
            uint64 topLine = m_iFirstLine;
            topLine += wxMax(m_iVisibleLines, 1);
            ScrollToLine(topLine);
        } // else: CmdMoveCursor will scroll display
    }

    //THSIZE byte = wxMin(doc->size, m_iCurByte + m_iVisibleLines * s.iLineBytes);
    THSIZE byte = Add(m_iCurByte, m_iVisibleLines * s.iLineBytes, DocSize());
    CmdMoveCursor(byte, ShiftDown());
}

void HexWnd::CmdViewLineUp(wxCommandEvent &event)
{
    if (m_iFirstLine > 0)
        ScrollToLine(m_iFirstLine - 1);
    MoveCursorIntoView();
}

void HexWnd::CmdViewLineDown(wxCommandEvent &event)
{
    ScrollToLine(m_iFirstLine + 1);
    MoveCursorIntoView();
}

void HexWnd::CmdViewPageUp(wxCommandEvent &event)
{
    if (m_iFirstLine > m_iVisibleLines)
        ScrollToLine(m_iFirstLine - m_iVisibleLines);
    else
        ScrollToLine(0);
}

void HexWnd::CmdViewPageDown(wxCommandEvent &event)
{
    ScrollToLine(m_iFirstLine + m_iVisibleLines);
}

void HexWnd::CmdViewTop(wxCommandEvent &event)
{
    uint64 line = ByteToLineCol(m_iCurByte, NULL);
    ScrollToLine(line);
}

void HexWnd::CmdViewCenter(wxCommandEvent &event)
{
    uint64 line = ByteToLineCol(m_iCurByte, NULL);
    uint32 half_screen = m_iVisibleLines / 2;
    if (line >= half_screen)
        ScrollToLine(line - half_screen);
    else
        ScrollToLine(0);
}

void HexWnd::CmdViewBottom(wxCommandEvent &event)
{
    uint64 line = ByteToLineCol(m_iCurByte, NULL);
    if (line > m_iVisibleLines)
        ScrollToLine(line - m_iVisibleLines);
    else
        ScrollToLine(0);
}

void HexWnd::CmdGotoAgain(wxCommandEvent &event) { }
//void HexWnd::CmdSave(wxCommandEvent &event) { }
//void HexWnd::CmdFindAgainForward(wxCommandEvent &event) { }
//void HexWnd::CmdFindAgainBackward(wxCommandEvent &event) { }

void HexWnd::CmdSelectAll(wxCommandEvent &event)
{
    CmdSetSelection(0, DocSize());
}

void HexWnd::CmdCopy(wxCommandEvent &event)
{
    thCopyFormat fmt(thCopyFormat::PANE);
    DoCopy(fmt);
}

void HexWnd::DoCopy(thCopyFormat fmt)
{
    uint64 iSelStart, iSelEnd;
    GetSelection(iSelStart, iSelEnd);

    if (iSelStart == iSelEnd)
        return; //! should we copy something else instead?
    //if (iSelEnd - iSelStart > 0x100000)
    //    return; // let's keep it simple for now

    if (fmt.dataFormat == thCopyFormat::PANE)
    {
        DisplayPane &pane = m_pane[m_iCurPane];

        // Extend selection boundary so it works for the current pane.
        //! Maybe this should happen somewhere else, like GetSelection()?  or Render()
        iSelStart -= iSelStart % pane.m_iColBytes;
        if (iSelEnd % pane.m_iColBytes)
            iSelEnd += pane.m_iColBytes - (iSelEnd % pane.m_iColBytes);

        switch(pane.id)
        {
        case DisplayPane::NUMERIC:
            if (pane.m_bFloat)
                fmt.dataFormat = thCopyFormat::FLOAT;
            else
                fmt.dataFormat = thCopyFormat::INTEGER;
            fmt.unicode = false;
            fmt.numberBase = pane.m_iBase;
            fmt.wordSize = pane.m_iColBytes;
            //fmt.wordsPerLine = s.iLineBytes / pane.m_iColBytes; //! what about remainder?
            fmt.wordsPerLine = 0;
            fmt.endianMode = s.iEndianMode;
            break;
        case DisplayPane::ANSI: //! to do: ensure CR+LF line endings and null terminator
        default:
            fmt.dataFormat = thCopyFormat::RAW;
            fmt.unicode = false;
            break;
        case DisplayPane::ID_UNICODE: //! to do: ensure CR+LF line endings and null terminator
            fmt.dataFormat = thCopyFormat::RAW;
            fmt.unicode = true;  //! allow for other source encodings
            break;
        }
    }
    else if (fmt.dataFormat == thCopyFormat::INTEGER)
        fmt.unicode = false;

    fmt.doc = doc;  //! maybe kinda temporary
    fmt.offset = iSelStart;
    fmt.srcLen = iSelEnd - iSelStart;
    
    wxString data = doc->Serialize(iSelStart, iSelEnd - iSelStart);
    frame->clipboard.SetData(data, m_pane[m_iCurPane].id, fmt);
}

bool CanConvertAsHex(const char *pData, size_t len, UINT srcFormat)
{
    const int incr = (srcFormat == CF_UNICODETEXT) ? 2 : 1;
    const char *end = pData + len;
    wchar_t c;
    while (pData < end)
    {
        if (incr == 1)
            c = *pData;
        else
            c = *(wchar_t*)pData;
        if (!c)
            break;
        if (c > 127 || (!isxdigit(c) && !isspace(c)))
            return false;
        pData += incr;
    }
    return true;
}

thString HexWnd::FormatPaste(const char* pData, size_t newSize, UINT srcFormat, int dstFormat)
{
    thString retval;

    //! need better IDs
    if (dstFormat == PasteFormatDialog::IDC_AUTO)
    {
        const DisplayPane &pane = m_pane[m_iCurPane];
        if (pane.IsHex() && CanConvertAsHex(pData, newSize, srcFormat))
            dstFormat = PasteFormatDialog::IDC_HEX;
        else if (pane.id == DisplayPane::ANSI)
            dstFormat = PasteFormatDialog::IDC_ASCII;
        else if (pane.id == DisplayPane::ID_UNICODE)
            dstFormat = PasteFormatDialog::IDC_UTF16;
        else if (srcFormat == CF_TEXT)
            dstFormat = PasteFormatDialog::IDC_ASCII;
        else if (srcFormat == CF_UNICODETEXT)
            dstFormat = PasteFormatDialog::IDC_UTF16;
        else
            dstFormat = PasteFormatDialog::IDC_RAW;
    }

    if (dstFormat != PasteFormatDialog::IDC_RAW)
    {
        // get string length and ensure we have terminator.
        size_t tmpSize;
        if (srcFormat == CF_TEXT)
        {
            if (SUCCEEDED(StringCchLengthA((char*)pData, newSize, &tmpSize)))
                newSize = tmpSize;
            else if (newSize > 0)
                newSize--;
            else
                return retval;
        }
        else if (srcFormat == CF_UNICODETEXT)
        {
            newSize /= 2;
            if (SUCCEEDED(StringCchLengthW((wchar_t*)pData, newSize, &tmpSize)))
                newSize = tmpSize;
            else if (newSize > 0)
                newSize--;
            else
                return retval;
            newSize *= 2;
        }
    }

    switch (dstFormat)
    {
    case PasteFormatDialog::IDC_ESCAPED:
        return PyString_DecodeEscape(pData, newSize, NULL,
           srcFormat == CF_TEXT ? 0 : 1, NULL);
        break;
    case PasteFormatDialog::IDC_HEX:
        if (srcFormat == CF_TEXT)
            return unhexlify(wxString(pData, wxConvLibc, newSize));
        else if (srcFormat == CF_UNICODETEXT)
            return unhexlify(wxString((wchar_t*)pData, wxConvLibc, newSize / 2));
        break;
    case PasteFormatDialog::IDC_RAW:
        return thString((uint8*)pData, newSize);  // Return unchanged.
    case PasteFormatDialog::IDC_ASCII:
        if (srcFormat == CF_TEXT)
            return thString::ToANSI((char*)pData, newSize);
        else if (srcFormat == CF_UNICODETEXT)
            return thString::ToANSI((wchar_t*)pData, newSize / 2);
        break;
    case PasteFormatDialog::IDC_UTF16:
        if (srcFormat == CF_TEXT)
            return thString::ToUnicode((char*)pData, newSize);
        else if (srcFormat == CF_UNICODETEXT)  // Return unchanged.
            return thString::ToUnicode((wchar_t*)pData, newSize / 2);
        break;
    }
    return retval;
}

void HexWnd::PasteWithFormat(int userFormat /*= -1*/)
{
    thString tmpData;
    HANDLE hglb = NULL;
    uint8* pData;

    if (!OpenClipboard(GetHwnd()))
        return;

    // Enumerate formats in order of preference until we find one we can read.
    // Don't use GetPriorityClipboardFormat(), because we want the first format
    // posted by the other app, Unicode or not.
    UINT srcFormat = 0;
    do {
        srcFormat = EnumClipboardFormats(srcFormat);
    } while (srcFormat != CF_UNICODETEXT &&
             srcFormat != CF_TEXT &&
             srcFormat != CF_DIB &&
             srcFormat != 0);

    //! test code for block select  2007-12-19
    //UINT cf_MSDEVColumnSelect = RegisterClipboardFormat("MSDEVColumnSelect");
    //if (IsClipboardFormatAvailable(cf_MSDEVColumnSelect))
    //   srcFormat = cf_MSDEVColumnSelect;
    // Visual Studio posts 1 byte of dummy data.  Notepad2 (Scintilla) posts (HGLOBAL)NULL.
    // I guess you just check whether the format is available, and then read lines of normal text.

    if (userFormat < 0)
    {
        PasteFormatDialog dlg(this);
        if (dlg.ShowModal() == wxID_CANCEL)
            goto error;
        userFormat = dlg.GetSelection();
    }

    if (srcFormat == 0) //! There's nothing here we can read.  Should we try raw data?
    {
        // try raw data.
        srcFormat = EnumClipboardFormats(0);

        if (srcFormat == 0)
            goto error;
    }

    // GetClipboardData doesn't always return a handle to a memory object.
    // In the case of CF_BITMAP you get an HBITMAP instead.
    //! What do we do about this?

    hglb = GetClipboardData(srcFormat);
    if (hglb == NULL)
        goto error;

    pData = (uint8*)GlobalLock(hglb);
    if (pData == NULL)
        goto error;

    tmpData = FormatPaste((char*)pData, GlobalSize(hglb), srcFormat, userFormat);
    DoPaste(tmpData);

error:
    DWORD err = GetLastError();
    if (hglb) GlobalUnlock(hglb);
    CloseClipboard();
}

void HexWnd::CmdPaste(wxCommandEvent &event)
{
    if (!IsClipboardFormatAvailable(frame->clipboard.m_nFormat))
    {
        PasteWithFormat(PasteFormatDialog::IDC_AUTO);
        return;
    }

    //! todo: get data from other T.Hex process, if necessary
    wxString data;
    if (frame->clipboard.GetData(data))
    {
        SerialData sdata(data);
        if (sdata.Ok())
            DoPaste(thString(), &sdata);
    }
}

void HexWnd::CmdPasteAs(wxCommandEvent &event)
{
    PasteWithFormat(-1);
}

bool HexWnd::DoPaste(thString byteData, SerialData *psData)
{
    uint64 iSelStart, iSelEnd, nOldSize;
    GetSelection(iSelStart, iSelEnd);
    THSIZE newSize = (psData ? psData->m_nTotalSize : byteData.len());
    //Selection sel;

    if (appSettings.bInsertMode)
        nOldSize = iSelEnd - iSelStart;
    else
        nOldSize = newSize;

    if (!doc->CanReplaceAt(iSelStart, nOldSize, newSize))
        goto end;

    if (psData)
        doc->ReplaceSerialized(iSelStart, nOldSize, *psData);
    else
        doc->ReplaceAt(iSelStart, nOldSize, byteData.data(), byteData.len());
    doc->MarkModified(ByteRange::fromSize(iSelStart, newSize));

    if (s.bSelectOnPaste)
        CmdSetSelection(iSelStart, iSelStart + newSize);
    else
        CmdMoveCursor(iSelStart + newSize);

    return true;
end:
    MessageBeep((DWORD)-1);
    return false;
}

void HexWnd::CmdCut(wxCommandEvent &event)
{
    CmdCopy(event);
    CmdDelete(event);
}

void HexWnd::CmdDelete(wxCommandEvent &event)
{
    uint64 iSelStart, iSelEnd;
    GetSelection(iSelStart, iSelEnd);
    uint64 size = 1;
    if (iSelEnd != iSelStart)
        size = iSelEnd - iSelStart;

    if (appSettings.bInsertMode)
        doc->RemoveAt(iSelStart, size);
    else {
        uint8 zero = 0;
        doc->ReplaceAt(iSelStart, size, &zero, 1, size); //! REALLY NEED FILL DATA
    }
    CmdMoveCursor(iSelStart);
}

void HexWnd::CmdSelectModeNormal() { }
void HexWnd::CmdSelectModeBlock() { }
void HexWnd::CmdSelectModeToggle() { }
void HexWnd::CmdNextView(wxCommandEvent &event)
{
    for (int i = 0; i < 10; i++)
    {
        int pane = (m_iCurPane + i + 1) % 10;
        if (m_pane[pane].id && m_pane[pane].id != DisplayPane::ADDRESS)
        {
            CmdSetSelection(m_iSelStart, m_iCurByte, pane, 0, JV_SHORTEST | JH_AUTO);
            return;
        }
    }
}

void HexWnd::CmdPrevView(wxCommandEvent &event)
{
    for (int i = 0; i < 10; i++)
    {
        int pane = (m_iCurPane + 9 - i) % 10;
        if (m_pane[pane].id && m_pane[pane].id != DisplayPane::ADDRESS)
        {
            CmdSetSelection(m_iSelStart, m_iCurByte, pane);
            return;
        }
    }
}

static const int TT_fontPointSizes[] = {1,2,3,4,5,6,7,8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
int fontCount;
int *fontSizes;
int CALLBACK EnumFontFamExProc(
  ENUMLOGFONTEX *lpelfe,    // logical-font data
  NEWTEXTMETRICEX *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
)
{
    LOGFONT &lf = lpelfe->elfLogFont;
    if (lParam)
        fontSizes[fontCount] = lf.lfHeight;
    else
        PRINTF(_T("%d: %s, %s, %s, %dx%d\n"), FontType,
        lpelfe->elfFullName, lpelfe->elfStyle, lpelfe->elfScript, lf.lfWidth, lf.lfHeight);
    fontCount++;
    return 1;
}

int GetNextMultiple(int start, int *bases, int count)
{
    int mindiff = -1, next = start;
    for (int i = 0; i < count; i++)
    {
        int tmp = start - (start % bases[i]) + bases[i];
        if (mindiff == -1 || tmp - start < mindiff)
        {
            mindiff = tmp - start;
            next = tmp;
        }
    }
    return next;
}

int GetPrevMultiple(int start, int *bases, int count)
{
    int mindiff = -1, prev = start;
    for (int i = 0; i < count; i++)
    {
        int tmp = start - 1 - ((start - 1) % bases[i]);
        if (mindiff == -1 || start - tmp < mindiff)
        {
            mindiff = start - tmp;
            prev = tmp;
        }
    }
    return prev;
}

void HexWnd::CmdIncreaseFontSize(wxCommandEvent &event)
{
    LOGFONT lf;
    GetObject((HFONT)GetFont().GetHFONT(), sizeof(lf), &lf);
    int height, mult = lf.lfHeight < 0 ? -1 : 1;
    if (m_tm.tmPitchAndFamily & TMPF_TRUETYPE) // TrueType
    {
        if (m_iDesiredFontSize == -1)
            m_iDesiredFontSize = m_tm.tmHeight - m_tm.tmInternalLeading;

        height = m_iDesiredFontSize;
        if (height < 20)
            height++;
        else
            height = (height * 6) / 5;

        lf.lfHeight = -height;
    }
    else // restrict to available sizes for non-TT fonts
    {
        if (m_iDesiredFontSize == -1)
            m_iDesiredFontSize = m_tm.tmHeight;

        //lf.lfPitchAndFamily = 0; //! why was this here?  2007-09-05
        fontCount = 0;
        fontSizes = NULL;
        HDC hdc = GetDC(NULL);
        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, 0, 0);
        fontSizes = new int[fontCount];
        fontCount = 0;
        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, 1, 0);
        height = GetNextMultiple(m_iDesiredFontSize, fontSizes, fontCount);
        lf.lfHeight = height;
        delete [] fontSizes;
        ReleaseDC(NULL, hdc);

        if (abs(lf.lfHeight) >= 72)
            return;
        //! things get weird beyond here with non-TT fonts.
        // First we just don't get all the steps you'd expect.
        // Then at some point the font mapper gives up and switches to a TT font.
    }
    lf.lfWidth = 0;

    //lf.lfHeight = wxGetNumberFromUser("New font size:", "", "T.Hex", height, -300, 300);

    //PRINTF("Old font size: %d (tm = %d,%d)\n", lf.lfHeight, tm.tmHeight, tm.tmInternalLeading);
    PRINTF(_T("Trying new font size %d\n"), lf.lfHeight);
    
    SetFont(&lf);
    m_iDesiredFontSize = height;
}

void HexWnd::CmdDecreaseFontSize(wxCommandEvent &event)
{
    LOGFONT lf;
    GetObject((HFONT)GetFont().GetHFONT(), sizeof(lf), &lf);
    int height;

    if (m_tm.tmPitchAndFamily & TMPF_TRUETYPE) // TrueType
    {
        if (m_iDesiredFontSize == -1)
            m_iDesiredFontSize = m_tm.tmHeight - m_tm.tmInternalLeading;

        height = m_iDesiredFontSize;
        if (height <= 24)
            height--;
        else
            height = (height * 5) / 6;
        lf.lfHeight = -height;
    }
    else // restrict to available sizes for non-TT fonts
    {
        if (m_iDesiredFontSize == -1)
            m_iDesiredFontSize = m_tm.tmHeight;

        //lf.lfPitchAndFamily = 0; //! why was this here?  2007-09-05
        fontCount = 0;
        fontSizes = NULL;
        HDC hdc = GetDC(NULL);
        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, 0, 0);
        fontSizes = new int[fontCount];
        fontCount = 0;
        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, 1, 0);
        height = GetPrevMultiple(m_iDesiredFontSize, fontSizes, fontCount);
        lf.lfHeight = height;
        delete [] fontSizes;
        ReleaseDC(NULL, hdc);
    }
    lf.lfWidth = 0;

    if (lf.lfHeight == 0)
        return;

    //PRINTF("Old font size: %d (tm = %d,%d)\n", lf.lfHeight, tm.tmHeight, tm.tmInternalLeading);
    PRINTF(_T("Trying new font size %d\n"), lf.lfHeight);
    SetFont(&lf);
    m_iDesiredFontSize = height;
}

void HexWnd::CmdUndo(wxCommandEvent &event)
{
    if (!doc->Undo())
        MessageBeep((DWORD)-1);
}

void HexWnd::CmdRedo(wxCommandEvent &event)
{
    if (!doc->Redo())
        MessageBeep((DWORD)-1);
}

void HexWnd::CmdBackspace(wxCommandEvent &event) // called by user code, not event handler
{
    uint64 iSelStart, iSelEnd;
    GetSelection(iSelStart, iSelEnd);
    if (iSelEnd != iSelStart)
    {
        CmdDelete(event);
        return;
    }

    if (iSelStart == 0)
    {
        MessageBeep((UINT)-1);
        return;
    }

    if (appSettings.bInsertMode)
        doc->RemoveAt(iSelStart - 1, 1);
    else
        doc->ReplaceAt(iSelStart - 1, 0);
    CmdMoveCursor(iSelStart - 1);
}

bool HexWnd::CmdChar(char c)
{
    if (c == '\b')
    {
        wxCommandEvent event;
        CmdBackspace(event);
        return true;
    }

    Selection sel;
    THSIZE nFirst, oldSize = 0, newSize = 1;
    uint8 oldval[8], newval[8];
    DisplayPane &pane = m_pane[m_iCurPane];
    bool ClearWholeWord = false;

    GetSelection(sel);

    //if (c == 'm') //! testing MarkModified
    //{
    //    doc->MarkModified(ByteRange::fromSize(sel.GetFirst(), wxMax(sel.GetSize(), 1)));
    //    SetSelection(sel.GetLast(), sel.GetLast());
    //    return true;
    //}
    //if (c == 'u') //! testing MarkModified
    //{
    //    doc->MarkUnmodified(ByteRange::fromSize(sel.GetFirst(), wxMax(sel.GetSize(), 1)));
    //    SetSelection(sel.GetLast(), sel.GetLast());
    //    return true;
    //}

    if (appSettings.bInsertMode && m_iCurDigit == 0)
    {
        sel.Get(nFirst, oldSize);
        oldval[0] = 0;
    }
    else
    {
        nFirst = m_iCurByte;
        oldSize = pane.m_iColBytes;
        oldval[0] = doc->GetAt(nFirst);
    }

    if (pane.id == DisplayPane::NUMERIC && !pane.m_bFloat)
    {
        if (pane.m_iColBytes > 1)
        {
            wxMessageBox(_T("Not yet implemented for more than one byte per column."));
            return false;
        }
        int val;
        if      (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'A' && c <= 'Z') val = c - 'A' + 10;
        else if (c >= 'a' && c <= 'z') val = c - 'a' + 10;
        else if (c == ' ') {
            ClearWholeWord = true;
            val = newval[0] = 0;
        }
        else
           return false; // invalid digit

        if (!ClearWholeWord)
        {
            if (val >= s.iByteBase)
                return false; // invalid digit for current number base

            TCHAR oldbuf[65];
            my_itoa((DWORD)oldval[0], oldbuf, s.iByteBase, m_iByteDigits);
            oldbuf[m_iCurDigit] = c;
            oldbuf[m_iByteDigits] = 0;
            uint64 newval64 = _tcstoui64(oldbuf, NULL, s.iByteBase);
            if (newval64 > 255) // can happen in octal or decimal
                return false; // value too large
            newval[0] = (uint8)newval64;
        }
    }
    else if (m_pane[m_iCurPane].id == DisplayPane::ANSI)
    {
        newval[0] = c;
    }
    else if (m_pane[m_iCurPane].id == DisplayPane::ID_UNICODE)
    {
        newval[0] = c;
        newval[1] = 0;
        newSize = 2;
    }
    else
    {
        // if Unicode mode: have to replace two bytes and take a different data type
        MessageBeep((DWORD)-1); //! can't edit Unicode or binary panes yet
        return false;
    }

    if (!doc->CanReplaceAt(nFirst, oldSize, newSize))
        return false; //! get error from doc

    doc->ReplaceAt(nFirst, oldSize, newval, newSize);
    doc->MarkModified(ByteRange::fromSize(nFirst, newSize));

    if (ClearWholeWord)
    {
        THSIZE byte = NextBytePos(1, nFirst, 0);
        SetSelection(byte, byte);
    }
    else
    {
        int digit = m_iCurDigit;
        THSIZE byte = NextBytePos(1, nFirst, &digit);
        SetSelection(byte, byte, 0, digit);
    }

    return true;
}

void HexWnd::CmdUndoAll(wxCommandEvent &WXUNUSED(event))
{
    //! undo back to beginning of buffer, which may be before last save
}

void HexWnd::CmdRedoAll(wxCommandEvent &WXUNUSED(event))
{
    //! redo everything
}

void HexWnd::CmdToggleEndianMode(wxCommandEvent &event)
{
    s.iEndianMode = 1 - s.iEndianMode;
    Refresh();
    UpdateViews(DataView::DV_ALL);
}

void HexWnd::CmdRelativeAddresses(wxCommandEvent &event)
{
    if (!s.bAbsoluteAddresses)
       return;
    s.bAbsoluteAddresses = false;
    AdjustForNewDataSize();
    UpdateViews(DataView::DV_ALL);
}

void HexWnd::CmdAbsoluteAddresses(wxCommandEvent &event)
{
    if (s.bAbsoluteAddresses)
        return;
    s.bAbsoluteAddresses = true;
    AdjustForNewDataSize();
    UpdateViews(DataView::DV_ALL);
}

void HexWnd::CmdCopyCurrentAddress(wxCommandEvent &event)
{
    TCHAR buf[100];
    FormatAddress(m_iCurByte, buf, 100);

    if (wxTheClipboard->Open())
    {
        // This data objects are held by the clipboard, 
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject(buf) );
        wxTheClipboard->Close();
    }
}

void HexWnd::CmdCopyAddress(wxCommandEvent &event)
{
    TCHAR buf[100];
    FormatAddress(LineColToByte(m_iMouseOverLine, 0), buf, 100);

    if (wxTheClipboard->Open())
    {
        // This data objects are held by the clipboard, 
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject(buf) );
        wxTheClipboard->Close();
    }
}

void HexWnd::CmdViewAdjustColumns(wxCommandEvent &event)
{
   s.bAdjustLineBytes = !s.bAdjustLineBytes;
   AdjustForNewDataSize();
}

void HexWnd::CmdOffsetLeft(wxCommandEvent &event)
{
    if (m_iAddressOffset)
        m_iAddressOffset--;
    else
        m_iAddressOffset = s.iLineBytes - 1;
    AdjustForNewDataSize();
}

void HexWnd::CmdOffsetRight(wxCommandEvent &event)
{
    m_iAddressOffset++;
    if ((int)m_iAddressOffset >= s.iLineBytes)
        m_iAddressOffset = 0;
    AdjustForNewDataSize();
}

//void HexWnd::CmdNewFile(wxCommandEvent &event)
//{
//    OpenBlank();
//}

void HexWnd::CmdViewRuler(wxCommandEvent &event)
{
    s.bShowRuler = !s.bShowRuler;
    frame->GetMenuBar()->Check(IDM_ViewRuler, s.bShowRuler);
    AdjustForNewDataSize();
}

void HexWnd::CmdViewStickyAddr(wxCommandEvent &WXUNUSED(event))
{
   s.bStickyAddr = !s.bStickyAddr;
    frame->GetMenuBar()->Check(IDM_ViewStickyAddr, s.bStickyAddr);
    AdjustForNewDataSize();
}

void HexWnd::CmdReadPalette(wxCommandEvent &event)
{
    //! ? s.palText.Read(doc, GetSelection()) ?
    //! ? s.palText.GetWriteBuf()             ?

    if (0) // testing new palette 2007-10-15
    {
        thPalette &pal = s.palText;
        int r = 0, g = 0, b = 0;
        for (size_t i = 0; i < 256; i++)
        {
            COLORREF rgb = pal.GetColor(i);
            r += GetRValue(rgb);
            g += GetGValue(rgb);
            b += GetBValue(rgb);
        }
        printf("R=%d  G=%d  B=%d\n", r, g, b);
        return;
    }

    RGBQUAD rgb[256];
    int count = 256;
    thPalette pal;
    Selection sel;

    GetSelection(sel);
    if (sel.GetSize() == 768)  // assume a full palette with three bytes per entry
    {
        count = 256;
        memset(rgb, 0, sizeof(rgb));
        for (int i = 0; i < count; i++)
            if (!doc->Read(sel.GetFirst() + i * 3, 3, (uint8*)(rgb + i)))
                return;
    }
    else 
    {
        if (sel.GetSize())
            count = wxMax(sel.GetSize() / 4, 256);
        if (sel.GetFirst() + count * 4 > DocSize())
            count = (DocSize() - sel.GetFirst()) / 4;

        if (!doc->Read(sel.GetFirst(), count * 4, (uint8*)rgb))
            return;
    }
    for (int i = 0; i < count; i++)
    {
        RGBQUAD &c = rgb[i];
        pal.Add(i, RGB(c.rgbRed, c.rgbGreen, c.rgbBlue));
    }
    pal.Realize();
    s.palText = pal;
    Refresh();
}


//void HexWnd::CmdFocusHexWnd(wxCommandEvent &event)
//{
//   SetFocus();
//}

void HexWnd::CmdToggleHexDec(wxCommandEvent &event)
{
    if (s.iAddressBase == 16)
    {
        s.iAddressBase = 10;
        appSettings.sb.SelectionBase =
        appSettings.sb.ValueBase     =
        appSettings.sb.FileSizeBase  =
            BASE_DEC;
    }
    else
    {
        s.iAddressBase = 16;
        appSettings.sb.SelectionBase =
        appSettings.sb.ValueBase     =
        appSettings.sb.FileSizeBase  =
            BASE_HEX;
    }
    AdjustForNewDataSize();
    UpdateViews(DataView::DV_ALL);
}

void HexWnd::CmdCopyFontCodePoints(wxCommandEvent &WXUNUSED(event))
{
    HDC hdc = GetDC(NULL);
    SelectObject(hdc, (HFONT)GetFont().GetHFONT());

    GLYPHSET* pgs;
    DWORD cBytes = GetFontUnicodeRanges(hdc, NULL);
    pgs = (GLYPHSET*)malloc(cBytes);
    GetFontUnicodeRanges(hdc, pgs);
    int charCount = 0;
    for (DWORD range = 0; range < pgs->cRanges; range++)
       charCount += pgs->ranges[range].cGlyphs;

    HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE, (charCount + 1) * sizeof(WCHAR));
    WCHAR *chars = (WCHAR*)GlobalLock(hMem);
    int base = 0;
    for (DWORD range = 0; range < pgs->cRanges; range++)
    {
        const int first = pgs->ranges[range].wcLow, last = first + pgs->ranges[range].cGlyphs - 1;
        for (int c = first; c <= last; c++)
            chars[base++] = c;
    }
    chars[base++] = 0;
    GlobalUnlock(hMem);

    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    }
    else
        GlobalFree(hMem);

    free(pgs);

    ReleaseDC(NULL, hdc);
}

inline uint32 memdiff(const uint8 *a, const uint8 *b, uint32 len)
{
   uint32 count = 0;
   for (uint32 i = 0; i < len; i++)
   {
      if (a[i] != b[i])
         count++;
   }
   return count;
}

void HexWnd::CmdFindDiffRec(wxCommandEvent &(event))
{
    ATimer timer;
    THSIZE start = GetSelection().GetFirst(), len = GetSelection().GetSize();
    if (len == 0)
        len = 1;
    if (len > 1000)
        len = 1000;
    const THSIZE end = DocSize();

    if (len == 1)  // special case because there was way too much overhead.
    {
        uint32 blocksize;
        uint8 target = doc->GetAt(start);
        while (start < end)
        {
            blocksize = wxMin(end - start, MEGA);
            const uint8 *tmp = doc->Load(start, blocksize, &blocksize);
            if (!tmp)
                break;
            for (uint32 i = 0; i < blocksize; i++)
                if (tmp[i] != target)
                {
                    CmdSetSelection(start + i, start + i + 1);
                    return;
                }
            start += blocksize;
        }
        MessageBeep(-1);
        if (frame->GetStatusBar())
            frame->SetStatusText(wxString::Format(_T("Reached end of file in %0.3f seconds"), timer.elapsed()));
        return;
    }

    uint8 *buf[2] = {new uint8[len], new uint8[len]};
    doc->Read(start, (int)len, buf[0]);
    int which = 1;
    //! todo: progress bar

    while (1)
    {
        start += len;
        if (start + len >= end) {
            MessageBeep(-1);
            if (frame->GetStatusBar())
                frame->SetStatusText(wxString::Format(_T("Reached end of file in %0.3f seconds"), timer.elapsed()));
            break;
        }
        if (!doc->Read(start, (int)len, buf[which]))
            break;
        which = 1 - which;
        int diffs = memdiff(buf[0], buf[1], len);
        if (diffs > (len >> 1)) // more than half the bytes changed?
        {
            CmdSetSelection(start, start + len);
            break;
        }
    }
    delete [] buf[0];
    delete [] buf[1];
}

void HexWnd::CmdFindDiffRecBack(wxCommandEvent &(event))
{
    ATimer timer;
    THSIZE start = GetSelection().GetFirst(), len = GetSelection().GetSize();
    if (len == 0)
        len = 1;
    if (len > 1000)
        len = 1000;
    if (start == 0)
        return;
    if (start == DocSize())
        start--;

    if (len == 1)  // special case because there was way too much overhead.
    {
        uint32 blocksize;
        uint8 target = doc->GetAt(start);
        while (start > 0)
        {
            blocksize = wxMin(start, MEGA);
            const uint8 *tmp = doc->Load(start - blocksize, blocksize);
            if (!tmp)
                break;
            for (uint32 i = blocksize; i > 0; i--)
                if (tmp[i - 1] != target)
                {
                    CmdSetSelection(start - blocksize + i, start - blocksize + i - 1);
                    return;
                }
            start -= blocksize;
        }
        MessageBeep(-1);
        if (frame->GetStatusBar())
            frame->SetStatusText(wxString::Format(_T("Reached beginning of file in %0.3f seconds"), timer.elapsed()));
        return;
    }

    uint8 *buf[2] = {new uint8[len], new uint8[len]};
    doc->Read(start, (int)len, buf[0]);
    int which = 1;
    //! todo: progress bar
    while (1)
    {
        if (start < len) {
            MessageBeep(-1);
            if (frame->GetStatusBar())
                frame->SetStatusText(wxString::Format(_T("Reached beginning of file in %0.3f seconds"), timer.elapsed()));
            break;
        }
        start -= len;
        if (!doc->Read(start, (int)len, buf[which]))
            break;
        which = 1 - which;
        if (memdiff(buf[0], buf[1], len) > (len >> 1)) // more than half the bytes changed?
        {
            CmdSetSelection(start, start + len);
            break;
        }
    }
    delete [] buf[0];
    delete [] buf[1];
}

wxArrayString g_asSectors;
#include "datasource.h"
void HexWnd::CmdCopySector(wxCommandEvent &event)
{
    thRecentChoiceDialog dlg(this, _T("Read sector"), _T("Starting address, (0 - ") + FormatHex(DocSize()) + _T(")"), g_asSectors);
    if (dlg.ShowModal() == wxID_CANCEL)
        return;
    THSIZE start;
    if (!ReadUserNumber(g_asSectors[0], start))
    {
        wxMessageBox(_T("Couldn't read number"));
        return;
    }

    uint8 *buf = new uint8[512];
    TCHAR *str = new TCHAR[1024];
    if (!m_pDS->Read(start, 512, buf))
    {
        wxMessageBox(_T("Read failed."));
        return;
    }

    for (int i = 0; i < 512; i++)
        my_itoa((uint32)buf[i], &str[i * 2], 16, 2);

    if (wxTheClipboard->Open())
    {
        // This data objects are held by the clipboard, 
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject(wxString(str, 1024)) );
        wxTheClipboard->Close();
    }
    delete [] str;
    delete [] buf;
}

bool HexWnd::DoFind(const uint8 *data, size_t searchLength, int flags /*= 0*/)
{
    const bool bReverse         = FLAGS(flags, THF_BACKWARD);
    const bool bIgnoreCase      = FLAGS(flags, THF_IGNORECASE);
    const bool bAllRegions      = FLAGS(flags, THF_ALLREGIONS);
    const bool bSelectionOnly   = FLAGS(flags, THF_SELONLY);
    const bool bAgain           = FLAGS(flags, THF_AGAIN);
    const bool bNoWrap          = FLAGS(flags, THF_NOWRAP);

    THSIZE start, end, restart;
    GetSelection(start, end);
    int type = 0; // plain text

    int found = false;
    bool startOver = false;
    //! todo: regular expression search
    //! HexDoc::Find() should test all bytes before end as the first byte of search string
    //! todo: test expression for validity

    if (searchLength == 0)
    {
        wxMessageBox(_T("Search string is empty."), _T("T. Hex"), wxICON_ERROR | wxOK);
        return false;
    }
    //if (searchLength > 65536)  // arbitrary limit
    //{
    //    wxMessageBox("Search string is too long.  (64KB max)", "T. Hex", wxICON_ERROR | wxOK);
    //    return false;
    //}

    if (bSelectionOnly)
    {
        start++;
        found = doc->Find(data, searchLength, type, !bIgnoreCase, start, end);
    }
    else
    {
        if (!bReverse)  // find forward
        {
            if (bAgain)
            {
                // This is based on the behavior of the MS VC++ IDE.
                if (start == end)
                    start++;
                else
                    start = end;
            }
            end = doc->GetSize();
            restart = 0;
        }
        else  // find backward
        {
            //start = Subtract(start, length);
            end = 0;
            restart = Subtract(doc->GetSize(), searchLength);
        }

        if (bAllRegions)
        {
            size_t curdoc = 0;
            const size_t numdocs = m_docs.size();
            while (curdoc < numdocs && m_docs[curdoc] != doc)
               curdoc++;
            for (size_t n = 0; n <= numdocs; n++)  //! forward only
            {
                if (!bReverse)  // find forward
                    doc = m_docs[(curdoc + n) % numdocs];
                else  // find backward
                    doc = m_docs[(curdoc - n + numdocs) % numdocs];
                if (n == numdocs)  // got all the way to the starting point?
                {
                    if (bNoWrap)
                        break;
                    end = start;
                    start = restart;
                }
                else if (n > 0)
                {
                    start = 0;
                    end = doc->GetSize();
                }
                found = doc->Find(data, searchLength, type, !bIgnoreCase, start, end);
                if (found < 0)
                    break;
                if (found > 0) {
                    SetDoc(doc);
                    break;
                }
            }
        }
        else
        {
            found = doc->Find(data, searchLength, type, !bIgnoreCase, start, end);
            if (found == 0 && !bNoWrap)
            {
                end = start;
                start = restart;
                found = doc->Find(data, searchLength, type, !bIgnoreCase, start, end);
                startOver = true;
            }
        }
    }

    if (found > 0)
    {
        CmdSetSelection(start, end);
        if (startOver)
        {
            //! tell user we had to start over
        }
        return true;
    }
    else if (found == 0)
        wxMessageBox(_T("Search string not found."), _T("T. Hex"), wxICON_ERROR | wxOK); //! need a better message
    // else if (found < 0): do nothing.
    return false;
}

void HexWnd::CmdCustomOp1(wxCommandEvent &event)
{
    THSIZE pos = GetSelection().GetFirst(), end = GetSelection().GetLast();
    if (end == pos) {
        wxMessageBox(_T("Select a range of bytes to use this operation."), _T("Add32"));
        return;
    }

    static wxArrayString choices;
    if (choices.GetCount() == 0) {
        choices.Add(_T("00 00 00 00 h"));
        choices.Add(_T("-0xBD"));
    }
    thRecentChoiceDialog dlg(this, _T("Add32"), _T("Enter the offset to add:"), choices);
    if (dlg.ShowModal() == wxID_CANCEL)
        return;
    int delta;
    if (!ReadUserNumber(choices[0], delta))
        return;

    uint32 tmp;
    while (pos < end)
    {
        if (!doc->ReadNumber(pos, 4, &tmp))
            break;
        if (tmp) {
            tmp += delta;
            if (s.iEndianMode != NATIVE_ENDIAN_MODE)
                reverse((uint8*)tmp, 4);
            if (!doc->ReplaceAt(pos, 4, (uint8*)&tmp, 4))
                break;
        }
        pos += 4;
    }
}

void HexWnd::CmdViewPrevRegion(wxCommandEvent &event)
{
    //! This is stupid, half-assed, and probably does more harm than good to T.Hex's integrity.
    int curdoc = -1;
    for (size_t n = 0; n < m_docs.size(); n++)
        if (m_docs[n] == this->doc) {
            curdoc = n;
            break;
        }
    if (curdoc < 0)
        return;

    if (curdoc > 0)
        SetDoc(m_docs[curdoc - 1]);
}

void HexWnd::CmdViewNextRegion(wxCommandEvent &event)
{
    int curdoc = -1;
    for (size_t n = 0; n < m_docs.size(); n++)
        if (m_docs[n] == this->doc) {
            curdoc = n;
            break;
        }
    if (curdoc < 0)
        return;

    if (curdoc + 1 < (int)m_docs.size())
        SetDoc(m_docs[curdoc + 1]);
}
