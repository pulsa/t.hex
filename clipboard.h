#ifndef _CLIPBOARD_H_
#define _CLIPBOARD_H_

#include "defs.h"

class HexDoc;

//****************************************************************************
//****************************************************************************
// thCopyFormat
//****************************************************************************
//****************************************************************************

class thCopyFormat
{
public:
    enum _DataFormat{
        RAW = 0,
        INTEGER,
        FLOAT,
        C_CODE,
        ESCAPED,
        SCREEN,
        PANE } dataFormat;

    enum _FileFormat {
        PLAIN = 0,
        RTF,
        HTML } fileFormat;

    int numberBase;
    int wordSize;
    int wordsPerLine;
    int endianMode;
    bool unicode;

    int iCodePage; // added 2008-01-18.  Long time since I thought about this stuff.

    HexDoc *doc; //! this should probably not be here
    THSIZE offset, srcLen; //! Where is this going?

    thCopyFormat( _DataFormat fmt = RAW)
    {
        dataFormat = fmt;
        fileFormat = PLAIN;
        iCodePage = 0;
    };

    bool EnableFileFormat();
    bool EnableCharSet();
    bool EnableNumberBase();
    bool EnableWordSize();
    bool EnableWordsPerLine();
    bool EnableEndianMode();

    //size_t EstimateMaxSize(THSIZE srcLen); // if you allocate this much, you'll be fine.
    //size_t GetRequiredSize(HexDoc *doc, THSIZE offset, THSIZE srcLen);
    bool Render(uint8 *target, size_t *bufferSize);
    //wxString Render(HexDoc *doc, THSIZE offset, THSIZE srcLen);

    bool DoCopy(HexDoc *doc, THSIZE offset, THSIZE srcLen);

protected:
    inline bool Append(uint8 *target, size_t bufferSize, size_t &offset, const uint8 *src, int len)
    {
        offset += len;
        if (target != NULL)
        {
            if (offset > bufferSize)
                return false;
            memcpy(target + offset, src, len);
        }
        return true;
    }

    inline bool Append(uint8 *target, size_t bufferSize, size_t &offset, uint8 src)
    {
        offset++;
        if (target != NULL)
        {
            if (offset > bufferSize)
                return false;
            target[offset] = src;
        }
        return true;
    }
};


//****************************************************************************
//****************************************************************************
// thClipboard
//****************************************************************************
//****************************************************************************

class thClipboard
{
public:
    thClipboard(HWND hWnd);
    bool SetData(wxString data, int pane, thCopyFormat fmt);
    bool GetData(wxString &data);
    bool Cycle();
    bool RenderFormat(UINT nFormat);
    static bool SetClipboardData(UINT nFormat, const void *pmem, size_t size);
    UINT m_nFormat;

    enum {FMT_DEFAULT = 0, FMT_AUTO }; // clipboard format when pasted to other apps

protected:
    int m_current, m_last, m_count;
    wxString m_data[20];
    thCopyFormat m_format[20];
    HWND m_hWnd;
    int m_nPane; //! bad way to store this info

    bool m_bActive;
    bool Activate(int slot);
};

#ifndef CF_TEXT
#define CF_TEXT             1
#define CF_BITMAP           2
#define CF_METAFILEPICT     3
#define CF_SYLK             4
#define CF_DIF              5
#define CF_TIFF             6
#define CF_OEMTEXT          7
#define CF_DIB              8
#define CF_PALETTE          9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12
#define CF_UNICODETEXT      13
#endif

#endif // _CLIPBOARD_H_
