#include "precomp.h"
#include "hexdoc.h"
#include "thex.h" //! for fatal() and WaitObjectList
#include "undo.h"
#include "settings.h"
#include "hexwnd.h"
#include "utils.h"
#include "datasource.h"

#define new New

//*****************************************************************************
//*****************************************************************************
// Constructor and destructor
//*****************************************************************************
//*****************************************************************************

HexDoc::HexDoc(DataSource *pDS, THSIZE start, THSIZE size, DWORD dwFlags)
{
    m_pDS = pDS;
    pDS->AddRef();
    m_head = new Segment(0, 0, NULL);
    m_tail = new Segment(0, 0, NULL);
    if (size)
    {
        m_current = new Segment(size, start, pDS);
        m_head->next = m_current;
        m_current->prev = m_head;
        m_current->next = m_tail;
        m_tail->prev = m_current;
    }
    else
    {
        m_head->next = m_current = m_tail;
        m_tail->prev = m_head;
    }
    this->size = size;
    this->display_address = start;
    m_curBase = 0;

    undo = new UndoManager(0x100000, true);
    modbuf = new ModifyBuffer();
    m_iFirstLine = 0;
    m_iChangeIndex = 0;
    this->dwFlags = dwFlags;
    NoCache = false;

    m_cacheStart = m_cacheSize = m_cacheBufferSize = 0;
    m_pCacheData = m_pCacheBuffer = NULL;
}

HexDoc::~HexDoc()
{
    //! Can closing our stuff ever fail?  Do we care?
    DeleteSegments();

    free(m_pCacheBuffer);

    if (m_pDS)
        m_pDS->Release();
    m_pDS = NULL;
    if (modbuf)
        modbuf->Release();  //! Horrible reference loop.  Needs work.
    modbuf = NULL;

    delete undo;
    undo = NULL;
}

// delete all segments, including m_head and m_tail
void HexDoc::DeleteSegments()
{
    while (m_head)
    {
        m_current = m_head->next;
        delete m_head;
        m_head = m_current;
    }
}

//*****************************************************************************
//*****************************************************************************
// Basic operations insert, remove, and replace
//*****************************************************************************
//*****************************************************************************

bool HexDoc::CanReplaceAt(uint64 nIndex, uint64 nOldSize, uint64 nNewSize)
{
    if (!IsWriteable() && (nOldSize || nNewSize))
        return false; // not writeable
    if (!CanChangeSize() && nOldSize != nNewSize)
        return false; // can't change size
    if (!CanAdd(nIndex, nOldSize, this->size))
        return false; // can't delete past EOF
    if (!CanAdd(this->size - nOldSize, nNewSize, 0x7FFFFFFFFFFFFFFF))
        return false; // can't end up with really huge document
    return true;
}

// Every call that modifies the document goes through ReplaceAt() or ReplaceSerialized().
bool HexDoc::ReplaceAt(THSIZE ToReplaceIndex, THSIZE ToReplaceLength,
                       const uint8 *pReplaceWith, uint32 ReplaceWithLength,
                       THSIZE nCount /*= 1*/)
{
    if (!CanReplaceAt(ToReplaceIndex, ToReplaceLength, ReplaceWithLength * nCount))
        return false;

    // begin undo
    UndoAction *ua = this->undo->AllocDataChange(this, ToReplaceIndex);
    if (ua)
    {
        ua->removedData = Serialize(ToReplaceIndex, ToReplaceLength);
        if (hw)
            ua->oldSel = hw->GetSelection();
    }

    if (!DoRemoveAt(ToReplaceIndex, ToReplaceLength))
        return false;
    if (!DoInsertAt(ToReplaceIndex, pReplaceWith, ReplaceWithLength, nCount))
        return false;

    // clear the affected range from the cache
    InvalidateCache(ToReplaceIndex, ToReplaceLength, ReplaceWithLength);

    // end undo
    if (ua)
        ua->insertedData = Serialize(ToReplaceIndex, ReplaceWithLength * nCount);

    m_iChangeIndex++;
    if (hw)
        hw->OnDataChange(ToReplaceIndex, ToReplaceLength, ReplaceWithLength);
    return true;
}

bool HexDoc::ReplaceSerialized(THSIZE nAddress, THSIZE ToReplaceLength, SerialData &sInsert,
                               bool bInsideUndo /*= false*/)
{
    if (!sInsert.Ok()) // not an error if len == 0
        return false;

    if (!CanReplaceAt(nAddress, ToReplaceLength, sInsert.m_nTotalSize))
        return false;

    if (!bInsideUndo)
    {
        UndoAction *ua = this->undo->AllocDataChange(this, nAddress);
        if (ua)
        {
            ua->removedData = Serialize(nAddress, ToReplaceLength);
            ua->insertedData = sInsert.GetData();
            if (hw)
                ua->oldSel = hw->GetSelection();
        }
    }

    if (!DoRemoveAt(nAddress, ToReplaceLength))
        return false;

    //! todo: get data sources

    THSIZE tmpAddress = nAddress;
    for (int iSeg = 0; iSeg < sInsert.hdr.nSegments; iSeg++)
    {
        Segment *ts = Segment::Unserialize(sInsert, iSeg);
        DoInsertSegment(tmpAddress, ts);
        tmpAddress += ts->size;
    }

    InvalidateCache(nAddress, ToReplaceLength, sInsert.m_nTotalSize);

    m_iChangeIndex++;
    if (hw)
        hw->OnDataChange(nAddress, ToReplaceLength, sInsert.m_nTotalSize);
    return true;
}

bool HexDoc::DoInsertAt(THSIZE nIndex, const uint8 *psrc, int nSize, THSIZE nCount /*= 1*/)
{
    if (nSize == 0 || nCount == 0)
        return true; // nothing to do

    // todo: if we can append to the preceding block, do that instead?  (ModifyBuffer does that.)

    // copy data to the permanent buffer
        THSIZE tmp = modbuf->GetSize();
            modbuf->Append(psrc, nSize);
    Segment *ts = new Segment(nSize, tmp, modbuf, nCount);

    if (!DoInsertSegment(nIndex, ts))
        return false;
    return true;
}

bool HexDoc::DoRemoveAt(uint64 nIndex, uint64 nSize)
{
    if (nIndex == this->size)
        return nSize == 0;

    MoveToSegment(nIndex);

    const THSIZE removeSize = nSize; // nSize gets modified

    while (nSize)
    {
        //Segment* ts = segments[n];
        Segment* ts = m_current;
        if (nIndex > 0 && ts->contains(nIndex - 1, nSize + 2, m_curBase))
        { //---xxx---
            Segment* ts2 = ts->RemoveMid(nIndex - m_curBase, nSize);
            ts2->prev = ts;
            ts2->next = ts->next;
            ts->next->prev = ts2;
            ts->next = ts2;
            break;
        }
        else if (nIndex > 0 && ts->contains(nIndex - 1, m_curBase))
        { // ---xxx
            nSize -= ts->size - (nIndex - m_curBase);
            ts->RemoveRight(nIndex - m_curBase);
            m_curBase += ts->size;
            m_current = m_current->next;
        }
        else if (ts->contains(nIndex + nSize, m_curBase))
        { // xxx---
            ts->RemoveLeft(nSize);
            break;
        }
        else
        { // xxx
            m_current = m_current->next; // m_curBase doesn't change
            nSize -= ts->size;
            delete ts; //! destructor fixes up the list pointers.  Sneaky, ain't it?
        }
    }

    this->size -= removeSize;

    RejoinSegments();

    return true;
}


bool HexDoc::DoInsertSegment(THSIZE nAddress, Segment *newSegment)
{
    THSIZE addedSize = newSegment->size;
    if (size == 0) // special case for first segment
    {
        m_head->next = m_current = newSegment;
        newSegment->prev = m_head;
        m_tail->prev = newSegment;
        newSegment->next = m_tail;
    }
    else
    {
        if (!MoveToSegment(nAddress))
            return false; // Only happens if nAddress > size, and that should be caught before here
        Segment *prev = m_current->prev;
        if (nAddress != m_curBase) // insert new segment inside existing segment
        {
            Segment* ts2 = m_current->Split(nAddress - m_curBase);
            m_current->next->prev = ts2;
            ts2->next = m_current->next;
            ts2->prev = newSegment;
            newSegment->next = ts2;
            newSegment->prev = m_current;
            m_current->next = newSegment;
        }
        else if (newSegment->fill == false &&
                 prev->fill == false &&
                 newSegment->pDS == prev->pDS &&
                 newSegment->stored_offset == prev->stored_offset + prev->size)
        {
            // extend range of previous segment forward
            prev->ExtendForward(newSegment->size);
            m_curBase += newSegment->size;
            delete newSegment;
            //!!! HOW DO WE UNDO THIS?
        }
        else if (newSegment->fill == false &&
                 prev->fill == false &&
                 newSegment->pDS == m_current->pDS &&
                 newSegment->stored_offset + newSegment->size == m_current->stored_offset)
        {
            // extend range of current segment backward
            m_current->ExtendBackward(newSegment->size);
            m_curBase -= newSegment->size;
            delete newSegment;
            //!!! HOW DO WE UNDO THIS?
        }
        else // insert new segment at beginning of current segment.
        {
            m_current->prev->next = newSegment;
            newSegment->prev = m_current->prev;
            newSegment->next = m_current;
            m_current->prev = newSegment;
            m_current = newSegment;
        }
    }

    RejoinSegments();

    //! todo: recover from failed allocation and don't leave data list broken

    this->size += addedSize;
    return true;
}

// Try to recombine adjacent segments from the same DataSource.
// Whether this is helpful remains to be seen.
bool HexDoc::RejoinSegments()
{
    Segment *prev = m_current->prev;
    if (m_curBase > 0 &&
        m_current->pDS == prev->pDS &&
        m_current->stored_offset == prev->stored_offset + prev->size)
    {
        m_curBase -= prev->size;
        prev->size += m_current->size;
        delete m_current;
        m_current = prev;
        return true;
    }
    return false;
}

//*****************************************************************************
//*****************************************************************************
// Data access
//*****************************************************************************
//*****************************************************************************

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

const uint8* HexDoc::Load(uint64 nIndex, uint32 nSize, uint32 *pCachedSize /*= 0*/)
{
    if (nSize > HEXDOC_BUFFER_SIZE)
        return NULL;  // Error: Caller is greedy.
    if (!Cache(nIndex, nSize))
        return NULL;
    if (pCachedSize)
        *pCachedSize = m_cacheSize - (DWORD_PTR)(nIndex - m_cacheStart);
    return m_pCacheData + (DWORD_PTR)(nIndex - m_cacheStart);
}

bool HexDoc::MoveToSegment(uint64 nIndex)
{
    if (nIndex > size) //! special case of (nIndex == size) is handled later
        return false;

    if (nIndex == 0)
    {
        m_current = m_head->next;
        m_curBase = 0;
        return true;
    }

    if (nIndex < m_curBase)
    {
        while (nIndex < m_curBase)
        {
            wxASSERT(m_current->prev != NULL); // check for underflow
            m_curBase -= m_current->prev->size;
            m_current = m_current->prev;
        }
    }
    else while (m_curBase + m_current->size <= nIndex)
    {
        m_curBase += m_current->size;
        if (!m_current->next)            //! special weirdness for EOF
            break;
        m_current = m_current->next;
    }

    return true;
}

bool HexDoc::Cache(THSIZE nIndex, THSIZE nSize)
{
    //! I don't know yet if we should have more than one cache buffer.
    //  When would we ever need to read from more than one point in the file at once?
    //  Anyway, this function may be an example of premature optimization.
    //  Yes.  This definitely needs some more thought.  Two caches would be nice.

    if (nIndex + nSize > this->size)
        return false;
    //! todo: invalidate cache when data changes
    if (IsRangeCached(nIndex, nSize))
        return true;

    if (NoCache)
    {
        PRINTF(_T("No cache %8I64X, %8I64X\n"), nIndex, nSize);
        return false;
    }

    MoveToSegment(nIndex);

    // Always try to read at least 4k.
    // If we can do that inside one segment, great.  We'll ask for up to 1M.
    // If nSize is greater than that, leave it alone.

    THSIZE nEnd = nIndex + nSize;  // Save this for later.
    //nIndex = Subtract(nIndex, 4096);  //! Grab a little cushion for StringCollectDialog.  Does it help?  Dunno.
    nIndex = RoundDown(nIndex, m_pDS->BlockAlign);  // Adjust nIndex downward to 4k boundary.

    if (nEnd - nIndex < m_pDS->BlockAlign)            // Try to read at least 4k,
        nEnd = wxMin(nIndex + m_pDS->BlockAlign, this->size);  // or until end of document.

    // See if we can get a base pointer.
    if (nIndex >= m_curBase && nEnd <= m_curBase + m_current->size)
    {
        // Adjust nSize upward to 1M or segment end, whichever is closest.
        nSize = wxMax(nEnd, wxMin(nIndex + MEGA + 8192, m_curBase + m_current->size)) - nIndex;
        const uint8 *tmp = m_current->GetBasePointer(nIndex - m_curBase, + nSize);
        if (tmp)  // It will probably be NULL, but maybe we'll get lucky.
        {
            m_pCacheData = tmp;
            //PRINTF("Got base pointer at %I64X, size %I64X\n", nIndex, nSize);
            m_cacheStart = nIndex;
            m_cacheSize = nSize;
            return true;
        }
    }

    nEnd = wxMin(RoundUp(nEnd, m_pDS->BlockAlign), this->size); // Adjust nEnd upward to 4k boundary.
    nSize = nEnd - nIndex;

    // Allocate a buffer and read segment(s) into it
    if (m_cacheBufferSize < nSize)
    {
        m_cacheBufferSize = nSize;
        m_pCacheBuffer = (uint8*)realloc(m_pCacheBuffer, m_cacheBufferSize);
    }

    THSIZE readStart = nIndex;
    THSIZE readSize = nSize;
    if (m_pCacheData == m_pCacheBuffer)
    {
        if (nIndex > m_cacheStart &&
            nIndex < m_cacheStart + m_cacheSize &&
            nEnd   > m_cacheStart + m_cacheSize)
        {  // reading forward; copy overlapping buffer from end to beginning
            memcpy(m_pCacheBuffer, m_pCacheBuffer + (nIndex - m_cacheStart), m_cacheStart + m_cacheSize - nIndex);
            readStart = m_cacheStart + m_cacheSize;
            readSize = nEnd - readStart;
        }
        else if (nIndex < m_cacheStart &&
                 nEnd > m_cacheStart &&
                 nEnd < m_cacheStart + m_cacheSize)
        {  // reading backward; copy overlapping buffer from beginning to end
            readSize = m_cacheStart - nIndex;
            memcpy(m_pCacheBuffer + readSize, m_pCacheBuffer, nEnd - m_cacheStart);
        }
        else if (nIndex == m_cacheStart)
        {  // same starting point; we already know nEnd is past the cache
            readStart = m_cacheStart + m_cacheSize;
            readSize = nEnd - readStart;
        }
    }

    m_cacheStart = nIndex;
    m_cacheSize = nSize;
    if (!DoRead(readStart, readSize, m_pCacheBuffer + (readStart - m_cacheStart)))
    {
        PRINTF(_T("Couldn't fill cache at %I64X, size %I64X\n"), nIndex, nSize);
        m_cacheSize = 0;
        return false;
    }
    PRINTF(_T("Cache filled at %I64X, size %I64X\n"), nIndex, nSize);
    m_pCacheData = m_pCacheBuffer;
    return true;
}

void HexDoc::InvalidateCache()
{
    m_cacheStart = 0;
    m_cacheSize = 0;
}

void HexDoc::InvalidateCache(THSIZE nAddress, THSIZE nOldSize, THSIZE nNewSize)
{
    //! This could be smarter about which half to throw away, or re-read if oldSize==newSize.
    //! But maybe we don't need it at all.
    THSIZE maxSize = wxMax(nOldSize, nNewSize);
    if (IsByteCached(nAddress))
        m_cacheSize = nAddress - m_cacheStart;
    else if (IsByteCached(nAddress + maxSize))
    {
        m_cacheSize -= (nAddress + maxSize) - m_cacheStart;
        m_cacheStart = nAddress + maxSize;
    }
    else if (RangeOverlapSize(m_cacheStart, m_cacheSize, nAddress, maxSize))
    {
        m_cacheStart = 0;
        m_cacheSize = 0;
    }
}

uint8* HexDoc::LoadWriteable(uint64 nIndex, uint32 nSize /*= 0*/)
{
    return NULL; //! need implementation
}

bool HexDoc::Read(uint64 nOffset, uint32 nSize, uint8 *target, uint8 *pModified /*= NULL*/)
{
    if (nOffset + nSize > this->size)
        return false;

    if (Cache(nOffset, nSize))
        memcpy(target, m_pCacheData + nOffset - m_cacheStart, nSize);
    else if (!DoRead(nOffset, nSize, target))
    {
        memset(target, 0xCD, nSize);
        if (pModified)
            memset(target, 0, nSize);
        return false;
    }

    if (pModified)
    {
        //for (uint32 n = 0; n < nSize; n++)
        //{
        //    pModified[n] = IsModified(nOffset + n); //! much room for improvement here.
        //}

        MoveToSegment(nOffset);
        THSIZE segOffset = nOffset - m_curBase;
        uint8 flag = (m_current->pDS != this->m_pDS);
        for (uint32 n = 0; n < nSize; )
        {
            uint32 blockSize = wxMin(m_current->size - segOffset, nSize - n);
            memset(pModified + n, flag, blockSize);
            m_curBase += m_current->size;
            m_current = m_current->next;
            flag = (m_current->pDS != this->m_pDS);
            n += blockSize;
            segOffset = 0;
        }
    }

    return true;
}

bool HexDoc::DoRead(THSIZE nOffset, size_t nSize, uint8* target)
{
    //if (!Load(nOffset))
    if (!MoveToSegment(nOffset))
        return false;
    Segment *ts = m_current;

    uint32 copySize, dstOffset = 0;
    uint64 srcOffset = nOffset - m_curBase;
    uint32 remaining = nSize;
    while (remaining > 0)
    {
        if (ts == NULL)
            return false; //! Help!
        copySize = wxMin(remaining, ts->size - srcOffset);
        if (!ts->Read(srcOffset, copySize, target + dstOffset))
            return false;
        dstOffset += copySize;
        srcOffset = 0;
        remaining -= copySize;
        ts = ts->next;
    }
    return true;
}

bool HexDoc::ReadNoUpdateCache(uint64 nOffset, size_t nSize, uint8 *target)
{
    if (nOffset + nSize > this->size)
        return false;

    if (IsRangeCached(nOffset, nSize))
    {
        memcpy(target, m_pCacheData + nOffset - m_cacheStart, nSize);
        return true;
    }
    return DoRead(nOffset, nSize, target);
}

bool HexDoc::ReadNumber(uint64 nIndex, int nBytes, void *target)
{
    //! I don't think this will work on big-endian machines.
    if (!Read(nIndex, nBytes, (uint8*)target)) return false;
    if (pSettings->iEndianMode != NATIVE_ENDIAN_MODE)
        reverse((uint8*)target, nBytes);
    return true;
}

bool HexDoc::ReadInt(uint64 nIndex, int nBytes, UINT64 *target, int mode /*= -1*/)
{
    if (nBytes < 1 || nBytes > 8) return false;
    if (mode == -1)
        mode = pSettings->iEndianMode;
    *target = 0;
    uint8 *p8 = (uint8*)target;
    if (NATIVE_ENDIAN_MODE == BIGENDIAN_MODE)
        p8 += 8 - nBytes;
    if (!Read(nIndex, nBytes, p8)) return false;
    if (mode != NATIVE_ENDIAN_MODE)
        reverse(p8, nBytes);
    return true;
}

wxString HexDoc::ReadString(THSIZE nIndex, size_t nSize, bool replaceNull /*= true*/)
{
    wxString str;
    if (!nSize)
        return str;
#ifdef _UNICODE
    char *data = new char[nSize + 1];
#else
    char *data = str.GetWriteBuf(nSize + 1);
#endif
    if (!Read(nIndex, nSize, (uint8*)data))
        nSize = 0;
    if (replaceNull)  // Replace embedded nulls with spaces.
        for (size_t i = 0; i < nSize; i++) {
            if (data[i] == 0)
                data[i] = ' ';
        }
    data[nSize] = 0;
#ifdef _UNICODE
    str = wxString(data, wxConvLibc, nSize + 1);
    delete [] data;
#else
    str.UngetWriteBuf(nSize);
#endif
    return str;
}

wxString HexDoc::ReadStringW(THSIZE nIndex, size_t nSize, bool replaceNull /*= true*/)
{
    wxString str;
    if (!nSize)
        return str;
#ifdef _UNICODE
    //wchar_t *data = (wchar_t*)str.GetWriteBuf(nSize+1);
    wxStringBuffer strbuf(str, nSize + 1);
    wchar_t *data = strbuf;
#else
    wchar_t *data = new wchar_t[nSize+1];
#endif
    if (!Read(nIndex, nSize * sizeof(wchar_t), (uint8*)data))
        nSize = 0;
    if (replaceNull)  // Replace embedded nulls with spaces.
        for (size_t i = 0; i < nSize; i++) {
            if (data[i] == 0)
                data[i] = ' ';
        }
    data[nSize] = 0;
#ifdef _UNICODE
    //str.UngetWriteBuf(nSize);
#else
    str = wxString(data, wxConvLibc, nSize+1);
    delete [] data;
#endif
    return str;
}

//*****************************************************************************
//*****************************************************************************
// Save
//*****************************************************************************
//*****************************************************************************

bool HexDoc::SaveRange(wxString filename, THSIZE begin, THSIZE length)
{
    wxFile f(filename, wxFile::write);
    if (!f.IsOpened())
    {
        wxMessageBox(_T("Couldn't open file for output."));
        return false;
    }

    wxString msg = _T("Saving ") + FormatDec(length) + _T(" bytes to \n") + filename;

    bool rc = WriteRange(f, begin, length, msg);
    f.Close();
    return rc;
}

bool HexDoc::WriteRange(wxFile &f, THSIZE begin, THSIZE length, wxString msg)
{
    // bad name.  Maybe SaveRange is wrong too?  (okay with Segments and Regions)
    //! should be in new thread.
    //! write in 1MB chunks and report to GUI thread.
    //uint8 buffer[4096];
    //DWORD cbWrite;
    //for (DWORD offset = 0; offset < length; offset += 4096)
    //{
    //    DWORD tmpSize = min(length - offset, 4096);
    //    if (!Read(begin + offset, tmpSize, buffer))
    //        return false;
    //    if (!WriteFile(hOutput, buffer, tmpSize, &cbWrite, NULL))
    //        return false;
    //}
    //return true;

    thProgressDialog progress(length, hw, msg);

    THSIZE blockSize = 0x10000;
    uint8 *buf = new uint8[blockSize];
    DWORD cBytes;

    for (THSIZE offset = 0; offset < length; offset += blockSize)
    {
        if (length - offset < blockSize)
            blockSize = length - offset;
        Read(begin + offset, blockSize, buf);
        cBytes = f.Write(buf, blockSize);
        if (!progress.Update(offset) && Confirm(_T("Are you sure you want to abort?")))
            break;
    }
    delete [] buf;
    return true;
}

bool HexDoc::Save()
{
    InvalidateCache();
    bool rc = true;
    //uint8 *buf = new uint8[MEGA]; //! To do: use cache buffer?
    //uint8 *buf = m_pCacheBuffer;  //! testing
    Segment *ts = 0;

    //! need a temp file here.
    //! also should be in separate thread.
    //! write in 1MB chunks and report to GUI thread.
    if (CanWriteInPlace())
    {
        //! no size changes, no other data sources
        //! call for each region
        //m_pDS->Flush();

        THSIZE base = 0;//display_address;  // fixed 2008-05-06.  Only need this for writing
        for (Segment *s = m_head->next; s != m_tail; )
        {
            if (s->pDS == m_pDS) //! do we need to check s->type?
            {
                base += s->size;
                s = s->next;
                continue;
            }

            // Collect all the segments that need to be written,
            // until the next segment bigger than MinTransfer that we can skip.
            THSIZE blocksize = MEGA, firstSegSize, writeSize = firstSegSize = s->size;
            s = s->next;
            while (s != m_tail)
            {
                if (s->pDS == m_pDS && s->size > m_pDS->MinTransfer)
                    break;
                writeSize += s->size;
                s = s->next;
            }
            
            for (THSIZE offset = 0; offset < writeSize; offset += blocksize)
            {
                blocksize = wxMin(blocksize, writeSize - offset);
                const uint8 *buf = Load(base + offset, blocksize);
                if (!buf) {
                    PRINTF(_T("Save(): Load(0x%I64X, 0x%X) failed.\n"), base + offset, blocksize);
                    rc = false;
                    goto done;
                }
                if (!m_pDS->Write(base + offset + display_address, blocksize, buf))
                {
                    PRINTF(_T("Save() Error code %d from Write()\n"), errno);
                    rc = false;
                    goto done;
                }
            }

            base += writeSize; // size of all segments we've written in this pass.
            // s already points to the next unwritten segment, or m_tail.
        }
        m_pDS->SetEOF(GetSize());
    
        m_pDS->Flush();
    }
    else if (GetSize() < MEGA && m_pDS->CanWriteInPlace() && m_pDS->CanChangeSize())
    {  // We'll just read everything into a buffer and overwrite the file.
        //uint8 *b = Load(0, size);  // I think this is OK.  If there's more than one DS it will use the buffer.
        //! Maybe not.  What if the only edit was to delete 100KB at the beginning?

        if (m_cacheBufferSize < GetSize())
        {
            m_cacheBufferSize = GetSize();
            m_pCacheBuffer = (uint8*)realloc(m_pCacheBuffer, m_cacheBufferSize);
        }
        m_cacheStart = 0;
        m_cacheSize = GetSize();
        if (!DoRead(m_cacheStart, m_cacheSize, m_pCacheBuffer))
        {
            PRINTF(_T("Couldn't fill cache at %I64X, size %I64X\n"), m_cacheStart, m_cacheSize);
            m_cacheSize = 0;
            return false;
        }

        m_pDS->Write(0, size, m_pCacheBuffer);  // That was easy.
        m_pDS->SetEOF(GetSize());  
        m_pDS->Flush();
    }
    else
    {
        //! save-as temp file, delete and rename, etc.
        //! this could be optimized if the first half of the file hasn't been changed,
        //!  or we could find a way where nothing overlaps.

        wxFileDialog saveDlg(hw, _T("Save File"), ZSTR, ZSTR, _T("*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (saveDlg.ShowModal() == wxID_OK)
            rc = SaveRange(saveDlg.GetPath(), 0, GetSize());
        else
            rc = true; // Changing your mind is not an error!

        goto done;  // skip rebuilding the buffer
    }

    if (size)
        ts = new Segment(size, display_address, m_pDS); // calls m_pDS->AddRef
    DeleteSegments();
    m_head = new Segment(0, 0, NULL);
    m_tail = new Segment(0, 0, NULL);
    m_curBase = 0;
    if (size)
    {
        m_current = ts;
        m_head->next = m_current;
        m_current->prev = m_head;
        m_current->next = m_tail;
        m_tail->prev = m_current;
    }
    else
    {
        m_head->next = m_current = m_tail;
        m_tail->prev = m_head;
    }

    //! To do: clear undo buffer

    InvalidateCache();
    if (hw)
        hw->OnDataChange(0, size, size);

done:
    //delete [] buf;
    return rc;
}

bool HexDoc::CanWriteInPlace()
{
    if (!m_pDS->CanWriteInPlace())
        return false;
    //! How do we handle file size changes?  (adding or removing bytes at EOF)
    THSIZE base = display_address;
    for (Segment *s = m_head->next; s != m_tail; base += s->size, s = s->next)
    {
        if (s->pDS == m_pDS && s->stored_offset != base)
            return false;
    }
    return true;
}

//*****************************************************************************
//*****************************************************************************
// Modified data
//*****************************************************************************
//*****************************************************************************

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

//*****************************************************************************
//*****************************************************************************
// Serialization
//*****************************************************************************
//*****************************************************************************

int HexDoc::GetSerializedLength(THSIZE nOffset, THSIZE nSize)
{
    if (nSize == 0)
        return 0;
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
        THSIZE copySize = wxMin(nSize, ts->size - segOffset);
        sSize += ts->GetSerializedLength(segOffset, copySize);
        segStart += ts->size;
        segOffset = 0;
        ts = ts->next;
    }
    return sSize;
}

void HexDoc::Serialize(THSIZE nOffset, THSIZE nSize, uint8 *target)
{
    if (nSize == 0)
        return;
    SerialDataHeader &hdr = *(SerialDataHeader*)target;
    int sOffset = sizeof(hdr);
    //hdr.endianMode = NATIVE_ENDIAN_MODE;
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
        THSIZE copySize = wxMin(nOffset + nSize, segStart + ts->size) - (segStart + segOffset);
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

//wxString HexDoc::SerializeData(const uint8* pData, size_t nSize)
//{
//    wxString str;
//    size_t nDataSize = nSize + sizeof(SerialDataHeader) + sizeof(SerialDataSegment);
//    char *target = str.GetWriteBuf(nDataSize);
//
//    // Fill in serial data header.  One MEM-type segment, no sources.
//    SerialDataHeader &hdr = *(SerialDataHeader*)target;
//    int sOffset = sizeof(hdr);
//    hdr.endianMode = NATIVE_ENDIAN_MODE;
//    hdr.nSegments = 1;
//    hdr.nSources = 0;
//
//    // Fill in the serial data segment.
//    SerialDataSegment &seg = *(SerialDataSegment*)(target + sOffset);
//    seg.offset = 0;
//    seg.size = nSize;
//    seg.src = -1; // Immediate data follows.
//    memcpy(target + sOffset, pData, nSize);
//
//    str.UngetWriteBuf(nDataSize);
//    return str;
//}


//*****************************************************************************
//*****************************************************************************
// Other stuff: undo, find, etc.
//*****************************************************************************
//*****************************************************************************

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

int HexDoc::Find(const uint8 *data, size_t count, int type, bool caseSensitive, THSIZE &start, THSIZE &end)
{
    if (type == 0)
        return FindHex(data, count, start, end, caseSensitive);
    return false;
}

void DumpLine(THSIZE address, const uint8* b)
{
    TCHAR line[81];
    //memset(line, ' ', 80 * sizeof(TCHAR));
    for (int i = 0; i < 80; i++)
        line[i] = ' ';
    line[80] = 0;
    int extra = 0;

    for (int i = 0; i < 16; i++)
    {
        extra = i / 4;
        my_itoa((uint32)b[i], line + i * 3 + extra, 16, 2);
        line[i * 3 + extra + 2] = ' ';
        if (isprint(b[i]))
            line[58 + i] = b[i];
    }
    PRINTF(_T("%04I64X %s\n"), address, line);
}

void HexDoc::DumpCache()
{
    PRINTF(_T("*** Cache info ***\n"));
    PRINTF(_T(" start = %I64X\n"), m_cacheStart);
    PRINTF(_T(" size  = %I64X\n"), m_cacheSize);
    PRINTF(_T(" buffer size = %I64X\n"), m_cacheBufferSize);
    if (m_pCacheData == m_pCacheBuffer)
        PRINTF(_T(" data points to m_pCacheBuffer\n"));
    else
        PRINTF(_T(" data does not point to m_pCacheBuffer\n"));
    for (THSIZE line = 0; line < 5; line++)
    {
        DumpLine(line * 16, m_pCacheData + line * 16);
    }
}





//*****************************
//uint8 HexDoc::Next8()
//{
//    if (m_nFilePointer < m_cacheStart || m_nFilePointer >= m_cacheStart + m_cacheSize)
//        Cache(m_nFilePointer, 0x4000);
//    return m_pCacheData[m_nFilePointer++ - m_cacheStart];
//}

// Use adler32 from zlib, included in wxWidgets.
extern "C" ULONG adler32(ULONG adler, const char *buf, int size);
bool HexDoc::ComputeAdler32(THSIZE offset, THSIZE size, ULONG &adler)
{
    THSIZE runner, blockSize = MEGA;
    thProgressDialog progress(size, NULL, _T("Computing Adler32 for\n") + info);
    adler = 0;
    const uint8* buf;
    for (runner = 0; runner < size; runner += blockSize)
    {
        if (!progress.Update(runner))
            return false;

        if (size - runner < blockSize)
            blockSize = size - runner;
        if (0 == (buf = Load(offset + runner, blockSize)))
        {
            PRINTF(_T("Couldn't read from doc at 0x%I64X\n"), offset + runner);
            return false;
        }

        adler = adler32(adler, (const char*)buf, wxMin(blockSize, size - runner));
    }
    return true;
}
