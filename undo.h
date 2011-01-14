#ifndef _UNDO_H_
#define _UNDO_H_

#pragma warning(disable: 4200) // nonstandard extension used : zero-sized array in struct/union

/*class UndoAction {
    friend class UndoManager;
public:
    int type;
    UndoAction *next, *prev;
    THSIZE nAddress, nAdr2OrSize;
    uint8 data[0];
};

enum {
    UA_SETSELECTION,
    UA_PASTE,
    UA_DELETE,
    UA_INSERT,
    UA_OVERWRITE
};*/


class UndoAction {
public:
    UndoAction()
    {
        next = prev = NULL;
    }

    UndoAction *next, *prev;
    virtual bool Redo(HexDoc *doc);
    virtual bool Undo(HexDoc *doc);
    virtual void Print();
    //virtual void DefaultNewSelection();

    THSIZE m_nAddress;
    //Selection newSel;
    Selection oldSel;

    wxString description;
    wxString removedData, insertedData;

    wxString GetDescription();
};

class UndoData : public UndoAction {
public:
    UndoData() { }
};

class UndoSelection : public UndoAction {
public:
    UndoSelection(HexDoc *doc, THSIZE iStart, THSIZE iEnd, uint32 iDigit);
    THSIZE xorStart, xorEnd;
    uint32 xorDigit;
};

class UndoManager
{
public:
    UndoManager(UINT_PTR nMaxSize, bool bIncludeCursor);
    ~UndoManager();

    bool CanUndo();
    bool CanRedo();
    UndoAction* Alloc(int type, THSIZE nAddress, UINT_PTR nSize);
    UndoAction* GetCurrent(int type);
    UndoAction* GetCurrent();
    void Truncate();
    bool SetMaxSize(UINT_PTR nMaxSize);
    void SetIncludeCursor(bool bIncludeCursor);
    void Clear();

    UndoData* AllocDataChange(HexDoc *doc, THSIZE nAddress);

    //CutAction* Cut(uint64 nIndex, uint64 nSize);
    //PasteAction* PasteInsert(uint64 nIndex);
    //bool PasteOverwrite(uint64 nIndex);
    //DeleteAction* Delete(uint64 nIndex, uint64 nSize);
    //bool InsertByte(uint64 nIndex, uint8 data);
    //bool InsertSecondNibble(uint8 data);
    //bool OverwriteByte(uint64 nIndex, uint8 data);
    //bool OverwriteSecondNibble(uint8 data);
    //bool InsertData(THSIZE nAddress, THSIZE nSize, const uint8 *pData);
    //bool SetSelection(THSIZE iSelStart, THSIZE iSelEnd, uint32 iDigit);

    //bool InsertAt(THSIZE nIndex, const uint8 *psrc, int nSize, int nCount = 1);
    //bool RemoveAt(THSIZE nIndex, THSIZE nSize);
    //bool ReplaceAt(THSIZE ToReplaceIndex, THSIZE ToReplaceLength, const uint8* pReplaceWith, uint32 ReplaceWithLength);

    UndoAction *GetUndo();
    UndoAction *GetRedo();
    DWORD GetLastTime() { return m_nLastTime; }

    // for debugging and DocHistoryView:
    UndoAction *GetFirst() const { return m_pFirst; }
    //UndoAction *GetCurrent();

protected:
    UINT_PTR m_nMaxSize, m_nCurSize;
    UndoAction *m_pFirst, *m_pLast, *m_pCurrent;
    bool m_bIncludeCursor;
    DWORD m_nLastTime;
};

#endif // _UNDO_H_
