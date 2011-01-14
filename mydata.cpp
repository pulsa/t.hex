#include "precomp.h"
#include "mydata.h"
#include "thex.h" //! for fatal() and WaitObjectList
#include "undo.h"
#include "settings.h"
#include "hexwnd.h"
#include "utils.h"
#include "datasource.h"

extern "C" {
#include "akrip32.h"
#include "scsipt.h"
}

#define new New

Segment::Segment(THSIZE size, THSIZE stored_offset, DataSource *pDS)
{
    this->type = Segment::FILE;
    this->size = size;
    this->stored_offset = stored_offset;
    this->pDS = pDS;
    pDS->AddRef();
    this->pData = NULL;
    this->next = this->prev = NULL;
}

Segment::Segment(THSIZE size)
{
    this->type = Segment::MEM;
    this->size = size;
    this->stored_offset = 0;
    this->pDS = NULL;
    this->pData = new uint8[size];
    this->next = this->prev = NULL;
}

Segment::Segment(THSIZE size, const uint8 *pData)
{
    this->type = Segment::MEM;
    this->size = size;
    this->stored_offset = 0;
    this->pDS = NULL;
    this->next = this->prev = NULL;
    this->pData = new uint8[size];
    if (this->pData)
        memcpy(this->pData, pData, size);
}

Segment::Segment(THSIZE size, uint8 fillData)
{
    this->type = Segment::FILL;
    this->size = size;
    this->stored_offset = fillData;
    this->pDS = NULL;
    this->pData = NULL;
    this->next = this->prev = NULL;
}

Segment::~Segment()
{
    if (pData)
        delete [] pData;
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
    if (type == MEM)
    {
        //! todo: shrink this segment and don't make a new one?
        tmp = new Segment(size - right, pData + right);
        if (!tmp->pData)
            return NULL;
    }
    else if (type == FILE)
    {
        tmp = new Segment(size - right, stored_offset + right, pDS);
    }
    else // type == FILL
        tmp = new Segment(size - right, stored_offset);

    RemoveRight(nOffset);
    return tmp;
}

void Segment::RemoveRight(uint64 nNewSize)
{
    if (pData && nNewSize < size)
    {
        uint8 *pTmpData = new uint8[nNewSize];
        if (pTmpData != NULL)
        {
            memcpy(pTmpData, pData, nNewSize);
            delete [] pData;
            pData = pTmpData;
        }
        // If this allocation fails, we just keep the bigger block.
        // Unlikely, I know.
    }
    size = nNewSize;
}

bool Segment::RemoveLeft(uint64 nRemoveSize)
{
    if (nRemoveSize > size)
        return false; // can't remove more bytes than we own

    if (pData && nRemoveSize > 0)
    {
        uint8 *pTmpData = new uint8[size - nRemoveSize];
        if (pTmpData == NULL)
            return false;
        memcpy(pTmpData, pData + nRemoveSize, size - nRemoveSize);
        delete [] pData;
        pData = pTmpData;
    }
    this->size -= nRemoveSize;
    stored_offset += nRemoveSize;
    if (pData)
        pData += nRemoveSize;
    return true;
}

uint8 Segment::Get(THSIZE offset)
{
    uint8 val = 0;
    Read(offset, 1, &val);
    return val;
}

bool HexDoc::InsertAt(uint64 nIndex, uint8 src, THSIZE nCount /*= 1*/)
{
    return InsertAt(nIndex, &src, 1, nCount);
}

bool HexDoc::InsertAt(THSIZE nIndex, const uint8 *psrc, int nSize, THSIZE nCount /*= 1*/, int flags /*= 0*/)
{
    if (!(flags & SUPPRESS_UPDATE) && !CanReplaceAt(nIndex, 0, nSize * nCount))
        return false;

    if (nSize == 0 || nCount == 0)
        return true; // nothing to do

    //! todo: if we can append to the preceding block, do that instead?

    Segment *ts = new Segment(nSize * nCount);
    if (!ts || !ts->pData)
        return false;

    // copy data to the newly allocated block
    if (nSize == 1)
        memset(ts->pData, *psrc, nCount); //! todo: use fill data?
    else for (int i = 0; i < nCount; i++)
        memcpy(ts->pData + i * nSize, psrc, nSize);

    if (!InsertSegment(nIndex, ts))
        return false;
    m_iChangeIndex++;
    if (hw && !(flags & SUPPRESS_UPDATE))
        hw->OnDataChange(nIndex, 0, nSize * nCount);
    return true;
}

bool HexDoc::InsertSegment(THSIZE nAddress, Segment *newSegment)
{
    if (nAddress == size)
    {
        InsertSegment2(segments.size(), nAddress, newSegment);
        // ... and we're done.  That was easy.
        goto end;
    }    

    int n = FindSegment(nAddress);
    if (n == -1)
        return false; // I don't know how this would happen... maybe nAddress > size?
    THSIZE base = bases[n];
    Segment *ts = segments[n];
    if (nAddress != base) // insert new segment inside existing segment        
    {
        n++;
        Segment* ts2 = ts->Split(nAddress - base);
        //ts2->next = SegmentAt(n);
        //ts->next = newSegment;
        //newSegment->next = ts2;
        //segments.insert(segments.begin() + n, ts2);
        //bases.insert(bases.begin() + n, nAddress); // second part of old segment now starts at nAddress
        InsertSegment2(n, nAddress, ts2);
    }
    //else: insert new segment at beginning of other segment.  n is already correct.

    InsertSegment2(n, nAddress, newSegment);

    //! todo: recover from failed allocation and don't leave data list broken

    // Adjust bases after index n to include newly inserted segment.
    while (++n < (int)bases.size())
        bases[n] += newSegment->size;

    //UpdateSegmentPointers();
    
end:
    this->m_curSeg = NULL;
    this->size += newSegment->size;
    
    return true;
}

bool HexDoc::InsertSegment2(size_t n, THSIZE base, Segment* s)
{
    if (n > 0)
        segments[n - 1]->next = s;
    if (n < bases.size())
        s->next = segments[n];
    else
        s->next = NULL;
    bases.insert(bases.begin() + n, base);
    segments.insert(segments.begin() + n, s);
    return true;
}

bool HexDoc::RemoveSegment(size_t n)
{
    if (n > 0)
    {
        if (n + 1 < segments.size())
            segments[n - 1]->next = segments[n + 1];
        else
            segments[n - 1]->next = NULL;
    }
    segments.erase(segments.begin() + n);
    bases.erase(bases.begin() + n);
    return true;
}

bool HexDoc::RemoveAt(uint64 nIndex, uint64 nSize, int flags /*= 0*/)
{
    if (!(flags & SUPPRESS_UPDATE) && !CanReplaceAt(nIndex, nSize, 0))
        return false;

    m_curSeg = NULL;

    if (nIndex == this->size)
        return nSize == 0;

    int n = FindSegment(nIndex), firstAffected = -1;
    THSIZE base = bases[n];
    const THSIZE removeSize = nSize; // nSize gets modified

    while (nSize)
    {
        Segment* ts = segments[n];
        if (ts->contains(nIndex - 1, base) && ts->contains(nIndex + nSize, base))
        { //---xxx---
            n++;
            Segment* ts2 = ts->RemoveMid(nIndex - base, nSize);
            InsertSegment2(n, nIndex, ts2);
            firstAffected = n + 1; // skip updating the newly inserted segment
            break;
        }
        else if (ts->contains(nIndex - 1, base))
        { // ---xxx
            nSize -= ts->size - (nIndex - base);
            ts->RemoveRight(nIndex - base);
            base += ts->size;
            firstAffected = n;
            n++;
        }
        else if (ts->contains(nIndex + nSize, base))
        { // xxx---
            ts->RemoveLeft(nSize);
            if (firstAffected == -1)
                firstAffected = n + 1;
            break;
        }
        else
        { // xxx
            nSize -= ts->size;
            delete ts;
            RemoveSegment(n);
            if (firstAffected == -1)
                firstAffected = n;
        }
    }

    m_iChangeIndex++;
    if (hw && !(flags & SUPPRESS_UPDATE))
        hw->OnDataChange(nIndex, removeSize, 0);

    if (firstAffected == -1)
        firstAffected = 0;
    for (n = firstAffected; n < (int)bases.size(); n++)
        bases[n] -= removeSize; //! this belongs somewhere else, or at least inside SUPPRESS_UPDATE check.

    this->size -= removeSize;

    return true;
}

bool HexDoc::ReplaceAt(uint64 ToReplaceIndex, uint8 ReplaceWith)
{
    return ReplaceAt(ToReplaceIndex, 1, &ReplaceWith, 1);
}

bool HexDoc::ReplaceAt(uint64 ToReplaceIndex, uint32 ToReplaceLength, const uint8 *pReplaceWith, uint32 ReplaceWithLength)
{
    if (!CanReplaceAt(ToReplaceIndex, ToReplaceLength, ReplaceWithLength))
        return false;

    if (ToReplaceLength > ReplaceWithLength)
    {
        //! is this a good idea?
        RemoveAt(ToReplaceIndex + ReplaceWithLength, ToReplaceLength - ReplaceWithLength, SUPPRESS_UPDATE);
        ToReplaceLength = ReplaceWithLength;
    }

    // Quick optimization:
    // If affected range is entirely inside one writeable Segment and size doesn't change,
    // just write data.
    //! is this a good idea?
    uint64 segmentStart;
    Segment *ts = GetSegment(ToReplaceIndex, &segmentStart);
    if (ts && ts->type == Segment::MEM && ToReplaceLength == ReplaceWithLength)
    {
        if (ts->contains(ToReplaceIndex, ToReplaceLength, segmentStart))
        {
            memcpy(&ts->pData[ToReplaceIndex - segmentStart], pReplaceWith, ToReplaceLength);
            return true;
        }
    }

    if (!RemoveAt(ToReplaceIndex, ToReplaceLength, SUPPRESS_UPDATE))
        return false;
    if (!InsertAt(ToReplaceIndex, pReplaceWith, ReplaceWithLength, 1, SUPPRESS_UPDATE))
        return false;

/*    THSIZE lengthDiff = (THSIZE)ToReplaceLength - (THSIZE)ReplaceWithLength; // overflow doesn't matter
    for (size_t n = FindModified(ToReplaceIndex, true); n < modRanges.size(); n++)
    {
        ByteRange& tr = modRanges[n];
        if (tr.start > ToReplaceIndex)
            tr.start += lengthDiff;
        tr.end += lengthDiff;
    }*/

    m_iChangeIndex++;
    if (hw)
        hw->OnDataChange(ToReplaceIndex, ToReplaceLength, ReplaceWithLength);
    return true;
}

HexDoc::HexDoc()
{
    m_pDS = NULL;
    m_curSeg = NULL;
    next = NULL;
    undo = new UndoManager(0x100000, true);
    m_iFirstLine = 0;
    m_iChangeIndex = 0;
    dwFlags = 0;
    size = 0;
}

HexDoc::~HexDoc()
{
    //! Can closing our stuff ever fail?  Do we care?
    SEGMENTVEC::iterator iter;
    for (iter = segments.begin(); iter != segments.end(); iter++)
    {
        delete *iter;
    }

    if (m_pDS)
        m_pDS->Release();
    m_pDS = NULL;

    delete undo;
    undo = NULL;
}

/*uint8 &DataView::operator [](uint64 nIndex)
{
    if (!IsByteLoaded(nIndex))
        if ((m_pLoadedData = Load(nIndex, 0)) == NULL)
            return m_scratch; //! should return error condition somehow
    return m_pLoadedData[nIndex - m_nLoadedStart];
}*/

uint8 HexDoc::GetAt(uint64 nIndex)
{
    uint8 data;
    if (!Read(nIndex, 1, &data))
        return 0;
    return data;
}

//const uint8* DataView::Load(uint64 nIndex, uint32 nSize)
//{
//	if (!OK())
//		return NULL;
//	if (nIndex >= GetSize())
//		return NULL;
//
//	//! is here the right place for this?
//	if (nIndex + nSize < nIndex)
//		return NULL;
//
//	//if (nSize == 0)
//	//	nSize = min(4096, GetSize() - nIndex);
//
//	if (IsRangeLoaded(nIndex, nSize))
//		goto done;
//
//	// find range containing starting byte
//	ByteRange *tr = GetRange(nIndex);
//
//	if (tr == NULL)
//		return NULL; //! we should never hit this
//
//	// See if the entire range we want is already in memory.
//	if (tr->pData != NULL && tr->contains(nIndex, nSize))
//	{ // If so, just point to it.
//		m_curRange = tr;
//		m_bCurRangeIsDuplicate = false;
//	}
//	else
//	{
//		m_tmpRange.Init(ByteRange::MEM, nSize, nIndex, 0, NULL, true);
//		ReadFromRange(nIndex, nSize, m_tmpRange.pData, tr);
//		m_curRange = &m_tmpRange;
//		m_bCurRangeIsDuplicate = true;
//	}
//
//done:
//	return &m_curRange->pData[nIndex - m_curRange->virtual_offset];
//}

bool HexDoc::Load(uint64 nIndex)
{
    if (nIndex >= size)
        return NULL;

    if (IsByteLoaded(nIndex))
        return true;

    m_curSeg = GetSegment(nIndex, &m_iCurSegOffset);
    return (m_curSeg != NULL);
}

Segment* HexDoc::GetSegment(uint64 nIndex, uint64 *pSegmentStart)
{
    int n = FindSegment(nIndex);
    if (n < 0)
        return NULL;
    if (pSegmentStart)
        *pSegmentStart = bases[n];
    return segments[n];
}

int HexDoc::FindSegment(THSIZE nIndex)
{
    if (nIndex >= size)
        return -1; //! OK?
    // binary search through vectors
    int lo = 0;
    int hi = bases.size() - 1;
    while (hi >= lo)
    {
        int mid = (hi + lo) / 2;
        THSIZE base = bases[mid];
        THSIZE size = segments[mid]->size;
        if (nIndex < base)
            hi = mid - 1;
        else if (nIndex >= base + size)
            lo = mid + 1;
        else // nIndex >= base && nIndex < base + size
            return mid;
    }
    return -1;
}

uint8* HexDoc::LoadWriteable(uint64 nIndex, uint32 nSize /*= 0*/)
{
    return NULL; //! need implementation
}

bool HexDoc::Read(uint64 nOffset, uint32 nSize, uint8 *target, uint8 *pModified /*= NULL*/)
{
    if (nOffset + nSize > this->size)
        return false;

    if (!Load(nOffset))
        return false;
    Segment *ts = m_curSeg;

    uint32 copySize, dstOffset = 0;
    uint64 srcOffset = nOffset - m_iCurSegOffset;
    uint32 remaining = nSize;
    while (remaining > 0)
    {
        if (ts == NULL)
            return false; //! Help!
        copySize = min(remaining, ts->size - srcOffset);
        if (!ts->Read(srcOffset, copySize, target + dstOffset))
            return false;
        dstOffset += copySize;
        srcOffset = 0;
        remaining -= copySize;
        ts = ts->next;
    }

    if (pModified)
    {
        for (uint32 n = 0; n < nSize; n++)
        {
            pModified[n] = IsModified(nOffset + n); //! much room for improvement here.
        }
    }

    return true;
}

bool HexDoc::SaveRange(LPCSTR filename, uint64 begin, uint64 length)
{
    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    bool rc = WriteRange(hFile, begin, length);
    CloseHandle(hFile);
    return rc;
}

bool HexDoc::WriteRange(HANDLE hOutput, uint64 begin, uint64 length)
{
    // bad name.  Maybe SaveRange is wrong too?  (okay with Segments and Regions)
    //! should be in new thread.
    //! write in 1MB chunks and report to GUI thread.
    uint8 buffer[4096];
    DWORD cbWrite;
    for (DWORD offset = 0; offset < length; offset += 4096)
    {
        DWORD tmpSize = min(length - offset, 4096);
        if (!Read(begin + offset, tmpSize, buffer))
            return false;
        if (!WriteFile(hOutput, buffer, tmpSize, &cbWrite, NULL))
            return false;
    }
    return true;
}

bool HexDoc::Save()
{
    //! need a temp file here.
    //! also should be in separate thread.
    //! write in 1MB chunks and report to GUI thread.
    if (CanWriteInPlace())
    {
        //! no size changes, no other data sources
        //! call for each region
        m_pDS->Flush();
        return true;
    }
    else
    {
        //! save-as temp file, delete and rename, etc.
        //! this could be optimized if the first half of the file hasn't been changed,
        //!  or we could find a way where nothing overlaps.
        return false;
    }
}

bool HexDoc::ReadNumber(uint64 nIndex, int nBytes, void *target)
{
    //! I don't think this will work on big-endian machines.
    if (!Read(nIndex, nBytes, (uint8*)target)) return false;
    if (pSettings->iEndianMode != NATIVE_ENDIAN_MODE)
        reverse((uint8*)target, nBytes);
    return true;
}

bool HexDoc::CanWriteInPlace()
{
    //ByteRange *tr = m_rootRange;
    //while (tr != NULL)
    //{
    //	if (tr->type != ByteRange::MEM && tr->virtual_offset != tr->stored_offset)
    //		return false;
    //}
    //return true;

    return false; //! need implementation
}

bool HexDoc::IsModified()
{
    return false;
}

bool HexDoc::IsModified(uint64 address)
{
    return FindModified(address) >= 0;
    //return !(address % 17);
}

int HexDoc::FindModified(THSIZE nAddress, bool wantIndexAnyway /*= false*/)
{
    // binary search through ByteRanges.
    // Could probably use same function to search Segments.
    int lo = 0;
    int hi = modRanges.size();
    int mid = 0;
    while (hi > lo)
    {
        mid = (hi + lo) / 2;
        ByteRange& range = modRanges[mid];
        if (nAddress < range.start)
            hi = mid;
        else if (nAddress >= range.end)
            lo = mid + 1;
        else // nAddress >= start && nAddress < end
            return mid;
    }
    if (wantIndexAnyway)
        return lo;
    return -1;
}

int HexDoc::FindModified(const ByteRange& range, int& count, bool includeAdjacent)
{
    //! this is not used.  It's pretty but sub-optimal.
    // binary search through ByteRanges.
    // Returns the index of the modRange at or after range.
    // count is set to the number of modRanges that overlap range.
    // if includeAdjacent is set, touching on either end counts as overlap.
    int lo = 0;
    int hi = modRanges.size();
    int mid = 0;
    while (hi > lo)
    {
        mid = (hi + lo) / 2;
        ByteRange& tr = modRanges[mid];
        if (range.start < tr.start)
            hi = mid;
        else if (range.start >= tr.end)
            lo = mid + 1;
        else // range.start >= tr.start && range.start < tr.end
            break;
    }
    if (lo < hi) // if we found a range, use it.  Otherwise lo is correct.
        lo = mid;
    hi = lo;

    // count modRanges that overlap range.
    //! todo: binary search for ending range?  Someday it might make sense.
    if (includeAdjacent &&
        lo > 0 &&
        modRanges[lo - 1].end == range.start)
    {
            lo--;
    }
    while (hi < (int)modRanges.size() && modRanges[hi].start < range.end)
        hi++;
    if (includeAdjacent && hi < (int)modRanges.size() && modRanges[hi].start == range.end)
        hi++;
    count = hi - lo;
    return lo;
}

void HexDoc::MarkModified(const ByteRange& range)
{    
    //! This looks real slick, but it traverses the array twice.
    //int count, m = FindModified(range, count, true);
    //if (count > 0)
    //{
    //    ByteRange& r1 = modRanges[m];
    //    r1.start = wxMin(r1.start, range.start);
    //    ByteRange& r2 = modRanges[m + count - 1];
    //    r1.end = wxMax(r2.end, range.end);

    //    while (--count)
    //        modRanges.erase(modRanges.begin() + m + count);
    //}
    //else // no overlap -- insert new range
    //    modRanges.insert(modRanges.begin() + m, range);
    //return;
    //! The ugly code below is more efficient here.

    int n = FindModified(range.start, true);

    // See if new range directly follows another one.  This happens a lot.
    if (n > 0 && modRanges[n - 1].end == range.start)
        n--;

    // If the new range is after all the other ones and not connected, just append it.
    if (n >= (int)modRanges.size())
    {
        modRanges.push_back(range);
        return;
    }

    // If there are one or more overlapping ranges, adjust the endpoints
    // of one and delete the rest.
    //   |-----------|   range
    // ================= 1 -- no action
    //                 = 2 -- no overlap; insert new range
    //     ============= 3 -- single range to modify
    // =====       ????? 4 -- look for more ranges
    //     ===     ????? 5 -- adjust tr.start and look for more ranges

    // If there are no overlapping ranges, insert a new one.
    // Does the first range contain range.start?
    ByteRange& tr = modRanges[n];
    if (tr.start <= range.start) // range.start is inside modRanges[n]
    {
        // Does it also contain range.end? (1)
        if (tr.end >= range.end)
            return; // nothing to do
        // else: combine other ranges until range.end (4)
        tr.end = range.end;
    }
    else if (tr.start > range.end) // no overlap with existing range (2)
    {
        // insert new range and quit
        modRanges.insert(modRanges.begin() + n, range);
        return;
    }
    else // tr.start is inside range (3, 5)
    {
        tr.start = range.start;
        if (tr.end >= range.end) // (3)
            return;
        // combine other ranges until range.end (5)
    }

    while (++n < (int)modRanges.size())
    {
        //! kinda screwy loop structure
        ByteRange& tr2 = modRanges[n];
        if (tr2.end <= range.end)
            modRanges.erase(modRanges.begin() + n);
        else if (tr2.start <= range.end)
        {
            tr.end = tr2.end;
            modRanges.erase(modRanges.begin() + n);
            break;
        }
        else
            break;
    }
}

void HexDoc::MarkUnmodified(const ByteRange& range)
{
    //! todo: test
    int n = FindModified(range.start, true);
    while (n < (int)modRanges.size())
    {
        ByteRange& tr = modRanges[n];
        if (tr.start >= range.end)
            break;
        // We know that there is some overlap.
        if (tr.start >= range.start && tr.end <= range.end) // tr completely inside range
            modRanges.erase(modRanges.begin() + n);
        else if (tr.start < range.start && tr.end > range.end) // range completely inside tr
        {
            ByteRange tr2 = ByteRange::fromEnd(range.end, tr.end);
            tr.end = range.start;
            modRanges.insert(modRanges.begin() + n + 1, tr2);
            break; // no more ranges inside range
        }
        else if (tr.start < range.start) // overlap on right side of tr
        {
            tr.end = range.start;
            n++;
        }
        else // overlap on left side of tr
        {
            tr.start = range.end;
            break;
        }
    }
}

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
    if (hdr.endianMode != NATIVE_ENDIAN_MODE)
    {
        reverse(&hdr.nSegments);
        reverse(&hdr.nSources);
    }
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
        if (hdr.endianMode != NATIVE_ENDIAN_MODE)
        {
            reverse(&seg.offset);
            reverse(&seg.size);
            reverse(&seg.src);
        }
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

int HexDoc::GetSerializedLength(THSIZE nOffset, THSIZE nSize)
{
    int sSize = sizeof(SerialDataHeader);

    std::vector<DataSource*> sources;
    std::vector<DataSource*>::const_iterator iter;

    THSIZE segStart;
    Segment *ts = GetSegment(nOffset, &segStart);
    THSIZE segOffset = nOffset - segStart;
    while (ts != NULL && segStart < nOffset + nSize)
    {
        int nSource = 0;
        if (ts->pDS)
        {
            iter = std::find(sources.begin(), sources.end(), ts->pDS);
            if (iter == sources.end())
            {
                sources.push_back(ts->pDS);
                sSize += ts->pDS->GetSerializedLength();
            }
        }
        THSIZE copySize = min(nSize, ts->size - segOffset);
        sSize += ts->GetSerializedLength(segOffset, copySize);
        segStart += ts->size;
        segOffset = 0;
        ts = ts->next;
    }
    return sSize;
}

void HexDoc::Serialize(THSIZE nOffset, THSIZE nSize, uint8 *target)
{
    SerialDataHeader &hdr = *(SerialDataHeader*)target;
    int sOffset = sizeof(hdr);
    hdr.endianMode = NATIVE_ENDIAN_MODE;
    hdr.nSegments = 0;
    hdr.nSources = 0;

    std::vector<DataSource*> sources;
    std::vector<DataSource*>::const_iterator iter;

    THSIZE segStart;
    Segment *ts = GetSegment(nOffset, &segStart);
    THSIZE segOffset = nOffset - segStart;
    while (ts != NULL && segStart < nOffset + nSize)
    {
        hdr.nSegments++;
        int nSource = 0;
        if (ts->pDS)
        {
            iter = std::find(sources.begin(), sources.end(), ts->pDS);
            if (iter == sources.end())
            {
                sources.push_back(ts->pDS);
                hdr.nSources++;
            }
        }
        THSIZE copySize = min(nOffset + nSize, segStart + ts->size) - (segStart + segOffset);
        int sSize = ts->GetSerializedLength(segOffset, copySize);
        ts->Serialize(segOffset, copySize, nSource, target + sOffset);
        sOffset += sSize;
        segStart += ts->size;
        segOffset = 0;
        ts = ts->next;
    }

    for (iter = sources.begin(); iter < sources.end(); iter++)
    {
        DataSource *pDS = *iter;
        int sSize = pDS->GetSerializedLength();
        pDS->Serialize(target + sOffset);
        sOffset += sSize;
    }
}

wxString HexDoc::SerializeData(uint8* pData, size_t nSize)
{
    wxString str;
    size_t nDataSize = nSize + sizeof(SerialDataHeader) + sizeof(SerialDataSegment);
    char *target = str.GetWriteBuf(nDataSize);

    // Fill in serial data header.  One MEM-type segment, no sources.
    SerialDataHeader &hdr = *(SerialDataHeader*)target;
    int sOffset = sizeof(hdr);
    hdr.endianMode = NATIVE_ENDIAN_MODE;
    hdr.nSegments = 1;
    hdr.nSources = 0;

    // Fill in the serial data segment.
    SerialDataSegment &seg = *(SerialDataSegment*)(target + sOffset);
    seg.offset = 0;
    seg.size = nSize;
    seg.src = -1; // Immediate data follows.
    memcpy(target + sOffset, pData, nSize);

    str.UngetWriteBuf(nDataSize);
    return str;
}

int Segment::GetSerializedLength(THSIZE nOffset, THSIZE nSize)
{
    int size = sizeof(SerialDataSegment);
    if (type == MEM)
        size += nSize;
    return size;
}

void Segment::Serialize(THSIZE nOffset, THSIZE nSize, int nSource, uint8 *target)
{
    SerialDataSegment *pSeg = (SerialDataSegment *) target;
    pSeg->offset = this->stored_offset + nOffset;
    pSeg->size = nSize;
    if (type == MEM)
        pSeg->src = -1;
    else //! todo: fill data
        pSeg->src = nSource;
    if (type == MEM)
        memcpy(pSeg->data, pData + nOffset, nSize);
}

Segment* Segment::Unserialize(SerialData sdata, int iSeg)
{
    SerialDataSegment seg;
    const uint8* memData;
    if (!sdata.GetSegment(iSeg, seg, &memData))
        return NULL;

    if (seg.src == -1) // MEM type
    {
        Segment *ts = new Segment(seg.size, memData);
        return ts;
    }
    else if (seg.src == -2) // FILL type
    {
        return new Segment(seg.size, seg.offset);
    }
    else
    {
        SerialDataSource src;
        sdata.GetSource(seg.src, src);
        return new Segment(seg.size, seg.offset, src.pDS);
    }
}

bool HexDoc::ReplaceSerialized(THSIZE nAddress, THSIZE ToReplaceLength, SerialData &sInsert)
{
    if (!sInsert.Ok()) // not an error if len == 0
        return false;

    if (!CanReplaceAt(nAddress, ToReplaceLength, sInsert.m_nTotalSize))
        return false;

    if (!RemoveAt(nAddress, ToReplaceLength, SUPPRESS_UPDATE))
        return false;

    //! todo: get data sources

    for (int iSeg = 0; iSeg < sInsert.hdr.nSegments; iSeg++)
    {
        Segment *ts = Segment::Unserialize(sInsert, iSeg);
        InsertSegment(nAddress, ts);
        nAddress += ts->size;
    }

    m_iChangeIndex++;
    if (hw)
        hw->OnDataChange(nAddress, ToReplaceLength, sInsert.m_nTotalSize);
    return true;
}

//! todo: don't store serialized data in clipboard.  Instead, store 0 bytes of custom data.
//! Monitor clipboard for changes.  If someone else sets clipboard data, erase SerialData
//  and Release() all data sources used.

bool Segment::Read(THSIZE nOffset, THSIZE nSize, uint8 *target)
{
    //! todo: parameter checking
    if (type == FILE)
        return pDS->Read(stored_offset + nOffset, nSize, target);
    else if (type == MEM)
        memcpy(target, pData + nOffset, nSize);
    else if (type == FILL)
        memset(target, stored_offset, nSize);
    else
        return false; //! should never happen
    return true;
}

bool HexDoc::Undo()
{
    if (!undo)
        return false;
    if (!undo->CanUndo())
        return false;
    return undo->GetUndo()->Undo(this);
}

bool HexDoc::Redo()
{
    if (!undo)
        return false;
    if (!undo->CanRedo())
        return false;
    return undo->GetRedo()->Redo(this);
}

bool HexDoc::CanReplaceAt(uint64 nIndex, uint64 nOldSize, uint64 nNewSize)
{
    if (!IsWriteable() && (nOldSize || nNewSize))
        return false; // not writeable
    if (!CanChangeSize() && nOldSize != nNewSize)
        return false; // can't change size
    if (nIndex + nOldSize > this->size)
        return false; // can't delete past EOF
    return true;
}

bool HexDoc::Find(const char *text, THSIZE length, int type, bool caseSensitive, THSIZE &start, THSIZE &end)
{
    if (type == 0)
        return FindHex((const wxByte*)text, length, start, end);
    return false;
}

wxString HexDoc::ReadString(THSIZE nIndex, size_t nSize)
{
    wxString str;
    uint8 *data = (uint8*)str.GetWriteBuf(nSize);
    if (!Read(nIndex, nSize, data))
        nSize = 0;
    str.UngetWriteBuf(nSize);
    return str;
}

wxString HexDoc::ReadStringW(THSIZE nIndex, size_t nSize)
{
    wxString str;
    wchar_t *data = new wchar_t[nSize]; //! bad
    if (Read(nIndex * sizeof(wchar_t), nSize * sizeof(wchar_t), (uint8*)data))
        str = wxString(data, wxConvLibc, nSize);
    delete [] data;
    return str;
}


//class RangeNode
//{
//public:
//   THSIZE csize;   // size of the block at this node
//   THSIZE lsize;   // size of all the blocks in the left branch
//   THSIZE rsize;   // size of all the blocks in the right branch
//   RangeNode *left, *right, *parent;
//   Segment *data;
//   int depth;
//
//   RangeNode(THSIZE size, Segment *data, RangeNode *parent)
//   {
//      lsize = rsize = 0;
//      csize = size;
//      this->data = data;
//      depth = 0;
//      this->parent = parent;
//      left = right = NULL;
//   }
//
//   // index is address relative to this node.
//   // base gets modified if we use this node or take a right branch.
//   RangeNode* find(THSIZE index, THSIZE &base)
//   {
//      if (index < lsize)
//         return left->find(index, base);
//      if (index < lsize + csize)
//      {
//         base += lsize;
//         return this;
//      }
//      if (!right)
//      {
//         // This returns the highest node if the node just past the total size is requested.
//         if (index == lsize + csize)
//         {
//            base += lsize;
//            return this;
//         }
//         return NULL;
//      }
//      base += lsize + csize;
//      return right->find(index - (lsize + csize), base);
//   }
//
//   void insert(THSIZE start, THSIZE size, Segment *data)
//   {
//      if (start <= lsize)
//      {
//         if (left)
//         {
//            left->insert(start, size, data);
//            lsize += size;
//         }
//         else
//         {
//            // make a new left node here
//            left = new RangeNode(size, data, this);
//            lsize = size;
//            csize -= size;
//         }
//      }
//      else if (start < lsize + csize)
//      {
//         // split this node into newleft and newright.
//         Segment *newleft, *newright;
//         this->data->Split(start, newleft, newright);
//         // insert newleft at left edge, newright at right edge.
//         insert(lsize, newleft->size, newleft);
//         insert(lsize + csize, newright->size, newright);
//         this->data->Release();
//         this->data = data;
//         data->AddRef();
//         csize = size;
//      }
//      else if (start <= lsize + csize + rsize)
//      {
//         if (right)
//         {
//            right->insert(start - (lsize + csize), size, data);
//            rsize += size;
//         }
//         else
//         {
//            // make a new right node here
//            right = new RangeNode(size, data, this);
//            rsize = size;
//         }
//      }
//      //! else:  start > lsize + csize + rsize.  Error.
//      //! Need rebalancing here.
//   }
//
//   void remove(THSIZE start, THSIZE size)
//   {
//      THSIZE dsize;
//      if (start < lsize)
//      {
//         // delete from left
//         dsize = min(size, lsize - start);
//         if (dsize == lsize)
//         {
//            delete left;
//            left = NULL;
//         }
//         else
//            left->remove(start, dsize);
//         lsize -= dsize;
//         size -= dsize;
//         start += dsize;
//      }
//
//      if (start + size > lsize + csize)
//      {
//         // delete from right
//         THSIZE dstart = max(start, lsize + csize);
//         dsize = start + size - dstart;
//         if (dsize == rsize)
//         {
//            delete right;
//            right = NULL;
//         }
//         else
//            right->remove(dstart - (lsize + csize), dsize);
//         rsize -= dsize;
//         size -= dsize;
//      }
//
//      if (size)
//      {
//         // delete from center
//         if (size == csize)
//         {
//            // delete whole center and replace with node from left or right.
//            data->Release();
//            data = NULL;
//            csize = 0;
//            if (right)
//            {
//               THSIZE base = 0;
//               RangeNode *node = right->find(0, base);
//               data = node->data;
//               data->AddRef();
//               right->remove(0, data->size);
//               csize = data->size;
//            }
//            else if (left)
//            {
//               THSIZE base = 0;
//               RangeNode *node = left->find(0, base);
//               data = node->data;
//               data->AddRef();
//               left->remove(0, data->size);
//               csize = data->size;
//            }
//            else
//            {
//               if (parent)
//               {
//                  if (parent->left == this)
//                     parent->left = NULL;
//                  else
//                     parent->right = NULL;
//               }
//               else
//               {
//                  // This is the root of the tree, and our last data range has been deleted.
//                  // What now?
//               }
//            }
//         }
//         else // size < csize
//         {
//            if (start == lsize)
//               data->RemoveLeft(size);
//            else if (start + size == lsize + csize)
//               data->RemoveRight(size);
//            else
//            {
//               // split node into two pieces
//               Segment *seg2;
//               data->RemoveMid(start - lsize, size, seg2);
//               insert(lsize + csize, seg2->size, seg2);
//               csize = data->size;
//            }
//         }
//      }
//   }
//};

//class RangeNode2
//{
//public:
//   THSIZE size;    // size of left + right + center
//   THSIZE center;
//   RangeNode *left, *right, *parent;
//   void *data;
//
//   RangeNode(THSIZE size) : size(size), left(NULL), right(NULL) {}
//
//   // index is address relative to this node.
//   // base gets modified if we use this node or take a right branch.
//   void* find(THSIZE index, THSIZE &base)
//   {
//      if (index > size)
//         return NULL;
//      if (left)
//      {
//         if (index < left->size)
//            return left->find(index, base);
//      }
//      if (right && index >= size - right->size)
//      {
//         base += center;
//         index -= center;
//         return right->find(index, base);
//      }
//      base +=
//      return data;
//   }

