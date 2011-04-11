#include "precomp.h"
#include "thex.h"
#include "thFrame.h"
#include "utils.h"

#define new New

// Return a space-separated list of argv[1:end].
// Internal spaces are escaped with backslash.
wxString ConvertArgsToString(int argc, wxChar** argv)
{
    wxString s;
    for (int i = 1; i < argc; i++)
    {
        wxString arg(argv[i]);
        arg.Replace(" ", "\\ ");
        if (i > 1)
            s += " ";
        s += arg;
    }
    return s;
}

//thApp::thApp()
//{
//    CreateTraits();
//    GetTraits();
//    //wxTheApp->CreateTraits();
//    wxTheApp->GetTraits();
//}

bool thApp::OnInit()
{
    bool useIPC = true;
    #ifdef TBDL
    HANDLE hFlag = CreateEventA(NULL, 0, 0, "T_Hex.event");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        wxClient *cli = new wxClient();
        wxConnectionBase *conn = cli->MakeConnection(_T("localhost"), _T("T_Hex.ipc"), _T("CmdLine"));
        if (conn)
        {
            // The topic name must be less than 255 characters on Windows.
            wxString data = wxGetCwd();
            conn->Poke(_T("dir"), (const wxChar*)data.c_str());
            data = ConvertArgsToString(argc, argv));
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
    #endif

#if defined(_MSC_VER) && defined(_DEBUG)
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
