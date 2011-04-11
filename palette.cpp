#include "precomp.h"
#include "palette.h"
#include "utils.h"

//thPaletteData thEmptyPalette;

bool ReadList(MyTokenizer &st, ByteArray &a, wxString &nextToken)
{
    uint8 val;
    a.clear();

    nextToken = st.GetNextToken();
    while (ReadUserNumber(nextToken, val))
    {
        a.push_back(val);
        nextToken = st.GetNextToken();
    }
    return a.size() > 0;
}

bool ReadColor(MyTokenizer &st, COLORREF &clr)
{
    uint32 rgb;
    uint8 r, g, b;
    wxString first = st.GetNextToken(), second, rest;

    // There may be only one number, like 0xFF0088.
    if (!ReadUserNumber(first, rgb))
        return false;

    if (!st.HasMoreTokens()) // added for single-color strings that don't have parentheses
    {
        clr = rgb;
        return true;
    }

    second = st.GetNextToken();
    if (second.IsSameAs(')'))
    {
        //if (appSettings.bReverseRGB)
        //    clr = RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
        //else
            clr = rgb;
        return true;
    }

    if (!ReadUserNumber(first, r)) // re-read first number with 8-bit size limit
        return false;

    if (!ReadUserNumber(second, g))
        return false;
    if (!ReadUserNumber(st.GetNextToken(), b))
        return false;
    clr = RGB(r, g, b);
    if (st.HasMoreTokens())
    {
        first = st.GetNextToken();
        if (!first.IsSameAs(')'))
            return false;
    }
    return true;
}

void SetColorRange(int begin, int end, COLORREF beginColor, COLORREF endColor, COLORREF clr[256])
{
    if (begin == end || end == -1) // prevent divide-by-zero
    {
        clr[begin] = beginColor;
        return;
    }

    if (begin > end)
    {
        th_swap(begin, end);
        th_swap(beginColor, endColor);
    }

    int count = end - begin;
    int r1 = GetRValue(beginColor);
    int g1 = GetGValue(beginColor);
    int b1 = GetBValue(beginColor);
    int r2 = GetRValue(endColor);
    int g2 = GetGValue(endColor);
    int b2 = GetBValue(endColor);
    for (int i = 0; i <= count; i++)
    {
        clr[i + begin] = RGB(r1 + i * (r2 - r1) / count,
                             g1 + i * (g2 - g1) / count,
                             b1 + i * (b2 - b1) / count);
    }
}

wxString thPaletteData::SetPalette(wxString palette)
{
    Clear();

    //! todo: show character positions in all error messages.

    //palette.Replace(",", " ");

    //wxStringTokenizer st(palette, "()-,;", wxTOKEN_RET_DELIMS);
    palette.Replace(_T(","), _T(" "));
    palette.Replace(_T("\t"), _T(" "));
    palette.Replace(_T("\r"), _T(" "));
    palette.Replace(_T("\n"), _T(" "));
    MyTokenizer st(palette, _T("()-,;"));
    wxString nextToken, msg;
    ByteArray beginIndex, endIndex;
    COLORREF beginColor, endColor;
    bool bInRange;

    while (st.HasMoreTokens())
    {
        bInRange = false;
        if (!ReadList(st, beginIndex, nextToken))
        {
            msg.Printf(_T("Couldn't read list of values."));
            return msg;
        }
        if (!nextToken.IsSameAs('('))
        {
            msg.Printf(_T("Expected '(', got %s."), nextToken.c_str());
            return msg;
        }

        if (!::ReadColor(st, beginColor))
        {
            msg.Printf(_T("Couldn't read color."));
            return msg;
        }

        nextToken = st.GetNextToken();
        while (nextToken.IsSameAs('-'))
        {
            bInRange = true;
            if (!ReadList(st, endIndex, nextToken))
            {
                msg.Printf(_T("ReadList() failed."));
                goto exit;
            }
            if (!nextToken.IsSameAs('('))
            {
                msg.Printf(_T("Expected '(', got %s."), nextToken.c_str());
                goto exit;
            }

            if (!::ReadColor(st, endColor))
            {
                msg.Printf(_T("ReadColor() failed."));
                goto exit;
            }

            // set range of colors
            if (beginIndex.size() != endIndex.size())
            {
                msg.Printf(_T("Starting index list and ending index list are different sizes."));
                goto exit;
            }

            ByteArray::const_iterator beginIter = beginIndex.begin(), endIter = endIndex.begin();
            while (beginIter != beginIndex.end())
            {
                Add(*beginIter, beginColor, *endIter, endColor);
                beginIter++;
                endIter++;
            }

            beginIndex = endIndex;
            beginColor = endColor;
            nextToken = st.GetNextToken();
        }

        if ((!st.HasMoreTokens() || nextToken.IsSameAs(';')) && !bInRange)
        {
            // set single colors
            ByteArray::const_iterator beginIter = beginIndex.begin();
            while (beginIter != beginIndex.end())
            {
                clr[*beginIter] = beginColor;
                Add(*beginIter, beginColor);
                beginIter++;
            }
        }
    }
exit:
    Realize();
    return msg;
}

int thPalette::FindRange(int index) const
{
    // search for selected index in color ranges
    for (int i = Ranges() - 1; i >= 0; i--)
    {
        int b, e;
        Get(i, &b, NULL, &e, NULL);
        if (b > e && e >= 0)
            th_swap(b, e);
        if ((e == -1 && index == b) ||
            (index >= b && index <= e))
            return i;
    }
    return -1;
}

void thPaletteData::Clear()
{
    begin.Clear();
    end.Clear();
    beginColor.Clear();
    endColor.Clear();
    msg.Clear();

    memset(clr, 0, Colors() * sizeof(COLORREF));
}

void thPaletteData::Add(int begin, COLORREF beginColor,
                        int end /*= -1*/, COLORREF endColor /*= 0*/)
{
    this->begin.Add(begin);
    this->end.Add(end);
    this->beginColor.Add(beginColor);
    this->endColor.Add(endColor);
}

void thPaletteData::Realize()
{
    //! SetPalette() should call this instead of SetColorRange().

    // First set everything to the first color we have, or black.
    COLORREF firstColor = Ranges() ? beginColor[0] : 0;
    SetColorRange(0, 255, firstColor, firstColor, clr);

    for (size_t i = 0; i < begin.GetCount(); i++)
    {
        SetColorRange(begin[i], end[i], beginColor[i], endColor[i], clr);
    }
}

wxString thPaletteData::GetPalette() const
{
    // at each index:
    //   if i > 0 "; "
    //   search where (end == -1) and (bc is same); "n n n (clr)"
    //   search where (bc is same) and (ec is same) and ((end - begin) is same); "n n (clr) - n n (clr)"
    //   search where (begin == le) and (bc == lec); "n (clr) - n (clr) - ..."

    wxString ret, tmp;
    int lb = -1, le = -1;
    COLORREF lbc = -1, lec = -1;
    size_t count, count2;
    for (size_t i = 0; i < Ranges(); i += count * count2)
    {
        if (i > 0)
            ret += _T("; ");

        if (end[i] == -1)
        {
            count2 = 1;
            for (count = 1; i + count < Ranges(); count++)
            {
                if (end[i + count] >= 0 ||
                    beginColor[i + count] != beginColor[i])
                    break;
            }
            for (size_t j = 0; j < count; j++)
                ret += wxString::Format(_T("%d "), begin[i + j]);
            ret += (wxChar)'(' + FormatColour(beginColor[i]) + (wxChar)')';
        }
        else
        {
            // count how many ranges are being set at once
            for (count2 = 1; i + count2 < Ranges(); count2++)
            {
                if (end[i + count2] - begin[i + count2] != end[i] - begin[i] ||
                    beginColor[i + count2] != beginColor[i] ||
                    endColor[i + count2] != endColor[i])
                    break;
            }

            // count how many points are in each range
            for (count = 1; ; count++)
            {
                size_t k = i + (count * count2);
                size_t l = i + ((count - 1) * count2);
                if (k >= Ranges())
                    break;
                if (begin[k] != end[l] ||
                    beginColor[k] != endColor[l])
                    break;
                // make sure each range in the second group uses the same colors and distance
                for (size_t j = 1; j < count2; j++)
                {
                    if (k + j >= Ranges() ||
                        end[k + j] - begin[k + j] != end[k] - begin[k] ||
                        beginColor[k + j] != beginColor[k] ||
                        endColor[k + j] != endColor[k])
                        goto break_break;
                }
            }
break_break:
            for (size_t j = 0; j < count; j++)
            {
                for (size_t k = 0; k < count2; k++)
                    ret += wxString::Format(_T("%d "), begin[i + j * count2 + k]);
                ret += (wxChar)'(' + FormatColour(beginColor[i + j * count2]) + _T(") - ");
            }
            for (size_t k = 0; k < count2; k++)
                ret += wxString::Format(_T("%d "), end[i + (count - 1) * count2 + k]);
            ret += (wxChar)'(' + FormatColour(endColor[i + (count - 1) * count2]) + (wxChar)')';
        }
    }
    return ret;
}

bool thPaletteData::Get(size_t n, int *begin, COLORREF *beginColor, int *end, COLORREF *endColor) const
{
    if (n >= Ranges())
        return false;
    if (begin)
        *begin = this->begin[n];
    if (beginColor)
        *beginColor = this->beginColor[n];
    if (end)
        *end = this->end[n];
    if (endColor)
        *endColor = this->endColor[n];
    return true;
}

void thPaletteData::Nudge(COLORREF dst, float amount)
{
    int dr = GetRValue(dst);
    int dg = GetGValue(dst);
    int db = GetBValue(dst);

    if (amount < 0.0)
        amount = 0.0;
    else if (amount > 1.0)
        amount = 1.0;

    for (int i = 0; i < 256; i++)
    {
        int r = GetRValue(clr[i]);
        int g = GetGValue(clr[i]);
        int b = GetBValue(clr[i]);
        
        r += (dr - r) * amount;
        g += (dg - g) * amount;
        b += (db - b) * amount;

        clr[i] = RGB(r, g, b);
    }
}

wxObjectRefData *thPalette::CreateRefData() const
{
    return new thPaletteData(256);
}

wxObjectRefData * thPalette::CloneRefData(const wxObjectRefData * data) const
{
    return new thPaletteData(*(thPaletteData *)data);
}

// This code originally came from GIMP -- gimppalette-import.c
// I don't think they'd recognize it anymore.
wxString thPaletteData::Import(wxString filename)
{
    int         number_of_colors;
    int         i, j;
    /*Maximum valid file size: 256 * 4 * 3 + 256 * 2  ~= 3650 bytes */
    wxString    msg;
    wxTextFile  lines(filename);
    int         color_ints[3];

    lines.Open();
    if (lines[0] != _T("JASC-PAL")) // signature
        return _T("First line should be \"JASC-PAL\".");
    if (lines[1] != _T("0100")) // version
        return _T("Second line should be \"0100\".");

    number_of_colors = wxAtol(lines[2]);
    wxStringTokenizer st;

    for (i = 0; i < number_of_colors; i++)
    {
        const int line = i + 3;
        if (line >= (int)lines.GetLineCount())
            return wxString::Format(_T("File too short.  Expected %d more colors."), number_of_colors - i);

        st.SetString(lines[line]);
        if (st.CountTokens() != 3)
            return _T("Corrupted palette file.");

        for (j = 0; j < 3; j++)
        {
           color_ints[j] = wxAtol(st.GetNextToken());
           if (color_ints[j] < 0 || color_ints[j] > 255)
               return _T("Corrupted palette file.");
        }
        Add(i, RGB(color_ints[0], color_ints[2], color_ints[2]));
    }
    Realize();
    return wxEmptyString;
}
