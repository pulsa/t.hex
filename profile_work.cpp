/* Simple profiler */

/* Okay, so here's how this works.
Compile source modules to be profiles with "/Gh /GH".
Every time _any_ function gets called, _penter() gets called first.
This puts a new PROFREC on the profile stack, where
rec.address = our return address, beginning of new function
rec.functionTime = now, and
rec.childTime = 0.
That's all it does.  Then when _pexit() gets called, it looks at the top of
the profile stack, which is the function that is returning.  It figures out how
much time has passed since the function was called, and adds this to
stack[rcount].functionTime.  Then it saves this PROFREC to save[], updating or
adding a new record as necessary.  It also adds the function time to
stack[--rcount].childTime.

There's major room for improvement all over the place.
I'm surprised this works at all.  But it's good enough for now,
and no one else will ever see it anyway, right?
*/

//#include "precomp.h"
//#define PROFILE
#undef UNICODE
#undef _UNICODE
#ifdef PROFILE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#undef UNICODE
#undef _UNICODE
#include <tchar.h>

#include <psapi.h>
#pragma comment(lib, "psapi")
#pragma comment(lib, "dbghelp")

#define PRINTF printf

typedef UINT32 ADDR;

#pragma pack(push, 4)
typedef struct tagPROFREC {
   ADDR address;
   UINT64 functionTime;
   UINT64 childTime;
   int count;
} PROFREC;
#pragma pack(pop)

const int MAXSTACK = 512;
const int MAXRECORDS = 2048;
static PROFREC stack[MAXSTACK];  // records on the current function stack
static PROFREC save[MAXRECORDS]; // records for all functions that have been called
static int rcount = 0, scount = 0, max_rcount = 0;
static bool atexit_registered = 0;
static DWORD64 totalCycles = 0;

BOOL lfaInit(HANDLE hProc, HMODULE hModule, DWORD64 &ModBase);
bool LineFromAddr(HANDLE hProc, DWORD64 SymAddr,
                  TCHAR *name, size_t nameLen,
                  DWORD64 *pLine, DWORD64 *pDisp);
int lfaUnload(DWORD64 ModBase);

void ReadTimestampCounter(DWORD64 *now)
{
    _asm
    {
        rdtsc
        mov ebx, now
        mov [ebx], eax
        mov [ebx + 4], edx
    }
}

static DWORD64 startTime_tsc, cpu;
static LARGE_INTEGER startTime_hpc;
static UINT64 g_threshold = 1;

int cmpfunc(const void *p1, const void *p2)
{
    const PROFREC *r1 = (const PROFREC*)p1;
    const PROFREC *r2 = (const PROFREC*)p2;
    if (r1->functionTime > r2->functionTime) return 1;
    if (r1->functionTime < r2->functionTime) return -1;
    if (r1->childTime > r2->childTime) return 1;
    if (r1->childTime < r2->childTime) return -1;
    if (r1->count > r2->count) return 1;
    if (r1->count < r2->count) return -1;
    return 0;
    //if (r1->functionTime > r2->functionTime)
    //    return 1;
    //if (r1->functionTime == r2->functionTime)
    //    return 0;
    //return -1;
}


//time        total       avg
//----------- ------      ------
//0.00012345       -           -
//0.0012345        -       0.001
//0.012345         -       0.012
//0.12345        0.1       0.123
//1.2345         1.2       1.235
//12.345        12.3       12.35
//123.45       123.5       123.5
//1234.5      1234.5      1234.5
//12345         12.3*      12.34*
//123450       123.5*      123.5*
//1234500     1234.5*     1234.5*

void sprintfTotal(char *buf, double ms)
{
    if (ms < 0.1)
        buf[0] = 0;
    else if (ms < 10000.0)
        sprintf(buf, "%6.1f", ms);
    else
        sprintf(buf, "%6.1fs", ms * .001);
}

void sprintfAvg(char *buf, double ms)
{
    if (ms < .001)
        buf[0] = 0;
    else if (ms < 10.0)
        sprintf(buf, "%6.3f", ms);
    else if (ms < 100.0)
        sprintf(buf, "%6.2f", ms);
    else if (ms < 10000.0)
        sprintf(buf, "%6.1f", ms);
    else
        sprintf(buf, "%6.1fs", ms * .001);
}

#define WPO_GOODFIRST  1
#define WPO_GOODLAST   0
#define WPO_LONGFMT    2
#define WPO_SHORTFMT   0
#define WPO_PRUNETINY  4
#define WPO_KEEPTINY   0

void WriteProfileOutput(FILE *f, double runtime = 0.0, DWORD flags = 0)
{
    HANDLE hProc = GetCurrentProcess();
    TCHAR name[100];
    char str_self[100], str_child[100], str_calls[10], str_savg[50], str_cavg[50];
    fprintf(f, "Times are in milliseconds, unless noted.\n");
    char *headers[2];
    bool longfmt = (flags & WPO_LONGFMT) != 0;
    bool goodfirst = (flags & WPO_GOODFIRST) != 0;
    bool prune = (flags & WPO_PRUNETINY) != 0;
    int pruned = 0;
    if (longfmt) {
       headers[0] = "Self    S.Avg   Child   C.Avg   Calls   Name\n";
       headers[1] = "------  ------  ------  ------  ------  ----------------\n";
    }
    else {
       headers[0] = "Self    Child   Calls  Name\n";
       headers[1] = "------  ------  ------ ----------------\n";
    }
    if (goodfirst)
    {
        fprintf(f, headers[0]);
        fprintf(f, headers[1]);
    }
    int i;
    for (int j = 0; j < scount; j++)
    {
        if (goodfirst)
            i = scount - j - 1;
        else
            i = j;
        if (prune && !save[i].functionTime && !save[i].childTime)
        {
            pruned++;
            continue;
        }

        if (!LineFromAddr(hProc, (DWORD64)save[i].address, name, 100, NULL, NULL))
            _sntprintf(name, 100, _T("%08X"), save[i].address);

        double self = save[i].functionTime / 10.0;
        double child = save[i].childTime / 10.0;

        str_self[0] = str_child[0] = str_savg[0] = str_cavg[0] = 0;
        sprintfTotal(str_self, self);
        sprintfTotal(str_child, child);
        if (longfmt)
        {
            if (save[i].count < 1000000)
                sprintf(str_calls, "%6d", save[i].count);
            else
                sprintf(str_calls, "%6.3fM", save[i].count * .000001);
            if (self && save[i].count > 1)
                sprintfAvg(str_savg, self / save[i].count);
            if (child && save[i].count > 1)
                sprintfAvg(str_cavg, child / save[i].count);

            fprintf(f, "%-7s %-7s %-7s %-7s %-7s %s\n", str_self, str_savg, str_child, str_cavg, str_calls, name);
        }
        else
            fprintf(f, "%-7s %-7s %-6d %s\n", str_self, str_child, save[i].count, name);
    }
    
    fprintf(f, headers[1]);  // reverse headers as footers
    fprintf(f, headers[0]);
    if (goodfirst)
    {
        fprintf(f, "\n");
        //fprintf(f, "CPU frequency: %d MHz\n", (int)(cpu / 1000000));
        fprintf(f, "Collected %d profile records.\n", scount);
        if (pruned)
            fprintf(f, "Pruned %d zero-time functions, leaving %d.\n", pruned, scount - pruned);
        if (longfmt && max_rcount)
            fprintf(f, "Max stack depth: %d\n", max_rcount);
        if (totalCycles)
            fprintf(f, "Total function time: %0.3f seconds.\n", totalCycles / (double)cpu);
        if (runtime)
            fprintf(f, "Total run time after first profiled function call: %0.3f seconds.\n", runtime);
    }
    else if (pruned)
        fprintf(f, "Pruned %d zero-time functions.\n", pruned);
}

void PrintProfileData()
{
    // calculate CPU speed
    DWORD64 now_tsc;
    LARGE_INTEGER now_hpc, freq;
    ReadTimestampCounter(&now_tsc);
    QueryPerformanceCounter(&now_hpc);
    QueryPerformanceFrequency(&freq);
    double seconds = (now_hpc.QuadPart - startTime_hpc.QuadPart) / (double)freq.QuadPart;
    cpu = (DWORD64)((now_tsc - startTime_tsc) / seconds);

    DWORD64 ModBase;
    HANDLE hProc = GetCurrentProcess();
    if (!lfaInit(hProc, GetModuleHandle(NULL), ModBase))
    {
       printf("Couldn't load debug info.  You don't want to see a sea of addresses, so I quit.\n");
       return;
    }

    int negative = 0;

    scount = 0;
    for (int i = 0; i < MAXRECORDS; i++) // updated for hash table
    {
        if (save[i].count)
            save[scount++] = save[i];
    }
    for (int i = 0; i < scount; i++)
    {
        save[i].functionTime -= save[i].childTime;
        //save[i].functionTime -= 21 * save[i].count; //! overhead test
        if ((INT64)save[i].functionTime <= save[i].count)
           negative++;
        totalCycles += save[i].functionTime;

        save[i].functionTime = save[i].functionTime * 10000 / cpu;
        save[i].childTime = save[i].childTime * 10000 / cpu;
    }
    // now we could sort by address, name, time, or whatever.
    g_threshold = cpu;
    qsort(save, scount, sizeof(PROFREC), cmpfunc);

    // max_rcount was being tracked inside _pexit(), but let's do it here instead.
    for (max_rcount = 0; max_rcount < MAXSTACK; max_rcount++)
    {
        if (stack[max_rcount].address == 0)
            break;
    }

    bool HaveConsole = false;//(GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE);
    if (HaveConsole)
    {
        WriteProfileOutput(stdout, seconds, WPO_PRUNETINY);
        PRINTF("Negative: %d\n", negative);
        //PRINTF("Press 'F' to write to profile.log.");
    }

    //_getch(); // Returns -1 if no console attached
    //if (!HaveConsole || toupper(_getch()) == 'F')
    {
        FILE *f = fopen("profile.log", "w");
        WriteProfileOutput(f, seconds, WPO_GOODFIRST | WPO_LONGFMT | WPO_PRUNETINY);
        fclose(f);
    }
    lfaUnload(ModBase);
}

extern "C" void __declspec(naked) _cdecl _penter( void )
{
#ifdef _DEBUG
   _asm {
      push ebx  // EAX and EDX need not be saved (at least for __cdecl functions).
      push ecx  // ECX holds the "this" pointer for class member functions.

      // stack[rcount].addr = ReturnAddress();
      // stack[rcount].functionTime = Now();
      // stack[rcount].childTime = 0;
      // rcount++;

      // push function and time onto call stack
      mov eax, rcount
      mov ebx, offset stack
      imul eax, 0x18 // sizeof(PROFREC)
      add ebx, eax
      //lea eax, [eax * 2 + eax]  // these two lines accomplish (ebx += eax * 0x18), but not much faster than the imul+add
      //lea ebx, [eax * 8 + ebx]
      rdtsc
      mov ecx, [esp + 8] // function starting address (our return address)
      mov [ebx], ecx
      xor ecx, ecx
      mov [ebx + 0x04], eax // function time lower
      mov [ebx + 0x08], edx // function time upper
      mov [ebx + 0x0C], ecx // child time lower = 0
      mov [ebx + 0x10], ecx // child time upper = 0
      inc rcount

      pop ecx
      pop ebx
      ret
   }
#else
   _asm ret
#endif //_DEBUG
}

extern "C" void __declspec(naked) _cdecl _pexit( void )
{
#ifdef _DEBUG
    UINT64 elapsed;
    register PROFREC *pSave;
    void *addr;
    int i, hash;

    _asm {                                       // function prologue
        //push ebp                                 // save base pointer
        pushad
        mov ebp, esp                             // set up stack frame
        sub esp, 0x18

        // save registers the caller needs unchanged.
        // Pushing 6 registers is faster than pushad/popad; would you believe 3% faster overall?  Me, neither.  But it is.
        //push eax
        //push ebx
        //push ecx
        //push edi     // shouldn't need this one, but I guess we do when we optimize.  Buh.
        //push esi

        rdtsc                                    // read 
        mov dword ptr [elapsed    ], eax
        mov dword ptr [elapsed + 4], edx
    }

    pSave = save;

    if (!atexit_registered)
    {
        // save starting time values to figure out CPU speed
        ReadTimestampCounter(&startTime_tsc);
        QueryPerformanceCounter(&startTime_hpc);
        atexit(PrintProfileData);
        atexit_registered = true;
    }

    --rcount;
    // compute time spent inside function, including children
    stack[rcount].functionTime = elapsed -= stack[rcount].functionTime;

    addr = stack[rcount].address;
    hash = (((DWORD)addr - 0x400000) >> 3) * 1000003;
    
    // If the call stack is not empty, update parent.childTime
    if (rcount > 0)
        stack[rcount - 1].childTime += elapsed;

    if (scount < MAXRECORDS)
    {
        pSave = &save[hash & (MAXRECORDS-1)];
        for (i = 0; i < MAXRECORDS; i++, pSave++)
        {
            if (i == MAXRECORDS - hash)  // wrap at end of array
                pSave = save;
            if (pSave->address == addr)  // found the function record matching this address?
                goto got_it;
            if (pSave->count == 0) // unused
            {
                *pSave = stack[rcount];
                pSave->count = 1;
                goto done;
            }
        }
        pSave = save;
        goto got_it;
    }
    else {
        pSave = save;
got_it:
        pSave->childTime += stack[rcount].childTime;
        pSave->functionTime += stack[rcount].functionTime;
        pSave->count++;
    }
done:
    _asm {
        //pop esi         // restore registers
        //pop edi     // shouldn't need this one
        //pop ecx
        //pop ebx
        //pop eax
        mov esp, ebp    // restore stack pointer
        //pop ebp         // restore base pointer
        popad
        ret
    }
#else
   _asm ret
#endif //_DEBUG
}

// end of original profile.cpp


///////////////////////////////////////////////////////////////////////////////
//
// LineFromAddr.cpp 
// 
// Author: Oleg Starodumov
// 
//


///////////////////////////////////////////////////////////////////////////////
// 
// Description: 
// 
// This example determines the file name and line number that corresponds 
// to an address specified by the user 
// 
// This example shows how to: 
// 
//   * Define _NO_CVCONST_H to be able to use various non-default declarations 
//     from DbgHelp.h (e.g. SymTagEnum enumeration) 
//   * Initialize DbgHelp 
//   * Load symbols for a module or from a .PDB file 
//   * Check what kind of symbols is loaded 
//   * Look up a symbol by address (supplied by the user)
//   * Display simple information about the symbol 
//   * Look up file name and line number information and display it 
//   * Unload symbols 
//   * Deinitialize DbgHelp 
//
// Actions: 
// 
//   * Enable debug option 
//   * Initialize DbgHelp 
//   * If symbols should be loaded from a .PDB file, determine its size 
//   * Load symbols 
//   * Obtain and display information about loaded symbols 
//   * Look up a symbol by address 
//   * Display simple information about the symbol
//   * Look up file name and line number information for an address 
//   * Unload symbols 
//   * Deinitialize DbgHelp 
//
// Command line parameters: 
// 
//   * Path to the module you want to load symbols for, 
//     or to a .PDB file to load the symbols from 
//   * Address 
//


///////////////////////////////////////////////////////////////////////////////
// Include files 
//
#include <windows.h>
#include <tchar.h>

#pragma warning(disable: 4005)  // __out_xcount : macro redefinition

// Now we have to define _NO_CVCONST_H to be able to access 
// various declarations from DbgHelp.h, which are not available by default 
#define _NO_CVCONST_H
// specstrings.h does not define __out_xcount
__if_not_exists(__out_xcount)
{
#define __out_xcount(x)
}

#include <dbghelp.h>
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
// Directives 
//

#pragma comment( lib, "dbghelp.lib" )


///////////////////////////////////////////////////////////////////////////////
// Declarations 
//

bool GetFileParams( const TCHAR* pFileName, DWORD64& BaseAddr, DWORD& FileSize );
bool GetFileSize( const TCHAR* pFileName, DWORD& FileSize );


///////////////////////////////////////////////////////////////////////////////
// Helper classes 
// 

// Wrapper for SYMBOL_INFO_PACKAGE structure 

struct CSymbolInfoPackage : public SYMBOL_INFO_PACKAGE 
{
	CSymbolInfoPackage() 
	{
		si.SizeOfStruct = sizeof(SYMBOL_INFO); 
		si.MaxNameLen   = sizeof(name); 
	}
};

// Wrapper for IMAGEHLP_LINE64 structure 

struct CImageHlpLine64 : public IMAGEHLP_LINE64 
{
	CImageHlpLine64() 
	{
		SizeOfStruct = sizeof(IMAGEHLP_LINE64); 
	}
}; 


///////////////////////////////////////////////////////////////////////////////
// main 
//

BOOL lfaInit(HANDLE hProc, HMODULE hModule, DWORD64 &ModBase)
{
	BOOL bRet = FALSE; 

	// Set options 

	DWORD Options = SymGetOptions(); 

		// SYMOPT_DEBUG option asks DbgHelp to print additional troubleshooting 
		// messages to debug output - use the debugger's Debug Output window 
		// to view the messages 

	Options |= SYMOPT_DEBUG; 

		// SYMOPT_LOAD_LINES option asks DbgHelp to load line information 

	Options |= SYMOPT_LOAD_LINES; // load line information 

	::SymSetOptions( Options ); 


	// Initialize DbgHelp and load symbols for all modules of the current process 

	bRet = ::SymInitialize ( 
	            hProc,  // Process handle of the current process 
	            NULL,                 // No user-defined search path -> use default 
	            FALSE                 // Do not load symbols for modules in the current process 
	          ); 

	if( !bRet ) 
        return false;

	// Determine the base address and the file size 

	TCHAR FileName[MAX_PATH];
    GetModuleFileName(hModule, FileName, MAX_PATH);

	DWORD64   BaseAddr  = 0; 
	DWORD     FileSize  = 0; 

	if( !GetFileParams( FileName, BaseAddr, FileSize ) ) 
	{
		_tprintf( _T("Error: Cannot obtain file parameters (internal error).\n") ); 
		return 0;
	}


	// Load symbols for the module 

	_tprintf( _T("Loading symbols for: %s ... \n"), FileName ); 

	ModBase = ::SymLoadModule64 ( 
							GetCurrentProcess(), // Process handle of the current process 
							NULL,                // Handle to the module's image file (not needed)
							FileName,           // Path/name of the file 
							NULL,                // User-defined short name of the module (it can be NULL) 
							BaseAddr,            // Base address of the module (cannot be NULL if .PDB file is used, otherwise it can be NULL) 
							FileSize             // Size of the file (cannot be NULL if .PDB file is used, otherwise it can be NULL) 
						); 

	if( ModBase == 0 ) 
	{
		_tprintf(_T("Error: SymLoadModule64() failed. Error code: %u \n"), ::GetLastError());
		return 0; 
	}
    return 1;
}

// Look up symbol by address 
bool LineFromAddr(HANDLE hProc, DWORD64 SymAddr,
                  TCHAR *name, size_t nameLen,
                  DWORD64 *pLine, DWORD64 *pDisp)
{
    CSymbolInfoPackage sip; // it contains SYMBOL_INFO structure plus additional 
                            // space for the name of the symbol 

    DWORD64 Displacement = 0; 

    BOOL bRet = ::SymFromAddr( 
        hProc,               // Process handle
        SymAddr,             // Symbol address 
        &Displacement,       // Address of the variable that will receive the displacement 
        &sip.si              // Address of the SYMBOL_INFO structure (inside "sip" object) 
    );

	if( !bRet ) 
        return false;

    if (pLine || pDisp)
    {
	    // Obtain file name and line number for the address 

	    CImageHlpLine64 LineInfo; 
	    DWORD LineDisplacement = 0; // Displacement from the beginning of the line 

	    bRet = SymGetLineFromAddr64( 
	              GetCurrentProcess(), // Process handle of the current process 
	              SymAddr,             // Address 
	              &LineDisplacement, // Displacement will be stored here by the function 
	              &LineInfo          // File name / line information will be stored here 
	            ); 

	    if( !bRet ) 
	    {
		    _tprintf( _T("Line information not found. Error code: %u \n"), ::GetLastError() ); 
            LineInfo.LineNumber = 0;
            LineDisplacement = 0;
	    }

        if (pLine)
            *pLine = LineInfo.LineNumber;
        if (pDisp)
            *pDisp = LineDisplacement;
    }
    if (name)
    {
        strncpy(name, sip.si.Name, nameLen);
        if (strlen(sip.si.Name) >= nameLen)
            name[nameLen - 1] = 0;
    }
    return true;
}

int lfaUnload(DWORD64 ModBase)
{
	// Unload symbols for the module 

	BOOL bRet = ::SymUnloadModule64( GetCurrentProcess(), ModBase ); 

	if( !bRet )
	{
		_tprintf( _T("Error: SymUnloadModule64() failed. Error code: %u \n"), ::GetLastError() ); 
	}

	// Deinitialize DbgHelp 

	bRet = ::SymCleanup( GetCurrentProcess() ); 

	if( !bRet ) 
	{
		_tprintf(_T("Error: SymCleanup() failed. Error code: %u \n"), ::GetLastError());
		return 0; 
	}

	// Complete 

	return 0; 
}


///////////////////////////////////////////////////////////////////////////////
// Functions 
//

bool GetFileParams( const TCHAR* pFileName, DWORD64& BaseAddr, DWORD& FileSize ) 
{
	// Check parameters 

	if( pFileName == 0 ) 
	{
		return false; 
	}


	// Determine the extension of the file 

	TCHAR szFileExt[_MAX_EXT] = {0}; 

	_tsplitpath( pFileName, NULL, NULL, NULL, szFileExt ); 

	
	// Is it .PDB file ? 

	if( _tcsicmp( szFileExt, _T(".PDB") ) == 0 ) 
	{
		// Yes, it is a .PDB file 

		// Determine its size, and use a dummy base address 

		BaseAddr = 0x10000000; // it can be any non-zero value, but if we load symbols 
		                       // from more than one file, memory regions specified 
		                       // for different files should not overlap 
		                       // (region is "base address + file size") 
		
		if( !GetFileSize( pFileName, FileSize ) ) 
		{
			return false; 
		}

	}
	else 
	{
		// It is not a .PDB file 

		// Base address and file size can be 0 

		BaseAddr = 0; 
		FileSize = 0; 
	}


	// Complete 

	return true; 

}

bool GetFileSize( const TCHAR* pFileName, DWORD& FileSize )
{
	// Check parameters 

	if( pFileName == 0 ) 
	{
		return false; 
	}


	// Open the file 

	HANDLE hFile = ::CreateFile( pFileName, GENERIC_READ, FILE_SHARE_READ, 
	                             NULL, OPEN_EXISTING, 0, NULL ); 

	if( hFile == INVALID_HANDLE_VALUE ) 
	{
		_tprintf( _T("CreateFile() failed. Error: %u \n"), ::GetLastError() ); 
		return false; 
	}


	// Obtain the size of the file 

	FileSize = ::GetFileSize( hFile, NULL ); 

	if( FileSize == INVALID_FILE_SIZE ) 
	{
		_tprintf( _T("GetFileSize() failed. Error: %u \n"), ::GetLastError() ); 
		// and continue ... 
	}


	// Close the file 

	if( !::CloseHandle( hFile ) ) 
	{
		_tprintf( _T("CloseHandle() failed. Error: %u \n"), ::GetLastError() ); 
		// and continue ... 
	}


	// Complete 

	return ( FileSize != INVALID_FILE_SIZE ); 

}

#endif // PROFILE
