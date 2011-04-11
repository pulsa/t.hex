#pragma once

#ifndef _THFRAME_H_
#define _THFRAME_H_

//#include "manager.h"
#include <wx/aui/framemanager.h>
#include "clipboard.h"
#include "defs.h"

//#define WXFLATNOTEBOOK

#ifdef WXFLATNOTEBOOK
#include <wx/wxFlatNotebook/wxFlatNotebook.h>
#else
#include <wx/aui/auibook.h>
#endif

class HexWnd;
class FileMap;
class DocList;
class DataView;
class FindDialog;
class GotoDlg2;
class NumberView;
class StringView;
class StructureView;
class ProfilerView;
class DisasmView;
class FatView;
class ExportView;
class HistogramView;
class SpawnHandler;
class thStatusBar;
class ipcServer;
class thPipeOutDialog;

class thFrame : public wxFrame
{
public:
    thFrame(wxString commandLine, bool useIPC);
    ~thFrame();

    virtual wxStatusBar* CreateStatusBar();
    void CreateToolBar();
    thStatusBar *m_status;
    wxToolBar *m_toolbar;
    wxToolBar* GetToolBar() { return m_toolbar; }
    wxMenu *m_mnuToolWnds;
    void UpdateUI(); // check/enable menu items
    GotoDlg2 *m_dlgGoto;
    thPipeOutDialog *m_dlgPipeOut;

    void SetHexWnd(HexWnd *hw);
    void AddHexWnd(HexWnd *hw);
    HexWnd *GetHexWnd() { return m_hw; }

#ifdef WXFLATNOTEBOOK
    wxFlatNotebook *tabs;
#else
    wxAuiNotebook *tabs;
#endif
    std::vector<HexWnd*> pendingWindows;
    bool bWindowShown;

    FileMap *map;
    DocList *docList;
    NumberView *viewNumber;
    StringView *viewString;
    StructureView *viewStruct;
    ProfilerView *profView;
    HistogramView *histogramView;
#ifdef INCLUDE_LIBDISASM
    DisasmView *disasmView;
#endif // INCLUDE_LIBDISASM
    FatView *fatView;
    ExportView *exportView;

    FindDialog *findDlg;
    ipcServer *ipcsvr;

    class thDropTarget : public wxFileDropTarget
    {
        thFrame *m_frame;
    public:
        thDropTarget(thFrame *frame) : m_frame(frame) { }
        virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &filenames)
        {
            return m_frame->OnDropFiles(x, y, filenames);
        }
    };
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &filenames);

    void OpenFile(wxString filename, bool bReadOnly);
    void PipeToProcess(wxString file, wxString args, wxString dir);
    SpawnHandler *m_pSpawnHandler;

    void OnPipeComplete(); // called by SpawnHandler

    void OnClose(wxCloseEvent &event);
    void OnActivate(wxActivateEvent &event);
    void OnPaneClose(wxAuiManagerEvent &event);
    void OnMenuOpen(wxMenuEvent &event);
    void OnSetFocus(wxFocusEvent &event);
    void OnDoubleClick(wxMouseEvent &event);  // can't get rid of the CS_DBLCLK style, so fake it out here
    void OnHelp(wxHelpEvent &event);

    void CmdExit(wxCommandEvent &event);
    void CmdGotoDlg(wxCommandEvent &event);
    void CmdFindDlg(wxCommandEvent &event);
    void CmdFindAgain(wxCommandEvent &event);  // handler for both forward and backward
    void CmdReplaceDlg(wxCommandEvent &event);
    void CmdSave(wxCommandEvent &event);
    void CmdSaveAs(wxCommandEvent &event);
    void CmdSaveAll(wxCommandEvent &event);
    void CmdOpenFile(wxCommandEvent &event);
    void CmdOpenDrive(wxCommandEvent &event);
    void CmdOpenProcess(wxCommandEvent &event);
    void CmdOpenLC1(wxCommandEvent &event);
    void CmdNewFile(wxCommandEvent &event);
    void CmdCopyAsDlg(wxCommandEvent &event);
    void CmdPasteDlg(wxCommandEvent &event);
    void CmdSelectDlg(wxCommandEvent &event);
    void CmdFontDlg(wxCommandEvent &event);
    void CmdFullScreen(wxCommandEvent &event);
    void CmdToggleInsert(wxCommandEvent &event);
    void CmdFileMap(wxCommandEvent &event);
    void OnFileProperties(wxCommandEvent &event);
    void CmdSettings(wxCommandEvent &event);
    void OnMenuOther(wxCommandEvent &event);
    void CmdCycleClipboard(wxCommandEvent &event);
    void CmdHistogram(wxCommandEvent &event);
    void CmdCollectStrings(wxCommandEvent &event);
    void CmdViewStatusBar(wxCommandEvent &event);
    void CmdViewToolBar(wxCommandEvent &event);
    void CmdViewDocList(wxCommandEvent &event);
    void CmdNumberView(wxCommandEvent &event);
    void CmdStringView(wxCommandEvent &event);
    void CmdStructureView(wxCommandEvent &event);
    void CmdDisasmView(wxCommandEvent &event);
    void CmdFatView(wxCommandEvent &event);
    void CmdExportView(wxCommandEvent &event);
    void CmdOpenSpecial(wxCommandEvent &event);
    void CmdWriteSpecial(wxCommandEvent &event);
    void CmdCloseTab(wxCommandEvent &event);
    void CmdToggleReadOnly(wxCommandEvent &event);
    void CmdAbout(wxCommandEvent &event);
    void CmdOpenTestFile(wxCommandEvent &event);
    void CmdOpenProcessFile(wxCommandEvent &event);
    void CmdNextBinChar(wxCommandEvent &event);
    void CmdPrevBinChar(wxCommandEvent &event);
    void CmdZipRecover(wxCommandEvent &event);
    void CmdCompressability(wxCommandEvent &event);
    void CmdSwapOrder2(wxCommandEvent &event);
    void CmdSwapOrder4(wxCommandEvent &event);
    void CmdUnZlib(wxCommandEvent &event);

#ifdef WXFLATNOTEBOOK
    void OnPageChanged(wxFlatNotebookEvent& event);
    void OnPageClosed(wxFlatNotebookEvent& event);
    void OnPageContextMenu(wxFlatNotebookEvent& event);
#else
    void OnPageChanged(wxAuiNotebookEvent& event);
    void OnPageClosed(wxAuiNotebookEvent& event);
    void OnPageContextMenu(wxAuiNotebookEvent& event);
#endif

    void AddView(DataView *view);
    void UpdateViews(HexWnd *hw, int flags);
    void ToggleView(DataView *view, bool &setting);
    std::vector<DataView*> m_views;
    int m_queuedUpdateFlags;

    #ifdef TBDL
    thClipboard clipboard;
    #endif
    #ifdef WIN32
    HWND hwndNextViewer; // next viewer in the clipboard chain
    #endif

    wxString m_cfgName;
    wxString strWP;
    virtual bool Show(bool show = true);

    void ProcessCommandLine(wxString cmdLine, wxString cwd = wxEmptyString);

    //! placeholder methods for ExitSaveDialog
    void SaveAll() {}
    void SaveState() {}

    bool GetDocsFromUser(int n, int *pnDoc);
    void CompareBuffers(); // temporary function to compare the first two documents and log to diffs.txt
    void PreloadFile(HexDoc *doc, wxString title);

#ifndef WIN32
    HWND GetHWND();
    HWND GetHwnd() { return GetHWND(); }
#endif

    wxAuiManager m_mgr;
    DECLARE_EVENT_TABLE()

#ifdef _WINDOWS
    virtual WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
#endif

private:
    HexWnd *m_hw;
};

#endif // _THFRAME_H_
