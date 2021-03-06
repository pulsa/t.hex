// -*- C++ -*- generated by wxGlade 0.4.1 on Sun May 28 10:49:42 2006

#include <wx/wx.h>
#include <wx/image.h>

#ifndef GOTODIALOG_H
#define GOTODIALOG_H

// begin wxGlade: ::dependencies
// end wxGlade


class GotoDialog: public wxDialog {
public:
    // begin wxGlade: GotoDialog::ids
    // end wxGlade

    GotoDialog(wxWindow* parent, int id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_DIALOG_STYLE);

private:
    // begin wxGlade: GotoDialog::methods
    void set_properties();
    void do_layout();
    // end wxGlade

protected:
    // begin wxGlade: GotoDialog::attributes
    wxStaticText* label_1;
    wxTextCtrl* txtAddress;
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
}; // wxGlade: end class


#endif // GOTODIALOG_H
