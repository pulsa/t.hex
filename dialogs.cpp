#include "precomp.h"
#include "dialogs.h"
#include "thFrame.h"
#include "resource.h"
#include "hexwnd.h"
#include "thex.h"
#include "settings.h"
#include "BarViewCtrl.h"
#include "palette.h"
#include "datasource.h"
//#include "physicaldrive.h"
#include "blockdevs.h"

#define new New

BEGIN_EVENT_TABLE(ProcessDialog, wxDialog)
    EVT_BUTTON(wxID_OK, OnOK)
    EVT_LISTBOX_DCLICK(-1, OnOK)
    EVT_LIST_ITEM_ACTIVATED(-1, OnItemActivate)
END_EVENT_TABLE()

ProcessDialog::ProcessDialog(wxWindow *parent, wxString caption, bool multiSel /*= false*/)
: wxDialog(parent, -1, caption, wxDefaultPosition, wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    //m_PID = -1;

    wxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    int style = wxLC_REPORT;
    if (!multiSel)
        style |= wxLC_SINGLE_SEL;
    list = new wxListCtrl(this, -1, wxDefaultPosition, wxDefaultSize, style);
    list->SetMinSize(wxSize(200, 100));
    //list->SetBestFittingSize(wxSize(400, 200));
    //list->SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));
    topSizer->Add(list, 1, wxGROW | wxALL, 10);
    wxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *btnOK = new wxButton(this, wxID_OK, _T("&OK"));
    //SetDefaultItem(btnOK);
    chkReadOnly = new wxCheckBox(this, -1, _T("&Read only"));
    chkReadOnly->SetValue(appSettings.bDefaultReadOnly);
    buttonSizer->Add(chkReadOnly, 1, wxALL, 5);
    btnOK->SetDefault();
    buttonSizer->Add(btnOK, 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, _T("&Cancel")), 0, wxALL, 5);
    //topSizer->Add(buttonSizer, 0, wxALIGN_RIGHT);
    topSizer->Add(buttonSizer, 0, wxGROW | wxALL & ~wxTOP, 5);

    //! This whole HICON thing is rather flimsy.  Beware!
    m_imageList = procList.MakeImageList();
#if wxCHECK_VERSION(2, 8, 0)  // I think that's when this changed...
    list->AssignImageList(m_imageList, wxIMAGE_LIST_SMALL);
#else
    list->SetImageList(m_imageList, wxIMAGE_LIST_SMALL);
#endif

    list->InsertColumn(0, _T("Process"));
    list->InsertColumn(1, _T("PID"));
    list->InsertColumn(2, _T("Window"));
    list->InsertColumn(3, _T("Description"));
    list->InsertColumn(4, _T("Path"));
    for (size_t n = 0; n < procList.GetCount(); n++)
    {
        //list->Append(wxString::Format("%8d  %s", procList.GetPID(n), procList.GetName(n)), (void*)n);
        list->InsertItem(n, procList.GetName(n), procList.GetIcon(n));
        //list->InsertItem(n, procList.GetName(n), n);
        list->SetItem(n, 1, wxString::Format(_T("%d"), procList.GetPID(n)));
        list->SetItem(n, 2, procList.GetTitle(n));
        list->SetItem(n, 3, procList.GetDescription(n));
        list->SetItem(n, 4, procList.GetPath(n));
    }
    list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
    list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
    //list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER); // too long
    //list->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
    //list->SetBestFittingSize(wxSize(400, 200)); //! could use contents to pick size
    list->SetInitialSize(wxSize(400, 200)); //! could use contents to pick size

    SetSizerAndFit(topSizer);
}

ProcessDialog::~ProcessDialog()
{
#if !wxCHECK_VERSION(2, 8, 0)
    delete m_imageList;
#endif
}

void ProcessDialog::OnOK(wxCommandEvent &WXUNUSED(event))
{
    //int sel = list->GetSelection();
    //if (sel >= 0)
    //{
    //    int proc = (int)list->GetClientData(sel);
    //    m_procName = procList.GetName(proc);
    //    m_PID = procList.GetPID(proc);
    //}
    EndModal(wxID_OK);
}

void ProcessDialog::OnItemActivate(wxListEvent &event)
{
   EndModal(wxID_OK);
}

wxArrayInt ProcessDialog::GetSelections()
{
    wxArrayInt sels;
    long item = -1;
    for (;;)
    {
        item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1)
            break;
        sels.Add(item);
    }
    return sels;
}

int ProcessDialog::GetSelection()
{
    return list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}


//****************************************************************************
//****************************************************************************
// MyMessageDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(MyMessageDialog, wxDialog)
    EVT_COMMAND_RANGE(wxID_HIGHEST, wxID_HIGHEST + 1000, wxEVT_COMMAND_BUTTON_CLICKED, OnButton)
END_EVENT_TABLE()

MyMessageDialog::MyMessageDialog(
    wxWindow *parent,
    wxString msg,
    wxString caption,
    int count,
    const wxString *choices,
    int defBtn /*= -1*/,
    int orientation /*= wxVERTICAL*/)
{
    wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *txtMsg = new wxStaticText(this, -1, caption);
    int width = wxMin(parent->GetSize().x, wxSystemSettings::GetMetric(wxSYS_SCREEN_X, parent));
#if wxCHECK_VERSION(2, 6, 2)
    txtMsg->Wrap(width / 2); // new feature in wxWidgets 2.6.2 -- cool!
#endif
    topsizer->Add(txtMsg, 0, wxALL, 10);
    wxSizer *buttonSizer = new wxBoxSizer(orientation);
    for (int i = 0; i < count; i++)
    {
        wxButton *btn = new wxButton(this, wxID_HIGHEST + i, choices[i]);
        if (i == defBtn)
        {
            //SetDefaultItem(btn);
            btn->SetDefault();
        }
        buttonSizer->Add(btn, 0, wxGROW | wxALL, 10);
    }
}

void MyMessageDialog::OnButton(wxCommandEvent &event)
{
    EndModal(event.GetId() - wxID_HIGHEST);
}


//****************************************************************************
//****************************************************************************
// ExitSaveDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(ExitSaveDialog, wxDialog)
    EVT_BUTTON(ID_SAVE_NONE, OnNone)
    EVT_BUTTON(ID_SAVE_ALL, OnAll)
    EVT_BUTTON(wxID_CANCEL, OnReturn)
    EVT_BUTTON(ID_SAVE_STATE, OnSaveState)
END_EVENT_TABLE()

ExitSaveDialog::ExitSaveDialog(thFrame *frame)
: wxDialog(frame, -1, wxString(_T("need a title")))
{
    this->frame = frame;
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, -1, _T("Need some text")), 0, wxALL, 10);
    sizer->Add(new wxButton(this, ID_SAVE_NONE, _T("Save &None")), 0, wxALL, 10);
    sizer->Add(new wxButton(this, ID_SAVE_ALL, _T("Save &All")), 0, wxALL, 10);
    sizer->Add(new wxButton(this, wxID_CANCEL, _T("&Cancel")), 0, wxALL, 10);
    sizer->Add(new wxButton(this, ID_SAVE_STATE, _T("Save &State")), 0, wxALL, 10);
    SetSizerAndFit(sizer);
    CenterOnParent();
}

void ExitSaveDialog::OnNone(wxCommandEvent &event)
{
    EndModal(1);
}

void ExitSaveDialog::OnAll(wxCommandEvent &event)
{
    frame->SaveAll();
    EndModal(1);
}

void ExitSaveDialog::OnReturn(wxCommandEvent &event)
{
    EndModal(0);
}

void ExitSaveDialog::OnSaveState(wxCommandEvent &event)
{
    frame->SaveState();
    EndModal(1);
}


//****************************************************************************
//****************************************************************************
// GotoDlg
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(GotoDlg, wxDialog)
    //EVT_BUTTON(wxID_OK, OnOK)
    //EVT_BUTTON(wxID_CANCEL, OnCancel)
    //EVT_BUTTON(IDC_GET_CURSOR, OnGetCursor)
    //EVT_TEXT(IDC_ADDRESS, OnAddressChange)
END_EVENT_TABLE()

GotoDlg::GotoDlg(HexWnd *hw)
: wxDialog(hw, -1, wxString(_T("Go To Address")))
{
}

GotoDlgPanel::GotoDlgPanel(wxWindow* parent, HexWnd *hw, int id, const wxPoint& pos, const wxSize& size, long style):
    wxPanel(parent, id, pos, size, wxTAB_TRAVERSAL)
{
    this->hw = hw;

    // begin wxGlade: GotoDlgPanel::GotoDlgPanel
    label_1 = new wxStaticText(this, IDC_ADDR_LBL, wxT("&Address"));
    //cbAddress = new wxComboBox(this, IDC_ADDRESS, wxT(""), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN);
    cbAddress = new thRecentChoice(this, IDC_ADDRESS, appSettings.asGoto);
    label_2 = new wxStaticText(this, -1, wxT("Prefix with \"+\", \"-\", or \"x\""));
    btnGetCursor = new wxButton(this, IDC_GET_CURSOR, wxT("Get &Cursor Position"));
    chkExtendSel = new wxCheckBox(this, -1, wxT("E&xtend Selection"));
    const wxString rbOrigin_choices[] = {
        wxT("&Absolute address"),
        wxT("&Beginning of file"),
        wxT("Cursor (&forward)"),
        wxT("Cursor (bac&kward)"),
        wxT("&End of file")
    };
    rbOrigin = new wxRadioBox(this, -1, wxT("&Origin"), wxDefaultPosition, wxDefaultSize, 5, rbOrigin_choices, 0, wxRA_SPECIFY_ROWS);
    btnGo = new wxButton(this, wxID_OK, wxT("&Go"));
    NEW_HIDDEN_CANCEL_BUTTON(this);

    set_properties();
    do_layout();
    // end wxGlade
}


BEGIN_EVENT_TABLE(GotoDlgPanel, wxPanel)
    // begin wxGlade: GotoDlgPanel::event_table
    EVT_TEXT(IDC_ADDRESS, GotoDlgPanel::OnAddressChange)
    EVT_BUTTON(IDC_GET_CURSOR, GotoDlgPanel::OnGetCursor)
    EVT_BUTTON(wxID_OK, GotoDlgPanel::OnGo)
    // end wxGlade
END_EVENT_TABLE();


void GotoDlgPanel::OnAddressChange(wxCommandEvent &event)
{
    event.Skip();
}


void GotoDlgPanel::OnGetCursor(wxCommandEvent &WXUNUSED(event))
{
    cbAddress->SetValue(wxString::Format(_T("%I64x"), hw->GetCursor()));  //! does this I64 work?
}


void GotoDlgPanel::OnGo(wxCommandEvent &WXUNUSED(event))
{
    //((wxDialog*)GetParent())->EndModal(1);
    //__int64 address = _atoi64(cbAddress->GetValue());
    uint64 address = 0;
    //sscanf(cbAddress->GetValue(), "%I64x", &address);
    if (!ReadUserNumber(cbAddress->GetValue(), address))
    {
        wxMessageBox(wxString::Format(_T("I don't understand %s."), cbAddress->GetValue().c_str()));
        return;
    }
    if (!hw->Goto(address, chkExtendSel->GetValue(), rbOrigin->GetSelection()))
    {
        wxMessageBox(wxString::Format(_T("Couldn't seek to %s."), cbAddress->GetValue().c_str()));
        return;
    }
    //GetParent()->Close();  //! This is good.
    ((wxDialog*)GetParent())->EndModal(wxID_OK); //! This is bad.
}


// wxGlade: add GotoDlgPanel event handlers


void GotoDlgPanel::set_properties()
{
    // begin wxGlade: GotoDlgPanel::set_properties
    rbOrigin->SetSelection(0);
    // end wxGlade
}

#define wxADJUST_MINSIZE 0

void GotoDlgPanel::do_layout()
{
    // begin wxGlade: GotoDlgPanel::do_layout
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
    vSizer->Add(label_1, 0, wxADJUST_MINSIZE, 10);
    vSizer->Add(cbAddress, 0, wxTOP|wxBOTTOM|wxEXPAND|wxADJUST_MINSIZE, 10);
    vSizer->Add(label_2, 0, wxADJUST_MINSIZE, 0);
    vSizer->Add(btnGetCursor, 0, wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 10);
    vSizer->Add(chkExtendSel, 0, wxADJUST_MINSIZE, 10);
    hSizer->Add(vSizer, 1, wxALL|wxEXPAND, 10);
    hSizer->Add(rbOrigin, 0, wxALL|wxADJUST_MINSIZE, 10);
    topSizer->Add(hSizer, 1, wxEXPAND, 0);
    topSizer->Add(btnGo, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxADJUST_MINSIZE, 10);

    //SetDefaultItem(btnGo);  // Can't do this in wx 2.7.0.  It didn't really work before, anyway.
    btnGo->SetDefault(); // This is better.

    SetAutoLayout(true);
    SetSizer(topSizer);
    topSizer->Fit(this);
    topSizer->SetSizeHints(this);
    // end wxGlade
}

GotoDlg2::GotoDlg2(wxWindow *parent, HexWnd *hw)
: wxDialog(parent, -1, wxString(_T("Go to address")))
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    m_panel = new GotoDlgPanel(this, hw);
    sizer->Add(m_panel);
    //SetEscapeId(wxID_CANCEL); //! weirdness
    SetSizerAndFit(sizer);
}

//int GotoDlg2::ShowModal(HexWnd *hw)
//{
//    m_panel->SetHexWnd(hw);
//    m_panel->OnShow();
//    return wxDialog::ShowModal();
//}


//****************************************************************************
//****************************************************************************
// FindDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(FindDialog, wxDialog)
    EVT_BUTTON(wxID_OK, OnFind)
    EVT_BUTTON(IDC_FINDPREV, OnFind)
    EVT_TEXT(IDC_TEXT, OnTextChange)
    EVT_TEXT(IDC_HEX, OnHexChange)
    EVT_CHECKBOX(IDC_UNICODE, OnUnicodeChange)
    EVT_INIT_DIALOG(OnInit)
END_EVENT_TABLE()

FindDialog::FindDialog(wxWindow *parent, wxString initText /*= wxEmptyString*/, wxString initHex /*= wxEmptyString*/)
: wxDialog(parent, -1, wxString(_T("Find and Replace")))
{
    wxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    const TCHAR hexChars[] = _T(" 0123456789ABCDEFabcdef");
    for (int i = 0; hexChars[i]; i++)
        strHexIncludes.Add(hexChars[i]);
    wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST, &m_dummy); // wx 2.6.1 needs a variable here
    validator.SetIncludes(strHexIncludes);
    int cw;

    {
        wxClientDC dc(this);
        dc.SetFont(GetFont());
        cw = dc.GetCharWidth();
    }

    txtText = new thRecentChoice(this, IDC_TEXT, appSettings.asFindText, initText, 0, wxDefaultPosition, wxSize(50 * cw, -1));
    txtHex  = new thRecentChoice(this, IDC_HEX, appSettings.asFindHex, initHex, 0, wxDefaultPosition, wxSize(50 * cw, -1), validator);
    changing = false;

    wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2, 10, 10);
    gridSizer->Add(new wxStaticText(this, -1, _T("&Text")), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(txtText);
    gridSizer->Add(new wxStaticText(this, -1, _T("&Hex")), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(txtHex);
    topSizer->Add(gridSizer, 0, wxALL, 10);

    chkUnicode = new wxCheckBox(this, IDC_UNICODE, _T("&Unicode"));
    chkUnicode->SetValue(appSettings.find.bUnicodeSearch);

    chkCaseSensitive = new wxCheckBox(this, -1, _T("Match &case"));
    chkCaseSensitive->SetValue(!appSettings.find.bIgnoreCase);

    chkAllRegions = new wxCheckBox(this, -1, _T("Search all &regions"));
    chkAllRegions->SetValue(appSettings.find.bAllRegions);

    gridSizer = new wxFlexGridSizer(2, 10, 10);
    gridSizer->AddGrowableCol(0, 1);
    wxSizer *checkSizer = new wxBoxSizer(wxVERTICAL);
    checkSizer->Add(chkUnicode, 0, wxALL, cw);
    checkSizer->Add(chkCaseSensitive, 0, wxALL, cw);
    checkSizer->Add(chkAllRegions, 0, wxALL, cw);
    gridSizer->Add(checkSizer, 1);

    gridSizer->Add(new wxButton(this, wxID_CANCEL, _T("Cancel"), wxDefaultPosition, wxSize(20 * cw, -1)), 0, wxALIGN_BOTTOM);
    wxButton *btnFind = new wxButton(this, wxID_OK, _T("&Find Next"), wxDefaultPosition, wxSize(25 * cw, -1));
    //SetDefaultItem(btnFind);
    btnFind->SetDefault(); //! does this work?
    gridSizer->Add(btnFind, 0, wxALIGN_RIGHT);
    gridSizer->Add(new wxButton(this, IDC_FINDPREV, _T("Find &Previous"), wxDefaultPosition, wxSize(20 * cw, -1)));
    topSizer->Add(gridSizer, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);
    SetSizerAndFit(topSizer);

    lastChangedText = true;
    //txtText->ShouldSave(false);
    txtHex->ShouldSave(false);
}

void FindDialog::SetHexWnd(HexWnd *hw)
{
    this->hw = hw;
    this->doc = hw->doc;
}

void FindDialog::OnInit(wxInitDialogEvent &event)
{
    txtText->SetFocus();
    txtText->SetSelection(-1, -1);
}

void FindDialog::OnFind(wxCommandEvent &event)
{
    ATimer timer;
    timer.StartWatch();
    bool forward = (event.GetId() == wxID_OK);
    DoFind(forward);
    timer.StopWatch();
    PRINTF(_T("Find took %0.3f ms\n"), timer.GetSeconds() * 1000);
    EndModal(event.GetId());
    if (lastChangedText)
        txtText->Finalize();
    else
        txtHex->Finalize();
}

bool FindDialog::DoFind(bool forward, bool again /*= false*/)
{
    wxString hex = StripSpaces(txtHex->GetValue());
    if (hex.Len() % 2 != 0)
    {
        wxMessageBox(_T("Invalid hex string"), _T("T. Hex"), wxOK | wxICON_ERROR, this);
        return false;
    }

    appSettings.find.bUnicodeSearch = chkUnicode->GetValue();
    appSettings.find.bIgnoreCase = !chkCaseSensitive->GetValue();
    //appSettings.find.bFindBackward = !forward;
    appSettings.find.bAllRegions = chkAllRegions->GetValue();

    delete [] appSettings.find.data;
    appSettings.find.length = hex.Len() / 2;
    appSettings.find.data = new uint8[hex.Len() / 2];
    HexToText(hex, appSettings.find.data, appSettings.find.length);

    int flags = appSettings.GetFindFlags();
    if (again)
        flags |= THF_AGAIN;
    if (!forward)
        flags |= THF_BACKWARD;
    return hw->DoFind(appSettings.find.data, appSettings.find.length, flags);
}

void FindDialog::OnTextChange(wxCommandEvent &event)
{
    if (changing) return;
    changing = true;
    wxString str = txtText->GetValue();
#ifdef _UNICODE
    if (chkUnicode->GetValue())
        txtHex->SetValue(TextToHex(str));  // use internal Unicode encoding
    else  // convert to UTF-8
        txtHex->SetValue(TextToHex((uint8*)str.mb_str(wxConvLibc).data(), str.Len()));
#else
    if (chkUnicode->GetValue())  // fake an 8-bit string with Unicode data
        str = wxString((const char*)str.wc_str(wxConvLibc).data(), str.Len() * 2);
    txtHex->SetValue(TextToHex(str));
#endif
    changing = false;
    lastChangedText = true;
}

void FindDialog::OnHexChange(wxCommandEvent &event)
{
    if (changing) return;
    changing = true;
    wxString hex = StripSpaces(txtHex->GetValue());
    wxString text;
#ifdef _UNICODE
    if (chkUnicode->GetValue()) {
        text = HexToText(hex);
    }
    else {
        uint8 *buf = new uint8[hex.Len() / 2 + 1];
        HexToText(hex, buf, hex.Len() / 2);
        buf[hex.Len() / 2] = 0;
        text = wxString((char*)buf, wxConvLibc, hex.Len() / 2 + 1);
        delete [] buf;
    }
#else
    if (chkUnicode->GetValue())
    {
        wchar_t *buf = new wchar_t[hex.Len() / 4 + 1];
        HexToText(hex, (uint8*)buf, hex.Len() / 2);
        buf[hex.Len() / 2] = 0;
        text = wxString(buf, wxConvLibc, hex.Len() / 4 + 1);
        delete [] buf;
    }
    else
        txtText->SetValue(HexToText(txtHex->GetValue()));
#endif
    txtText->SetValue(text);
    changing = false;
    lastChangedText = false;
}

void FindDialog::OnUnicodeChange(wxCommandEvent &event)
{
    OnTextChange(event);
}

//****************************************************************************
//****************************************************************************
// thColorRampCtrl
//****************************************************************************
//****************************************************************************

class thColorRampCtrl : public thBarViewCtrl
{
public:
    thColorRampCtrl(wxWindow *parent, int id = -1,
        wxPoint pos = wxDefaultPosition,
        wxSize size = wxDefaultSize,
        wxString title = _T("thColorRampCtrl"))
        : thBarViewCtrl(parent, id, pos, size, title)
    {
        //UpdateView(false);
    }

    virtual int GetBarHeight(int sel) { return m_rcBorder.height; }
    virtual wxColour GetBarColour(int sel)
    {
        if (sel < 0 || sel > 255)
            return *wxBLACK;
        return pal.GetColor((size_t)sel);
    }
    thPalette pal;

protected:
};

//****************************************************************************
//****************************************************************************
// SettingsDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, OnOK)
    EVT_COMMAND(-1, wxEVT_BAR_SELECTED, OnBarSelected)
    EVT_CHECKBOX(IDC_ADJUST_LINE_BYTES, OnChkAdjustLineBytes)
    EVT_LISTBOX(IDC_VALUE_COLOR_RANGES, OnRangeSelected)
    EVT_TEXT(IDC_RANGE_TEXT, OnRangeText)
    EVT_BUTTON(IDC_RANGE_ADD, OnRangeAdd)
    //EVT_BUTTON(IDC_RANGE_EDIT, OnRangeEdit)
    EVT_BUTTON(IDC_RANGE_DELETE, OnRangeDelete)
    EVT_BUTTON(IDC_RANGE_MOVE_UP, OnRangeMoveUp)
    EVT_BUTTON(IDC_RANGE_MOVE_DOWN, OnRangeMoveDown)
    EVT_BUTTON(IDC_RANGE_SET_ALL, OnRangeSetAll)
END_EVENT_TABLE()

SettingsDialog::SettingsDialog(thFrame *frame, HexWndSettings *s)
: wxDialog(frame, -1, wxString(_T("Settings")),
           wxDefaultPosition, wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    this->ps = s;
    DisableUpdates = 0;

    wxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    wxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    hSizer->Add(new wxButton(this, wxID_CANCEL, _T("&Cancel")), 0, wxALL, 10);
    wxButton *btnOK = new wxButton(this, wxID_OK, _T("&OK"));
    btnOK->SetDefault();
    hSizer->Add(btnOK, 0, wxALL, 10);
    topSizer->Add(hSizer);

    ramp = new thColorRampCtrl(this, -1, wxDefaultPosition, wxSize(268, 22));
    ramp->SetBackgroundColour(s->clrWndBack);
    //ramp->pal.SetPalette(s->TextPalette);
    ramp->pal = s->palText;
    ramp->UpdateView(false);
    //topSizer->Add(ramp, 1, wxGROW);

    //wxSizer *szr = new wxFlexGridSizer(2, 10, 10);
    //topSizer->Add(szr, 0, wxALL, 10);
    wxSizer *szr = topSizer;
    hSizer = new wxBoxSizer(wxHORIZONTAL);

    const int pad = 5;

    // Number of bytes per line, plus auto-adjust checkbox
    hSizer->Add(new wxStaticText(this, -1, _T("Bytes per line")), 0, wxALL | wxALIGN_CENTER_VERTICAL, pad);
    spnLineBytes = new wxSpinCtrl(this, IDC_LINE_BYTES,
        wxString::Format(_T("%d"), ps->iLineBytes),
        wxDefaultPosition, wxDefaultSize,
        wxSP_ARROW_KEYS,
        1, 1024*64, ps->iLineBytes);  // 64KB per line is arbitrary, but it should be big enough.  :)
    hSizer->Add(spnLineBytes, 0, wxALL, pad);
    chkAdjustLineBytes = new wxCheckBox(this, IDC_ADJUST_LINE_BYTES, _T("Adjust to fit window?"));
    hSizer->Add(chkAdjustLineBytes, 0, wxALL | wxALIGN_CENTER_VERTICAL, pad);
    szr->Add(hSizer);

    // Byte order, little- or big-endian
    hSizer = new wxBoxSizer(wxHORIZONTAL);
    hSizer->Add(new wxStaticText(this, -1, _T("Byte order")), 0, wxALL, pad);
    rbByteOrderLittle = new wxRadioButton(this, -1, _T("Little-endian"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rbByteOrderBig = new wxRadioButton(this, -1, _T("Big-endian"));
    hSizer->Add(rbByteOrderLittle, 0, wxALL, pad);
    hSizer->Add(rbByteOrderBig, 0, wxALL, pad);
    szr->Add(hSizer);

    // Other yes/no display options
    chkPrettyAddresses = new wxCheckBox(this, -1, _T("Pretty addresses"));
    chkRelativeAddresses = new wxCheckBox(this, -1, _T("Relative addresses"));
    chkRuler = new wxCheckBox(this, -1, _T("Ruler"));
    chkGridLines = new wxCheckBox(this, -1, _T("Grid lines"));
    chkHighlightModified = new wxCheckBox(this, -1, _T("Highlight modified data"));
    chkFontCharsOnly = new wxCheckBox(this, -1, _T("Restrict characters to font"));
    chkSelectOnPaste = new wxCheckBox(this, -1, _T("Select on paste"));
    wxFlexGridSizer *fgs = new wxFlexGridSizer(2, pad, pad);
    fgs->Add(chkPrettyAddresses);
    fgs->Add(chkRelativeAddresses);
    fgs->Add(chkRuler);
    fgs->Add(chkGridLines);
    fgs->Add(chkHighlightModified);
    fgs->Add(chkFontCharsOnly);
    fgs->Add(chkSelectOnPaste);
    szr->Add(fgs, 0, wxALL, pad);

    // Font
    hSizer = new wxBoxSizer(wxHORIZONTAL);
    wxString choices[] = {_T("Default"), _T("Anti-alias"), _T("Non AA"), _T("ClearType")};
    hSizer->Add(new wxStaticText(this, -1, _T("Font style")));
    hSizer->Add(cmbFontStyle = new wxChoice(this, -1, DPOS, DSIZE, 4, choices));
    hSizer->Add(new wxButton(this, -1, _T("Set font...")));
    hSizer->Add(20, 1);
    hSizer->Add(new wxStaticText(this, -1, _T("Code Page")), 0, wxALIGN_CENTER_VERTICAL);
    cmbCodePage = new thRecentChoice(this, IDC_CODEPAGE, s->asCodePages, 0, 0, wxDefaultPosition, wxSize(200, -1));
    hSizer->Add(cmbCodePage, 0, wxALL, 5);
    szr->Add(hSizer, 0, wxALL, 10);

    //! color combo box: [wndBack, Grid, Adr, AdrBack, Highlight, Sel, Mod)

    wxSizer *sbs = new wxStaticBoxSizer(wxVERTICAL, this, _T("Pane"));
    szr->Add(sbs, 1, wxGROW);
    sbs->Add(new wxButton(this, -1, _T("Background color")), 0, wxALL, 5);

    rbColorColumns = new wxRadioButton(this, -1, _T("Color by columns"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rbColorRows = new wxRadioButton(this, -1, _T("Color by rows"));
    spnColumnColors = new wxSpinCtrl(this, -1, _T("2"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 256, 2);
    spnRowColors = new wxSpinCtrl(this, -1, _T("2"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 256, 2);
    //! color combo box: N columns
    cmbColumnColors = new wxBitmapComboBox(this, IDC_CMB_COLUMN_COLORS);
    cmbRowColors = new wxBitmapComboBox(this, IDC_CMB_ROW_COLORS);

    wxMemoryDC dc;
    wxBitmap bmp(16, 8);
    COLORREF colors[5] = {0x800000, 0xff0000, 0x80, 0xff, 0x808080};
    dc.SetPen(*wxBLACK_PEN);
    for (int i = 0; i < 5; i++)
    {
        dc.SelectObject(bmp); // this makes a copy if necessary
        dc.SetBrush(*wxTheBrushList->FindOrCreateBrush(colors[i]));
        dc.DrawRectangle(0, 0, 16, 8);
        cmbColumnColors->Append(wxString::Format(_T("%d"), i), bmp);
        cmbRowColors->Append(wxString::Format(_T("%d"), i), bmp);
    }

    fgs = new wxFlexGridSizer(4, 5, 5);
    fgs->Add(rbColorColumns, 0, wxALIGN_CENTER_VERTICAL);
    fgs->Add(spnColumnColors);
    fgs->Add(cmbColumnColors);
    fgs->Add(new wxButton(this, IDC_BTN_COL_COLOR, _T("Change")));

    fgs->Add(rbColorRows, 0, wxALIGN_CENTER_VERTICAL);
    fgs->Add(spnRowColors);
    fgs->Add(cmbRowColors);
    fgs->Add(new wxButton(this, IDC_BTN_ROW_COLOR, _T("Change")));
    sbs->Add(fgs, 0, wxALL, 5);

    hSizer = new wxBoxSizer(wxHORIZONTAL);
    sbs->Add(rbColorValues = new wxRadioButton(this, -1, _T("Color by values")), 0, wxALL, 5);
    sbs->Add(ramp, 1, wxGROW);
    sbs->Add(hSizer, 0, wxALL, 5);

    text = new wxTextCtrl(this, -1);
    sbs->Add(text, 0, wxGROW);

    txtRange = new wxTextCtrl(this, IDC_RANGE_TEXT);
    sbs->Add(txtRange, 0, wxGROW | wxALL, pad);

    hSizer = new wxBoxSizer(wxHORIZONTAL);
    lstValueColorRanges = new wxListBox(this, IDC_VALUE_COLOR_RANGES);
    lstValueColorRanges->SetInitialSize(wxSize(-1, 100));
    hSizer->Add(lstValueColorRanges, 0, wxALL, pad);
    fgs = new wxFlexGridSizer(2, pad * 2, pad);
    fgs->Add(new wxButton(this, IDC_RANGE_ADD, _T("New")), 0, wxGROW);
    fgs->Add(new wxButton(this, IDC_RANGE_MOVE_UP, _T("Move Up")), 0, wxGROW);
    fgs->Add(new wxButton(this, IDC_RANGE_DELETE, _T("Delete")), 0, wxGROW);
    fgs->Add(new wxButton(this, IDC_RANGE_MOVE_DOWN, _T("Move Down")), 0, wxGROW);
    fgs->Add(new wxButton(this, IDC_RANGE_SET_ALL, _T("Set All...")), 0, wxGROW);
    hSizer->Add(fgs);
    sbs->Add(hSizer);

    InitControls();
    SetSizerAndFit(topSizer);
    Center();
}

void AddColorRanges(wxListBox *lst, thPalette &pal)
{
    int begin, end, last_end = -1;
    COLORREF clrBegin, clrEnd;
    wxString str;
    for (size_t i = 0; i < pal.Ranges(); i++)
    {
        pal.Get(i, &begin, &clrBegin, &end, &clrEnd);
        if (end == begin || end == -1)
            str.Printf(_T("%d (%s)"), begin, FormatColour(clrBegin).c_str());
        else
            str.Printf(_T("%d (%s) - %d (%s)"), begin, FormatColour(clrBegin).c_str(),
                end, FormatColour(clrEnd).c_str());
        lst->Append(str);
        last_end = end;
    }
}

void SettingsDialog::InitControls()
{
    chkAdjustLineBytes->SetValue(ps->bAdjustLineBytes);
    spnLineBytes->Enable(!ps->bAdjustLineBytes);
    rbByteOrderLittle->SetValue(ps->iEndianMode == LITTLEENDIAN_MODE);
    rbByteOrderBig->SetValue(ps->iEndianMode == BIGENDIAN_MODE);
    chkPrettyAddresses->SetValue(ps->bPrettyAddress);
    chkRelativeAddresses->SetValue(!ps->bAbsoluteAddresses);
    chkRuler->SetValue(ps->bShowRuler);
    chkGridLines->SetValue(ps->bGridLines);
    chkHighlightModified->SetValue(ps->bHighlightModified);
    chkFontCharsOnly->SetValue(ps->bFontCharsOnly);
    chkSelectOnPaste->SetValue(ps->bSelectOnPaste);

    AddColorRanges(lstValueColorRanges, ramp->pal);

    switch (ps->iFontQuality)
    {
    case ANTIALIASED_QUALITY:
        cmbFontStyle->SetSelection(1); break;
    case NONANTIALIASED_QUALITY:
        cmbFontStyle->SetSelection(2); break;
    case CLEARTYPE_QUALITY:
        cmbFontStyle->SetSelection(3); break;
    case -1:
    case DEFAULT_QUALITY:
    default:
        cmbFontStyle->SetSelection(0); break;
    }
}

bool SettingsDialog::UpdateData()
{
    wxString scp = cmbCodePage->GetValue();
    uint32 cp = 0;
    if (!scp.Len() ||
        (ReadUserNumber(scp, cp) &&
         (cp == 0 || IsValidCodePage(cp))))
    {
        ps->iCodePage = cp;
        cmbCodePage->Finalize();
    }
    else {
        wxMessageBox(cmbCodePage->GetValue() + _T(" is not a valid code page."), _T("T.Hex"), wxICON_ERROR, this);
        return false;
    }

    ps->bAdjustLineBytes = chkAdjustLineBytes->GetValue();
    ps->iLineBytes = spnLineBytes->GetValue();
    ps->iEndianMode = (rbByteOrderLittle->GetValue() ? LITTLEENDIAN_MODE : BIGENDIAN_MODE);
    ps->bPrettyAddress = chkPrettyAddresses->GetValue();
    ps->bAbsoluteAddresses = !chkRelativeAddresses->GetValue();
    ps->bShowRuler = chkRuler->GetValue();
    ps->bGridLines = chkGridLines->GetValue();
    ps->bHighlightModified = chkHighlightModified->GetValue();
    ps->bFontCharsOnly = chkFontCharsOnly->GetValue();
    ps->bSelectOnPaste = chkSelectOnPaste->GetValue();

    ps->SetPalette(ramp->pal.GetPalette());

    switch (cmbFontStyle->GetSelection())
    {
    case 0:
    default:
        ps->iFontQuality = -1; break;  // Let the OS choose.
    case 1:
        ps->iFontQuality = ANTIALIASED_QUALITY; break;
    case 2:
        ps->iFontQuality = NONANTIALIASED_QUALITY; break;
    case 3:
        ps->iFontQuality = CLEARTYPE_QUALITY; break;
    }

    return true;
}

void SettingsDialog::OnOK(wxCommandEvent &event)
{
    //if (spnLineBytes->GetValue() > MEGA)
    //{ // never used because of the max on spnLineBytes
    //    wxString msg = FormatDec((uint64)spnLineBytes->GetValue()) + " bytes per line?  That's ridiculous!";
    //    wxMessageBox(msg, "T.Hex", wxICON_HAND, this);
    //    return;
    //}
    if (UpdateData()) // read all control values into ps
        EndDialog(event.GetId());
}

void SettingsDialog::OnBarSelected(wxCommandEvent &event)
{
    int bar = event.GetInt();
    UpdateDisabler ud(this);

    if (bar >= 0)
        text->SetValue(wxString::Format(_T("%d 0x%02X '%c' (%s)"),
        bar, bar, bar, FormatColour(ramp->GetBarColour(bar)).c_str()));
    else
        text->Clear();

    int range = ramp->pal.FindRange(bar);
    if (range >= 0 && lstValueColorRanges->GetSelection() != range)
    {
        lstValueColorRanges->SetSelection(range);
        txtRange->SetValue(lstValueColorRanges->GetString(range));
    }
}

void SettingsDialog::OnChkAdjustLineBytes(wxCommandEvent &event)
{
    spnLineBytes->Enable(!event.IsChecked());
}

void SettingsDialog::OnRangeSelected(wxCommandEvent &event)
{
    UpdateDisabler ud(this);
    txtRange->SetValue(lstValueColorRanges->GetString(event.GetSelection()));
}

void SettingsDialog::OnRangeText(wxCommandEvent &WXUNUSED(event))
{
    if (DisableUpdates)
        return;

    int sel = lstValueColorRanges->GetSelection();
    if (sel < 0) return;
    lstValueColorRanges->SetString(sel, txtRange->GetValue());
    UpdatePalette();
}

void SettingsDialog::OnRangeAdd(wxCommandEvent &WXUNUSED(event))
{
    UpdateDisabler ud(this);
    int sel = lstValueColorRanges->GetSelection();
    lstValueColorRanges->Insert(wxString(_T("0 (0 0 0) - 0 (0 0 0)")), sel + 1);
    lstValueColorRanges->SetSelection(sel + 1);
    txtRange->SetValue(lstValueColorRanges->GetString(sel + 1));
    txtRange->SetFocus();
}

//void SettingsDialog::OnRangeEdit(wxCommandEvent &event)
//{
//}

void SettingsDialog::OnRangeDelete(wxCommandEvent &WXUNUSED(event))
{
    int sel = lstValueColorRanges->GetSelection();
    if (sel >= 0)
        lstValueColorRanges->Delete(sel);
    lstValueColorRanges->SetFocus();
    UpdatePalette();
}

void SettingsDialog::OnRangeMoveUp(wxCommandEvent &WXUNUSED(event))
{
    int sel = lstValueColorRanges->GetSelection();
    if (sel > 0)
    {
        wxString item = lstValueColorRanges->GetString(sel);
        lstValueColorRanges->Delete(sel);
        lstValueColorRanges->Insert(item, sel - 1);
    }
    UpdatePalette();
    lstValueColorRanges->SetFocus();
}

void SettingsDialog::OnRangeMoveDown(wxCommandEvent &WXUNUSED(event))
{
    int sel = lstValueColorRanges->GetSelection();
    if (sel + 1 < (int)lstValueColorRanges->GetCount())
    {
        wxString item = lstValueColorRanges->GetString(sel);
        lstValueColorRanges->Delete(sel);
        lstValueColorRanges->Insert(item, sel + 1);
    }
    UpdatePalette();
    lstValueColorRanges->SetFocus();
}

void SettingsDialog::UpdatePalette()
{
    wxString str;
    for (size_t i = 0; i < lstValueColorRanges->GetCount(); i++)
    {
        str += lstValueColorRanges->GetString(i) + _T("; ");
    }
    wxString msg = ramp->pal.SetPalette(str);
    text->SetValue(msg); // handily clears text box if no error message
    ramp->UpdateView();
}

void SettingsDialog::OnRangeSetAll(wxCommandEvent &event)
{
    wxString str = ramp->pal.GetPalette();
    str = wxGetTextFromUser(_T("message"), _T("caption"), str, this);//,
        //wxDefaultCoord, wxDefaultCoord, false);
    if (str.Len())
    {
        wxString msg = ramp->pal.SetPalette(str);
        text->SetValue(msg); // handily clears text box if no error message
        lstValueColorRanges->Clear();
        AddColorRanges(lstValueColorRanges, ramp->pal);
        ramp->UpdateView();
    }
}


//****************************************************************************
//****************************************************************************
// StringCollectDialog
// Virtual list control to show millions of strings.
//****************************************************************************
//****************************************************************************

class StringCollectListCtrl : public wxListCtrl
{
public:
    StringCollectListCtrl(wxWindow *parent, int id, int style, StringCollectDialog *scd)
    : wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, style), scd(scd)
    {
    }

    StringCollectDialog *scd;

    // From MSDN:
    // "Note  Dynamically switching to and from the LVS_OWNERDATA style is not supported."
    // I guess that means this class can't just step in when it needs to.
    //! Oh, wait... if we put the results on a separate notebook page...

    virtual wxString OnGetItemText(long item, long column) const
    {
        StringCollectDialog::T_StringInfo &info = scd->stringOffsets[item];
        switch (column) {
        case 0: return wxString::Format(_T("%u"), item);
        case 1: return scd->FormatOffset(info.start);
        case 2: return wxString::Format(_T("%u"), info.GetChars());
        case 3:
            // Windows imposes a string length limit of 260 characters, I think.
            if (info.IsUnicode())
                return Escape(scd->m_hw->doc->ReadStringW(info.start, wxMin(info.GetChars(), 1024/*scd->m_maxSize*/)));
            else
                return Escape(scd->m_hw->doc->ReadString(info.start, wxMin(info.GetChars(), 1024/*scd->m_maxSize*/)));
        default: return wxEmptyString;
        }
    }
};


BEGIN_EVENT_TABLE(StringCollectDialog, wxDialog)
    EVT_BUTTON(wxID_OK, OnGo)
    EVT_BUTTON(wxID_CANCEL, OnCancel)
    EVT_LIST_ITEM_SELECTED(-1, OnSelect)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_MENU(IDM_HEXOFFSET, OnHexOffset)
    EVT_MENU(IDM_RELATIVEOFFSET, OnRelativeOffset)
    EVT_BUTTON(IDC_FIND, OnFind)
    EVT_BUTTON(IDC_SAVE, OnSave)
END_EVENT_TABLE()

StringCollectDialog::StringCollectDialog(thFrame *frame, HexWnd *hw)
: wxDialog(frame, -1, wxString(_T("String Collector")), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    this->m_hw = hw;
    //this->m_doc = hw->doc;
    m_StringCount = 0;
    
    //wxWindow *panel = this; //! todo: use a wxNotebook or wxFlatNotebook
    notebook = new wxNotebook(this, -1);
    wxWindow *panel = new wxPanel(notebook, -1);

    //! Bleah.  Are validators really what we want here?  Not sure they're worth the trouble...
    m_strMin = _T("5");
    m_strMax = _T("260");
    m_strCount = _T("10000");
    m_strLetters = _T("3");
    wxTextValidator vMin(wxFILTER_INCLUDE_CHAR_LIST, &m_strMin);
    wxTextValidator vMax(wxFILTER_INCLUDE_CHAR_LIST, &m_strMax);
    wxTextValidator vCount(wxFILTER_INCLUDE_CHAR_LIST, &m_strCount);
    wxTextValidator vLetters(wxFILTER_INCLUDE_CHAR_LIST, &m_strCount);
    wxArrayString as;
    wxString includeList = _T("0123456789abcdefABCDEF&x$");
    for (size_t i = 0; i < includeList.Len(); i++)
        as.Add(includeList[i]);
    vMin.SetIncludes(as);
    vMax.SetIncludes(as);
    vCount.SetIncludes(as);
    vLetters.SetIncludes(as);

    wxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *fgSizer = new wxFlexGridSizer(4);
    fgSizer->Add(new wxStaticText(panel, -1, _T("Mi&n. Size")), 0, wxALL, 5);
    txtMin = new wxTextCtrl(panel, -1, m_strMin, wxDefaultPosition, wxDefaultSize, 0, vMin);
    fgSizer->Add(new wxStaticText(panel, -1, _T("Ma&x. Size")), 0, wxALL, 5);
    txtMax = new wxTextCtrl(panel, -1, m_strMax, wxDefaultPosition, wxDefaultSize, 0, vMax);
    fgSizer->Add(new wxStaticText(panel, -1, _T("String count &Limit")), 0, wxALL, 5);
    txtLimit = new wxTextCtrl(panel, -1, m_strCount, wxDefaultPosition, wxDefaultSize, 0, vCount);
    fgSizer->Add(new wxStaticText(panel, -1, _T("Minimum letters")), 0, wxALL, 5);
    txtLetters = new wxTextCtrl(panel, -1, m_strLetters, wxDefaultPosition, wxDefaultSize, 0, vCount);
    fgSizer->Add(txtMin,     0, wxALL & ~wxTOP, 5);
    fgSizer->Add(txtMax,     0, wxALL & ~wxTOP, 5);
    fgSizer->Add(txtLimit,   0, wxALL & ~wxTOP, 5);
    fgSizer->Add(txtLetters, 0, wxALL & ~wxTOP, 5);
    topSizer->Add(fgSizer);

    wxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *vSizer = new wxBoxSizer(wxVERTICAL);
    chkANSI = new wxCheckBox(panel, -1, _T("&ANSI"));
    chkANSI->SetValue(true);
    chkUnicode = new wxCheckBox(panel, -1, _T("&Unicode"));
    chkCaseSens = new wxCheckBox(panel, -1, _T("Case &sensitive"));
    chkTerminator = new wxCheckBox(panel, -1, _T("Require &zero terminator"));
    chkWeirdnessFilter = new wxCheckBox(panel, -1, _T("&Weirdness filter"));
    chkSelOnly = new wxCheckBox(panel, -1, _T("Selection &only"));
    vSizer->Add(chkANSI, 0, wxALL, 5);
    vSizer->Add(chkUnicode, 0, wxALL, 5);
    vSizer->Add(chkCaseSens, 0, wxALL, 5);
    vSizer->Add(chkTerminator, 0, wxALL, 5);
    vSizer->Add(chkWeirdnessFilter, 0, wxALL, 5);
    vSizer->Add(chkSelOnly, 0, wxALL, 5);
    hSizer->Add(vSizer);

    const TCHAR charset[] = _T("abcdefghijklmnopqrstuvwxyz ")
                            _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ ")
                            _T(".,:;\"'!?()$%&#0123456789 \\t\\r\\n\\\\~/@^*_-+[]{}|")
                            _T("<=>")
                            ;

    txtCharSet = new wxTextCtrl(panel, -1, Escape(charset), wxDefaultPosition, wxSize(300, 100), wxTE_MULTILINE);
    vSizer = new wxBoxSizer(wxVERTICAL);
    vSizer->Add(new wxStaticText(panel, -1, _T("&Character set")), 0, wxALL & ~wxBOTTOM, 5);
    vSizer->Add(txtCharSet, 0, wxALL, 5);
    hSizer->Add(vSizer);
    topSizer->Add(hSizer);

    NEW_HIDDEN_CANCEL_BUTTON(panel);
    btnGo = new wxButton(panel, wxID_OK, _T("&Go"));
    btnGo->SetDefault(); //! wxButton::SetDefault() includes SetDefaultItem()
    topSizer->Add(btnGo, 0, wxALL, 5);

    panel->SetSizerAndFit(topSizer);
    notebook->AddPage(panel, _T("Se&ttings"));

    // results panel
    panel = new wxPanel(notebook, -1);
    topSizer = new wxBoxSizer(wxVERTICAL);
    list = new StringCollectListCtrl(panel, -1, wxLC_REPORT | wxLC_VIRTUAL , this);
    topSizer->Add(list, 1, wxGROW | wxALL, 5);
    list->InsertColumn(0, _T("#"));
    list->InsertColumn(1, _T("Position"));
    list->InsertColumn(2, _T("Size"));
    list->InsertColumn(3, _T("String"));
    hSizer = new wxBoxSizer(wxHORIZONTAL);
    hSizer->Add(new wxButton(panel, IDC_FIND, _T("Find...")), 0, wxALL, 5);
    hSizer->Add(new wxButton(panel, IDC_SAVE, _T("Save...")), 0, wxALL, 5);
    topSizer->Add(hSizer);
    panel->SetSizerAndFit(topSizer);
    notebook->AddPage(panel, _T("&Results"));

    // status panel
    panel = new wxPanel(notebook, -1);
    topSizer = new wxBoxSizer(wxVERTICAL);
    txtDebug = new wxTextCtrl(panel, -1, ZSTR, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    topSizer->Add(txtDebug, 1, wxGROW | wxALL, 5);
    panel->SetSizerAndFit(topSizer);
    notebook->AddPage(panel, _T("Debug"));

    // Done adding controls.  Finalize the dialog.
    topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(notebook, 1, wxGROW);
    SetSizerAndFit(topSizer);

    popup = new wxMenu();
    popup->AppendCheckItem(IDM_HEXOFFSET, _T("Hex Offset"));
    popup->AppendCheckItem(IDM_RELATIVEOFFSET, _T("Relative Offset"));

    bRelativeOffset = false;
    bHexOffset = true;

    memset(isLetter, 0, 256);
    for (int i = 0; i < 26; i++)
        isLetter['a' + i] = isLetter['A' + i] = 1;
}

StringCollectDialog::~StringCollectDialog()
{
    delete popup;
}

void StringCollectDialog::OnSelect(wxListEvent &event)
{
    int index = event.GetIndex();
    //size_t size = list->GetItemData(index);
    //THSIZE start = stringOffsets[index];
    T_StringInfo &info = stringOffsets[index];
    m_hw->CmdSetSelection(info.start, info.start + info.GetBytes());
}

int StringCollectDialog::Add(THSIZE start, size_t size)
{
    //stringOffsets.push_back(start);

    //wxString str = Escape(m_hw->doc->ReadString(start, size));

    //int index = list->InsertItem(list->GetItemCount(), FormatOffset(start));
    //list->SetItem(index, 1, wxString::Format(_T("%u"), size));
    //list->SetItem(index, 2, str);
    //list->SetItemData(index, size);

    stringOffsets.push_back(T_StringInfo(start, size, 0));
    count8++;
    totalLength8 += size;
    if (size > m_longestLength8) {
        m_longestLength8 = size;
        m_longestString8 = m_StringCount;
    }

    m_StringCount++;
    return (m_limit && m_StringCount >= m_limit);
}

int StringCollectDialog::AddW(THSIZE start, size_t size)
{
    //stringOffsets.push_back(start);

    //wxString str = Escape(m_hw->doc->ReadStringW(start, size));

    //int index = list->InsertItem(list->GetItemCount(), FormatOffset(start));
    //list->SetItem(index, 1, wxString::Format(_T("%u"), size));
    //list->SetItem(index, 2, str);
    //list->SetItemData(index, size * 2);

    stringOffsets.push_back(T_StringInfo(start, size, 1));
    count16++;
    totalLength16 += size;
    if (size > m_longestLength16) {
        m_longestLength16 = size;
        m_longestString16 = m_StringCount;
    }

    m_StringCount++;
    return (m_limit && m_StringCount >= m_limit);
}


void StringCollectDialog::OnContextMenu(wxContextMenuEvent &event)
{
    //! what was I thinking here?  No real GUI?
    popup->Check(IDM_HEXOFFSET, bHexOffset);
    popup->Check(IDM_RELATIVEOFFSET, bRelativeOffset);
    PopupMenu(popup, ScreenToClient(event.GetPosition()));
}

wxString StringCollectDialog::FormatOffset(THSIZE offset)
{
    if (!bRelativeOffset)
        offset += m_hw->doc->display_address;
    if (bHexOffset)
        return wxString::Format(_T("%I64X"), offset);
    return wxString::Format(_T("%I64u"), offset);
}

void StringCollectDialog::OnHexOffset(wxCommandEvent &event)
{
    bHexOffset = !bHexOffset;
    RedoOffsets();
}

void StringCollectDialog::OnRelativeOffset(wxCommandEvent &event)
{
    bRelativeOffset = !bRelativeOffset;
    RedoOffsets();
}

void StringCollectDialog::RedoOffsets()
{
    //list->Freeze();
    //for (size_t i = 0; i < stringOffsets.size(); i++)
    //    list->SetItem(i, 0, FormatOffset(stringOffsets[i]));
    //list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
    //list->Thaw();

    AutoSizeColumns();
    list->Refresh();  // RefreshItems() with a huge range doesn't work.
}

void StringCollectDialog::OnGo(wxCommandEvent &event)
{
    ATimer timer;
    timer.StartWatch();
    bool bTerminated = chkTerminator->GetValue(); // require a zero terminator?
    bool filter = chkWeirdnessFilter->GetValue();
    bool caseSens = !!chkCaseSens->GetValue();
    const size_t WEIRDNESS_LIMIT = 4;  // maximum repeated characters, if "weirdness filter" selected.

    wxString charset = Unescape(txtCharSet->GetValue());

    BitArray match16(65536);
    uint8 match8[256];
    memset(match8, 0, 256);

    // If the user requested a case-insensitive search,
    // include both upper and lower case for each letter.
    if (!caseSens)
        charset = charset.Lower() + charset.Upper();

#ifdef _UNICODE
    wxCharBuffer charbuf8 = charset.mb_str(wxConvLibc);
    const char *charset8 = charbuf8;
    for (size_t i = 0; i < charset.Len(); i++)
    {
        match16.SetBit(charset[i]);
        match8[charset8[i]] = 1;
    }
#else
    //! bit of a weak spot here in the conversion from escaped Unicode to ANSI...
    wxWCharBuffer charbuf16 = charset.wc_str(wxConvLibc);
    const wchar_t *charset16 = charbuf16;
    for (size_t i = 0; i < charset.Len(); i++)
    {
        match8[charset[i]] = 1;
        match16.SetBit(charset16[i]);
    }
#endif

    //! todo: What is m_maxSize?
    // Do we truncate long strings there, or not even add them to our list?

    m_minSize = ReadUserNumber(txtMin->GetValue());
    m_maxSize = ReadUserNumber(txtMax->GetValue());
    m_limit = ReadUserNumber(txtLimit->GetValue());
    m_minLetters = ReadUserNumber(txtLetters->GetValue());

    if (m_maxSize == 0)
        m_maxSize = m_hw->DocSize(); //! this is wrong for Unicode, but it doesn't matter.

    if (m_minSize > m_maxSize || m_minSize == 0)
    {
        //! complain
        return;
    }

    list->Freeze();
    list->DeleteAllItems();
    stringOffsets.clear();

    m_StringCount = 0;
    m_longestString8 = m_longestLength8 = 0;
    m_longestString16 = m_longestLength16 = 0;
    totalLength8 = totalLength16 = 0;
    count8 = count16 = 0;
    const bool ansi = chkANSI->GetValue(), uni = chkUnicode->GetValue();

    THSIZE startpos = 0, endpos = m_hw->DocSize();
    if (chkSelOnly->GetValue() && m_hw->GetSelection().GetSize())
        m_hw->GetSelection(startpos, endpos);
    THSIZE blockSize = MEGA;

    thProgressDialog *progress = new thProgressDialog(endpos - startpos, this, _T("Reading..."), _T("String Collection"));

    size_t size8 = 0, size16 = 0;
    uint8 last8 = 0;
    uint16 last16 = 0;
    size_t run8 = 0, run16 = 0;
    bool reject8 = false, reject16 = false;
    size_t letterCount8 = 0, letterCount16 = 0;
    //uint8 *pData = new uint8[MEGA];
    THSIZE offset;

    //! todo: more processing at end of string, and less on each character?  (Needs better caching)
    //! todo: better benchmark for string search speed -- read large file into memory
    // Read speeds from RAM in MB/s, using test file Win2003_ddk.iso
    // Debug Unicode build: ANSI 53, Unicode 106, both 39
    // Release Unicode build: ANSI 86, Unicode 231, both 76

    for (offset = startpos; offset < endpos; offset += blockSize)
    {
        if (endpos - offset < MEGA)
            blockSize = endpos - offset;
        // We can use HexDoc::Load() here because Add() doesn't actually read from the document.
        const uint8 *pData = (uint8*)m_hw->doc->Load(offset, blockSize);
        if (!pData)
            break;
        //if (!m_hw->doc->Read(offset, blockSize, pData))
        //    break;

        for (int iter = 0; iter < blockSize; iter += 2)
        {
            if (ansi)
            {
                for (int iter8 = iter; iter8 < iter + 2; iter8++)  //! This is ugly and slow.
                {
                    const uint8 &c = pData[iter8];
                    if (match8[c])
                    {
                        size8++;
                        letterCount8 += isLetter[c];

                        if (filter)
                        {
                            if (c == last8)
                            {
                                if (++run8 >= WEIRDNESS_LIMIT)
                                    reject8 = true;
                            }
                            else
                            {
                                last8 = c;
                                run8 = 0;
                            }
                        }
                    }
                    else
                    {
                        if (size8 >= m_minSize /*&& size8 <= m_maxSize*/ &&
                            (!c || !bTerminated) && !reject8 &&
                            letterCount8 >= m_minLetters)
                            if (Add(offset + iter8 - size8, size8))
                                goto done;
                        size8 = 0;
                        run8 = 0;
                        letterCount8 = 0;
                        last8 = 0;
                        reject8 = 0;
                    }
                }
                //! bug: the last character in an odd-length document is not checked.
            }

            if (uni)
            {
                const uint16 &uc = *(uint16*)(pData + iter);
                if (match16[uc])
                {
                    size16++;
                    if (uc < 128)
                        letterCount16 += isLetter[uc];

                    if (filter)
                    {
                        if (uc == last16)
                        {
                            if (++run16 >= WEIRDNESS_LIMIT)
                                reject16 = true;
                        }
                        else
                        {
                            last16 = uc;
                            run16 = 0;
                        }
                    }
                }
                else
                {
                    if (size16 >= m_minSize /*&& size16 <= m_maxSize*/ &&
                        (!uc || !bTerminated) && !reject16 &&
                        letterCount16 >= m_minLetters)
                        if (AddW(offset + iter - (size16 * 2), size16))
                            goto done;
                    size16 = 0;
                    run16 = 0;
                    letterCount16 = 0;
                    last16 = 0;
                    reject16 = 0;
                }
            }
        }

        if (!progress->Update(offset - startpos))
            break;
    }

    // catch strings at EOF
    if (!bTerminated && size8  >= m_minSize && size8  <= m_maxSize && !reject8  && m_StringCount < m_limit)
        Add(endpos - size8, size8);
    if (!bTerminated && size16 >= m_minSize && size16 <= m_maxSize && !reject16 && m_StringCount < m_limit)
        AddW(RoundDown(endpos, 2) - size16 * 2, size16);

done:
    //delete [] pData;
    list->SetItemCount(m_StringCount);
    delete progress;

    //! todo: store size and position instead of string?  (virtual list control)
    //! todo: check for duplicates?
    //! todo: (better) garbage filter
    //! todo: auto-size function for virtual list control
    //! todo: search in strings
    //! todo: save to file

    timer.StopWatch();
    wxString msg;
    msg.Printf(_T("%d strings found in %0.2f seconds"), m_StringCount, timer.GetSeconds());
    if (m_StringCount == m_limit && offset < endpos)
        msg += wxString::Format(_T(" (limit reached at %0.1f%% of document)"), (offset - startpos) * 100.0 / (endpos - startpos));
    msg += _T(".\n");
    msg += wxString::Format(_T("Average read speed: %0.2f MB/sec.\n"), (offset - startpos) / 1048576 / timer.GetSeconds());
    if (count8) {
        msg += wxString::Format(_T("\n%d 8-bit strings, average length %0.1f\n"), count8, (double)totalLength8 / count8);
        msg += wxString::Format(_T("Longest 8-bit string: #%d, with %d characters.\n"), m_longestString8, m_longestLength8);
    }
    if (count16) {
        msg += wxString::Format(_T("\n%d 16-bit strings, average length %0.1f\n"), count16, (double)totalLength16 / count16);
        msg += wxString::Format(_T("Longest 16-bit string: #%d, with %d characters.\n"), m_longestString16, m_longestLength16);
    }
    txtDebug->SetValue(msg);

    AutoSizeColumns();
    list->Thaw();
    notebook->ChangeSelection(1);  // show results page
}

void StringCollectDialog::AutoSizeColumns()
{
    // This gets complicated when you can't call GetTextExtent() for every string.
    if (m_StringCount)
    {
        // First column is the item index.  Easy.
        AutoSizeVirtualWxListColumn(list, 0, wxString::Format(_T("%u"), m_StringCount - 1));
        // Second column is the offset.  Largest number is not necessarily the widest string.
        wxString digits, repeated;
        int dcount;
        THSIZE offset = stringOffsets[m_StringCount - 1].start;
        if (!bRelativeOffset)
            offset += m_hw->doc->display_address;
        if (bHexOffset)
            digits = _T("0123456789ABCDEF"), dcount = CountDigits(offset, 16);
        else
            digits = _T("0123456789"), dcount = CountDigits(offset, 10);
        int x, y, xmax = 0, maxdigit = 0;
        wxClientDC dc(list);
        dc.SetFont(list->GetFont());
        for (size_t d = 0; d < digits.Len(); d++)
        {
            dc.GetTextExtent(digits.Mid(d, 1), &x, &y);
            if (x > xmax) {
                xmax = x;
                maxdigit = d;
            }
        }
        while (dcount--)
            repeated += digits[maxdigit];
        AutoSizeVirtualWxListColumn(list, 1, repeated);
        // Third column is the length.  Easy.
        AutoSizeVirtualWxListColumn(list, 2, wxString::Format(_T("%u"),
            wxMax(m_longestLength8, m_longestLength16)));
        // Fourth column -- I'm not even going to try.
        list->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
    }
    else for (int i = 0; i < 4; i++)
        list->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
}

void StringCollectDialog::OnCancel(wxCommandEvent &event)
{
    // We may have a lot of memory allocated, so delete ourselves here.
    Destroy();
}

void StringCollectDialog::OnFind(wxCommandEvent &event)
{
    wxMessageBox(_T("Not yet implemented."));
}

void StringCollectDialog::OnSave(wxCommandEvent &event)
{
    if (m_StringCount == 0)
        return;

    wxFileDialog fileDlg(this, _T("Save string list"), ZSTR, ZSTR,
        _T("Text files (*.txt)|*.txt|All files|*.*"),
        wxFD_SAVE);

    if (fileDlg.ShowModal() == wxID_CANCEL)
        return;

    wxString filename = fileDlg.GetPath();
    FILE *f = _tfopen(filename, _T("w"));

    T_StringInfo &last = stringOffsets[m_StringCount-1];
    int nOffsetDigits = wxMax(5, FormatOffset(last.start).Len());
    int nSizeDigits = wxMax(4, CountDigits(last.GetChars(), 10));
    _ftprintf(f, _T("%*s %*s String\n"), nOffsetDigits, _T("Start"), nSizeDigits, _T("Size"));
    wxString str;
    for (size_t n = 0; n < m_StringCount; n++)
    {
        T_StringInfo &info = stringOffsets[n];
        if (info.IsUnicode())
            str = Escape(m_hw->doc->ReadStringW(info.start, wxMin(m_maxSize, info.GetChars())));
        else
            str = Escape(m_hw->doc->ReadString(info.start, wxMin(m_maxSize, info.GetChars())));
        _ftprintf(f, _T("%*s %*u %s\n"), nOffsetDigits, FormatOffset(info.start).c_str(), nSizeDigits, info.GetChars(), str.c_str());
    }
    fclose(f);
}

//****************************************************************************
//****************************************************************************
// CopyFormatPanel
//****************************************************************************
//****************************************************************************

CopyFormatPanel::CopyFormatPanel(wxWindow *parent)
: wxPanel(parent)
{
    wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *fgs = new wxFlexGridSizer(2, 10, 10);

    wxString formats[] = {
        _T("Raw string"),
        _T("Integers"),
        _T("Floats"),
        _T("C code"),
        _T("Escaped string"),
        _T("As shown on screen"),
        _T("Same as current pane") };
    cmbFormat = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
        DIM(formats), formats);
    fgs->Add(new wxStaticText(this, -1, _T("Format")));
    fgs->Add(cmbFormat, 0, wxGROW);

    // base
    wxString bases[] = {
        _T("2  binary"),
        _T("8  octal"),
        _T("10 decimal"),
        _T("12 duodecimal"),
        _T("16 hexadecimal"),
        _T("36"),
        _T("64") };
    cmbBase = new wxComboBox(this, -1, ZSTR, wxDefaultPosition, wxDefaultSize,
        DIM(bases), bases);
    fgs->Add(new wxStaticText(this, -1, _T("Number base")));
    fgs->Add(cmbBase, 0, wxGROW);

    // word size
    wxString wordsizes[] = {
        _T("1"),
        _T("2"),
        _T("4"),
        _T("8") };
    cmbWord = new wxComboBox(this, -1, ZSTR, wxDefaultPosition, wxDefaultSize,
        DIM(wordsizes), wordsizes);
    fgs->Add(new wxStaticText(this, -1, _T("Word size (Bytes)")));
    fgs->Add(cmbWord, 0, wxGROW);

    // words per line
    wxString wordcounts[] = {
        _T("1"),
        _T("2"),
        _T("4"),
        _T("8"),
        _T("10"),
        _T("16"),
        _T("20") };
    cmbWords = new wxComboBox(this, -1, ZSTR, wxDefaultPosition, wxDefaultSize,
        DIM(wordcounts), wordcounts);
    fgs->Add(new wxStaticText(this, -1, _T("Words per line")));
    fgs->Add(cmbWords, 0, wxGROW);

    // endian mode
#if NATIVE_ENDIAN_MODE == LITTLEENDIAN_MODE
    wxString endian[] = { _T("Little (Native)"), _T("Big") };
#else
    wxString endian[] = { _T("Little"), _T("Big (Native)") };
#endif
    cmbEndian = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
        DIM(endian), endian);
    fgs->Add(new wxStaticText(this, -1, _T("Endian mode")));
    fgs->Add(cmbEndian, 0, wxGROW);

    // for screen data: RTF, HTML, plain text
    wxString rtf[] = {
        _T("HTML"),
        _T("RTF"),
        _T("Plain text") };
    cmbRTF = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
        DIM(rtf), rtf);
    fgs->Add(new wxStaticText(this, -1, _T("File format")));
    fgs->Add(cmbRTF, 0, wxGROW);

    wxString charsets[] = { _T("8-bit"), _T("UTF-8"), _T("UTF-16"), _T("UTF-32") };
    cmbCharset = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, DIM(charsets), charsets);
    fgs->Add(new wxStaticText(this, -1, _T("Character set")), 0, wxGROW);
    fgs->Add(cmbCharset, 0, wxGROW);

    mainSizer->Add(fgs, 1, wxGROW);

    mainSizer->Add(new wxStaticText(this, -1, _T("Preview")), 0, wxLEFT | wxTOP, 5);
    txtPreview = new wxTextCtrl(this, -1, ZSTR, wxDefaultPosition, wxSize(100, 100),
        wxTE_READONLY | wxTE_RICH2 | wxTE_MULTILINE | wxTE_DONTWRAP);
    mainSizer->Add(txtPreview, 1, wxGROW | wxALL, 5);

    wxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *btnOK = new wxButton(this, wxID_OK, _T("OK"));
    hSizer->Add(btnOK, 0, wxALL, 10);
    hSizer->Add(new wxButton(this, wxID_CANCEL, _T("&Cancel")));
    mainSizer->Add(hSizer);
    SetSizerAndFit(mainSizer);
}

thCopyFormat CopyFormatPanel::GetFormat()
{
    thCopyFormat fmt((thCopyFormat::_DataFormat)cmbFormat->GetSelection());
    fmt.numberBase = _tstoi(cmbBase->GetValue());
    fmt.wordSize = _tstoi(cmbWord->GetValue());
    fmt.wordsPerLine = _tstoi(cmbWords->GetValue());

    return fmt;
}

//****************************************************************************
//****************************************************************************
// CopyFormatDialog
//****************************************************************************
//****************************************************************************

CopyFormatDialog::CopyFormatDialog(wxWindow* parent)
: wxDialog(parent, -1, _T("Copy Format"), wxDefaultPosition, wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    formatPanel = new CopyFormatPanel(this);
    mainSizer->Add(formatPanel, 1, wxGROW);
    SetSizerAndFit(mainSizer);
}


//****************************************************************************
//****************************************************************************
// PasteFormatDialog
//****************************************************************************
//****************************************************************************

//BEGIN_EVENT_TABLE(PasteFormatDialog, wxDialog)
//    EVT_COMMAND_RANGE(IDC_FIRST, IDC_LAST, wxEVT_COMMAND_RADIOBUTTON_SELECTED, OnSelection)
//END_EVENT_TABLE()

PasteFormatDialog::PasteFormatDialog(wxWindow* parent)
: wxDialog(parent, -1, _T("Paste Format"), wxDefaultPosition, wxDefaultSize)
{
    wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    //rbAuto  = new wxRadioButton(this, IDC_AUTO, "&Auto");
    //rbRaw   = new wxRadioButton(this, IDC_RAW,  "&Raw");
    //rbASCII = new wxRadioButton(this, IDC_ASCII, "&Locale-dependent");
    //rbUTF8  = new wxRadioButton(this, IDC_UTF8,  "UTF-&8");
    //rbUTF16 = new wxRadioButton(this, IDC_UTF16, "UTF-&16");
    //rbUTF32 = new wxRadioButton(this, IDC_UTF32, "UTF-&32");
    //rbPython = new wxRadioButton(this, IDC_PYTHON, "Python string");
    //rbHex   = new wxRadioButton(this, IDC_HEX, "Hex string");

    //wxSizer *fgSizer = new wxFlexGridSizer(
    //mainSizer->Add(rbAuto,  0, wxALL, 5);
    //mainSizer->Add(rbRaw,   0, wxALL, 5);
    //mainSizer->Add(rbASCII, 0, wxALL, 5);
    //mainSizer->Add(rbUTF8,  0, wxALL, 5);
    //mainSizer->Add(rbUTF16, 0, wxALL, 5);
    //mainSizer->Add(rbUTF32, 0, wxALL, 5);
    //mainSizer->Add(rbPython, 0, wxALL, 5);
    //mainSizer->Add(rbHex,   0, wxALL, 5);

    wxString strFormats[] = {
        _T("&Auto"),
        _T("&Raw"),
        _T("&Locale-dependent"),
        _T("UTF-&8"),
        _T("UTF-&16"),
        _T("UTF-&32"),
        _T("&Escaped string"),
        _T("&Hex string") };
    rbFormat = new wxRadioBox(this, -1, _T("Format"), wxDefaultPosition, wxDefaultSize,
        DIM(strFormats), strFormats, 1);
    mainSizer->Add(rbFormat, 0, wxALL, 5);

    wxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *btnOK = new wxButton(this, wxID_OK, _T("OK"));
    //SetDefaultItem(btnOK);
    btnOK->SetDefault();
    hSizer->Add(btnOK, 0, wxALL, 10);
    hSizer->Add(new wxButton(this, wxID_CANCEL, _T("&Cancel")), 0, wxALL, 10);
    mainSizer->Add(hSizer);
    SetSizerAndFit(mainSizer);
}

//void PasteFormatDialog::OnSelection(wxCommandEvent &event)
//{
//}

//int PasteFormatDialog::GetSelection()
//{
//}


//****************************************************************************
//****************************************************************************
// HistogramView
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(HistogramView, thBarViewCtrl)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_MENU(IDM_LogScale, OnLogScale)
    EVT_MENU(IDM_SnapToPeak, OnSnapToPeak)
    //EVT_ERASE_BACKGROUND(OnErase)
END_EVENT_TABLE()

HistogramView::HistogramView(wxWindow *parent, HexWnd *hw, wxSize size /*= wxDefaultSize*/)
: thBarViewCtrl(parent, -1, wxDefaultPosition, size, wxT("Histogram"))
{
    m_bLogScale = false;
    m_bSnapToPeak = true;

    // get colors from hw->m_settings
    HexWndSettings s = hw->GetSettings();
    SetBackgroundColour(s.GetBackgroundColour());
    m_pal = s.palText;
    m_bEvenOddColors = s.bEvenOddColors;
    m_clrEOText[0] = s.clrEOText[0];
    m_clrEOText[1] = s.clrEOText[1];
}

HistogramView::~HistogramView()
{
}

bool HistogramView::BeginDraw()
{
    if (!pHist)
        return false;
    if (!pHist->m_highest)
        return false;
    m_dScale = m_rcBorder.GetHeight() / log((double)pHist->m_highest); // for log scale display
    return true;

    //! todo:
    // logarithmic scale
    // if available width < 256 (or used range), wrap around?
    //  - scroll bar?
    //  - becomes less of a problem when you have the list view
    // if available width > 256, enforce integral number of pixels per bar?
    // cancel button or separate thread when accumulating stats
}

void HistogramView::OnContextMenu(wxContextMenuEvent &event)
{
    wxPoint pt = ScreenToClient(event.GetPosition());
    wxMenu popup;
    popup.AppendCheckItem(IDM_LogScale, _T("&Log scale"));
    popup.AppendCheckItem(IDM_SnapToPeak, _T("&Snap to closest peak"));
    popup.Check(IDM_LogScale, m_bLogScale);
    popup.Check(IDM_SnapToPeak, m_bSnapToPeak);
    PopupMenu(&popup, pt);
}

void HistogramView::OnLogScale(wxCommandEvent &event)
{
   m_bLogScale = !m_bLogScale;
   UpdateView();
}

void HistogramView::OnSnapToPeak(wxCommandEvent &event)
{
   m_bSnapToPeak = !m_bSnapToPeak;
}

int HistogramView::HitTest(wxPoint pt)
{
    int width = wxMax(m_rcBorder.width, 256);

    if (!m_bSnapToPeak) // old way: use only X-coordinate of pt
    {
        return thBarViewCtrl::HitTest(pt);
    }

    // new way: pick bar whose end point is closest to pt.

    int bottom = m_rcBorder.y + m_rcBorder.height;
    int closest = -1;
    double distance = 0;
    for (int i = 0; i < 256; i++)
    {
        int bar_height = GetBarHeight(i);
        int bar_width, bar_left = GetBarArea(i, bar_width);
        int bar_mid = GetBarMid(bar_left, bar_width);

        double dx = bar_mid - pt.x;
        double dy = (bottom - bar_height) - pt.y;
        // This calculation should favor points that are closer in the vertical dimension,
        // while also taking horizontal distance into account.  (I hope.)
        double tmp = sqrt(pow(dx * dx, 1.5) + dy * dy);
        if (closest == -1 || tmp < distance)
        {
            distance = tmp;
            closest = i;
        }
    }
    return closest;
}

int HistogramView::GetBarHeight(int bar)
{
    if (bar < 0 || bar >= GetBarCount())
        return -1;

    if (!pHist->m_highest)
        return 0;

    int sel = pHist->dispOrder[bar];

    if (pHist->m_count[sel] == 0)
        return 0;

    if (m_bLogScale)
        return ceil(log((double)pHist->m_count[sel]) * m_dScale);
    else
        //return (int)DivideRoundUp((THSIZE)pHist->m_count[sel] * m_rcBorder.height, pHist->m_highest);
        return ceil((double)pHist->m_count[sel] * m_rcBorder.height / pHist->m_highest);
}

wxColour HistogramView::GetBarColour(int bar)
{
    int sel = this->pHist->dispOrder[bar];
    if (sel < 0 || sel > 255)
        return *wxBLACK;
    if (m_bEvenOddColors)
        return m_clrEOText[sel & 1];
    return m_pal.GetColor(sel);
}

CHistogram::CHistogram()
{
    m_highest = 0;
    for (int i = 0; i < 256; i++)
    {
        m_count[i] = 0;
        dispOrder[i] = i;
    }
    m_nSize = 0;
}

bool CHistogram::Calc(HexWnd *hw, HistogramView *pView /*= NULL*/)
{
    ATimer optimer; optimer.StartWatch();
    // zero out byte counts
    for (int i = 0; i < 256; i++)
        m_count[i] = 0;
    m_highest = 0;

    THSIZE start, end;
    m_nSize = hw->GetSelection().GetSize();
    if (m_nSize == 0)
    {
        start = 0;
        end = m_nSize = hw->doc->GetSize();
    }
    else
    {
        start = hw->GetSelection().GetFirst();
        end = start + m_nSize;
    }

    //! create progress bar
    thProgressDialog progbar(m_nSize, pView, hw->GetTitle(), _T("Building Histogram..."));

    // count all bytes in document
#if 0 // old way -- works great.
    ByteIterator2 iter(start, m_nSize, hw->doc);
    THSIZE index = 0;
    while (!iter.AtEnd())
    {
        ++m_count[*iter];
        ++iter;
        ++index;

        //! should these events be on a timer instead of (or in addition to) a counter?
        //! yes, and we should be running a separate thread...

        // check for cancel button
        if ((index & 0xFFFFF) == 0)
        {
            if(!progbar.Update(index))
            {
                m_nSize = index; // we only read this many bytes
                return 0; // user cancelled
            }
            wxGetApp().DoEvents();
        }

        // update pHistView
        if ((index & 0xFFFFF) == 0 && pView)
        {
            for (int i = 0; i < 256; i++)
                m_highest = wxMax(m_highest, m_count[i]);
            pView->UpdateView();
        }
    }

    for (int i = 0; i < 256; i++)
        m_highest = wxMax(m_highest, m_count[i]);

#else // new way -- speed test
    // Wow!  It runs twice as fast with half the CPU time.  ByteIterator2 sucks.
    int blocksize = MEGA;
    for (THSIZE index = 0; index < m_nSize; index += blocksize)
    {
        if (m_nSize - index < blocksize)
            blocksize = m_nSize - index;
        const uint8* buf = hw->doc->Load(start + index, blocksize);
        if (!buf)
            continue; // error has already been printed.

        // This loop takes about 3 cycles per byte.
        // Unrolling it and changing m_count from 64 to 32 bits didn't make it any faster.
        // Multithreading might.  Yes, it's already fast enough, but this is fun.  AEB  2009-01-07
        for (int i = 0; i < blocksize; i++)
            ++m_count[buf[i]];

        //! should these events be on a timer instead of (or in addition to) a counter?
        //! yes, and we should be running a separate thread...

        // check for cancel button
        if(!progbar.Update(index))
        {
            m_nSize = index; // we only read this many bytes
            return 0; // user cancelled
        }
        wxGetApp().DoEvents();

        // update pHistView
        for (int i = 0; i < 256; i++)
            m_highest = wxMax(m_highest, m_count[i]);
        pView->UpdateView();
    }
#endif

    optimer.StopWatch();
    PRINTF(_T("Read %s in %0.2f seconds; %0.2f MB/s\n"),
        FormatBytes(m_nSize), optimer.GetSeconds(), m_nSize / optimer.GetSeconds() / MEGA);

    return true;
}

//Histogram
//
//read with progress bar
//store in data class
//show in output dialog with two panes:
//1. graphical display
//2. list view
//
//when you click the either display, the corresponding item in the other display is highlighted.
//list view columns:
//pick number base

BEGIN_EVENT_TABLE(HistogramDialog, wxDialog)
    EVT_LIST_ITEM_SELECTED(-1, OnListItemSelected)
    EVT_COMMAND(-1, wxEVT_BAR_SELECTED, OnBarSelected)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_LIST_COL_CLICK(-1, OnColumnClick)
END_EVENT_TABLE()

HistogramDialog::HistogramDialog(HexWnd *hw)
: wxDialog(hw, -1, hw->GetTitle() + _T(" - Histogram"), wxDefaultPosition, wxDefaultSize,
           wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU | wxRESIZE_BORDER)
{
    wxSplitterWindow *splitter = new wxSplitterWindow(this, -1, 
        wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);

    pHistView = new HistogramView(splitter, hw, wxSize(268, 30));
    pHistView->pHist = &hgram;
    pList = new wxListCtrl(splitter, -1, wxDefaultPosition, wxSize(-1, 3 * hw->GetCharHeight()),
       wxLC_REPORT | wxLC_SINGLE_SEL | wxNO_BORDER);


    splitter->SplitHorizontally(pHistView, pList, 50);
    splitter->SetMinimumPaneSize(30);
    int lineHeight = hw->GetTextMetric().tmHeight;
    if (lineHeight < 10) lineHeight = 10;
    if (lineHeight > 50) lineHeight = 50;
    pList->SetInitialSize(wxSize(-1, 10 * lineHeight));

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    //sizer->Add(pHistView, 0, wxGROW);
    //sizer->Add(pList, 1, wxGROW);
    sizer->Add(splitter, 1, wxGROW);
    SetSizerAndFit(sizer);

    // set the minimum dialog size so that the HistogramView always has at least 1 pixel per bar.
    wxSize splitterSize = splitter->GetSize();
    wxSize histSize = pHistView->GetClientSize();
    wxSize minSize = sizer->GetMinSize();
    minSize.x = splitterSize.x + 268 - histSize.x;
    sizer->SetMinSize(minSize.x, minSize.y);
    SetSizerAndFit(sizer);

    //wxSize currentSize = GetSize();
    //SetSize(currentSize.x, currentSize.y + 50);

    Show();
    hgram.Calc(hw, pHistView);
    pHistView->UpdateView();

    pList->SetFont(hw->GetFont());
    pList->InsertColumn(0, _T("Byte"));
    pList->InsertColumn(1, _T("Char"));
    pList->InsertColumn(2, _T("Count"));
    pList->InsertColumn(3, _T("Total"));
    pList->InsertColumn(4, _T("Rank"));
    //pList->InsertColumn(5, "Ties");

    wxListItem col;
    col.SetAlign(wxLIST_FORMAT_RIGHT);
    col.SetMask(wxLIST_MASK_FORMAT);
    pList->SetColumn(2, col);
    pList->SetColumn(4, col);
    //pList->SetColumn(5, col);

    for (int i = 0; i < 256; i++)
    {
        THSIZE count = hgram.m_count[i];

        int rank = 1, ties = 0;
        for (int j = 0; j < 256; j++)
        {
            if (hgram.m_count[j] > count)
                rank++;
            else if (j != i && hgram.m_count[j] == count)
                ties++;
        }

        pList->InsertItem(i, wxString::Format(_T("%02X"), i));
        pList->SetItemData(i, i);
        pList->SetItem(i, 1, wxString((TCHAR)i));
        pList->SetItem(i, 2, FormatNumber(count, 10));
        if (hgram.m_count[i]) {
            pList->SetItem(i, 3, wxString::Format(_T("%05.2f%%"), count * 100.0 / hgram.m_nSize));
            pList->SetItem(i, 4, wxString::Format(_T("%d"), rank));
            //if (ties)
            //    pList->SetItem(i, 5, wxString::Format("%d", ties));
        }
        // else: leave the columns blank
    }


    for (int col = 0; col < 5; col++)
        pList->SetColumnWidth(col, wxLIST_AUTOSIZE_USEHEADER);
}

void HistogramDialog::OnListItemSelected(wxListEvent &event)
{
    pHistView->SetSelection(event.GetIndex());
}

void HistogramDialog::OnBarSelected(wxCommandEvent &event)
{
    int sel = event.GetInt();

    pList->SetItemState(sel, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    pList->EnsureVisible(sel);
}

void HistogramDialog::OnContextMenu(wxContextMenuEvent &event)
{
    wxPoint pt = event.GetPosition();
    if (pt == wxDefaultPosition) // message came from context menu key (VK_APPS)
    {
        //pt = pList->GetPosition();
        int item = pHistView->GetSelection();
        pList->EnsureVisible(item);
        pList->GetItemPosition(item, pt);
        wxRect rc = pList->GetClientRect();
        if (pt.y > rc.GetBottom()) pt.y = rc.GetBottom();
        pt.x = 0;

        pt += pList->GetPosition();
    }
    else
        pt = ScreenToClient(pt);

    wxMenu popup;
    popup.Append(-1, _T("Nothing"));
    PopupMenu(&popup, pt);
}

int wxCALLBACK HistogramListCompare(long item1, long item2, wxIntPtr data)
{
    HistogramDialog *dlg = (HistogramDialog*)data;
    CHistogram &hgram = dlg->hgram;
    int order = dlg->sortColumn;
    switch (order)
    {
    case 2: // by count
    case -2:
        if (hgram.m_count[item1] > hgram.m_count[item2])
            return order;
        else if (hgram.m_count[item1] < hgram.m_count[item2])
            return -order;
        // else: fall through
    case 1: // by value
    case -1:
        return (item1 - item2) * order;
    }
    return 0;
}

void HistogramDialog::OnColumnClick(wxListEvent &event)
{
    int col = event.GetColumn();
    int order;
    switch (event.GetColumn())
    {
        case 0: // byte value
        case 1: // char
            order = 1; break;
        case 2: // count
        case 3: // percent of total
            order = 2; break;
        case 4: // rank
            order = -2; break; // low rank = high count
        default:
            return;
    }
    if (order == sortColumn)
        sortColumn = -order;
    else
        sortColumn = order;
    
    pList->SortItems(HistogramListCompare, (long)this);
    for (int i = 0; i < 256; i++)
    {
        hgram.dispOrder[i] = pList->GetItemData(i);
    }
    this->pHistView->UpdateView();
}


//****************************************************************************
//****************************************************************************
// FontWidthChooserDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(FontWidthChooserDialog, wxDialog)
    EVT_BUTTON(wxID_OK, OnOK)
    EVT_LISTBOX(-1, OnSelection)
END_EVENT_TABLE()

FontWidthChooserDialog::FontWidthChooserDialog(wxWindow *parent, LOGFONT &lf)
: wxDialog(parent, -1, wxString(lf.lfFaceName) + _T(" Font")), m_lf(lf)
{
    list = new wxListBox(this, -1, wxDefaultPosition, wxDefaultSize,
        0, NULL,
        wxLB_SINGLE);

    HDC hdc = GetDC(NULL);
    EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)this, 0);
    ReleaseDC(NULL, hdc);

    sizes.Sort(CmpFunc);
    int sel = -1;
    int maxWidth = 0, maxHeight = 0;
    for (size_t i = 0; i < sizes.GetCount(); i++)
    {
        int data = sizes[i];
        int width = data & 0xFFFF, height = data >> 16;
        maxWidth = wxMax(width, maxWidth);
        maxHeight = wxMax(height, maxHeight);
        list->Append(wxString::Format(_T("%d x %d"), width, height), (void*)data);
        if (height >= abs(m_lf.lfHeight) && sel == -1)
            list->SetSelection(sel = i);
    }
    if (sel == -1)
        list->SetSelection(sel = 0);

    sample = new wxStaticText(this, -1, _T("01 23 45 67\n89 AB CD EF"), 
        wxDefaultPosition, wxDefaultSize,
        wxALIGN_CENTRE | wxSIMPLE_BORDER | wxST_NO_AUTORESIZE);
    //sample->SetBestFittingSize(wxSize(maxWidth * 12, maxHeight * 3));
    sample->SetInitialSize(wxSize(maxWidth * 12, maxHeight * 3));

    wxButton *btnOK = new wxButton(this, wxID_OK, _T("&OK"));
    wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _T("&Cancel"));
    btnOK->SetDefault();

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, -1, _T("Character size in pixels")), 0, wxALL & ~wxBOTTOM, 5);
    sizer->Add(list, 1, wxGROW | wxALL & ~wxTOP, 5);
    sizer->Add(new wxStaticText(this, -1, _T("Sample")), 0, wxLEFT, 5);
    sizer->Add(sample, 0, wxALL & ~wxTOP, 5);
    wxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(btnOK, 0, wxALL, 5);
    buttonSizer->Add(btnCancel, 0, wxALL, 5);
    sizer->Add(buttonSizer);
    SetSizerAndFit(sizer);
}

int CALLBACK FontWidthChooserDialog::EnumFontFamExProc(
    ENUMLOGFONTEX *lpelfe,    // logical-font data
    NEWTEXTMETRICEX *lpntme,  // physical-font data
    DWORD FontType,           // type of font
    LPARAM lParam)            // application-defined data
{
    LOGFONT &lf = lpelfe->elfLogFont;
    FontWidthChooserDialog *dlg = (FontWidthChooserDialog*)lParam;
    int data = lf.lfWidth | (lf.lfHeight << 16);
    if (dlg->sizes.Index(data) == wxNOT_FOUND) // 8 x 12 gets enumerated twice.  I don't know why.
        dlg->sizes.Add(data);
    return 1;
}

void FontWidthChooserDialog::OnOK(wxCommandEvent &event)
{
    int data = sizes[list->GetSelection()];
    m_lf.lfWidth = data & 0xFFFF;
    m_lf.lfHeight = data >> 16;
    EndModal(wxID_OK);
}

void FontWidthChooserDialog::TestFont(int sel)
{
    LOGFONT lf = m_lf;
    lf.lfWidth = sizes[sel] & 0xFFFF;
    lf.lfHeight = sizes[sel] >> 16;
    sample->SetFont(wxCreateFontFromLogFont(&lf));
}

//****************************************************************************
//****************************************************************************
// wxRecentChoice
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(thRecentChoice, wxComboBox)
    EVT_TEXT(-1, OnTextChange)
    //EVT_CHAR(OnChar)
    //EVT_CHAR_HOOK(OnChar)
    EVT_COMMAND_ENTER(-1, OnEnter)
END_EVENT_TABLE()

thRecentChoice::thRecentChoice(wxWindow *parent,
                               wxWindowID id,
                               wxArrayString &choices,
                               wxString value /*= wxEmptyString*/,
                               int nMax /*= -1*/,
                               wxPoint position /*= wxDefaultPosition*/,
                               wxSize size /*= wxDefaultSize*/,
                               const wxValidator& validator /*= wxDefaultValidator*/)
: wxComboBox(parent, id, value, position, size, choices, wxCB_DROPDOWN, validator),
m_choices(choices), m_nMax(nMax), m_bSave(true)
{
    if (choices.GetCount() == 0)
        Append(wxEmptyString);
}

thRecentChoice::thRecentChoice(wxWindow *parent,
                               wxWindowID id,
                               wxArrayString &choices,
                               int selection,
                               int nMax /*= -1*/,
                               wxPoint position /*= wxDefaultPosition*/,
                               wxSize size /*= wxDefaultSize*/,
                               const wxValidator& validator /*= wxDefaultValidator*/)
: wxComboBox(parent, id, wxEmptyString, position, size, choices, wxCB_DROPDOWN, validator),
m_choices(choices), m_nMax(nMax)
{
    if (choices.GetCount() == 0)
        Append(wxEmptyString);
    SetSelection(selection);
}

//void thRecentChoice::OnChar(wxKeyEvent &event)
//{
//   if (event.GetKeyCode() == WXK_ESCAPE)
//   {
//      
//   }
//   else
//      event.Skip();
//}

void thRecentChoice::Finalize()
{
    wxString val = GetValue();
    for (size_t i = 0; i < m_choices.GetCount(); i++)
    {
        if (m_choices[i] == val)
        {
            m_choices.RemoveAt(i);
            break;
        }
    }

    m_choices.Insert(val, 0);
    if (m_nMax > 0 && (int)m_choices.GetCount() > m_nMax)
        m_choices.RemoveAt(m_nMax, m_choices.GetCount() - m_nMax);

    //for (size_t i = 0; i < m_choices.GetCount(); i++)
    //    PRINTF("%2d %s\n", i, m_choices[i].c_str());
}

void thRecentChoice::OnTextChange(wxCommandEvent &event)
{
    event.Skip();
}

wxSize thRecentChoice::DoGetBestSize() const
{
    wxSize size = wxComboBox::DoGetBestSize();
    return size;
}

void thRecentChoice::OnEnter(wxCommandEvent &event)
{

}

//****************************************************************************
//****************************************************************************
// thRecentChoiceDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(thRecentChoiceDialog, wxDialog)
    EVT_BUTTON(wxID_OK, OnOK)
    //EVT_TEXT(-1, OnTextChange)
END_EVENT_TABLE()

thRecentChoiceDialog::thRecentChoiceDialog(wxWindow *parent,
                                           wxString title, wxString message,
                                           wxArrayString &choices,
                                           int nMax /*= -1*/)
: wxDialog(parent, -1, title, wxDefaultPosition, wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxSizer *vsizer = new wxBoxSizer(wxVERTICAL);
    vsizer->Add(new wxStaticText(this, -1, message), 0, wxALL, 10);

    int x, y;
    GetTextExtent(_T("0x0123456789ABCDEF mmm"), &x, &y);
    cmb = new thRecentChoice(this, -1, choices, wxEmptyString, nMax, wxDefaultPosition, wxSize(x, -1));
    vsizer->Add(cmb, 0, wxALL | wxEXPAND | wxADJUST_MINSIZE, 10);
    //wxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    //wxButton *btnOK = new wxButton(this, wxID_OK, "&OK");
    //btnOK->SetDefault();
    //hsizer->Add(btnOK);
    //hsizer->Add(new wxButton(this, wxID_CANCEL, "&Cancel"));
    wxSizer *hsizer = CreateButtonSizer(wxOK | wxCANCEL);
    if (hsizer)
        vsizer->Add(hsizer, 0, wxALL, 10);
    SetSizerAndFit(vsizer);
    cmb->SetFocus();
}

void thRecentChoiceDialog::OnOK(wxCommandEvent &event)
{
    cmb->Finalize();
    EndModal(wxID_OK);
}

//****************************************************************************
//****************************************************************************
// SlowReadDialog
//****************************************************************************
//****************************************************************************

static SlowReadDialog *g_srd;

VOID CALLBACK SlowReadCompletion(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped)
{
    g_srd->OnComplete(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
}


BEGIN_EVENT_TABLE(SlowReadDialog, wxDialog)
    EVT_TIMER(-1, OnTimer)
END_EVENT_TABLE()

SlowReadDialog::SlowReadDialog(wxWindow *parent, HANDLE hFile)
: wxDialog(parent, -1, _T("Read Special"), wxDefaultPosition, wxDefaultSize),
timer(this)
{
    g_srd = this;
    fwb = new ModifyBuffer();
    fwb->Alloc();

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    label1 = new wxStaticText(this, -1, ZSTR);
    sizer->Add(label1);
    label2 = new wxStaticText(this, -1, ZSTR);
    sizer->Add(label2);
    sizer->Add(new wxButton(this, wxID_CANCEL, _T("&Cancel")));
    SetSizerAndFit(sizer);

    dtStart = wxDateTime::Now();
    timer.Start(1000, false);

    overlapped.Offset = 0;
    overlapped.OffsetHigh = 0;
    overlapped.hEvent = 0;
    ReadFileEx(hFile, *fwb, 16 * 1024, &overlapped, ::SlowReadCompletion);
}

SlowReadDialog::~SlowReadDialog()
{
    delete fwb;
}

void SlowReadDialog::OnTimer(wxTimerEvent &event)
{
    label1->SetLabel((wxDateTime::Now() - dtStart).Format());
}

void SlowReadDialog::OnComplete(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped)
{
    label2->SetLabel(wxString::Format(_T("%d bytes"), fwb->GetSize()));
    ReadFileEx(hFile, *fwb, 16 * 1024, &overlapped, ::SlowReadCompletion);
}

//****************************************************************************
//****************************************************************************
// thPipeOutDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(thPipeOutDialog, wxDialog)
//    EVT_BUTTON(wxID_OK, OnOK)
    EVT_CHECKBOX(IDC_SHELL, OnChkShell)
END_EVENT_TABLE()

thPipeOutDialog::thPipeOutDialog(wxWindow *parent)
: wxDialog(parent, -1, _T("Special Output"), wxDefaultPosition, wxDefaultSize,
           wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU | wxRESIZE_BORDER)
{
    //! Should be command.com on Win95/98/ME, but I don't care.  Also check COMSPEC.
    m_sShell = wxGetenv(_T("COMSPEC"));
    if (!m_sShell)
       m_sShell = _T("cmd.exe");
    m_sArgPrefix = _T("/c");

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(hSizer);
    wxArrayString choices;

    choices.Add(_T("&Whole document"));
    choices.Add(_T("Selection &only"));
    rgWhat = new wxRadioBox(this, -1, _T("Send What"),
        wxDefaultPosition, wxDefaultSize, choices, 1);
    rgWhat->SetSelection(0);
    hSizer->Add(rgWhat, 0, wxALL, 10);
    choices.Clear();

    choices.Add(_T("&Pipe to process"));
    choices.Add(_T("Special &file"));
    rgWhither = new wxRadioBox(this, -1, _T("Send Whither"),
        wxDefaultPosition, wxDefaultSize, choices, 1);
    rgWhither->SetSelection(0);
    hSizer->Add(rgWhither, 0, wxALL, 10);
    choices.Clear();

    chkShell = new wxCheckBox(this, IDC_SHELL, _T("Default &shell"));
    sizer->Add(chkShell, 0, wxALL, 10);

    wxFlexGridSizer *fgSizer = new wxFlexGridSizer(2, 10, 10);
    fgSizer->AddGrowableCol(1);
    fgSizer->Add(new wxStaticText(this, -1, _T("&Target file or command")), 0, wxALIGN_CENTER_VERTICAL);
    txtFile = new thRecentChoice(this, -1, appSettings.asSpawnCmd);
    txtFile->SetSize(200, 20);
    fgSizer->Add(txtFile, 0, wxGROW);
    fgSizer->Add(new wxStaticText(this, -1, _T("&Arguments")), 0, wxALIGN_CENTER_VERTICAL);
    txtArgs = new thRecentChoice(this, -1, appSettings.asSpawnArgs);
    txtArgs->SetSize(200, 20);
    fgSizer->Add(txtArgs, 0, wxGROW);
    fgSizer->Add(new wxStaticText(this, -1, _T("Working &directory")), 0, wxALIGN_CENTER_VERTICAL);
    txtDir = new thRecentChoice(this, -1, appSettings.asSpawnPath);
    txtDir->SetSize(200, 20);
    fgSizer->Add(txtDir, 0, wxGROW);
    sizer->Add(fgSizer, 0, wxGROW | wxALL, 10);

    wxSizer *buttonSizer = CreateButtonSizer(wxOK | wxCANCEL);
    if (buttonSizer)
        sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    //wxButton *btnOK = new wxButton(this, wxID_OK, "&OK");
    //btnOK->SetDefault();
    //sizer->Add(btnOK, 0, wxALL, 10);
    //sizer->Add(new wxButton(this, wxID_CANCEL, "&Cancel"), 0, wxALL, 10);
    SetSizerAndFit(sizer);
}

void thPipeOutDialog::Init(const Selection sel)
{
    rgWhat->SetSelection(sel.GetSize() ? 1 : 0);
    if (txtFile->GetCount())
        txtFile->SetSelection(0);
    if (txtArgs->GetCount())
        txtArgs->SetSelection(0);
    if (txtDir->GetCount())
        txtDir->SetSelection(0);
}

//void thPipeOutDialog::OnOK(wxCommandEvent &event)
//{
//    End
//}

void thPipeOutDialog::OnChkShell(wxCommandEvent &event)
{
    if (event.IsChecked())
    {
        txtFile->Disable();
        txtFile->SetValue(m_sShell + _T(" ") + m_sArgPrefix);
    }
    else
    {
        txtFile->Enable();
        txtFile->Clear();
    }
}

//****************************************************************************
//****************************************************************************
// OpenDriveDialog
//****************************************************************************
//****************************************************************************

BEGIN_EVENT_TABLE(OpenDriveDialog, wxDialog)
    EVT_LIST_ITEM_ACTIVATED(-1, OnActivate)
    EVT_BUTTON(wxID_OK, OnOK)
    EVT_CHECKBOX(IDC_ExactSize, OnExactSize)
END_EVENT_TABLE()

void OpenDriveDialog::OnOK(wxCommandEvent &event)
{
    if (event.GetId() == wxID_OK)
    {
        m_bReadOnly = !!chkReadOnly->GetValue();
        m_bExactSize = !!chkExactSize->GetValue();
        long item = lc->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item >= 0)
            m_sTarget = devlist[item].devPath;
    }
    EndModal(event.GetId());
}

//*******************************************************************
//#include "native_pas.cpp"

OpenDriveDialog::OpenDriveDialog(wxWindow *parent)
: wxDialog(parent, -1, _T("Open Drive"),
           wxDefaultPosition, wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_bExactSize = appSettings.bExactDriveSize;

    thBlockDevices devs; // get list of devices we can read

    wxArrayString VolumeNames;
    wxString label, fsys;
    for (size_t n = 0; n < devs.devs.size(); n++)
    {
        thBlockDevice &dev = devs.devs[n];
        if (dev.volume >= 0)  // CD and floppy drives
        {
            thVolume &vol = devs.volumes[dev.volume];
            if (VolumeNames.Index(vol.name) >= 0) // already added?
                continue;
            VolumeNames.Add(vol.name);
            if (!vol.size() || !vol.GetVolumeInformation(label, fsys))
                label = fsys = wxEmptyString;
            DriveListInfo dli(dev.driveNum, dev.name,
                0, vol.de.ExtentLength.QuadPart,
                vol.GetPaths(), label, fsys, dev.type, vol.GetDevPath());
            devlist.push_back(dli);
        }
        else  // hard disks, probably Windows "basic disks" with MBR and one or more partitions.
        {
            // Add the whole drive to the list, then each partition separately.
            DriveListInfo dli(dev.driveNum, dev.name, 0, dev.size, ZSTR, ZSTR, ZSTR, dev.type, dev.GetDevPath());
            devlist.push_back(dli);

            for (size_t p = 0; p < dev.Partitions.size(); p++)
            {
                DISK_EXTENT &part = dev.Partitions[p];
                if (part.ExtentLength.QuadPart == 0)
                    continue;
                DriveListInfo dli(p, wxString::Format(_T("%d %d"), dev.driveNum, p),
                    part.StartingOffset.QuadPart, part.ExtentLength.QuadPart);
                dli.isPartition = true;
                if (part.DiskNumber >= 0) // structure abuse!
                {
                    thVolume &vol = devs.volumes[part.DiskNumber];
                    if (VolumeNames.Index(vol.name) >= 0) // already added?
                        continue;
                    dli.paths = vol.GetPaths();
                    if (vol.size() && vol.GetVolumeInformation(label, fsys))
                    {
                        dli.label = label;
                        dli.fsType = fsys;
                    }
                    dli.devType = _T("Partition");
                    dli.devPath = vol.GetDevPath();
                    VolumeNames.Add(vol.name);
                    devlist.push_back(dli);
                }
            }
        }
    }

    lc = new wxListCtrl(this, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    lc->InsertColumn(0, _T("Type"));
    lc->InsertColumn(1, _T("#"));
    lc->InsertColumn(2, _T("Device"));
    lc->InsertColumn(3, _T("Size"));
    lc->InsertColumn(4, _T("Path(s)"));
    lc->InsertColumn(5, _T("Label"));
    lc->InsertColumn(6, _T("FS"));

    wxArrayInt imgIndexes = MakeDriveImageList();
#if wxCHECK_VERSION(2, 8, 0)
    lc->AssignImageList(m_imgList, wxIMAGE_LIST_SMALL);
#else
    lc->SetImageList(m_imgList, wxIMAGE_LIST_SMALL);
#endif

    for (size_t i = 0; i < devlist.size(); i++)
    {
        const DriveListInfo &dli = devlist[i];
        lc->InsertItem(i, dli.devType, imgIndexes[i]);
        lc->SetItem(i, 1, dli.isPartition ? ZSTR : wxString::Format(_T("%d"), dli.devNum));
        lc->SetItem(i, 2, dli.name);
        SetItemSizeOffset(i);
        lc->SetItem(i, 4, dli.paths);
        lc->SetItem(i, 5, dli.label);
        lc->SetItem(i, 6, dli.fsType);
    }

    for (int i = 0; i < 7; i++)
      lc->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);

    wxSizer *vsizer = new wxBoxSizer(wxVERTICAL);
    wxRect rc;
    if (devlist.size())
    {
        lc->GetItemRect(devlist.size() - 1, rc, wxLIST_RECT_BOUNDS);
        lc->SetInitialSize(wxSize(rc.GetRight() + rc.GetHeight(), rc.GetBottom() + rc.GetHeight()));
    }
    vsizer->Add(lc, 1, wxGROW);

    wxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    chkReadOnly = new wxCheckBox(this, -1, _T("&Read only"));
    chkReadOnly->SetValue(appSettings.bDefaultReadOnly);
    chkExactSize = new wxCheckBox(this, IDC_ExactSize, _T("Show &exact size"));
    chkExactSize->SetValue(appSettings.bExactDriveSize);
    hsizer->Add(chkReadOnly, 0, wxALL, 10);
    hsizer->Add(chkExactSize, 0, wxALL, 10);
    hsizer->Add(1, 1, 1);
    wxButton *btnOK = new wxButton(this, wxID_OK, _T("&OK"));
    btnOK->SetDefault();
    wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _T("&Cancel"));
    hsizer->Add(btnCancel, 0, wxALL, 10);
    hsizer->Add(btnOK, 0, wxALIGN_RIGHT | wxALL, 10);
    vsizer->Add(hsizer, 0, wxGROW);
    SetSizerAndFit(vsizer);

    lc->SetInitialSize(lc->GetBestSize());
}

OpenDriveDialog::~OpenDriveDialog()
{
    //! I was going to update appSettings here...
}

void OpenDriveDialog::OnActivate(wxListEvent &event)
{
    m_bReadOnly = !!chkReadOnly->GetValue();
    m_bExactSize = !!chkExactSize->GetValue();
    m_sTarget = devlist[event.GetIndex()].devPath;
    EndModal(wxID_OK);
}

void OpenDriveDialog::OnExactSize(wxCommandEvent &event)
{
    m_bExactSize = event.IsChecked();
    for (size_t n = 0; n < devlist.size(); n++)
        SetItemSizeOffset(n);
}

void OpenDriveDialog::SetItemSizeOffset(int item)
{
    const DriveListInfo &dli = devlist[item];
    if (m_bExactSize)
    {
        if (dli.isPartition)
            lc->SetItem(item, 2, wxString::Format(_T("Partition %d at "), dli.devNum) + FormatDec(dli.offset));
        lc->SetItem(item, 3, FormatDec(dli.size));
    }
    else
    {
        if (dli.isPartition)
            lc->SetItem(item, 2, wxString::Format(_T("Partition %d at "), dli.devNum) + FormatBytes(dli.offset));
        lc->SetItem(item, 3, FormatBytes(dli.size));
    }
}

wxArrayInt OpenDriveDialog::MakeDriveImageList()
{
    // USB, Disk, Partition, CD, Floppy
    //! Todo: use shell icons for drives?

    std::vector<HICON> hIcons;
    int typeIconIndex[5] = {-1, -1, -1, -1, -1};
    wxArrayInt indexes;
    int type, idx;
    SHFILEINFO shfi;
    HMODULE shell32 = LoadLibraryEx(_T("shell32"), 0, LOAD_LIBRARY_AS_DATAFILE);
    int smIconX = GetSystemMetrics(SM_CXSMICON);
    int smIconY = GetSystemMetrics(SM_CYSMICON);

    for (size_t n = 0; n < devlist.size(); n++)
    {
        DriveListInfo &dli = devlist[n];
        if (dli.paths.Len())
        {
            SHGetFileInfo(dli.paths.BeforeFirst(';'), 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON);
            indexes.Add(hIcons.size());
            hIcons.push_back(shfi.hIcon);
            continue;
        }

        idx = -1;
        if (!dli.devType.Cmp(_T("USB Disk")))
            type = 0;
        else if (!dli.devType.Cmp(_T("Disk")))
            type = 1, idx = 9;
        else if (!dli.devType.Cmp(_T("CD/DVD")))
            type = 2;
        else if (!dli.devType.Cmp(_T("Floppy")))
            type = 3;
        else if (!dli.devType.Cmp(_T("Partition")))
            type = 4;
        else
            indexes.Add(-1);  // I don't know what this is.  No icon.
        if (typeIconIndex[type] < 0 && idx >= 0)
        {
            typeIconIndex[type] = hIcons.size();
            hIcons.push_back((HICON)LoadImage(shell32, MAKEINTRESOURCE(idx), IMAGE_ICON, smIconX, smIconY, 0));
        }
        indexes.Add(typeIconIndex[type]);
    }

    m_imgList = new wxImageList(smIconX, smIconY, true, hIcons.size());
    for (size_t n = 0; n < hIcons.size(); n++)
    {
       wxIcon icon;
       icon.SetHICON(hIcons[n]);
       m_imgList->Add(icon);
       // Our app should delete this icon.
    }

    FreeLibrary(shell32);
    return indexes;
}

AboutDialog::AboutDialog(wxWindow *parent)
: wxDialog(parent, -1, _T("About T.Hex"), wxDefaultPosition)
{
    wxString msg =
        _T("Tyrannosaurus Hex is a futuristic yet fatally flawed hex editor for Windows (and someday, Linux).\n")
        _T("Copyright 2006-2008 Adam Bailey\n")
#ifdef _DEBUG
        _T("Debug ")
#else
        _T("Release ")
#endif
#ifdef _UNICODE
        _T("Unicode build ")
#else
        _T("ANSI build ")
#endif
        _T("compiled ") _T(__DATE__) _T("\n")
        ;
    wxFileName fn(wxGetApp().argv[0]);
    msg += _T("File path: ") + fn.GetPath() + _T("\n");
    msg += _T("Current directory: ") + wxGetCwd() + _T("\n");
    msg += _T("\n");

    msg += _T("Credit goes to\n");
    msg += _T("   James Brown (my hero), for Hexedit and programming tutorials.  http://catch22.net/\n");
    msg += _T("   wxWidgets developers, for making portability possible.\n");
    msg += _T("   Henry Spencer, for wxRegEx.\n");
    msg += _T("   libdisasm.  http://bastard.sourceforge.net/libdisasm.html\n");
    msg += _T("   Jay A. Key, for the CD extraction library AKRip.  http://akrip.sourceforge.net/intro.html\n");
    msg += _T("   Simon Tatham, for fancy data structures and friendly advice.\n");
    msg += _T("   icy's Hexplorer.  http://artemis.wszib.edu.pl/~mdudek/\n");
    msg += _T("   Marty, for an excuse to add data recovery features.\n");
    msg += _T("   Sarah, for the fierce name ;)\n");

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, -1, msg), 0, wxALL, 5);
    wxButton *btnOK = new wxButton(this, wxID_OK, _T("&Groovy!"));
    btnOK->SetDefault();
    sizer->Add(btnOK, 0, wxCENTER | wxALL, 5);
    SetSizerAndFit(sizer);
}
