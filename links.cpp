#include "precomp.h"
#include "links.h"

#define new New

#include <shlobj.h>
#include <initguid.h>

CLnkFile::CLnkFile(wxString filename)
{
    isValidLink = false;
    //targetIsFile = false;
    psl = NULL;

    HRESULT hres;
    TCHAR szFile[MAX_PATH];

    // First see if the file exists, is readable, and has data.
    //HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    //if (hFile == INVALID_HANDLE_VALUE)
    //{
    //    strError.Printf( _T("Couldn't 
    //}

    // Get pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

    if (SUCCEEDED(hres))
    {
        // Get pointer to the IPersistFile interface.

        IPersistFile *ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);

        if (SUCCEEDED(hres))
        {
            WCHAR wsz[MAX_PATH];

#ifdef _UNICODE
            _tcscpy(wsz, filename.c_str());
#else
            // Convert link path to Unicode for IPersistFile::Load().
            MultiByteToWideChar(CP_ACP, 0, filename.c_str(), -1, wsz, MAX_PATH);
#endif

            // Load the shell link
            hres = ppf->Load(wsz, STGM_READ);
            ppf->Release();
            if (SUCCEEDED(hres))
            {
                hres = psl->GetPath(szFile, MAX_PATH, &wfd, SLGP_UNCPRIORITY);
                if (hres == S_FALSE) // no path read; usually means file is not LNK format
                {
                    strError.Printf( _T("This doesn't look like a shortcut file."));
                    Close();
                    return;
                }
                else if (SUCCEEDED(hres))
                {
                    isValidLink = true;
                    strTarget = szFile;
                    //if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    //    targetIsFile = false;
                    //else
                    //    targetIsFile = true;
                }
                else
                    strError.Printf( _T("IShellLink::GetPath(SLGP_UNCPRIORITY) failed.  Code %08x"), hres);
            }
            if (FAILED(hres))
            {
                strError.Printf( _T("IPersistFile::Load() failed.  Code %08x"), hres);
                Close();
            }
        }
        else
        {
            strError.Printf( _T("IShellLink::QueryInterface(IID_IPersistFile) failed.  Code %08x"), hres);
            Close();
        }
    }
    else
    {
        strError.Printf( _T("CoCreateInstance Error - hres = %08x"), hres);
        psl = NULL;
    }
}

void CLnkFile::Close()
{
    if (psl)
        psl->Release();
    psl = NULL;
}

HRESULT CLnkFile::Resolve(HWND hWnd, DWORD flags) // update strTarget
{
    if (!psl)
        return E_FAIL;

    //hres = psl->Resolve(hWnd, SLR_ANY_MATCH);
    //hres = psl->Resolve(hWnd, SLR_NOUPDATE | SLR_NOSEARCH | SLR_NOTRACK | SLR_NOLINKINFO);

    HRESULT hres = psl->Resolve(hWnd, flags);

    if (SUCCEEDED(hres))
    {
        TCHAR szFile[MAX_PATH];
        hres = psl->GetPath(szFile, MAX_PATH, &wfd, SLGP_UNCPRIORITY);
        if (SUCCEEDED(hres))
            strTarget = szFile;
        else
            strError.Printf( _T("GetPath(SLGP_UNCPRIORITY) failed.  HRESULT = 0x%08x"), hres);
    }
    else
        strError.Printf( _T("IShellLink::Resolve(0x%X, 0x%X) failed.  HRESULT = 0x%08x"), hWnd, flags, hres);

    return hres;
}
