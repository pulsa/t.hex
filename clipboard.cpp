#include "precomp.h"
#include "clipboard.h"
#include "thex.h"
#include "hexdoc.h"
#include "utils.h"

#include "hexwnd.h"

#define new New

//*****************************************************************************
//*****************************************************************************
// thCopyFormat
//*****************************************************************************
//*****************************************************************************

//size_t thCopyFormat::EstimateMaxSize(THSIZE srcLen)
//{
//    int tmp1, tmp2;
//    switch (dataFormat)
//    {
//    case RAW: return srcLen + 1;
//    case NUMERIC: {
//        const int words = (srcLen + wordSize - 1) / wordSize;
//        const int lines = (words + wordsPerLine - 1) / wordsPerLine;
//        const int digitsPerWord = ceil(wordSize * 8 * log10(2) / log10(numberBase));
//        return lines * (2 + wordsPerLine * (1 + digitsPerWord)); }
//    case C_CODE: {
//    }
//}
//
//size_t thCopyFormat::GetRequiredSize(HexDoc *doc, THSIZE offset, THSIZE srcLen)
//{
//}

bool thCopyFormat::Render(uint8 *target, size_t *bufferSize)
{
    if (srcLen > 0x100000)
        srcLen = 0x100000; // cut off at 1M for now
    size_t outputSize = 0;
    bool success = true, terminate = true;
    //uint8 tmp8;
    uint64 tmp64;
    char buf[100];
    switch (dataFormat)
    {
    case RAW:
        if (unicode)
        {
            //! to do: allow for other source encodings than UTF-16
            if (target && 0) // test converting from 8-bit Russian character set, 2008-01-18
            {
                const uint8 *data = doc->Load(offset, srcLen);
                //int mbrc = MultiByteToWideChar(this->iCodePage, MB_USEGLYPHCHARS, (LPCSTR)data, srcLen, (LPWSTR)target, *bufferSize);
                int mbrc = MultiByteToWideChar(this->iCodePage, 0, (LPCSTR)data, srcLen, (LPWSTR)target, *bufferSize);
                if (mbrc)
                    outputSize = mbrc * 2;
                else
                    PRINTF(_T("Didn't work.  Error code %d.\n"), GetLastError());
            }
            else

            if (target)
            {
                WordIterator2 iter(offset / 2, doc);
                for (size_t n = 0; n < srcLen; n += 2, ++iter)
                {
                    if (outputSize + 2 > *bufferSize)
                        return false;
                    *(uint16*)&target[outputSize] = *iter;
                    outputSize += 2;
                }
            }
            else
                outputSize += RoundDown(srcLen, 2);
        }
        else // not Unicode
        {
            if (target)
            {
                ByteIterator2 iter(offset, doc);
                for (size_t n = 0; n < srcLen; n++, ++iter)
                {
                    if (outputSize >= *bufferSize)
                        return false;
                    target[outputSize] = *iter;
                    outputSize++;
                }
            }
            else
                outputSize += srcLen;
        }
        break;
    case INTEGER: {
        srcLen -= srcLen % wordSize;
        const int digitsPerWord = ceil(wordSize * 8 * log10(2.0) / log10((double)numberBase));
        size_t word = 1;
        for (size_t n = 0; n < srcLen; n += wordSize)
        {
            if (!doc->ReadInt(offset + n, wordSize, &tmp64))
                return false;
            my_itoa(tmp64, buf, numberBase, digitsPerWord);
            int len = digitsPerWord;
            if ((wordsPerLine == 0 || (int)word < wordsPerLine) && n + 1 < srcLen) {
                word++;
                buf[len++] = ' ';
            }
            else {
                word = 1;
                buf[len++] = '\r';
                buf[len++] = '\n';
            }
            //if (!Append(target, *bufferSize, outputSize, (uint8*)buf, len))
            //    return false;
            if (target != NULL)
            {
                if (outputSize + len > *bufferSize)
                    return false;
                memcpy(target + outputSize, buf, len);
            }
            outputSize += len;
        }
    } break;
    case FLOAT:
        break;
    case C_CODE:
        break;
    case ESCAPED:
        if (unicode) break;  // Can't do it, captain!
        else {
            ByteIterator2 iter(offset, doc);
            for (size_t n = 0; n < srcLen; n++, ++iter)
            {
                TCHAR c[4] = {*iter, 0, 0, 0};
                int count = 2;
                switch (c[0])
                {
                case '\\': c[1] = '\\'; break;
                case '\'': c[1] = '\''; break;
                case '\"': c[1] = '\"'; break;
                case '\a': c[1] = 'a'; break;
                case '\b': c[1] = 'b'; break;
                case '\f': c[1] = 'f'; break;
                case '\n': c[1] = 'n'; break;
                case '\r': c[1] = 'r'; break;
                case '\t': c[1] = 't'; break;
                case '\v': c[1] = 'v'; break;
                default:
                    if (c[0] < ' ' || c[0] > '~')
                    {
                        // Add '\xFF' escape sequence.
                        //! to do: use number base setting to indicate octal?  (I hate octal.)
                        my_itoa((uint32)c[0], (TCHAR*)&c[2], 16, 2);
                        c[0] = '\\';
                        c[1] = 'x';
                        count = 4;
                    }
                    else
                        count = 1;
                }
                if (target != NULL)
                {
                    if (outputSize + 2 > *bufferSize)
                        return false;
                    if (count >= 2) {
                        target[outputSize    ] = '\\';
                        target[outputSize + 1] = c[1];
                    } else
                        target[outputSize] = c[0];
                    if (count == 4) {
                        target[outputSize + 2] = c[2];
                        target[outputSize + 3] = c[3];
                    }
                }
                outputSize += count;
            }
        }
        break;
    default: break;
    }

    if (terminate)
    {
        if (target) {
            target[outputSize] = 0;
            if (unicode)
                target[outputSize + 1] = 0;
        }
        outputSize += (unicode ? 2 : 1);
    }

    *bufferSize = outputSize;
    return success;
}

//wxString thCopyFormat::Render(HexDoc *doc, THSIZE offset, THSIZE srcLen)
//{
//}


//****************************************************************************
//****************************************************************************
// thClipboard
//****************************************************************************
//****************************************************************************

thClipboard::thClipboard(HWND hWnd)
{
    m_nFormat = RegisterClipboardFormat(APPNAME);
    m_hWnd = hWnd;
    m_count = 0;
    m_current = 0;
    m_last = 0;
}

bool thClipboard::SetData(wxString data, int pane, thCopyFormat format)
{
    //int slot;
    //if (m_count == 20)
    //    slot = m_last = (m_last + 1) % 20;
    //else
    //    slot = m_last = m_count++;
    //m_data[slot] = data;
    //return Activate(slot);
    //! I don't know how this multiple-clipboard thing should work yet.
    //  Maybe it doesn't belong here at all.  Anyway, just use one for now.
    m_data[0] = data;
    m_format[0] = format;
    m_count = 1;
    m_nPane = pane;
    return Activate(0);
}

bool thClipboard::Activate(int slot)
{
// Don't store serialized data in clipboard.  Instead, store 0 bytes of custom data.

    m_bActive = false;
    if (!OpenClipboard(m_hWnd))
        return m_bActive;
    if (EmptyClipboard())
        m_bActive = true;  //! todo: be smart if the clipboard is locked.
    ::SetClipboardData(m_nFormat, NULL);
    thCopyFormat &fmt = m_format[slot];

#if 1  // This works, 2008-05-13.  Disabled for delayed rendering.
    size_t size;
    if (fmt.Render(NULL, &size))
    {
        HANDLE hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
        if (hGlobal == NULL)
            return false;
        uint8 *pClip = (uint8*)GlobalLock(hGlobal);
        if (pClip == NULL)
        {
            GlobalFree(hGlobal);
            return false;
        }

        fmt.Render(pClip, &size);
        GlobalUnlock(hGlobal);
        // Windows will set the last character to zero here, but it should be done in Render().
        if (fmt.unicode)
            ::SetClipboardData(CF_UNICODETEXT, hGlobal);
        else
            ::SetClipboardData(CF_TEXT, hGlobal);
    }
#else // delayed rendering
    if (fmt.unicode)
        ::SetClipboardData(CF_UNICODETEXT, NULL);
    else
        ::SetClipboardData(CF_TEXT, NULL);
#endif

    CloseClipboard();

    if (m_bActive)
        m_current = slot;
    return m_bActive;
}

bool thClipboard::Cycle()
{
    if (m_count == 0)
        return false;
    return Activate(m_current ? m_count - 1 : m_current - 1);
}

bool thClipboard::GetData(wxString &data)
{
    if (m_count == 0)
        return false;

    data = m_data[m_current];
    return true;
}

bool thClipboard::SetClipboardData(UINT nFormat, const void *pmem, size_t size)
{
    HANDLE hGlobal = GlobalAlloc(GMEM_MOVEABLE, size + 1);
    if (hGlobal == NULL)
        return false;
    uint8 *pClip = (uint8*)GlobalLock(hGlobal);
    if (pClip == NULL)
    {
        GlobalFree(hGlobal);
        return false;
    }

    if (pmem)
        memcpy(pClip, pmem, size);
    pClip[size] = 0;
    GlobalUnlock(hGlobal);
    return ::SetClipboardData(nFormat, hGlobal) != NULL;
}

bool thClipboard::RenderFormat(UINT nFormat)
{
    //! This gets weird because we allocate memory at the end of the procedure,
    //  only after we know exactly how much we need.  There's gotta be a better way.
    wxString data;
    if (!GetData(data))
        goto end;
    if (nFormat == m_nFormat)
        return SetClipboardData(nFormat, data.c_str(), data.Len());
    else if (nFormat == CF_UNICODETEXT)
    {
        SerialData sdata(data);
        if (!sdata.Ok())
            goto end;

        HANDLE hGlobal = GlobalAlloc(GMEM_MOVEABLE, sdata.m_nTotalSize + 2);
        if (hGlobal == NULL)
            goto end;
        uint8 *pClip = (uint8*)GlobalLock(hGlobal);
        if (pClip == NULL)
        {
            GlobalFree(hGlobal);
            goto end;
        }

        THSIZE len = 0;

        // unserialize and read all serialized segments
        for (int iSeg = 0; iSeg < sdata.hdr.nSegments; iSeg++)
        {
            Segment *ts = Segment::Unserialize(sdata, iSeg);
            ts->Read(0, ts->size, pClip + len);
            len += ts->size;
            delete ts;
        }

        pClip[len] = pClip[len + 1] = 0;
        GlobalUnlock(hGlobal);
        return ::SetClipboardData(nFormat, hGlobal) != NULL;
    }
    else if (nFormat == CF_TEXT)
    {
        //! todo: render as selected style -- raw bytes, or formatted somehow
        SerialData sdata(data);
        if (!sdata.Ok())
            goto end;

        THSIZE size = sdata.m_nTotalSize, len = 0;
        if (m_nPane == DisplayPane::HEX)
            size *= 3 * sizeof(TCHAR);
        uint8 *pdata = new uint8[size];
        TCHAR *hdata = (TCHAR*)pdata;

        // unserialize and read all serialized segments
        for (int iSeg = 0; iSeg < sdata.hdr.nSegments; iSeg++)
        {
            Segment *ts = Segment::Unserialize(sdata, iSeg);
            ts->Read(0, ts->size, pdata + len);
            len += ts->size;
            delete ts;
        }

        if (m_nPane == DisplayPane::HEX)
        {
            // convert to hex-formatted ASCII
            for (int i = len - 1; i >= 0; i--)
            {
                my_itoa((DWORD)pdata[i], hdata + i * 3, 16, 2);
                hdata[i * 3 + 2] = ' ';
            }
            size--;
            pdata[size] = 0;
        }

        if (!SetClipboardData(nFormat, pdata, size))
            return false;

        delete [] pdata;
        return true;
    }

end:
    return SetClipboardData(nFormat, NULL, 0);
}
