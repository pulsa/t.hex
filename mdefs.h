#define MCCLIBEXPORT

#ifndef MCCLIBEXPORT
   #ifdef _MSC_VER
      #ifdef MCCLIB_DLL_EXPORTS
         #define MCCLIBEXPORT __declspec(dllexport)
      #else
         #define MCCLIBEXPORT __declspec(dllimport)
      #endif
      #undef CreateDialog
      #include <wx/wx.h>
      #include <wx/defs.h>
      //#include <ocl_wx.hpp>
      #define IC_DEFAULT_FRAME_ID   1000 // from ICCONST.H
      #define Boolean bool
      #pragma warning(disable: 4800) // converting int to bool
   #else
      #define MCCLIBEXPORT _Export
   #endif
#endif

#pragma warning(disable: 4355)  // warning C4355: 'this' : used in base member initializer list

#if defined(__WXMSW__) && !wxCHECK_VERSION(2, 5, 0)

#ifndef WXLRESULT
#ifdef __WIN64__
typedef unsigned __int64    WXWPARAM;
typedef __int64            WXLPARAM;
typedef __int64            WXLRESULT;
#else
typedef unsigned int    WXWPARAM;
typedef long            WXLPARAM;
typedef long            WXLRESULT;
#endif
#endif

#define SetBestFittingSize(size) SetSizeHints(size.GetWidth(), size.GetHeight()) // Is this appropriate?

#ifndef DECLARE_EXPORTED_EVENT_TYPE
#define DECLARE_EXPORTED_EVENT_TYPE  DECLARE_EXPORTED_LOCAL_EVENT_TYPE
#endif

#endif
