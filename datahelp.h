#ifndef _DATAHELP_H_
#define _DATAHELP_H_

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
    return (target + other <= ubound) &&
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
    //uint8 endianMode;
    int nSegments, nSources;
} SerialDataHeader;

//typedef struct {
//    THSIZE offset, size;
//    int src; // src < 0 means immediate data follows
//    uint8 data[0];
//} SerialDataSegment;

typedef struct
{
    uint64 stored_offset;   // offset of data in source
    uint64 size;            // number of bytes this segment displays, including repetition
    uint64 srcSize;         // number of bytes used in data source
    //uint32 repCount;        // repeat count (Usually 0.  Never 1.)
    bool fill;              // fill flag can be inferred if size > srcSize... unless size changes.  Hmm.
    union {
        int src;
        DataSource *pDS;
    };
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

    wxString GetData()
    {
        if (strData.IsEmpty())
            strData = wxString((const TCHAR*)data, len);
        return strData;
    }

    THSIZE m_nTotalSize;
    SerialDataHeader hdr;

    bool Extract(THSIZE size, uint8 *target);
    bool Ok() { return len ? loaded : true; }

    bool GetSegment(int nSegment, SerialDataSegment &seg, const uint8 **ppData = NULL);
    bool GetSource(int nSource, SerialDataSource &src);

protected:
    wxString strData;
    const uint8 *data;
    int len;
    bool loaded;
    int offset, m_nSourceOffset;
    void Clear(); // reset all data to 0 if corrupted
    bool Read(void *target, int size);
    bool ReadNumber(void *target, int size);
};


//*****************************************************************************
//*****************************************************************************
// Segment
//*****************************************************************************
//*****************************************************************************

class Segment : public SerialDataSegment
{
public:
    Segment *next, *prev;

    //void InsertAfter(Segment *ts);
    //void InsertBefore(Segment *ts);

    Segment(uint64 size, uint64 stored_offset, DataSource *pDS, THSIZE nCount = 1);
    //Segment(uint64 size);
    //Segment(uint64 size, const uint8 *pData); // copies the data into a new buffer
    // do we need a constructor that takes ownership of a pointer?  Not for now...
    //Segment(uint64 size, uint8 fillData);

    ~Segment();

    Segment* Split(uint64 nOffset) { return RemoveMid(nOffset, 0); }
    void RemoveRight(uint64 nNewSize);
    bool RemoveLeft(uint64 nRemoveSize);
    Segment* RemoveMid(THSIZE nIndex, THSIZE nSize); // returns the right remainder, doesn't update list

    void ExtendForward(uint64 nAddSize);
    void ExtendBackward(uint64 nAddSize);

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

    const uint8* GetBasePointer(THSIZE nOffset, THSIZE nSize); // return pointer to data, if memory-mapped
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

    void Get(THSIZE &first, THSIZE &size) const
    {
        if (nEnd < nStart)
            first = nEnd, size = nStart - nEnd;
        else
            first = nStart, size = nEnd - nStart;
    }

    THSIZE GetFirst() const { return wxMin(nStart, nEnd); }
    THSIZE GetLast() const { return wxMax(nStart, nEnd); }
    THSIZE GetSize() const { return GetLast() - GetFirst(); }
    void SetLastFromFirst(THSIZE size)
    {
        if (nEnd >= nStart)
            nEnd = nStart + size;
        else
            nStart = nEnd + size;
    }

    Selection &Order()
    {
        if (nEnd < nStart)
            Set(nEnd, nStart, iRegion, iDigit);
        return *this;
    }

    bool IsSet() const { return (iDigit >= 0); }

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

    enum {BUFFER_COUNT = 0x4000, BUFFER_MAX = 0x3FFF};

    THSIZE          pos, m_nEnd;
    //wxFileOffset    linepos;
    HexDoc         *doc;
    uint8          *buffer;

    ByteIterator2() { buffer = new uint8[BUFFER_COUNT]; doc = NULL; }

    ~ByteIterator2() { delete [] buffer; }

    ByteIterator2(const ByteIterator2 &it)
    {
        this->operator =( it );
    }

    ByteIterator2(wxFileOffset pos0, HexDoc *doc0);
    ByteIterator2(wxFileOffset pos0, wxFileOffset size0, HexDoc *doc0);

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
        return buffer[(UINT)pos & BUFFER_MAX];
    }

    // pre-increment operator
    ByteIterator2 & operator++()
    {
        ++pos;
        if (((UINT)pos & BUFFER_MAX) == 0)
            Read();

        return *this;
    }

    // pre-decrement operator
    ByteIterator2 & operator--()
    {
        wxASSERT(pos>0);
        --pos;
        if (((UINT)pos & BUFFER_MAX) == BUFFER_MAX)
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

    void Read();

    bool AtEnd() const { return pos == m_nEnd; }
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
        return buffer[(UINT)pos & BUFFER_MAX];
    }

    // pre-increment operator
    WordIterator2 & operator++()
    {
        ++pos;
        if (((UINT)pos & BUFFER_MAX) == 0)
            Read();

        return *this;
    }

    // pre-decrement operator
    WordIterator2 & operator--()
    {
        wxASSERT(pos>0);
        --pos;
        if (((UINT)pos & BUFFER_MAX) == BUFFER_MAX)
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

    void Read();
    bool AtEnd() const;
};

class thDocInputStream : public wxInputStream
{
public:
    //! ToDo: include progress bar?
    thDocInputStream(HexDoc *doc, Selection *pSel = NULL);
    virtual ~thDocInputStream() { }

    wxFileOffset GetLength() const { return m_size; }
    size_t GetSize() const { return m_size; }

    bool Ok() const { return IsOk(); }
    virtual bool IsOk() const;
    bool IsSeekable() const { return m_bSeekable; }
    void SetSeekable(bool bSeekable) { m_bSeekable = bSeekable; }

protected:
    thDocInputStream();

    size_t OnSysRead(void *buffer, size_t size);
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode);
    wxFileOffset OnSysTell() const { return m_pos; }

protected:
    HexDoc *m_doc;
    bool m_bSeekable;
    wxFileOffset m_pos;
    THSIZE m_start, m_size;

    DECLARE_NO_COPY_CLASS(thDocInputStream)
};

class thNullOutputStream : public wxOutputStream
{
public:
    thNullOutputStream()
    {
        m_pos = 0;
    }

protected:
    size_t OnSysWrite(const void * WXUNUSED(buffer), size_t bufsize);
    wxFileOffset OnSysTell() const { return m_pos; }

    THSIZE m_pos;
};

#endif // _DATAHELP_H_
