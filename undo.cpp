#include "precomp.h"
#include "hexdoc.h"
#include "undo.h"
#include "hexwnd.h"

#define new New

UndoManager::UndoManager(UINT_PTR nMaxSize, bool bIncludeCursor)
{
    m_nMaxSize = nMaxSize;
    m_pFirst = m_pLast = m_pCurrent = NULL;
    m_bIncludeCursor = bIncludeCursor;
    m_nCurSize = 0;
}

UndoManager::~UndoManager()
{
    Clear();
}

#if 0
UndoAction* UndoManager::Alloc(int type, THSIZE nAddress, UINT_PTR nSize)
{
    if (m_nCurSize + nSize >= m_nMaxSize)
        return NULL;
    UndoAction *a = (UndoAction*)new char[sizeof(UndoAction) + nSize];

    while (m_pLast != m_pCurrent)
    {
        UndoAction *a = m_pLast->prev;
        delete m_pLast;
        m_pLast = a;
    }
    if (m_pCurrent == NULL)
        m_pFirst = m_pLast = NULL;

    if (m_pLast)
        m_pLast->next = a;
    if (a)
    {
        a->type = type;
        a->nAddress = nAddress;
        a->nAdr2OrSize = 0;
        a->next = NULL;
        a->prev = m_pLast;
        m_pLast = a;
        if (!m_pFirst)
            m_pFirst = a;
        m_pCurrent = a;
    }

    m_nLastTime = GetTickCount();
    return a;
}
#endif // 0

bool UndoManager::CanUndo()
{
    return m_pCurrent != NULL;
}

bool UndoManager::CanRedo()
{
    return (m_pCurrent != m_pLast);
}

UndoAction* UndoManager::GetUndo()
{
    UndoAction *p = m_pCurrent;
    if (m_pCurrent)
        m_pCurrent = m_pCurrent->prev;
    return p;
}

UndoAction* UndoManager::GetRedo()
{
    if (m_pCurrent && m_pCurrent->next)
        m_pCurrent = m_pCurrent->next;
    else if (m_pFirst)
        m_pCurrent = m_pFirst;
    return m_pCurrent;
}

//UndoAction* UndoManager::GetCurrent(int type)
//{
//    if (m_pCurrent &&
//        m_pCurrent->type == type)
//        return m_pCurrent;
//    return NULL;
//}

UndoData* UndoManager::AllocDataChange(HexDoc *doc, THSIZE nAddress)
{
    Truncate();
    UndoData *u = new UndoData();

    if (m_pLast)
        m_pLast->next = u;
    if (u)
    {
        u->next = NULL;
        u->prev = m_pLast;
        m_pLast = u;
        if (!m_pFirst)
            m_pFirst = u;
        m_pCurrent = u;
    }

    doc->hw->GetSelection(u->oldSel);
    u->m_nAddress = nAddress;

    m_nLastTime = GetTickCount();
    return u;
}

void UndoManager::Truncate()
{
    while (m_pLast != m_pCurrent)
    {
        UndoAction *u = m_pLast->prev;
        delete m_pLast;
        m_pLast = u;
    }
    if (m_pCurrent == NULL)
        m_pFirst = m_pLast = NULL;
}

void UndoManager::Clear()
{
    m_pCurrent = NULL;
    Truncate();
}

bool UndoAction::Undo(HexDoc *doc)
{
    SerialData sInsert(insertedData);
    SerialData sRemove(removedData);

    if (!sInsert.Ok() || !sRemove.Ok())
        return false;

    doc->ReplaceSerialized(m_nAddress, sInsert.m_nTotalSize, sRemove, true);

    doc->hw->SetSelection(oldSel);
    doc->hw->ScrollToCursor();
    if (!doc->undo->CanUndo()) //! no more actions -- for now this means last save point
        doc->hw->SetModified(false);
    //! how do we tell if only cursor movement actions are left on the undo list?
    return true;
}

bool UndoAction::Redo(HexDoc *doc)
{
    SerialData sInsert(insertedData);
    SerialData sRemove(removedData);

    if (!sInsert.Ok() || !sRemove.Ok())
        return false;

    doc->ReplaceSerialized(m_nAddress, sRemove.m_nTotalSize, sInsert, true);

    Selection newSel = oldSel;
    newSel.Set(m_nAddress + sInsert.m_nTotalSize);
    doc->hw->SetSelection(newSel);
    doc->hw->ScrollToCursor();
    return true;
}

UndoAction* UndoManager::GetCurrent()
{
    return m_pCurrent;
}

void UndoAction::Print()
{
    PRINTF(_T("UA desc %s\n"), description.c_str());
    PRINTF(_T("UA removed=%d inserted=%d\n"), removedData.Len(), insertedData.Len());
    PRINTF(_T("UA oldSel %d-%d.%d,%d\n"), oldSel.nStart, oldSel.nEnd, oldSel.iDigit, oldSel.iRegion);
    //PRINTF("UA newSel %d-%d.%d,%d\n", newSel.nStart, newSel.nEnd, newSel.iDigit, newSel.iRegion);
}

//void UndoAction::DefaultNewSelection()
//{
//    SerialData sInsert(insertedData);
//    THSIZE first = oldSel.GetFirst();
//    if (sInsert.len && sInsert.loaded)
//        first += sInsert.m_nTotalSize;
//    newSel.Set(first, first, 0, 0);
//}

wxString GetUndoData(SerialData &sdata)
{
    if (sdata.m_nTotalSize > 4)
        return wxString::Format(_T("%I64d bytes"), sdata.m_nTotalSize);

    size_t size = sdata.m_nTotalSize;
    uint8 data[4];
    sdata.Extract(size, data);
    wxString ret;
    for (size_t i = 0; i < size; i++)
    {
        if (i > 0)  ret += ' ';
        ret += wxString::Format(_T("%02X"), data[i]);
    }
    //return '(' + ret + ')';
    return ret;
}

wxString UndoAction::GetDescription()
{
    if (description.Length())
        return description;

    SerialData inserted(insertedData);
    SerialData removed(removedData);
    description.Printf(_T("At %02I64X: "), oldSel.GetFirst());
    if (removed.m_nTotalSize)
    {
        if (inserted.m_nTotalSize == removed.m_nTotalSize &&
            inserted.m_nTotalSize > 4)
            description += wxString::Format(_T("* %I64d bytes"), removed.m_nTotalSize);
        else if (inserted.m_nTotalSize)
            description += GetUndoData(removed) + _T(" => ") + GetUndoData(inserted);
        else
            description += _T("- ") + GetUndoData(removed);
    }
    else if (inserted.m_nTotalSize)
        description += _T("+ ")+ GetUndoData(inserted);
    else
        description = _T("Selection change");

    return description;
}
