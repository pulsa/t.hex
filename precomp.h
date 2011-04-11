#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#define WIN32_LEAN_AND_MEAN       // Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE 1 // Today (2006-08-03) I have to define this.  Why?
//#define _HAS_EXCEPTIONS 0  // leave this out at work, where we use exceptions in WX
//#define _INC_TYPEINFO  // VS.NET 2003 typeinfo.h always uses exception class

#define _WIN32_WINNT 0x501 //! target Windows XP (dangerous!)
//#define _WIN32_WINNT 0x500  // Today I need to run on Win2k
//#define _WIN32_WINNT 0x0400   // or maybe Win98
#define _SCL_SECURE_NO_DEPRECATE

//#define WX_AEB_MOD 1  // Use Adam's modifications to the wx libraries.

#pragma warning (disable:4530) // C++ exception handler used, but unwind semantics are not enabled.
#pragma warning (disable:4995) // 'strcpy': name was marked as #pragma deprecated

#include <tchar.h>

#endif  // _MSC_VER

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/caret.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <wx/clipbrd.h>
#include <wx/tooltip.h>
#include <wx/progdlg.h>
#include <wx/fontdlg.h>
#include <wx/splitter.h>
#include <wx/ipc.h>
#include <wx/bmpcbox.h>
#include <wx/socket.h>
#include <wx/notebook.h>
#include <wx/dnd.h>
#include <wx/zipstrm.h>  // zip recovery test, 2008-09-23
#include <wx/wfstream.h> // same.
#include <wx/zstream.h>  // zlib compressability test, 2008-09-29

template <class T>
inline T fnmax(const T& a, const T& b)
{
    if (a < b) return b;
    return a;
}

#ifdef _MSC_VER
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <tlhelp32.h> 
#include <shlobj.h>
#include <psapi.h>

#include <Setupapi.h>
#include <winioctl.h>
//#include <cfgmgr32.h>
#include <conio.h>
#include <strsafe.h>
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

#include <map>
#include <vector>
#include <algorithm>
#include <set>

//#include "QString.h"
#include "defs.h"
#include "atimer.h"

#ifdef INCLUDE_LIBDISASM
extern "C" {
#ifdef LIBDISASM_OLD
#undef LIBDISASM_NEW
#include "libdisasm-old/libdis.h"
#else  // new libdisasm
#define LIBDISASM_NEW
#include "libdisasm/libdis.h"
#endif
}
#endif

#ifdef _DEBUG
#define PRINTF _tprintf
#else
#define PRINTF
#endif

#ifndef UNUSED
#define UNUSED(param)
#endif

#define DIM(a) (sizeof(a) / sizeof((a)[0]))
#define SGN(n) ((n) > 0 ? 1 : (n) < 0 ? -1 : 0)

#define minmax(n, low, high) ((n) < (low) ? (low) : (n) > (high) ? (high) : (n))
#define ZSTR wxEmptyString  // or possibly _T("") ?
#define DPOS wxDefaultPosition
#define DSIZE wxDefaultSize

#ifdef _DEBUG
#include <crtdbg.h>
// Define New for debug builds to get source file and line numbers on memory leaks.
//#define New new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define New new
#else
#define New new
#endif

#endif // _PRECOMP_H_
