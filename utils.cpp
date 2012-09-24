#include "precomp.h"
#include "utils.h"
#include "settings.h"
#include "thex.h"
#include <wx/regex.h>

#define new New

wxString GetLastErrorMsg(int code /*= 0*/)
{
    if (!code)
        code = GetLastError();

    wxString s;
    if (!code)
        //return "No error.\n";
        return s;

    if (FormatMessage(
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        wxStringBuffer(s, 1024),
        1024, NULL))
    {
        s.Prepend(wxString::Format(_T("Code %d: "), code));
    }
    else
    {
        s.Printf(_T("UNKNOWN_EXCEPTION(%d)\n"), code);
    }
    if (s.Right(1) != '\n')
        s += '\n';
    return s;
}

void fatal(LPCTSTR msg)
{
    //extern HexWnd *g_hexWnd;
    MessageBox(NULL, msg, APPNAME, MB_OK);

    //PostQuitMessage(0); // This prevents memory leaks, but not errors.
    // Turn off leak reporting.  //! ?
    ExitProcess(0); // This prevents errors.
}

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
void CreateConsole(LPCTSTR title)
{
    if (!AllocConsole())
        return;

    // this code from MSDN'S "Calling CRT Output Routines from a GUI Application"
    int hCrt;

    hCrt = _open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT);
    *stdin =  *_fdopen( hCrt, "r" );
    setvbuf(stdin, NULL, _IONBF, 0);

    hCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    *stdout = *_fdopen( hCrt, "w" );
    setvbuf(stdout, NULL, _IONBF, 0);

    hCrt = _open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
    *stderr = *_fdopen( hCrt, "w" );
    setvbuf(stderr, NULL, _IONBF, 0);

#if _WIN32_WINNT >= 0x0500
    HWND hWnd = GetConsoleWindow();
#else
    TCHAR tmp[200];
    _sntprintf(tmp, 200, _T("Unique %d %d %d"), rand(), GetCurrentThreadId(), GetCurrentProcessId());
    SetConsoleTitle(tmp);
    HWND hWnd = FindWindow(NULL, tmp);
#endif
    SetConsoleTitle(title);
    //SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE);
}

const char hexdigits[] = "0123456789ABCDEF";
/*void itoh(uint32 i, char *buffer, int digits)
{
	for (int digit = 0; digit < digits; digit++)
	{
		buffer[digits - digit - 1] = hexdigits[i & 0x0F];
		i >>= 4;
	}
	//! buffer[digits] = 0;
}*/

template<class CHAR_T>
void my_itoa(uint32 i, CHAR_T *buffer, int base, int digits)
{
    for (int digit = 0; digit < digits; digit++)
    {
        buffer[digits - digit - 1] = hexdigits[i % base];
        i /= base;
    }
}

// same function, with 64-bit input
template<class CHAR_T>
void my_itoa(uint64 i, CHAR_T *buffer, int base, int digits)
{
    for (int digit = 0; digit < digits; digit++)
    {
        buffer[digits - digit - 1] = hexdigits[i % base];
        i /= base;
    }
}

// prints either minus sign or space; not used as of 2006-09-09
template<class CHAR_T>
void my_itoa(int64 i, CHAR_T *buffer, int base, int digits)
{
    if (i < 0)
    {
        *buffer++ = '-';
        i = -i;
    }
    else
        *buffer++ = ' ';
    if (i <= 0xFFFFFFFFUL)
        my_itoa((uint32)i, buffer, base, digits);
    else
        my_itoa((uint64)i, buffer, base, digits);
}

//  int my_ftoa(float  f, char *buffer, int digits)
//  int my_dtoa(double d, char *buffer, int digits)
// These floating point conversion functions are for screen output.
// The output buffer must be at least (digits+7) bytes long.
// No terminator is appended.
// The first character written to the buffer is either '-' or ' '.
// The return value is the number of characters in the output string.

template<class CHAR_T>
int my_ftoa(float f, CHAR_T *buffer, int digits)
{
    // check for special cases directly
    uint32 raw = *(uint32*)&f;
    if ((raw << 1) == 0) // shift away the sign bit and check for zero
    {
        buffer[0] = (raw >> 31) ? '-' : ' ';
        buffer[1] = '0';
        return 2;
    }

    if ((raw & 0x7F800000) == 0x7F800000) // f is either +/- INF or NAN
    {
        buffer[0] = (raw >> 31) ? '-' : ' ';
        if (raw & 0x7FFFFF)
        {
            buffer[1] = 'N';
            buffer[2] = 'a';
            buffer[3] = 'N';
        }
        else
        {
            buffer[1] = 'I';
            buffer[2] = 'n';
            buffer[3] = 'f';
        }
        return 4;
    }

    int dec, sign;
    char *cvtbuf = _ecvt(f, digits, &dec, &sign);
    int chars = 3;
    if (sign)
        buffer[0] = '-';
    else
        buffer[0] = ' ';
    buffer[1] = *cvtbuf++;
    buffer[2] = '.';
    for (int digit = 1; digit < digits; digit++)
        buffer[chars++] = *cvtbuf++;
    buffer[chars++] = 'e';
    dec--;
    if (dec < 0)
    {
        buffer[chars++] = '-';
        my_itoa((uint32)-dec, buffer + chars, 10, 2);
    }
    else
    {
        buffer[chars++] = '+';
        my_itoa((uint32)dec, buffer + chars, 10, 2);
    }
    return chars + 2;
}

template<class CHAR_T>
int my_dtoa(const double &d, CHAR_T *buffer, int digits)
{
    // check for special cases directly
    uint64 raw = *(uint64*)&d;
    if ((raw << 1) == 0) // shift away the sign bit and check for zero
    {
        buffer[0] = (raw >> 63) ? '-' : ' ';
        buffer[1] = '0';
        return 2;
    }

    if ((raw & 0x7FF0000000000000LL) == 0x7FF0000000000000LL) // d is either +/- Inf or NaN
    {
        buffer[0] = (raw >> 63) ? '-' : ' ';
        if (raw & 0xFFFFFFFFFFFFFLL)
        {
            buffer[1] = 'N';
            buffer[2] = 'a';
            buffer[3] = 'N';
        }
        else
        {
            buffer[1] = 'I';
            buffer[2] = 'n';
            buffer[3] = 'f';
        }
        return 4;
    }

    int dec, sign;
    char *cvtbuf = _ecvt(d, digits, &dec, &sign);
    int chars = 3;
    if (sign)
        buffer[0] = '-';
    else
        buffer[0] = ' ';
    buffer[1] = *cvtbuf++;
    buffer[2] = '.';
    for (int digit = 1; digit < digits; digit++)
        buffer[chars++] = *cvtbuf++;
    buffer[chars++] = 'e';
    dec--;
    if (dec < 0)
    {
        buffer[chars++] = '-';
        my_itoa((uint32)-dec, buffer + chars, 10, 3);
    }
    else
    {
        buffer[chars++] = '+';
        my_itoa((uint32)dec, buffer + chars, 10, 3);
    }
    return chars + 3;
}

const bool my_isprint[256] = {
    0,0,0,0,0,0,0,0, 0,1,1,0,0,1,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,0,
};


int CountDigits(uint64 i, int base)
{
    if (base < 2)
        return -1;
    int digits = 1;
    while (i >= base)
        i /= base, digits++;
    return digits;
}

wxString StripSpaces(wxString in)
{
    const size_t inLen = in.Len();
    // Look for spaces.  If we don't find any, return early.
    for (size_t n = 0; n < inLen; n++)
        switch((wxChar)in[n])
        {
        case ' ': case '\t': case '\r': case '\n':  goto found_space;
        }
    return in;
found_space:
    wxString out;
    {
    wxStringBuffer wxbuf(out, inLen);
    TCHAR *buf = wxbuf;
    size_t outLen = 0;
    for (size_t n = 0; n < inLen; n++)
        switch(TCHAR c = in[n])
        {
        case ' ': case '\t': case '\r': case '\n':  break;
        default:
            buf[outLen++] = c;
        }
    buf[outLen] = 0;
    } // release wxStringBuffer
    return out;
}

wxString TextToHex(wxString text)
{
    wxString hex;
    size_t length = text.Length();
    if (length == 0)
        return text;
    {
    wxStringBuffer wxbuf(hex, length * 3);
    TCHAR *buf = wxbuf;
    for (size_t i = 0; i < length; i++)
    {
        my_itoa((uint32)(uint8)text[i], buf, 16, 2);
        buf[2] = ' ';
        buf += 3;
    }
    buf[-1] = 0;
    }
    return hex;
}

wxString TextToHex(const uint8 *text, size_t length)
{
    wxString hex;
    if (length == 0)
        return hex;
    {
    wxStringBuffer strbuf(hex, length * 3);
    TCHAR *buf = strbuf;
    for (size_t i = 0; i < length; i++)
    {
        my_itoa((uint32)(uint8)text[i], buf, 16, 2);
        buf[2] = ' ';
        buf += 3;
    }
    buf[-1] = 0;
    }
    return hex;
}

uint8 HexUtils::HexTable[256];
static HexUtils dummyHexUtils;

wxString HexToText(wxString hex)
{
    hex = StripSpaces(hex);
    wxString text;
    size_t len = hex.Len() / 2;
    if (len == 0)  return text;

    wxStringBuffer wxbuf(text, len);
    TCHAR *buf = wxbuf;
    const TCHAR *src = hex;
    for (size_t i = 0; i < len; i++)
        *buf++ = HexUtils::atoi(&src[i * 2]);
    return text;
}

size_t HexToText(wxString hex, uint8 *buf, size_t bufferSize)
{
    hex = StripSpaces(hex);
    size_t len = hex.Len() / 2;
    if (len == 0)  return 0;

    const wxChar* hexbuf = hex;
    for (size_t i = 0; i < len && i < bufferSize; i++)
        *buf++ = HexUtils::atoi(&hexbuf[i * 2]);
    return len;
}

//! bleah.  This is getting out of hand.
//template <class srcT, class dstT>
//size_t HexToText(const T *hex, dstT *buf, size_t bufferSize)
//{
//    hex = StripSpaces(hex);
//    size_t len = hex.Len() / 2;
//    if (len == 0)  return 0;
//
//    for (size_t i = 0; i < len && i < bufferSize; i++)
//        *buf++ = (HexUtils::NibbleVal(hex[i*2]) << 4) + HexUtils::NibbleVal(hex[i*2+1]);
//    return len;
//}

//! This could run faster if we didn't convert source to wxString first.
thString unhexlify(wxString hex)  // convert hex string to an array of BYTES, not characters.
{
    wxString text;
    size_t nBytes = hex.Len() / 2;
    if (nBytes == 0)  return text;

    size_t nChars = DivideRoundUp(nBytes, sizeof(TCHAR));
    //uint8 *buf = (uint8*)text.GetWriteBuf(nChars);
    wxStringBuffer wxbuf(text, nChars);
    uint8 *buf = (uint8*)((TCHAR*)wxbuf);
    nBytes = HexToText(hex, buf, nBytes);
    return thString(text, nBytes);
}


int GetBase(wxString s, wxString &rest)
{
    int base;

    // first see if we can identify the number base by using prefixes
    if (s.StartsWith(_T("0x"), &rest) ||
        s.StartsWith(_T("0X"), &rest) ||
        s.StartsWith(_T("h"), &rest) ||
        s.StartsWith(_T("x"), &rest) ||
        s.StartsWith(_T("$"), &rest) ||
        s.StartsWith(_T("#"), &rest) ||
        s.StartsWith(_T("&H"), &rest) ||
        s.StartsWith(_T("&h"), &rest))
        base = 16;
    else if (s.StartsWith(_T("0o"), &rest) ||
        s.StartsWith(_T("0O"), &rest)) // I don't like octal, by the way.
        base = 8;
    else if (s.StartsWith(_T("0b"), &rest) ||
        s.StartsWith(_T("0B"), &rest))
        base = 2;
    // then check suffixes
    else if (!s.Right(1).CmpNoCase(_T("h")))
        rest = s.RemoveLast(1), base = 16;
    else if (!s.Right(1).CmpNoCase(_T("d")))
        rest = s.RemoveLast(1), base = 10;
    else if (!s.Right(1).CmpNoCase(_T("o")))
        rest = s.RemoveLast(1), base = 8;
    else if (!s.Right(1).CmpNoCase(_T("b")))
        rest = s.RemoveLast(1), base = 2;
    //! b and d are valid hex digits.  Check these last.
    else if (s.StartsWith(_T("d"), &rest))
        base = 10;
    else if (s.StartsWith(_T("b"), &rest))
        base = 2;
    else
        rest = s, base = 10;
    return base;
}

bool ReadUserChar(const wxString &s, uint8 &c)
{
    if (s.Len() == 3 && s[0] == '\'' && s[2] == '\'')
    {
        c = s[1];
        return true;
    }
    if (s.Len() == 4 && s[0] == '\'' && s[1] == '\\' && s[3] == '\'')
    {
        wxChar tmp = s[2];
        switch (tmp)
        {
        case 'r':  c = '\r'; break;
        case 'n':  c = '\n'; break;
        case 't':  c = '\t'; break;
        case 'b':  c = '\b'; break;
        default:
            c = s[2];
            return true; //! questionable
        }
        return true;
    }
    c = -1;
    return false;
}

// Three different versions of ReadUserNumber, with different data types.
//! They all do different things.  Bad, bad, bad.
bool ReadUserNumber(wxString s, uint32 &n)
{
    //s = s.Trim(true).Trim(false);
    s.Replace(_T(" "), ZSTR);
    s.Replace(_T("_"), ZSTR);

    TCHAR *endptr;
    wxString rest;
    int base = GetBase(s, rest);
    n = _tcstoul(rest, &endptr, base);
    return (*endptr) == 0;
}

bool ReadUserNumber(wxString s, uint8 &n)
{
    if (s.IsEmpty())
        return false;
    s = s.Trim(true).Trim(false);
    if (ReadUserChar(s, n))
        return true;
    TCHAR *endptr;
    wxString rest;
    int base = GetBase(s, rest);

    uint32 n32 = _tcstoul(rest, &endptr, base);
    if (n32 > 255)
        return false;
    n = (uint8)n32;
    return (*endptr == 0);
}

bool ReadUserNumber(wxString s, uint64 &n)
{
    s.Replace(_T(" "), ZSTR);
    s.Replace(_T("_"), ZSTR);
    s.Replace(_T(":"), ZSTR);
    s.Replace(_T(","), ZSTR);

    TCHAR *endptr;
    wxString rest;
    int base = GetBase(s, rest);
    n = _tcstoui64(rest, &endptr, base);
    return (*endptr) == 0;
}

//! moderately unsafe cast
bool ReadUserNumber(wxString s, size_t &n) { return ReadUserNumber(s, (uint32&)n); }

bool ReadUserNumber(wxString s, int32 &n)
{
    s.Replace(_T(" "), ZSTR);
    s.Replace(_T("_"), ZSTR);
    int sign = 1;
    if (s.Len() > 1 && s[0] == '-') {
        sign = -1;
        s = s.Mid(1);
    }

    TCHAR *endptr;
    wxString rest;
    int base = GetBase(s, rest);
    n = _tcstoul(rest, &endptr, base) * sign;
    return (*endptr) == 0;
}

wxString Printable(wxString src)
{
    wxString s;
    s.Alloc(src.Len());
    for (size_t i = 0; i < src.Len(); i++)
    {
        int c = (unsigned)(TCHAR)src[i];
        if (c == '\\')
            s += _T("\\\\");
        else if (c == '\n')
            s += _T("\\n");
        else if (c == '\r')
            s += _T("\\r");
        else if (c == '\t')
            s += _T("\\t");
        else if (c == '\b')
            s += _T("\\b");
        else if (!isprint(c))
            s += wxString::Format(_T("\\x%02X"), c);
        else
            s += c;
    }
    return s;
}

wxString Unescape(wxString src)
{
    wxString s;
    const size_t len = src.Len();
    s.Alloc(src.Len());
    for (size_t i = 0; i < len; i++)
    {
        int c = (unsigned)src[i];

        if (c == '\r' || c == '\n' || c == '\t')  // skip unescaped whitespace
            continue;

        if (c == '\\' && i < len - 1)
        {
            wxChar tmp = src[i+1];
            switch (tmp)
            {
            case '\r':
            case 'r':  s += '\r'; i++; break;
            case '\n':
            case 'n':  s += '\n'; i++; break;
            case '\t':
            case 't':  s += '\t'; i++; break;
            case 'b':  s += '\b'; i++; break;
            case '\\': s += '\\'; i++; break;
            case 'x':  s += HexToText(src.Mid(i + 2, 2)); i += 3; break;
            default:   s += '\\';
            }
        }
        //else if (c >= 256)  //! these don't belong in Unescape()!
        //    s += wxString::Format("\\U%04X", c);
        //else if (!isprint(c))
        //    s += wxString::Format("\\x%02X", c);
        else
            s += c;
    }
    return s;
}

wxString Escape(wxString src)
{
    wxString s;
    const size_t len = src.Len();
    s.Alloc(src.Len());
    for (size_t i = 0; i < len; i++)
    {
        unsigned int c = (unsigned)(uint8)src[i];
        switch (c)
        {
        case '\r': s += _T("\\r"); break;
        case '\n': s += _T("\\n"); break;
        case '\t': s += _T("\\t"); break;
        case '\b': s += _T("\\b"); break;
        case '\\': s += _T("\\\\"); break;
        default:
            if (c < 256 && isprint(c))
                s += (wxChar)c;
            else if (c < 256)
                s += wxString::Format(_T("\\x%02X"), c);
            else
                s += wxString::Format(_T("\\U%04X"), c);
        }
    }
    return s;
}

size_t FormatNumber(uint64 n, TCHAR *buffer, size_t bufferSize, int base, size_t digits /*= 0*/)
{
    char sep;
    int group_digits, block;
    switch (base)
    {
    case 16: group_digits = 4; block = 0x10000; sep = ' '; break;
    case 10: group_digits = 3; block = 1000;    sep = ','; break;
    //case  8: // Do we need octal?
    case  2: group_digits = 8; block = 0x100;   sep = ' '; break;
    default: return 0;
    }

    if (digits == 0)
    {
        digits = 1;
        uint64 tmp = n;
        while (tmp /= base)
            digits++;
    }

    int groups = DivideRoundUp(digits, group_digits);
    size_t len = groups + digits - 1, end = len;
    int groupsize = group_digits;

    if (buffer == NULL)
        return len;

    if (len >= bufferSize)
        return 0;
   
    for (int g = groups - 1; g >= 0; g--)
    {
        if (g == 0 && digits % group_digits)
            end = groupsize = digits % group_digits;
        my_itoa((uint32)(n % block), buffer + end - groupsize, base, groupsize);
        if (g > 0)
            buffer[end - groupsize - 1] = sep;
        n /= block;
        end -= groupsize + 1;
    }
    buffer[len] = 0;
    return len;
}

wxString FormatDec(int64 n)
{
    TCHAR s[99];
    if (n >= 0)
        return wxString(s, FormatNumber(n, s, 99, 10));
    s[0] = '-';
    return wxString(s+1, FormatNumber((uint64)-n, s, 98, 10));
}

//wxString FormatHumanReadable(uint64 n, TCHAR &prefix, int decDigits /*= 2*/)
//{
//    if (n < 1024) { prefix = ' '; return FormatDec(n); }
//    if ((n >> 10) < 1024) { prefix = 'k'; return FormatDouble(n / 1024.0, decDigits); }
//    if ((n >> 20) < 1024) { prefix = 'M'; return FormatDouble((n >> 10) / 1024.0, decDigits); }
//    if ((n >> 30) < 1024) { prefix = 'G'; return FormatDouble((n >> 20) / 1024.0, decDigits); }
//    if ((n >> 40) < 1024) { prefix = 'T'; return FormatDouble((n >> 30) / 1024.0, decDigits); }
//    if ((n >> 50) < 1024) { prefix = 'P'; return FormatDouble((n >> 40) / 1024.0, decDigits); }
//    prefix = 'E';
//    return FormatDouble((n >> 50) / 1024.0, decDigits);
//}

//wxString FormatBytes(uint64 n, uint64 no_prefix_cutoff /*= 0*/, int decDigits /*= 2*/)
//{
//    if (n == 1)
//        return _T("1 byte");
//    if (n < no_prefix_cutoff)
//        return FormatDec(n) + _T(" bytes");
//    TCHAR label[4] = _T(" xB");
//    wxString num = FormatHumanReadable(n, label[1], decDigits);
//    if (label[1] == ' ')
//        return num + _T(" bytes");
//    return num + label;
//}

//! TODO: use human.c from GNU coreutils
int FormatDouble_(TCHAR *buf, int bufsize, double n, int significant /*= 3*/)
{
    int wholeDigits = (int)log10(n);
    wholeDigits = 1 + wxMax(wholeDigits, 0);
    if (wholeDigits + significant >= bufsize)
        return -1;
        
    int decimalPlaces = wxMax(significant - wholeDigits, 0);
    return _sntprintf(buf, bufsize, _T("%.*f"), decimalPlaces, n);
}

const TCHAR *FormatBytes(uint64 n, uint64 no_prefix_cutoff /*= 0*/)
{
    //! Not thread safe.
    static TCHAR s[100];
    if (n == 1)
        return _T("1 byte");
    if (n < 1024) {
        _tcscpy(s + FormatNumber(n, s, 90, 10), _T(" bytes"));
        return s;
    }
    TCHAR prefixes[] = {'k', 'M', 'G', 'T', 'P', 'E'};
    TCHAR label[4] = _T(" xB");
    for (int i = 0; i < 6; i++)
    {
        if ((n >> (10 * i + 10)) >= 1024)
            continue;
        label[1] = prefixes[i];
        _tcscpy(s + FormatDouble_(s, 90, (n >> (10 * i)) / 1024.0, 3), label);
        return s;
    }
    return _T("#Error"); // should never happen
}

wxString FormatDouble(double n, int decimalPlaces /*= 0*/)
{
    if (decimalPlaces)
        return wxString::Format(_T("%.*f"), decimalPlaces, n);
    TCHAR s[1000];
    int len = _tprintf(s, _T("%f"), n);
    while (s[len - 1] == '0')
        len--;
    if (s[len - 1] == '.')
        len--;
    return wxString(s, len);
}

wxString FormatWithBase(THSIZE n, int base)
{
    if (base == BASE_DEC)
        return FormatDec(n) + _T("d");
    else if (base == BASE_HEX)
        return FormatHex(n) + _T("h");
    else if (base == BASE_BOTH)
        return FormatDec(n) + _T("d=") + FormatHex(n) + _T("h");
    else if (base == BASE_OCT)
        return wxString::Format(_T("%I64oo"), n);
    else if (base == BASE_ALL)
        return FormatDec(n) + _T("d=") + FormatHex(n) + _T("h=") + FormatBin(n) + _T("b");
    else
        return wxEmptyString;
}

wxString FormatColour(const wxColour &clr)
{
    return wxString::Format(_T("%d %d %d"), clr.Red(), clr.Green(), clr.Blue());
}

void reverse(uint8 *data, uint32 len)
{
    for (uint32 i = 0; i < len / 2; i++)
        th_swap(data[i], data[len - i - 1]);
}


/* ibm2ieee - Converts a number from IBM 370 single precision floating
   point format to IEEE 754 single precision format.  For normalized
   numbers, the IBM format has greater range but less precision than the
   IEEE format.  Numbers within the overlapping range are converted
   exactly.  Numbers which are too large are converted to IEEE Infinity
   with the correct sign.  Numbers which are too small are converted to
   IEEE denormalized numbers with a potential loss of precision (including
   complete loss of precision which results in zero with the correct
   sign).  When precision is lost, rounding is toward zero (because it's
   fast and easy -- if someone really wants round to nearest it shouldn't
   be TOO difficult). */

#include <sys/types.h>
//#include <netinet/in.h>

void ibm2ieee(void *to, const void *from, int len)
{
   register unsigned fr;   /* fraction */
   register int exp;       /* exponent */
   register int sgn;       /* sign */

   for (; len-- > 0; to = (char *)to + 4, from = (char *)from + 4) {
      /* split into sign, exponent, and fraction */
      fr = my_ntohl(*(long *)from); /* pick up value */
      sgn = fr >> 31;         /* save sign */
      fr <<= 1;               /* shift sign out */
      exp = fr >> 25;         /* save exponent */
      fr <<= 7;               /* shift exponent out */

      if (fr == 0) { /* short-circuit for zero */
         exp = 0;
         goto done;
      }

      /* adjust exponent from base 16 offset 64 radix point before first digit
         to base 2 offset 127 radix point after first digit */
      /* (exp - 64) * 4 + 127 - 1 == exp * 4 - 256 + 126 == (exp << 2) - 130 */
      exp = (exp << 2) - 130;

      /* (re)normalize */
      while (fr < 0x80000000) {  /* 3 times max for normalized input */
         --exp;
         fr <<= 1;
      }

      if (exp <= 0) {   /* underflow */
         if (exp < -24) /* complete underflow - return properly signed zero */
            fr = 0;
         else           /* partial underflow - return denormalized number */
            fr >>= -exp;
         exp = 0;
      } else if (exp >= 255) {   /* overflow - return infinity */
         fr = 0;
         exp = 255;
      } else { /* just a plain old number - remove the assumed high bit */
         fr <<= 1;
      }

done:
      /* put the pieces back together and return it */
      *(unsigned *)to = (fr >> 9) | (exp << 23) | (sgn << 31);
   }

}

/* ieee2ibm - Converts a number from IEEE 754 single precision floating
   point format to IBM 370 single precision format.  For normalized
   numbers, the IBM format has greater range but less precision than the
   IEEE format.  IEEE Infinity is mapped to the largest representable
   IBM 370 number.  When precision is lost, rounding is toward zero
   (because it's fast and easy -- if someone really wants round to nearest
   it shouldn't be TOO difficult). */

void ieee2ibm(void *to, const void *from, int len)
{
   register unsigned fr;   /* fraction */
   register int exp;       /* exponent */
   register int sgn;       /* sign */

   for (; len-- > 0; to = (char *)to + 4, from = (char *)from + 4) {
      /* split into sign, exponent, and fraction */
      fr = *(unsigned *)from; /* pick up value */
      sgn = fr >> 31;         /* save sign */
      fr <<= 1;               /* shift sign out */
      exp = fr >> 24;         /* save exponent */
      fr <<= 8;               /* shift exponent out */

      if (exp == 255) {    /* infinity (or NAN) - map to largest */
         fr = 0xffffff00;
         exp = 0x7f;
         goto done;
      }
      else if (exp > 0)    /* add assumed digit */
         fr = (fr >> 1) | 0x80000000;
      else if (fr == 0)    /* short-circuit for zero */
         goto done;

      /* adjust exponent from base 2 offset 127 radix point after first digit
         to base 16 offset 64 radix point before first digit */
      exp += 130;
      fr >>= -exp & 3;
      exp = (exp + 3) >> 2;

      /* (re)normalize */
      while (fr < 0x10000000) {  /* never executed for normalized input */
         --exp;
         fr <<= 4;
      }

done:
      /* put the pieces back together and return it */
      fr = (fr >> 8) | (exp << 24) | (sgn << 31);
      *(unsigned *)to = my_htonl(fr);
   }

}

/* Test harness for IEEE systems */
#ifdef TEST
#define MAX     1000000         /* number of iterations */
#define IBM_EPS 4.7683738e-7    /* worst case error */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double epsm;

void check(float f1)
{
   int exp;
   float f2;
   double eps;
   unsigned ibm1, ibm2;

   frexp(f1, &exp);
   ieee2ibm(&ibm1, &f1, 1);
   ibm2ieee(&f2, &ibm1, 1);
   ieee2ibm(&ibm2, &f2, 1);
   if (memcmp(&ibm1, &ibm2, sizeof ibm1) != 0)
      printf("Error: %08x <=> %08x\n", *(unsigned *)&ibm1, *(unsigned *)&ibm2);
   eps = ldexp(fabs(f1 - f2), -exp);
   if (eps > epsm) epsm = eps;
   if (eps > IBM_EPS)
      printf("Error: %.8g != %.8g\n", f1, f2);

}

int main()
{
   int i;
   float f1;

   epsm = 0.0;
   for (i = 0; i < MAX; i++) {
      f1 = drand48();
      check(f1);
      check(-f1);
   }
   printf("Max eps: %g\n", epsm);
   return 0;

}

#endif 

// from majkinetor at http://forum.sysinternals.com/forum_posts.asp?TID=7974
bool EnableDebugPriv( void ) 
{
     HANDLE hToken;
     LUID sedebugnameValue;
     TOKEN_PRIVILEGES tkp;

     // enable the SeDebugPrivilege
     if ( ! OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
     {
            //_tprintf( _T("OpenProcessToken() failed, Error = %d SeDebugPrivilege is not available.\n") , GetLastError() );
            return false;
     }

     if ( ! LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &sedebugnameValue ) )
     {
            //_tprintf( _T("LookupPrivilegeValue() failed, Error = %d SeDebugPrivilege is not available.\n"), GetLastError() );
            CloseHandle( hToken );
            return false;
     }


     tkp.PrivilegeCount = 1;
     tkp.Privileges[0].Luid = sedebugnameValue;
     tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

     if ( ! AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof tkp, NULL, NULL ) )
            return false; //_tprintf( _T("AdjustTokenPrivileges() failed, Error = %d SeDebugPrivilege is not available.\n"), GetLastError() );
            
     CloseHandle( hToken );
     return true;
}


/*
    lpDevicePath - should be something like "\Device\HarddiskVolume1",
        "\Device\Floppy0" or "\Device\CdRom0"
    Thanks to Nish -- http://blog.voidnish.com/?p=72
    AEB - check out ZwQuerySymbolicLinkObject() at
        http://www.osronline.com/ddkx/kmarch/k111_2rle.htm
    
    About QueryDosDevice():
      // Now if this is an AFS network drive mapping, {szMapping} will be:
      //
      //   \Device\LanmanRedirector\<Drive>:\<netbiosname>\submount
      //
      // on Windows NT. On Windows 2000, it will be:
      //
      //   \Device\LanmanRedirector\;<Drive>:0\<netbiosname>\submount
      //
      // (This is presumably to support multiple drive mappings with
      // Terminal Server).
      //
      // on Windows XP and 2003, it will be :
      //   \Device\LanmanRedirector\;<Drive>:<AuthID>\<netbiosname>\submount
      //
      //   where : <Drive> : DOS drive letter
      //           <AuthID>: Authentication ID, 16 char hex.
      //           <netbiosname>: Netbios name of server
      //
http://web.mit.edu/AFS/sipb/project/openafs/cvs-checkout/openafs/src/WINNT/client_config/drivemap.cpp
    However, for network paths GetMappedFileName() gives something like
       \Device\LanmanRedirector\<netbiosname>\submount\path_to_file
    so we strip out the <Drive> part from szMapping and try matching the new string.
*/

TCHAR GetDriveLetter(LPCTSTR lpDevicePath, LPCTSTR *rest)
{
    wxRegEx re(_T("\\\\Device\\\\LanmanRedirector\\\\;?[A-Z]:[^\\]*\\\\(.*)"),
               wxRE_EXTENDED | wxRE_ICASE);
    for (TCHAR d = _T('A'); d <= _T('Z'); d++)
    {
        TCHAR szDeviceName[3] = {d, _T(':'), _T('\0')};
        TCHAR szMapping[512] = {0};
        if(QueryDosDevice(szDeviceName, szMapping, 511) != 0)
        {
            size_t len = _tcslen(szMapping);
            if (!_tcsncmp(lpDevicePath, szMapping, len))
            {
                *rest = lpDevicePath + len;
                return d;
            }
            if (re.Matches(szMapping))
            {
                wxString netpath = _T("\\Device\\LanmanRedirector\\") + re.GetMatch(szMapping, 1);
                if (!_tcsncmp(lpDevicePath, netpath, netpath.Len()))
                {
                    *rest = lpDevicePath + netpath.Len();
                    return d;
                }
            }
        }
    }
    return 0;
}

wxString GetRealFilePath(LPCTSTR path1, HANDLE hProcess, void *base)
{
    if (!wxFile::Exists(path1))
    {
        TCHAR devicepath[MAX_PATH];
        DWORD cBytes = GetMappedFileName(hProcess, base, devicepath, MAX_PATH);
        if (cBytes)
        {
            LPCTSTR rest;
            TCHAR driveLetter = GetDriveLetter(devicepath, &rest);
            if (driveLetter)
                return wxString::Format(_T("%c:%s"), driveLetter, rest);
        }
    }
    return path1;
}

// http://www-db.stanford.edu/~manku/bitcount/bitcount.html
//int bitcount(unsigned int n)
int bitcount(uint64 n)
{  
    int count = 0;
    while (n)
    {
        count++;
        n &= (n - 1);
    }
    return count;
}

//wxPoint MoveInside(const wxRect &rc, const wxPoint &pt, int tolX /*= 0*/, int tolY /*= 0*/)
//{
//    wxPoint pt2 = pt;
//    wxRect rc2 = rc;
//    rc2.Inflate(tolX, tolY);
//    if (pt.x < rc.x && pt.x >= rc2.x)
//        pt2.x = rc.x;
//    else if (pt.x > rc.GetRight() && pt.x <= rc2.GetRight())
//        pt2.x = rc.GetRight();
//
//    if (pt.y < rc.y && pt.y >= rc2.y)
//        pt2.y = rc.y;
//    else if (pt.y > rc.GetBottom() && pt.y <= rc2.GetBottom())
//        pt2.y = rc.GetBottom();
//
//    return pt2;
//}

wxString GetFileDescription(LPCTSTR szExeFile)
{
    DWORD infoSize, zero;
    infoSize = GetFileVersionInfoSize(szExeFile, &zero);
    if (!infoSize)
        //return wxString::Format("Error %d", GetLastError());
        return wxEmptyString;
    void *info = malloc(infoSize);
    GetFileVersionInfo(szExeFile, 0, infoSize, info);

    // Structure used to store enumerated languages and code pages.
    struct LANGANDCODEPAGE {
      WORD wLanguage;
      WORD wCodePage;
    } *lpTranslate;
    UINT cbTranslate = 0;

    // Read the list of languages and code pages.
    VerQueryValue(info,
                  TEXT("\\VarFileInfo\\Translation"),
                  (LPVOID*)&lpTranslate,
                  &cbTranslate);
    wxString descr;

    // Read the file description for the first language and code page.
    if (cbTranslate / sizeof(struct LANGANDCODEPAGE))
    {
        wxString s;
        s.Printf(_T("\\StringFileInfo\\%04x%04x\\FileDescription"),
            lpTranslate[0].wLanguage,
            lpTranslate[0].wCodePage);

        // Retrieve file description for language and code page "i".
        void *lpBuffer;
        UINT dwBytes;
        if (VerQueryValue(info, (LPCTSTR)s.c_str(), &lpBuffer, &dwBytes))
            descr = wxString((LPTSTR)lpBuffer);
    }

    free(info);
    return descr;
}

LRESULT EnableSeDebugPrivilege()
{
 /////////////////////////////////////////////////////////
   //   Note: Enabling SeDebugPrivilege adapted from sample
   //     MSDN @ http://msdn.microsoft.com/en-us/library/aa446619%28VS.85%29.aspx
   // Enable SeDebugPrivilege
   HANDLE hToken = NULL;
   TOKEN_PRIVILEGES tokenPriv;
   LUID luidDebug;
   LRESULT err = 0;
   bool success = false;
   if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken) != FALSE) 
   {
      if(LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidDebug) != FALSE)
      {
         tokenPriv.PrivilegeCount           = 1;
         tokenPriv.Privileges[0].Luid       = luidDebug;
         tokenPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
         if(AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, 0, NULL, NULL) != FALSE)
         {
            success = true;
         }
      }
   }
   if (!success)
      err = GetLastError();
   CloseHandle(hToken);
   // Enable SeDebugPrivilege
   /////////////////////////////////////////////////////////
   return err;
}

wxString GetProcessPath(const PROCESSENTRY32 &proc)
{
    TCHAR path[MAX_PATH];
    HMODULE hMods[1024];
    DWORD cbNeeded;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, proc.th32ProcessID);
    if (!hProc)
    {
        printf("OpenProcess error %d ", GetLastError());
        return wxEmptyString;
    }

    ON_BLOCK_EXIT(CloseHandle, hProc);  // TODO
    if (GetModuleFileNameEx(hProc, NULL, path, MAX_PATH))
    {
        // Works with 32-bit processes.
        if (!_tcsnicmp(path, _T("\\??\\"), 4)) // temporary hack
            return wxString(path + 4);
        else
            return wxString(path);
    }

    //! This doesn't work for some system procs (smss, csrss, winlogon, maybe svchost).
    // How do we get the real file path?  How does Process Explorer do it?
    // Task Manager doesn't even try.
    // GetCommandLine()
    // Dbghelp.dll (EnumerateLoadedModules64)
    // Toolhelp32 snapshot, OpenProcess, GetMappedFileName

    //  Works, but requires Vista.
    cbNeeded = MAX_PATH;
    if (QueryFullProcessImageName(hProc, 1, path, &cbNeeded))
    {
        // Returns a path like QueryDosDevice(), \Device\HarddiskVolume*
        return path;
    }

    // Doesn't work yet.
    //HANDLE hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, proc.th32ProcessID );
    //if( hModuleSnap != INVALID_HANDLE_VALUE )
    //{
    //    MODULEENTRY32 me = { sizeof(me) };
    //    Module32First(hModuleSnap, &me);
    //    CloseHandle(hModuleSnap);
    //    if (me.szExePath[0])
    //        return wxString(me.szExePath);
    //}

    // Seems to work about as well as GetModuleFileName(hProc, NULL...)
    // Get a list of all the modules in this process.
    //if (!EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded))
    //    return wxEmptyString;

    //for (size_t iMod = 0; iMod < cbNeeded / sizeof(HMODULE); iMod++)
    //{
    //    if (GetModuleFileNameEx(hProc, hMods[iMod], path, MAX_PATH))
    //    {
    //        PRINTF(_T("  %s\n"), path);
    //        wxFileName fn(path);
    //        if (fn.GetFullName().IsSameAs(proc.szExeFile, false))
    //            return path;
    //    }
    //}

    //if (!_tcsnicmp(path, _T("\\??\\"), 4)) // temporary hack
    //    fullPaths.Add(path + 4);
    //else
    //    fullPaths.Add(path);

    return wxEmptyString;
}

void ProcList::Init(HANDLE hSnapshot /*= NULL*/)
{
    if (hSnapshot == NULL)
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        fatal(_T("CreateToolhelp32Snapshot() failed."));

    PROCESSENTRY32 proc = { sizeof(proc) };
    if (!Process32First(hSnapshot, &proc))
        fatal(_T("Process32First() failed."));

    //LRESULT err = EnableSeDebugPrivilege();
    //if (err)
    //    wxMessageBox(wxString::Format("EnableSeDebugPrivilege() failed.  Win32 error %d", err));

    do {
        wxString spath, exeFile = proc.szExeFile;
        procNames.Add(exeFile);
        pids.Add(proc.th32ProcessID);

        spath = GetProcessPath(proc);

        fullPaths.Add(spath);
        if (spath.length())
            descr.Add(GetFileDescription(spath));
        else
            descr.Add(wxEmptyString);

    } while (Process32Next(hSnapshot, &proc));
    CloseHandle(hSnapshot);

    windowTitle.Add(wxEmptyString, GetCount()); // insert placeholders
    //hIcons.Add(0, GetCount());
    imageListIconIndex.Add(-1, GetCount());
    EnumWindows(EnumWindowsProc, (LPARAM)this); // get window titles
}

BOOL CALLBACK ProcList::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    ((ProcList*)lParam)->EnumWindowsProc(hWnd);
    return true;
}

void ProcList::EnumWindowsProc(HWND hWnd)
{
    if (!IsWindowVisible(hWnd))
        return;
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    int index = Index(pid);
    if (index == wxNOT_FOUND)
        return;
    if (windowTitle[index].Len())
        return;
    int len = GetWindowTextLength(hWnd);
    if (!len)
        return;
    wxString title;
    GetWindowText(hWnd, wxStringBuffer(title, len+1), len + 1);
    windowTitle[index] = title;

    //LPTSTR classname[256];
    //if (GetClassName(hWnd, classname, 256))
    //{
    //    WNDCLASSEX wcex = { sizeof(wcex) };
    //    if (GetClassInfoEx(hinst, classname, &wcex)) // Needs thread injection.  Ick.
    //        hIcons[index] = (int)wcex.hSmallIcon;
    //}
    //hIcons[index] = GetClassLong(hWnd, GCL_HICONSM);
}

wxString ProcList::NameFromPID(DWORD pid)
{
    int index = pids.Index(pid);
    if (index == wxNOT_FOUND)
        return wxEmptyString;
    return procNames[index];
}

DWORD ProcList::PIDFromName(wxString name)
{
    for (size_t n = 0; n < procNames.GetCount(); n++)
    {
        //PRINTF("  %6d %s\n", pids[n], procNames[n].c_str());
        if (procNames[n].CmpNoCase(name) == 0)
            return pids[n];
    }
    return 0; //! Didn't find a matching name.
}

wxImageList* ProcList::MakeImageList()
{
    wxImageList *list = new wxImageList(16, 16, true, GetCount());
    int index = 0;
    SHFILEINFO shfi;

    thStringHash iconCache;  // cache icon index to avoid repeating queries
    ATimer timer;

    for (size_t n = 0; n < GetCount(); n++)
    {
        wxString path = fullPaths[n];
        PRINTF(_T("\n%2d: %s"), n, fullPaths[n].Len() ? (LPCTSTR)fullPaths[n] : (LPCTSTR)procNames[n]);
        //PRINTF(_T("\n%2d: %s"), n, (LPCTSTR)procNames[n]);
        timer.StartWatch();
        if (path.length() && iconCache.has_key(path))
        {
            int ii = iconCache[path];
            imageListIconIndex[n] = ii;
            PRINTF(_T(" (copy #%d)"), ii);
        }
        else if (path.Len() &&
            SHGetFileInfo(fullPaths[n], 0, &shfi, sizeof(shfi),
                          SHGFI_ICON | SHGFI_SMALLICON))
        {
           wxIcon icon;
           icon.SetHICON(shfi.hIcon);
           list->Add(icon);
           PRINTF(_T(" file #%d %dx%d, %dms"), index, icon.GetWidth(), icon.GetHeight(), int(timer.elapsed() * 1000));
           iconCache[path] = index;
           imageListIconIndex[n] = index++;
           // Our app _should_ delete this icon.
        }
        //else if (hIcons[n])
        //{
        //    wxIcon icon;
        //    HICON hIcon = (HICON)hIcons[n];
        //    //ICONINFO ii;
        //    //GetIconInfo(hIcon, &ii);
        //    //BITMAP bmp;
        //    //GetObject(ii.hbmColor, sizeof(bmp), &bmp);
        //    icon.SetHICON(hIcon);
        //    list->Add(icon);
        //    PRINTF(" win #%d %dx%d", index, icon.GetWidth(), icon.GetHeight());
        //    imageListIconIndex[n] = index++;
        //    // Our app _should not_ delete this icon.  Keep a local copy.
        //    //m_imageList->Add(icon);
        //}
    }
    PRINTF(_T("\n"));
    return list;
}

//*****************************************************************************

/* Unescape a backslash-escaped string. If unicode is non-zero,
   the string is a u-literal. If recode_encoding is non-zero,
   the string is UTF-8 encoded and should be re-encoded in the
   specified encoding.  */

#define Py_CHARMASK

wxString PyString_DecodeEscape(const char *s,
                               Py_ssize_t len,
                               const char *errors,
                               Py_ssize_t unicode,
                               const char *recode_encoding)
{
        int c;
        TCHAR *p, *buf;
        const char *end;
        wxString v;
        Py_ssize_t newlen = recode_encoding ? 4*len:len;
        wxStringBuffer wxbuf(v, newlen);
        p = buf = wxbuf;
        const int incr = (unicode ? 2 : 1);
        end = s + len;
        while (s < end) {
                if (*s != '\\') {
                  non_esc:
//#ifdef Py_USING_UNICODE
                        //if (recode_encoding && (*s & 0x80)) {
                        //        PyObject *u, *w;
                        //        char *r;
                        //        const char* t;
                        //        Py_ssize_t rn;
                        //        t = s;
                        //        /* Decode non-ASCII bytes as UTF-8. */
                        //        while (t < end && (*t & 0x80)) t++;
                        //        u = PyUnicode_DecodeUTF8(s, t - s, errors);
                        //        if(!u) goto failed;

                        //        /* Recode them in target encoding. */
                        //        w = PyUnicode_AsEncodedString(
                        //                u, recode_encoding, errors);
                        //        Py_DECREF(u);
                        //        if (!w)        goto failed;

                        //        /* Append bytes to output buffer. */
                        //        assert(PyString_Check(w));
                        //        r = PyString_AS_STRING(w);
                        //        rn = PyString_GET_SIZE(w);
                        //        Py_MEMCPY(p, r, rn);
                        //        p += rn;
                        //        Py_DECREF(w);
                        //        s = t;
                        //} else {
                        //        *p++ = *s++;
                        //}
//#else
                        *p++ = *s;
                        s += incr;
//#endif
                        continue;
                }
                s += incr;
                if (s==end) {
                        return _T("Trailing \\ in string");
                }
                switch (*s) {
                /* XXX This assumes ASCII! */
                case '\n': break;
                case '\\': *p++ = '\\'; break;
                case '\'': *p++ = '\''; break;
                case '\"': *p++ = '\"'; break;
                case 'b': *p++ = '\b'; break;
                case 'f': *p++ = '\014'; break; /* FF */
                case 't': *p++ = '\t'; break;
                case 'n': *p++ = '\n'; break;
                case 'r': *p++ = '\r'; break;
                case 'v': *p++ = '\013'; break; /* VT */
                case 'a': *p++ = '\007'; break; /* BEL, not classic C */
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                        c = *s - '0';
                        s += incr;
                        if ('0' <= *s && *s <= '7') {
                                c = (c<<3) + *s - '0';
                                s += incr;
                                if ('0' <= *s && *s <= '7')
                                {
                                        c = (c<<3) + *s - '0';
                                        s += incr;
                                }
                        }
                        s -= incr; // gets added again later
                        *p++ = c;
                        break;
                case 'x':
                        if (isxdigit(Py_CHARMASK(s[0]))
                            && isxdigit(Py_CHARMASK(s[incr]))) {
                                unsigned int x = 0;
                                c = Py_CHARMASK(*s);
                                s += incr;
                                if (isdigit(c))
                                        x = c - '0';
                                else if (islower(c))
                                        x = 10 + c - 'a';
                                else
                                        x = 10 + c - 'A';
                                x = x << 4;
                                c = Py_CHARMASK(*s);
                                s++;
                                if (isdigit(c))
                                        x += c - '0';
                                else if (islower(c))
                                        x += 10 + c - 'a';
                                else
                                        x += 10 + c - 'A';
                                *p++ = x;
                                break;
                        }
                        if (!errors || strcmp(errors, "strict") == 0) {
                                return _T("invalid \\x escape");
                        }
                        if (strcmp(errors, "replace") == 0) {
                                *p++ = '?';
                        } else if (strcmp(errors, "ignore") == 0)
                                /* do nothing */;
                        else {
                                return wxString::Format(
                                             _T("decoding error; ")
                                             _T("unknown error handling code: %.400s"),
                                             errors);
                        }
//#ifndef Py_USING_UNICODE
                case 'u':
                case 'U':
                case 'N':
                        if (unicode) {
                                return _T("Unicode escapes not legal ")
                                       _T("when Unicode disabled");
                        }
//#endif
                default:
                        *p++ = '\\';
                        s -= incr;
                        goto non_esc; /* an arbitry number of unescaped
                                         UTF-8 bytes may follow. */
                }
                s += incr;
        }
        //if (p-buf < newlen)
        //        _PyString_Resize(&v, p - buf);
        return v;
  //failed:
  //      return wxEmptyString;
}

wxString Plural(THSIZE n, LPCTSTR s1 /*= NULL*/, LPCTSTR s2 /*= NULL*/)
{
    if (n == 1)
        return s1;
    if (s2)
        return s2;
    if (s1)
        return wxString(s1) + _T("s");
    return _T("s");
}

int ClearWxListSelections(wxListCtrl *list)
{
    if (!list)
        return -1;
    int count = 0, item = -1;
    while (1)
    {
        item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1)
            break;
        list->SetItemState(item, 0, wxLIST_STATE_SELECTED);
        count++;
    }
    return count;
}

int AutoSizeVirtualWxListColumn(wxListCtrl *list, int col, wxString longestData)
{
    list->SetColumnWidth(col, wxLIST_AUTOSIZE_USEHEADER);
    wxListItem li;
    li.SetMask(wxLIST_MASK_TEXT);
    list->GetColumn(col, li);
    wxClientDC dc(list);
    int hx, hy, dx, dy, width = list->GetColumnWidth(col);
    dc.SetFont(list->GetFont());
    dc.GetTextExtent(li.GetText(), &hx, &hy);
    dc.GetTextExtent(longestData, &dx, &dy);
    if (dx > hx)
        list->SetColumnWidth(col, width = dx + (width - hx));
    return width;
}

//*****************************************************************************
//*****************************************************************************
// thString
//*****************************************************************************
//*****************************************************************************

thString::thString(const uint8* data, size_t bytes)
{
    m_str = wxString((TCHAR*)data, DivideRoundUp(bytes, sizeof(TCHAR)));
    m_len = bytes;
    //m_data = (uint8*)m_str.c_str();  //! WX29
}

thString thString::ToANSI(const char *data, size_t chars /*= 0*/)
{
    if (chars == 0)
        chars = strlen(data);
    return thString((const uint8*)data, chars);
}

thString thString::ToUnicode(const wchar_t *data, size_t chars /*= 0*/)
{
    if (chars == 0)
        chars = wcslen(data);
    return thString((const uint8*)data, chars * 2);
}


#ifdef _UNICODE
thString thString::ToANSI(const wchar_t *pData, size_t chars /*= 0*/)
{
    if (chars == 0)
        chars = wcslen(pData);
    wxString retval;
    size_t dstLen = wxConvLibc.FromWChar(NULL, 0, pData, chars);
    if (dstLen == wxCONV_FAILED)
        dstLen = 0;
    else
    {
        wxStringBuffer wxbuf(retval, dstLen);
        char *wbuf = (char*)(TCHAR*)wxbuf;
        wxConvLibc.FromWChar(wbuf, dstLen, pData, chars);
        if (wbuf[dstLen-1] == 0)
            dstLen--;  // remove trailing null, shouldn't happen.
    }
    return thString(retval, dstLen);
}

thString thString::ToUnicode(const char *data, size_t chars /*= 0*/)
{
    return wxString(data, wxConvLibc, chars ? chars : wxString::npos);
}

#else
thString thString::ToANSI(const wchar_t *data, size_t chars /*= 0*/)
{
    return wxString(data, wxConvLibc, chars ? chars : wxString::npos);
}

thString thString::ToUnicode(const char *data, size_t chars /*= 0*/)
{
    if (chars == 0)
        chars = strlen(data);
    wxString retval;
    size_t dstLen = wxConvLibc.ToWChar(NULL, 0, data, chars);
    if (dstLen != wxCONV_FAILED)
    {
        wchar_t *wbuf = (wchar_t*)retval.GetWriteBuf(dstLen * 2);
        wxConvLibc.ToWChar(wbuf, dstLen, data, chars);
        retval.UngetWriteBuf((dstLen - 1) * 2); // remove trailing null
    }
    return retval;
}
#endif // _UNICODE


//*************************************************************************************************
// GetFileNameFromHandle() -- handy function from MSDN documentation.

#define BUFSIZE 512

wxString GetFileNameFromHandle(HANDLE hFile)
{
  BOOL bSuccess = FALSE;
  TCHAR pszFilename[MAX_PATH+1];
  HANDLE hFileMap;

  // Get the file size.
  DWORD dwFileSizeHi = 0;
  DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

  if( dwFileSizeLo == 0 && dwFileSizeHi == 0 )
     return _T("length=0");

  // Create a file mapping object.
  hFileMap = CreateFileMapping(hFile, 
                    NULL, 
                    PAGE_READONLY,
                    0, 
                    MAX_PATH,
                    NULL);

  if (hFileMap) 
  {
    // Create a file mapping to get the file name.
    void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

    if (pMem) 
    {
      if (GetMappedFileName (GetCurrentProcess(), 
                             pMem, 
                             pszFilename,
                             MAX_PATH)) 
      {

        // Translate path with device name to drive letters.
        TCHAR szTemp[BUFSIZE];
        szTemp[0] = '\0';

        if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
        {
          TCHAR szName[MAX_PATH];
          TCHAR szDrive[3] = TEXT(" :");
          BOOL bFound = FALSE;
          TCHAR* p = szTemp;

          do 
          {
            // Copy the drive letter to the template string
            *szDrive = *p;

            // Look up each device name
            if (QueryDosDevice(szDrive, szName, BUFSIZE))
            {
              UINT uNameLen = _tcslen(szName);

              if (uNameLen < MAX_PATH) 
              {
                bFound = _tcsnicmp(pszFilename, szName, 
                    uNameLen) == 0;

                if (bFound) 
                {
                  // Reconstruct pszFilename using szTempFile
                  // Replace device path with DOS path
                  TCHAR szTempFile[MAX_PATH];
                  StringCchPrintf(szTempFile,
                            MAX_PATH,
                            TEXT("%s%s"),
                            szDrive,
                            pszFilename+uNameLen);
                  StringCchCopyN(pszFilename, MAX_PATH+1, szTempFile, _tcslen(szTempFile));
                }
              }
            }

            // Go to the next NULL character.
            while (*p++);
          } while (!bFound && *p); // end of string
        }
      }
      bSuccess = TRUE;
      UnmapViewOfFile(pMem);
    } 

    CloseHandle(hFileMap);
  }
  return pszFilename;
}

//*************************************************************************************************
// thProgressDialog
// Adds automatic speed monitoring to wxProgressDialog.
//*************************************************************************************************

thProgressDialog::thProgressDialog(THSIZE range, wxWindow *parent, wxString msg, wxString caption,
                                   DWORD updateInterval /*= 1000*/)
: wxProgressDialog(
        caption, msg + _T("\n "), 1000, parent,
        wxPD_APP_MODAL |
        wxPD_SMOOTH |
        wxPD_CAN_ABORT |
        wxPD_ELAPSED_TIME |
        wxPD_ESTIMATED_TIME |
        wxPD_REMAINING_TIME),
updateTimer(ATimer::TICKCOUNT)  // imprecise, but fast.
{
    m_range = range;
    //m_lastStart = 0;
    m_lastStart[0] = m_lastStart[1] = 0;
    //m_startTime = m_lastUpdateTime = GetTickCount();
    m_startTime = m_lastUpdateTime[0] = m_lastUpdateTime[1] = GetTickCount();
    m_msg = msg;
    SetUpdateInterval(updateInterval);
    SetSpeedScale(1.0);
}

bool thProgressDialog::Update(THSIZE value, bool showSpeed /*= true*/, wxString extraMsg /*= ZSTR*/)
{
    wxString progressMsg;
    if (updateTimer.IsTimeUp())
    {
        DWORD now = GetTickCount();
        progressMsg = m_msg;
        if (showSpeed || extraMsg.Len())
            progressMsg << _T("\n");
        if (showSpeed)
        {
            //! Really need a better moving average here.
            progressMsg << FormatBytes((value - m_lastStart[0]) * 1000.0 * m_speedScale / (now - m_lastUpdateTime[0])) << _T("/s");
            progressMsg << _T(", avg. ") << FormatBytes(value * 1000.0 * m_speedScale / (now - m_startTime)) << _T("/s");
            //progressMsg += wxString::Format(_T("\n= %s bytes in %0.3f seconds"), FormatBytes(value - m_lastStart), (now - m_lastUpdateTime) * .001);
            m_lastStart[0] = m_lastStart[1];
        }
        progressMsg << extraMsg;
        m_lastStart[0] = m_lastStart[1];
        m_lastUpdateTime[0] = m_lastUpdateTime[1];
        m_lastStart[1] = value;
        m_lastUpdateTime[1] = now;
        updateTimer.SetTimeout(m_updateInterval * 1000);
    }

    return wxProgressDialog::Update(value * 1000.0 / m_range, progressMsg);
}

void thProgressDialog::SetUpdateInterval(DWORD updateInterval)
{
    m_updateInterval = updateInterval;
    updateTimer.SetTimeout(updateInterval * 1000);
}

void thProgressDialog::SetSpeedScale(double scale)
{
    m_speedScale = scale;
}

bool Confirm(wxString msg, wxString caption /*= _T("T. Hex")*/, wxWindow *parent /*= NULL*/)
{
    return (wxMessageBox(msg, caption, wxYES_NO, parent) == wxYES);
}
