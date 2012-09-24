#pragma once

// This class isn't used... yet.
// But I think it might be an improvement.
// Maybe someday I'll get around to it.

class HexWnd;
class HexDoc;

class HexView
{
public:
    HexWnd *m_hw;
    HexDoc *m_doc;

    Selection m_sel;
    bool m_bWriteable;  // may have read-only view of writeable data source

    THSIZE 

    Selection m_highlight;
    DataView *m_dvHighlight; // which view window controls the highlighting?
    void Highlight(THSIZE start, THSIZE size, DataView *dv = NULL);
    void SetCurrentView(DataView *dv)
    {
        Highlight(0, 0, NULL);
        m_dvHighlight = dv;
    }

    void GetSelection(uint64 &iSelStart, uint64 &iSelEnd)
    {
        if (m_iSelStart >= m_iCurByte)
            iSelStart = m_iCurByte, iSelEnd = m_iSelStart;
        else
            iSelStart = m_iSelStart, iSelEnd = m_iCurByte;
    }

    void GetSelection(Selection &sel)
    {
        sel.Set(m_iSelStart, m_iCurByte, m_iCurPane, m_iCurDigit);
    }
    Selection GetSelection()
    {
        Selection sel;
        GetSelection(sel);
        return sel;
    }
    Selection GetSelectionOrAll()
    {
        if (m_iCurByte == m_iSelStart)
            return Selection(0, DocSize(), m_iCurPane, m_iCurDigit);
        else
            return GetSelection();
    }
    void SetSelection(Selection &sel)
    {
        SetSelection(sel.nStart, sel.nEnd, sel.iRegion, sel.iDigit);
        Update();
    }

    uint64 GetCursor() { return m_iCurByte; }
    uint64 GetCursorVirtual();
};
