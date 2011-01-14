#include "precomp.h"
#include "thex.h"
#include "thFrame.h"
#include "utils.h"

#define new New

//thApp::thApp()
//{
//    CreateTraits();
//    GetTraits();
//    //wxTheApp->CreateTraits();
//    wxTheApp->GetTraits();
//}

bool thApp::OnInit()
{
    HANDLE hFlag = CreateEventA(NULL, 0, 0, "T_Hex.event");
    bool useIPC = true;
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        wxClient *cli = new wxClient();
        wxConnectionBase *conn = cli->MakeConnection(_T("localhost"), _T("T_Hex.ipc"), _T("CmdLine"));
        if (conn)
        {
            // The topic name must be less than 255 characters on Windows.
            wxString data = wxGetCwd();
            conn->Poke(_T("dir"), (const wxChar*)data.c_str());
            data = GetCommandLine();
            conn->Poke(_T("cmd"), (const wxChar*)data.c_str());  //! What if this fails?

            char *swnd = (char*)conn->Request(_T("HWND"), NULL, wxIPC_TEXT);
            if (swnd)
            {
                HWND hWnd = (HWND)strtol(swnd, NULL, 16);
                SetForegroundWindow(hWnd);
            }
            delete conn;
            delete cli;
            return false;
        }
        else {
            //! In debug mode, wx gives us a suitable message already.
            wxMessageBox(_T("Couldn't create IPC connection.  Running normally."));
            delete conn;
            delete cli;
            useIPC = false;
        }
    }

#ifdef _DEBUG
    //_CrtSetBreakAlloc(9558);
    CreateConsole(_T("TH Console"));
#endif
    wxFrame* frame = new thFrame(useIPC);
    SetTopWindow(frame);
    frame->Show();
    //::PostMessage((HWND)frame->GetHWND(), WM_CLOSE, 0, 0); // close immediately for profiling
    return true;
}

#ifdef __WXDEBUG__
//blah blah blah
#endif

IMPLEMENT_APP(thApp)
