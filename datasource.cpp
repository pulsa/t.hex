#include "precomp.h"
#include "datasource.h"
#include "hexdoc.h"
//#include "sequence.h"

//#include "thex.h" //! for fatal() and WaitObjectList
//#include "undo.h"
//#include "settings.h"
#include "hexwnd.h"
#include "utils.h"

extern "C" {
#include "akrip32.h"
#include "scsipt.h"
}

#define new New

static THSIZE g_granularity_mask = 0;

//#define SetFilePointerEx SetFilePointerEx_Adam

//BOOL WINAPI SetFilePointerEx_Adam(
//    HANDLE hFile,
//    LARGE_INTEGER liDistanceToMove,
//    PLARGE_INTEGER lpNewFilePointer,
//    DWORD dwMoveMethod
//    )
//{
//   DWORD rc = SetFilePointer(hFile, liDistanceToMove.LowPart, &liDistanceToMove.HighPart, dwMoveMethod);
//   return !(rc == INVALID_SET_FILE_POINTER && liDistanceToMove.LowPart == INVALID_SET_FILE_POINTER);
//}

DataSource::DataSource()
{
    m_nRefs = 1;
    m_bOpen = false;
    m_bMemoryMapped = false;
    m_bCanChangeSize = false;
    m_bWriteable = false;
    //m_pRootRegion = NULL;

    MinTransfer = 1;
    MaxTransfer = 1048576;
    BlockAlign  = 4096;  // one memory page is a good size.
}

void DataSource::ShowProperties(HexWnd *hw)
{
   wxMessageBox(_T("Nothing to show here."), _T("Properties"), wxOK, hw);
}

FileDataSource::FileDataSource()
{
    m_bOpen = FALSE;
    hMapping = NULL;
    m_pData = NULL;
    m_bMemoryMapped = false;
    m_bCanChangeSize = m_bWriteable = false;
    hFile = INVALID_HANDLE_VALUE;
    tryMapping = false;
}

FileDataSource::FileDataSource(LPCTSTR filename, bool bReadOnly)
{
    //! todo: if file opened read-only, option to allow writes and update immediately

    if (g_granularity_mask == 0)
    {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        if (bitcount(sysinfo.dwAllocationGranularity) != 1)
            wxMessageBox(wxString::Format(_T("Huh? Allocation granularity is 0x%X."), sysinfo.dwAllocationGranularity));
        g_granularity_mask = sysinfo.dwAllocationGranularity - 1;
    }

    //m_pRootRegion = NULL;
    m_bOpen = FALSE;
    m_bWriteable = !bReadOnly;
    hFile = INVALID_HANDLE_VALUE;
    hMapping = NULL;
    m_pData = NULL;
    m_bMemoryMapped = false; //true;
    tryMapping = true;
	tryMapping = false;  //! speed testing, 2009-05-02

    wxFileName fn(filename);
    m_title = fn.GetFullName();  // strip off any "\\.\" stuff
    m_fullpath = fn.GetFullPath();

    if (m_bWriteable)
    {
        hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            m_bWriteable = false; // try opening read-only
    }

    if (!m_bWriteable)
    {
        DWORD dwShare = FILE_SHARE_READ;
        if (appSettings.bFileShareWrite)
            dwShare |= FILE_SHARE_WRITE;
        hFile = CreateFile(filename, GENERIC_READ,
           dwShare, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            wxString tmp;
            tmp.Printf(_T("Couldn't open \"%s\".\nCode %d."), filename, GetLastError());
            wxMessageBox(tmp);
            return;
        }
    }

    m_bCanChangeSize = m_bWriteable;

    fileSize.LowPart = GetFileSize(hFile, (DWORD*)&fileSize.HighPart);
    if (fileSize.LowPart == 0xFFFFFFFF && GetLastError() != 0)
    { // 0xFFFFFFFF is error code and valid file size.  Must check GetLastError too.
        wxString msg;
        msg.Printf(_T("GetFileSize() failed.  Code %d"), GetLastError());
        wxMessageBox(msg);
        return;
    }

    MemoryMap(0, wxMin(fileSize.QuadPart, 0x100000));

    HexDoc* doc = AddRegion(0, 0, fileSize.QuadPart, wxString(_T("File: ")) + filename);
    doc->dwFlags = PAGE_READWRITE; //! this isn't quite right for files

    m_bOpen = true;
}


FileDataSource::~FileDataSource()
{
    if (m_pData != NULL)
        UnmapViewOfFile(m_pData);
    if (hMapping != NULL)
        CloseHandle(hMapping);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
}

void FileDataSource::Flush()
{
    FlushFileBuffers(hFile);
}

bool FileDataSource::Read(uint64 nIndex, uint32 nSize, uint8 *pData)
{
    //! todo: use memory-mapped file here?  Beware, it's tricky...

    LARGE_INTEGER offset;
    offset.QuadPart = nIndex;
    SetFilePointer(hFile, offset.LowPart, &offset.HighPart, FILE_BEGIN);
    //! check return from SetFilePointer()
    //! todo: enforce 4GB limit on Win98?  or can we even try to open that file?
    return !!ReadFile(hFile, pData, nSize, &nSize, NULL);
}

bool FileDataSource::Write(uint64 nIndex, uint32 nSize, const uint8 *pData)
{
    if (nIndex >= m_mapStart && nIndex + nSize <= m_mapStart + m_mapSize)
    {
        memcpy(m_pData + nIndex - m_mapStart, pData, nSize);
        return true;
    }

    LARGE_INTEGER offset;
    offset.QuadPart = nIndex;
    SetFilePointer(hFile, offset.LowPart, &offset.HighPart, FILE_BEGIN);
    //! check return from SetFilePointer()
    //! todo: enforce 4GB limit on Win98?  or can we even try to open that file?
    return !!WriteFile(hFile, pData, nSize, &nSize, NULL);
}

bool FileDataSource::MemoryMap(THSIZE nIndex, THSIZE nSize)
{
    m_mapSize = 0; // clear this first
    if (!tryMapping)
        return false;

    if (nIndex + nSize > (THSIZE)fileSize.QuadPart)
        return false;

    // Round nIndex downward to system-dependent boundary to avoid ERROR_MAPPED_ALIGNMENT.
    // nSize doesn't matter so much; the caller can ask for more if they want it.
    //! We could maybe improve performance just a hair by increasing nSize a few kB... Test this.
    nSize += nIndex & g_granularity_mask;
    nIndex &= ~(THSIZE)g_granularity_mask;

    if (hMapping == NULL)
        hMapping = CreateFileMapping(hFile, NULL, m_bWriteable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL)
    {
        tryMapping = false;
        m_pData = NULL;
        wxString tmp;
        tmp.Printf(_T("CreateFileMapping() failed.\nCode %d."), GetLastError());
        wxMessageBox(tmp);
        return false;
    }

    if (m_pData)
        UnmapViewOfFile(m_pData);
    m_pData = (uint8*)MapViewOfFile(hMapping,
        m_bWriteable ? FILE_MAP_WRITE : FILE_MAP_READ,
        DWORD(nIndex >> 32), DWORD(nIndex), nSize);
    if (m_pData == NULL)
    {
        tryMapping = false;
        wxString tmp;
        tmp.Printf(_T("MapViewOfFile() failed.\nCode %d."), GetLastError());
        wxMessageBox(tmp);
        return false;
    }
    // if m_pData is NULL, we will use ReadFile() instead.
    m_mapStart = nIndex;
    m_mapSize = nSize;
    return true;
}

const uint8* FileDataSource::GetBasePointer(THSIZE nIndex, THSIZE nSize)
{
    if (nIndex >= m_mapStart && nIndex + nSize <= m_mapStart + m_mapSize)
        return m_pData + nIndex - m_mapStart;
    if (!MemoryMap(nIndex, nSize))
        return NULL;
    return m_pData + nIndex - m_mapStart;
}

void FileDataSource::ShowProperties(HexWnd *hw)
{
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpFile = m_fullpath;
    sei.lpVerb = _T("properties");
    sei.fMask  = SEE_MASK_INVOKEIDLIST;
    sei.hwnd = (HWND)hw->GetHWND();
    ShellExecuteEx(&sei);
}

bool FileDataSource::SetEOF(THSIZE nIndex)
{
    if (nIndex == (THSIZE)fileSize.QuadPart)
        return true;

    // Unmap views and delete mapping objects before changing file size.
    m_mapSize = 0;
    if (m_pData)
        UnmapViewOfFile(m_pData);
    m_pData = NULL;
    if (hMapping != NULL)
        CloseHandle(hMapping);
    hMapping = NULL;

    LARGE_INTEGER pos;
    pos.QuadPart = nIndex;
    if (!SetFilePointerEx(hFile, pos, NULL, FILE_BEGIN))
        return false;
    return !!SetEndOfFile(hFile);
}

///////////////////////////////////////////////////////////////////////////////

DWORD MSB2DWORD(const BYTE* b)
{
    DWORD retVal = b[0];
    retVal = (retVal<<8) + (DWORD)b[1];
    retVal = (retVal<<8) + (DWORD)b[2];
    retVal = (retVal<<8) + (DWORD)b[3];
    return retVal;
}

DiskDataSource::DiskDataSource(LPCTSTR filename, bool bReadOnly)
{
    //DWORD errcode;
    wxString errmsg;

    //m_pRootRegion = NULL;
    m_bOpen = FALSE;
    m_bWriteable = !bReadOnly;
    hDisk = INVALID_HANDLE_VALUE;
    m_bMemoryMapped = false;
    m_track = NULL;
    m_bIsAudioCD = false;
    m_pData = NULL;

    wxFileName fn(filename);
    m_title = fn.GetFullName();  // strip off any "\\.\" stuff

    if (m_bWriteable)
    {
        hDisk = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);
        if (hDisk == INVALID_HANDLE_VALUE)
            m_bWriteable = false; // try opening read-only
    }

    if (!m_bWriteable)
        hDisk = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if (hDisk == INVALID_HANDLE_VALUE)
    {
        errmsg.Printf(_T("Couldn't open \"%s\".\nCode %d."), filename, GetLastError());
        wxMessageBox(errmsg);
        return;
    }

    m_bCanChangeSize = false;

    DWORD cBytes;
    BOOL bResult;

#if _WIN32_WINNT >= 0x0501
    GET_LENGTH_INFORMATION lengthInfo; //! requires XP or later
    bResult = DeviceIoControl(hDisk,  // device to be queried
      IOCTL_DISK_GET_LENGTH_INFO,  // operation to perform
                             NULL, 0, // no input buffer
                            &lengthInfo, sizeof(lengthInfo),     // output buffer
                            &cBytes,                 // # bytes returned
                            (LPOVERLAPPED) NULL);  // synchronous I/O


    if (!bResult)
    {
        errmsg.Printf(_T("IOCTL_DISK_GET_LENGTH_INFO failed.  Code %d"), GetLastError());
        wxMessageBox(errmsg);
        return;
    }
#endif

    DISK_GEOMETRY g;
    bResult = DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL, 0, &g, sizeof(DISK_GEOMETRY), &cBytes, 0);
    if (!bResult)
    {
        errmsg.Printf(_T("IOCTL_DISK_GET_DRIVE_GEOMETRY failed.  Code %d"), GetLastError());
        wxMessageBox(errmsg);
        return;
    }

    MinTransfer = g.BytesPerSector;
    m_pData = new uint8[MinTransfer];
    curOffset = (THSIZE)-1;

    // Instruct the file system driver not to perform bounds checking.
    // The hardware device driver will do it.
    // This works on my "Disk" devices, but not my CD drives.
    // Does it have any effect?  I don't know.
    bResult = DeviceIoControl(hDisk, FSCTL_ALLOW_EXTENDED_DASD_IO,
        NULL, 0, NULL, 0, &cBytes, 0);
    //if (!bResult)
    //{
    //    errcode = GetLastError();
    //    errmsg.Printf(_T("FSCTL_ALLOW_EXTENDED_DASD_IO failed.  Code %d"), errcode);
    //    PRINTF(_T("%s\n"), errmsg.c_str());
    //    if (errcode != ERROR_INVALID_FUNCTION)
    //        wxMessageBox(errmsg);
    //    //return;  // We can probably run anyway.
    //}

    // if audio CD, use akrip32
    m_bIsAudioCD = false;

    if (!ReadSector(0))
    {
        // See if we can find akrip.dll
        HMODULE hMod = LoadLibrary(_T("akrip"));
        if (hMod == NULL)
        {
            wxMessageBox(_T("Couldn't load akrip.dll"));
            return;
        }
        // We don't need to call FreeLibrary.  Its reference count will never go to zero anyway.

        m_bIsAudioCD = true;
        // loadAspi(); // already called from DllMain() in aspilib.c
        SPT_DRIVE drive;
        SPT_GetDriveInformation(fn.GetFullName().Upper()[0] - 'A', &drive);

        GETCDHAND cdh;
        ZeroMemory( &cdh, sizeof(cdh) );
        cdh.size     = sizeof(GETCDHAND);
        cdh.ver      = 1;
        cdh.ha       = drive.ha;
        cdh.tgt      = drive.tgt;
        cdh.lun      = drive.lun;
        cdh.readType  = CDR_ANY;      // set for autodetect

        m_hCD = GetCDHandle( &cdh );

        //LARGE_INTEGER capacity;
        //GetCDCapacity(drive.ha, drive.tgt, drive.lun, &capacity);
        // returns same as IOCTL_DISK_GET_LENGTH_INFO

        delete [] m_pData;
        LPTRACKBUF t = (LPTRACKBUF)malloc(2352 + TRACKBUFEXTRA);
        if (t)
        {
            t->startFrame = 0;
            t->numFrames = 0;
            t->maxLen = 2352;
            t->len = 0;
            t->status = 0;
            t->startOffset = 0;
            m_pData = (uint8*)t->buf;
        }
        m_track = t;

        CloseHandle(hDisk);
        hDisk = INVALID_HANDLE_VALUE;

        MinTransfer = 2352;
        curOffset = (THSIZE)-1;

        //! This bit of code requires a lot of faith in the CD's table of contents.
        TOC toc;
        DWORD dw = ReadTOC(m_hCD, &toc);
        for (int track = toc.firstTrack; track < toc.lastTrack; track++)
        {
            THSIZE start = MSB2DWORD(toc.tracks[track - 1].addr);
            THSIZE len = MSB2DWORD(toc.tracks[track].addr) - start;
            start *= MinTransfer;
            len *= MinTransfer;
            HexDoc *doc = AddRegion(start, start, len, wxString::Format(_T("Track %02d"), track));
            doc->dwFlags = PAGE_READONLY;
        }
    }
    else
    {
        HexDoc* doc = AddRegion(0, 0, lengthInfo.Length.QuadPart, wxString(_T("Disk: ")) + filename);
        doc->dwFlags = PAGE_READONLY; //! this isn't quite right
    }

    m_bOpen = true; // if we didn't bail by now, everything's probably OK
}


DiskDataSource::~DiskDataSource()
{
    if (m_bIsAudioCD)
    {
        CloseCDHandle(m_hCD);
        free(m_track);
        m_pData = NULL; // had pointed to LPTRACKBUF m_track
    }
    if (hDisk != INVALID_HANDLE_VALUE)
        CloseHandle(hDisk);
    delete [] m_pData;
}

bool DiskDataSource::ReadSector(THSIZE nIndex)
{
    THSIZE newOffset = RoundDown(nIndex, MinTransfer);
    if (newOffset != curOffset)
    {
        if (m_bIsAudioCD)
        {
            LPTRACKBUF t = (LPTRACKBUF)m_track;
            t->numFrames = 1;
            t->startFrame = newOffset / MinTransfer;
            t->startOffset = 0;
            t->len = 0;
            curOffset = newOffset;
            DWORD dw = ReadCDAudioLBA(m_hCD, t);
            if (dw != SS_COMP)
            {
                dw = dw;
                printf( "Error (%d:%d)\n", GetAspiLibError(), GetAspiLibAspiError() );
                return false;
            }
        }
        else
        {
            LARGE_INTEGER offset;
            DWORD dw;
            offset.QuadPart = newOffset;
            BOOL r = SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN);
            if (!r)
            {
                DWORD err = GetLastError();
                err = err;
            }
            curOffset = newOffset;
            r = ReadFile(hDisk, m_pData, MinTransfer, &dw, NULL);
            if (!r || dw < MinTransfer)
                return false;
        }
    }
    return true;
}

bool DiskDataSource::Read(uint64 nIndex, uint32 nSize, uint8 *pData)
{
    if (m_bIsAudioCD || (nIndex % MinTransfer) || (nSize % MinTransfer))
    {
        uint32 dst, src = nIndex % MinTransfer;
        for (dst = 0; dst < nSize; )
        {
            if (!ReadSector(nIndex + dst))
                return false;
            uint32 blockSize = wxMin(nSize - dst, MinTransfer);
            if (src != 0) // only happens on first pass
                blockSize = wxMin(MinTransfer - src, nSize);
            memcpy(pData + dst, m_pData + src, blockSize);
            src = 0;
            dst += blockSize;
        }
    }
    else
    {
        LARGE_INTEGER offset, actual;
        offset.QuadPart = nIndex;
        if (!SetFilePointerEx(hDisk, offset, &actual, FILE_BEGIN)) {
            PRINTF(_T("SetFilePointerEx(%I64X) failed.  Code %d\n"), nIndex, GetLastError());
            memset(pData, 0xFF, nSize);
            return false;
        }
        DWORD cBytes;
        if (!ReadFile(hDisk, pData, nSize, &cBytes, NULL)) {
            PRINTF(_T("ReadFile(%p, %X) failed.  Code %d\n"), pData, nSize, GetLastError());
            memset(pData, 0xFF, nSize);
            return false;
        }
        if (cBytes != nSize)
        {
            PRINTF(_T("Read %X of %X bytes at %I64X.  Code %d\n"), cBytes, nSize, nIndex, GetLastError());
            if (cBytes < nSize)
                memset(pData + cBytes, 0xFF, nSize - cBytes);
            return false;
        }
        offset.QuadPart = 0;
        if (!SetFilePointerEx(hDisk, offset, &actual, FILE_CURRENT) ||
            (THSIZE)actual.QuadPart != nIndex + nSize)
        {
           cBytes = actual.QuadPart - (nIndex + nSize);
        }
        cBytes = cBytes;
    }
    return true;
}

bool DiskDataSource::Write(uint64 nIndex, uint32 nSize, const uint8 *pData)
{
    LARGE_INTEGER offset;
    DWORD cBytes;
    uint32 blockSize;

    if (m_bIsAudioCD)
        return false; // We can't write to CDs.

    if ((nIndex % MinTransfer) || (nSize < MinTransfer)) // R-M-W the first sector
    {
        uint32 src = nIndex % MinTransfer;
        if (!ReadSector(nIndex))
           return false;
        blockSize = wxMin(nSize, MinTransfer - src);
        memcpy(m_pData + src, pData, blockSize);
        offset.QuadPart = nIndex - src;
        if (!SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN) ||
            !WriteFile(hDisk, m_pData, MinTransfer, &cBytes, 0))
        {
           PRINTF(_T("DiskDataSource::Write() line %d, code %d\n"), __LINE__, GetLastError());
           return false;
        }
        nIndex += blockSize;
        nSize -= blockSize;
        pData += blockSize;
    }

    blockSize = wxMin(nSize, RoundDown(MEGA, MinTransfer));

    // Do the main sector-aligned write.
    while (nSize >= MinTransfer && nSize > 0) //! something is funny about these conditions (maybe fixed now?)
    {
        offset.QuadPart = nIndex;
        if (!SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN) ||
            !WriteFile(hDisk, pData, blockSize, &cBytes, 0))
        {
           PRINTF(_T("DiskDataSource::Write() line %d, code %d\n"), __LINE__, GetLastError());
           return false;
        }

        nIndex += blockSize;
        nSize -= blockSize;
        pData += blockSize;
    }

    if (nSize) // R-M-W the last sector
    {
        if (!ReadSector(nIndex))
           return false;
        memcpy(m_pData, pData, nSize);
        offset.QuadPart = nIndex;
        if (!SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN) ||
            !WriteFile(hDisk, m_pData, MinTransfer, &cBytes, 0))
        {
           PRINTF(_T("DiskDataSource::Write() line %d, code %d\n"), __LINE__, GetLastError());
           return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

void CALLBACK OnProcessExit(HANDLE WXUNUSED(handle), DWORD_PTR WXUNUSED(dwUser))
{
    fatal(_T("Process went away."));
}

#if 0
#include <wx/dynlib.h>
#include <wx/msw/debughlp.h>

class WXDLLIMPEXP_BASE thDynamicLibraryDetails
{
public:
    // ctor, normally never used as these objects are only created by
    // thDynamicLibrary::ListLoaded()
    thDynamicLibraryDetails() { m_address = NULL; m_length = 0; }

    // get the (base) name
    wxString GetName() const { return m_name; }

    // get the full path of this object
    wxString GetPath() const { return m_path; }

    // get the load address and the extent, return true if this information is
    // available
    bool GetAddress(void **addr, size_t *len) const
    {
        if ( !m_address )
            return false;

        if ( addr )
            *addr = m_address;
        if ( len )
            *len = m_length;

        return true;
    }

    // return the version of the DLL (may be empty if no version info)
    wxString GetVersion() const
    {
        return m_version;
    }

//private:
    wxString m_name,
             m_path,
             m_version;

    void *m_address;
    size_t m_length;

    //friend class wxDynamicLibraryDetailsCreator;
};

// return the module handle for the given base name
static
HMODULE thGetModuleHandle(const char *name, void *addr)
{
    // we want to use GetModuleHandleEx() instead of usual GetModuleHandle()
    // because the former works correctly for comctl32.dll while the latter
    // returns NULL when comctl32.dll version 6 is used under XP (note that
    // GetModuleHandleEx() is only available under XP and later, coincidence?)

    // check if we can use GetModuleHandleEx
    typedef BOOL (WINAPI *GetModuleHandleEx_t)(DWORD, LPCSTR, HMODULE *);

    static const GetModuleHandleEx_t INVALID_FUNC_PTR = (GetModuleHandleEx_t)-1;

    static GetModuleHandleEx_t s_pfnGetModuleHandleEx = INVALID_FUNC_PTR;
    if ( s_pfnGetModuleHandleEx == INVALID_FUNC_PTR )
    {
        wxDynamicLibrary dll(_T("kernel32.dll"), wxDL_VERBATIM);
        s_pfnGetModuleHandleEx =
            (GetModuleHandleEx_t)dll.RawGetSymbol(_T("GetModuleHandleExA"));

        // dll object can be destroyed, kernel32.dll won't be unloaded anyhow
    }

    // get module handle from its address
    if ( s_pfnGetModuleHandleEx )
    {
        // flags are GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
        //           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        HMODULE hmod;
        if ( s_pfnGetModuleHandleEx(6, (char *)addr, &hmod) && hmod )
            return hmod;
    }

    // Windows CE only has Unicode API, so even we have an ANSI string here, we
    // still need to use GetModuleHandleW() there and so do it everywhere to
    // avoid #ifdefs -- this code is not performance-critical anyhow...
    return ::GetModuleHandle(wxString::FromAscii((char *)name));
}

WX_DECLARE_OBJARRAY(thDynamicLibraryDetails, thDynamicLibraryDetailsArray);
#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(thDynamicLibraryDetailsArray);

BOOL CALLBACK
EnumModulesProc(PTSTR name, DWORD64 base, ULONG size, void *data)
{
    //EnumModulesProcParams *params = (EnumModulesProcParams *)data;
    thDynamicLibraryDetailsArray *dlls = (thDynamicLibraryDetailsArray*)data;

    thDynamicLibraryDetails *details = new thDynamicLibraryDetails;

    // fill in simple properties
    details->m_name = wxString::FromAscii(name);
    details->m_address = wx_reinterpret_cast(void *, base);
    details->m_length = size;

    // to get the version, we first need the full path
    HMODULE hmod = thGetModuleHandle(name, (void *)base);
    if ( hmod )
    {
        wxString fullname = wxGetFullModuleName(hmod);
        if ( !fullname.empty() )
        {
            details->m_path = fullname;
            //details->m_version = params->verDLL->GetFileVersion(fullname);
        }
    }

    //params->dlls->Add(details);
    dlls->Add(details);

    // continue enumeration (returning FALSE would have stopped it)
    return TRUE;
}

thDynamicLibraryDetailsArray wxEnumLoadedModules(HANDLE hProcess)
{
    thDynamicLibraryDetailsArray dlls;

    if ( !EnumerateLoadedModules64( hProcess, EnumModulesProc, &dlls))
    {
        wxLogLastError(_T("EnumerateLoadedModules"));
    }

    return dlls;
}
#endif // 0

ProcMemDataSource::ProcMemDataSource(DWORD pid, wxString procName, bool bReadOnly)
{
    //m_pRootRegion = NULL; //! can this go in parent constructor?
    m_bWriteable = !bReadOnly;

    m_title = procName + wxString::Format(_T(" (PID %d)"), pid);

    PRINTF(_T("EnableDebugPriv %s.\n"), EnableDebugPriv() ? _T("succeeded") : _T("failed"));

    IsThisProc = (pid == GetCurrentProcessId());
    if (IsThisProc)
        hProcess = GetCurrentProcess();
    else
    {
        DWORD access = PROCESS_VM_READ | SYNCHRONIZE;
        if (!bReadOnly)
            access |= PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
        access |= PROCESS_QUERY_INFORMATION; // so we can use VirtualQueryEx()
        hProcess = ::OpenProcess(access, 0, pid);
        if (hProcess == NULL)
        {
            wxString tmp;
            tmp.Printf(_T("OpenProcess(%d) failed.\nCode %d."), pid, GetLastError());
            wxMessageBox(tmp);
            return;
        }
    }
    //!g_wol.Add(hProcess, OnProcessExit, 0);

    //wxEnumLoadedModules(hProcess);

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPHEAPLIST, pid);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
       wxMessageBox(wxString::Format(_T("CreateToolhelp32Snapshot() failed.\nCode %d."), GetLastError()));
       return;
    }

    // enumerate modules
    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    if (Module32First(hSnap, &me))
    {
        do {
            wxString tmp = _T("Module: ") + GetRealFilePath(me.szExePath, hProcess, me.modBaseAddr);
            THSIZE base = (THSIZE)me.modBaseAddr;
            HexDoc* doc = AddRegion(base, base, me.modBaseSize, tmp, false, PAGE_READONLY);
        } while (Module32Next(hSnap, &me));
    }
    else if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        int code = GetLastError();
        wxString msg = _T("Module32First() failed. ") + GetLastErrorMsg(code);
        PRINTF(_T("%s"), msg.c_str());
        wxMessageBox(msg);
        return;
    }


    // Enumerate heaps.
    // This could use some work -- how do you get the size of a heap without
    // enumerating all of its allocated blocks?
    HEAPLIST32 hl;
    hl.dwSize = sizeof(hl);
    if (!Heap32ListFirst(hSnap, &hl))
    {
        int code = GetLastError();
        wxString msg = _T("Heap32ListFirst() failed. ") + GetLastErrorMsg(code);
        PRINTF(_T("%s"), msg.c_str());
    }
    else
    {
        do {
            wxString tmp;
            wxString type;
            LPCVOID baseAddr;

            HEAPENTRY32 he = { sizeof(he) };
            if (Heap32First(&he, pid, hl.th32HeapID))
            {
                if      (he.dwFlags == LF32_FIXED)    type = _T("fixed");
                else if (he.dwFlags == LF32_FREE)     type = _T("free");
                else if (he.dwFlags == LF32_MOVEABLE) type = _T("moveable");
                else                                  type.Printf(_T("%d"), he.dwFlags);
                tmp.Printf(_T("Heap (%s), handle 0x%X, flags 0x%X"), type, (DWORD)he.hHandle, (DWORD)hl.dwFlags);  //what's wrong with %x HANDLE?
                baseAddr = (LPCVOID)he.dwAddress;
            }
            else
            {
                int code = GetLastError();
                wxString msg = wxString::Format(_T("Heap32First(0x%08X) failed. "), hl.th32HeapID) + GetLastErrorMsg(code);
                PRINTF(_T("%s"), msg.c_str());
                //continue;

                // Put something in the list anyway.  This is probably good enough.  Only observed in VCExpress.exe.
                tmp.Printf(_T("Heap (strange), handle 0x%X, flags 0x%X"), hl.th32HeapID, hl.dwFlags);
                baseAddr = (LPCVOID)hl.th32HeapID;
            }

            MEMORY_BASIC_INFORMATION memInfo;
            VirtualQueryEx(hProcess, baseAddr, &memInfo, sizeof(memInfo));
            THSIZE base = (THSIZE)memInfo.BaseAddress;

            bool bWriteable = true;
            if (!(memInfo.Protect & PAGE_READWRITE) &&
                !(memInfo.Protect & PAGE_EXECUTE_READWRITE))
               bWriteable = false;
            HexDoc* doc = AddRegion(base, base, memInfo.RegionSize, tmp, bWriteable);

        } while (Heap32ListNext(hSnap, &hl));
    }

    CloseHandle(hSnap); // done enumerating stuff from the ToolHelp32 snapshot.

    // Experimental code, trying to find process's stack space.
    //! Now I know how to do this better... use AllocationBase and make 3 calls.
    //! But we're not going to, because it's weird and the list of all blocks is better.
    //if (pid == GetCurrentProcessId())
    //{
    //    MEMORY_BASIC_INFORMATION memInfo;
    //    VirtualQuery(&memInfo, &memInfo, sizeof(memInfo));
    //    THSIZE begin, end, reserve_begin, guard_begin;
    //    if (memInfo.State != MEM_COMMIT)
    //       fatal("Can't find process stack.");
    //    end = (THSIZE)memInfo.BaseAddress + memInfo.RegionSize;
    //    reserve_begin = (THSIZE)memInfo.AllocationBase;

    //    // search backward for guard page
    //    while (memInfo.State == MEM_COMMIT)
    //    {
    //       begin = (THSIZE)memInfo.BaseAddress;
    //       VirtualQuery((LPCVOID)(begin - 1), &memInfo, sizeof(memInfo));
    //    }
    //    guard_begin = (THSIZE)memInfo.BaseAddress;

    //    // search forward for non-committed page
    //    VirtualQuery(&memInfo, &memInfo, sizeof(memInfo));
    //    while (memInfo.State == MEM_COMMIT)
    //    {
    //       end = (THSIZE)memInfo.BaseAddress + memInfo.RegionSize;
    //       VirtualQuery((LPCVOID)end, &memInfo, sizeof(memInfo));
    //    }
    //    
    //    AddRegion(begin, begin, end - begin, "Stack");

    //    VirtualQuery((LPCVOID)reserve_begin, &memInfo, sizeof(memInfo)); // for debugging
    //    VirtualQuery((LPCVOID)end, &memInfo, sizeof(memInfo)); // for debugging        

    //    AddRegion(reserve_begin, reserve_begin, guard_begin - reserve_begin, "Reserved stack");
    //    AddRegion(guard_begin, guard_begin, begin - guard_begin, "Stack guard");
    //}

    // More experimental code.
    // Enumerate modules using PSAPI and see if there are any that didn't show up before.
    // Result: It didn't find any.
    //{
    //    DWORD cbNeeded = 0, modules = 0;
    //    HMODULE* hMods = NULL;
    //    EnumProcessModules(hProcess, NULL, 0, &cbNeeded);
    //    modules = cbNeeded / sizeof(HMODULE);
    //    hMods = new HMODULE[modules];
    //    EnumProcessModules(hProcess, hMods, cbNeeded, &cbNeeded);
    //    for (size_t i = 0; i < modules; i++)
    //    {
    //        MODULEINFO modInfo;
    //        GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo));
    //        THSIZE base = (THSIZE)modInfo.lpBaseOfDll;
    //        if (GetDocAt(base))
    //           continue; // already got this one

    //        char szModName[MAX_PATH];

    //        // Get the full path to the module's file.

    //        if ( GetModuleFileNameEx( hProcess, hMods[i], szModName, sizeof(szModName)))
    //        {
    //            AddRegion(base, base, modInfo.SizeOfImage, szModName);
    //        }
    //    }
    //    delete [] hMods;
    //}

    // More experimental code.
    // Enumerate all virtual memory regions and add anything we didn't get already.
    {
        THSIZE begin = 0, size = 0;
        MEMORYSTATUS memStatus = { sizeof(memStatus) };
        GlobalMemoryStatus(&memStatus);

        MEMORY_BASIC_INFORMATION memInfo;

        while (1)
        {
            if (!VirtualQueryEx(hProcess, (LPCVOID)begin, &memInfo, sizeof(memInfo)))
            {
               PRINTF(_T("VirtualQueryEx() failed.  Error code %d.\n"), GetLastError());
               break;
            }

            // VirtualQueryEx() combines contiguous memory blocks of the same type,
            // so we don't have to worry about it.
            // Except when it doesn't.  Weird.
            begin = (THSIZE)memInfo.BaseAddress;
            size = memInfo.RegionSize;

            if (size && memInfo.State == MEM_COMMIT)
            {
                wxString type = _T("Data");
                HexDoc *olddoc = GetDocAt(begin);
                if (olddoc && olddoc->info.StartsWith(_T("Module:")))
                {
                    //! To do: Use wxTreeListCtrl or something to group these with the module entry.
#if 1  // section-level view of modules.  Makes region list slow.
                    type = _T("- ") + wxFileName(olddoc->info).GetFullName();
                    HexDoc* doc = AddRegion(begin, begin, size, type,
                        (memInfo.Protect & PAGE_READWRITE) != 0,
                        memInfo.Protect);
#endif
                }
                else if (olddoc && begin == olddoc->display_address && size == olddoc->GetSize() &&
                    olddoc->dwFlags == 0 && olddoc->info.StartsWith(_T("Heap")))
                {
                    olddoc->dwFlags = memInfo.Protect;
                }
                else if (olddoc)
                    wxMessageBox(_T("ProcMemDataSource weirdness."), _T("T. Hex"));  //! this shouldn't happen
                else
                {
                    TCHAR devicepath[MAX_PATH];
                    DWORD cBytes = GetMappedFileName(hProcess, (LPVOID)begin, devicepath, MAX_PATH);
                    if (cBytes)
                    {
                        LPCTSTR rest;
                        TCHAR driveLetter = GetDriveLetter(devicepath, &rest);
                        if (driveLetter)
                            type.Printf(_T("Mapped file: %c:%s"), driveLetter, rest);
                        else
                            type.Printf(_T("Mapped file: %s"), devicepath);
                    }

                    HexDoc* doc = AddRegion(begin, begin, size, type,
                        (memInfo.Protect & PAGE_READWRITE) != 0,
                        memInfo.Protect);
                }
            }

            begin += size;
            if (begin >= memStatus.dwTotalVirtual)
                break;
        }
        //VirtualQueryEx();
    }

    if (docs.size())
        m_bOpen = true;
    else
        wxMessageBox(_T("Couldn't load anything"));
}

ProcMemDataSource::~ProcMemDataSource()
{
    CloseHandle(hProcess);
}

// This is kinda neat, but causes a program crash when a memory block goes away.
//const uint8* ProcMemDataSource::GetBasePointer(THSIZE nIndex, THSIZE nSize)
//{
//    return (IsThisProc) ? (uint8*)nIndex : NULL;
//}

bool ProcMemDataSource::Read(uint64 nIndex, uint32 nSize, uint8 *pData)
{
    // Works for the current process.
    return !!ReadProcessMemory(hProcess, (void*)nIndex, pData, nSize, NULL);
}

bool ProcMemDataSource::Write(uint64 nIndex, uint32 nSize, const uint8 *pData)
{
    return !!WriteProcessMemory(hProcess, (void*)nIndex, pData, nSize, NULL);
}

void ProcMemDataSource::ShowProperties(HexWnd *hw)
{
    THSIZE totalSize = 0;
    int nModules = 0, nHeaps = 0, nMappedFiles = 0;
    for (size_t n = 0; n < docs.size(); n++)
    {
        HexDoc *doc = docs[n];
        totalSize += doc->GetSize();

        if (doc->info.StartsWith(_T("Module")))
            nModules++;
        else if (doc->info.StartsWith(_T("Heap")))
            nHeaps++;
        else if (doc->info.StartsWith(_T("Mapped file")))
            nMappedFiles++;
    }
    PRINTF(_T("%d regions;\n  %d modules\n  %d heaps\n  %d mapped files\n"),
        docs.size(), nModules, nHeaps, nMappedFiles);
    PRINTF(_T("Total size of all regions: %s\n"), FormatBytes(totalSize));
}

//*************************************************************************************************

int DataSource::GetSerializedLength()
{
    return sizeof(DataSource*); //! quick hack
}

void DataSource::Serialize(uint8 *target)
{
    *(DataSource**)target = this; //! quick hack
}

HexDoc* DataSource::AddRegion(uint64 display_address, uint64 stored_address, uint64 size, wxString info,
    bool bWriteable /*= true*/, DWORD dwFlags /*= 0*/)
{
    //! needs help again after name change
    HexDoc *region = new HexDoc(this, display_address, size, 0);
    region->info = info;
    region->bWriteable = bWriteable && this->IsWriteable();
    region->bCanChangeSize = this->CanChangeSize();
    region->dwFlags = dwFlags;

    // Build vector of regions, ordered by starting address.
    //! todo: look for region overlap and data > MAX_ADDRESS
#if 1
    size_t n = 0, nDocs = docs.size();
    for (n = 0; n < nDocs; n++)
    {
        if (docs[n]->display_address > display_address)
            break;
    }
#else
    int lo = 0, mid = 0, hi = docs.size();
    while (hi > lo)
    {
        mid = (hi + lo) / 2;
        HexDoc *doc = docs[mid];
        if (display_address < doc->display_address)
            hi = mid;
        else if (display_address >= doc->display_address + doc->GetSize())
            lo = mid + 1;
        else // There is already a region at the specified address.
            break;  // shouldn't happen.
    }
    size_t n = lo;
#endif
    docs.insert(docs.begin() + n, region);

    return region;
}

HexDoc* DataSource::GetDocAt(THSIZE address)
{
#if 1
    // In Debug build, using iterators is much slower than an integer index.
    // In Release build, I notice no difference.  (They're both fast.)
    for (size_t n = 0; n < docs.size(); n++)
    {
        HexDoc *doc = docs[n];
        if (doc->display_address <= address && doc->display_address + doc->GetSize() > address)
            return doc;
    }
#else
    // Since the array is sorted, we can do this much faster with a binary search.
    // Yes, it matters -- when you open Firefox's memory, for example.
    //! Except we need to account for overlap.  Blah.
    int lo = 0, mid = 0, hi = docs.size();
    while (hi > lo)
    {
        mid = (hi + lo) / 2;
        HexDoc *doc = docs[mid];
        if (address < doc->display_address)
            hi = mid;
        else if (address >= doc->display_address + doc->GetSize())
            lo = mid + 1;
        else // address >= doc->display_address && addres < doc->display_address + doc->GetSize()
            return doc;
    }
#endif

    return NULL;
}

//*************************************************************************************************

int UnsavedDataSource::docNumber = 0;

UnsavedDataSource::UnsavedDataSource()
{
    m_title.Printf(_T("Untitled %d"), ++docNumber);
    m_bWriteable = true;
    m_bCanChangeSize = true;
    m_bOpen = true;

    HexDoc *doc = AddRegion(0, 0, 0, _T("New document"));
    doc->dwFlags = PAGE_READWRITE;
}

bool UnsavedDataSource::Read(uint64 nIndex, uint32 nSize, uint8 *pData)
{
    return false; // we have nothing to read from except the Segment objects.
}

//void DataSource::CleanUp()
//{
//    for (size_t n = 0; n < docs.size(); n++)
//        delete docs[n];
//    docs.clear();
//}
//
//HexDoc *DataSource::GetDoc(size_t index)
//{
//    if (index <= docs.size())
//        return docs[index];
//    return NULL;
//}
//

///////////////////////////////////////////////////////////////////////////////

//DSModifyBuffer::DSModifyBuffer(size_t size /*= 10000*/)
//{
//    buffer = new uint8[size];
//    used = 0;
//    total = size;
//}
//
//bool DSModifyBuffer::Read(uint64 nIndex, uint32 nSize, uint8 *pData)
//{
//    memcpy(pData, buffer + nIndex, nSize);
//    return true;
//}
//
//bool DSModifyBuffer::Write(uint64 nIndex, uint32 nSize, const uint8 *pData)
//{
//    return false;
//}



ProcessFileDataSource::ProcessFileDataSource(DWORD procID, DWORD hForeignFile)
: FileDataSource()
{
    wxString msg;
    HANDLE hProc = OpenProcess(PROCESS_DUP_HANDLE, TRUE, procID);
    if (!hProc)
    {
        msg.Printf(_T("OpenProcess() failed.  Code %d"), GetLastError());
        wxMessageBox(msg);
        return;
    }
    BOOL bResult = DuplicateHandle(hProc, (HANDLE)hForeignFile, GetCurrentProcess(), &hFile, GENERIC_READ, TRUE, 0);
    if (!bResult)
    {
        msg.Printf(_T("DuplicateHandle() failed.  Code %d"), GetLastError());
        wxMessageBox(msg);
        return;
    }

    fileSize.LowPart = GetFileSize(hFile, (DWORD*)&fileSize.HighPart);
    if (fileSize.LowPart == 0xFFFFFFFF && GetLastError() != 0)
    { // 0xFFFFFFFF is error code and valid file size.  Must check GetLastError too.
        msg.Printf(_T("GetFileSize() failed.  Code %d"), GetLastError());
        wxMessageBox(msg);
        return;
    }

    wxFileName fn(GetFileNameFromHandle(hFile));
    m_title = fn.GetFullName();  // remove path
    m_fullpath = fn.GetFullPath();

    HexDoc* doc = AddRegion(0, 0, fileSize.QuadPart, m_title);
    doc->dwFlags = PAGE_READONLY; //! this isn't quite right for files

    m_bOpen = true;
}

bool ProcessFileDataSource::Read(uint64 nIndex, uint32 nSize, uint8 *pData)
{
    LARGE_INTEGER filepos, move;
    move.QuadPart = 0;
    SetFilePointerEx(hFile, move, &filepos, FILE_CURRENT);
    bool result = FileDataSource::Read(nIndex, nSize, pData);
    SetFilePointerEx(hFile, filepos, NULL, FILE_BEGIN);
    return result;
}
