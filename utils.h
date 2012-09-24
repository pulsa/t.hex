#pragma once

#ifndef _UTILS_H_
#define _UTILS_H_

#include "defs.h"

wxString GetLastErrorMsg(int code = 0);
void fatal(LPCTSTR msg);

class thString;

// declare a hash map with string keys and int values
WX_DECLARE_STRING_HASH_MAP( int, thStringHash_internal );

class thStringHash : public thStringHash_internal
{
public:
    bool has_key(const wxString &k)
    {
        return find(k) != end();
    }
};

void CreateConsole(LPCTSTR title);

template<class T>void th_swap(T& x, T& y){
    T temp = x;
    x = y;
    y = temp;
}

template<class T>
inline T RoundDown(T a, unsigned factor)
{
    return a - a % factor;
}

template<class T>
inline T RoundUp(T a, unsigned factor)
{
    unsigned rem = a % factor;
    if (rem)
        return a + (factor - rem);
    return a;
}

template<class T>
inline T DivideRoundUp(T a, unsigned factor)
{
    return (a + factor - 1) / factor;
}

template<class CHAR_T> void my_itoa(uint32 i, CHAR_T *buffer, int base, int digits);
template<class CHAR_T> void my_itoa(uint64 i, CHAR_T *buffer, int base, int digits);
// These functions take signed input and print either a minus sign or space on the front.
template<class CHAR_T> void my_itoa(int64 i, CHAR_T *buffer, int base, int digits);
template<class CHAR_T> int my_ftoa(float f, CHAR_T *buffer, int digits);
template<class CHAR_T> int my_dtoa(const double &d, CHAR_T *buffer, int digits);

template void my_itoa(uint32 i, char    *buffer, int base, int digits);
template void my_itoa(uint32 i, wchar_t *buffer, int base, int digits);
template void my_itoa(uint64 i, char    *buffer, int base, int digits);
template void my_itoa(uint64 i, wchar_t *buffer, int base, int digits);
template int my_ftoa(float f, TCHAR *buffer, int digits);
template int my_dtoa(const double &d, TCHAR *buffer, int digits);

extern const bool my_isprint[256];

int CountDigits(uint64 i, int base);
wxString StripSpaces(wxString in);

class HexUtils
{
public:
    HexUtils() // dummy constructor to initialize HexTable
    {
        memset(HexTable, 0, 256);
        for (int i = 0; i < 10; i++)
            HexTable['0' + i] = i;
        for (int i = 0; i < 6; i++)
            HexTable['A' + i] = HexTable['a' + i] = 0x0A + i;
    }

    static inline uint8 NibbleVal(TCHAR c)
    {
        return HexTable[(uint8)c];
    }

    static inline unsigned char atoi(LPCTSTR hex)
    {
        return ((NibbleVal(hex[0]) << 4) + NibbleVal(hex[1]));
    }

protected:
    static uint8 HexTable[256];
};

wxString TextToHex(wxString text);
wxString TextToHex(const uint8 *text, size_t length);
wxString HexToText(wxString hex);
size_t HexToText(wxString hex, uint8 *buf, size_t bufferSize);
thString unhexlify(wxString hex);  // convert hex string to an array of BYTES, not characters.

inline bool RangeOverlapSize(THSIZE start1, THSIZE size1, THSIZE start2, THSIZE size2)
{
    return (start1 + size1 > start2 && start1 < start2 + size2);
}

inline bool RangeOverlapSizeI(int start1, int size1, int start2, int size2)
{
    return (start1 + size1 > start2 && start1 < start2 + size2);
}

inline bool RangeOverlapProper(THSIZE start1, THSIZE end1, THSIZE start2, THSIZE end2)
{
    return (start1 < end2 && start2 < end1);
}

inline bool RangeOverlap(THSIZE start1, THSIZE end1, THSIZE start2, THSIZE end2)
{
    if (start1 > end1) th_swap(start1, end1);
    if (start2 > end2) th_swap(start2, end2);
    return RangeOverlapProper(start1, end1, start2, end2);
}

//template <class T>
bool ReadUserNumber(wxString s, uint8 &n);
bool ReadUserNumber(wxString s, uint32 &n);
bool ReadUserNumber(wxString s, size_t &n);
bool ReadUserNumber(wxString s, uint64 &n);
bool ReadUserNumber(wxString s, int32 &n);

inline size_t ReadUserNumber(wxString s) { size_t n; if (!ReadUserNumber(s, n)) n = 0; return n; }

//wxString FormatNumber(int32 n);
//wxString FormatNumber(uint32 n);
//wxString FormatNumber(int64 n);
//wxString FormatNumber(uint64 n);
wxString FormatDouble(double n, int decimalPlaces = 0);

//wxString FormatHumanReadable(uint64 n, TCHAR &prefix, int decDigits = 2);
//wxString FormatBytes(uint64 n, uint64 no_prefix_cutoff = 0, int decDigits = 2);
wxString FormatWithBase(THSIZE n, int base);
const TCHAR *FormatBytes(uint64 n, uint64 no_prefix_cutoff = 0);

//size_t FormatHex(uint64 n, char *buffer, size_t bufferSize, size_t digits = 0);
//size_t FormatDec(uint64 n, char *buffer, size_t bufferSize, size_t digits = 0);
//size_t FormatBin(uint64 n, char *buffer, size_t bufferSize, size_t digits = 0);
size_t FormatNumber(uint64 n, TCHAR *buffer, size_t bufferSize, int base, size_t digits = 0);
inline wxString FormatNumber(uint64 n, int base, size_t digits = 0)
{
   TCHAR s[99]; return wxString(s, FormatNumber(n, s, 99, base, digits));
}

inline wxString FormatHex(uint64 n) { return FormatNumber(n, 16); }
inline wxString FormatDec(uint64 n) { return FormatNumber(n, 10); }
inline wxString FormatBin(uint64 n) { return FormatNumber(n,  2); }
wxString FormatDec(int64 n);

wxString FormatColour(const wxColour &clr);
inline wxString FormatOffset(THSIZE n) { return _T("0x") + FormatHex(n); }

extern void reverse(uint8 *data, uint32 len);
inline void reverse(THSIZE *data) { reverse((uint8*)data, sizeof(THSIZE)); }
inline void reverse(int *data) { reverse((uint8*)data, sizeof(int)); }

inline uint32 reverse(uint32 n) { return (((n << 16) | (n & 0xFF00)) << 8) | (((n & 0xFF0000) | (n >> 16)) >> 8); }
#define my_htonl reverse
#define my_ntohl reverse

void ibm2ieee(void *to, const void *from, int len);
void ieee2ibm(void *to, const void *from, int len);

bool EnableDebugPriv( void );
TCHAR GetDriveLetter(LPCTSTR lpDevicePath, LPCTSTR *rest);
wxString GetRealFilePath(LPCTSTR path1, HANDLE hProcess, void *address);
wxString Unescape(wxString src);
wxString Escape(wxString src);

//int bitcount(unsigned int n);
int bitcount(uint64 n);

//wxPoint MoveInside(const wxRect &rc, const wxPoint &pt, int tolX = 0, int tolY = 0);

class ProcList
{
public:
    ProcList(HANDLE hSnapshot = NULL) { Init(hSnapshot); }

    void Init(HANDLE hSnapshot = NULL);
    wxImageList* MakeImageList();

    wxString NameFromPID(DWORD pid);
    DWORD PIDFromName(wxString name);

    int Index(DWORD pid) { return pids.Index(pid); }

    DWORD GetPID(int index) { return pids[index]; }
    wxString GetName(int index) { return procNames[index]; }
    wxString GetPath(int index) { return fullPaths[index]; }
    wxString GetTitle(int index) { return windowTitle[index]; }
    wxString GetDescription(int index) { return descr[index]; }
    size_t GetCount() { return procNames.GetCount(); }
    //int GetIcon(int index) { return hIcons[index] ? iconIndex++ : -1; }
    int GetIcon(int index) { return imageListIconIndex[index]; }

    // Other ways to get process name from PID:
    // GetModuleFileNameEx(hProcess, NULL, path, sizeof(path)) (WinNT - mostly returns full path)
    // http://www.elists.org/pipermail/delphi-talk/2004-September/019577.html

private:
    wxArrayString procNames;
    wxArrayString fullPaths;
    wxArrayInt pids; //w64 -- int is close enough to DWORD for now
    //wxArrayInt hIcons; //w64 -- same here.  HICON is currently 32 bits.
    wxArrayInt imageListIconIndex;
    wxArrayString windowTitle;
    wxArrayString descr;
    int iconIndex;

    static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
    void EnumWindowsProc(HWND hWnd);
};

class DblBufWindow
{
public:
    wxBitmap m_bmp;

    void UpdateBitmapSize(wxSize clientSize)
    {
        if (m_bmp.Ok() && clientSize == wxSize(m_bmp.GetWidth(), m_bmp.GetHeight()))
            return;
        m_bmp.Create(clientSize.x, clientSize.y);
    }
};

inline void Copy4(void *dst, const void *src, bool reverse = false)
{
    if (reverse)
    {
        ((char*)dst)[0] = ((char*)src)[3];
        ((char*)dst)[1] = ((char*)src)[2];
        ((char*)dst)[2] = ((char*)src)[1];
        ((char*)dst)[3] = ((char*)src)[0];
    }
    else
    {
        ((char*)dst)[0] = ((char*)src)[0];
        ((char*)dst)[1] = ((char*)src)[1];
        ((char*)dst)[2] = ((char*)src)[2];
        ((char*)dst)[3] = ((char*)src)[3];
    }
}

inline void Copy8(void *dst, const void *src, bool reverse = false)
{
    //! can we ensure that data will always be 64-bit aligned?
    if (reverse)
    {
        ((char*)dst)[0] = ((char*)src)[7];
        ((char*)dst)[1] = ((char*)src)[6];
        ((char*)dst)[2] = ((char*)src)[5];
        ((char*)dst)[3] = ((char*)src)[4];
        ((char*)dst)[4] = ((char*)src)[3];
        ((char*)dst)[5] = ((char*)src)[2];
        ((char*)dst)[6] = ((char*)src)[1];
        ((char*)dst)[7] = ((char*)src)[0];
    }
    else
    {
        ((char*)dst)[0] = ((char*)src)[0];
        ((char*)dst)[1] = ((char*)src)[1];
        ((char*)dst)[2] = ((char*)src)[2];
        ((char*)dst)[3] = ((char*)src)[3];
        ((char*)dst)[4] = ((char*)src)[4];
        ((char*)dst)[5] = ((char*)src)[5];
        ((char*)dst)[6] = ((char*)src)[6];
        ((char*)dst)[7] = ((char*)src)[7];
    }
}

inline uint32 Read4(const void *src, bool reverse = false)
{
    uint32 tmp;
    Copy4(&tmp, src, reverse);
    return tmp;
}

inline uint32 Read8(const void *src, bool reverse = false)
{
    uint64 tmp;
    Copy8(&tmp, src, reverse);
    return tmp;
}

inline uint32 Read4Little(const void *src)
{
    return Read4(src, NATIVE_ENDIAN_MODE != LITTLEENDIAN_MODE);
}
typedef size_t Py_ssize_t;
wxString PyString_DecodeEscape(const char *s,
                               Py_ssize_t len,
                               const char *errors,
                               Py_ssize_t unicode,
                               const char *recode_encoding);

class thMutexLocker
{
public:
   thMutexLocker(HANDLE mutex, bool bypass = false)
   {
      if (bypass)
         m_mtx = 0;
      else
         WaitForSingleObject(m_mtx = mutex, INFINITE);
   }
   ~thMutexLocker() { if (m_mtx) ReleaseMutex(m_mtx); }
private:
   HANDLE m_mtx;
};

wxString Plural(THSIZE n, LPCTSTR s1 = NULL, LPCTSTR s2 = NULL);

#define FLAGS(f, mask)  (((f) & (mask)) == (mask))

int ClearWxListSelections(wxListCtrl *list);
//int FindFirstWxListSelection(wxListCtrl *list);
//int FindWxListFocus(wxListCtrl *list);
int AutoSizeVirtualWxListColumn(wxListCtrl *list, int col, wxString longestData);
//int AutoSizeVirtualWxListColumn(wxListCtrl *list, int col, wxArrayString longestData);

wxString GetFileNameFromHandle(HANDLE hFile);

// Quick class that works like std::vector<bool>.
// AEB  2008-03-24
#define WORDSIZE 32  //! x86 hack.  Change for x64 someday.
#define WORDSHIFT 5

class BitArray
{
public:
    BitArray(size_t nBits = 0) { m_data = NULL; Init(nBits); }
    ~BitArray() { Clear(); }

    size_t Init(size_t nBits)
    {
        Clear();
        m_nBits = nBits;
        m_nWords = (nBits + WORDSIZE - 1) / WORDSIZE;
        m_data = new unsigned int[m_nWords];
        ClearAll();
        return m_nWords;
    }

    void Clear()
    {
        delete m_data;
        m_nWords = 0;
        m_data = NULL;
    }

    inline void SetBit(size_t bit)
    {
        m_data[bit >> WORDSHIFT] |= (1 << (bit & (WORDSIZE - 1)));
    }

    inline void ClearBit(size_t bit)
    {
        m_data[bit >> WORDSHIFT] &= ~(1 << (bit & (WORDSIZE - 1)));
    }

    void ClearAll()
    {
        memset(m_data, 0, m_nWords * sizeof(m_data[0]));
    }

    inline int operator[](size_t bit)
    {
        return (m_data[bit >> WORDSHIFT] >> (bit & (WORDSIZE - 1))) & 1;
    }

    size_t m_nWords;
    size_t m_nBits;
    unsigned int *m_data;
};

//*****************************************************************************
//*****************************************************************************
// FastAppendVector
//*****************************************************************************
//*****************************************************************************

// This class is similar to ModifyBuffer.
// By using a vector of small arrays, we enjoy extremely fast append operations,
// without sacrificing much anywhere else.
template<class T, size_t BLOCK_SIZE>
class FastAppendVector
{
public:
    FastAppendVector() { size = 0; }

    ~FastAppendVector() { clear(); }

    void clear()
    {
        for (size_t i = 0; i < ptrs.size(); i++)
            delete [] ptrs[i];
        ptrs.clear();
        size = 0;
    }

    T& operator[](size_t index)
    {
        return ptrs[index / BLOCK_SIZE][index % BLOCK_SIZE];
    }

    void push_back(const T& t)
    {
        size_t offset = size % BLOCK_SIZE;
        if (offset == 0)
            ptrs.push_back(new T[BLOCK_SIZE]);
        ptrs[size / BLOCK_SIZE][offset] = t;
        size++;
    }

    size_t GetSize() { return size; }

private:
    size_t size;
    std::vector<T*> ptrs;
};

class thRefCount
{
public:
    thRefCount() {
        m_nRefs = 1;
    }

    inline int AddRef() {
        m_nRefs++; return m_nRefs;
    }

    inline int Release() {
        wxASSERT(m_nRefs > 0);
        int tmp = --m_nRefs; if (tmp == 0) delete this; return tmp;
    }

protected:
    int m_nRefs;
};

//*************************************************************************************************
// thString
// Yet another string class.  It's really an array of bytes with wxString's memory management.
// At this point, in fact, it's a wrapper around wxString.
// wxString::Len() depends on the size of TCHAR, and there is no way to represent
// odd number of bytes in Unicode mode with wxString.  Hence, this class.
//*************************************************************************************************

class thString
{
public:
    wxString m_str;
    //uint8 *m_data;
    size_t m_len;

    thString(const uint8 *data, size_t bytes);
    //thString(const char *data, size_t chars = 0);
    //thString(const wchar_t *data, size_t chars = 0);
    thString(const wxString &src = wxEmptyString, size_t bytes = 0) { SetString(src, bytes); }
    thString(size_t bufferSize);

    static thString ToANSI(const char *data, size_t chars = 0);
    static thString ToANSI(const wchar_t *data, size_t chars = 0);
    static thString ToUnicode(const char *data, size_t chars = 0);
    static thString ToUnicode(const wchar_t *data, size_t chars = 0);

    //const uint8 *data() const { return m_data; }
    const uint8 *data() const { return (const uint8*)(LPCTSTR)m_str.c_str(); }
    size_t len() const { return m_len; }

    //uint8 *GetWriteBuffer(size_t bytes);
    //void UngetWriteBuf(size_t bytes);
    void SetString(const wxString &src, size_t bytes = 0)
    {
        m_str = src;
        if (bytes)
            m_len = bytes;
        else
            m_len = src.Len() * sizeof(TCHAR);
        //m_data = (uint8*)m_str.data();
    }
};

//*************************************************************************************************
// thProgressDialog
// Adds automatic speed monitoring to wxProgressDialog.
//*************************************************************************************************

class thProgressDialog : public wxProgressDialog
{
public:
    thProgressDialog(
        THSIZE range,
        wxWindow *parent,
        wxString msg = _T("Reading..."),
        wxString caption = _T("T. Hex"),
        DWORD updateInterval = 1000);
    virtual bool Update(THSIZE value, bool showSpeed = true, wxString extraMsg = ZSTR);

    virtual void SetUpdateInterval(DWORD updateInterval);
    virtual void SetSpeedScale(double scale);

    THSIZE m_range, m_lastStart[2];
    DWORD m_startTime;
    DWORD m_lastUpdateTime[2];
    DWORD m_updateInterval;
    ATimer updateTimer;  // Update the speed message periodically.
    wxString m_msg;
    double m_speedScale;
};

bool Confirm(wxString msg, wxString caption = _T("T. Hex"), wxWindow *parent = NULL);

#endif // _UTILS_H_
