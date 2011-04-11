#pragma once

#ifndef _DEFS_H_
#define _DEFS_H_

#ifndef int32
typedef int int32;
#endif

//#ifndef uint32
typedef unsigned int uint32;
//#endif

#ifndef int64
typedef wxLongLong_t int64;
#endif

#ifndef uint64
typedef wxULongLong_t uint64;
#endif

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef LITTLEENDIAN_MODE
#define LITTLEENDIAN_MODE   0
#define BIGENDIAN_MODE      1
#define NATIVE_ENDIAN_MODE  LITTLEENDIAN_MODE
#endif

#define MAX_ADDRESS 0x7FFFFFFFFFFFFFFF // ((1 << 63) - 1)
//#define THSIZE uint64
typedef uint64 THSIZE;

typedef enum {BASE_DEC, BASE_HEX, BASE_BOTH, BASE_OCT, BASE_ALL} THBASE;

const uint32 MEGA = 1024 * 1024;

#ifndef _WINDOWS
typedef wxChar TCHAR;
typedef const wxChar* LPCTSTR;
typedef void* HANDLE;
typedef uint32 HWND;
typedef uint32 DWORD;
typedef bool BOOL;
typedef unsigned int  UINT;
typedef unsigned long ULONG;
typedef uint64 UINT64;
typedef int64 INT64;
//typedef void* UINT_PTR;  //! not very good
typedef uint64 UINT_PTR;
typedef uint64 DWORD_PTR;
typedef DWORD COLORREF;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef long LONG;
typedef wchar_t WCHAR;

#define GetRValue(x) (unsigned char)(x)
#define GetGValue(x) (unsigned char)((x) >> 8)
#define GetBValue(x) (unsigned char)((x) >> 16)

typedef struct tagRGBQUAD { 
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved;
} RGBQUAD;

#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

typedef struct tagRECT {
    int left, top, right, bottom;
} RECT;

typedef struct _SYSTEMTIME {
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME;


typedef struct tagTEXTMETRICW
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    WCHAR       tmFirstChar;
    WCHAR       tmLastChar;
    WCHAR       tmDefaultChar;
    WCHAR       tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
} TEXTMETRICW, *PTEXTMETRICW;

typedef TEXTMETRICW TEXTMETRIC;
typedef PTEXTMETRICW PTEXTMETRIC;


#define PAGE_NOACCESS          0x01     
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     
#define PAGE_WRITECOPY         0x08     
#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80     
#define PAGE_GUARD            0x100     
#define PAGE_NOCACHE          0x200     
#define PAGE_WRITECOMBINE     0x400     


#define ETO_OPAQUE                   0x0002
#define ETO_CLIPPED                  0x0004
#define ETO_GLYPH_INDEX              0x0010
#define ETO_RTLREADING               0x0080
#define ETO_NUMERICSLOCAL            0x0400
#define ETO_NUMERICSLATIN            0x0800
#define ETO_IGNORELANGUAGE           0x1000
#define ETO_PDY                      0x2000
#define ETO_REVERSE_INDEX_MAP        0x10000

DWORD GetTickCount();

#endif


#endif // _DEFS_H_
