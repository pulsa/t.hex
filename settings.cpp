#include "precomp.h"
#include "settings.h"
#include "thex.h"
#include "utils.h"
#include "palette.h"

#define new New

HexWndSettings ghw_settings;

HexWndSettings::HexWndSettings()
{
    LoadDefaults();
}

void HexWndSettings::LoadDefaults()
{
    iLineBytes = 16;
    iAddressDigits = 10;
    iExtraLineSpace = 0;//5;
    iAddressBase = 16;
    iByteBase = 16;
    iDigitGrouping = 4;
    iGroupSpace = 7;
    iPanePad = 10; //! todo: express relative to font size?
    //bGridLines = true;
    bPrettyAddress = false;
    bAdjustLineBytes = false;
    bAbsoluteAddresses = true;
    bShowRuler = true;
    bHighlightModified = true;
    bStickyAddr = true;
    bFontCharsOnly = true;
    iCodePage = 0; // use default from font character set
    bFakeMonoSpace = true;
    bDrawNulAsSpace = true;  // VEDIT fonts actually have glyphs for control chars.  Cool.

    TextPalette = _T("0 ( 0 93 30 ) - 128 ( 0 156 30 ) - 255 ( 127 210 157 )");

    //SelPalette = "0 ( 90 90 30 ) - 128 ( 124 124 60 ) - 255 ( 187 187 127 )";
    //ModPalette = "0 ( 0 60 60 ) - 128 ( 0 124 124 ) - 255 ( 127 187 187 )";
    //for (int val = 0; val < 256; val++)
    //{
       // int c1 = (val >> 1) + 93, c2 = 0;
       // if (val >= 128)
          //  c2 = val - 127;
       // clrText[val]    = RGB(c2, c1, c2 + 30);
       // clrSelText[val] = RGB(c1, c1, c2);
       // clrModText[val] = RGB(c2, c1, c1);
    //}
    //DoPalette(clrText, TextPalette);
    //DoPalette(clrSelText, SelPalette);
    //DoPalette(clrModText, ModPalette);

    clrTextBack = RGB(0, 0, 0);
    clrSelBack  = 0x401540;
    clrSelBorder = RGB(0, 255, 255);
    clrModBack = RGB(56, 20, 20);
    clrAdr = RGB(0, 255, 255);
    clrAdrBack = RGB(64, 64, 64);
    clrWndBack = RGB(0, 0, 0);
    clrGrid = RGB(128, 128, 128);
    clrHighlight = RGB(200, 200, 0);

    //bEvenOddColors = true;
    clrEOText[0] = RGB(0, 0, 128);
    clrEOText[1] = RGB(0, 0, 128);
    clrEOBack[0] = RGB(255, 255, 255);
    clrEOBack[1] = RGB(240, 240, 240);

    iFontQuality = DRAFT_QUALITY;
    //iDefaultChar = 0x80;
    //iDefaultChar = 0xb7;
}

void HexWndSettings::LoadDefaults1()
{
    iLineBytes = 16;
    iAddressDigits = 10;
    iExtraLineSpace = 0;//5;
    iAddressBase = 16;
    iByteBase = 16;
    iDigitGrouping = 4;
    iGroupSpace = 7;
    iPanePad = 15; //! todo: express relative to font size?
    bGridLines = true;
    bPrettyAddress = true;
    bAdjustLineBytes = true;
    bAbsoluteAddresses = true;
    bShowRuler = true;
    bHighlightModified = true;
    bStickyAddr = true;
    bFontCharsOnly = true;
    iCodePage = 0; // use default from font character set

    // text, highlight CR and LF
    //TextPalette = "0 (0 60 30) - 128 (0 124 30) - 255 (127 187 157); 10 13 (255 192 0); 65 97 (80 80 255) - 90 122 (155 155 255)";
    TextPalette = _T("0 (10 100 30) - 128 (96 150 60) - 255 (100 220 170); '\r' '\n' (00c0ffh); '\t' 32 (006080h); 'A' 'a' (80 80 255) - 'Z' 'z' (155 155 255); '0' (150 50 120) - '9' (200 60 180)");
    //! '\r' and '\n' are translated by wxConfig, but that doesn't mean we can't use them here.
    // Backslashes must be escaped, too.
    //TextPalette = "00h 10h 20h 30h 40h 50h 60h 70h 80h 90h a0h b0h c0h d0h e0h f0h (0 60 30) - 0fh 1fh 2fh 3fh 4fh 5fh 6fh 7fh 8fh 9fh afh bfh cfh dfh efh ffh (0 200 0)";

    //SelPalette = "0 ( 60 60 0 ) - 128 ( 124 124 0 ) - 255 ( 187 187 127 )";
    //ModPalette = "0 ( 0 60 60 ) - 128 ( 0 124 124 ) - 255 ( 127 187 187 )";
    //for (int val = 0; val < 256; val++)
    //{
       // int c1 = (val >> 1) + 60, c2 = 0;
       // if (val >= 128)
          //  c2 = val - 127;
       // clrText[val]    = RGB(c2, c1, c2 + 30);
       // clrSelText[val] = RGB(c1, c1, c2);
       // clrModText[val] = RGB(c2, c1, c1);
    //}
    //for (int val = 0; val < 26; val++)
    //{
    //    int c1 = 80 + val * 3;
    //    clrText[val + 'A'] = clrText[val + 'a'] = RGB(c1, c1, 255);
    //}
    //clrText[10] = clrText[13] = RGB(255, 192, 0);
    //DoPalette(clrText, TextPalette);
    //thPalette::SetPalette(TextPalette);
    palText.SetPalette(TextPalette);
    //DoPalette(clrSelText, SelPalette);
    //DoPalette(clrModText, ModPalette);

    clrSelBack  = RGB(0x80, 0x80, 0xFF); //! todo: make this partially transparent?
    clrSelBorder = RGB(0, 255, 255);
    clrModBack = RGB(0xFF, 0xE0, 0xE0);
    clrAdr = RGB(0, 0, 0);
    clrAdrBack = RGB(192, 192, 192);
    clrWndBack = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).GetPixel();
    clrTextBack = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).GetPixel();
    clrAnsiBack = RGB(244, 244, 255);
    clrUnicodeBack = RGB(255, 244, 244);
    clrGrid = RGB(0, 0, 0);
    clrHighlight = RGB(128, 64, 0);

    //bEvenOddColors = true;
    clrEOText[0] = RGB(0, 0, 128);
    clrEOText[1] = RGB(0, 0, 128);
    clrEOBack[0] = RGB(255, 255, 255);
    clrEOBack[1] = RGB(240, 240, 240);
}

void HexWndSettings::Load(wxFileConfig &cfg)
{
    cfg.Read(_T("iLineBytes"), &iLineBytes);
    cfg.Read(_T("iAddressDigits"), &iAddressDigits);
    cfg.Read(_T("iExtraLineSpace"), &iExtraLineSpace);
    cfg.Read(_T("iAddressBase"), &iAddressBase);
    cfg.Read(_T("iByteBase"), &iByteBase);
    cfg.Read(_T("iDigitGrouping"), &iDigitGrouping);
    cfg.Read(_T("iGroupSpace"), &iGroupSpace);
    cfg.Read(_T("iPanePad"), &iPanePad);
    cfg.Read(_T("bGridLines"), &bGridLines);
    cfg.Read(_T("bPrettyAddress"), &bPrettyAddress);
    cfg.Read(_T("bAdjustLineBytes"), &bAdjustLineBytes);
    cfg.Read(_T("bAbsoluteAddresses"), &bAbsoluteAddresses);
    cfg.Read(_T("bShowRuler"), &bShowRuler);
    cfg.Read(_T("bHighlightModified"), &bHighlightModified);
    cfg.Read(_T("bStickyAddr"), &bStickyAddr);
    cfg.Read(_T("bFontCharsOnly"), &bFontCharsOnly);
    cfg.Read(_T("iCodePage"), &iCodePage);
    cfg.Read(_T("sFont"), &sFont);
    cfg.Read(_T("bFakeMonoSpace"), &bFakeMonoSpace);

    cfg.Read(_T("TextPalette"), &TextPalette);
    //cfg.Read(_T("SelPalette"), &SelPalette);
    //cfg.Read(_T("ModPalette"), &ModPalette);

//#define ReadColor(cfg, str, val) cfg.Read(str, (long*)&(val))

    ReadColor(cfg, _T("clrSelBack"),     clrSelBack);
    ReadColor(cfg, _T("clrSelBorder"),   clrSelBorder);
    ReadColor(cfg, _T("clrModBack"),     clrModBack);
    ReadColor(cfg, _T("clrAdr"),         clrAdr);
    ReadColor(cfg, _T("clrAdrBack"),     clrAdrBack);
    ReadColor(cfg, _T("clrWndBack"),     clrWndBack);
    ReadColor(cfg, _T("clrTextBack"),    clrTextBack);
    ReadColor(cfg, _T("clrAnsiBack"),    clrAnsiBack);
    ReadColor(cfg, _T("clrUnicodeBack"), clrUnicodeBack);
    ReadColor(cfg, _T("clrGrid"),        clrGrid);
    ReadColor(cfg, _T("clrHighlight"),   clrHighlight);

    cfg.Read(_T("bEvenOddColors"), &bEvenOddColors);
    ReadColor(cfg, _T("clrEOText0"), clrEOText[0]);
    ReadColor(cfg, _T("clrEOText1"), clrEOText[1]);
    ReadColor(cfg, _T("clrEOBack0"), clrEOBack[0]);
    ReadColor(cfg, _T("clrEOBack1"), clrEOBack[1]);

    //DoPalette(clrText, TextPalette);
    //thPalette::SetPalette(clrText, TextPalette);
    //DoPalette(clrSelText, SelPalette);
    //DoPalette(clrModText, ModPalette);

    //AdjustPalette(clrSelText, clrText, clrSelBack, .2);
    //AdjustPalette(clrModText, clrText, clrModBack, .2);

    if (wxFile::Exists(TextPalette))
    {
        wxString msg = palText.Import(TextPalette);
        palSelText = palModText = palText;
    }
    else // treat it as gradient string
        SetPalette(TextPalette);
}

void HexWndSettings::Save(wxFileConfig &cfg)
{
    cfg.Write(_T("iLineBytes"), iLineBytes);
    cfg.Write(_T("iAddressDigits"), iAddressDigits);
    cfg.Write(_T("iExtraLineSpace"), iExtraLineSpace);
    cfg.Write(_T("iAddressBase"), iAddressBase);
    cfg.Write(_T("iByteBase"), iByteBase);
    cfg.Write(_T("iDigitGrouping"), iDigitGrouping);
    cfg.Write(_T("iGroupSpace"), iGroupSpace);
    cfg.Write(_T("iPanePad"), iPanePad);
    cfg.Write(_T("bGridLines"), bGridLines);
    cfg.Write(_T("bPrettyAddress"), bPrettyAddress);
    cfg.Write(_T("bAdjustLineBytes"), bAdjustLineBytes);
    cfg.Write(_T("bAbsoluteAddresses"), bAbsoluteAddresses);
    cfg.Write(_T("bShowRuler"), bShowRuler);
    cfg.Write(_T("bHighlightModified"), bHighlightModified);
    cfg.Write(_T("bStickyAddr"), bStickyAddr);
    cfg.Write(_T("bFontCharsOnly"), bFontCharsOnly);
    cfg.Write(_T("iCodePage"), iCodePage);
    cfg.Write(_T("sFont"), sFont);
    cfg.Write(_T("bFakeMonoSpace"), bFakeMonoSpace);

    //cfg.Write(_T("TextPalette"), _T("(0, 60, 30) 128 (0, 124, 30) (127, 187, 157)"));
    // text, highlight CR and LF
    cfg.Write(_T("TextPalette"), TextPalette);
    //cfg.Write(_T("SelPalette"), SelPalette);
    //cfg.Write(_T("ModPalette"), ModPalette);

//#define WriteColor(cfg, str, val) cfg.Write(str, (long)val)

    WriteColor(cfg, _T("clrSelBack"),     clrSelBack);
    WriteColor(cfg, _T("clrSelBorder"),   clrSelBorder);
    WriteColor(cfg, _T("clrModBack"),     clrModBack);
    WriteColor(cfg, _T("clrAdr"),         clrAdr);
    WriteColor(cfg, _T("clrAdrBack"),     clrAdrBack);
    WriteColor(cfg, _T("clrWndBack"),     clrWndBack);
    WriteColor(cfg, _T("clrTextBack"),    clrTextBack);
    WriteColor(cfg, _T("clrAnsiBack"),    clrAnsiBack);
    WriteColor(cfg, _T("clrUnicodeBack"), clrUnicodeBack);
    WriteColor(cfg, _T("clrGrid"),        clrGrid);
    WriteColor(cfg, _T("clrHighlight"),   clrHighlight);

    cfg.Write(_T("bEvenOddColors"), bEvenOddColors);
    WriteColor(cfg, _T("clrEOText0"), clrEOText[0]);
    WriteColor(cfg, _T("clrEOText1"), clrEOText[1]);
    WriteColor(cfg, _T("clrEOBack0"), clrEOBack[0]);
    WriteColor(cfg, _T("clrEOBack1"), clrEOBack[1]);
}

bool HexWndSettings::ReadColor(wxFileConfig &cfg, wxString name, COLORREF &clr)
{
    wxString tmp;
    if (!cfg.Read(name, &tmp))
        return false;
    return ::ReadColor(MyTokenizer(tmp, wxEmptyString), clr);
}

bool HexWndSettings::WriteColor(wxFileConfig &cfg, wxString name, COLORREF clr)
{
    return cfg.Write(name, wxString::Format(_T("%d %d %d"),
        GetRValue(clr), GetGValue(clr), GetBValue(clr)));
}

BYTE HexWndSettings::GetFontQuality()
{
    if (iFontQuality >= 0)
        return (BYTE)iFontQuality;

    // SPI_GETFONTSMOOTHING is available on Win95 with Plus! pack, and later OSes.
    // SPI_GETFONTSMOOTHINGTYPE is available beginning with Windows XP.
    BOOL bFontSmoothing;
    UINT type;
    if (SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &bFontSmoothing, 0) &&
        bFontSmoothing &&
        SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &type, 0))
    {
        if (type == FE_FONTSMOOTHINGSTANDARD)
            return ANTIALIASED_QUALITY;
        if (type == FE_FONTSMOOTHINGCLEARTYPE)
            return CLEARTYPE_QUALITY;
    }

    return DRAFT_QUALITY; // I think this is reasonable if nothing else works.

    // DEFAULT_QUALITY seems to imply anti-aliasing only if the font renderer
    // already has the anti-aliased font available.
}

void HexWndSettings::SetPalette(wxString pal)
{
    TextPalette = pal;
    palText.SetPalette(TextPalette);
    palSelText = palText;
    palSelText.Nudge(clrSelBack, .2f);
    palModText = palText;
    palModText.Nudge(clrModBack, .2f);
}

//void HexWndSettings::GetColours(const THSIZE &line, uint32 col, uint32 val, wxColour &text, wxColour &back)
//{
//    int tmp;
//    if (iColorByColumns)
//    {
//        tmp = col % iColorByColumns;
//        text = clrText[tmp];
//        back = clrBack[tmp];
//    }
//    else if (iColorByRows)
//    {
//        tmp = line % iColorByRows;
//        text = clrText[tmp];
//        back = clrBack[tmp];
//    }
//    else if (bColorByValues)
//    {
//        text = palText.GetColor(val);
//    }
//    else
//       ;
//}

//*****************************************************************************
//*****************************************************************************
// thAppSettings
//*****************************************************************************
//*****************************************************************************

thAppSettings::thAppSettings()
{
    bInsertMode = true;
    bStatusBar = true;
    bToolBar = false;
    bFileMap = true;
    bDocList = true;
    bNumberView = true;
    bStringView = true;
    bStructureView = true;
    bDisasmView = false;
    bFatView = false;
    bExportView = false;
    bAdjustAddressDigits = true;
    bReverseRGB = true;
    bFullScreen = false;
    bReuseWindow = false;
    bDefaultReadOnly = true;
    bFileShareWrite = false;
    bExactDriveSize = false;

    find.bIgnoreCase = false;
    find.bAllRegions = false;
    //find.bFindBackward = false;
    find.bSelectionOnly = false;
    find.bUnicodeSearch = false;
    find.bNoWrap = false;
    find.data = NULL;
    find.length = 0;

    DisasmOptions = 0;              // default = 32-bit protected mode
#ifdef LIBDISASM_OLD
    DisasmOptions |= LEGACY_MODE;   // 16-bit real mode
    DisasmOptions |= IGNORE_NULLS;  // don't disassemble sequences of > 4 NULLs
#else
    DisasmOptions |= opt_ignore_nulls;
#endif

    sb.bExactFileSize = false;
    sb.bFileSizeInKB = false;
    sb.FileSizeBase = BASE_DEC;
    sb.SelectionBase = BASE_BOTH;
    sb.ValueBase = BASE_ALL;
    sb.bSignedValue = false;

    nRecentChoices = 4;
    nSmartCompareSize = 10 * MEGA;
}

thAppSettings::~thAppSettings()
{
    delete [] find.data;
}

int thAppSettings::GetFindFlags()
{
    int flags = 0;
    if (find.bAllRegions)    flags |= THF_ALLREGIONS;
    //if (find.bFindBackward)  flags |= THF_BACKWARD;
    if (find.bSelectionOnly) flags |= THF_SELONLY;
    if (find.bIgnoreCase)    flags |= THF_IGNORECASE;
    if (find.bNoWrap)        flags |= THF_NOWRAP;
    return flags;
}

void thAppSettings::Load(wxFileConfig &cfg)
{
    cfg.Read(_T("bInsertMode"),    &bInsertMode);
    cfg.Read(_T("bStatusBar"),     &bStatusBar);
    cfg.Read(_T("bToolBar"),       &bToolBar);
    cfg.Read(_T("bFileMap"),       &bFileMap);
    cfg.Read(_T("bDocList"),       &bDocList);
    cfg.Read(_T("bNumberView"),    &bNumberView);
    cfg.Read(_T("bStringView"),    &bStringView);
    cfg.Read(_T("bStructureView"), &bStructureView);
    cfg.Read(_T("bDisasmView"),    &bDisasmView);
    cfg.Read(_T("bFatView"),       &bFatView);
    cfg.Read(_T("bExportView"),    &bExportView);
    cfg.Read(_T("bAdjustAddressDigits"), &bAdjustAddressDigits);
    cfg.Read(_T("bReverseRGB"),    &bReverseRGB);
    cfg.Read(_T("bReuseWindow"),   &bReuseWindow);
    cfg.Read(_T("bDefaultReadOnly"),&bDefaultReadOnly);
    cfg.Read(_T("bFileShareWrite"),&bFileShareWrite);
    cfg.Read(_T("bExactDriveSize"), &bExactDriveSize);
    cfg.Read(_T("DisasmOptions"),  &DisasmOptions);
    cfg.Read(_T("find_bIgnoreCase"),    &find.bIgnoreCase);
    cfg.Read(_T("find_bUnicodeSearch"), &find.bUnicodeSearch);
    cfg.Read(_T("nRecentChoices"), &nRecentChoices);
    cfg.Read(_T("nSmartCompareSize"), &nSmartCompareSize);

    ReadArray(cfg, _T("asGoto"), asGoto, nRecentChoices);
    ReadArray(cfg, _T("asFindText"), asFindText, nRecentChoices);
    ReadArray(cfg, _T("asSpawnCmd"), asSpawnCmd, nRecentChoices);
    ReadArray(cfg, _T("asSpawnArgs"), asSpawnArgs, nRecentChoices);
    ReadArray(cfg, _T("asSpawnPath"), asSpawnPath, nRecentChoices);
}

void thAppSettings::Save(wxFileConfig &cfg)
{
    cfg.Write(_T("bInsertMode"),    bInsertMode);
    cfg.Write(_T("bStatusBar"),     bStatusBar);
    cfg.Write(_T("bToolBar"),       bToolBar);
    cfg.Write(_T("bFileMap"),       bFileMap);
    cfg.Write(_T("bDocList"),       bDocList);
    cfg.Write(_T("bNumberView"),    bNumberView);
    cfg.Write(_T("bStringView"),    bStringView);
    cfg.Write(_T("bStructureView"), bStructureView);
    cfg.Write(_T("bFatView"),       bFatView);
    cfg.Write(_T("bExportView"),    bExportView);
    cfg.Write(_T("bDisasmView"),    bDisasmView);
    cfg.Write(_T("bAdjustAddressDigits"), bAdjustAddressDigits);
    cfg.Write(_T("bReverseRGB"),    bReverseRGB);
    cfg.Write(_T("bReuseWindow"),   bReuseWindow);
    cfg.Write(_T("bDefaultReadOnly"), bDefaultReadOnly);
    cfg.Write(_T("bFileShareWrite"),bFileShareWrite);
    cfg.Write(_T("bExactDriveSize"), bExactDriveSize);
    cfg.Write(_T("DisasmOptions"),  DisasmOptions);
    cfg.Write(_T("find_bIgnoreCase"),find.bIgnoreCase);
    cfg.Write(_T("find_bUnicodeSearch"),find.bUnicodeSearch);
    cfg.Write(_T("nRecentChoices"), nRecentChoices);
    cfg.Write(_T("nSmartCompareSize"), nSmartCompareSize);

    WriteArray(cfg, _T("asGoto"), asGoto, nRecentChoices);
    WriteArray(cfg, _T("asFindText"), asFindText, nRecentChoices);
    WriteArray(cfg, _T("asSpawnCmd"), asSpawnCmd, nRecentChoices);
    WriteArray(cfg, _T("asSpawnArgs"), asSpawnArgs, nRecentChoices);
    WriteArray(cfg, _T("asSpawnPath"), asSpawnPath, nRecentChoices);
}

void WriteArray(wxFileConfig &cfg, wxString name, const wxArrayString &a, int count /*= -1*/)
{
    if (count < 0)
        count = a.GetCount();
    else if (count > (int)a.GetCount())
        count = a.GetCount();

    wxString str;

    for (int i = 0; i < count; i++)
    {
        if (str.Len())
            str += _T("\t");
        str += Escape(a[i].Left(1024));
    }

    cfg.Write(name, str);
}

void ReadArray(wxFileConfig &cfg, wxString name, wxArrayString &a, int count /*= -1*/)
{
    wxString str;
    cfg.Read(name, &str);
    a.Clear();

    wxStringTokenizer tok(str, _T("\t"));
    for (int i = 0; tok.HasMoreTokens() && (count == -1 || i < count); i++)
    {
        a.Add(Unescape(tok.GetNextToken()));
    }
}

