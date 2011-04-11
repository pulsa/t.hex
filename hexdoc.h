#ifndef _HEXDOC_H_
#define _HEXDOC_H_

#include "thex.h"
#include "datahelp.h"

class ModifyBuffer;

//*****************************************************************************
//*****************************************************************************
// HexDoc
//*****************************************************************************
//*****************************************************************************

class HexDoc
{
public:
    //HexDoc();
    HexDoc(DataSource *pDS, THSIZE start, THSIZE size, DWORD dwFlags);
    ~HexDoc();

    HexWndSettings *pSettings;

    //enum { SUPPRESS_UPDATE = 1 }; // for InsertAt/RemoveAt flags

    inline THSIZE GetSize() const { return size; }
    uint64 size;
    wxString info;
    Segment *m_current;
    THSIZE m_curBase; // base offset of m_current
    Segment *m_head, *m_tail;
    ModifyBuffer *modbuf;
    bool NoCache; // for debugging only.  No UI.

    //THSIZE m_iCurSegOffset;
    bool bWriteable;
    bool bCanChangeSize;
    DataSource *m_pDS; // root data source from which this is created.  Store so we can save to it.
    //HexDoc *next;
    DWORD dwFlags; //! so far this holds protection flags from VirtualQueryEx()

    //! this really shouldn't be here -- where does it belong?  HexView class?
    Selection m_sel;
    THSIZE m_iFirstLine;
    THSIZE display_address;

    bool IsWriteable() { return bWriteable; }
    bool IsReadOnly() { return !bWriteable; }

    bool CanChangeSize() const { return bCanChangeSize; }

    bool Read(uint64 nOffset, uint32 nSize, uint8 *target, uint8 *pModified = NULL);

    const inline uint8 operator[](uint64 nIndex)  { return GetAt(nIndex); }
    //uint8& operator[](uint64 nIndex);
    uint8 GetAt(uint64 nIndex);
    bool IsModified(uint64 address);
    bool IsModified();

    void MarkModified(const ByteRange& range);
    void MarkUnmodified(const ByteRange& range);
    std::vector<ByteRange> modRanges;
    int FindModified(THSIZE nAddress, bool wantIndexAnyway = false);
    int FindModified(const ByteRange& range, int& count, bool includeAdjacent);

    bool InsertAt(THSIZE nIndex, uint8 src, THSIZE nCount = 1)
    {
        return InsertAt(nIndex, &src, 1, nCount);
    }

    bool InsertAt(THSIZE nIndex, const uint8* psrc, int32 nSize, THSIZE nCount = 1)
    {
        return ReplaceAt(nIndex, 0, psrc, nSize, nCount);
    }

    bool RemoveAt(THSIZE nIndex, THSIZE nSize)
    {
        return ReplaceAt(nIndex, nSize, 0, 0);
    }

    bool ReplaceAt(THSIZE ToReplaceIndex, uint8 ReplaceWith)
    {
        return ReplaceAt(ToReplaceIndex, 1, &ReplaceWith, 1);
    }

    bool ReplaceAt(THSIZE ToReplaceIndex, THSIZE ToReplaceLength, const uint8* pReplaceWith,
                   uint32 ReplaceWithLength, THSIZE nCount = 1);

    bool CanReplaceAt(THSIZE nIndex, THSIZE nOldSize, THSIZE nNewSize);
    bool CanReplaceAt(THSIZE nIndex, THSIZE nSize) { return CanReplaceAt(nIndex, nSize, nSize); }

    uint8* LoadWriteable(uint64 nIndex, uint32 nSize);  //! not implemented
    const uint8* Load(uint64 nIndex, uint32 nSize, uint32 *pCachedSize = 0);  // convenience function to get a read-only buffer
    bool Cache(THSIZE nIndex, THSIZE nSize);

    bool Seek(THSIZE nOffset) { m_nFilePointer = nOffset; return true; }
    inline uint8 Next8()
    {
        if (m_nFilePointer < m_cacheStart || m_nFilePointer >= m_cacheStart + m_cacheSize)
            Cache(m_nFilePointer, 0x4000);
        return m_pCacheData[m_nFilePointer++ - m_cacheStart];
    }
    THSIZE m_nFilePointer;

    bool Save();
    bool SaveRange(wxString filename, THSIZE begin, THSIZE length);
    bool WriteRange(wxFile &hOutput, THSIZE begin, THSIZE length, wxString msg = _T("Writing..."));
    /*inline bool Save()
    {
        if (!OK()) return false;
    }

    inline bool SaveRange(LPCSTR filename, uint64 begin, uint64 length)
    {
        if (!OK()) return false;
    }*/

    inline uint16 Read16(uint64 nIndex) { uint16 t=0; ReadNumber(nIndex, sizeof(t), &t); return t; }
    inline uint32 Read32(uint64 nIndex) { uint32 t=0; ReadNumber(nIndex, sizeof(t), &t); return t; }
    inline uint64 Read64(uint64 nIndex) { uint64 t=0; ReadNumber(nIndex, sizeof(t), &t); return t; }
    inline double ReadDouble(uint64 nIndex) { double t=0; ReadNumber(nIndex, sizeof(t), &t); return t; }
    inline float ReadFloat(uint64 nIndex) { float t=0; ReadNumber(nIndex, sizeof(t), &t); return t; }
    bool ReadNumber(uint64 nIndex, int nBytes, void *target);
    bool ReadInt(uint64 nIndex, int nBytes, UINT64 *target, int mode = -1);
    wxString ReadString(THSIZE nIndex, size_t nSize, bool replaceNull = true);
    wxString ReadStringW(THSIZE nIndex, size_t nSize, bool replaceNull = true);

    int find_bytes(uint64 nStart, int ls, const uint8* pb, int lb, int mode, uint8 (*cmp) (uint8, uint8));

    // Find() and FindHex() return 1 if found, 0 if not, or -1 if aborted.
    int Find(const uint8 *data, size_t count, int type, bool caseSensitive, THSIZE &start, THSIZE &end);
    int FindHex(const uint8 *data, size_t count, /*IN_OUT*/THSIZE &startpos, /*IN_OUT*/THSIZE &endpos, bool caseSensitive = true);

    bool MoveToSegment(THSIZE nIndex); // sets m_current and m_curBase
    Segment *GetSegment(THSIZE nIndex, THSIZE *pSegmentBase)
    {
        if (!MoveToSegment(nIndex)) return NULL;
        *pSegmentBase = m_curBase;
        return m_current;
    }
    //int FindSegment(THSIZE nIndex);
    //Segment* SegmentAt(size_t n) { return (n < segments.size()) ? segments[n] : NULL; }
    //void UpdateSegmentPointers();

    int GetSerializedLength(THSIZE nOffset, THSIZE nSize);
    void Serialize(THSIZE nOffset, THSIZE nSize, uint8 *target);
    wxString Serialize(THSIZE nOffset, THSIZE nSize)
    {
        wxString str;
#if !wxCHECK_VERSION(2, 9, 0)
        int len = GetSerializedLength(nOffset, nSize);
        if (len) {
            Serialize(nOffset, nSize, (uint8*)str.GetWriteBuf(len));
            str.UngetWriteBuf(len);
        }
#endif  //!
        return str;
    }
    bool ReplaceSerialized(THSIZE nAddress, THSIZE ToReplaceLength, SerialData &sInsert, bool bInsideUndo = false);
    //wxString SerializeData(const uint8* pData, size_t nSize);

    HexWnd *hw;
    UndoManager *undo;
    bool Undo();
    bool Redo();

    bool DoRead(THSIZE nOffset, size_t nSize, uint8* target);
    bool ReadNoUpdateCache(THSIZE nOffset, size_t nSize, uint8* target);

    DWORD GetChange() const { return m_iChangeIndex; }
    DWORD m_iChangeIndex; // not decremented on undo
    void DumpCache(); // debug

    void InvalidateCache(); // was protected, but now called by HexWnd.  OK?
    void InvalidateCache(THSIZE nAddress, THSIZE nOldSize, THSIZE nNewSize);

    bool ComputeAdler32(THSIZE offset, THSIZE size, ULONG &adler);

protected:
    THSIZE m_cacheStart, m_cacheSize, m_cacheBufferSize;
    uint8* m_pCacheBuffer; // scratch
    const uint8* m_pCacheData;   // points to m_pCacheBuffer OR some other memory

    inline bool IsByteCached(uint64 nIndex) const { return IsRangeCached(nIndex, 1); }

    bool IsRangeCached(uint64 nIndex, uint64 nSize) const
    {
        //return m_curSeg && m_curSeg->contains(nIndex, nSize, m_iCurSegOffset);
        return (m_cacheStart <= nIndex && m_cacheStart + m_cacheSize >= nIndex + nSize);
    }

    //bool ReadFromSegment(uint64 nIndex, uint32 nSize, uint8 *target, Segment *seg);
    //Segment* InsertSegment(uint64 nIndex, uint64 nSize, Segment *prev, Segment *next);
    bool CanWriteInPlace();

    bool DoRemoveAt(THSIZE nIndex, THSIZE nSize);
    bool DoInsertAt(THSIZE nIndex, const uint8* psrc, int32 nSize, THSIZE nCount = 1);
    bool DoInsertSegment(THSIZE nAddress, Segment *ts); //! always returns true for now

    void DeleteSegments(); // delete all segments, including m_head and m_tail
    bool RejoinSegments(); // try to recombine adjacent segments from the same DataSource

    friend class DataSource;

    // features for std::map
public:
    bool operator<(const HexDoc &d) const;
};

// Increasing this doesn't do much good, and holds up the GUI thread longer.  Bad design.
#define HEXDOC_BUFFER_SIZE (MEGA)

#endif // _HEXDOC_H_
