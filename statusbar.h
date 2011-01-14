#pragma once

class thStatusBar : public wxStatusBar
{
public:
    enum {
        SBF_STATUS = 0,
        SBF_SELECTION,
        SBF_VALUE,
        SBF_ENDIANMODE,
        SBF_CHARSET,
        SBF_FILESIZE,
        SBF_EDITMODE,
        SB_FIELD_COUNT};

    thStatusBar(wxFrame *parent, HexWnd *hw);

    //void SetHexWnd(HexWnd *hw);
    void UpdateView(HexWnd *hw, int flags = -1);

    void SetStatus(wxString str);
    void SetSelection();
    void SetEndianMode();
    void SetCharSet();
    void SetFileSize();
    void SetEditMode();

protected:
    int HitTest(const wxPoint &pt) const;
    void FitPane(int pane);
    bool SetDynamicPane(const wxString &str, int pane, bool bForceUpdate = true);

    int widths[SB_FIELD_COUNT], unusableWidth[SB_FIELD_COUNT], minWidth[SB_FIELD_COUNT];
    HexWnd *hw;
    int m_lastField;
    THBASE *pBase;

    DWORD dynamicResizeTime[SB_FIELD_COUNT];

    DECLARE_EVENT_TABLE()

    enum {IDM_EXACT_SIZE = 100};
    void OnContextMenu(wxContextMenuEvent &event);
    void OnDoubleClick(wxMouseEvent &event);
    void ToggleExactSize(wxCommandEvent &event);
    void OnBase(wxCommandEvent &event);
};
