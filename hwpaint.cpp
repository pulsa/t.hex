#include "precomp.h"
#include "hexwnd.h"
#include "hexdoc.h"


//OnPaint()
//    creates wxMemoryDC
//    paints the unused area  (should use wxDC::Clear)
//    calls PaintRuler
//    calls PaintRect for each region
//    calls BitBlt
//
//PaintRuler does what it says
//
//PaintRect
//    calls PaintLine for each line in the region
//    draws vertical pane separators
//
//PaintLine
//    handles m_lineBuffer, m_pModified
//    calls GetLineInfo, doc->Read
//    calls PaintPaneLine for each pane
//    draws address pane

#define DRAWLINE_ON_SCREEN

//*****************************************************************************
//*****************************************************************************
// Ye olde glorious painting code
//*****************************************************************************
//*****************************************************************************

void HexWnd::OnPaint(wxPaintEvent &event)
{
    wxPaintDC paintDC(this);
    if (!m_ok)
        return;
    m_nPaintLines = 0;
    panesPainted = 0;

    //int x, y, width, height;
    wxRegion updateRegion = GetUpdateRegion();
    //updateRegion.GetBox(x, y, width, height);
    wxRect rcBound = updateRegion.GetBox();

    wxMemoryDC dc;
    //HDC hdc = (HDC)dc.GetHDC();
    wxSize clientSize = GetClientSize();
    wxBitmap bmp(clientSize.x, clientSize.y);
    dc.SelectObject(bmp);
    dc.SetFont(m_font);

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(m_hbrWndBack);
    dc.DrawRectangle(rcBound);

#if 0  // Testing Dan's paint code,  2008-05-28.  This looks cool, but it's SLOW!
    wxRect rect = rcBound;
    static wxBrush m_brushDisabled;
    if (!m_brushDisabled.Ok())
    {
        wxMemoryDC memDc;
        wxBitmap bmp(4, 4);
        memDc.SelectObject(bmp);
        //memDc.SetBackground(*wxBLACK_BRUSH);  // bitmaps seem to be black initially;
        //memDc.Clear();                        // can we count on this?
        memDc.SetPen(wxColour(80, 80, 80));
        memDc.DrawLine(0, 0, 5, 5);
        memDc.SelectObject(wxNullBitmap);

        m_brushDisabled = wxBrush(bmp);
        m_brushDisabled.SetStyle(wxSTIPPLE);
    }

    // limit scope of selectBrush object...
    {
        SelectInHDC selectBrush((HDC)dc.GetHDC(), GetHbrushOf(m_brushDisabled));

        // ROP for "dest |= pattern" operation -- as it doesn't have a standard
        // name, give it our own
        static const DWORD PATTERNPAINT = 0xFA0089UL;

        ::PatBlt((HDC)dc.GetHDC(), rect.x, rect.y - m_iYOffset, rect.x + rect.width, rect.y + rect.height, PATTERNPAINT);
        //! Why does time spent here get counted as wxRegion::Subtract() or wxDCBase::DrawLine()?
    }
#endif

    for (int n = 0; n < 10; n++)
    {
        DisplayPane &pane = m_pane[n];
        if (!pane.id)
            continue;
        if (pane.IsAddress() && s.bStickyAddr)
            continue;
        int left = pane.m_left - m_iScrollX;
        if (!::RangeOverlapSizeI(left, pane.m_width2, rcBound.x, rcBound.width))
            continue;
        dc.SetBrush(pane.m_hbrBack);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(left, rcBound.y, pane.m_width2, rcBound.height);

#ifdef DRAWLINE_ON_SCREEN
        dc.SetPen(m_penGrid);
        int line = pane.GetRight() - m_iScrollX;
        dc.DrawLine(line, rcBound.y, line, rcBound.y + rcBound.height);
#endif
    }

    // If update region contains empty space at right side, remove that.
    // Don't have to scroll because (m_iLineWidth - m_iScrollX > client_width) always.
    // If we are scrolled, there is no empty space.
    if (m_iScrollX == 0 && rcBound.x + rcBound.width >= m_iLineWidth)
    {
        updateRegion.Subtract(wxRect(m_iLineWidth, rcBound.y,
            rcBound.x + rcBound.width - m_iLineWidth, rcBound.height));
    }

    // If update region contains ruler, shrink the region and paint it later.
    if (s.bShowRuler && rcBound.y < m_iRulerHeight)
        updateRegion.Subtract(0, 0, m_iPaintWidth, m_iRulerHeight); // remove ruler from update region
    // Same goes for address pane.
    if (s.bStickyAddr && rcBound.x < m_iAddrPaneWidth)
        updateRegion.Subtract(0, 0, m_iAddrPaneWidth, rcBound.y + rcBound.height);

    // make sure we have a big enough data buffer
    int pageBytes = s.iLineBytes * (m_iVisibleLines + 2);  // can have half a line at the top & bottom
    if (pageBytes > m_iLineBufferSize)
    {
        m_iLineBytes = s.iLineBytes;
        delete [] m_lineBuffer;
        delete [] m_pModified;
        m_lineBuffer = new uint8[pageBytes];
        m_pModified  = new uint8[pageBytes];
        m_iLineBufferSize = pageBytes;
    }

    // figure out what bytes we will need to read
    int datatop = rcBound.y;
    int dataheight = rcBound.height;
    if (s.bShowRuler && datatop < m_iRulerHeight)
    {
        dataheight -= (m_iRulerHeight - datatop);
        datatop = m_iRulerHeight;
    }
    THSIZE start_line = YToLine(datatop);
    THSIZE end_line = YToLine(datatop + dataheight - 1);
    THSIZE firstVisibleByte = LineColToByte(m_iFirstLine, 0);
    THSIZE firstUpdateByte = LineColToByte(start_line, 0);
    THSIZE lastVisibleByte = LineColToByte(end_line, s.iLineBytes);
    int count = wxMin(doc->size, lastVisibleByte) - firstUpdateByte;
    if ((int)(firstUpdateByte - firstVisibleByte) < 0 || firstUpdateByte - firstVisibleByte + count > pageBytes)
        count = count; //! breakpoint
    if (end_line >= m_iTotalLines)
        end_line = m_iTotalLines - 1;

    // ... and read from the data source
    doc->Read(firstUpdateByte, count,
        m_lineBuffer + firstUpdateByte - firstVisibleByte,
        m_pModified + firstUpdateByte - firstVisibleByte);

    if (m_iCurByte != m_iSelStart)
        PaintSelection(dc);

    while (!updateRegion.IsEmpty())
    {
        wxRegionIterator upd(updateRegion);
        updateRegion.Subtract(PaintRect(dc, upd.GetRect()));
    }

    // paint address pane
    if (s.bStickyAddr && rcBound.x < m_iAddrPaneWidth)
    {
        DisplayPane &pane = m_pane[0];
        dc.SetBrush(pane.m_hbrBack);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, rcBound.y, pane.m_width2, rcBound.height);
        PaintPane(dc, 0, start_line, end_line, rcBound);
        dc.SetPen(m_penGrid);
        dc.DrawLine(pane.m_width2, rcBound.y, pane.m_width2, rcBound.y + rcBound.height);
    }

    if (s.bShowRuler && rcBound.y < m_iRulerHeight)
        PaintRuler(dc, rcBound);

    //BitBlt(hdc, x, y, width, height, (HDC)memDC.GetHDC(), x, y, SRCCOPY);
    paintDC.Blit(rcBound.x, rcBound.y, rcBound.width, rcBound.height, &dc, rcBound.x, rcBound.y);
}

wxRect HexWnd::PaintRect(wxDC &memDC, const wxRect &rcPaint)
{
    wxRect paintRect = rcPaint;

    int datatop = rcPaint.y;
    int dataheight = rcPaint.height;
    if (s.bShowRuler && datatop < m_iRulerHeight)
    {
        dataheight -= (m_iRulerHeight - datatop);
        datatop = m_iRulerHeight;
    }
    THSIZE start_line = YToLine(datatop);
    if (start_line > m_iTotalLines)
        return paintRect;
    THSIZE end_line = YToLine(rcPaint.y + rcPaint.height - 1);
    if (end_line >= m_iTotalLines)
        end_line = m_iTotalLines - 1;
    int line_y = LineToY(start_line);

    //paintRect.y = line_y;
    //paintRect.height = LineToY(end_line + 1) - 1 - line_y;

    //PRINTF("PaintRect %3d,%3d     %3dx%3d  Line %I64X\n", x, y, width, height, start_line);
    //PRINTF("PaintRect Lines %I64X-%I64X,  horz %d-%d\n", start_line, end_line, x, x+width);

    for (int n = 0; n < 10; n++)
    {
        DisplayPane &pane = m_pane[n];
        if (!pane.id)
            continue;
        if (pane.IsAddress() && s.bStickyAddr)
            continue;  // We will paint it later on top of other panes
        if (!::RangeOverlapSizeI(pane.m_left - m_iScrollX, pane.m_width2 + 1, rcPaint.x, rcPaint.width))
            continue;
        panesPainted |= (1 << n);
        PaintPane(memDC, n, start_line, end_line, paintRect);
#ifndef DRAWLINE_ON_SCREEN
        memDC.SetPen(m_penGrid);
        int line = pane.GetRight();
        if (n > 0 || !s.bStickyAddr)
            line -= m_iScrollX;
        //memDC.DrawLine(line, y, line, height);
        memDC.DrawLine(line, rcPaint.y, line, rcPaint.y + rcPaint.height);
#endif
    }

    return paintRect;
}

void HexWnd::PaintPane(wxDC &dc, int nPane, const THSIZE &firstLine, const THSIZE &lastLine, const wxRect &rcPaint)
{
    DisplayPane &pane = m_pane[nPane];
    int pane_left = GetPaneLeft(pane);
    int left = wxMax(rcPaint.x, pane_left);
    int right = wxMin(rcPaint.x + rcPaint.width, pane_left + pane.m_width2);
    if (right < left)
        return;

    int top = LineToY(firstLine);
    THSIZE address;
    int bytes, start;
    if (!GetLineInfo(firstLine, address, bytes, start))
        return;

    THSIZE line = firstLine;
    int offset = (line - m_iFirstLine) * s.iLineBytes + start;
    PaintPaneLine(dc, line, nPane, offset, bytes, start, address, top);

    if (line < lastLine)
    {
        top += m_iLineHeight;
        address += bytes;
        offset += bytes;
        bytes = s.iLineBytes;
        line++;

        while (line < lastLine)
        {
            PaintPaneLine(dc, line, nPane, offset, bytes, start, address, top);
            top += m_iLineHeight;
            address += bytes;
            offset += bytes;
            line++;
        }

        GetLineInfo(line, address, bytes, start);
        PaintPaneLine(dc, line, nPane, offset, bytes, start, address, top);
    }
}

#if 0 // no longer used
void HexWnd::PaintLine(uint64 line, wxDC &dc, int start_x, int width, int top)
{
    HDC hdc = (HDC)dc.GetHDC();
    int byteCount, byteStart;
    uint64 address;
    m_nPaintLines++;

    if (s.iLineBytes != m_iLineBytes)
    {
        m_iLineBytes = s.iLineBytes;
        delete [] m_lineBuffer;
        delete [] m_pModified;
        m_lineBuffer = new uint8[m_iLineBytes];
        m_pModified  = new uint8[m_iLineBytes];
    }

    uint8 *pData = m_lineBuffer;
    bool bLineValid = GetLineInfo(line, address, byteCount, byteStart);

    if (PrintPaintMessages())
        line = line; //! breakpoint

    if (s.bGridLines && address <= DocSize())
    {
        dc.SetPen(m_penGrid);
        dc.DrawLine(0, top + m_iLineHeight - 1, m_iPaintWidth, top + m_iLineHeight - 1);
    }

    if (!bLineValid)
        return;

    if (!doc->Read(address, byteCount, m_lineBuffer, m_pModified))
        pData = NULL;

    SelectObject(hdc, (HFONT)GetFont().GetHFONT());
    const int pad = s.iPanePad, pad2 = 2 * s.iPanePad;

    for (int nPane = 0; nPane < 10; nPane++)
    {
        DisplayPane &pane = m_pane[nPane];
        if (!pane.id)
            continue;
        //if (RangeOverlapSize(start_x, width, m_pane[nPane].m_start - pad, m_pane[nPane].m_width + pad2))
        // That check causes problems in panes outside the auto-panning window.  2007-08-22

        //if (RangeOverlapSize(m_iScrollX, m_iPaintWidth, m_pane[nPane].m_start - pad, m_pane[nPane].m_width + pad2))
            PaintPaneLine(dc, line, nPane, pData, byteCount, byteStart, address, top);
    }
}
#endif // 0

void HexWnd::PaintRuler(wxDC &dc, const wxRect &rcPaint)
{
    TCHAR buf[65];

    // draw background rectangle
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(m_hbrAdrBack);
    dc.DrawRectangle(0, 0, m_iPaintWidth, m_iRulerHeight);

    // draw bottom and right edges with grid color
    dc.SetPen(m_penGrid);
    int width, left = m_iAddrPaneWidth, paint_right = rcPaint.GetRight();
    if (!s.bStickyAddr)
        left -= m_iScrollX;
    dc.DrawLine(left, m_iRulerHeight - 1, m_iLineWidth - 1, m_iRulerHeight - 1); // bottom
    dc.DrawLine(m_iLineWidth - 1,      0, m_iLineWidth - 1, m_iRulerHeight);     // right

    dc.SetTextForeground(s.clrAdr);
    dc.SetBackgroundMode(wxTRANSPARENT);
    dc.SetFont(m_font);

    for (int nPane = 1; nPane < 10; nPane++) // skip the address pane
    {
        DisplayPane &pane = m_pane[nPane];
        if (!pane.id)
            continue;
        if (!(panesPainted & (1 << nPane)))
            continue;

        int col = m_iAddressOffset;

        for (int byte = 0; byte < s.iLineBytes / pane.m_iColBytes; byte++)
        {
            left = pane.GetRect(byte, 1, width, 0) - m_iScrollX;

            if (left >= paint_right) {
                nPane = 99;
                break;
            }

            if (col % s.iAddressBase == 0 && s.iLineBytes > s.iAddressBase)
            {
                int digits = CountDigits(s.iLineBytes - 1, s.iAddressBase);
                my_itoa((uint32)col, buf, s.iAddressBase, digits);
                for (int i = 0; i < digits; i++)
                    dc.DrawText(wxString(buf[i]), left + (width - width_hex) / 2, i * m_tm.tmHeight);
            }
            else
            {
                my_itoa((uint32)col % s.iLineBytes, buf, s.iAddressBase, 1);
                dc.DrawText(wxString(buf[0]), left + (width - width_hex) / 2, m_iRulerHeight - m_tm.tmHeight);
            }
            col += pane.m_iColBytes;
        }
    }

    if (s.bStickyAddr && rcPaint.x < m_iAddrPaneWidth)
    {
        // patch up background rect above sticky address pane
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(m_hbrAdrBack);
        dc.DrawRectangle(0, 0, m_iAddrPaneWidth, m_iRulerHeight);
    }
}

void HexWnd::PaintPaneLine(wxDC &dc, uint64 line, int nPane, size_t offset,
                           int byteCount, int byteStart, THSIZE address, int top /*= 0*/)
{
    uint64 iSelStart, iSelEnd;
    GetSelection(iSelStart, iSelEnd);
    int x, y = s.iExtraLineSpace / 2;
    wxRect rc;
    const uint8 *pData = m_lineBuffer + offset;
    const uint8 *pMod = m_pModified + offset;

    if (line > m_iTotalLines)
        return;

    //PRINTF("PaintPaneLine L%I64X  p%d  b%d  a%I64X  t%d\n", line, nPane, byteStart, address, top);

    thPaletteData *palText = s.palText.GetRealPaletteData();
    thPaletteData *palModText = s.palModText.GetRealPaletteData();
    thPaletteData *palSelText = s.palSelText.GetRealPaletteData();

    DisplayPane &pane = m_pane[nPane];

    rc.x = pane.m_left - m_iScrollX;
    rc.width = pane.m_width2;
    rc.y = 0;

    //COLORREF backClr = s.GetBackgroundColour();
    rc.height = m_tm.tmHeight + s.iExtraLineSpace;

    int selStartCol = 0, selEndCol = 0;
    int col = 0;
    THSIZE orig_address = address;

    if (pane.id == DisplayPane::ADDRESS)
    {
        dc.SetTextForeground(s.clrAdr);
        dc.SetBackgroundMode(wxTRANSPARENT);  //! called too often

        int x = s.bStickyAddr ? 0 : -m_iScrollX;

        TCHAR buf[100]; //! hard-coded buffer size -- ensure m_iAddressChars is reasonable

        if (m_bMousePosValid && !m_iYOffset && line == m_iMouseOverLine && m_iMouseOverRegion >= 1)
        {
            address = m_iMouseOverByte;

            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(*wxWHITE_PEN);
            dc.SetLogicalFunction(wxINVERT);
            dc.DrawRectangle(wxRect(GetPaneLeft(pane) + s.iPanePad / 2, top, pane.m_width1 + s.iPanePad, m_iLineHeight));
            dc.SetLogicalFunction(wxCOPY);
        }

        wxString text(buf, FormatAddress(address, buf, 100));
        if (s.bFakeMonoSpace)
            CenterTextOut(dc, x + s.iPanePad, top + y, text, width_hex);
        else
            dc.DrawText(text, x + s.iPanePad, top + y);

        return;
    }

    int leftedge = m_iScrollX + (s.bStickyAddr ? m_iAddrPaneWidth : 0);
    if (leftedge > pane.m_start1)
    {
        // Another case where if iLineBytes is small, painting is fast anyway, and if it's large,
        // then we stand to gain much by skipping bytes; so we do the extra work here.
        int digit, half;
        col = pane.HitTest(leftedge, digit, half);
        address += col * pane.m_iColBytes;
    }

    for (col; col < byteCount / pane.m_iColBytes; col++, address += pane.m_iColBytes)
    {
        uint8 val = 0xCD;
        int width, etoFlags = 0;
        if (m_bFontCharsOnly)
           etoFlags = ETO_IGNORELANGUAGE;  // We can handle this.
        x = pane.GetRect(col + byteStart / pane.m_iColBytes, 1, width, 0) - m_iScrollX;
        int charCount;
        TCHAR buf[65]; //! how big can this get?  64-bit binary, long double...
        int lead = 0;

        if (x > m_iPaintWidth)
            break;

        if (pData == NULL)
        {
            if (pane.id == DisplayPane::NUMERIC)
            {
                charCount = pane.m_iColChars;
                memset(buf, '-', charCount);
            }
            else
            {
                charCount = 1;
                buf[0] = ' ';
                buf[1] = 0;
            }
            lead = iLeftLead[(uint8)buf[0]];
        }

        //! these really should be one or more separate functions
        else if (pane.id == DisplayPane::HEX)
        {
            val = pData[col];
            my_itoa((uint32)val, buf, s.iByteBase, m_iByteDigits);
            charCount = m_iByteDigits;
            lead = iLeftLead[(uint8)buf[0]];
        }
        else if (pane.id == DisplayPane::ANSI)
        {
            charCount = 1;
            uint16 val16 = charMap256[val = pData[col]];
            buf[0] = val16;
            lead = iLeftLead[val16];
        }
        else if (pane.id == DisplayPane::ID_UNICODE)
        {
            //! todo: draw real character
            uint8 *buf8 = (uint8*)buf;
            if (s.iEndianMode == NATIVE_ENDIAN_MODE)
            {
                buf8[0] = pData[col * 2];
                buf8[1] = pData[col * 2 + 1];
            }
            else
            {
                buf8[0] = pData[col * 2 + 1];
                buf8[1] = pData[col * 2];
            }
            uint16 val16 = *(uint16*)buf;
            val = val16 >> 8;
            if (val16 == 0 && s.bDrawNulAsSpace)
                buf[0] = ' ', lead = 0;
            else if (m_bFontCharsOnly && !iCharWidths[val16])
            {
                //! How do we tell if the system can really render this character?
                *(uint16*)buf = val16 = m_iDefaultChar;
                lead = iLeftLead[val16];
            }
            else
                lead = iLeftLead[val16];
            //charCount = 1;
            // fix left leading space
            //lead = iLeftLead[val16];
        }
        else if (pane.id == DisplayPane::BIN)
        {
            val = pData[col];
            my_itoa((uint32)val, buf, 2, 8);
            charCount = 8;
            lead = iLeftLead[(uint8)buf[0]];
        }
        else if (pane.id == DisplayPane::NUMERIC && pane.m_bFloat)
        {
            //! make sure text buffer is big enough
            //! todo: give user option to set min/max for float and non-float colors

            if (pane.m_iColBytes == 4)
            {
                float fval;
                //Copy4(&fval, pData + col * 4, pane.m_iEndianness != NATIVE_ENDIAN_MODE);
                Copy4(&fval, pData + col * 4, s.iEndianMode != NATIVE_ENDIAN_MODE);
                charCount = my_ftoa(fval, buf, 7);

                // extract exponent
                uint32 raw = *(uint32*)&fval;
                val = (uint8)(raw >> 23);
            }
            else if (pane.m_iColBytes == 8)
            {
                double dval;
                //Copy8(&dval, pData + col * 8, pane.m_iEndianness != NATIVE_ENDIAN_MODE);
                Copy8(&dval, pData + col * 8, s.iEndianMode != NATIVE_ENDIAN_MODE);
                charCount = my_dtoa(dval, buf, 7);

                // extract exponent
                uint64 raw = *(uint64*)&dval;
                val = (uint8)(raw >> 54);
            }
            else // Houston, we have a problem...
            {
            }

            //charCount = pane.m_iColChars;
            //charCount = strlen(buf);
            lead = 0; //!
        }
        else if (pane.id == DisplayPane::NUMERIC) // ... and not bFloat
        {
            // Ooh, boy.  Here we go.
            uint64 tmp = 0;
            memcpy(&tmp, pData + col * pane.m_iColBytes, pane.m_iColBytes); //! force intrinsic?
            //! todo: fix for big-endian machines
            if (s.iEndianMode != NATIVE_ENDIAN_MODE)
                reverse((uint8*)&tmp, pane.m_iColBytes);

            //! todo: get better color value
            if (pane.m_iColBytes == 1)// || pane.m_iEndianness == BIGENDIAN_MODE)
                val = pData[col];
            else
            {
                val = tmp >> ((pane.m_iColBytes - 1) * 8);
                if (pane.m_bSigned)
                    val ^= 0x80;
            }

            if (pane.m_bSigned)
            {
                if (tmp > pane.m_iMax)
                {
                    tmp = ((pane.m_iMax + 1) << 1) - tmp;
                    buf[0] = '-';
                }
                else
                    buf[0] = ' ';
                my_itoa(tmp, buf + 1, pane.m_iBase, pane.m_iColChars - 1);
            }
            else
                my_itoa(tmp, buf, pane.m_iBase, pane.m_iColChars);
            charCount = pane.m_iColChars;
            //lead = 0;
            lead = iLeftLead[(uint8)buf[0]];
        }
        #ifdef LC1VECMEM
        else if (pane.id == DisplayPane::VECMEM)
        {
            *(T_VecMemDec*)buf = DecodeVecMem(pData - ((((int)address % 80) >> 4) << 4), (int)address % 80);
            val = 128; // color everything medium
            charCount = 4;
            lead = iLeftLead[(uint8)buf[0]];
        }
        #endif

        if (col >= selStartCol && col < selEndCol)
        {
            dc.SetTextForeground(palSelText->GetColor(val));
            dc.SetBackgroundMode(wxTRANSPARENT);  //! called too often
        }
        else if (s.bHighlightModified && m_pModified && pMod[col * pane.m_iColBytes])
        {
            //SetTextColor(hdc, s.clrModText[val]);
            dc.SetTextForeground(palModText->GetColor(val));
            dc.SetTextBackground(s.clrModBack);
            dc.SetBackgroundMode(wxSOLID);  //! called too often
            etoFlags |= ETO_OPAQUE;
        }
        else
        {
            if (s.bEvenOddColors)
            {
                dc.SetTextForeground(s.clrEOText[address & 1]);
                dc.SetTextBackground(s.clrEOBack[address & 1]);
                dc.SetBackgroundMode(wxSOLID);  //! called too often
                etoFlags |= ETO_OPAQUE;
            }
            else
            {
                dc.SetTextForeground(palText->GetColor(val));
                dc.SetBackgroundMode(wxTRANSPARENT);  //! called too often
            }
        }

        RECT rcText = { x, top + y, x + width, top + y + m_iLineHeight };
        rcText.bottom -= s.iExtraLineSpace; //!
        rcText.bottom -= s.bGridLines ? 1 : 0; //!
        // ExtTextOut() is the fastest way to draw text, and gives us the best control.
        // ExtTextOutW() is one of the few Unicode functions supported natively on Win95.
        if (pane.id == DisplayPane::ID_UNICODE || pane.id == DisplayPane::ANSI)
        {
            #ifdef WIN32
            if (m_tm.tmCharSet == OEM_CHARSET && pane.id == DisplayPane::ANSI)
                etoFlags |= ETO_GLYPH_INDEX;
            #endif
            if (m_bFixedWidthFont)
                lead += pane.m_extra;
            else if (s.bFakeMonoSpace && iCharWidths[*(uint16*)buf])
                lead += (width - iCharWidths[*(uint16*)buf]) / 2;
            #ifdef WIN32
            ExtTextOutW(hdc, x + lead, top + y, etoFlags, &rcText, (LPCWSTR)buf, 1, NULL);
            #else
            dc.DrawText(wxString(buf[0]), x + lead, top + y);
            #endif
        }
        else
        {
            if (lead)
               lead = lead; //! breakpoint
            if (buf[0] == '0' && buf[1] == 'A')
               lead = lead; //! breakpoint
            if (!m_bFixedWidthFont && s.bFakeMonoSpace)
                CenterTextOut(dc, x + lead, top + y, wxString(buf, charCount), pane.m_iCharWidth);
            else
                dc.DrawText(wxString(buf, charCount), x + lead + pane.m_extra, top + y);
        }
    }

    int colCount = byteCount / pane.m_iColBytes;
    address = orig_address;
    if (m_bMousePosValid && !m_iYOffset && RangeOverlapSize(m_iMouseOverByte, m_iMouseOverCount, address, colCount * pane.m_iColBytes))
    {
        int col = (m_iMouseOverByte - address + byteStart) / pane.m_iColBytes;
        int count = DivideRoundUp(m_iMouseOverCount, pane.m_iColBytes);
        GetColRect(0, col, count, nPane, rc, /*GBR_NOHSCROLL |*/ GBR_VFILL);
        //! can't call InvertRect() with wx, but we shouldn't be using that anyway.
        rc.y += top;
        dc.SetLogicalFunction(wxINVERT);
        if (m_iMouseOverRegion != nPane)
        {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(*wxWHITE_PEN);
        }
        else
            dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(rc);
        dc.SetLogicalFunction(wxCOPY);
    }

    //! I wanted to indicate where the selection would start if you dragged the cursor,
    //  since it's not always obvious.  However, this particular bit of code doesn't work,
    //  because m_iCurByte changes when you start in the second have a column and drag the mouse.
    //else if (m_bMouseSelecting && m_iSelStart == m_iCurByte && m_iCurByte > address && m_iCurByte < address + byteCount)
    //{
    //    int col = (m_iCurByte - address + byteStart) / pane.m_iColBytes;
    //    GetColRect(0, col, 1, nPane, rc, /*GBR_NOHSCROLL |*/ GBR_VFILL);
    //    rc.y += top;
    //    rc.width = 2;
    //    dc.SetLogicalFunction(wxINVERT);
    //    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    //    dc.SetPen(*wxWHITE_PEN);
    //    dc.DrawRectangle(rc);
    //    dc.SetLogicalFunction(wxCOPY);
    //}

    // do highlighting, or whatever this is
    if (RangeOverlapProper(m_highlight.nStart, m_highlight.nEnd, address, address + byteCount))
    {
        //! What we really want here (if we keep this at all) is something like GetByteRect()
        //  that adjusts for columns of more that one byte, but can take GBR_NOHSCROLL | GBR_VFILL.
        // Or maybe this will work.
        THSIZE hlStart = (wxMax(m_highlight.nStart, address) - address) / pane.m_iColBytes;
        THSIZE hlEnd = DivideRoundUp(wxMin(m_highlight.nEnd, address + byteCount) - address,
                                     pane.m_iColBytes);
        wxRect rc;
        GetColRect(0, hlStart, hlEnd - hlStart, nPane, rc, /*GBR_NOHSCROLL |*/ GBR_VFILL);
        rc.y += top;
        //GetByteRect(hlStart, hlEnd - hlStart, nPane, rc);
        dc.SetPen(*wxThePenList->FindOrCreatePen(s.clrHighlight, 1, wxSOLID));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rc);
    }
}

struct SelStuff
{
    THSIZE firstByte, lastByte;
    THSIZE firstLine, lastLine;
    int byteInFirstLine, byteInLastLine;
};

void HexWnd::PaintSelection(wxDC &dc)
{
    THSIZE firstLine, lastLine;
    int firstByte, lastByte;

    THSIZE firstSel, lastSel;
    if (m_iCurByte > m_iSelStart)
    {
        firstSel = m_iSelStart;
        lastSel = m_iCurByte;
    }
    else
    {
        firstSel = m_iCurByte;
        lastSel = m_iSelStart;
    }

    firstLine = ByteToLineCol(firstSel, &firstByte);
    lastLine = ByteToLineCol(lastSel, &lastByte);

    if (!RangeOverlapSize(firstLine, lastLine + 1 - firstLine, m_iFirstLine, m_iVisibleLines + 2))
        return;

    dc.SetBrush(*wxTheBrushList->FindOrCreateBrush(s.clrSelBack));
    dc.SetPen(*wxThePenList->FindOrCreatePen(s.clrSelBorder, 1, wxSOLID));

    for (int n = 1; n < 10; n++) // skip the address pane
    {
        DisplayPane &pane = m_pane[n];
        if (pane.id)
            PaintPaneSelection(dc, pane, firstLine, lastLine, firstByte, lastByte);
    }
}

void HexWnd::PaintPaneSelection(wxDC &dc, DisplayPane &pane, THSIZE firstLine, THSIZE lastLine, int firstByte, int lastByte)
{
#if 0 // bleah
    int width, x;
    int relFirst = wxMax(firstLine, m_iFirstLine) - m_iFirstLine;
    int relLast = wxMin(lastLine, m_iFirstLine + m_iVisibleLines + 1) - m_iFirstLine;
    int flt = m_iFirstLineTop + relFirst * m_iLineHeight;
    int llt = m_iFirstLineTop + relLast * m_iLineHeight;
    if (lastLine >= firstLine + 2)
    {
        x = pane.GetRect(0, pane.m_iCols, width, GBR_VFILL) - m_iScrollX;
        dc.DrawRectangle(x, flt + m_iLineHeight, width, llt - flt - m_iLineHeight);
    }
    if (lastLine == firstLine)
    {
        x = pane.GetRect(firstByte / pane.m_iColBytes, (lastByte - firstByte) / pane.m_iColBytes, width, GBR_VFILL) - m_iScrollX;
        dc.DrawRectangle(x, flt, width, m_iLineHeight);
    }
    else
    {
        x = pane.GetRect(firstByte / pane.m_iColBytes, (s.iLineBytes - firstByte) / pane.m_iColBytes, width, GBR_VFILL) - m_iScrollX;
        dc.DrawRectangle(x, flt, width, m_iLineHeight);
        x = pane.GetRect(0, lastByte / pane.m_iColBytes, width, GBR_VFILL) - m_iScrollX;
        dc.DrawRectangle(x, llt, width, m_iLineHeight);
    }
#else
    wxPoint pt[16];
    int n = 0;
    int x, width;
    const int left = pane.m_start1 - m_iScrollX;

    THSIZE firstSel = wxMin(m_iSelStart, m_iCurByte);
    THSIZE  lastSel = wxMax(m_iSelStart, m_iCurByte);

    if (lastByte == 0)
    {
        lastLine--;
        lastByte = s.iLineBytes;
    }

    // add the top points
    if (firstLine < m_iFirstLine) // selection starts above view
    {
        pt[0].x = left;
        pt[0].y = -1;
        pt[1].x = left + pane.m_width1 - 1;
        pt[1].y = pt[0].y;
        n = 2;
    }
    else if (lastLine > firstLine && firstByte == 0)  // whole first line selected
    {
        pt[0].x = pane.GetRect(0, pane.m_iCols, width, 0) - m_iScrollX;
        pt[0].y = LineToY(firstLine) + 2;
        pt[1].x = pt[0].x + 2;
        pt[1].y = pt[0].y - 2;
        pt[2].x = pt[1].x + width - 5;
        pt[2].y = pt[1].y;
        pt[3].x = pt[2].x + 2;
        pt[3].y = pt[2].y + 2;
        n = 4;
    }
    else  // part of first line selected
    {
        int top = LineToY(firstLine);
        //! not quite the right condition where colBytes > 1
        if (lastSel - firstSel > s.iLineBytes) // overlap with next line?  draw squiggle
        {
            pt[0].x = left;
            pt[0].y = top + m_iLineHeight + 2;
            pt[1].x = pt[0].x + 2;
            pt[1].y = pt[0].y - 2;
            pt[2].x = pane.GetRect(firstByte / pane.m_iColBytes, pane.m_iCols - firstByte / pane.m_iColBytes, width, 0) - 2 - m_iScrollX;
            pt[2].y = top + m_iLineHeight;
            pt[3].x = pt[2].x + 2;
            pt[3].y = pt[2].y - 2;
            pt[4].x = pt[3].x;
            pt[4].y = top + 2;
            pt[5].x = pt[4].x + 2;
            pt[5].y = top;
            pt[6].x = pt[3].x + width - 3;
            pt[6].y = top;
            pt[7].x = pt[6].x + 2;
            pt[7].y = pt[6].y + 2;
            n = 8;
        }
        else // draw complete box in this line
        {
            if (lastLine > firstLine)
                pt[0].x = pane.GetRect(firstByte / pane.m_iColBytes, pane.m_iCols - firstByte / pane.m_iColBytes, width, 0) - m_iScrollX;
            else
                pt[0].x = pane.GetRect(firstByte / pane.m_iColBytes, (lastByte - firstByte) / pane.m_iColBytes, width, 0) - m_iScrollX;
            pt[0].y = top + 2;
            pt[1].x = pt[0].x + 2;
            pt[1].y = top;
            pt[2].x = pt[0].x + width - 3;
            pt[2].y = top;
            pt[3].x = pt[2].x + 2;
            pt[3].y = pt[2].y + 2;
            pt[4].x = pt[3].x;
            pt[4].y = top + m_iLineHeight - 3;
            pt[5].x = pt[4].x - 2;
            pt[5].y = pt[4].y + 2;
            pt[6].x = pt[1].x;
            pt[6].y = top + m_iLineHeight - 1;
            pt[7].x = pt[0].x;
            pt[7].y = pt[4].y;
            dc.DrawPolygon(8, pt);
            if (lastLine > firstLine)  // draw box in next line and be done with it
            {
                top += m_iLineHeight;
                pt[0].x = pane.GetRect(0, lastByte / pane.m_iColBytes, width, 0) - m_iScrollX;
                pt[0].y = top + 2;
                pt[1].x = pt[0].x + 2;
                pt[1].y = pt[0].y - 2;
                pt[2].x = pt[0].x + width - 3;
                pt[2].y = top;
                pt[3].x = pt[2].x + 2;
                pt[3].y = pt[2].y + 2;
                pt[4].x = pt[3].x;
                pt[4].y = top + m_iLineHeight - 3;
                pt[5].x = pt[4].x - 2;
                pt[5].y = pt[4].y + 2;
                pt[6].x = pt[1].x;
                pt[6].y = pt[5].y;
                pt[7].x = pt[0].x;
                pt[7].y = pt[4].y;
                dc.DrawPolygon(8, pt);
            }
            return;
        }
    }

    // now add the bottom points
    if (lastLine > m_iFirstLine + m_iVisibleLines) // selection ends below last line
    {
        pt[n].x = left + pane.m_width1 - 1;
        pt[n].y = LineToY(m_iFirstLine + m_iVisibleLines + 1) + m_iLineHeight;
        pt[n+1].x = left;
        pt[n+1].y = pt[n].y;
        n += 2;
    }
    else if (DivideRoundUp(lastByte, pane.m_iColBytes) == pane.m_iCols) // whole last line
    {
        pt[n].x = left + pane.m_width1 - 1;
        pt[n].y = LineToY(lastLine) + m_iLineHeight - 3;
        pt[n+1].x = pt[n].x - 2;
        pt[n+1].y = pt[n].y + 2;
        pt[n+2].x = left + 2;
        pt[n+2].y = pt[n+1].y;
        pt[n+3].x = left;
        pt[n+3].y = pt[n+2].y - 2;
        n += 4;
    }
    else  // overlap with previous line -- draw squiggle
    {
        pt[n].x = left + pane.m_width1 - 1;
        pt[n].y = LineToY(lastLine) - 2;
        pt[n+1].x = pt[n].x - 2;
        pt[n+1].y = pt[n].y + 2;
        x = pane.GetRect(0, lastByte / pane.m_iColBytes, width, 0) - m_iScrollX;
        pt[n+2].x = x + width + 2;
        pt[n+2].y = pt[n+1].y;
        pt[n+3].x = x + width;
        pt[n+3].y = pt[n+2].y + 2;
        pt[n+4].x = pt[n+3].x;
        pt[n+4].y = pt[n+1].y + m_iLineHeight - 3;
        pt[n+5].x = pt[n+4].x - 2;
        pt[n+5].y = pt[n+4].y + 2;
        pt[n+6].x = left + 2;
        pt[n+6].y = pt[n+5].y;
        pt[n+7].x = left;
        pt[n+7].y = pt[n+6].y - 2;
        n += 8;
    }

    dc.DrawPolygon(n, pt);
#endif
}
