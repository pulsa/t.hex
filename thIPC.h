#ifndef _THIPC_H_
#define _THIPC_H_
#pragma once

#include <wx/ipc.h>

class ipcConnection : public wxConnection
{
public:
    thFrame *frame;
    wxString cwd;
    char *swnd;
    ipcConnection(thFrame *frame) : frame(frame)
    {
        swnd = NULL;
    }

    virtual ~ipcConnection()
    {
        delete swnd;
    }

    virtual bool OnPoke(const wxString& topic, const wxString& item, TCHAR* data, int size, wxIPCFormat format)
    {
        PRINTF(_T("%s\n"), item.c_str());
        if (item == _T("dir"))
            cwd = wxString(data);
        else if (item == _T("cmd")) {
            frame->Raise(); // Bring T. Hex window the the foreground
            frame->ProcessCommandLine(wxString(data), cwd);
        }
        // Note: If frame puts up a message box, this ipcConnection object will go away.
        return true;
    }

    // Added 2008-07-25 because sometimes frame->Raise() isn't enough.
    // Steps to test:  Close the program.
    // Find 3 files in Explorer.  Right-click and "SendTo T-Hex" on each, without switching focus otherwise.
    virtual wxChar *OnRequest(const wxString& topic, const wxString& item, int *size, wxIPCFormat format )
    {
        if (item == _T("HWND"))
        {
            swnd = new char[20]; 
            *size = sprintf(swnd, "%X", frame->GetHWND()) + 1;  // include null terminator
            return (wxChar*)swnd;
        }
        return NULL;
    }
};

class ipcServer : public wxServer
{
public:
    thFrame *frame;
    ipcServer(thFrame *frame) : frame(frame)
    {
    }

    virtual wxConnectionBase * OnAcceptConnection(const wxString& topic)
    {
        return new ipcConnection(frame);
    }
};

#endif // _THIPC_H_
