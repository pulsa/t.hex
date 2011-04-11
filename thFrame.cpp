#include "precomp.h"
#include "hexwnd.h"
#include "resource.h"
#include "thex.h"
#include "thFrame.h"
//#include "wx/msw/private.h"
//#include "manager.h"
#include "toolwnds.h"
#include "dialogs.h"
#include "settings.h"
#ifdef WIN32
#include "links.h"
#endif
#include "utils.h"
#ifdef TBDL
#include "spawn.h"
#endif
#include <wx/cmdline.h>
#include <wx/regex.h>
#include "datasource.h"
#include "statusbar.h"
#include "thIPC.h"
#include <wx/sstream.h>

#define new New

//HexWndSettings s;
thAppSettings appSettings;

static bool testflag = false;  //! for debugging window messages

BEGIN_EVENT_TABLE(thFrame, wxFrame)
    EVT_CLOSE(thFrame::OnClose)
    //EVT_ACTIVATE(thFrame::OnActivate)
    EVT_AUI_PANE_CLOSE(thFrame::OnPaneClose)
    EVT_MENU_OPEN(thFrame::OnMenuOpen)
    EVT_SET_FOCUS(thFrame::OnSetFocus)
    EVT_LEFT_DCLICK(thFrame::OnDoubleClick)
    //EVT_HELP(-1, thFrame::OnHelp)

    EVT_MENU(wxID_EXIT,             thFrame::CmdExit)
    #ifdef TBDL
    EVT_MENU(IDM_FontDlg,           thFrame::CmdFontDlg)
    #endif
    EVT_MENU(IDM_GotoDlg,           thFrame::CmdGotoDlg)
    EVT_MENU(IDM_FullScreen,        thFrame::CmdFullScreen)
    EVT_MENU(IDM_ViewFileMap,       thFrame::CmdFileMap)
    EVT_MENU(IDM_ToggleInsert,      thFrame::CmdToggleInsert)
    EVT_MENU(IDM_OpenFile,          thFrame::CmdOpenFile)
    #ifdef TBDL
    EVT_MENU(IDM_OpenDrive,         thFrame::CmdOpenDrive)
    EVT_MENU(IDM_OpenProcess,       thFrame::CmdOpenProcess)
    #ifdef LC1VECMEM
    EVT_MENU(IDM_OpenLC1,           thFrame::CmdOpenLC1)
    #endif
    #endif // TBDL
    EVT_MENU(IDM_FileNew,           thFrame::CmdNewFile)
    EVT_MENU(IDM_Save,              thFrame::CmdSave)
    EVT_MENU(IDM_SaveAs,            thFrame::CmdSaveAs)
    EVT_MENU(IDM_Settings,          thFrame::CmdSettings)
    EVT_MENU(IDM_Find,              thFrame::CmdFindDlg)
    EVT_MENU(IDM_FindNext,          thFrame::CmdFindAgain)
    EVT_MENU(IDM_FindPrevious,      thFrame::CmdFindAgain)
    EVT_MENU(IDM_CycleClipboard,    thFrame::CmdCycleClipboard)
    EVT_MENU(IDM_Histogram,         thFrame::CmdHistogram)
    EVT_MENU(IDM_CollectStrings,    thFrame::CmdCollectStrings)
    EVT_MENU(IDM_ViewStatusBar,     thFrame::CmdViewStatusBar)
    EVT_MENU(IDM_ViewToolBar,       thFrame::CmdViewToolBar)
    EVT_MENU(IDM_ViewDocList,       thFrame::CmdViewDocList)
    EVT_MENU(IDM_ViewNumberView,    thFrame::CmdNumberView)
    EVT_MENU(IDM_ViewStringView,    thFrame::CmdStringView)
    EVT_MENU(IDM_ViewStructureView, thFrame::CmdStructureView)
    #ifdef TBDL
    EVT_MENU(IDM_ViewFatView,       thFrame::CmdFatView)
    EVT_MENU(IDM_ViewExportView,    thFrame::CmdExportView)
    EVT_MENU(IDM_ViewDisasmView,    thFrame::CmdDisasmView)
    EVT_MENU(IDM_OpenSpecial,       thFrame::CmdOpenSpecial)
    EVT_MENU(IDM_WriteSpecial,      thFrame::CmdWriteSpecial)
    EVT_MENU(IDM_OpenTestFile,      thFrame::CmdOpenTestFile)
    EVT_MENU(IDM_OpenProcessFile,   thFrame::CmdOpenProcessFile)
    #endif // TBDL
    EVT_MENU(IDM_FileProperties,    thFrame::OnFileProperties)
    EVT_MENU(IDM_WindowCloseTab,    thFrame::CmdCloseTab)
    EVT_MENU(IDM_CopyAsDlg,         thFrame::CmdCopyAsDlg)
    EVT_MENU(IDM_ToggleReadOnly,    thFrame::CmdToggleReadOnly)
    EVT_MENU(IDM_About,             thFrame::CmdAbout)
    EVT_MENU(IDM_NextBinChar,       thFrame::CmdNextBinChar)
    EVT_MENU(IDM_PrevBinChar,       thFrame::CmdPrevBinChar)
    EVT_MENU(IDM_ZipRecover,        thFrame::CmdZipRecover)
    EVT_MENU(IDM_Compressability,   thFrame::CmdCompressability)
    EVT_MENU(IDM_SwapOrder2,        thFrame::CmdSwapOrder2)
    EVT_MENU(IDM_SwapOrder4,        thFrame::CmdSwapOrder4)
    EVT_MENU(IDM_UnZlib,            thFrame::CmdUnZlib)

    // Forward other commands to current HexWnd.
    EVT_COMMAND_RANGE(0, IDM_Max, wxEVT_COMMAND_MENU_SELECTED, thFrame::OnMenuOther)

#ifdef WXFLATNOTEBOOK
EVT_FLATNOTEBOOK_PAGE_CHANGED(-1, thFrame::OnPageChanged)
EVT_FLATNOTEBOOK_PAGE_CLOSED(-1, thFrame::OnPageClosed)
EVT_FLATNOTEBOOK_CONTEXT_MENU(-1, thFrame::OnPageContextMenu)
#else
    EVT_AUINOTEBOOK_PAGE_CLOSED(-1, thFrame::OnPageClosed)
    EVT_AUINOTEBOOK_PAGE_CHANGED(-1, thFrame::OnPageChanged)
    EVT_AUINOTEBOOK_BUTTON(-1, thFrame::OnPageContextMenu)
#endif

END_EVENT_TABLE()

thFrame::thFrame(wxString commandLine, bool useIPC)
: wxFrame(NULL, -1, wxGetEmptyString(),
          wxDefaultPosition, wxSize(600,500),
          wxDEFAULT_FRAME_STYLE)/*,
  clipboard(GetHwnd())*/
{
    m_hw = NULL;
    m_status = NULL;
    m_toolbar = NULL;
    m_pSpawnHandler = NULL;
    bWindowShown = false;
    tabs = NULL;
    ipcsvr = NULL;
    findDlg = NULL;
    m_pSpawnHandler = NULL;
    m_dlgGoto = NULL;
    ipcsvr = NULL;
    m_dlgPipeOut = NULL;

    SetDropTarget(new thDropTarget(this));

    map = NULL;
    viewNumber = NULL;
    viewString = NULL;
    viewStruct = NULL;
    docList = NULL;
    profView = NULL;
#ifdef INCLUDE_LIBDISASM
    disasmView = NULL;
#endif
    fatView = NULL;
    exportView = NULL;

#ifdef WIN32
    CoInitialize(NULL);
#endif

#ifndef WIN32
const char *IDI_THEX_xpm = "res/thex.xpm";  //! probably wrong.
#endif
    SetIcon(wxICON(IDI_THEX));

    // Get name of configuration file.
    wxFileName fn(wxGetApp().argv[0]);
    fn.SetName(_T("T_Hex"));
    fn.SetExt(_T("ini"));
    m_cfgName = fn.GetFullPath();

    // Read configuration file.
    wxFileConfig cfg(_T("T Hex"), _T("Adam Bailey"), m_cfgName);
    cfg.Read(_T("WindowPlacement"), &strWP);
    ghw_settings.Load(cfg);
    appSettings.Load(cfg);

    if (appSettings.bStatusBar)
        CreateStatusBar();

    if (appSettings.bToolBar)
        CreateToolBar();

    //wxAcceleratorEntry entries[1];
    //entries[0].Set(wxACCEL_NORMAL,  WXK_F11,    IDM_FullScreen); //! dummy entry
    //wxAcceleratorTable accel(1, entries);
    //accel.SetHACCEL(LoadAccelerators(GetModuleHandle(NULL), "THexAccel"));
    //SetAcceleratorTable(accel);

    wxMenuBar *menuBar = new wxMenuBar();
    wxMenu *menu = new wxMenu();
    menu->Append(IDM_FileNew, _T("&New file\tCtrl+N")); //! do we need a shortcut?
    menu->Append(IDM_OpenFile, _T("&Open file\tCtrl+O"));
    menu->Append(IDM_OpenDrive, _T("Open &drive\tCtrl+D"));
    menu->Append(IDM_OpenProcess, _T("Open process\tCtrl+Shift+O"));
    menu->Append(IDM_OpenLC1, _T("Open LC-1 vector &memory\tCtrl+M"));
    menu->Append(IDM_OpenSpecial, _T("Open special\tCtrl+L"));
    menu->Append(IDM_WriteSpecial, _T("Write special\tCtrl+Shift+S"));
    menu->Append(IDM_ToggleReadOnly, _T("Toggle read-only"));
    menu->Append(IDM_OpenTestFile, _T("Open test file\tCtrl+Shift+T"));
    menu->Append(IDM_OpenProcessFile, _T("Open &foreign process file..."));
    menu->AppendSeparator();
    menu->Append(IDM_Save, _T("&Save\tCtrl+S"));
    menu->Append(IDM_SaveAs, _T("Save &as...\tF12"));
    menu->Append(IDM_SaveAll, _T("Save A&ll\tCtrl+Shift+S"));
    menu->AppendSeparator();
    menu->Append(IDM_FileProperties, _T("P&roperties...\tAlt+Enter"));
    menu->AppendSeparator();
    menu->Append(IDM_Print, _T("&Print...\tCtrl+P"));
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, _T("E&xit"), _T("helpstring")); //!
    menuBar->Append(menu, _T("&File"));

    // edit menu
    menu = new wxMenu();
    menu->Append(IDM_Undo, _T("&Undo\tCtrl+Z"));
    menu->Append(IDM_Redo, _T("&Redo\tCtrl+Y"));
    menu->Append(IDM_UndoAll, _T("Undo All\tCtrl+Shift+Z"));
    menu->Append(IDM_RedoAll, _T("Redo All\tCtrl+Shift+Y"));
    menu->AppendSeparator();
    menu->Append(IDM_Cut, _T("Cu&t\tCtrl+X"));
    menu->Append(IDM_Copy, _T("&Copy\tCtrl+C"));
    menu->Append(IDM_CopyAsDlg, _T("Copy &as...\tCtrl+Shift+C"));
    menu->Append(IDM_Paste, _T("&Paste\tCtrl+V"));
    menu->Append(IDM_PasteDlg, _T("Paste as...\tCtrl+Shift+V"));
    menu->Append(IDM_Delete, _T("&Delete\tDEL"));
    //menu->Append(IDM_CycleClipboard, _T("Cycle Clipboard Ring\tCtrl+Shift+Insert"));
    menu->AppendSeparator();
    menu->Append(IDM_SelectAll, _T("&Select all\tCtrl+A"));
    menu->AppendSeparator();
    menu->Append(IDM_Find, _T("&Find...\tCtrl+F")); //! should this be a submenu, with "again" options?
    menu->Append(IDM_FindNext, _T("Find Next\tF3"));
    menu->Append(IDM_FindPrevious, _T("Find Previous\tShift+F3"));
    menu->Append(IDM_Replace, _T("&Replace...\tCtrl+H"));
    menu->Append(IDM_GotoDlg, _T("&Go to address...\tCtrl+G"));
    menu->AppendSeparator();
    menu->Append(IDM_NextBinChar, _T("Next binary character\tCtrl+B"));
    menu->Append(IDM_PrevBinChar, _T("Previous binary character\tCtrl+Shift+B"));
    menu->AppendSeparator();
    menu->Append(IDM_FindDiffRec, _T("Find next diff\tCtrl+E")); //! hmm.  Whole thing needs thought.
    menu->Append(IDM_FindDiffRecBack, _T("Find prev diff\tCtrl+Shift+E")); //! hmm.  Whole thing needs thought.
    menu->AppendSeparator();
    menu->Append(IDM_InsertRange, _T("&Insert range..."));
    menu->Append(IDM_ToggleInsert, _T("Toggle insert/overwrite mode\tINSERT"));
    menu->AppendSeparator();
    menu->Append(IDM_Settings, _T("Settings...\tAlt+S"));
    menuBar->Append(menu, _T("&Edit"));

    // view->tool_windows submenu
    wxMenu *sub = m_mnuToolWnds = new wxMenu();
    sub->AppendCheckItem(IDM_ViewFileMap, _T("File &Map"));
    sub->AppendCheckItem(IDM_ViewDocList, _T("&Region List"));
    sub->AppendCheckItem(IDM_ViewNumberView, _T("&Number View"));
    sub->AppendCheckItem(IDM_ViewStringView, _T("&String View"));
    sub->AppendCheckItem(IDM_ViewStructureView, _T("S&tructure View"));
    sub->AppendCheckItem(IDM_ViewDisasmView, _T("&Disassembly View"));
    sub->AppendCheckItem(IDM_ViewFatView, _T("&FAT32 View"));
    sub->AppendCheckItem(IDM_ViewExportView, _T("&Export View"));

    // view menu
    menu = new wxMenu();
    menu->Append(IDM_ViewLineUp, _T("Scroll Up\tCtrl+Up"));
    menu->Append(IDM_ViewLineDown, _T("Scroll Down\tCtrl+Down"));
    menu->Append(IDM_ViewPageUp, _T("Scroll Page Up\tCtrl+PgUp"));
    menu->Append(IDM_ViewPageDown, _T("Scroll Page Down\tCtrl+PgDn"));
    menu->AppendSeparator();
    menu->Append(IDM_ViewPrevRegion, _T("Previous region\tAlt+PgUp"));
    menu->Append(IDM_ViewNextRegion, _T("Next region\tAlt+PgDn"));
    menu->AppendSeparator();
    menu->Append(IDM_FontDlg, _T("&Font...\tAlt+Shift+F"));
    menu->Append(IDM_ViewFontSizeUp, _T("&Increase Font Size\tCtrl+KP_ADD"));
    menu->Append(IDM_ViewFontSizeDown, _T("&Decrease Font Size\tCtrl+KP_SUBTRACT"));
    menu->AppendSeparator();
    menu->AppendCheckItem(IDM_ViewRuler, _T("&Ruler"));
    menu->AppendCheckItem(IDM_ViewStickyAddr, _T("Sticky &Address Bar"));
    menu->AppendCheckItem(IDM_ViewStatusBar, _T("Status &Bar"));
    menu->AppendCheckItem(IDM_ViewToolBar, _T("&Tool Bar"));
    menu->AppendCheckItem(IDM_FullScreen, _T("F&ull Screen\tF11"));
    menu->Append(-1, _T("Tool &Windows"), sub);
    menu->AppendSeparator();
    menu->AppendCheckItem(IDM_ViewAdjustColumns, _T("Adjust columns to fit window"));
    menuBar->Append(menu, _T("&View"));

    // tools->operations submenu
    sub = new wxMenu();
    sub->Append(IDM_OpsCustom1, _T("&Add32"));
    sub->Append(IDM_OpsRepeatLast, _T("Repeat Last"));
    sub->Append(IDM_ZipRecover, _T("&ZIP recovery"));
    sub->Append(IDM_Compressability, _T("&Compressability"));
    sub->Append(IDM_SwapOrder2, _T("Swap order (&2 bytes)"));
    sub->Append(IDM_SwapOrder4, _T("Swap order (&4 bytes)"));
    sub->AppendSeparator();
    sub->Append(-1, _T("Nothing here"));

    // tools menu
    menu = new wxMenu();
    menu->Append(IDM_Histogram, _T("&Histogram"));
    menu->Append(IDM_CollectStrings, _T("&Collect Strings"));
    menu->Append(IDM_ReadPalette, _T("Read &palette"));
    menu->Append(IDM_CopyCodePoints, _T("Copy &font code points"));
    menu->Append(IDM_CopySector, _T("Copy &sector"));
    menu->AppendSeparator();
    menu->Append(-1, _T("&Operations"), sub);
    menuBar->Append(menu, _T("&Tools"));

    // FAT menu
    menu = new wxMenu();
    menu->Append(IDM_GotoPath, _T("Go to &path\tF4"));
    menu->Append(IDM_GotoCluster, _T("&Go to cluster\tAlt+G"));
    menu->Append(IDM_JumpToFromFat, _T("Go to corresponding FAT cluster\tAlt+J"));
    menu->Append(IDM_FirstCluster, _T("&First cluster of file\tAlt+Home"));
    menu->Append(IDM_LastCluster, _T("&Last cluster of file\tAlt+End"));
    menu->Append(IDM_FatAutoSave, _T("FAT Auto-save\tCtrl+F1"));  // formerly F1 for Marty
    menuBar->Append(menu, _T("F&AT"));

    // window menu
    menu = new wxMenu();
    menu->Append(IDM_WindowCloseTab, _T("&Close tab\tCtrl+F4"));
    menuBar->Append(menu, _T("&Window"));

    // Help menu
    menu = new wxMenu();
    menu->Append(IDM_About, _T("&About\tF1"));
    menuBar->Append(menu, _T("&Help"));

    SetMenuBar(menuBar); // beware -- generates size event
    UpdateUI();

    // notify wxAUI which frame to use
    m_mgr.SetManagedWindow(this);

#ifdef WXFLATNOTEBOOK
    tabs = new wxFlatNotebook(this, -1, wxDefaultPosition, wxDefaultSize,
        wxFNB_DROPDOWN_TABS_LIST | wxFNB_NO_NAV_BUTTONS // these two are interdependent
        //| wxFNB_VC8
        | wxFNB_FANCY_TABS
        //| wxFNB_VC71
        //| wxFNB_SMART_TABS
        );
    tabs->SetPadding(wxSize(3, 0)); //! only width is used
    //tabs->SetActiveTabColour(wxColour(0, 150, 0));
    //tabs->SetTabAreaColour(wxColour(80, 0, 0));
    //tabs->SetGradientColorBorder(wxColour(0, 0, 200));
    //tabs->SetGradientColorFrom(wxColour(0, 200, 200));

    //tabs->SetGradientColorFrom(wxColour(64, 64, 64));
    tabs->SetGradientColorFrom(wxColour(128, 128, 128));
    tabs->SetActiveTabTextColour(*wxWHITE);
#else
    tabs = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize,
        wxAUI_NB_WINDOWLIST_BUTTON
        | wxAUI_NB_CLOSE_BUTTON
        | wxNO_BORDER // wxAuiNotebook sizing is broken.  Needs this.
        );
    //tabs->SetLabel(_T("wxAuiNotebook"));
#endif

    tabs->Hide();

    ProcessCommandLine(commandLine);

    //if (m_hw == NULL) // nothing to open from the command line?
    if (pendingWindows.size() == 0)
    {
        HexWnd *hw = new HexWnd(this);
        hw->OpenBlank();
        AddHexWnd(hw);
    }

    //if (!m_hw->Ok())
    //{
    //    wxMessageBox(_T("Couldn't create hex window.\n") + m_hw->error(), _T("Error"), wxOK);
    //}
    //!m_hw->SetFocus();

    if (pendingWindows.size() > 1)
    {
        for (size_t i = 0; i < pendingWindows.size(); i++)
        {
            tabs->AddPage(pendingWindows[i], pendingWindows[i]->GetTitle());
        }
        tabs->SetSelection(0);
        SetHexWnd(pendingWindows[0]);
        m_mgr.AddPane(tabs, wxAuiPaneInfo().Name(_T("HexWnd")).CenterPane().PaneBorder(false));
        m_hw->SetFocus();  // needed for multiple docs on the command line
    }
    else
    {
        tabs->Hide();
        SetHexWnd(pendingWindows[0]);
        m_mgr.AddPane(pendingWindows[0], wxAuiPaneInfo().Name(_T("HexWnd")).CenterPane().PaneBorder(false));
    }
    bWindowShown = true;

    if (appSettings.bFileMap)
    {
        map = new FileMap(this);
        AddView(map);
    }

    if (appSettings.bNumberView)
    {
        viewNumber = new NumberView(this);
        AddView(viewNumber);
    }

    if (appSettings.bStringView)
    {
        viewString = new StringView(this);
        AddView(viewString);
    }

    if (appSettings.bStructureView)
    {
        viewStruct = new StructureView(this);
        AddView(viewStruct);
    }

    if (appSettings.bDocList)
    {
        docList = new DocList(this);
        AddView(docList);
    }

#ifdef PROFILE
    {
        profView = new ProfilerView(this);
        AddView(profView);
        m_mgr.GetPane(profView).BestSize(300, -1);
    }
#endif // PROFILE

#ifdef INCLUDE_LIBDISASM
    if (appSettings.bDisasmView)
    {
        disasmView = new DisasmView(this);
        AddView(disasmView);
    }
#endif // INCLUDE_LIBDISASM

    if (1)
    {
        DocHistoryView *undoView = new DocHistoryView(this);
        AddView(undoView);
    }

#ifdef TBDL
    if (appSettings.bFatView)
    {
        fatView = new FatView(this);
        AddView(fatView);
    }

    if (appSettings.bExportView)
    {
        exportView = new ExportView(this);
        AddView(exportView);
    }
#endif // TBDL

    //HACCEL hAccel = LoadMyAccelerators();
    //HACCEL hAccel = LoadAccelerators(hInstance, _T("AhedAccel"));

    // add the panes to the manager
    //m_mgr.AddPane(map, wxAuiPaneInfo().CaptionVisible(false).Gripper().Left());
    //m_mgr.AddPane(hw, wxAuiPaneInfo().Name(_T("HexWnd")).CenterPane());
    //m_mgr.AddPane(tabs, wxAuiPaneInfo().Name(_T("HexWnd")).CenterPane());
    //m_mgr.AddPane(tabs, wxAuiPaneInfo().Name(_T("HexWnd")).DefaultPane());
    //m_mgr.AddPane(tabs, wxAuiPaneInfo().Name(_T("HexWnd")).CenterPane().PaneBorder(false));

    //GotoDlgPanel *gotoDlg = new GotoDlgPanel(this);
    //m_mgr.AddPane(gotoDlg, wxBOTTOM, wxT(_T("Go to Address")));

    //m_mgr.GetPane(map).CaptionVisible(false);

    wxString current, cfgString;
    current = m_mgr.SavePerspective();
    cfg.Read(_T("Perspective"), &cfgString);
    if (!m_mgr.LoadPerspective(cfgString, false))
        m_mgr.LoadPerspective(current, false); //! try to recover

    //wxGetApp().DoEvents(); //! trying to get the right client area of main window
    // tell the manager to "commit" all the changes just made
    m_mgr.Update();

    if (useIPC) {
        ipcsvr = new ipcServer(this);
        ipcsvr->Create(_T("T_Hex.ipc"));
    }

    // Add the window to the clipboard viewer chain.
    //hwndNextViewer = SetClipboardViewer(GetHwnd());
//! todo: Monitor clipboard for changes.  If someone else sets clipboard data,
//  erase SerialData and Release() all data sources used.
}

thFrame::~thFrame()
{
    //ChangeClipboardChain(GetHwnd(), hwndNextViewer);

    delete ipcsvr;
    delete tabs;

    // deinitialize the frame manager
    m_mgr.UnInit();

    //! todo: delete DataViews?

    delete findDlg;
    delete m_dlgGoto;
    #ifdef TBDL
    delete m_pSpawnHandler;
    delete m_dlgPipeOut;
    #endif

#ifdef WIN32
    CoUninitialize();
#endif
}

void thFrame::ProcessCommandLine(wxString cmdLine, wxString cwd /*= wxEmptyString*/)
{
    HexWnd *hw = NULL;

    if (cwd != wxEmptyString)
        ::wxSetWorkingDirectory(cwd);

    // First try to open entire argument list as a single file.
    int i = 0;
    if (cmdLine[0] == '"') // if first character is '"', look for its partner
    {
        for (i = 1; cmdLine[i]; i++)
        {
            if (cmdLine[i] == '"')
            {
                i++;
                break;
            }
        }
    }
    else
        for (i = 0; cmdLine[i] && cmdLine[i] != ' '; i++)
            ;
    while (cmdLine[i] == ' ')
        i++;
    if (cmdLine[i] && cmdLine[i] != '"' && (wxFile::Exists(cmdLine.Mid(i)) || !cmdLine.Mid(i, 4).Cmp("\\\\.\\")))
    {
        OpenFile(cmdLine.Mid(i), appSettings.bDefaultReadOnly);
        return; // skip normal arg processing loop
    }

    static const wxCmdLineEntryDesc cmdLineDesc[] =
    {  //!WX29 WTF... wxCmdLineParser only takes char* now?
        { wxCMD_LINE_SWITCH, ("h"), ("help"), ("show this help message"),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
        { wxCMD_LINE_OPTION, ("p"), ("pid"),  ("Process ID"), wxCMD_LINE_VAL_NUMBER },
        { wxCMD_LINE_OPTION, ("e"), ("exe"),  ("Process name") },
        { wxCMD_LINE_PARAM,  NULL,    NULL,       ("File"), wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
        { wxCMD_LINE_NONE }
    };

    wxCmdLineParser parser(cmdLineDesc, cmdLine.Mid(i));
    if (parser.Parse())
       return;

    #ifdef TBDL
    long pid;
    if (parser.Found(("p"), &pid))
    {
        ProcList procList;
        if (pid <= 0)
            pid = GetCurrentProcessId();
        hw = new HexWnd(this);
        hw->OpenProcess(pid, procList.NameFromPID(pid), true);
        AddHexWnd(hw);
    }

    wxString procName;
    if (parser.Found(("e"), &procName))
    {
         ProcList procList;
         DWORD pid = procList.PIDFromName(procName);
         if (pid > 0)
         {
             HexWnd *hw = new HexWnd(this);
             hw->OpenProcess(pid, procList.NameFromPID(pid), true);
             AddHexWnd(hw);
         }
         else
             wxMessageBox(_T("Couldn't get PID for '") + procName + _T("'"));
    }
    #endif

    for (i = 0; i < (int)parser.GetParamCount(); i++)
    {
        OpenFile(parser.GetParam(i), appSettings.bDefaultReadOnly);
    }
}

void thFrame::OnClose(wxCloseEvent &event)
{
    if (m_hw && m_hw->doc->IsModified() && event.CanVeto())
    {
        ExitSaveDialog dlg(this);
        if (!dlg.ShowModal())
        {
            event.Veto();
            return;
        }
    }

    wxFileConfig cfg(_T("T Hex"), _T("Adam Bailey"), m_cfgName);

#ifdef _WINDOWS
    WINDOWPLACEMENT wp;
    if (!appSettings.bFullScreen)
        GetWindowPlacement(GetHwnd(), &wp);
    RECT &rc = wp.rcNormalPosition;
    cfg.Write(_T("WindowPlacement"), wxString::Format(_T("%d %d %d %d %d"),
        rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top,
        wp.showCmd == SW_MAXIMIZE));
#endif  // _WINDOWS

    //s.Save(cfg);
    //for (int tab = 0; tab < tabs->GetPageCount(); tab++)
    //{
    //    ((HexWnd*)tabs->GetPage(tab))->s.Save(cfg);
    //}
    //if (m_hw)
    //    m_hw->s.Save(cfg); //! should maybe save in ~HexWnd()

    // clean up dialogs here that use thRecentChoice
    #ifdef TBDL
    delete m_dlgPipeOut;
    m_dlgPipeOut = NULL;
    #endif

    ghw_settings.Save(cfg);
    appSettings.Save(cfg);

    //cfg.Write(_T("Layout/FileMap"), m_mgr.SavePaneInfo(m_mgr.GetPane(map)));
    //cfg.Write(_T("Layout/Profiler"), m_mgr.SavePaneInfo(m_mgr.GetPane(profView)));
    //cfg.Write(_T("Layout/DocList"), m_mgr.SavePaneInfo(m_mgr.GetPane(docList)));
    cfg.Write(_T("Perspective"), m_mgr.SavePerspective());

    cfg.Flush();

    //UpdateViews(m_hw, DataView::DV_CLOSE);
    m_views.clear();
    m_status = NULL;

    SetHexWnd(NULL);

    Destroy();
}

void thFrame::CmdExit(wxCommandEvent &event)
{
    wxCloseEvent dummy;
    OnClose(dummy);
}

void thFrame::CmdGotoDlg(wxCommandEvent &event)
{
    if (!m_hw) {
        wxBell();
        return;
    }

    //if (!m_dlgGoto)
    //    m_dlgGoto = new GotoDlg2(this, m_hw);

    //m_dlgGoto->ShowModal(m_hw);

    GotoDlg2 dlg(this, m_hw);
    dlg.ShowModal();
}

void thFrame::CmdFindDlg(wxCommandEvent &event)
{
    if (!m_hw) {
        wxBell();
        return;
    }
    //if (!findDlg)
    //{
    //    ::SetFocus(0);  // needed so focus isn't always restored to the same page when the dialog is hidden
    //    findDlg = new FindDialog(this);
    //}

    //findDlg->SetHexWnd(m_hw);
    //findDlg->ShowModal();
    Selection sel = m_hw->GetSelection();
    FindDialog dlg(this, m_hw->doc->ReadString(sel.GetFirst(), sel.GetSize()));
    dlg.SetHexWnd(m_hw);
    dlg.ShowModal();
}

void thFrame::CmdFindAgain(wxCommandEvent &event)
{
    if (!m_hw) {
        wxBell();
        return;
    }

    if (appSettings.find.length == 0)
    {
        CmdFindDlg(event);
        return;
    }

    int flags = appSettings.GetFindFlags() | THF_AGAIN;
    if (event.GetId() == IDM_FindPrevious)
        flags |= THF_BACKWARD;

    ATimer timer;
    timer.StartWatch();
    m_hw->DoFind(appSettings.find.data, appSettings.find.length, flags);
    timer.StopWatch();
    PRINTF(_T("Find took %0.3f ms\n"), timer.GetSeconds() * 1000);
}

void thFrame::CmdReplaceDlg(wxCommandEvent &event)
{
}

void thFrame::CmdSave(wxCommandEvent &event)
{
    if (m_hw && m_hw->doc)
    {
        if (!m_hw->doc->Save())
             wxMessageBox(_T("Problem."));
        m_hw->Refresh();
        //! To do: clear all file modified indicators.
    }
}

void thFrame::CmdSaveAs(wxCommandEvent &event)
{
    wxFileDialog saveDlg(this, _T("Save File"), ZSTR, ZSTR, _T("*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
        );

    if (saveDlg.ShowModal() == wxID_CANCEL)
        return;

    m_hw->doc->SaveRange(saveDlg.GetPath(), 0, m_hw->DocSize());
}

void thFrame::CmdOpenFile(wxCommandEvent &event)
{
    wxFileDialog fileDlg(this, _T("Open File"), ZSTR, ZSTR, _T("*.*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST
#if WX_AEB_MOD // changes to wxMSW-2.8.5  AEB  2007-10-17
        | (appSettings.bDefaultReadOnly ? wxFD_READONLY : 0)
        | wxFD_NO_FOLLOW_LINKS
#endif
#if wxCHECK_VERSION(2, 7, 0)
        | wxFD_MULTIPLE
#endif
        );

begin:
    if (fileDlg.ShowModal() == wxID_CANCEL)
        return;

#if WX_AEB_MOD
    appSettings.bDefaultReadOnly = fileDlg.IsReadOnly();
#else
    appSettings.bDefaultReadOnly = false;
#endif

    wxArrayString filenames;
    fileDlg.GetPaths(filenames);
    for (size_t i = 0; i < filenames.GetCount(); i++)
    {
        wxString filename = filenames[i];

        #ifdef WIN32
        CLnkFile lnk(filename);
        if (lnk.isValidLink)
        {
            if (wxFile::Exists(lnk.strTarget))
            {
                if (wxMessageBox(_T("Open target file '") + lnk.strTarget + _T("'?"), APPNAME, wxYES_NO) == wxYES)
                    filename = lnk.strTarget;
            }
            else if (wxDir::Exists(lnk.strTarget))
            {
                //! todo: show file open dialog in this directory
                if (wxMessageBox(_T("Link target is a directory.  Browse there?"), APPNAME, wxYES_NO) == wxYES)
                {
                    fileDlg.SetDirectory(lnk.strTarget);
                    goto begin;
                }
            }
            else // target is not a file or directory that we can get to
            {
                if (wxMessageBox(_T("Resolve link?"), APPNAME, wxYES_NO) == wxYES)
                {
                    DWORD flags = SLR_ANY_MATCH;
                    //DWORD flags = SLR_NOUPDATE | SLR_NOSEARCH | SLR_NOTRACK | SLR_NOLINKINFO;
                    HRESULT hres = lnk.Resolve(GetHwnd(), flags);
                    if (SUCCEEDED(hres))
                    {
                        //! see if new target is file or directory and act accordingly
                        if (wxMessageBox(_T("Open target file '") + lnk.strTarget + _T("'?"), APPNAME, wxYES_NO) == wxYES)
                            filename = lnk.strTarget;
                    }
                    else
                        wxMessageBox(lnk.strError);
                }
            }
        }
        #endif // WIN32

        //hw->SetDataSource(new FileDataSource(filename, false));
        HexWnd *hw = new HexWnd(this);
        //hw->OpenFile(filename, appSettings.bDefaultReadOnly);
#ifdef WX_AEB_MOD
        hw->OpenFile(filename, fileDlg.IsReadOnly());
#else
        hw->OpenFile(filename, false);
#endif
        AddHexWnd(hw);
    }
}

#ifdef TBDL
void thFrame::CmdOpenDrive(wxCommandEvent &event)
{
    //wxArrayString volumes;
    //char name[MAX_PATH];
    //HANDLE hf = FindFirstVolume(name, MAX_PATH);
    //if (hf != INVALID_HANDLE_VALUE)
    //{
    //    volumes.Add(name);
    //    puts(name);
    //    while (FindNextVolume(hf, name, MAX_PATH))
    //    {
    //        volumes.Add(name);
    //        puts(name);
    //    }
    //    FindVolumeClose(hf);
    //}

    //int choice = wxGetSingleChoiceIndex(_T("message"), _T("caption"), volumes, this);
    //if (choice < 0)
    //    return;

    //wxArrayInt driveNumbers;
    //wxArrayString driveNames;
    //for (int iDrive = 0; iDrive < 26; iDrive++)
    //{
    //    const char *type = NULL;
    //    switch (RealDriveType(iDrive, 0))
    //    {
    //    case DRIVE_REMOVABLE: type = _T("Removable disk"); break;
    //    case DRIVE_FIXED: type = _T("Fixed disk"); break;
    //    //case DRIVE_REMOTE: type = _T("Network drive"); break; // can't dump network drives
    //    case DRIVE_CDROM: type = _T("Optical drive"); break;
    //    case DRIVE_RAMDISK: type = _T("RAM disk"); break; //! does this work?
    //    }
    //    if (type)
    //    {
    //        driveNumbers.Add(iDrive);
    //        char drive[4] = {'A' + iDrive, ':', '\\'};

    //        char label[256];
    //        char fsys[256];
    //        if (!GetVolumeInformation(drive, label, 256, NULL, NULL, NULL, fsys, 256))
    //        {
    //            DWORD err = GetLastError();
    //            label[0] = fsys[0] = 0;
    //            if (err == ERROR_NOT_READY)
    //                strcpy(label, _T("Device not ready"));
    //            else
    //                sprintf(label, _T("Error %d"), err);
    //        }
    //        driveNames.Add(wxString::Format(_T("%s %s %s %s"), drive, type, label, fsys));
    //    }
    //}

    //int choice = ::wxGetSingleChoiceIndex(_T("message"), _T("caption"), driveNames, this);
    //if (choice >= 0)
    //{
    //    HexWnd *hw = new HexWnd(this);
    //    if (hw->OpenDrive(wxString::Format(_T("\\\\.\\%c:"), driveNumbers[choice] + 'A'), true))
    //        AddHexWnd(hw);
    //    else
    //        delete hw;
    //}

    OpenDriveDialog dlg(this);
    dlg.CentreOnParent();
    if (dlg.ShowModal() == wxID_OK)
    {
        appSettings.bExactDriveSize = dlg.GetExactSize();
    }
    if (dlg.GetTarget().Len())
    {
        HexWnd *hw = new HexWnd(this);
        hw->OpenDrive(dlg.GetTarget(), dlg.GetReadOnly());
        AddHexWnd(hw);
    }
}

void thFrame::CmdOpenProcess(wxCommandEvent &event)
{
    ProcessDialog dlg(this, _T("Open Process"));
    if (dlg.ShowModal() == wxID_CANCEL)
        return;

    wxArrayInt sels = dlg.GetSelections();
    for (size_t n = 0; n < sels.GetCount(); n++)
    {
        int sel = sels[n];
        HexWnd *hw = new HexWnd(this);
        hw->OpenProcess(dlg.GetPID(sel), dlg.GetProcName(sel), dlg.GetReadOnly());
        AddHexWnd(hw);
    }
}
#endif // TBDL

#ifdef LC1VECMEM
void thFrame::CmdOpenLC1(wxCommandEvent &event)
{
    HexWnd *hw = new HexWnd(this);
    //hw->OpenLC1VectorMemory(_T("127.0.0.1"));
    //hw->OpenLC1VectorMemory(_T("192.168.10.1"));
    hw->OpenLC1VectorMemory(_T("10.2.2.1"));
    AddHexWnd(hw);
}
#endif

void thFrame::CmdNewFile(wxCommandEvent &event)
{
    if (appSettings.bReuseWindow)
        m_hw->OpenBlank(); //! check for modified file first
    else
    {
        HexWnd *hw = new HexWnd(this);
        hw->OpenBlank();
        AddHexWnd(hw);
    }
}

void thFrame::CmdSettings(wxCommandEvent &event)
{
    HexWndSettings s = ghw_settings; // make a copy (why was this again?)
    if (m_hw)
        s = m_hw->s;
    SettingsDialog dlg(this, &s);
    dlg.ShowModal();
    //m_hw->s = s; // this should avoid reading 200 bytes into a 16-byte buffer that happened in OnPaint()
    //m_hw->OnDataChange(0, -1, m_hw->DocSize()); //! hack
    //m_hw->SetFont(m_hw->GetFont()); //! WRONG, but it works (except for bytes-per-line changes)
    if (m_hw) {
        m_hw->UpdateSettings(s);

        if (dlg.FontChanged(&ghw_settings)) {
            //LOGFONT lf;
            //GetObject((HFONT)m_hw->GetFont().GetHFONT(), sizeof(lf), &lf);
            //m_hw->SetFont(&lf);  // This uses new font quality setting.
            wxFont font(m_hw->GetFont());
            m_hw->SetFont(font); //! only uses new quality setting.  Needs work.
        }
    }

    UpdateViews(m_hw, DataView::DV_ALL); //! also a bit of a hack
    //! What else could change?

    // Save settings for later.
    ghw_settings = s;
}

void thFrame::CmdCopyAsDlg(wxCommandEvent &event)
{
    CopyFormatDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK)
        //dlg.fmt.DoCopy();
        m_hw->DoCopy(dlg.GetFormat());
}

//void thFrame::CmdPasteDlg(wxCommandEvent &event)
//{
//}

void thFrame::CmdSelectDlg(wxCommandEvent &event)
{
}

#ifdef TBDL
UINT_PTR CALLBACK FontDlgHook(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    const int idStrikeout = 0x410, idFixedWidth = 0x1337;
    //HWND cbfw; // fixed-width check box
    static WINDOWPLACEMENT wp;
    static CHOOSEFONT *cf;
    static WPARAM init_wParam;
    HDC hdc;
    switch (uMsg)
    {
    case WM_INITDIALOG: {
        cf = (CHOOSEFONT*)lParam;
        if (*(int*)cf->lCustData) // restart flag?
        {
            SetWindowPlacement(hDlg, &wp);
            //return 0; // start normally
        }
        init_wParam = wParam;
        *(int*)cf->lCustData = 0; // don't restart
        HWND hw = GetDlgItem(hDlg, idStrikeout);
        // adjust the window size by using the old text, then change the text
        TCHAR text[100];
        GetWindowText(hw, text, 100);
        hdc = GetDC(hw);
        SIZE size1, size2;
        GetTextExtentPoint32(hdc, text, lstrlen(text), &size1);
        lstrcpy(text, _T("Fixed-&width only"));
        GetTextExtentPoint32(hdc, text, lstrlen(text), &size2);
        ReleaseDC(hw, hdc);
        RECT rc;
        GetWindowRect(hw, &rc);
        POINT pos = {rc.left, rc.top};
        POINT size = {rc.right - rc.left, rc.bottom - rc.top};
        ScreenToClient(hDlg, &pos);
        HWND cbfw = CreateWindow(_T("BUTTON"), text, WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
            pos.x, pos.y,
            size.x + size2.cx - size1.cx + 1, size.y,
            hDlg, (HMENU)idFixedWidth, 0, 0);
        SendMessage(cbfw, WM_SETFONT, (WPARAM)SendMessage(hw, WM_GETFONT, 0, 0), 0);
        //SetWindowPos(cbfw, 0, 0, 0, rc.right + size2.cx - size1.cx + 1, rc.bottom, SWP_NOMOVE);
        //SetWindowText(cbfw, text);
        //EnableWindow(cbfw, TRUE);
        SendMessage(cbfw, BM_SETCHECK, (cf->Flags & CF_FIXEDPITCHONLY) ? BST_CHECKED : BST_UNCHECKED, 0);
        //ShowWindow(cbfw, SW_SHOW);
        } break;
    case WM_COMMAND:
        if (LOWORD(wParam) == idFixedWidth && HIWORD(wParam) == BN_CLICKED)
        {
            cf->Flags ^= CF_FIXEDPITCHONLY;
            *(int*)cf->lCustData = 1; // set restart flag
            GetWindowPlacement(hDlg, &wp); // see where to put new window
            PostMessage(hDlg, WM_COMMAND, MAKELONG(IDABORT, 0), 0); // close dialog
            // Hmm... let's try something a little less drastic.
            //PostMessage(hDlg, WM_INITDIALOG, init_wParam, (LPARAM)cf);  // Doesn't work.
            return 1; // no msg for you!
        }
        break;
    }
    return 0; // Return non-zero and the dialog will not process the message.
}

void thFrame::CmdFontDlg(wxCommandEvent &event)
{
    //! wxFontDialog doesn't allow restriction to fixed-pitch fonts.
    LOGFONT lf;
    GetObject((HFONT)m_hw->GetFont().GetHFONT(), sizeof(lf), &lf);
    CHOOSEFONT cf = { sizeof(cf) };
    cf.hwndOwner = GetHwnd();
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | /*CF_FIXEDPITCHONLY |*/ CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;

    cf.Flags |= CF_ENABLEHOOK;
    int restart_flag = 0;
    //cf.lCustData = (LPARAM)this;
    cf.lCustData = (LPARAM)&restart_flag;
    cf.lpfnHook = FontDlgHook;
    int rc;
    while (0 == (rc = ChooseFont(&cf)) && restart_flag)
        ;
    //if (ChooseFont(&cf))
    if (rc)
    {
        if (!wxString(_T("Terminal")).CmpNoCase(lf.lfFaceName))
        { // User selected the Terminal font.  Ask what width to use.
            FontWidthChooserDialog dlg(this, lf);
            dlg.ShowModal();
        }
        m_hw->SetFont(&lf); //! todo: apply to all windows and default for new ones
    }
}

bool MarkFullScreenWindow(HWND hWnd, bool fullscreen)
{
    // get ITaskBarList2 and tell the shell about this window
    bool ok = false;
    ITaskbarList2 *tl2;
    HRESULT hres = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList2, (LPVOID *)&tl2);
    if (SUCCEEDED(hres))
    {
        hres = tl2->HrInit();
        if (SUCCEEDED(hres))
        {
            hres = tl2->MarkFullscreenWindow(hWnd, fullscreen);
            if (SUCCEEDED(hres))
            {
                ok = true;
                PRINTF(_T("MarkFullscreenWindow(%d) succeeded.\n"), fullscreen);
            }
        }
        tl2->Release();
    }
    return ok;
}
#endif // TBDL

void thFrame::CmdFullScreen(wxCommandEvent &event)
{
    //static bool bFullScreen = false;
    appSettings.bFullScreen = !appSettings.bFullScreen;

    // wow.  Now I feel silly.
    ShowFullScreen(appSettings.bFullScreen,
       wxFULLSCREEN_NOCAPTION |
       wxFULLSCREEN_NOBORDER |
       wxFULLSCREEN_NOMENUBAR |
       wxFULLSCREEN_NOSTATUSBAR );
    return;

#if 0
    HWND hWnd = GetHwnd();

    static DWORD orig_style = WS_VISIBLE;

    if (appSettings.bFullScreen)
    {
        GetWindowPlacement(GetHwnd(), &m_wndPlacement);

        //HWND hTray = ::FindWindow(_T("Shell_TrayWnd"), NULL);
        //ShowWindow(hTray, SW_HIDE);

        //! Make the taskbar appear automatically.
        //! This doesn't work without DoEvents(), and makes all other apps resize.
        //! That's pretty ugly.  IE6 apparently does its own mouse tracking and
        //  shows or hides the taskbar as appropriate. (No SetCapture(), though.)
        //APPBARDATA ab = { sizeof(ab) };
        //ab.lParam = ABS_AUTOHIDE;
        //SHAppBarMessage(ABM_SETSTATE, &ab);
        //wxGetApp().DoEvents();

        //! todo: track mouse.  if it gets close to the edge of the screen with the
        // taskbar, show the taskbar.  If the mouse moves away again, hide the taskbar.
        // We only have to do anything if ABS_ALWAYSONTOP bit is set,
        // because if it's not, then the user probably isn't expecting a start menu.
        // But IE always pops it up when you get close.  Maybe that's okay too.
        // Can we do this all with SHAppBarMessage, or do we need an HWND?

        //DWORD style = GetWindowLong(hWnd, GWL_STYLE);
        //SetWindowLong(hWnd, GWL_STYLE, style & ~WS_CAPTION);
        orig_style = (DWORD)GetWindowLong(hWnd, GWL_STYLE);
        SetWindowLong(GetHwnd(), GWL_STYLE, WS_VISIBLE); // remove title bar
        //SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        //SetWindowPos(GetHwnd(), HWND_TOPMOST,
        //    0,
        //    0,
        //    GetSystemMetrics(SM_CXSCREEN),
        //    GetSystemMetrics(SM_CYSCREEN),
        //    SWP_SHOWWINDOW | SWP_FRAMECHANGED);

        //! testing:
        //MarkFullScreenWindow(hWnd, true);

        HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hMonitor, &mi);
        //SetWindowPos(GetHwnd(), HWND_TOPMOST,
        SetWindowPos(GetHwnd(), HWND_TOP,
            mi.rcMonitor.left,
            mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOSENDCHANGING);

        //InvalidateRect(GetHwnd(), NULL, 0);

        // Hide status bar when going full-screen.  Removed 2007-09-05.
        // This conflicts with CmdViewStatusBar and leaves the main window in a weird state.
        //if (m_status)
        //{
        //    //m_status->SetWindowStyle(0);
        //    SetStatusBar(NULL);
        //    m_status->Hide();
        //    Layout();
        //}
    }
    else
    {
        //APPBARDATA ab = { sizeof(ab) };
        //ab.lParam = ABS_ALWAYSONTOP;
        //SHAppBarMessage(ABM_SETSTATE, &ab);

        //HWND hTray = ::FindWindow(_T("Shell_TrayWnd"), NULL);
        //ShowWindow(hTray, SW_SHOW);

        //MarkFullScreenWindow(hWnd, false);

        //SetWindowLong(GetHwnd(), GWL_STYLE, WS_OVERLAPPEDWINDOW); // restore title bar
        //DWORD style = GetWindowLong(hWnd, GWL_STYLE);
        //SetWindowLong(hWnd, GWL_STYLE, style | WS_CAPTION);
        SetWindowLong(hWnd, GWL_STYLE, orig_style);
        SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        SetWindowPlacement(GetHwnd(), &m_wndPlacement);
        //SetWindowPos(GetHwnd(), HWND_NOTOPMOST, 0, 0, 0, 0,
        //    SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

        //InvalidateRect(NULL, NULL, 0);

        // Hide status bar when going full-screen.  Removed 2007-09-05
        //if (m_status)
        //{
        //    //m_status->SetWindowStyle(wxST_SIZEGRIP);
        //    m_status->Show();
        //    SetStatusBar(m_status);
        //    Layout();
        //}
    }
    InvalidateRect(NULL, NULL, 0);
#endif // 0
}

void thFrame::CmdToggleInsert(wxCommandEvent &event)
{
    appSettings.bInsertMode = !appSettings.bInsertMode;
    m_hw->set_caret_pos(); // cursor size changes
    if (m_status)
        m_status->SetEditMode();
}


//void thFrame::SetStatusBarParts()
//{
//    //! stuff to show on status bar:
//    //multi-byte value at cursor
//    //    endianness
//    //    signed/unsigned
//
//    //0 last action - user or system
//    //1 cursor/selection
//    //2 value
//    //3 endianness
//    //4 character set
//    //5 file size (toggle short/long)
//    //6 ins/ovr/read
//
//    wxString str;
//
//    if (hw && hw->doc)
//        SetStatusText(
//    SetStatusText(_T("No file"), SBF_FILESIZE);
//
//    if (!hw)
//        str.Clear();
//    else if (hw->doc->IsReadOnly())
//        str = _T("READ");
//    else if (appSettings.bInsertMode)
//        str = _T("INS");
//    else
//        str = _T("OVR");
//    SetStatusText(str, SBF_EDITMODE);
//
//    int widths[SB_FIELD_COUNT] = { -1, 400, 150, 60, 60, 100, 60 };
//    GetStatusBar()->SetStatusWidths(SB_FIELD_COUNT, widths);
//
//    //! todo: adjust SBF_SELECTION and other fields' width for content
//}

void thFrame::CmdFileMap(wxCommandEvent &event)
{
    if (!map)
        map = new FileMap(this);
    ToggleView(map, appSettings.bFileMap);
}

void thFrame::OnFileProperties(wxCommandEvent &event)
{
    //wxMessageBox(_T("Nothing to show here."), _T("File Properties"));
    if (!m_hw || !m_hw->m_pDS) return;
    m_hw->m_pDS->ShowProperties(m_hw);
}

void thFrame::OnMenuOther(wxCommandEvent &event)
{
    //! This is kind of ugly, but it seems to work so far.
    static bool recursionFlag = false;
    if (recursionFlag)
        return;
    recursionFlag = true;
    if (m_hw)
        m_hw->ProcessWindowEvent(event);
    recursionFlag = false;
}

void thFrame::AddView(DataView *view)
{
    wxAuiPaneInfo pane;
    pane.Caption(view->GetTitle());
    pane.Name(view->GetTitle()); //! should be something different, like view->GetName()
    pane.Direction(view->GetSide());
    m_mgr.AddPane(view->win, pane);

    m_views.push_back(view);
    if (m_hw)
        view->UpdateView(m_hw);
}

void thFrame::UpdateViews(HexWnd *hw, int flags)
{
    std::vector<DataView*>::const_iterator iter;
    for (iter = m_views.begin(); iter < m_views.end(); iter++)
    {
        DataView *v = *iter;
        if (v->win->IsShown())
            v->UpdateView(hw, flags);
    }

    if (m_status)
    {
        m_status->UpdateView(hw, flags);
    }
}

#ifdef _WINDOWS
WXLRESULT thFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    if (testflag && (message == WM_WINDOWPOSCHANGING))
    {
        message = message; //! breakpoint
    }

    if (message == WM_RENDERALLFORMATS)
    {
         //! todo: ask user whether to leave on clipboard?
    }

    else if (message == WM_RENDERFORMAT)
    {
        clipboard.RenderFormat(wParam);
    }
    //else if (message == WM_CHANGECBCHAIN)
    //{
    //    // If the next window is closing, repair the chain.
    //    if ((HWND) wParam == hwndNextViewer)
    //        hwndNextViewer = (HWND) lParam;

    //    // Otherwise, pass the message to the next link.
    //    else if (hwndNextViewer != NULL)
    //    {
    //        SendMessage(hwndNextViewer, message, wParam, lParam);
    //    }
    //    return 0;
    //}

    return wxFrame::MSWWindowProc(message, wParam, lParam);
}
#endif

void thFrame::CmdCycleClipboard(wxCommandEvent &event)
{
    #ifdef TBDL
    clipboard.Cycle();
    m_hw->CmdPaste(event); //! Do something intelligent here
    #endif
}

void thFrame::CmdHistogram(wxCommandEvent &event)
{
    /*HistogramDialog *dlg =*/ new HistogramDialog(m_hw);
    return;

    //THSIZE count[256];
    //for (int i = 0; i < 256; i++)
    //    count[i] = 0;
    //ByteIterator2 iter(0, m_hw->doc);
    //while (!iter.AtEnd())
    //{
    //    ++count[*iter];
    //    ++iter;
    //}

    ////! todo: show results in HistogramDialog

    //double total = m_hw->doc->GetSize() * .01;
    //int digits = 1;
    //for (int i = 0; i < 256; i++)
    //    digits = wxMax(digits, CountDigits(count[i], 10));

    //for (int i = 0; i < 256; i++)
    //    PRINTF("%03d  %02X  %c  %*I64d  %2.2f%%\n", i, i, (i < 0x20 ? ' ' : i), digits, count[i], count[i] / total);
}

void thFrame::CmdCollectStrings(wxCommandEvent &event)
{
    StringCollectDialog *dlg = new StringCollectDialog(this, m_hw);
    dlg->Show();
}

void thFrame::CmdViewStatusBar(wxCommandEvent &event)
{
    appSettings.bStatusBar = !appSettings.bStatusBar;
    wxStatusBar *bar = GetStatusBar();
    if (appSettings.bStatusBar && !bar)
        CreateStatusBar();
    else
    {
        bar->Show(appSettings.bStatusBar);
        SendSizeEvent();
    }
    //Refresh(false); //! maybe this will fix some painting problems... nope.
}

wxStatusBar* thFrame::CreateStatusBar()
{
    m_status = new thStatusBar(this, m_hw);
    return m_status;
}

bool thFrame::Show(bool show /*= true*/)
{
    #ifdef TBDL
    if (show)
    {
        WINDOWPLACEMENT wp;
        GetWindowPlacement(GetHwnd(), &wp);
        RECT &rc = wp.rcNormalPosition;
        int maximize = 0;
        _stscanf(strWP, _T("%d %d %d %d %d"), &rc.left, &rc.top, &rc.right, &rc.bottom, &maximize);
        rc.right += rc.left;
        rc.bottom += rc.top;
        wp.showCmd = maximize ? SW_MAXIMIZE : SW_SHOWNORMAL;
        SetWindowPlacement(GetHwnd(), &wp);
        //SetWindowPos(GetHwnd(), NULL, 0, 0, 0, 0,
        //    SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    }
    #endif // TBDL

    return wxFrame::Show(show);
}

void thFrame::CmdViewToolBar(wxCommandEvent &event)
{
    appSettings.bToolBar = !appSettings.bToolBar;
    wxToolBar *bar = GetToolBar();
    if (appSettings.bToolBar && !bar)
        CreateToolBar();
    else
    {
        wxAuiPaneInfo &pane = m_mgr.GetPane(m_toolbar);
        pane.Show(!pane.IsShown());
    }
    m_mgr.Update();
}

void thFrame::CreateToolBar()
{
    m_toolbar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize,
                                   wxTB_FLAT | wxTB_NODIVIDER | wxTB_TEXT | wxTB_NOICONS);
    m_toolbar->AddTool(IDM_OpenFile, _T("Open File"), wxNullBitmap);
    m_toolbar->AddTool(IDM_OpenProcess, _T("Open Process"), wxNullBitmap);
    m_toolbar->AddSeparator();
    m_toolbar->AddTool(IDM_Save, _T("Save"), wxNullBitmap);
    m_toolbar->Realize();

    m_mgr.AddPane(m_toolbar, wxAuiPaneInfo().
        Name(wxT("Toolbar1")).Caption(wxT("Caption")).ToolbarPane().Top().
        LeftDockable(false).RightDockable(false));
}

void thFrame::CmdViewDocList(wxCommandEvent &event)
{
    if (!docList)
        docList = new DocList(this);
    ToggleView(docList, appSettings.bDocList);
}

void thFrame::ToggleView(DataView *view, bool &setting)
{
    setting = !setting;
    wxAuiPaneInfo &pane = m_mgr.GetPane(view->win);
    if (pane.IsOk()) // If the view has not been added yet, we get empty pane object.
    {
        pane.Show(setting);
    }
    else // add pane for the first time
    {
        //m_mgr.AddPane(view->win, view->GetSide(), view->GetTitle());
        AddView(view);
    }
    m_mgr.Update();
}

void thFrame::CmdNumberView(wxCommandEvent &event)
{
    if (!viewNumber)
        viewNumber = new NumberView(this);
    ToggleView(viewNumber, appSettings.bNumberView);
}

void thFrame::CmdStringView(wxCommandEvent &event)
{
    if (!viewString)
        viewString = new StringView(this);
    ToggleView(viewString, appSettings.bStringView);
}

void thFrame::CmdStructureView(wxCommandEvent &event)
{
    if (!viewStruct)
        viewStruct = new StructureView(this);
    ToggleView(viewStruct, appSettings.bStructureView);
}

#ifdef INCLUDE_LIBDISASM
void thFrame::CmdDisasmView(wxCommandEvent &event)
{
    if (!disasmView)
        disasmView = new DisasmView(this);
    ToggleView(disasmView, appSettings.bDisasmView);
}
#endif

#ifdef TBDL
void thFrame::CmdFatView(wxCommandEvent &event)
{
    if (!fatView)
        fatView = new FatView(this);
    ToggleView(fatView, appSettings.bFatView);
}

void thFrame::CmdExportView(wxCommandEvent &event)
{
    if (!exportView)
        exportView = new ExportView(this);
    ToggleView(exportView, appSettings.bExportView);
}
#endif // TBDL

void thFrame::SetHexWnd(HexWnd *hw)
{
    if (hw == m_hw)
        return;
    this->m_hw = hw;
    if (hw)
    {
        SetTitle(hw->GetTitle() + _T(" - Tyrannosaurus Hex"));
        DisplayPane::s_pSettings = &hw->s; //!
        //hw->Raise(); // need this to get focus from wxAuiNotebook
        hw->SetFocus(); // Testing 2007-08-28.  Automatic focus quit working a while ago.
        // now fixed with SetFocus() in thFrame() after adding initial windows
    }
    else
        SetTitle(_T("Tyrannosaurus Hex"));
    UpdateViews(hw, DataView::DV_ALL);
    UpdateUI();
}

void thFrame::AddHexWnd(HexWnd *hw)
{
    if (!hw || !hw->DataOk()) {
        delete hw;
        return;
    }

    if (bWindowShown)
    {
        tabs->Freeze();
        if (!tabs->IsShown())
        {
            wxAuiPaneInfo &pane = m_mgr.GetPane(_T("HexWnd"));
            wxWindow *old_panel = NULL;
            if (m_hw)
            {
                tabs->AddPage(m_hw, m_hw->GetTitle());
                tabs->Show();
                pane.Window(tabs);
            }
            else
            {
                old_panel = pane.window;
                pane.Window(hw);
            }
            m_mgr.Update();
            delete old_panel;
        }
        if (m_hw) // at least one HexWnd already shown?
        {
            tabs->AddPage(hw, hw->GetTitle());
            tabs->SetSelection(tabs->GetPageCount() - 1);
        }
        SetHexWnd(hw);
        tabs->Thaw();
    }
    else
        pendingWindows.push_back(hw);
}

void thFrame::OnActivate(wxActivateEvent &event)
{
    //! not needed when not using HWND_TOPMOST
    //if (appSettings.bFullScreen)
    //{
    //    if (event.GetActive()) // Our window is being reactivated
    //        PRINTF("Setting TOPMOST bit.\n");
    //    else
    //        PRINTF("Clearing TOPMOST bit.\n");
    //    if (event.GetActive()) // Our window is being reactivated
    //        SetWindowPos(GetHwnd(), HWND_TOPMOST, 0, 0, 0, 0,
    //            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    //    else // Our window is being deactivated
    //        SetWindowPos(GetHwnd(), HWND_NOTOPMOST, 0, 0, 0, 0,
    //            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    //}
    event.Skip();
}

void thFrame::CmdOpenSpecial(wxCommandEvent &event)
{
    thRecentChoiceDialog dlg(this, _T("Open Special"),
       _T("Enter the name of a special file, pipe, socket, etc."),
       appSettings.asOpenSpecial);
    if (dlg.ShowModal() == wxID_OK)
        OpenFile(appSettings.asOpenSpecial[0], true);
}

#ifdef TBDL
void thFrame::CmdWriteSpecial(wxCommandEvent &event)
{
    thPipeOutDialog dlg(this);
    m_dlgPipeOut = &dlg;
    //if (m_dlgPipeOut == NULL)
    //    m_dlgPipeOut = new thPipeOutDialog(this);
    m_dlgPipeOut->Init(m_hw->GetSelection());

    if (m_dlgPipeOut->ShowModal() == wxID_OK)
    {
        if (m_dlgPipeOut->PipeToProcess())
        {
            //PipeToProcess(dlg.GetFile(), dlg.GetArgs(), dlg.GetDir());
            if (!m_pSpawnHandler)
                m_pSpawnHandler = new SpawnHandler(this);
            m_pSpawnHandler->Start(m_dlgPipeOut->GetFile(), m_dlgPipeOut->GetArgs(), m_dlgPipeOut->GetDir(),
                m_dlgPipeOut->SelectionOnly());
        }
        else // Write to handle.  May be a pipe, socket, device, whatever.
        {
            //wxMessageBox(_T("Not yet implemented."));
            if (m_dlgPipeOut->SelectionOnly())
            {
                Selection sel = m_hw->GetSelection();
                m_hw->doc->SaveRange(m_dlgPipeOut->GetFile(), sel.GetFirst(), sel.GetSize());
            }
            else
                m_hw->doc->SaveRange(m_dlgPipeOut->GetFile(), 0, m_hw->DocSize());
        }
    }

    m_dlgPipeOut = NULL;
}

void thFrame::OnPipeComplete()
{
    if (m_pSpawnHandler->bComplete)
        PRINTF(_T("The child process exited with code %d (0x%X)\n"),
            m_pSpawnHandler->exitCode,
            m_pSpawnHandler->exitCode);
    else
        PRINTF(_T("The child process is still active.\n"));

    // SpawnHandler could delete itself after it calls this method...
}
#endif // TBDL

void thFrame::OpenFile(wxString filename, bool bReadOnly) //! need a better read/write argument
{
    #ifdef TBDL
    wxFileName fn(filename);
    bool isDrive = false;
    if (fn.HasVolume() && !fn.HasName() &&  // filename is drive root
        filename[0] != '\\')                // and filename isn't special (confused by "\\?\Volume{...}\")
    {
        filename = _T("\\\\.\\") + fn.GetVolume().Upper() + _T(":");
        isDrive = true;
    }

    // Check if filename looks like "\\.\PhysicalDrive0" or "\\.\C:".
    // arrgh -- too many backslashes.
    wxRegEx re(_T("\\\\\\\\\\.\\\\[a-zA-Z0-9]+:?\\\\?$"));
    if (re.Matches(filename))
       isDrive = true;
    // check for "\\?\Volume{...}\"
    re.Compile(_T("\\\\\\\\\\?\\\\Volume\\{[a-fA-F0-9]+\\}\\\\?$"));
    if (re.Matches(filename))
       isDrive = true;

    if (isDrive)
    {
        HexWnd *hw = new HexWnd(this);
        hw->OpenDrive(filename, true); // always read-only for drives?
        AddHexWnd(hw);
        return;
    }

    if (filename.Contains(_T("*")) || filename.Contains(_T("?")))
    {
        wxString single;
        wxDir dir(wxGetCwd());
        if (dir.GetFirst(&single, filename))
        {
            do {
                OpenFile(single, bReadOnly);
            } while (dir.GetNext(&single));
        }
        else
            wxMessageBox(_T("No files found matching ") + filename);
        return;
    }

    HANDLE hFile = 0;
    if (!bReadOnly)
    {
        hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
        if (hFile == INVALID_HANDLE_VALUE)
            bReadOnly = true; // try again
    }
    if (bReadOnly)
        hFile = CreateFile(filename, GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           0, OPEN_EXISTING, 0, 0);

    DWORD error = GetLastError();

    DWORD type = GetFileType(hFile);
    if (type == FILE_TYPE_PIPE)
    {
        //CreateReaderThread(hFile);
        //FastWriteBuf buf(1024);
        //ReadFileEx(hFile, buf, 1024,
    }
    //if (type == FILE_TYPE_DISK) // Oops.  This applies to real files on a disk.
    //{
    //    CloseHandle(hFile);
    //    HexWnd *hw = new HexWnd(this);
    //    if (hw->OpenDrive(filename, bReadOnly))
    //        AddHexWnd(hw);
    //    else
    //        delete hw;
    //    return;
    //}
    CloseHandle(hFile);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        wxString msg;
        msg.Printf(_T("Couldn't open %s.\nError code %d"), filename.c_str(), error);
        wxMessageBox(msg);
    }
    else
    #endif // TBDL
    {
        HexWnd *hw = new HexWnd(this);
        if (hw->OpenFile(filename, bReadOnly))
            AddHexWnd(hw);
        else
            delete hw;
    }
}

void thFrame::OnPaneClose(wxAuiManagerEvent &event)
{
    wxWindow *win = event.GetPane()->window;
    if (win == map)
        appSettings.bFileMap = false;
    if (win == viewString)
        appSettings.bStringView = false;
    if (win == viewStruct)
        appSettings.bStructureView = false;
    if (win == viewNumber)
        appSettings.bNumberView = false;
    if (win == docList)
        appSettings.bDocList = false;
    //if (win == profView)
    //{
    //    //! can't get it back.  UI doesn't know about it.
    //}
#ifdef INCLUDE_LIBDISASM
    if (win == disasmView)
        appSettings.bDisasmView = false;
#endif
    #ifdef TBDL
    if (win == fatView)
        appSettings.bFatView = false;
    if (win == exportView)
        appSettings.bExportView = false;
    #endif
    UpdateUI();
}

#ifdef WXFLATNOTEBOOK
#define NotebookEventType wxFlatNotebookEvent
#else
#define NotebookEventType wxAuiNotebookEvent
#endif

void thFrame::OnPageChanged(NotebookEventType& event)
{
    SetHexWnd((HexWnd*)tabs->GetPage(event.GetSelection()));
    UpdateUI();
}

//void thFrame::OnPageClosing(wxAuiNotebookEvent& event)
//{
//    //if (tabs->GetPageCount() == 1) // removing the last page?
//    //    event.Veto(); // can't do that
//   UpdateViews((HexWnd*)tabs->GetPage(event.GetSelection()), DataView::DV_CLOSE); //! ???
//}

void thFrame::OnPageClosed(NotebookEventType& WXUNUSED(event))
{
    if (tabs->GetPageCount() == 1) // removed second-to-last page, only one left
    {
        HexWnd *otherWnd = (HexWnd*)tabs->GetPage(0);
        otherWnd->Freeze();
        tabs->RemovePage(0);
        tabs->Hide();
        otherWnd->Reparent(this);
        wxAuiPaneInfo &pane = m_mgr.GetPane(tabs);
        pane.Window(otherWnd);
        otherWnd->Thaw();
        m_mgr.Update(); //! do we need this?
    }
}

void thFrame::CmdCloseTab(wxCommandEvent &WXUNUSED(event))
{
    if (tabs->IsShown())
    {
        tabs->DeletePage(tabs->GetSelection());
#ifndef WXFLATNOTEBOOK
        wxAuiNotebookEvent e;
        OnPageClosed(e); // wxAuiNotebook doesn't do this.
#endif
        UpdateUI();
    }
    else if (m_hw)
    {
        //m_mgr.DetachPane(m_hw);
        wxAuiPaneInfo &pane = m_mgr.GetPane(_T("HexWnd"));
        pane.Window(new wxPanel(this));
        m_mgr.Update();
        delete m_hw;
        SetHexWnd(NULL);
    }
    //else
        //MessageBeep(-1); //! setstatus: can't close this page
}

void thFrame::OnPageContextMenu(NotebookEventType& event)
{
    wxMenu menu;
    menu.Append(IDM_Save, _T("&Save ") + m_hw->GetTitle());
    menu.Append(-1, _T("Close"));
    menu.Append(-1, _T("Close &All But This"));
    menu.AppendSeparator();
    menu.Append(-1, _T("Copy &Full Path"));
    menu.Append(-1, _T("&Open Containing Folder"));
}

void thFrame::UpdateUI()
{ // update checked menu items
    wxMenuBar *menuBar = GetMenuBar();
    if (!menuBar)  // shouldn't happen
        return;

    menuBar->Check(IDM_ViewFileMap, appSettings.bFileMap);
    menuBar->Check(IDM_ViewDocList, appSettings.bDocList);

    menuBar->Check(IDM_ViewNumberView, appSettings.bNumberView);
    menuBar->Check(IDM_ViewStringView, appSettings.bStringView);
    menuBar->Check(IDM_ViewStructureView, appSettings.bStructureView);
    menuBar->Check(IDM_ViewDisasmView, appSettings.bDisasmView);
    menuBar->Check(IDM_ViewFatView, appSettings.bFatView);
    menuBar->Check(IDM_ViewExportView, appSettings.bExportView);

    menuBar->Check(IDM_ViewStatusBar, appSettings.bStatusBar);
    menuBar->Check(IDM_ViewToolBar, appSettings.bToolBar);
    menuBar->Check(IDM_FullScreen, appSettings.bFullScreen);

    if (m_hw)
    {
        menuBar->Check(IDM_ViewRuler, m_hw->m_settings.bShowRuler);
        menuBar->Check(IDM_ViewStickyAddr, m_hw->m_settings.bStickyAddr);
        menuBar->Check(IDM_ViewAdjustColumns, m_hw->m_settings.bAdjustLineBytes);
        menuBar->SetLabel(IDM_ToggleReadOnly, m_hw->IsReadOnly() ? _T("Re-open writeable") : _T("Re-open read-only"));
    }
}

void thFrame::OnMenuOpen(wxMenuEvent &event)
{
    //! todo: update menu checks and clipboard options here?
}

void thFrame::OnSetFocus(wxFocusEvent &event)
{
    // This is only needed for dialog reuse hack, I think.
    if (m_hw)
        m_hw->SetFocus();
}

void thFrame::CmdToggleReadOnly(wxCommandEvent &event)
{
    if (!m_hw || !m_hw->m_pDS)
        return;

    if (m_hw->ToggleReadOnly())
    {
        HexWnd *tmp = m_hw;
        SetHexWnd(NULL);
        SetHexWnd(tmp);
        return;
    }

    wxString fullpath = m_hw->m_pDS->GetFullPath();
    bool bReadOnly = !m_hw->IsReadOnly();
    if (!wxFile::Exists(fullpath))
       return;

    if (tabs->IsShown())
        tabs->DeletePage(tabs->GetSelection());
    else
    {
        wxAuiPaneInfo &pane = m_mgr.GetPane(_T("HexWnd"));
        pane.Window(new wxPanel(this));
        m_mgr.Update();
        delete m_hw;
    }
    m_hw = NULL;

    HexWnd *hw = new HexWnd(this);
    if (hw->OpenFile(fullpath, bReadOnly))
        AddHexWnd(hw);
    else
        delete hw;
    m_mgr.Update();
    //! to do: change status bar to reflect new edit mode
}

void thFrame::CmdAbout(wxCommandEvent &WXUNUSED(event))
{
    AboutDialog about(this);
    about.ShowModal();
}

void thFrame::OnHelp(wxHelpEvent &event)
{
    AboutDialog about(this);
    about.ShowModal();
}

#ifdef TBDL
void thFrame::CmdOpenTestFile(wxCommandEvent &event)
{
    HexWnd *hw = new HexWnd(this);
    //wxString file = _T("C:\\Program Files\\Microsoft Virtual PC\\Virtual Machine Additions\\VMAdditions.iso");
    wxString file = _T("D:\\download\\Win2003_ddk.iso");
    if (hw->OpenTestFile(file))
        AddHexWnd(hw);
    else
        delete hw;
    m_mgr.Update();
}

void thFrame::CmdOpenProcessFile(wxCommandEvent &event)
{
    ProcessDialog dlg(this, _T("Open Process"));
    dlg.chkReadOnly->SetValue(true);
    dlg.chkReadOnly->Disable();
    if (dlg.ShowModal() == wxID_CANCEL)
        return;

    int sel = dlg.GetSelection();
    DWORD procID = dlg.GetPID(sel);
    wxString strNum = ::wxGetTextFromUser(_T("Enter the file handle.\n(Sorry, I can't give you a list.)"), _T("Open foreign file"));
    DWORD hForeignFile = ReadUserNumber(strNum);
    HexWnd *hw = new HexWnd(this);
    DataSource *pDS = new ProcessFileDataSource(procID, hForeignFile);
    if (hw->OpenDataSource(pDS))
        AddHexWnd(hw);
    else
        delete hw;
    pDS->Release();
    m_mgr.Update();
}
#endif // TBDL

void thFrame::CmdNextBinChar(wxCommandEvent &event)
{
    if (!m_hw) return;
    HexDoc *doc = m_hw->doc;
    const bool isUnicode = m_hw->m_pane[m_hw->GetSelection().iRegion].id == DisplayPane::ID_UNICODE;

    THSIZE start = m_hw->GetSelection().nStart;
    if (!isUnicode && start < doc->size)
        start++;

    for (; start < doc->size; start += MEGA)
    {
        int blocksize = (int)wxMin(doc->size - start, MEGA);
        const uint8 *buf = doc->Load(start, blocksize);
        if (!buf)
            break;
        if (isUnicode)
        {
            blocksize--;
            if (m_hw->s.iEndianMode == NATIVE_ENDIAN_MODE)
            {
                for (int x = 0; x < blocksize; x += 2)
                {
                    //uint32 c = buf[x] + ((uint32)buf[x+1] << 8);
                    uint32 c = *(uint16*)(buf+x);  // works for aligned data
                    if (c >= 128 || !my_isprint[c])
                    {
                        m_hw->CmdSetSelection(start + x, start + x + 2);
                        return;
                    }
                }
            }
            else //if (m_hw->s.iEndianMode == LITTLE_ENDIAN_MODE)
            {
                for (int x = 0; x < blocksize; x += 2)
                {
                    uint32 c = ((uint32)buf[x] << 8) + buf[x];
                    if (c >= 128 || !my_isprint[c])
                    {
                        m_hw->CmdSetSelection(start + x, start + x + 2);
                        return;
                    }
                }
            }
        }
        else
        {
            for (int x = 0; x < blocksize; x++)
                if (!my_isprint[buf[x]])
                {
                    m_hw->CmdMoveCursor(start + x);
                    return;
                }
        }
    }
    SetStatusText(_T("EOF"));
    wxBell();
}

void thFrame::CmdPrevBinChar(wxCommandEvent &event)
{
    if (!m_hw) return;
    HexDoc *doc = m_hw->doc;

    int blocksize = MEGA;
    for (THSIZE start = m_hw->GetSelection().nStart; start > 0; start -= blocksize)
    {
        blocksize = (int)wxMin(start, MEGA);
        const uint8 *buf = doc->Load(start - blocksize, blocksize);
        if (!buf)
            break;
        for (int x = blocksize; x > 0; x--)
            if (!my_isprint[buf[x - 1]])
            {
                m_hw->CmdMoveCursor(start - blocksize + x - 1);
                return;
            }
    }
    SetStatusText(_T("EOF"));
    wxBell();
}

bool thFrame::GetDocsFromUser(int n, int *pnDoc)
{
    wxArrayString names;
    for (int i = 0; i < (int)tabs->GetPageCount(); i++)
        names.Add(((HexWnd*)tabs->GetPage(i))->GetTitle());

    wxArrayInt selections;
    for (int i = 0; i < n && i < (int)names.GetCount(); i++)
        if (pnDoc[i] >= 0)
            selections.Add(pnDoc[i]);
    int selCount = ::wxGetSelectedChoices(selections, _T("Select two documents to compare."), _T("T. Hex"), names, this);
    if (selCount == 0)
        return false;
    if (selCount == 1)
    {
        names.RemoveAt(selections[0]);
        int i = ::wxGetSingleChoiceIndex(_T("Select the second document to compare."), _T("T. Hex"), names, this);
        if (i == -1)
            return false;
        pnDoc[1] = i;
        return true;
    }

    for (int i = 0; i < n && i < selCount; i++)
        pnDoc[i] = selections[i];
    return true;
}

void thFrame::PreloadFile(HexDoc *doc, wxString title)
{
    THSIZE offset, blockSize = MEGA, fileSize = doc->GetSize();
    thProgressDialog progress(fileSize, this, _T("Pre-caching ") + title);
    for (offset = 0; offset < fileSize; offset += blockSize)
    {
        if (!progress.Update(offset))
            break;

        if (0 == doc->Load(offset, blockSize))
        {
            PRINTF(_T("Couldn't read from doc at 0x%I64X\n"), offset);
            break;
        }
    }
}

void thFrame::CompareBuffers()
{
    int nDoc[2];
    if (tabs->GetPageCount() < 2)
    {
        wxMessageBox(_T("Need at least two buffers open."), _T("CompareBuffers"));
        return;
    }
    else if (tabs->GetPageCount() == 2)
    {
        nDoc[0] = 0;
        nDoc[1] = 1;
    }
    else // tabs->GetPageCount() > 2
    {
        nDoc[0] = tabs->GetSelection();
        if (nDoc[0] == tabs->GetPageCount() - 1)
            nDoc[1] = nDoc[0] - 1;
        else
            nDoc[1] = nDoc[0] + 1;
        if (!GetDocsFromUser(2, nDoc))
            return;
    }

    HexWnd *hw1 = (HexWnd*)tabs->GetPage(nDoc[0]);
    HexWnd *hw2 = (HexWnd*)tabs->GetPage(nDoc[1]);
    HexDoc *doc1 = hw1->doc;
    HexDoc *doc2 = hw2->doc;
    const uint8 *buf1, *buf2;

    Selection sel1 = hw1->GetSelection();
    Selection sel2 = hw2->GetSelection();
    THSIZE base1 = 0, base2 = 0;
    THSIZE size1 = doc1->GetSize(), size2 = doc2->GetSize();
    if (sel1.GetSize() > 0)
        base1 = sel1.GetFirst(), size1 = sel1.GetSize();
    if (sel2.GetSize() > 0)
        base2 = sel2.GetFirst(), size2 = sel2.GetSize();
    THSIZE compareSize = wxMin(size1, size2);
    THSIZE diffCount = 0, diffBytes = 0;
    int blockSize = HEXDOC_BUFFER_SIZE;

    //! Speed test, 2009-05-02.
    //PreloadFile(hw1->doc, hw1->GetTitle());
    //PreloadFile(hw2->doc, hw2->GetTitle());

    // New procedure, 2009-05-02.
    // Comparing goes much faster off this 750GB NTFS WDC7500AACS if we read both files separately first.
    // If files are the same size (and larger than some arbitrary size, like 1MB),
    //  compare the first megabyte of both files.
    // If this matches, compute a hash for both files.
    // Inform the user of the results and ask if they want to do a full comparison.

    if (compareSize > appSettings.nSmartCompareSize &&
        appSettings.nSmartCompareSize > 0)
    {
        if (0 == (buf1 = doc1->Load(base1, blockSize)))
        {
            PRINTF(_T("Couldn't read from doc 1 at 0x%I64X\n"), base1);
            return;
        }
        if (0 == (buf2 = doc2->Load(base2, blockSize)))
        {
            PRINTF(_T("Couldn't read from doc 2 at 0x%I64X\n"), base2);
            return;
        }

        wxString sizemsg, hashmsg;
        int iconStyle = wxICON_HAND;
        if (size1 > size2)
            sizemsg = hw1->GetTitle() + _T("\n (left side) is longer by ") + FormatBytes(size1 - size2, MEGA) + _T(".");
        else if (size2 > size1)
            sizemsg = hw2->GetTitle() + _T("\n (right side) is longer by ") + FormatBytes(size2 - size1, MEGA) + _T(".");
        //else
        //    sizemsg = _T("Both files are the same size.");

        if (memcmp(buf1, buf2, blockSize) == 0)
        {
            ULONG hash1, hash2;
            if (doc1->ComputeAdler32(base1, compareSize, hash1) &&
                doc2->ComputeAdler32(base2, compareSize, hash2) &&
                hash1 == hash2)
            {
                hashmsg = _T("Checksums match.\n");
                iconStyle = wxICON_INFORMATION;  //! todo: better icon
            }
            else
                hashmsg = _T("Files are different.\n");
        }
        else
            hashmsg = _T("Differences found in first megabyte.\n");

        if (wxMessageBox(hashmsg + sizemsg + _T("\nDo full byte-by-byte comparison?"),
            _T("T.Hex Buffer Comparison"), wxYES_NO | iconStyle) == wxNO)
            return;
    }

    wxString msg = hw1->GetTitle() + _T("\n") + hw2->GetTitle();
    thProgressDialog progress(compareSize, this, msg, _T("T.Hex Buffer Comparison"));

    wxFFile df("diffs.txt", "a");
    df.Write(hw1->GetTitle() + '\n');
    df.Write(hw2->GetTitle() + '\n');
    TCHAR diffbuf[21];
    int ad = CountDigits(wxMax(hw1->GetLastDisplayAddress(), hw2->GetLastDisplayAddress()), 16);
    int cd = 2;

    THSIZE offset, firstDiff, lastDiff = 0;
    bool firstSet = false;
    for (offset = 0; offset < compareSize; offset += blockSize)
    {
        if (!progress.Update(offset, true, wxString::Format(_T(", %I64d difference"), diffCount) + Plural(diffCount)))
            break;

        if (compareSize - offset < blockSize)
            blockSize = compareSize - offset;

        if (0 == (buf1 = doc1->Load(offset + base1, blockSize)))
        {
            PRINTF(_T("Couldn't read from doc 1 at 0x%I64X\n"), offset + base1);
            break;
        }
        if (0 == (buf2 = doc2->Load(offset + base2, blockSize)))
        {
            PRINTF(_T("Couldn't read from doc 2 at 0x%I64X\n"), offset + base2);
            break;
        }

        if (memcmp(buf1, buf2, blockSize))
        {
            for (int i = 0; i < blockSize; i++)
            {
                if (buf1[i] != buf2[i]) {
                    if (!firstSet) {
                        firstDiff = offset + i;
                        firstSet = true;
                    }
                    lastDiff = offset + i;

                    // Update 2008-09-24.  Now treat this case as one difference.
                    //    File 1: 11002200330044005500660077008800
                    //    File 2: 11002200330044000000000000008800

                    const uint8 save1 = buf1[i], save2 = buf2[i];
                    int changed1 = 0, changed2 = 0;

                    THSIZE bytes = 1, lastDiffBytes = 1;
                    int start = i;
                    i++;
                    while (i < blockSize && (buf1[i] != buf2[i] || !changed1 || !changed2))
                    {
                        bytes++;
                        if (buf1[i] != buf2[i])
                            lastDiffBytes = bytes;
                        changed1 |= buf1[i] ^ save1;
                        changed2 |= buf2[i] ^ save2;
                        i++;
                    }

                    bytes = lastDiffBytes;

                    if (diffCount < 1000) {
                        // Give a quick summary of how the bytes changed.
                        TCHAR *s = diffbuf;
                        for (int j = 0; j < wxMin(bytes, 4); j++, s += 2)
                            my_itoa((uint32)buf1[start + j], s, 16, 2);
                        wxStrcpy(s, _T(" => "));
                        s += 4;
                        for (int j = 0; j < wxMin(bytes, 4); j++, s += 2)
                            my_itoa((uint32)buf2[start + j], s, 16, 2);
                        *s = 0;  // Terminate string.

                        if (bytes == 1) {
                            PRINTF(  _T("0x%0*I64X,  %*d byte  %s\n"), ad, offset + start, cd, 1, diffbuf);
                            df.Write(wxString::Format("0x%0*I64X,  %*d byte  %S\n" , ad, offset + start, cd, 1, diffbuf));
                        } else {
                            PRINTF(  _T("0x%0*I64X, %*I64d bytes  %s\n"), ad, offset + start, cd, bytes, diffbuf);
                            df.Write(wxString::Format("0x%0*I64X, %*I64d bytes  %S\n" , ad, offset + start, cd, bytes, diffbuf));
                        }
                        cd = fnmax(cd, CountDigits(bytes, 10));
                    }
                    else if (diffCount == 1000)
                        PRINTF( _T("Too many differences.\n"));
                    diffBytes += bytes;
                    diffCount++;
                }
            }
            df.Flush();
        }
    }

    progress.Update(compareSize - 1); // does stupid things if you pass the max value
    //progress.Hide();

    if (offset >= compareSize) // not canceled?
    {
        msg = wxString(FormatBytes(compareSize, MEGA)) + _T(" compared.\n");
        if (diffCount)
        {
            msg += FormatDec(diffCount) + _T(" different block") + Plural(diffCount) +
                _T(" found in ") + FormatBytes(diffBytes, MEGA);
            msg += _T(".\nFirst difference is at 0x") + FormatHex(firstDiff);
            msg += _T(".\nLast difference is at 0x") + FormatHex(lastDiff);
            hw1->ScrollToRange(base1 + firstDiff, base1 + firstDiff, J_AUTO);
            hw2->ScrollToRange(base2 + firstDiff, base2 + firstDiff, J_AUTO);
        }
        else
            msg += _T("No differences found");
        if (size1 > size2)
            msg += _T(".\n") + hw1->GetTitle() + _T("\n (left side) is longer by ") + FormatBytes(size1 - size2, MEGA);
        if (size2 > size1)
            msg += _T(".\n") + hw2->GetTitle() + _T("\n (right side) is longer by ") + FormatBytes(size2 - size1, MEGA);
        PRINTF(_T("%s.\n"), msg.c_str());
        df.Write(msg + ".\n\n");
        wxMessageBox(msg + (wxChar)'.');
    }

    df.Close();
}

void thFrame::OnDoubleClick(wxMouseEvent &event)
{
    event.SetEventType(wxEVT_LEFT_DOWN);
    GetEventHandler()->ProcessEvent(event);
    // Yes!  I fooled wx!
    event.Skip(false);
}

bool thFrame::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &filenames)
{
    wxArrayInt failed;
    for (int i = 0; i < (int)filenames.GetCount(); i++)
    {
        HexWnd *hw = new HexWnd(this);
        if (hw->OpenFile(filenames[i], appSettings.bDefaultReadOnly))
            AddHexWnd(hw);
        else
        {
            delete hw;
            failed.Add(i);
        }
    }

    if (failed.GetCount())
    {
        wxMessageBox(_T("Couldn't open some files."));
    }

    return true;
}

void thFrame::CmdZipRecover(wxCommandEvent &event)
{
    if (!m_hw || !m_hw->doc || !m_hw->doc->m_pDS->IsFile())
    {
        wxMessageBox(_T("This only works on files."));
        return;
    }

    wxFileName fn(m_hw->doc->m_pDS->GetFullPath());
    thDocInputStream in(m_hw->doc);
    in.SetSeekable(false);
    wxZipInputStream zip(in);

    wxZipEntry *entry;
    wxString msg;

    //! Must trick WX into thinking file is not seekable.
    //  For now, intercept wxZipInputStream::FindEndRecord().
    while ((entry = zip.GetNextEntry()) != NULL)
    {
        wxString name = entry->GetName();
        wxString newname = fn.GetPathWithSep() + name;
        bool write = true;

        if (wxFile::Exists(newname))
        {
            int result = wxMessageBox(name + _T(" exists.  Overwrite?"), _T("ZIP recovery"), wxYES_NO | wxCANCEL);
            if (result == wxYES)
                if (wxFile::Exists(newname) && !wxRemoveFile(newname))  // user may delete it himself.
                {
                    wxMessageBox(_T("Couldn't delete ") + name);
                    write = false;
                }
            else if (result == wxNO)
                write = false;
            else if (result == wxCANCEL)
            {
                delete entry;
                return;
            }
        }

        if (write)
        {
            //! todo: Progress bar.
            //! todo: What if the file name has a path?
            wxFFileOutputStream out(newname);
            zip.Read(out);  // read until EOF or error

            msg += _T("\n ") + name;
            if (out.TellO() != entry->GetSize())
                msg += _T(" (") + wxString(FormatBytes(out.TellO())) +
                       _T(" of ") + wxString(FormatBytes(entry->GetSize())) + _T(")");
        }

        delete entry;
    }

    if (msg.Len())
        msg = _T("These files were extracted:") + msg;
    else
        msg += _T("No files were extracted.");

    PRINTF(_T("%s\n"), msg.c_str());
    wxMessageBox(msg, _T("T. Hex"));
}

void thFrame::CmdCompressability(wxCommandEvent &event)
{
    if (!m_hw || !m_hw->doc)
        return;
    Selection sel = m_hw->GetSelectionOrAll();
    if (sel.GetSize() == 0)
        return;

    thDocInputStream in(m_hw->doc, &sel);
    thNullOutputStream null;
    int compression = wxZ_BEST_SPEED;  // wxZ_DEFAULT_COMPRESSION
    wxZlibOutputStream out(null, compression, wxZLIB_NO_HEADER);
    thProgressDialog progress(sel.GetSize(), m_hw, _T("Fast zlib compression..."));

    //in.Read(out);  // Read everything.
    THSIZE offset, size = sel.GetSize(), start = sel.GetFirst();
    wxString progressMsg;
    int blocksize;
    for (offset = 0; offset < size; offset += blocksize)
    {
        blocksize = wxMin(MEGA, size - offset);
        const uint8 *data = m_hw->doc->Load(offset + start, blocksize);
        if (!data)
            return;
        out.Write(data, blocksize);
        if (offset > 5 * MEGA && progress.updateTimer.IsTimeUp())
            progressMsg.Printf(_T(", %0.1f%% savings"), 100.0 - null.TellO() * 100.0 / offset);
        if (!progress.Update(offset, true, progressMsg))
            break;
    }

    out.Close();  // flush zlib buffer
    wxFileOffset outsize = null.TellO();
    double ratio = (double)offset / outsize;
    int savingsPct = 0;
    if (outsize < (wxFileOffset)offset)
        savingsPct = (offset - outsize) * 100 / offset;
    LPCTSTR explanation;
    switch (savingsPct / 10)
    {
    case 0: explanation = _T("Abysmal"); break;
    case 1: explanation = _T("Horrible"); break;
    case 2: explanation = _T("Bad"); break;
    case 3: explanation = _T("Lousy"); break;
    case 4: explanation = _T("Disappointing"); break;
    case 5: explanation = _T("Mediocre"); break;
    case 6: explanation = _T("Decent"); break;
    case 7: explanation = _T("Good"); break;
    case 8: explanation = _T("Excellent"); break;
    case 9:
    case 10: explanation = _T("Fantastic"); break;
    }
    wxString compSize = FormatBytes(outsize);
    wxString howMuchFile;
    if (offset < sel.GetSize())
        howMuchFile.Printf(_T(" (%.0f%% of input)"), offset * 100.0 / sel.GetSize());
    wxMessageBox(wxString::Format(_T("Tested %s with fast zlib compression.%s\n")
                                  _T("Compressed size: %s.\n")
                                  _T("Compression ratio: %0.*f.\n")
                                  _T("Space savings:\n%d%% (%s)"),
                                  FormatBytes(offset),
                                  howMuchFile.c_str(),
                                  compSize.c_str(),
                                  (ratio < 1.1 ? 3 : 2), ratio,
                                  savingsPct, explanation),
        _T("T. Hex"), wxOK | wxCENTRE, &progress);
}

void thFrame::CmdSwapOrder2(wxCommandEvent &event)
{
    if (!m_hw)
        return;
    Selection sel = m_hw->GetSelection();

    if (sel.GetSize() == 0)
    {
        wxMessageBox(_T("Select some bytes first."), APPNAME);
        return;
    }

    uint8 tmp[2];
    for (THSIZE offset = sel.GetFirst(); offset + 2 <= sel.GetLast(); offset += 2)
    {
        m_hw->doc->Read(offset, 2, tmp);
        reverse(tmp, 2);
        m_hw->doc->ReplaceAt(offset, 2, tmp, 2);
    }
}

void thFrame::CmdSwapOrder4(wxCommandEvent &event)
{
    if (!m_hw)
        return;
    Selection sel = m_hw->GetSelection();

    if (sel.GetSize() == 0)
    {
        wxMessageBox(_T("Select some bytes first."), APPNAME);
        return;
    }

    uint8 tmp[4];
    for (THSIZE offset = sel.GetFirst(); offset + 4 <= sel.GetLast(); offset += 4)
    {
        m_hw->doc->Read(offset, 4, tmp);
        reverse(tmp, 4);
        m_hw->doc->ReplaceAt(offset, 4, tmp, 4);
    }
}

void thFrame::CmdUnZlib(wxCommandEvent &event)
{
    if (!m_hw)
        return;
    Selection sel = m_hw->GetSelection();

    if (sel.GetSize() == 0)
    {
        wxMessageBox(_T("Select some bytes first."), APPNAME);
        return;
    }

    wxString uncomp;
    thDocInputStream docstream(m_hw->doc, &sel);

    {
        wxZlibInputStream in(&docstream);
        wxStringOutputStream out(&uncomp, wxConvISO8859_1);
        in.Read(out);
    }

    #ifdef TBDL
    clipboard.SetData(uncomp, DisplayPane::NUMERIC, thCopyFormat::RAW);
    #endif
}

#ifndef WIN32
DWORD thFrame::GetHWND()
{
    return 0;  //! TBDL
}
#endif
