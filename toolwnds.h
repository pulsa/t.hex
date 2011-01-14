#pragma once

#ifndef _TOOLWNDS_H_
#define _TOOLWNDS_H_

#include "precomp.h"
#include "thex.h"
#include "struct.h"
#include "utils.h"
#include "wx/treelistctrl.h"
#include "wx/aui/framemanager.h"

class HexWnd;
class HexDoc;
class wxTreeListCtrl;
class Segment;
class DataSource;

class DataView
{
public:
    DataView(wxWindow *win);
    virtual ~DataView();

    enum {DV_SCROLL   = 0x01,
        DV_WINDOWSIZE = 0x02,
        DV_DATASIZE   = 0x04,
        DV_DATA       = 0x08,
        DV_SELECTION  = 0x10,
        DV_VIEWSIZE   = 0x20,
        DV_NEWDOC     = 0x40,
        DV_ALL        = 0x7F, // update everything
        DV_CLOSE      = 0x10000,
    };
    //virtual void UpdateView(HexWnd *hw, int flags = -1) = 0;
    virtual void Detach(HexWnd *hw) { }
    virtual wxString GetTitle() = 0;
    virtual int GetSide() { return wxAUI_DOCK_RIGHT; }
    wxWindow *win;

    HexWnd *m_hw;
    THSIZE m_iSelStart, m_iSelSize;
    DWORD m_iChangeIndex;
    bool bSkipUpdate;

    int m_queuedUpdates;
    void QueueUpdates(int flags) { m_queuedUpdates |= flags; }
    virtual void ProcessUpdates() { }  // Override this for delayed processing.
    void OnSetFocus(wxFocusEvent &event);  // not virtual
    void OnKillFocus(wxFocusEvent &event);

    virtual void UpdateView(HexWnd *hw, int flags = -1)
    {
        if (hw != m_hw)
        {
            m_hw = hw;
            m_queuedUpdates = -1;
        }
        m_queuedUpdates |= flags;
    }

    //virtual void OnEsc(wxKeyEvent &event);
};

class FileMap : public wxWindow, public DataView, public DblBufWindow
{
public:
    FileMap(wxWindow *parent, wxSize size = wxDefaultSize);
    virtual ~FileMap();

    enum {IDM_SELSTART = 100, IDM_SELEND,
        IDM_BLOCKSTART, IDM_BLOCKEND,
        IDM_SELECTBLOCK,
        IDM_AUTO, IDM_VERTICAL, IDM_HORIZONTAL,
        IDM_CLOSE};

    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual wxString GetTitle() { return wxT("Map"); }
    virtual int GetSide() { return wxAUI_DOCK_LEFT; }
    
    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnMouseDown(wxMouseEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnMouseUp(wxMouseEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void OnCommand(wxCommandEvent &event);
#if wxCHECK_VERSION(2, 7, 0)
    void OnCaptureLost(wxMouseCaptureLostEvent &event) { }
#endif

    void OnMouse(const wxPoint &pt);

    wxMemoryDC memDC;
    int orient; // wxVERTICAL or wxHORIZONTAL
    int forceOrient; // wxVERTICAL, wxHORIZONTAL, or 0 (default)
    int m_majorSize;
    wxMenu *popup;
    wxPoint m_mousePos;
    wxRect m_rcInside;

    bool HasCapture() { return ::GetCapture() == GetHwnd(); }

    wxBrush brFile, brMem, brFill, brBack, brBorder, brForward, brBackward;
    wxPen borderPen;

    int HitTest(const wxPoint pt, THSIZE &pos);
    int GetCoord(THSIZE pos);
    wxBrush *GetBrush(const Segment *s, const DataSource *pDS, THSIZE offset);

    DECLARE_EVENT_TABLE()
};

class DocList : public wxPanel, public DataView
{
public:
    DocList(wxWindow *parent);
    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual void ProcessUpdates();  // resize list columns on idle time
    virtual void OnInternalIdle()
    {
        ProcessUpdates();
        m_queuedUpdates = 0;
        //wxTextCtrl::OnInternalIdle();
    }

    wxListCtrl *list;
    void OnSelChange(wxListEvent &event);
    void OnActivate(wxListEvent &event);
    void OnColumnClick(wxListEvent &event);
    void OnColumnRightClick(wxListEvent &event);
    void OnItemRightClick(wxListEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void CmdActiveDoc(wxCommandEvent &event);

    void SelectDoc(HexDoc *doc);

    virtual wxString GetTitle() { return _T("Ranges"); }
    virtual int GetSide() { return wxAUI_DOCK_BOTTOM; }

    DECLARE_EVENT_TABLE()

    enum {selstate = wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED};
    enum {IDM_ActiveDoc = 100};

public:
    enum {HEADER, ITEM, /*KEYBOARD,*/ NONE} m_menuSource;
    int m_menuSourceItem;
    bool m_bListFrozen;

    int sortColumn;
    int sortOrder[4]; // 1 = ascending, -1 = descending
};

class StringView : public wxTextCtrl, public DataView
{
public:
    StringView(wxWindow *parent);
    virtual ~StringView();
    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual wxString GetTitle() { return _T("String"); }
    wxTextCtrl *text;

    virtual void ProcessUpdates();
    virtual void OnInternalIdle()
    {
        ProcessUpdates();
        m_queuedUpdates = 0;
        wxTextCtrl::OnInternalIdle();
    }

    DECLARE_EVENT_TABLE()
    void OnContextMenu(wxContextMenuEvent &event);
    void OnFont(wxCommandEvent &event);
    void OnWordWrap(wxCommandEvent &event);
    void OnSelChange(wxCommandEvent &event); //! custom event type I added

    void OnSetFocus(wxFocusEvent &event) { DataView::OnSetFocus(event); }
    void OnKillFocus(wxFocusEvent &event) { DataView::OnKillFocus(event); }

    void OnEncoding(wxCommandEvent &event);
    int m_encoding; // ASCII, Unicode, or automatic.  Use IDM_ViewXXX constants.
    bool bWordWrap;
    //virtual void SetInsertionPoint(long pos);
    bool m_bHighlight;
};

class NumberView : public wxTextCtrl, public DataView
{
public:
    NumberView(wxWindow *parent);
    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual wxString GetTitle() { return _T("Numbers"); }
    wxTextCtrl *text;
    virtual void ProcessUpdates();
    virtual void OnInternalIdle()
    {
        ProcessUpdates();
        m_queuedUpdates = 0;
        wxTextCtrl::OnInternalIdle();
    }
};

class StructureView : public wxPanel, public DataView
{
public:
    StructureView(wxWindow *parent);
    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual wxString GetTitle() { return _T("Structure"); }
    void SetStruct(int nStruct);
    int FindTreeItem(wxTreeItemId item);

    //wxTextCtrl *text;
    //wxListCtrl *list;
    wxTreeListCtrl *list;
    wxButton *btnNext, *btnPrev;
    wxSpinCtrl *spinAddress;
    wxChoice *cmbStruct;

    std::vector<StructInfo> m_structs;
    std::vector<wxTreeItemId> m_treeIDs;
    int m_nStruct, m_nIndex;
    THSIZE m_iTotalSize;

    enum {IDC_NEXT = 100, IDC_PREV, IDC_ADDRESS, IDC_STRUCT, IDC_LIST};

    DECLARE_EVENT_TABLE()
    void OnSetStruct(wxCommandEvent &event);
    void OnSelectMember(wxListEvent &event);
    void OnActivateMember(wxListEvent &event);
    void OnSelectMemberTree(wxTreeEvent &event);
    void OnActivateMemberTree(wxTreeEvent &event);
    void OnNextStruct(wxCommandEvent &event);
    void OnPrevStruct(wxCommandEvent &event);

    void OnSetFocus(wxFocusEvent &event) { DataView::OnSetFocus(event); }
    void OnKillFocus(wxFocusEvent &event) { DataView::OnKillFocus(event); }

    virtual void ProcessUpdates();
    virtual void OnInternalIdle()
    {
        ProcessUpdates();
        m_queuedUpdates = 0;
        wxPanel::OnInternalIdle();
    }
};

#ifdef INCLUDE_LIBDISASM
class DisasmView : public DataView, public wxTextCtrl
{
public:
    DisasmView(wxWindow *parent);
    virtual ~DisasmView();

    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual wxString GetTitle() { return wxT("Disassembly"); }

    virtual void ProcessUpdates();
    virtual void OnInternalIdle()
    {
        ProcessUpdates();
        m_queuedUpdates = 0;
        wxTextCtrl::OnInternalIdle();
    }
};
#endif // INCLUDE_LIBDISASM

class OutputView
{
public:
    HexWnd *m_hw;
    HexDoc *m_doc;
};

struct DataRange {
    THSIZE start, size;
};

class ProfilerView : public wxPanel, public DataView, public DblBufWindow
{
public:
    ProfilerView(wxWindow *parent, wxSize size = wxDefaultSize);
    virtual ~ProfilerView();

    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual wxString GetTitle() { return wxT("Profiler"); }
    
    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);
    DECLARE_EVENT_TABLE()

public:
    static HDC   ghDC;
    static HGLRC ghRC;

    static void gl_printText(float x, float y, char *str);
    static float gl_textWidth(char *str);

    static ProfilerView *g_profView;
};

class DocHistoryView : public wxListBox, public DataView
{
public:
    DocHistoryView(wxWindow *parent);
    virtual ~DocHistoryView();

    virtual void UpdateView(HexWnd *hw, int flags = -1);
    virtual wxString GetTitle() { return wxT("Document History"); }

    virtual void ProcessUpdates();
    virtual void OnInternalIdle()
    {
        ProcessUpdates();
        m_queuedUpdates = 0;
        wxListBox::OnInternalIdle();
    }
};

class FatView : public wxTextCtrl, public DataView
{
public:
    FatView(wxWindow *parent);
    virtual ~FatView();

    virtual wxString GetTitle() { return wxT("FAT32"); }

    virtual void ProcessUpdates();
    virtual void OnInternalIdle()
    {
        ProcessUpdates();
        m_queuedUpdates = 0;
        wxTextCtrl::OnInternalIdle();
    }
};

class ExportView : public wxPanel, public DataView
{
public:
    ExportView(wxWindow *parent);
    virtual void ProcessUpdates();
    wxListCtrl *list;
    //void OnSelChange(wxListEvent &event);
    void OnActivate(wxListEvent &event);
    //void OnColumnClick(wxListEvent &event);
    //void OnColumnRightClick(wxListEvent &event);
    //void OnItemRightClick(wxListEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void OnGotoExportsTable(wxCommandEvent &event);
    void OnGotoRVA(wxCommandEvent &event);
    void OnGotoName(wxCommandEvent &event);

    virtual wxString GetTitle() { return _T("Exports"); }
    virtual int GetSide() { return wxAUI_DOCK_RIGHT; }

    DECLARE_EVENT_TABLE()

public:
    enum {HEADER, ITEM, /*KEYBOARD,*/ NONE} m_menuSource;
    enum {IDM_GotoExportsTable = 100, IDM_GotoRVA, IDM_GotoName};
    int m_menuSourceItem;

    int sortColumn;
    int sortOrder[4]; // 1 = ascending, -1 = descending

    DWORD export_rva, export_size, export_offset, names_offset;
};

#endif // _TOOLWNDS_H_
