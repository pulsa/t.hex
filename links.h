#pragma once

#ifndef _LINKS_H_

/*
CLnkFile lnk(filename); // calls IShellLink::Resolve()
if (lnk.isValidLink)
{
    if (user wants to open target)
        filename = strTarget;
}

or this:

CLnkFile lnk(filename); // doesn't call IShellLink::Resolve()
if (lnk.isValidLink)
{
    if (wxFile::Exists(lnk.strTarget))
    {
        if (user wants to open target)
            filename = strTarget;
    }
    else
    {
        if (user wants to resolve target)
        {
            wxString tmpName = lnk.Resolve(hwnd);
            if (user wants to open new target)
                filename = tmpName;
        }
    }
}

The question is whether you can call Resolve() to see whether the file exists
as originally intended, without searching other files or directories.
*/

//class IShellLink;
#include <shlobj.h>
//#include <initguid.h>

class CLnkFile
{
public:
    CLnkFile(wxString filename);
    virtual ~CLnkFile() { Close(); }

    HRESULT Resolve(HWND hWnd, DWORD flags); // update strTarget

    wxString strTarget, strError;
    bool isValidLink;
    //bool targetIsFile;
    WIN32_FIND_DATA wfd;

protected:
    IShellLink *psl;
    void Close();
};

#endif // _LINKS_H_
