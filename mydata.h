#ifndef _MYDATA_H_
#define _MYDATA_H_

#include "thex.h"

class DataSource;
class Segment;
class HexDoc;
class UndoManager;
class HexWnd;
class HexWndSettings;


//*****************************************************************************
//*****************************************************************************
// thSize
//*****************************************************************************
//*****************************************************************************

inline bool CanAdd(THSIZE target, THSIZE other)
{
    return target <= 0xFFFFFFFFFFFFFFFFLL - other;
}

inline bool CanAdd(THSIZE target, THSIZE other, THSIZE ubound)
{
    return (target + other < ubound) &&
        (target + other >= target); // check for overflow
}

inline bool CanSubtract(THSIZE target, THSIZE other)
{
    return target >= other;
}

inline THSIZE Add(THSIZE target, THSIZE other)
{
    if (CanAdd(target, other))
        return target + other;
    return 0xFFFFFFFFFFFFFFFFULL;
}

inline THSIZE Add(THSIZE target, THSIZE other, THSIZE ubound)
{
    if (CanAdd(target, other, ubound))
        return target + other;
    return ubound;
}

inline THSIZE Subtract(THSIZE target, THSIZE other)
{
    if (CanSubtract(target, other))
        return target - other;
    return 0;
}

//*****************************************************************************
//*****************************************************************************
// ByteRange
//*****************************************************************************
//*****************************************************************************

class ByteRange
{
public:
    THSIZE start;
    THSIZE end;

    //ByteRange(THSIZE start, THSIZE end, THSIZE size) : start(start), end(start + size) { }

    // named constructors
    static ByteRange fromSize(THSIZE start, THSIZE size) { return ByteRange(start, start + size); }
    static ByteRange fromEnd(THSIZE start, THSIZE end) { return ByteRange(start, end); }

    // member functions
    bool contains(THSIZE nAddress) { return (nAddress >= start && nAddress < end); }
    THSIZE size() { return end - start; }

private:
    ByteRange(THSIZE start, THSIZE end) : start(start), end(end) { }
};


//*****************************************************************************
//*****************************************************************************
// SerialData
//*****************************************************************************
//*****************************************************************************

#pragma warning(disable: 4200) // nonstandard extension used: zero-size array
#pragma pack(push, 1)

typedef struct {
    uint8 endianMode;
    int nSegments, nSources;
} SerialDataHeader;

typedef struct {
    THSIZE offset, size;
    int src; // src < 0 means immediate data follows
    uint8 data[0];
} SerialDataSegment;

typedef struct {
    DataSource *pDS;
} SerialDataSource;

#pragma pack(pop)

class SerialData
{
public:
    SerialData(const uint8* data, int len);
    SerialData(wxString data);
    void Init(const uint8* data, int len);
    wxString strData;
    const uint8 *data;
    int len;
    bool loaded;
    SerialDataHeader hdr;
    int offset, m_nSourceOffset;
    THSIZE m_nTotalSize;
    void Clear(); // reset all data to 0 if corrupted
    bool Ok() { return len ? loaded : true; }
    bool GetSegment(int nSegment, SerialDataSegment &seg, const uint8 **ppData = NULL);
    bool GetSource(int nSource, SerialDataSource &src);
    bool Read(void *target, int size);
    bool ReadNumber(void *target, int size);
    wxString GetData() { return strData.IsEmpty() ? wxString((const char*)data, len) : strData; }
    //bool Extract(uint8 *target,
};


//*****************************************************************************
//*****************************************************************************
// Segment
//*****************************************************************************
//*****************************************************************************

class Segment
{
public:
    enum _type {FILE, MEM, FILL, NONE} type;
    uint64 size, stored_offset;
    DataSource *pDS;
    uint8 *pData;

    Segment *next, *prev;

    //void InsertAfter(Segment *ts);
    //void InsertBefore(Segment *ts);

    Segment(uint64 size, uint64 stored_offset, DataSource *pDS);
    Segment(uint64 size);
    Segment(uint64 size, const uint8 *pData); // copies the data into a new buffer
    // do we need a constructor that takes ownership of a pointer?  Not for now...
    Segment(uint64 size, uint8 fillData);

    ~Segment();

    Segment* Split(uint64 nOffset) { return RemoveMid(nOffset, 0); }
    void RemoveRight(uint64 nNewSize);
    bool RemoveLeft(uint64 nRemoveSize);
    Segment* RemoveMid(THSIZE nIndex, THSIZE nSize);

	bool contains(uint64 nIndex, uint64 base)
	{
		return (nIndex >= base) &&
			(nIndex < base + this->size);
	}

	bool contains(uint64 nIndex, uint64 nSize, uint64 base)
	{
		return (nIndex >= base) &&
			(nIndex + nSize <= base + this->size);
	}

    int Compare(THSIZE address, THSIZE base)
    {
       if (address < base)
          return 1;                 // address comes before this
       if (address >= base + size)
          return -1;                // address comes after this
       return 0;                    // address is inside this
    }

    bool Read(THSIZE nOffset, THSIZE nSize, uint8 *target);

    int GetSerializedLength(THSIZE nOffset, THSIZE nSize);
    void Serialize(THSIZE nOffset, THSIZE nSize, int nSource, uint8 *target);
    static Segment* Unserialize(SerialData sdata, int iSeg);

    uint8 Get(THSIZE offset); // added for ByteIterator in search.cpp
};




//*****************************************************************************
//*****************************************************************************
// Selection
//*****************************************************************************
//*****************************************************************************

class Selection
{
public:
    THSIZE nStart, nEnd;
    int iRegion, iDigit;

    Selection()
    {
        Set(0, 0, -1, -1);
    }

    Selection(THSIZE start)
    {
        Set(start);
    }

    Selection(THSIZE start, THSIZE end, int region = 0, int digit = 0)
    {
        Set(start, end, region, digit);
    }

    void Set(THSIZE start) { Set(start, start, 0, 0); }

    void Set(THSIZE start, THSIZE end, int region = 0, int digit = 0)
    {
        nStart = start;
        nEnd = end;
        iRegion = region;
        iDigit = digit;
    }

    void Get(THSIZE &first, THSIZE &size)
    {
        if (nEnd < nStart)
            first = nEnd, size = nStart - nEnd;
        else
            first = nStart, size = nEnd - nStart;
    }

    THSIZE GetFirst() { return wxMin(nStart, nEnd); }
    THSIZE GetLast() { return wxMax(nStart, nEnd); }
    THSIZE GetSize() { return GetLast() - GetFirst(); }
    void SetSize(THSIZE size) { nEnd = nStart + size; }

    bool IsSet() { return (iDigit >= 0); }

    bool operator==(const Selection &other)
    {
        return
            nStart == other.nStart &&
            nEnd == other.nEnd &&
            (iRegion == other.iRegion || iRegion == 0 || other.iRegion == 0) &&
            iDigit == other.iDigit;
    }
};


//*****************************************************************************
//*****************************************************************************
// HexDoc
//*****************************************************************************
//*****************************************************************************

class HexDoc
{
public:
    HexDoc();
    ~HexDoc();

    HexWndSettings *pSettings;

    enum { SUPPRESS_UPDATE = 1 }; // for InsertAt/RemoveAt flags

    inline THSIZE GetSize() const { return size; }
    uint64 size, display_address;
    wxString info;
    Segment *m_curSeg;

    // Tweaking any balanced tree structure I could find to look up nodes by
    // position only turned out to be more complicated than I wanted to mess
    // with.  I decided to go with vector<> for now, for its ease of use.
    // You can do a binary search on a vector if you have bases and sizes.
    // While it may be technically possible to store Segments in a tree
    // where the base is never stored directly (only the size of the left and
    // right subtrees), rebalancing such a tree after insertions or deletions
    // would be a complete pain.  So we do it this way.
    // Using a vector means you still have to update N items when the size of
    // one Segment changes, but the data structure is simple and gives the best
    // possible search performance.  I think that's fair.
    // (And it's better than a linked list.)
    typedef std::vector<Segment*> SEGMENTVEC;
    typedef std::vector<THSIZE> THSIZEVEC;
    SEGMENTVEC segments;
    THSIZEVEC bases;

    THSIZE m_iCurSegOffset;
    bool bWriteable;
    bool bCanChangeSize;
    DataSource *m_pDS;
    HexDoc *next;
    DWORD dwFlags; //! so far this holds protection flags from VirtualQueryEx()

    //! this really shouldn't be here -- where does it belong?  HexView class?
    Selection m_sel;
    THSIZE m_iFirstLine;

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

    bool InsertAt(uint64 nIndex, uint8 src, THSIZE nCount = 1);
    bool InsertAt(uint64 nIndex, const uint8* psrc, int32 nSize, THSIZE nCount = 1, int flags = 0);
    bool RemoveAt(uint64 nIndex, uint64 nSize, int flags = 0);
    bool ReplaceAt(uint64 ToReplaceIndex, uint8 ReplaceWith);
    bool ReplaceAt(uint64 ToReplaceIndex, uint32 ToReplaceLength, const uint8* pReplaceWith, uint32 ReplaceWithLength);
    //bool ReplaceAt(uint64 ToReplaceIndex, uint32 ToReplaceLength, uint8 pReplaceWith, int nCount = 1);

    bool CanReplaceAt(THSIZE nIndex, THSIZE nOldSize, THSIZE nNewSize);
    bool CanReplaceAt(THSIZE nIndex, THSIZE nSize) { return CanReplaceAt(nIndex, nSize, nSize); }

    uint8* LoadWriteable(uint64 nIndex, uint32 nSize);
    //const uint8* Load(uint64 nIndex, uint32 nSize);
    bool Load(uint64 nIndex);
    virtual bool Save();
    virtual bool SaveRange(LPCSTR filename, uint64 begin, uint64 length);
    virtual bool WriteRange(HANDLE hOutput, uint64 begin, uint64 length);
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
    wxString ReadString(THSIZE nIndex, size_t nSize);
    wxString ReadStringW(THSIZE nIndex, size_t nSize);

    int find_bytes(uint64 nStart, int ls, const uint8* pb, int lb, int mode, uint8 (*cmp) (uint8, uint8));
    bool Find(const char *text, THSIZE length, int type, bool caseSensitive, THSIZE &start, THSIZE &end);
    bool FindHex(const wxByte *hex, size_t count, /*IN_OUT*/THSIZE &startpos, /*IN_OUT*/THSIZE &endpos);

    Segment *GetSegment(uint64 nIndex, uint64 *pSegmentStart = NULL);
    int FindSegment(THSIZE nIndex);
    Segment* SegmentAt(size_t n) { return (n < segments.size()) ? segments[n] : NULL; }
    void UpdateSegmentPointers();

    int GetSerializedLength(THSIZE nOffset, THSIZE nSize);
    void Serialize(THSIZE nOffset, THSIZE nSize, uint8 *target);
    wxString Serialize(THSIZE nOffset, THSIZE nSize)
    {
        wxString str;
        int len = GetSerializedLength(nOffset, nSize);
        Serialize(nOffset, nSize, (uint8*)str.GetWriteBuf(len));
        str.UngetWriteBuf(len);
        return str;
    }
    bool ReplaceSerialized(THSIZE nAddress, THSIZE ToReplaceLength, SerialData &sInsert);
    wxString SerializeData(uint8* pData, size_t nSize);

    HexWnd *hw;
    UndoManager *undo;
    bool Undo();
    bool Redo();

    DWORD GetChange() const { return m_iChangeIndex; }
    DWORD m_iChangeIndex; // not decremented on undo

protected:
    bool IsByteLoaded(uint64 nIndex) const
    {
        return m_curSeg && m_curSeg->contains(nIndex, m_iCurSegOffset);
    }

    bool IsRangeLoaded(uint64 nIndex, uint64 nSize) const
    {
        return m_curSeg && m_curSeg->contains(nIndex, nSize, m_iCurSegOffset);
    }

    bool ReadFromSegment(uint64 nIndex, uint32 nSize, uint8 *target, Segment *seg);
    //Segment* InsertSegment(uint64 nIndex, uint64 nSize, Segment *prev, Segment *next);
    bool InsertSegment(THSIZE nAddress, Segment *ts); //! always returns true for now
    bool InsertSegment2(size_t n, THSIZE base, Segment* s);
    bool RemoveSegment(size_t n);
    bool CanWriteInPlace();

    friend class DataSource;

    // features for std::map
public:
    bool operator<(const HexDoc &d) const;
};

class ModifiedRange
{
public:
    ModifiedRange *next;
    uint64 size;
    bool bModified;
};


//*****************************************************************************
//*****************************************************************************
// ByteIterator2
//*****************************************************************************
//*****************************************************************************

struct ByteIterator2
{
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef wxByte value_type;
    typedef wxFileOffset difference_type;
    typedef const value_type *pointer;
    typedef const value_type &reference;

    THSIZE          pos;
    //wxFileOffset    linepos;
    HexDoc         *doc;
    uint8          *buffer;

    ByteIterator2() { buffer = new uint8[0x4000]; doc = NULL; }

    ~ByteIterator2() { delete [] buffer; }

    ByteIterator2(const ByteIterator2 &it)
    {
        this->operator =( it );
    }

    ByteIterator2(wxFileOffset pos0, HexDoc *doc0)
        :pos(pos0), doc(doc0)
    {
        buffer = new uint8[0x4000];
        Read();
    }

    ByteIterator2 & operator=(const ByteIterator2 & it)
    {
        bool read = true;
        if (doc != NULL && ((pos ^ it.pos) >> 12) == 0)
            read = false;
        pos = it.pos;
        doc = it.doc;
        if (read)
            Read();
        return *this;
    }

    const value_type operator*()
    {
        //wxASSERT(pos < doc->GetSize());
        return buffer[pos & 0x3FFF];
    }

    // pre-increment operator
    ByteIterator2 & operator++()
    {
        ++pos;
        if (((DWORD)pos & 0x3FFF) == 0)
            Read();

        return *this;
    }

    // pre-decrement operator
    ByteIterator2 & operator--()
    {
        wxASSERT(pos>0);
        --pos;
        if ((pos & 0x3FFF) == 0x3FFF)
            Read();

        return *this;
    }

    bool operator==(const ByteIterator2 & it) const
    {
        return (pos == it.pos);
    }

    bool operator!=(const ByteIterator2 & it) const
    {
        return ! (this->operator==(it)) ;
    }

    void Read()
    {
        int size = 0x4000;
        THSIZE offset = pos & (THSIZE)~0x3FFF;
        if (offset + size > doc->GetSize())
            size = doc->GetSize() - offset;
        doc->Read(offset, size, buffer);
    }

    bool AtEnd() const { return pos == doc->GetSize(); }
};

struct WordIterator2
{
    typedef uint16 value_type;

    THSIZE          pos;
    //wxFileOffset    linepos;
    HexDoc         *doc;
    uint16         *buffer;

    enum {BUFFER_COUNT = 0x4000, BUFFER_MAX = 0x3FFF};

    WordIterator2() { buffer = new value_type[BUFFER_COUNT]; doc = NULL; }

    ~WordIterator2() { delete [] buffer; }

    WordIterator2(const WordIterator2 &it)
    {
        this->operator =( it );
    }

    WordIterator2(wxFileOffset pos0, HexDoc *doc0)
        :pos(pos0), doc(doc0)
    {
        buffer = new value_type[BUFFER_COUNT];
        Read();
    }

    WordIterator2 & operator=(const WordIterator2 & it)
    {
        bool read = true;
        if (doc != NULL && ((pos ^ it.pos) >> 12) == 0)
            read = false;
        pos = it.pos;
        doc = it.doc;
        if (read)
            Read();
        return *this;
    }

    const value_type operator*()
    {
        //wxASSERT(pos < doc->GetSize()); //! Not relevant since pos is word position.  Could fix it, but let's not.
        return buffer[pos & BUFFER_MAX];
    }

    // pre-increment operator
    WordIterator2 & operator++()
    {
        ++pos;
        if (((DWORD)pos & BUFFER_MAX) == 0)
            Read();

        return *this;
    }

    // pre-decrement operator
    WordIterator2 & operator--()
    {
        wxASSERT(pos>0);
        --pos;
        if ((pos & BUFFER_MAX) == BUFFER_MAX)
            Read();

        return *this;
    }

    bool operator==(const WordIterator2 & it) const
    {
        return (pos == it.pos);
    }

    bool operator!=(const WordIterator2 & it) const
    {
        return ! (this->operator==(it)) ;
    }

    void Read()
    {
        int size = BUFFER_COUNT * sizeof(value_type);
        THSIZE offset = (pos & (THSIZE)~BUFFER_MAX) * sizeof(value_type);
        if (offset + size > doc->GetSize())
            size = doc->GetSize() - offset;
        doc->Read(offset, size, (uint8*)buffer);
    }

    bool AtEnd() const { return (pos * 2) + 1 >= doc->GetSize(); }
};

#endif // _MYDATA_H_
