#include "precomp.h"
#include "hexwnd.h"
#include "resource.h"
#include "thex.h"
#include "thFrame.h"
//#include "wx/msw/private.h"
#include "statusbar.h"
#include "toolwnds.h"

//*****************************************************************************
//*****************************************************************************
// thStatusBar
//*****************************************************************************
//*****************************************************************************

BEGIN_EVENT_TABLE(thStatusBar, wxStatusBar)
    EVT_LEFT_DCLICK(OnDoubleClick)
    EVT_CONTEXT_MENU(OnContextMenu)
    EVT_MENU(IDM_EXACT_SIZE, ToggleExactSize)
    EVT_MENU(IDM_BASE_DEC, OnBase)
    EVT_MENU(IDM_BASE_HEX, OnBase)
    EVT_MENU(IDM_BASE_BOTH, OnBase)
    EVT_MENU(IDM_BASE_ALL, OnBase)
END_EVENT_TABLE()

thStatusBar::thStatusBar(wxFrame *parent, HexWnd *hw)
: wxStatusBar(parent)
{
    memset(unusableWidth, 0, sizeof(unusableWidth));
    widths[SBF_STATUS] = -1;
    widths[SBF_SELECTION] = 150;
    widths[SBF_VALUE] = 150;
    widths[SBF_ENDIANMODE] = 60;
    widths[SBF_CHARSET] = 60;
    widths[SBF_FILESIZE] = 100;
    widths[SBF_EDITMODE] = 60;
    memcpy(minWidth, widths, sizeof(minWidth));
    SetFieldsCount(SB_FIELD_COUNT, widths);
    this->hw = NULL;
    UpdateView(hw);
    parent->SetStatusBar(this);

    memset(dynamicResizeTime, 0, sizeof(dynamicResizeTime));
}

void thStatusBar::UpdateView(HexWnd *hw, int flags /*= -1*/)
{
    if (hw != this->hw)
        flags = DataView::DV_ALL;
    this->hw = hw;

    if (FLAGS(flags, DataView::DV_ALL))
    {
        SetStatus(wxEmptyString);
        SetCharSet();
        SetEditMode();
    }

    if (FLAGS(flags, DataView::DV_SELECTION))
        SetSelection();

    if (FLAGS(flags, DataView::DV_DATASIZE))
        SetFileSize();

    if (FLAGS(flags, DataView::DV_ALL))
        SetEndianMode();
}

void thStatusBar::SetStatus(wxString str)
{
}

void thStatusBar::SetSelection()
{
    if (!hw)
    {
        SetStatusText(wxEmptyString, SBF_SELECTION);
        return;
    }

    Selection sel = hw->GetSelection();
    wxString str;
    int base = appSettings.sb.SelectionBase;
    THSIZE size = sel.GetSize();
    if (size == 0)
        str = _T("Cursor: ") + FormatWithBase(sel.nStart, base); //! todo: print digit?
    else
        str = _T("Selection: ") + FormatWithBase(sel.nStart + hw->doc->display_address, base) +
              _T(" - ") + FormatWithBase(sel.nEnd + hw->doc->display_address, base);
    //SetStatusText(str, SBF_SELECTION);
    bool changed = SetDynamicPane(str, SBF_SELECTION, false);

    base = appSettings.sb.ValueBase;
    if (size == 0)
        str = FormatWithBase((int64)hw->doc->GetAt(sel.nStart), base);
    else if (size >= 1 && size <= 8)
    {
        union {
            uint64 ull;
            uint8 byte[8];
        } data = { 0 };
        hw->doc->Read(sel.GetFirst(), size, data.byte);
        if (hw->s.iEndianMode != NATIVE_ENDIAN_MODE) {
            if (hw->s.iEndianMode == BIGENDIAN_MODE)
                reverse(data.byte, size);
            else
                reverse(data.byte, 8);
        }
        str = wxString::Format(_T("%I64d Byte%s: "), size, size > 1 ? _T("s") : ZSTR);

        // "The text for each part is limited to 127 characters."  -MSDN.
        // We hit this limit when printing 64-bit numbers in three bases.
        // That's way too much text, anyway; so let's restrict things a little bit.
        if (size > 4 && base == BASE_ALL)
            base = BASE_BOTH;

        if (appSettings.sb.bSignedValue)
            //str += FormatDec((int64)data.ull);
            str += FormatWithBase((int64)data.ull, base);
        else
            //str += FormatDec(data.ull);
            str += FormatWithBase(data.ull, base);
    }
    else
    {
        if (base == BASE_OCT || base == BASE_ALL)
            base = BASE_BOTH;
        str = _T("Size: ") + FormatWithBase(size, base);
    }
    //SetStatusText(str, SBF_VALUE);
    SetDynamicPane(str, SBF_VALUE, changed);
}


void thStatusBar::SetEndianMode()
{
    if (!hw) {
        SetStatusText(_T("   "), SBF_ENDIANMODE);
        FitPane(SBF_ENDIANMODE);
        return;
    }

    wxString str;
    if (hw->s.iEndianMode == LITTLEENDIAN_MODE)
        str = _T("Little");
    else
        str = _T("Big");
    SetStatusText(str, SBF_ENDIANMODE);
    FitPane(SBF_ENDIANMODE);

    if (0)
    {
        int pane = SBF_ENDIANMODE;
        wxRect rect;
        GetFieldRect(pane, rect);
        int bx = GetBorderX();
        int tmp = rect.width + bx - widths[pane];
        tmp = tmp;
    }

}

void thStatusBar::SetCharSet()
{
}

void thStatusBar::SetFileSize()
{
    if (!hw || !hw->doc)
    {
        SetStatusText(_T("No file"), SBF_FILESIZE);
        FitPane(SBF_FILESIZE);
        return;
    }

    wxString str;
    THSIZE size = hw->doc->GetSize();

    if (appSettings.sb.bExactFileSize)
    {
        str = FormatWithBase(size, appSettings.sb.FileSizeBase) + Plural(size, _T(" byte"));
    }
    else if (size >= 1024 && appSettings.sb.bFileSizeInKB) // Windows-style size in kB
        str.Printf(_T("%s kB"), FormatDouble(size / 1024.0, 1).c_str());
    else // human-readable approximation of file size
        str = FormatBytes(size);
    SetStatusText(str, SBF_FILESIZE);
    FitPane(SBF_FILESIZE);
}

void thStatusBar::SetEditMode()
{
    wxString str;
    if (!hw)
        str = _T("   ");
    else if (hw->doc->IsReadOnly())
        str = _T("READ");
    else if (appSettings.bInsertMode)
        str = _T("INS");
    else
        str = _T("OVR");
    SetStatusText(str, SBF_EDITMODE);
    FitPane(SBF_EDITMODE);
}

void thStatusBar::FitPane(int pane)
{
    wxString str = GetStatusText(pane);
    int x, y;
    GetTextExtent(str, &x, &y);
    if (pane == SB_FIELD_COUNT - 1)
        widths[pane] = x + GetSize().y;
    else
        widths[pane] = x + 5; //unusableWidth[pane];
    SetStatusWidths(SB_FIELD_COUNT, widths);
    //if (unusableWidth[pane] == 0)
    //{
    //    wxRect rect;
    //    GetFieldRect(pane, rect);
    //    int bx = GetBorderX();
    //    unusableWidth[pane] = rect.width - x;
    //    widths[pane] += unusableWidth[pane];
    //    SetStatusWidths(SB_FIELD_COUNT, widths);
    //}
}

//! This works, but it looks funny when you're paging along and your status text slides over.
//  A better idea might be to keep track of the minimum size required, and adjust all dynamic
//  panes when one field needs more space, or the window size changes.
// AEB  2008-05-12
bool thStatusBar::SetDynamicPane(const wxString &str, int pane, bool bForceUpdate /*= true*/)
{
    SetStatusText(str, pane);

    int x, y;
    GetTextExtent(str, &x, &y);
    if (pane == SB_FIELD_COUNT - 1)
        x += GetSize().y;
    else
        x += 5; //unusableWidth[pane];
    x = wxMax(x, minWidth[pane]);

    DWORD now = GetTickCount();
    bool changed = false;
    if (x > widths[pane]) {
        widths[pane] = x;
        changed = true;
        dynamicResizeTime[pane] = now;
    }
    else if (x < widths[pane] &&
             //now - dynamicResizeTime[pane] > 400) // this is annoying
             now - dynamicResizeTime[pane] > 800)
    {
        //widths[pane]--;  // too slow if not holding down arrow keys
        //widths[pane] -= DivideRoundUp(widths[pane] - x, 20);  // too fast if holding arrow keys

        // up to 5 pixels per second
        int diff = (now - dynamicResizeTime[pane]) / 200;
        diff = wxMin(diff, 50);                 // limit to this many
        diff = wxMin(diff, widths[pane] - x);   // don't chop off text
        
        widths[pane] -= diff;
        dynamicResizeTime[pane] = now;
        changed = true;
    }
    if (changed || bForceUpdate)
        SetStatusWidths(SB_FIELD_COUNT, widths);
    return changed;
}

void thStatusBar::OnContextMenu(wxContextMenuEvent &event)
{
    //! Base selection for SBF_VALUE really ought to be more flexible,
    //  offering type, size, base, separators, prefix+suffix, etc.

    wxPoint pt = ScreenToClient(event.GetPosition());
    wxMenu popup;
    bool showBase = false;
    pBase = NULL;
    m_lastField = HitTest(pt);
    switch (m_lastField)
    {
    case -1:
    case SBF_STATUS:
        break;
    case SBF_SELECTION:
        showBase = true;
        pBase = &appSettings.sb.SelectionBase;
        break;
    case SBF_VALUE:
        showBase = true;
        pBase = &appSettings.sb.ValueBase;
        break;
    case SBF_ENDIANMODE:
    case SBF_CHARSET:
        break;
    case SBF_FILESIZE:
        showBase = true;
        pBase = &appSettings.sb.FileSizeBase;
        break;
    case SBF_EDITMODE:
    default:
        break;
    }

    popup.AppendCheckItem(IDM_ViewStatusBar, _T("Show status bar"));
    popup.Check(IDM_ViewStatusBar, appSettings.bStatusBar); // always there if we can click it

    if (m_lastField == SBF_FILESIZE)
    {
        popup.AppendSeparator();
        popup.AppendCheckItem(IDM_EXACT_SIZE, _T("E&xact File Size"));
        popup.Check(IDM_EXACT_SIZE, appSettings.sb.bExactFileSize);
    }
    if (showBase && pBase)
    {
        popup.AppendSeparator();
        popup.AppendRadioItem(IDM_BASE_DEC, _T("&Dec"));
        popup.AppendRadioItem(IDM_BASE_HEX, _T("&Hex"));
        popup.AppendRadioItem(IDM_BASE_BOTH, _T("Dec + Hex"));
        //popup.AppendRadioItem(IDM_BASE_BIN, _T("&Bin"));
        if (m_lastField == SBF_VALUE)
            popup.AppendRadioItem(IDM_BASE_ALL, _T("Dec + Hex + Bin"));
        popup.Check(IDM_BASE_DEC + *pBase - BASE_DEC, true);
    }

    PopupMenu(&popup, pt);
}

int thStatusBar::HitTest(const wxPoint &pt) const
{
    wxRect rc;
    for (int i = 0; i < SB_FIELD_COUNT; i++)
    {
        GetFieldRect(i, rc);
        if (rc.Contains(pt))
            return i;

        // SB_GETRECT returns a stupid size for the far-right field, so fix it here.
        if (i == SB_FIELD_COUNT - 1 && pt.x >= rc.x)
            return i;
    }
    return -1;
}

void thStatusBar::ToggleExactSize(wxCommandEvent &WXUNUSED(event))
{
    appSettings.sb.bExactFileSize = !appSettings.sb.bExactFileSize;
    SetFileSize();
}

void thStatusBar::OnDoubleClick(wxMouseEvent &event)
{
    wxCommandEvent cmd(wxEVT_COMMAND_MENU_SELECTED);
    int field = HitTest(event.GetPosition());
    switch (field)
    {
    case -1:
    case SBF_STATUS:
    case SBF_SELECTION: //! todo: popup up Goto/SetSelection box?
    case SBF_VALUE: //! todo: pop up value editor?
    case SBF_CHARSET:
    case SBF_FILESIZE:
    default:
        return;
    case SBF_ENDIANMODE:
        cmd.SetId(IDM_ToggleEndianMode);
        break;
    case SBF_EDITMODE:
        cmd.SetId(IDM_ToggleInsert);
        break;
    }
    GetParent()->GetEventHandler()->AddPendingEvent(cmd);
}

void thStatusBar::OnBase(wxCommandEvent &event)
{
    int base = event.GetId() + BASE_DEC - IDM_BASE_DEC;
    if (base != BASE_DEC &&
        base != BASE_HEX &&
        base != BASE_BOTH &&
        base != BASE_ALL)
        return;
    if (base == BASE_ALL && m_lastField != SBF_VALUE)
        return;
    if (!pBase)
        return;

    *pBase = (THBASE)base;
    if (m_lastField == SBF_SELECTION ||
        m_lastField == SBF_VALUE)
        SetSelection();
    else if (m_lastField == SBF_FILESIZE)
    {
        appSettings.sb.bExactFileSize = true;
        SetFileSize();
    }
}
