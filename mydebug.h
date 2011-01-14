#ifdef _MSC_VER
#ifdef _DEBUG
#ifdef _MEMDEBUG
   #define _CRTDBG_MAP_ALLOC
   #include <crtdbg.h>
   #define MYDEBUG_NEW   new( _NORMAL_BLOCK, __FILE__, __LINE__)
   #define new MYDEBUG_NEW
#endif // _MEMDEBUG
#endif // _DEBUG
#endif // _MSC_VER

