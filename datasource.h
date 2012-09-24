#ifndef _DATASOURCE_H_
#define _DATASOURCE_H_

#include "defs.h"

class HexDoc;
class HexWnd;

//*****************************************************************************
//*****************************************************************************
// DataSource
//*****************************************************************************
//*****************************************************************************

class DataSource
{
public:
    DataSource();
    virtual ~DataSource() { /*CleanUp();*/ }

    inline int AddRef() {
        m_nRefs++;
        return m_nRefs;
    }
    inline int Release() {
        wxASSERT(m_nRefs > 0);
        int tmp = --m_nRefs;
        if (tmp == 0)
            delete this;
        return tmp;
    }

    //void CleanUp(); // deletes all HexDocs (not anymore.  Now they're owned by HexWnd.

    int GetSerializedLength();
    void Serialize(uint8 *target);

    inline bool IsWriteable() const { return m_bWriteable; }
    inline bool IsReadOnly() const { return !m_bWriteable; }
    inline bool CanChangeSize() const { return m_bCanChangeSize; }
    inline bool IsMemoryMapped() const { return m_bMemoryMapped; }
    inline bool IsOpen() const { return m_bOpen; }

    virtual std::vector<HexDoc*> GetDocs() { return docs; }
    virtual HexDoc* AddRegion(uint64 display_address, uint64 stored_address, uint64 size, wxString info,
        bool bWriteable = true, DWORD dwFlags = 0);
    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData) = 0;
    virtual bool Write(uint64 UNUSED(nIndex), uint32 UNUSED(nSize), const uint8 *UNUSED(pData)) { return false; }
    virtual void Flush() { }
    virtual bool SetEOF(THSIZE WXUNUSED(nIndex)) { return false; }
    virtual void ShowProperties(HexWnd *hw);
    virtual wxString GetFullPath() { return wxEmptyString; }
    virtual bool CanWriteInPlace() const { return IsWriteable(); }

    virtual bool IsFile() const { return false; }
    virtual bool IsLoadedDLL() const { return false; }

    wxString GetTitle() { return m_title; }

    HexDoc *GetDocAt(THSIZE address);
    virtual const uint8* GetBasePointer(THSIZE WXUNUSED(base), THSIZE WXUNUSED(size)) { return NULL; }

    virtual bool ToggleReadOnly() { return false; }

    size_t MinTransfer;  // Smallest unit of data.  Sector size for disks.
    size_t MaxTransfer;  // Buffers will be this size.  1M is good if there is no hard limit.
    size_t BlockAlign;   // Cached buffers will be aligned on this boundary.

protected:
    bool m_bWriteable;
    bool m_bCanChangeSize;
    bool m_bOpen;
    bool m_bMemoryMapped;
    //HexDoc *m_pRootRegion;
    int m_nRefs;
    wxString m_title;

    //std::set<HexDoc> docs;
    std::vector<HexDoc*> docs; // used only to set up.  After that, HexWnd calls GetDocs() and owns them.
};

class FileDataSource : public DataSource
{
public:
    FileDataSource(LPCTSTR filename, bool bReadOnly);
    ~FileDataSource();

    virtual void Flush();
    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);
    virtual bool Write(uint64 nIndex, uint32 nSize, const uint8 *pData);
    virtual void ShowProperties(HexWnd *hw);
    virtual bool SetEOF(THSIZE nIndex);
    virtual bool IsFile() const { return true; }

    virtual const uint8* GetBasePointer(THSIZE nIndex, THSIZE nSize);
    virtual wxString GetFullPath() { return m_fullpath; }
protected:
    FileDataSource();  // for classes that derive from this

    wxString m_fullpath;
    bool MemoryMap(THSIZE nIndex, THSIZE nSize);
    HANDLE hFile, hMapping;
    uint8 *m_pData;
    THSIZE m_mapStart, m_mapSize;
    bool tryMapping; // set to false on first error
    LARGE_INTEGER fileSize;
};

class DiskDataSource : public DataSource
{
public:
    DiskDataSource(LPCTSTR filename, bool bReadOnly);
    ~DiskDataSource();

    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);
    virtual bool Write(uint64 nIndex, uint32 nSize, const uint8 *pData);

protected:
    bool ReadSector(THSIZE nIndex);
    HANDLE hDisk;
    uint8 *m_pData;
    THSIZE curOffset;

    bool m_bIsAudioCD;
    HANDLE m_hCD;
    void *m_track; // Use void type to avoid #including "akrip32.h"
};

class ProcMemDataSource : public DataSource
{
public:
    ProcMemDataSource(DWORD pid, wxString procName, bool bReadOnly);
    ~ProcMemDataSource();
    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);
    virtual bool Write(uint64 nIndex, uint32 nSize, const uint8 *pData);

    //virtual const uint8* GetBasePointer(THSIZE nIndex, THSIZE nSize);
    virtual bool IsLoadedDLL() const { return true; }

    virtual void ShowProperties(HexWnd *hw);

    bool IsThisProc;
    HANDLE hProcess;
};

class UnsavedDataSource : public DataSource
{
public:
    UnsavedDataSource();
    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);
    virtual bool CanWriteInPlace() const { return false; }

protected:
    static int docNumber;
};


//class DSModifyBuffer : public DataSource
//{
//public:
//    DSModifyBuffer(size_t size = 10000);
//    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);
//    virtual bool Write(uint64 nIndex, uint32 nSize, const uint8 *pData);
//
//protected:
//    size_t used, total;
//    uint8 *buffer;
//};


class ModifyBuffer : public DataSource  // formerly known as FastWriteBuffer
{
public:
    ModifyBuffer(size_t blockSize = DEFAULT_BLOCK_SIZE)
    {
        m_nBlockSize = blockSize;
        size = 0;
        //Alloc(); // start off empty
    }

    ~ModifyBuffer()
    {
        for (size_t i = 0; i < ptrs.size(); i++)
            delete ptrs[i];
    }

    void Alloc()
    {
        ptrs.push_back(new uint8[m_nBlockSize]);
    }

    operator uint8*() const
    {
        return ptrs.back();
    }

    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData)
    {
        if (nIndex + nSize > size)
            return false;

        size_t block = nIndex / m_nBlockSize;
        size_t offset = nIndex % m_nBlockSize;
        while (nSize)
        {
            size_t blocksize = wxMin(m_nBlockSize - offset, nSize);
            memcpy(pData, ptrs[block] + offset, blocksize);
            nIndex += blocksize;
            pData += blocksize;
            nSize -= blocksize;
            offset = 0;
            block++;
        }
        return true;
    }

    virtual bool Write(uint64 nIndex, uint32 nSize, const uint8 *pData)
    {
        if (nIndex != size)
            return false;

        size += nSize;

        size_t block  = nIndex / m_nBlockSize;
        size_t offset = nIndex % m_nBlockSize;
        while (nSize)
        {
            size_t blocksize = wxMin(m_nBlockSize - offset, nSize);
            if (block == ptrs.size())
                Alloc();
            memcpy(ptrs[block] + offset, pData, blocksize);
            pData += blocksize;
            //size += blocksize;
            nSize -= blocksize;
            offset = 0;
            block++;
        }
        return true;
    }

    virtual bool Append(const uint8 *pData, uint32 nSize) { return Write(size, nSize, pData); }

    size_t GetSize() { return size; }
    size_t GetBlockSize() { return m_nBlockSize; }

    enum { DEFAULT_BLOCK_SIZE = 16384 };

private:
    size_t size;
    std::vector<uint8*> ptrs;
    size_t m_nBlockSize;
};

//*****************************************************************************

class CachedFile : public DataSource  // Reads an entire file into RAM.
{
public:
    CachedFile(wxString filename)
    {
        m_pData = NULL;
        m_bOpen = false;
        HANDLE hFile = CreateFile(filename, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            wxString tmp;
            tmp.Printf(_T("Couldn't open \"%s\".\nCode %d."), filename, GetLastError());
            wxMessageBox(tmp);
            return;
        }

        m_nSize = ::GetFileSize(hFile, NULL);
        m_pData = new uint8[m_nSize];
        DWORD cBytes;
        ReadFile(hFile, m_pData, m_nSize, &cBytes, NULL);
        CloseHandle(hFile);

        m_bWriteable = true;
        m_bCanChangeSize = false;
        m_bOpen = true;
        m_bMemoryMapped = true;
        m_title = filename;

        AddRegion(0, 0, m_nSize, filename);
    }

    ~CachedFile()
    {
        delete [] m_pData;
        m_pData = NULL;
    }

    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData)
    {
        if (nIndex + nSize > m_nSize || pData == NULL)
            return false;
        memcpy(pData, m_pData + nIndex, nSize);
        return true;
    }

    virtual bool Write(uint64 nIndex, uint32 nSize, const uint8 *pData) const
    {
        if (nIndex + nSize > m_nSize || pData == NULL)
            return false;
        memcpy(m_pData + nIndex, pData, nSize);
        return true;
    }

    size_t GetSize() { return m_nSize; }

private:
    uint8 *m_pData;
    THSIZE m_nSize;
};


//*****************************************************************************
// Very dangerous!  Will probably crash a process or lead to less obvious problems.

class ProcessFileDataSource : public FileDataSource
{
public:
    ProcessFileDataSource(DWORD procID, DWORD hForeignFile);
    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);
};

#endif // _DATASOURCE_H_
