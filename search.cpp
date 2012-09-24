///////////////////////////////////////////////////////////////////////////////
// Name:        MadEdit/MadEditSearch.cpp
// Description: searching and replacing functions
// Author:      madedit@gmail.com
// Licence:     GPL
///////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "hexdoc.h"
//#include "ucs4_t.h"
#include <iostream>
#include <string>
#include "utils.h"
#include "hexwnd.h"

//#include <boost/xpressive/xpressive.hpp>
//#include <boost/xpressive/xpressive_dynamic.hpp>
//#include <boost/xpressive/traits/null_regex_traits.hpp>
//#include <boost/xpressive/traits/cpp_regex_traits.hpp>

//using namespace std;
//using namespace boost::xpressive;

#define new New

template<typename char_type>
inline char_type xtolower(char_type ch)
{
    if(ch<0 || ch>0xFFFF) return ch;
    return towlower(wchar_t(ch));
}

template<>
inline wchar_t xtolower(wchar_t ch)
{
    return towlower(ch);
}

template<>
inline wxByte xtolower(wxByte ch)
{
    return ch;
}

template <typename char_type, typename Iter>
bool Search(Iter &begin, Iter &end, const char_type *wanted, size_t count, bool bCaseSensitive)
{
    wxASSERT(count != 0);

    size_t idx=0;
    Iter beginpos;
    char_type c1, c2;

    while(begin != end)
    {
        c1 = *begin;
        c2 = wanted[idx];

        if(bCaseSensitive==false)
        {
            c1=xtolower(c1);
            c2=xtolower(c2);
        }

        if(c1 == c2)
        {
            if(idx==0)
            {
                beginpos = begin;
            }
            ++idx;

            if(idx==count) // found whole wanted data
            {
                end = begin;
                ++end;
                begin = beginpos;
                return true;
            }
        }
        else // c1 != c2
        {
            if(idx!=0)
            {
                idx = 0;
                begin = beginpos;
            }
        }

        ++begin;
    }

    return false;
}

template <typename char_type, typename Iter>
bool SearchReverse(Iter &begin, Iter &end, const char_type *wanted, size_t count, bool bCaseSensitive)
{
    wxASSERT(count != 0);

    size_t idx=0;
    Iter beginpos;
    char_type c1, c2;

    while(begin != end)
    {
        c1 = *begin;
        c2 = wanted[idx];

        if(bCaseSensitive==false)
        {
            c1=xtolower(c1);
            c2=xtolower(c2);
        }

        if(c1 == c2)
        {
            if(idx==0)
            {
                beginpos = begin;
            }
            ++idx;

            if(idx==count) // found whole wanted data
            {
                end = begin;
                ++end;
                begin = beginpos;
                return true;
            }

            ++begin;
        }
        else // c1 != c2
        {
            if(idx!=0)
            {
                idx = 0;
                begin = beginpos;
            }
            --begin;
        }
    }

    return false;
}

// Hex Search
//struct ByteIterator
//{
//    typedef std::bidirectional_iterator_tag iterator_category;
//    typedef wxByte value_type;
//    typedef wxFileOffset difference_type;
//    typedef const value_type *pointer;
//    typedef const value_type &reference;
//
//    wxFileOffset    pos;
//    Segment         *pSeg;
//    wxFileOffset    linepos;
//
//    ByteIterator() {}
//
//    ByteIterator(const ByteIterator &it)
//    {
//        this->operator =( it );
//    }
//
//    ByteIterator(wxFileOffset pos0, Segment *pSeg0, wxFileOffset linepos0)
//        :pos(pos0), pSeg(pSeg0), linepos(linepos0)
//    {
//        if(linepos == pSeg->size && pSeg->next)
//        {
//            pSeg = pSeg->next;
//            linepos = 0;
//        }
//    }
//
//    ByteIterator & operator=(const ByteIterator & it)
//    {
//        pos=it.pos;
//        pSeg=it.pSeg;
//        linepos=it.linepos;
//        return *this;
//    }
//
//    const value_type operator*()
//    {
//        wxASSERT((uint64)linepos < pSeg->size);
//        return pSeg->Get(linepos);
//    }
//
//    /***
//    ucs4_t *operator->() const
//    {
//        return _ws_ + pos;
//    }
//    ***/
//
//    // pre-increment operator
//    ByteIterator & operator++()
//    {
//        ++pos;
//        ++linepos;
//
//        if(linepos == pSeg->size)
//        {
//            if(!pSeg->next) return *this; // end
//
//            pSeg = pSeg->next;
//            linepos = 0;
//        }
//
//        return *this;
//    }
//
//    /***
//    // post-increment operator
//    ByteIterator operator++(int)
//    {
//        ByteIterator tmp = *this;
//        ++*this;
//        return tmp;
//    }
//    ***/
//
//    //***
//    // pre-decrement operator
//    //ByteIterator & operator--()
//    //{
//    //    wxASSERT(pos>0);
//
//    //    --pos;
//
//    //    if(linepos == 0)
//    //    {
//    //        pSeg = pSeg->prev;
//    //        linepos = pSeg->size;
//    //    }
//    //    --linepos;
//
//    //    return *this;
//    //}
//    //***/
//
//    /***
//    // post-decrement operator
//    ByteIterator operator--(int)
//    {
//        ByteIterator tmp = *this;
//        --*this;
//        return tmp;
//    }
//    ***/
//
//    bool operator==(const ByteIterator & it) const
//    {
//        if(pos == it.pos) return true;
//        return (AtEnd() && it.AtEnd());
//    }
//
//    bool operator!=(const ByteIterator & it) const
//    {
//        return ! (this->operator==(it)) ;
//    }
//
//    bool AtEnd() const
//    {
//        return linepos == pSeg->size && pSeg->next == NULL;
//    }
//};

// Boyer-Moore search from Hexplorer

bool Find(bool forward,
          const uint8 *hexFind, int hexFind_size,
          const uint8 *psrc, THSIZE data_len,
          THSIZE &foundPosition)
{
    if (hexFind_size > data_len)
        return false;  //! todo: make this not happen

    // o ile bajtow mozna sie przesunac naprzod dla danego bajtu
    int shift_forward[256];
    for(int i = 0; i < 256; i++)
        shift_forward[i] = hexFind_size;
    for(int i = 0; i < hexFind_size; i++)
        shift_forward[hexFind[i]] = hexFind_size - i - 1;
    // o ile bajtow mozna sie przesunac w tyl dla danego bajtu
    int shift_backward[256];
    for(int i = 0; i < 256; i++)
        shift_backward[i] = hexFind_size;
    for(int i = hexFind_size - 1; i >= 0; i--)
        shift_backward[hexFind[i]] = i;
    // p - wskaznik na dane w pamieci z uwzglednieniem polozenia kursora i kierunku przeszukiwania
    const unsigned char *p = psrc + (forward ? hexFind_size - 1: data_len - hexFind_size);
    const unsigned char *end = psrc + data_len;
    if(!data_len || !hexFind_size || p < psrc)
        return 0;

    if(forward)
        for(int j =  hexFind_size - 1; j >= 0; j--, p--)
            while(*p != hexFind[j])
            {
                int x = shift_forward[*p];
                if(hexFind_size - j > x)
                    p += hexFind_size - j;
                else
                    p += x;
                if(p>=end)
                    return 0;
                j = hexFind_size - 1;
            }
    else
        for(int j = 0; j < hexFind_size; j++, p++)
            while(*p != hexFind[j])
            {
                int x = shift_backward[*p];
                if(j + 1 > x)
                    p -= j + 1;
                else
                    p -= x;
                if(p<psrc)
                    return 0;
                j = 0;
            }
    // ustawienie zaznaczenia na znalezionym ciagu
    if(forward)
        p++;
    else
        p-=hexFind_size;

    foundPosition = p - psrc;
    return 1; // znaleziono szukany ciag
}

class LowerCaseConverter
{
public:
    LowerCaseConverter()
    {
        table = new uint8[256];
        for (int i = 0; i < 256; i++)
            table[i] = (i >= 'A' && i <= 'Z') ? (i | 0x20) : i;
    }

    ~LowerCaseConverter()
    {
        delete [] table;
    }

    void convert(uint8 *pData, size_t count)
    {
        while (count--)
        {
            *pData = table[*pData];
            pData++;
        }
    }

    uint8 *table;
};


int HexDoc::FindHex(const wxByte *hex, size_t count,
                     /*IN_OUT*/THSIZE &startpos, /*IN_OUT*/THSIZE &endpos,
                     bool caseSensitive /*= true*/)
{
    if ((startpos <= endpos && startpos + count > endpos) ||
        (startpos >= endpos && endpos + count > startpos) ||
        count == 0)
        return false;

    //THSIZE startbase, endbase;
    //Segment *startseg = GetSegment(startpos, &startbase), *endseg;
    //if (endpos == GetSize())
    //    endseg = GetSegment(endpos - 1, &endbase);
    //else
    //    endseg = GetSegment(endpos, &endbase);
    //if (startseg == NULL || endseg == NULL)
    //    return false;

    //ByteIterator start(startpos, startseg, startpos - startbase);
    //ByteIterator end(endpos, endseg, endpos - endbase);
    ByteIterator2 start(startpos, this);
    ByteIterator2 end(endpos, this);
    uint32 blockSize = MEGA;
    THSIZE foundPos;
    uint8 *pData, *psrc;
    LowerCaseConverter lc;
    int result = false;
    if (caseSensitive)
        psrc = (uint8*)hex;
    else {
        pData = new uint8[blockSize];
        psrc = new uint8[count];
        memcpy(psrc, hex, count);
        lc.convert(psrc, count);
    }

    THSIZE findSize;
    if (startpos > endpos)
        findSize = startpos - endpos;
    else
        findSize = endpos - startpos;
    thProgressDialog *progress = NULL;
    if (findSize > MEGA)
        progress = new thProgressDialog(findSize, hw, _T("Searching..."));
    THSIZE pos = startpos, lastStart = startpos;

    if (startpos > endpos)
    {
        //if (::SearchReverse(start, end, hex, count, true))
        //{
        //    startpos = start.pos;
        //    endpos = end.pos;
        //    return true;
        //}

        for (pos = startpos; pos > endpos; )
        {
            if (pos - endpos < MEGA)
                blockSize = pos - endpos;
            if (pos < startpos && pos - blockSize > endpos)
                pos += count - 1;  // Go back and see if the end of the previous block matches.
            pos -= blockSize;
            if (caseSensitive) {
                pData = (uint8*)Load(pos, blockSize);
                if (!pData)  // compiler says "warning C4701: potentially uninitialized local variable 'pData' used."  WTF!?
                    goto done;
            }
            else {
                if (!Read(pos, blockSize, pData))
                    goto done;
                lc.convert(pData, blockSize);
            }
            if (::Find(false, psrc, count, pData, blockSize, foundPos))
            {
                startpos = pos + foundPos + count;
                endpos = startpos - count;
                result = true;
                goto done;
            }
            if (progress && !progress->Update(startpos - pos) &&
                Confirm(_T("Are you sure you want to abort?")))
                goto abort;
        }
    }
    //else if(::Search(start, end, hex, count, true))
    //{
    //    startpos = start.pos;
    //    endpos = end.pos;
    //    return true;
    //}
    else
    {
        for (pos = startpos; pos < endpos; pos += blockSize)
        {
            if (pos > startpos && pos + blockSize >= endpos)
                pos -= count - 1;  // Go back and see if the end of the previous block matches.
            if (endpos - pos < MEGA)
                blockSize = endpos - pos;
            if (caseSensitive) {
                pData = (uint8*)Load(pos, blockSize);
                if (!pData)
                    goto done;
            }
            else {
                if (!Read(pos, blockSize, pData))  // aligned read is better than cache-bypassing DoRead().
                    goto done;
                lc.convert(pData, blockSize);
            }
            if (::Find(true, psrc, count, pData, blockSize, foundPos))
            {
                startpos = pos + foundPos;
                endpos = startpos + count;
                result = true;
                goto done;
            }
            if (progress && !progress->Update(pos - startpos) &&
                Confirm(_T("Are you sure you want to abort?")))
                goto abort;
        }
    }

done:
    if (!caseSensitive)
    {
        delete [] pData;
        delete [] psrc;
    }

    delete progress;

    return result;

abort:
    hw->ScrollToRange(pos, pos, J_AUTO);
    result = -1;  // aborted
    goto done;
}
