#pragma once

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "defs.h"
#include "palette.h"

class HexWndSettings
{
public:
    int iLineBytes;
    int iAddressDigits;
    int iPanePad;
    int iExtraLineSpace;
    int iAddressBase;
    int iByteBase;
    int iEndianMode;    // LITTLEENDIAN_MODE, BIGENDIAN_MODE, or NATIVE_ENDIAN_MODE
    int iDigitGrouping; // columns in a group, padded by more space
    int iGroupSpace;
    bool bGridLines;
    bool bPrettyAddress; // insert spaces, colons, commas, whatever
    bool bAdjustLineBytes;
    bool bAbsoluteAddresses;
    bool bShowRuler;
    bool bHighlightModified;
    bool bStickyAddr;
    bool bFontCharsOnly;
    bool bDrawNulAsSpace;
    int iCodePage;
    wxArrayString asCodePages;
    wxString sFont;
    bool bFakeMonoSpace; // try to face a monospace look with proportional fonts
    bool bSelectOnPaste;

    int iFontQuality; // LOGFONT.lfQuality setting, or -1 to use OS default
    BYTE GetFontQuality(); // return value is suitable for LOGFONT.lfQuality
    //int iDefaultChar;  // placeholder where no valid glyph can be drawn.

    wxString TextPalette;
    //wxString SelPalette, ModPalette;

    //COLORREF clrText[256];
    COLORREF clrTextBack;
    //COLORREF clrSelText[256];
    COLORREF clrSelBack;
    COLORREF clrSelBorder;
    //COLORREF clrModText[256];
    COLORREF clrModBack;
    COLORREF clrAdr;
    COLORREF clrAdrBack;
    COLORREF clrWndBack;
    COLORREF clrGrid;
    COLORREF clrAnsiBack;
    COLORREF clrUnicodeBack;
    COLORREF clrHighlight; // not really highlighting -- we have too many colors already

    // even/odd color style
    bool bEvenOddColors;
    COLORREF clrEOText[2];
    COLORREF clrEOBack[2];

    HexWndSettings();
    void LoadDefaults();
    void LoadDefaults1();
    void Load(wxFileConfig &cfg);
    void Save(wxFileConfig &cfg);

    bool ReadColor(wxFileConfig &cfg, wxString name, COLORREF &clr);
    bool WriteColor(wxFileConfig &cfg, wxString name, COLORREF clr);

    void SetPalette(wxString pal);
    void AdjustPalette(COLORREF dst[256], const COLORREF src[256], COLORREF d, float trans);
    thPalette palText, palSelText, palModText;

    COLORREF GetBackgroundColour()
    {
        if (bEvenOddColors)
            return clrEOBack[0];
        else
            return clrWndBack;
    }
};

struct StatusBarSettings
{
    bool bExactFileSize;
    bool bFileSizeInKB;
    THBASE FileSizeBase;
    THBASE SelectionBase;
    THBASE ValueBase;
    bool bSignedValue;
};

struct FindSettings
{
    bool bUnicodeSearch;  // used to build the search string, not perform the actual search
    bool bIgnoreCase;
    bool bAllRegions;
    bool bSelectionOnly;
    bool bNoWrap;
    //bool bFindBackward;
    uint8 *data;
    size_t length;
};

const int THF_BACKWARD   =  1;  // default is forward
const int THF_IGNORECASE =  2;  // default is case-sensitive
const int THF_ALLREGIONS =  4;  // default is current document only
const int THF_SELONLY    =  8;  // default is whole document
const int THF_AGAIN      = 16;  // start after the current selection.  Default is to include it.
const int THF_NOWRAP     = 32;  // default = wrap at end of region(s)

void WriteArray(wxFileConfig &cfg, wxString name, const wxArrayString &a, int count = -1);
void ReadArray(wxFileConfig &cfg, wxString name, wxArrayString &a, int count = -1);

class thAppSettings
{
public:
    thAppSettings();
    ~thAppSettings();

    bool bStatusBar, bToolBar;
    bool bFileMap, bDocList, bNumberView, bStringView, bStructureView, bDisasmView, bFatView,
        bExportView;
    bool bInsertMode;
    bool bAdjustAddressDigits;
    //bool bZeroOctalPrefix; // this is stupid.  I want nothing to do with it.
    bool bReverseRGB; // allow #RRGGBB, instead of C-style 0xBBGGRR
    bool bFullScreen;
    bool bReuseWindow; //! I think this makes a bad UI.  What could be better?
    bool bDefaultReadOnly;
    bool bFileShareWrite;  // when files opened RO, allow writes from other procs and update screen
    bool bExactDriveSize; // should this depend on status bar format setting?
    int DisasmOptions;  // disassembler options in bastard.h
    int nRecentChoices;  // number of entries to remember for thRecentChoice
    int nSmartCompareSize;

    wxArrayString asFindText, asFindHex;
    FindSettings find;
    int GetFindFlags();

    struct StatusBarSettings sb;

    wxArrayString asGoto, asOpenSpecial;
    wxArrayString asSpawnCmd, asSpawnArgs, asSpawnPath;

    void Load(wxFileConfig &cfg);
    void Save(wxFileConfig &cfg);
};

//extern HexWndSettings s; //! fix this
extern HexWndSettings ghw_settings;
extern thAppSettings appSettings;


#if(_WIN32_WINNT < 0x0501)  // These are defined in WinUser.h if >= 0x0501
#define SPI_GETFONTSMOOTHINGTYPE            0x200A
#define FE_FONTSMOOTHINGSTANDARD            0x0001
#define FE_FONTSMOOTHINGCLEARTYPE           0x0002
#define CLEARTYPE_QUALITY       5
#endif


#endif // _SETTINGS_H_
