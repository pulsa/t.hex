#ifndef _MYDATA_H_
#define _MYDATA_H_

#include "thex.h"

class DataSource;
class Segment;
class HexDoc;
class UndoManager;
class HexWnd;


//*****************************************************************************
//*****************************************************************************
// thSize
//*****************************************************************************
//*****************************************************************************

inline bool CanAdd(THSIZE target, THSIZE other)
{
    return target <= 0xFFFFFFFFFFFFFFFFLL - other;
}

inline bool CanSubtract(THSIZE target, THSIZE other)
{
    return target >= other;
}

inline THSIZE Add(THSIZE target, THSIZE other)
{
    if (CanAdd(target, other))
        target += other;
    else
        target = 0xFFFFFFFFFFFFFFFFULL;
    return target;
}

inline THSIZE Subtract(THSIZE target, THSIZE other)
{
    if (CanSubtract(target, other))
        target -= other;
    else
        target = 0;
    return target;
}


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
	enum _type {FILE, MEM, FILL} type;
	uint64 size, stored_offset;
    DataSource *pDS;
    uint8 *pData;

	Segment *next, *prev;

    void InsertAfter(Segment *ts);
    void InsertBefore(Segment *ts);

    Segment(uint64 size, uint64 stored_offset, DataSource *pDS);
    Segment(uint64 size);
	Segment(uint64 size, const uint8 *pData); // copies the data into a new buffer
    //! do we need a constructor that takes ownership of a pointer?
    Segment(uint64 size, uint8 fillData);

	~Segment();

    bool Split(uint64 nOffset) { return RemoveMid(nOffset, 0); }
	void RemoveRight(uint64 nNewSize);
	bool RemoveLeft(uint64 nRemoveSize);
    bool RemoveMid(THSIZE nIndex, THSIZE nSize);

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

    bool Read(THSIZE nOffset, THSIZE nSize, uint8 *target);

    int GetSerializedLength(THSIZE nOffset, THSIZE nSize);
    void Serialize(THSIZE nOffset, THSIZE nSize, int nSource, uint8 *target);
    static Segment* Unserialize(SerialData sdata, int iSeg);

    uint8 Get(THSIZE offset); // added for ByteIterator in search.cpp
};


//*****************************************************************************
//*****************************************************************************
// DataSource
//*****************************************************************************

class DataSource
{
public:
    DataSource();
	virtual ~DataSource() {}

    inline int AddRef() { m_nRefs++; return m_nRefs; }
    inline int Release() { int tmp = --m_nRefs; if (tmp == 0) delete this; return tmp; }

    int GetSerializedLength();
    void Serialize(uint8 *target);

	inline bool IsWriteable() const { return m_bWriteable; }
	inline bool IsReadOnly() const { return !m_bWriteable; }
	inline bool CanChangeSize() const { return m_bCanChangeSize; }
	inline bool IsMemoryMapped() const { return m_bMemoryMapped; }
	inline bool IsOpen() const { return m_bOpen; }

    virtual HexDoc *GetRootView() { return m_pRootRegion; }
    virtual bool AddRegion(uint64 display_address, uint64 stored_address, uint64 size, wxString info);
	virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData) = 0;
	virtual bool Write(uint64 UNUSED(nIndex), uint32 UNUSED(nSize), const uint8 *UNUSED(pData)) { return false; }
	virtual void Flush() { }

    wxString GetTitle() { return m_title; }

protected:
	bool m_bWriteable;
	bool m_bCanChangeSize;
	bool m_bOpen;
	bool m_bMemoryMapped;
    HexDoc *m_pRootRegion;
    int m_nRefs;
    wxString m_title;
};

class FileDataSource : public DataSource
{
public:
	FileDataSource(LPCSTR filename, bool bReadOnly);
	~FileDataSource();

	virtual void Flush();
    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);

protected:
	HANDLE hFile, hMapping;
	uint8 *m_pData;
};

class ProcMemDataSource : public DataSource
{
public:
	ProcMemDataSource(DWORD pid, wxString procName, bool bReadOnly);
	~ProcMemDataSource();
    virtual bool Read(uint64 nIndex, uint32 nSize, uint8 *pData);

    HANDLE hProcess;
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

    bool IsSet() { return (iDigit >= 0); }
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

    enum { SUPPRESS_UPDATE = 1 }; // for InsertAt/RemoveAt flags

    inline THSIZE GetSize() const { return size; }
	uint64 size, display_address;
    wxString info;
    Segment *m_rootSeg, *m_curSeg;
    THSIZE m_iCurSegOffset;
    bool bWriteable;
    bool bCanChangeSize;
    DataSource *m_pDS;
    HexDoc *next;

    //! this really shouldn't be here -- where does it belong?  HexView class?
    Selection m_sel;
    THSIZE m_iFirstLine;

    bool IsWriteable() { return bWriteable; }
    bool IsReadOnly() { return !bWriteable; }

    uint64 GetLines(int iLineBytes) const
    {
        return size / iLineBytes + 1; //! includes space at (size+1).  ok if !CanChangeSize?
    }

    bool CanChangeSize() const { return bCanChangeSize; }

    bool Read(uint64 nOffset, uint32 nSize, uint8 *target);

	const inline uint8 operator[](uint64 nIndex)  { return GetAt(nIndex); }
	//uint8& operator[](uint64 nIndex);
	uint8 GetAt(uint64 nIndex);
    bool IsModified(uint64 address);
    bool IsModified();

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

	int find_bytes(uint64 nStart, int ls, const uint8* pb, int lb, int mode, uint8 (*cmp) (uint8, uint8));
    bool Find(const char *text, THSIZE length, int type, bool caseSensitive, THSIZE &start, THSIZE &end);
    bool FindHex(const wxByte *hex, size_t count, /*IN_OUT*/THSIZE &startpos, /*IN_OUT*/THSIZE &endpos);

    Segment *GetSegment(uint64 nIndex, uint64 *pSegmentStart = NULL);

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
	bool CanWriteInPlace();
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
        wxASSERT(pos < doc->GetSize());
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


#endif // _MYDATA_H_