#include "precomp.h"
#include "hexdoc.h"
#include "thex.h" //! for fatal() and WaitObjectList
#include "undo.h"
#include "settings.h"
#include "hexwnd.h"
#include "utils.h"
#include "datasource.h"

#define new New

Segment::Segment(THSIZE size, THSIZE stored_offset, DataSource *pDS, THSIZE nCount /*= 1*/)
{
    //this->type = Segment::FILE;
    this->size = size * nCount;
    this->srcSize = size;
    this->stored_offset = stored_offset;
    this->pDS = pDS;
    this->fill = (nCount > 1);
    if (pDS)
        pDS->AddRef();
    //this->pData = NULL;
    this->next = this->prev = NULL;
}

Segment::~Segment()
{
    //if (pData)
    //    delete [] pData;
    if (pDS)
        pDS->Release();
    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
}

//void Segment::InsertBefore(Segment *ts)
//{
//    if (prev)
//    {
//        prev->next = ts;
//        ts->prev = prev;
//    }
//    ts->next = this;
//    this->prev = ts;
//}
//
//void Segment::InsertAfter(Segment *ts)
//{
//    if (next)
//    {
//        next->prev = ts;
//        ts->next = next;
//    }
//    ts->prev = this;
//    this->next = ts;
//}

Segment* Segment::RemoveMid(THSIZE nOffset, THSIZE nSize)
{
    THSIZE right = nOffset + nSize;

    if (right > size)
        return NULL;

    Segment *tmp;
    if (fill)
    {
        //! Fuck.
        return NULL;
    }
    else
        tmp = new Segment(size - right, stored_offset + right, pDS);

    RemoveRight(nOffset);
    return tmp;
}

void Segment::RemoveRight(uint64 nNewSize)
{
    size = nNewSize;
}

bool Segment::RemoveLeft(uint64 nRemoveSize)
{
    if (nRemoveSize > size)
        return false; // can't remove more bytes than we own

    this->size -= nRemoveSize;
    if (fill)
    {
        //! todo: handle fill data from non-zero offset
    }
    else
        stored_offset += nRemoveSize;
    return true;
}

//! This doesn't look very safe.  More thought needed.
void Segment::ExtendForward(uint64 nAddSize)
{
    this->size += nAddSize;
}

void Segment::ExtendBackward(uint64 nAddSize)
{
    this->size += nAddSize;
    if (!fill)
        stored_offset -= nAddSize;
}

uint8 Segment::Get(THSIZE offset)
{
    uint8 val = 0;
    Read(offset, 1, &val);
    return val;
}


int Segment::GetSerializedLength(THSIZE nOffset, THSIZE nSize)
{
    int size = sizeof(SerialDataSegment);
    return size;
}

void Segment::Serialize(THSIZE nOffset, THSIZE nSize, int nSource, uint8 *target)
{
    //SerialDataSegment *pSeg = (SerialDataSegment *) target;
    //pSeg->offset = this->stored_offset + nOffset;
    //pSeg->size = nSize;
    //pSeg->src = nSource;
    //! todo: handle fill data.  This doesn't work.

    SerialDataSegment *st = (SerialDataSegment*)target;
    memcpy(st, this, sizeof(SerialDataSegment));
    st->stored_offset += nOffset;
    //st->size -= nOffset;  // wtf?!  2008-07-24
    st->size = st->srcSize = nSize;
}

Segment* Segment::Unserialize(SerialData sdata, int iSeg)
{
    SerialDataSegment seg;
    const uint8* memData;
    if (!sdata.GetSegment(iSeg, seg, &memData))
        return NULL;

    {
        SerialDataSource src;
        sdata.GetSource(seg.src, src);
        //return new Segment(seg.size, seg.offset, src.pDS);
        return new Segment(seg.srcSize, seg.stored_offset, src.pDS);
    }
    //! todo: handle fill data
}


//! todo: don't store serialized data in clipboard.  Instead, store 0 bytes of custom data.
//! Monitor clipboard for changes.  If someone else sets clipboard data, erase SerialData
//  and Release() all data sources used.

bool Segment::Read(THSIZE nOffset, THSIZE nSize, uint8 *target)
{
    //! todo: parameter checking
    if (!fill)  // straight copy from data source
        return pDS->Read(stored_offset + nOffset, nSize, target);
    THSIZE fillOffset = nOffset % srcSize;
    THSIZE size1 = wxMin(srcSize - fillOffset, nSize);
    if (!pDS->Read(stored_offset + fillOffset, size1, target))
        return false;
    if (size1 == nSize)
        return true;
    THSIZE size2 = wxMin(nSize, srcSize) - size1;
    if (!pDS->Read(stored_offset, size2, target + size1))
        return false;
    if (size2 + size1 == nSize)
        return true;
    // replicate data
    if (srcSize == 1)
        memset(target + 1, *target, nSize - 1);
    else {
        THSIZE count = nSize / srcSize;
        uint8 *next = target + srcSize;
        while (--count)
        {
            memcpy(next, target, srcSize);
            next += srcSize;
        }
        if (nSize % srcSize)
            memcpy(next, target, nSize % srcSize);
    }
    return true;
}

// return pointer to data, if memory-mapped
const uint8* Segment::GetBasePointer(THSIZE nOffset, THSIZE nSize)
{
    if (Add(nOffset, nSize) > this->size) return false;
    if (pDS)
        return pDS->GetBasePointer(stored_offset + nOffset, nSize);
    return NULL;
}

//*****************************************************************************
//*****************************************************************************
// SerialData
//*****************************************************************************
//*****************************************************************************


SerialData::SerialData(const uint8 *data, int len)
{
    Init(data, len);
}

SerialData::SerialData(wxString data)
{
    this->strData = data;
    Init((const uint8*)data.c_str(), data.Len());
}

void SerialData::Init(const uint8* data, int len)
{
    this->data = data;
    this->len = len;
    loaded = false;
    this->offset = 0;
    if (!Read(&hdr, sizeof(hdr)))
    {
        Clear();
        return; // invalid serial data
    }
    //if (hdr.endianMode != NATIVE_ENDIAN_MODE)
    //{
    //    reverse(&hdr.nSegments);
    //    reverse(&hdr.nSources);
    //}
    m_nTotalSize = 0;
    for (int i = 0; i < hdr.nSegments; i++)
    {
        SerialDataSegment seg;
        if (!GetSegment(i, seg))
        {
            Clear();
            return;
        }
        m_nTotalSize += seg.size;
    }
    m_nSourceOffset = offset;
    for (int i = 0; i < hdr.nSources; i++)
    {
        SerialDataSource sds;
        if (!GetSource(i, sds))
        {
            Clear();
            return;
        }
    }
    loaded = true;
}

void SerialData::Clear()
{
    loaded = false;
    m_nTotalSize = 0;
    strData.Empty();
    data = NULL;
    // don't change len -- we use that to see if there was data
    hdr.nSegments = 0;
    hdr.nSources = 0;
}

bool SerialData::Read(void *target, int size)
{
    if (offset + size > len)
        return false;
    memcpy(target, data + offset, size);
    offset += size;
    return true;
}

bool SerialData::GetSegment(int nSegment, SerialDataSegment &seg, const uint8 **ppData /*= NULL*/)
{
    offset = sizeof(SerialDataHeader);
    while (nSegment >= 0)
    {
        if (!Read(&seg, sizeof(seg)))
            return false;
        //if (hdr.endianMode != NATIVE_ENDIAN_MODE)
        //{
        //    reverse(&seg.offset);
        //    reverse(&seg.size);
        //    reverse(&seg.src);
        //}
        if (seg.src < 0) {
            if (offset + seg.size > len)
                return false;
            offset += seg.size;
        }
        if (nSegment == 0)
        {
            if (ppData)
                *ppData = data + offset - seg.size;
            return true;
        }
        nSegment--;
    }
    return false;
}



bool SerialData::GetSource(int nSource, SerialDataSource &src)
{
    offset = m_nSourceOffset;
    while (nSource >= 0)
    {
        if (!Read(&src, sizeof(src)))
            return false;
        //if (hdr.endianMode != NATIVE_ENDIAN_MODE)
        //{
        //    reverse((uint8*)&src.pDS, sizeof(src.pDS));
        //}
        //! it doesn't make much sense to have cross-platform serialized pointers, does it?
        if (nSource == 0)
            return true;
        nSource--;
    }
    return false;
}

bool SerialData::Extract(THSIZE size, uint8* target)
{
    for (int iSeg = 0; iSeg < hdr.nSegments; iSeg++)
    {
        Segment *ts = Segment::Unserialize(*this, iSeg);
        THSIZE blocksize = wxMin(ts->size, size);
        ts->Read(0, blocksize, target);
        target += blocksize;
        size -= blocksize;
        delete ts;
        if (!size) break;
    }
    return true;
}


//*****************************************************************************
//*****************************************************************************
// 
//*****************************************************************************
//*****************************************************************************

ByteIterator2::ByteIterator2(wxFileOffset pos0, HexDoc *doc0)
    :pos(pos0), doc(doc0)
{
    buffer = new uint8[BUFFER_COUNT];
    m_nEnd = doc->GetSize();
    Read();
}

ByteIterator2::ByteIterator2(wxFileOffset pos0, wxFileOffset size0, HexDoc *doc0)
    :pos(pos0), m_nEnd(pos0 + size0), doc(doc0)
{
    buffer = new uint8[BUFFER_COUNT];
    //! size doesn't belong here.  STL style would be to compare with a new iterator.
    if (m_nEnd > doc->GetSize())
        m_nEnd = doc->GetSize();
    Read();
}


void ByteIterator2::Read()
{
    int size = BUFFER_COUNT;
    THSIZE offset = pos & (THSIZE)~BUFFER_MAX;
    if (offset + size > m_nEnd)
        size = m_nEnd - offset;
    doc->Read(offset, size, buffer);
}

//bool ByteIterator2::AtEnd() const { return pos == m_nEnd; }

void WordIterator2::Read()
{
    int size = BUFFER_COUNT * sizeof(value_type);
    THSIZE offset = (pos & (THSIZE)~BUFFER_MAX) * sizeof(value_type);
    if (offset + size > doc->GetSize())
        size = doc->GetSize() - offset;
    doc->Read(offset, size, (uint8*)buffer);
}

bool WordIterator2::AtEnd() const { return pos == doc->GetSize(); }

//*****************************************************************************
//*****************************************************************************
// thDocInputStream
//*****************************************************************************
//*****************************************************************************

thDocInputStream::thDocInputStream(HexDoc *doc, Selection *pSel /*= NULL*/)
{
    m_doc = doc;
    SetSeekable(true);
    //m_doc->AddRef();  // never implemented... should we?
    m_pos = 0;

    if (pSel) {
        m_start = pSel->GetFirst();
        m_size = pSel->GetSize();
    } else {
        m_start = 0;
        m_size = doc->GetSize();
    }
}

bool thDocInputStream::IsOk() const
{
    return m_doc->m_pDS && m_doc->m_pDS->IsOpen();
}

size_t thDocInputStream::OnSysRead(void *buffer, size_t size)
{
    if (!CanAdd(m_start + m_pos, size, m_size))
        size = m_size - (m_start + m_pos);
    if (m_doc->Read(m_start + m_pos, size, (uint8*)buffer))
    {
        m_pos += size;
        return size;
    }
    else
        return 0;
}

wxFileOffset thDocInputStream::OnSysSeek(wxFileOffset pos, wxSeekMode mode)
{
    if (IsSeekable())
        m_pos = pos;

    return OnSysTell();
}

//*****************************************************************************

size_t thNullOutputStream::OnSysWrite(const void * WXUNUSED(buffer), size_t bufsize)
{
    m_pos += bufsize;
    return bufsize;
}
