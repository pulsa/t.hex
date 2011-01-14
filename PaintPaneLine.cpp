void HexWnd::PaintPaneLine(wxDC &dc, uint64 line, int nPane, const uint8 *pData,
                           int byteCount, int byteStart, THSIZE address)
{
    //Prof(PaintPaneLine);
    uint64 iSelStart, iSelEnd;
    GetSelection(iSelStart, iSelEnd);
    HDC hdc = (HDC)dc.GetHDC();
    int x, y = s.iExtraLineSpace / 2;
    wxRect rc;

    DisplayPane &pane = m_pane[nPane];

    rc.x = pane.m_left - m_iScrollX;
    rc.width = pane.m_width2;
    rc.y = 0;

    COLORREF backClr = pane.m_hbrBack.GetColour().GetPixel();
    rc.height = m_tm.tmHeight + s.iExtraLineSpace;
    dc.SetBrush(pane.m_hbrBack);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rc);

    if (pane.id == DisplayPane::HEX && s.bGridLines) // draw vertical grid lines
    {
        dc.SetPen(m_penGrid);
        //int x, byte, width;
        //for (byte = 0; byte < pane.m_iCols; byte += pane.m_iColGrouping)
        //{
            //pane.GetRect(byte, pane.m_iColGrouping, x, width, 0);
            //if (byte == 0)
            //    dc.DrawLine(x, 0, x, m_iLineHeight);
            //x += width + pane.m_iGroupSpace / 2;
            //dc.DrawLine(x, 0, x, m_iLineHeight);
        //}
    }

    int selStartCol = 0;
    int selEndCol = 0;

    // is part of this line in the selection?
    if (iSelStart != iSelEnd && iSelStart < address + byteCount && iSelEnd > address)
    {
        uint64 iLineSelStart = max(iSelStart, address);
        int iLineSelBytes = min(iSelEnd, address + byteCount) - iLineSelStart;
        selStartCol = (iLineSelStart - address) / pane.m_iColBytes;
        selEndCol = DivideRoundUp(iLineSelStart - address + iLineSelBytes, pane.m_iColBytes);
        GetColRect(0, selStartCol, selEndCol - selStartCol, nPane, rc, GBR_VFILL /*| GBR_NOHSCROLL*/);
        //rc.x--;
        //rc.width += 2; // 1 pixel past each end of character cell

        //if (s.bGridLines)
        //   rc.height++;

        dc.SetBrush(*wxTheBrushList->FindOrCreateBrush(s.clrSelBack));
        dc.SetPen(*wxTRANSPARENT_PEN);
        if (rc.width > 4 && rc.height > 4) // draw slightly rounded rectangle
        {
            wxRect tmp = rc;
            tmp.x += 1;
            tmp.width -= 2;
            //!tmp.height--;
            dc.DrawRectangle(tmp);
        }
        else
            dc.DrawRectangle(rc);
        dc.SetPen(*wxThePenList->FindOrCreatePen(s.clrSelBorder, 1, wxSOLID));

        int sides = 0;

        // draw top and left border lines
        if (iSelStart >= address) // selection starts in this line
            sides |= wxLEFT | wxTOP;
        else if (iSelStart + s.iLineBytes >= iSelEnd) // starts in previous line, no overlap
            sides |= wxALL;
        else if (iSelStart + s.iLineBytes > address) // starts in previous line, with overlap
        {
            wxRect rc2; // not done with old rect yet, so use a new one
            int prevCol = (iSelStart + s.iLineBytes - address) / pane.m_iColBytes;
            GetColRect(0, 0, prevCol, nPane, rc2, GBR_VFILL | /*GBR_NOHSCROLL |*/ GBR_RSPACE);
            //rc2.x--;
            //rc2.width += 2; // line up with left edge of rectangle in previous line
            if (iSelEnd <= address + byteCount) // selection ends in this line?
            {
                DrawSides(dc, rc2, wxLEFT | wxTOP | wxBOTTOM);
                sides = wxBOTTOM | wxRIGHT;
            }
            else
            {
                DrawSides(dc, rc2, wxLEFT | wxTOP);
                //sides = wxRIGHT;
            }

            rc.width -= rc2.GetRight() - rc.x;
            rc.x = rc2.GetRight();
        }
        else // selection does not start in this or the previous line
            sides |= wxLEFT;

        // draw bottom and right border lines
        if (iSelEnd <= address + byteCount) // selection ends in this line
            sides |= wxRIGHT | wxBOTTOM;
        else if (iSelEnd - s.iLineBytes <= iLineSelStart) // selection ends in next line, without overlap
            sides |= wxALL;
        else if (iSelEnd - s.iLineBytes < address + byteCount) // selection ends in next line, with overlap
        {
            wxRect rc2; // not done with old rect yet, so use a new one
            int nextCol = DivideRoundUp(iSelEnd - s.iLineBytes - address, pane.m_iColBytes);
            int colCount = (byteCount / pane.m_iColBytes) - nextCol;
            GetColRect(0, nextCol, colCount, nPane, rc2, GBR_VFILL /*| GBR_NOHSCROLL*/ | GBR_LSPACE);
            //!rc2.x--; // line up with right edge of rectangle in next line
            //! 1 pixel past end of character cell

            if (iSelStart >= address) // selection starts in this line?
            {
                DrawSides(dc, rc2, wxTOP | wxRIGHT | wxBOTTOM);
                sides |= wxLEFT | wxTOP;
            }
            else
            {
                DrawSides(dc, rc2, wxRIGHT | wxBOTTOM);
                //sides |= wxLEFT;
            }
            rc.width = rc2.x - rc.x;
        }
        else // selection does not end in this or the next line
            sides |= wxRIGHT;

        if (sides)
            DrawSides(dc, rc, sides);
    }

    int col = 0;
    THSIZE orig_address = address;

    if (m_iScrollX > pane.m_start1)
    {
        // Another case where if iLineBytes is small, painting is fast anyway, and if it's large,
        // then we stand to gain much by skipping bytes; so we do the extra work here.
        int digit, half;
        col = pane.HitTest(m_iScrollX, digit, half);
        address += col * pane.m_iColBytes;
    }

    for (col; col < byteCount / pane.m_iColBytes; col++, address += pane.m_iColBytes)
    {
        uint8 val;
        //x = pane.GetX(col + byteStart / pane.m_iColBytes);
        int width;
        int etoFlags = 0;
        x = pane.GetRect(col + byteStart / pane.m_iColBytes, 1, width, 0) - m_iScrollX;
        int charCount;
        char buf[65]; //! how big can this get?  64-bit binary, long double...
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
            val = pData[col];
            //! if there is a background, we draw even when val==0
            buf[0] = val ? val : ' ';
            charCount = 1;
            lead = iLeftLead[(uint8)buf[0]];
        }
        else if (pane.id == DisplayPane::UNICODE)
        {
            //! todo: draw real character
            if (s.iEndianMode == NATIVE_ENDIAN_MODE)
            {
                buf[0] = pData[col * 2];
                buf[1] = pData[col * 2 + 1];
            }   
            else
            {
                buf[0] = pData[col * 2 + 1];
                buf[1] = pData[col * 2];
            }
            uint16 val16 = *(uint16*)buf;
            val = val16 >> 8;
            if (val16 == 0)
                buf[0] = ' ', lead = 0;
            else if (s.bFontCharsOnly && !iCharWidths[val16])
            {
                //! How do we tell if the system can really render this character?
                *(uint16*)buf = val16 = s.iDefaultChar;
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

            //! doesn't work for signed data
            //if (pane.m_iEndianness == LITTLEENDIAN_MODE)
            //    val = pData[pane.m_iColBytes - 1];
            //else
            //    val = pData[0];

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
        else if (pane.id == DisplayPane::VECMEM)
        {
            *(ULONG*)buf = DecodeVecMem(pData - (address & 0x70), (int)address & 0x7F);
            val = 128; // color everything medium
            charCount = 4;
            lead = iLeftLead[(uint8)buf[0]];
        }

        if (col >= selStartCol && col < selEndCol)
        {
            // Hexplorer seems to do this by PatBlt()-ing with CreateSolidBrush(0x401540).
            // DeleteObject(SelectObject(hdc, CreateSolidBrush(0x401540)));
            // This is rather crude, and produces a discontinuous color spectrum.
            // These two lines accomplish the same thing as Hexplorer:
            //SetTextColor(hdc, s.clrText[val] ^ 0x401540);
            //SetBkColor(hdc, s.clrTextBack ^ 0x401540);
            SetBkMode(hdc, TRANSPARENT);

            //SetTextColor(hdc, s.clrSelText[val]);
            SetTextColor(hdc, s.palSelText.GetColor(val));
            //SetBkColor(hdc, s.clrSelBack);
        }
        //else if (doc->IsModified(address))
        else if (s.bHighlightModified && m_pModified && m_pModified[col * pane.m_iColBytes])
        {
            //SetTextColor(hdc, s.clrModText[val]);
            SetTextColor(hdc, s.palModText.GetColor(val));
            SetBkColor(hdc, s.clrModBack);
            SetBkMode(hdc, OPAQUE);
            etoFlags |= ETO_OPAQUE;
        }
        else
        {
            if (s.bEvenOddColors)
            {
                //SetTextColor(hdc, s.clrText[address & 1]);
                SetTextColor(hdc, s.clrEOText[address & 1]);
                SetBkColor(hdc, s.clrEOBack[address & 1]);
            }
            else
            {
                //SetTextColor(hdc, s.clrText[val]);
                SetTextColor(hdc, s.palText.GetColor(val));
                SetBkColor(hdc, backClr);
            }
            SetBkMode(hdc, OPAQUE);
            etoFlags |= ETO_OPAQUE;
        }

        RECT rcText = { x, y, x + width, y + m_iLineHeight };
        rcText.bottom -= s.iExtraLineSpace; //!
        rcText.bottom -= s.bGridLines ? 1 : 0; //!
        // ExtTextOut() is the fastest way to draw text, and gives us the best control.
        // ExtTextOutW() is one of the few Unicode functions supported natively on Win95.
        if (pane.id == DisplayPane::UNICODE)
        {
            //TextOutW(hdc, x, y, (LPCWSTR)buf, charCount); // supported on Win98, believe it or not.
            ExtTextOutW(hdc, x + lead + 1, y, etoFlags, &rcText, (LPCWSTR)buf, 1, NULL);
        }
        else
        {
            if (lead)
               lead = lead; //! breakpoint
            if (buf[0] == '0' && buf[1] == 'A')
               lead = lead; //! breakpoint
            //TextOutA(hdc, x, y, buf, charCount);
            ExtTextOutA(hdc, x + lead + 1, y, etoFlags, &rcText, buf, charCount, NULL);
        }
    }

    int colCount = byteCount / pane.m_iColBytes;
    //address -= colCount * pane.m_iColBytes;
    //address -= col * pane.m_iColBytes;
    address = orig_address;
    if (m_bMousePosValid && RangeOverlapSize(m_iMouseOverByte, m_iMouseOverCount, address, colCount * pane.m_iColBytes))
    {
        int col = (m_iMouseOverByte - address + byteStart) / pane.m_iColBytes;
        int count = DivideRoundUp(m_iMouseOverCount, pane.m_iColBytes);
        GetColRect(0, col, count, nPane, rc, /*GBR_NOHSCROLL |*/ GBR_VFILL);
        //! can't call InvertRect() with wx, but we shouldn't be using that anyway.
        dc.SetLogicalFunction(wxINVERT);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(rc);
        dc.SetLogicalFunction(wxCOPY);
    }

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
        //GetByteRect(hlStart, hlEnd - hlStart, nPane, rc);
        dc.SetPen(*wxThePenList->FindOrCreatePen(s.clrHighlight, 1, wxSOLID));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rc);
    }
    
    // draw lines between panes
    dc.SetPen(m_penGrid);
    //dc.DrawLine(pane.m_start - s.iPanePad - m_iScrollX, 0,
    //    pane.m_start - s.iPanePad - m_iScrollX, m_iLineHeight);
    dc.DrawLine(pane.GetRight() - m_iScrollX, 0,
        pane.GetRight() - m_iScrollX, m_iLineHeight);
}
