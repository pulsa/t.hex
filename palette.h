#pragma once

#ifndef _PALETTE_H_
#define _PALETTE_H_

#include "defs.h"

// Syntax of palette, in Yacc grammar:

// Palette         : ColorPointRange
//                 | Palette ';' ColorPointRange
//
// ColorPointRange : ColorPoint
//                 | ColorPointRange '-' ColorPoint
//
// ColorPoint      : ColorIndexList '(' Color ')'
//
// ColorIndexList  : ColorIndex
//                 | ColorIndexList ',' ColorIndex
//
// ColorIndex      : tNumber { /* 0 - 255 */ }
//
// Color           : tNumber { /* RGB */ }
//                 | tNumber tNumber tNumber { /* R, G, B */ }

typedef std::vector<uint8> ByteArray;

// I needed a string tokenizer that would treat symbols "()-,;" as delimiters,
// but also return them as strings.
// wxStringTokenizer didn't do that, so I wrote MyTokenizer.

class MyTokenizer
{
public:
    MyTokenizer(wxString str, wxString extraDelims = wxEmptyString) // spaces are automatic
    {
        m_string = str;
        m_delims = extraDelims;
        m_spaces = _T(" \t\r\n");
        m_all = extraDelims + m_spaces;
    }

    bool HasMoreTokens()
    {
        return m_string.find_first_not_of(m_all) != wxString::npos;
    }

    wxString PeekNextToken()
    {
        if (!m_string) return wxEmptyString;
        wxString token;

        size_t pos = m_string.find_first_not_of(m_spaces); // skip spaces
        if (pos == wxString::npos) // at end of string
            return m_string = wxEmptyString;
        if (pos) m_string = m_string.Mid(pos);
        pos = m_string.find_first_of(m_all);
        if (pos == wxString::npos)
            pos = m_string.Len();
        else if (pos == 0 && m_delims.Contains(m_string[0]))
            pos = 1;
        token = m_string.Left(pos);
        return token;
    }

    wxString GetNextToken()
    {
        if (!m_string) return wxEmptyString;
        wxString token = PeekNextToken();
        m_string = m_string.Mid(token.Len());
        return token;
    }

protected:
    wxString m_string, m_delims, m_spaces, m_all;
};

bool ReadList(MyTokenizer &st, ByteArray &a, wxString &nextToken);

bool ReadColor(MyTokenizer &st, COLORREF &clr);

void SetColorRange(int begin, int end,
                   COLORREF beginColor,
                   COLORREF endColor,
                   COLORREF clr[256]);

//*****************************************************************************
//*****************************************************************************

class thPaletteData : public wxObjectRefData
{
public:
    thPaletteData(size_t numColors = 0)
    {
        this->numColors = numColors;
        if (numColors)
            clr = new COLORREF[numColors];
        else
            clr = NULL; //! this causes bad problems
    }

    thPaletteData(const thPaletteData &src)
    {
        //*this = src; // this is bad.  It copies ref count.
        begin = src.begin;
        end = src.end;
        beginColor = src.beginColor;
        endColor = src.endColor;
        numColors = src.numColors;

        if (numColors)
        {
            clr = new COLORREF[numColors];
            memcpy(clr, src.clr, numColors * sizeof(COLORREF));
        }
    }

    virtual ~thPaletteData()
    {
        delete [] clr;
    }

    wxArrayInt begin, end, beginColor, endColor; //! must ensure sizeof(int) >= sizeof(COLORREF)
    wxString msg;
    COLORREF *clr;
    size_t numColors;

    size_t Ranges() const { return begin.GetCount(); }
    size_t Colors() const { return numColors; }
    inline COLORREF GetColor(BYTE n) const { return clr[n]; }
    wxString SetPalette(wxString palette);
    wxString GetPalette() const; // return the stored ranges as a string
    void Clear();
    void Add(int begin, COLORREF beginColor, int end = -1, COLORREF endColor = 0);

    bool Get(size_t r, int *begin, COLORREF *beginColor, int *end, COLORREF *endColor) const;
    void Insert(int range, int begin, COLORREF beginColor, int end, COLORREF endColor);
    void Remove(int range);
    void Nudge(COLORREF dst, float amount);
    void Realize();
    wxString Import(wxString filename);

private:
    thPaletteData& operator=(const thPaletteData& src);  // Don't call this function.
};

//extern thPaletteData thEmptyPalette;

//*****************************************************************************

class thPalette : public wxObject
{
public:
    thPalette(wxString palette = wxEmptyString)
    {
        //! refdata = new thPaletteData(), or default object
        if (palette.Len())
            msg = SetPalette(palette);
    }

    virtual ~thPalette()
    {
        //! GetData()->Release();
    }

    // create a new m_refData
    virtual wxObjectRefData *CreateRefData() const;

    // create a new m_refData initialized with the given one
    virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const;

    inline size_t Colors() const
    {
        if (!Ok()) return false;
        return GetData()->Colors();
    }
    inline COLORREF GetColor(size_t n) const
    {
        if (!Ok()) return false;
        return GetData()->GetColor((BYTE)n);
    }

    int FindRange(int index) const; // return the range that set the color at index, or -1
    inline size_t Ranges() const
    {
        if (!Ok()) return false;
        return GetData()->Ranges();
    }

    //bool Ok() const { return GetData() != 0; }
    bool Ok() const { return m_refData != 0; }

    wxString SetPalette(wxString palette)
    {
        UnShare();
        return GetData()->SetPalette(palette);
    }

    inline wxString GetPalette() const
    {
        if (!Ok())
            return wxEmptyString;
        return GetData()->GetPalette();
    }
    //wxString GetPalette(); // return the stored ranges as a string

    bool Get(size_t r, int *begin, COLORREF *beginColor, int *end, COLORREF *endColor) const
    {
        if (!Ok()) return false;
        return GetData()->Get(r, begin, beginColor, end, endColor);
    }

    void Add(int begin, COLORREF beginColor, int end = -1, COLORREF endColor = 0)
    {
        UnShare();
        GetData()->Add(begin, beginColor, end, endColor);
    }

    void Insert(int range, int begin, COLORREF beginColor, int end = -1, COLORREF endColor = 0)
    {
        UnShare();
        GetData()->Insert(range, begin, beginColor, end, endColor);
    }

    void Remove(int range)
    {
        UnShare();
        GetData()->Remove(range);
    }

    void Nudge(COLORREF dst, float amount)
    {
        UnShare();
        GetData()->Nudge(dst, amount);
    }

    void Clear()
    {
        UnShare();
        GetData()->Clear();
    }

    void Realize()
    {
        UnShare();
        GetData()->Realize();
    }
    
    wxString Import(wxString filename)
    {
        UnShare();
        return GetData()->Import(filename);
    }

    thPaletteData *GetRealPaletteData() { return GetData(); }

protected:
    wxString msg;
    thPaletteData *GetData() const { return wx_static_cast(thPaletteData*, m_refData); }
};


#endif // _PALETTE_H_
