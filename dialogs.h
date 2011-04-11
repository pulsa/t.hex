#pragma once

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include "thex.h"
#include "utils.h"
#include "BarViewCtrl.h"
//#include "toolwnds.h"
#include "palette.h"
#include "clipboard.h"

class HexWnd;
class HexDoc;
class thFrame;
class HexWndSettings;
class thColorRampCtrl;
class thRecentChoice;

// The cancel button must be visible and enabled for ESC to trigger it.
// In fact if it isn't, ESC triggers the default button instead.
// The same thing happens if wxID_CANCEL isn't present at all.
// So we make a button and give it zero size.
// A visible button would probably be better.
#define NEW_HIDDEN_CANCEL_BUTTON(parent) new wxButton( \
    parent, wxID_CANCEL, wxEmptyString, wxPoint(0, 0), wxSize(0, 0))

#ifdef TBDL
class ProcessDialog : public wxDialog
{
public:
    ProcessDialog(wxWindow *parent, wxString caption, bool multiSel = false);
    virtual ~ProcessDialog();
    //int GetPID() { return m_PID; }
    //wxString GetProcName() { return m_procName; }
    int GetPID(int n) { return procList.GetPID(n); }
    wxString GetProcName(int n) { return procList.GetName(n); }
    wxArrayInt GetSelections();
    int GetSelection();
    bool GetReadOnly() { return chkReadOnly->GetValue(); }
//protected:
    //int m_PID;
    //wxString m_procName;
    wxListCtrl *list;
    wxImageList *m_imageList;
    //wxArrayString procNames;
    wxCheckBox *chkReadOnly;
    ProcList procList;
    DECLARE_EVENT_TABLE()
    void OnOK(wxCommandEvent &event);
    void OnItemActivate(wxListEvent &event);
};

class DriveListInfo
{
public:
    int devNum;       
    wxString name;    
    THSIZE offset;    
    THSIZE size;      
    wxString paths;   
    wxString label;   
    wxString fsType;  
    wxString devType; 
    wxString devPath; // passed to CreateFile(); optional column 8
    bool isPartition;

    DriveListInfo(int devNum_ = 0, wxString name_ = wxEmptyString,
        THSIZE offset_ = 0, THSIZE size_ = 0, wxString paths_ = wxEmptyString,
        wxString label_ = wxEmptyString, wxString fsType_ = wxEmptyString,
        wxString devType_ = wxEmptyString, wxString devPath_ = wxEmptyString, bool isPartition_ = false)
        : devNum (devNum_), name (name_), offset (offset_), size (size_), paths (paths_),
          label (label_), fsType (fsType_), devType (devType_), devPath (devPath_)
    {
        isPartition = (offset != 0);
    }
};

class OpenDriveDialog : public wxDialog
{
public:
    OpenDriveDialog(wxWindow *parent);
    virtual ~OpenDriveDialog();
    wxString GetTarget() const { return m_sTarget; }
    bool GetReadOnly() const { return m_bReadOnly; }
    bool GetExactSize() const { return m_bExactSize; }  // stupid name
    void SetItemSizeOffset(int item);  // update offset and size fields of list view

    wxCheckBox *chkReadOnly, *chkExactSize;
    wxListCtrl *lc;

    DECLARE_EVENT_TABLE()
    virtual void OnOK(wxCommandEvent &event);
    virtual void OnActivate(wxListEvent &event);
    virtual void OnExactSize(wxCommandEvent &event);

    enum {IDC_ExactSize = 100};

private:
    wxImageList *m_imgList;
    wxArrayInt MakeDriveImageList();  // returns a list of image list indexes
    std::vector<DriveListInfo> devlist;
    wxString m_sTarget;
    bool m_bReadOnly, m_bExactSize;
};
#endif // TBDL

class GotoDlg : public wxDialog
{
public:
    GotoDlg(HexWnd *hw);
    DECLARE_EVENT_TABLE()
    HexWnd *hw;
    void OnOK(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void OnGetCursor(wxCommandEvent &event);
    void OnAddressChange(wxCommandEvent &event);
    wxLongLong_t GetTarget(int &source);

    // Goto dialog items
    enum {IDC_ADDR_LBL = 100,
         IDC_ADDRESS    ,
         IDC_GET_CURSOR ,
         IDC_EXTEND_SEL ,
         IDC_ORIGIN_BOF ,
         IDC_ORIGIN_CF  ,
         IDC_ORIGIN_CB  ,
         IDC_ORIGIN_EOF ,
    };
};


class GotoDlgPanel: public wxPanel {
public:
    // begin wxGlade: GotoDlgPanel::ids
    // end wxGlade

    GotoDlgPanel(wxWindow *parent, HexWnd* hw, int id = -1, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=0);

    void SetHexWnd(HexWnd *hw) { this->hw = hw; }

private:
    // begin wxGlade: GotoDlgPanel::methods
    void set_properties();
    void do_layout();
    // end wxGlade

    HexWnd *hw;

protected:
    // begin wxGlade: GotoDlgPanel::attributes
    wxStaticText* label_1;
    wxComboBox* cbAddress;
    wxStaticText* label_2;
    wxButton* btnGetCursor;
    wxCheckBox* chkExtendSel;
    wxRadioBox* rbOrigin;
    wxButton* btnGo;
    // end wxGlade

    DECLARE_EVENT_TABLE();

public:
    void OnAddressChange(wxCommandEvent &event); // wxGlade: <event_handler>
    void OnGetCursor(wxCommandEvent &event); // wxGlade: <event_handler>
    void OnGo(wxCommandEvent &event); // wxGlade: <event_handler>
    void OnShow() { cbAddress->SetFocus(); }

    // Goto dialog items
    enum {IDC_ADDR_LBL = 100,
         IDC_ADDRESS    ,
         IDC_GET_CURSOR ,
         IDC_EXTEND_SEL ,
         IDC_ORIGIN_BOF ,
         IDC_ORIGIN_CF  ,
         IDC_ORIGIN_CB  ,
         IDC_ORIGIN_EOF ,
    };
}; // wxGlade: end class

class GotoDlg2 : public wxDialog
{
public:
    GotoDlg2(wxWindow *parent, HexWnd *hw);
    //virtual int ShowModal(HexWnd *hw);

    GotoDlgPanel *m_panel;
};

class FindDialog : public wxDialog
{
public:
    enum {IDC_TEXT = 100, IDC_HEX, IDC_FINDPREV, IDC_UNICODE};
    FindDialog(wxWindow *parent, wxString initText = wxEmptyString, wxString initHex = wxEmptyString);
    void SetHexWnd(HexWnd *hw);
    //virtual int ShowModal(wxWindow *parent = NULL);
    HexWnd *hw;
    HexDoc *doc;
    thRecentChoice *txtText, *txtHex;
    wxArrayString strHexIncludes;
    wxCheckBox *chkUnicode, *chkCaseSensitive, *chkAllRegions;
    bool DoFind(bool forward, bool again = false);
    void OnFind(wxCommandEvent &event);  // same event for forward and backward searches
    void OnTextChange(wxCommandEvent &event);
    void OnHexChange(wxCommandEvent &event);
    void OnUnicodeChange(wxCommandEvent &event);
    void OnInit(wxInitDialogEvent &event);
    bool changing, lastChangedText;
    wxString m_dummy;

    DECLARE_EVENT_TABLE()
};

class MyMessageDialog : public wxDialog
{
public:
    MyMessageDialog(
        wxWindow *parent,
        wxString msg,
        wxString caption,
        int count,
        const wxString *choices,
        int defBtn = -1,
        int orientation = wxVERTICAL);

    void OnButton(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()
};

class ExitSaveDialog : public wxDialog
{
public:
    ExitSaveDialog(thFrame *frame);

    void OnNone(wxCommandEvent &event);
    void OnAll(wxCommandEvent &event);
    void OnReturn(wxCommandEvent &event);
    void OnSaveState(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()

    thFrame *frame;
    enum { ID_SAVE_NONE, ID_SAVE_ALL, ID_SAVE_STATE };
};

class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(thFrame *frame, HexWndSettings *s);
    void InitControls();
    bool UpdateData();
    void UpdatePalette();
    void OnOK(wxCommandEvent &event);
    bool FontChanged(HexWndSettings *old);
    HexWndSettings *ps;
    DECLARE_EVENT_TABLE()

    thColorRampCtrl *ramp;
    //wxListBox *list;
    wxTextCtrl *text;

    wxSpinCtrl      *spnLineBytes;
    wxCheckBox      *chkAdjustLineBytes;
    wxRadioButton   *rbByteOrderLittle, *rbByteOrderBig;
    wxCheckBox      *chkPrettyAddresses;
    wxCheckBox      *chkRelativeAddresses;
    wxCheckBox      *chkRuler;
    wxCheckBox      *chkGridLines;
    wxCheckBox      *chkHighlightModified;
    wxChoice        *cmbFontAntiAlias;
    wxChoice        *cmbFontStyle;
    thRecentChoice  *cmbCodePage;
    wxCheckBox      *chkFontCharsOnly;

    //! You could argue that this stuff belongs to each pane, and should be
    //  a separate class.  Maybe only some of it.
    wxRadioButton   *rbColorColumns, *rbColorRows, *rbColorValues;
    wxSpinCtrl      *spnColumnColors, *spnRowColors;
    wxBitmapComboBox *cmbColumnColors, *cmbRowColors;
    thColorRampCtrl *barValueColors;
    wxTextCtrl      *txtValueColorsRange;
    wxListBox       *lstValueColorRanges;
    wxTextCtrl      *txtRange; // this one is editable
    //! column grouping, group space, column padding... what else in DisplayPane?
    //! Separate font for hex and Unicode panes?  Nah... that's silly...

    enum {
        IDC_SET_FONT = 100,
        IDC_CODEPAGE,
        IDC_BACKGROUND_COLOR,
        IDC_LINE_BYTES,
        IDC_ADJUST_LINE_BYTES,
        IDC_BYTE_ORDER_LITTLE,
        IDC_BYTE_ORDER_BIG,
        IDC_GRID_LINES,
        IDC_PRETTY_ADDRESSES,
        IDC_RELATIVE_ADDRESSES,
        IDC_SHOW_RULER,
        IDC_HIGHLIGHT_MODIFIED,
        IDC_FONT_ANTI_ALIAS,
        IDC_COLOR_COLUMNS,
        IDC_COLOR_LINES,
        IDC_COLOR_VALUES,
        IDC_SPN_COLUMN_COLORS,
        IDC_CMB_COLUMN_COLORS,
        IDC_SPN_ROW_COLORS,
        IDC_CMB_ROW_COLORS,
        IDC_BTN_COL_COLOR,
        IDC_BTN_ROW_COLOR,
        IDC_BAR_VALUE_COLORS,
        IDC_VALUE_COLORS_RANGE,
        IDC_VALUE_COLOR_RANGES,
        IDC_RANGE_TEXT,
        IDC_RANGE_ADD,
        //IDC_RANGE_EDIT, // automatic
        IDC_RANGE_DELETE,
        IDC_RANGE_MOVE_UP,
        IDC_RANGE_MOVE_DOWN,
        IDC_RANGE_SET_ALL,
    };

    void OnBarSelected(wxCommandEvent &event);
    void OnChkAdjustLineBytes(wxCommandEvent &event);
    void OnRangeSelected(wxCommandEvent &event);
    void OnRangeText(wxCommandEvent &event);
    void OnRangeAdd(wxCommandEvent &event);
    //void OnRangeEdit(wxCommandEvent &event);
    void OnRangeDelete(wxCommandEvent &event);
    void OnRangeMoveUp(wxCommandEvent &event);
    void OnRangeMoveDown(wxCommandEvent &event);
    void OnRangeSetAll(wxCommandEvent &event);

private:
    int DisableUpdates;
    friend class UpdateDisabler;
    class UpdateDisabler
    {
    public:
        UpdateDisabler(SettingsDialog *dlg)
        {
            m_dlg = dlg;
            dlg->DisableUpdates++;
        }

        ~UpdateDisabler()
        {
            m_dlg->DisableUpdates--;
        }
        SettingsDialog *m_dlg;
    };
};


class ColorSchemeDialog : public wxDialog
{
public:
    ColorSchemeDialog(thFrame *frame);
    void OnOK(wxCommandEvent &event);
    HexWndSettings *ps;
    HexWnd *hw;
    virtual int ShowModal(HexWnd *hw, HexWndSettings *ps);
    DECLARE_EVENT_TABLE()
};

class StringCollectDialog : public wxDialog
{
public:
    StringCollectDialog(thFrame *parent, HexWnd *hw);
    virtual ~StringCollectDialog();

    int Add(THSIZE start, size_t size);
    int AddW(THSIZE start, size_t size);
    virtual wxString GetTitle() { return _T("Collected Strings"); }

    DECLARE_EVENT_TABLE()
    void OnGo(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);

    void OnSelect(wxListEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void OnHexOffset(wxCommandEvent &event);
    void OnRelativeOffset(wxCommandEvent &event);
    void OnFind(wxCommandEvent &event);
    void OnSave(wxCommandEvent &event);

    enum {IDM_HEXOFFSET = 100, IDM_RELATIVEOFFSET, IDC_FIND, IDC_SAVE};

    wxListCtrl *list;
    wxTextCtrl *txtMin, *txtMax, *txtLimit, *txtLetters;
    wxButton *btnGo;
    wxCheckBox *chkTerminator;
    wxMenu *popup;
    bool bHexOffset, bRelativeOffset;
    wxCheckBox *chkUnicode, *chkANSI, *chkCaseSens, *chkWeirdnessFilter, *chkSelOnly;
    wxTextCtrl *txtCharSet, *txtDebug;
    wxNotebook *notebook;

    void RedoOffsets();
    void AutoSizeColumns();

public:
    wxString FormatOffset(THSIZE offset);

    class T_StringInfo
    {
    public:
        THSIZE start;
        size_t size; // Character count.  Bit 31 indicates unicode
        T_StringInfo(THSIZE start_ = 0, size_t size_ = 0, bool isUnicode = 0)
            : start(start_), size(size_ | (isUnicode ? (1 << 31) : 0)) { }
        bool IsUnicode() const { return (size & 0x80000000) != 0; }
        size_t GetBytes() const  { return GetChars() * (IsUnicode() ? 2 : 1); }
        size_t GetChars() const { return size & 0x7fffffff; }
    };

    //std::vector<THSIZE> stringOffsets;
    //std::vector<T_StringInfo> stringOffsets;
    FastAppendVector<T_StringInfo, 1024> stringOffsets;
    HexWnd *m_hw;
    wxString m_strMin, m_strMax, m_strCount, m_strLetters;
    size_t m_minSize, m_maxSize;
    size_t m_StringCount, m_minLetters, m_limit;
    size_t m_longestString8, m_longestLength8;
    size_t m_longestString16, m_longestLength16;
    THSIZE totalLength8, totalLength16;
    size_t count8, count16;
    uint8 isLetter[256];
    bool m_bFilter;

    enum {WEIRDNESS_LIMIT = 4};  // maximum repeated characters, if "weirdness filter" selected.
};


class CharSetDialog : public wxDialog
{
public:
   CharSetDialog(wxWindow *parent);

   DECLARE_EVENT_TABLE()
};


class CopyFormatPanel : public wxPanel
{
public:
    CopyFormatPanel(wxWindow* parent);
    //thCopyFormat fmt;
    thCopyFormat GetFormat();

    wxChoice   *cmbFormat;
    wxComboBox *cmbBase;
    wxComboBox *cmbWord;
    wxComboBox *cmbWords;
    wxChoice   *cmbEndian;
    wxChoice   *cmbRTF;
    wxChoice   *cmbCharset;
    wxTextCtrl *txtPreview;
    //DECLARE_EVENT_TABLE()
};

class CopyFormatDialog : public wxDialog
{
public:
    CopyFormatDialog(wxWindow* parent);

    //thCopyFormat* m_pFormat;
    CopyFormatPanel *formatPanel;
    thCopyFormat GetFormat() { return formatPanel->GetFormat(); }

    //DECLARE_EVENT_TABLE()
};

class PasteFormatDialog : public wxDialog
{
public:
    PasteFormatDialog(wxWindow* parent);

    enum {IDC_FIRST = 0,
        IDC_AUTO = IDC_FIRST, IDC_RAW, IDC_ASCII, IDC_UTF8, IDC_UTF16, IDC_UTF32,
        IDC_ESCAPED, IDC_HEX,
        IDC_LAST};

    //wxRadioButton *rbAuto;
    //wxRadioButton *rbRaw;
    //wxRadioButton *rbASCII;
    //wxRadioButton *rbUTF8;
    //wxRadioButton *rbUTF16;
    //wxRadioButton *rbUTF32;
    //wxRadioButton *rbPython;
    //wxRadioButton *rbHex;

    wxRadioBox *rbFormat;

    wxChoice *cmbCodePage;

    //void OnSelection(wxCommandEvent &event);
    int GetSelection() { return rbFormat->GetSelection(); }
    //DECLARE_EVENT_TABLE()
};

//****************************************************************************
//****************************************************************************
// Histogram
//****************************************************************************
//****************************************************************************

class HistogramDialog;
class HistogramView;

class CHistogram
{
public:
    THSIZE m_count[256]; // count of each byte value
    THSIZE m_highest;    // highest byte value (not index)
    THSIZE m_nSize;      // total size of data, or sum of all counts
    int dispOrder[256];  // order in which the items are displayed

    CHistogram();

    bool Calc(HexWnd *hw, HistogramView *pView = NULL);
};

class HistogramView : public thBarViewCtrl
{
public:
    HistogramView(wxWindow *parent, HexWnd *hw, wxSize size = wxDefaultSize);
    virtual ~HistogramView();

    virtual int HitTest(wxPoint pt);
    virtual int GetBarHeight(int sel);
    virtual int GetBarCount() { return 256; }
    virtual wxColour GetBarColour(int bar);
    virtual bool BeginDraw();

    void OnContextMenu(wxContextMenuEvent &event);
    void OnLogScale(wxCommandEvent &event);
    void OnSnapToPeak(wxCommandEvent &event);
    DECLARE_EVENT_TABLE()

    enum {IDM_LogScale, IDM_SnapToPeak};

public:
    CHistogram *pHist;
    bool m_bLogScale, m_bSnapToPeak;
    double m_dScale;
    thPalette m_pal;
    wxColour m_clrEOText[2];
    bool m_bEvenOddColors;
};

class HistogramDialog : public wxDialog
{
public:
    HistogramDialog(HexWnd *hw);
    CHistogram hgram;
    HistogramView *pHistView;
    wxListCtrl *pList;
    int sortColumn;

    DECLARE_EVENT_TABLE()
    void OnListItemSelected(wxListEvent &event);
    void OnBarSelected(wxCommandEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void OnColumnClick(wxListEvent &event);
};

#ifdef TBDL
class FontWidthChooserDialog : public wxDialog
{
public:
    FontWidthChooserDialog(wxWindow *parent, LOGFONT &lf);
    void TestFont(int sel);

    wxListBox *list;
    wxStaticText *sample;
    wxArrayInt sizes;
    LOGFONT &m_lf;

    static int CALLBACK EnumFontFamExProc(
        ENUMLOGFONTEX *lpelfe,    // logical-font data
        NEWTEXTMETRICEX *lpntme,  // physical-font data
        DWORD FontType,           // type of font
        LPARAM lParam);           // application-defined data

    static int CmpFunc(int *first, int *second)
    {
        return *first - *second;
    }

    void OnOK(wxCommandEvent &event);
    void OnSelection(wxCommandEvent &event) { TestFont(event.GetSelection()); }
    DECLARE_EVENT_TABLE()
};
#endif //TBDL

class thRecentChoice : public wxComboBox
{
public:
    thRecentChoice(wxWindow *parent, wxWindowID id,
        wxArrayString &choices, wxString value = wxEmptyString, int nMax = -1,
        wxPoint position = wxDefaultPosition,
        wxSize size = wxDefaultSize,
        const wxValidator& validator = wxDefaultValidator);

    thRecentChoice(wxWindow *parent, wxWindowID id,
        wxArrayString &choices, int selection, int nMax = -1,
        wxPoint position = wxDefaultPosition,
        wxSize size = wxDefaultSize,
        const wxValidator& validator = wxDefaultValidator);

    virtual void Finalize();
    virtual ~thRecentChoice() { if (m_bSave) Finalize(); }

    virtual void ShouldSave(bool shouldSave) { m_bSave = shouldSave; }
    virtual bool ShouldSave() { return m_bSave; }

    virtual wxSize DoGetBestSize() const;

protected:
    wxArrayString &m_choices;
    int m_nMax;
    bool m_bSave;

    void OnTextChange(wxCommandEvent &event);
    void OnChar(wxKeyEvent &event);
    void OnEnter(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()
};

class thRecentChoiceDialog : public wxDialog
{
public:
    thRecentChoiceDialog(wxWindow *parent,
        wxString title, wxString message,
        wxArrayString &choices, int nMax = -1);

    void OnOK(wxCommandEvent &event);
    //void OnTextChange(wxCommandEvent &event);

    thRecentChoice *cmb;
    //wxArrayString &m_choices;
    //int m_nMax;

    DECLARE_EVENT_TABLE()
};

class ModifyBuffer;
#ifdef TBDL
class SlowReadDialog : public wxDialog
{
public:
    SlowReadDialog(wxWindow *parent, HANDLE hFile);
    ~SlowReadDialog();
    void EndModal(int retCode)
    {
        CancelIo(hFile);
        wxDialog::EndModal(retCode);
    }

    void OnTimer(wxTimerEvent &event);

    void OnComplete(
        DWORD dwErrorCode,
        DWORD dwNumberOfBytesTransfered,
        LPOVERLAPPED lpOverlapped);

private:
    HANDLE hFile;
    ModifyBuffer *fwb;
    OVERLAPPED overlapped;

    wxStaticText *label1, *label2;
    wxTimer timer;
    wxDateTime dtStart;

    DECLARE_EVENT_TABLE()
};

class Selection;

class thPipeOutDialog : public wxDialog
{
public:
    thPipeOutDialog(wxWindow *parent);
    virtual ~thPipeOutDialog() { }  //! need this so ~thRecentChoice gets called?!
    void Init(const Selection sel);

    wxString GetFile()   { return chkShell->GetValue() ? m_sShell : txtFile->GetValue(); }
    wxString GetArgs()   { return (chkShell->GetValue() ? m_sArgPrefix : ZSTR) + txtArgs->GetValue(); }
    wxString GetDir()    { return txtDir->GetValue(); }
    bool PipeToProcess() { return rgWhither->GetSelection() == 0; }
    bool SelectionOnly() { return rgWhat->GetSelection() == 1; }
    bool DefaultShell()  { return chkShell->GetValue(); }

    wxCheckBox *chkShell;  // if checked, use "cmd.exe /c", "/bin/bash", or whatever.
    thRecentChoice *txtFile, *txtArgs, *txtDir;
    wxRadioBox *rgWhat; // radio group for selection/all
    wxRadioBox *rgWhither; // send to process or file?

    enum { IDC_SHELL = wxID_HIGHEST + 1 };
    //void OnOK(wxCommandEvent &event);
    void OnChkShell(wxCommandEvent &event);
    DECLARE_EVENT_TABLE()

    wxString m_sShell, m_sArgPrefix;
};
#endif // TBDL

class AboutDialog : public wxDialog
{
public:
    AboutDialog(wxWindow *parent);
};

#endif // _DIALOGS_H_
