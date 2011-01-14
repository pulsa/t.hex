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

/*  To do:
Use an expandable hash table.  Try Judy arrays, or uthash.

Would we get better performance by doing hash table lookups in _penter()?
    This might eliminate the need to store anything more than pointers on the function stack.

Make everything thread-safe.
*/

/* Calling conventions de-mystified
http://www.codeproject.com/KB/cpp/calling_conventions_demystified.aspx

__cdecl
    Stack cleanup is done by the caller

__stdcall
    Stack cleanup is done by the called function.

__fastcall
    The first two arguments are in ECX and EDX.
    Further arguments are popped from the stack by the called function.

__thiscall
    Like stdcall, except ECX contains "this."

Member functions with a variable number of arguments get "this" on the stack last.

VC++ uses esi and edi to store intermediate evaluation results.
What is EBX for?

EAX, ECX, and EDX are caller-save registers.
EBX, ESI, and EDI are callee-save registers.

I found out using ESP as an index register takes an extra byte to encode the instruction.  Interesting.
*/

//#include "precomp.h"
//#define PROFILE
#undef UNICODE
#undef _UNICODE
#ifdef PROFILE

//*****************************************************************************
// Parameters that need to change for different projects.

#ifndef PROFILE_MODULE_NAME
#define PROFILE_MODULE_NAME     NULL  // Change this to the name of the DLL to profile.
#endif

// Size of stack.  Bad things happen if this is overflowed.
#ifndef PROFILE_MAXSTACK
#define PROFILE_MAXSTACK 512
#endif

// Size of hash table.  Must be a power of two for hash function bit mask.
#ifndef PROFILE_MAXRECORDS
#define PROFILE_SAVE_BITCOUNT 11
#define PROFILE_MAXRECORDS (1 << (PROFILE_SAVE_BITCOUNT))
#endif

//#define PROFILE_HASH_DEBUG  // Define this to collect and print information about hash collisions.

#define PROFILE_HDBG(x)

//*****************************************************************************

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

#include "MyTable.h"
#include "RegConfig.h"

#define PRINTF printf

typedef UINT32 ADDR;

#pragma pack(push, 4)
typedef struct tagPROFREC_STACK {
    ADDR address;
    UINT64 functionTime;
    UINT64 childTime;
} PROFREC_STACK;
#pragma pack(pop)

typedef struct tagPROFREC {
    ADDR address;
    int count;
    UINT64 functionTime;
    UINT64 childTime;
#ifdef PROFILE_HASH_DEBUG
    int stepped_under, stepped_over;
#endif
} PROFREC;

typedef struct tagPROFREC_FINAL {
    PROFREC *rec;     // points to save[i]
    char *funcname;   // function name, allocated by strdup()
    char *filename;   // source file name, allocated by strdup()
    DWORD64 sourceLine;
#ifdef PROFILE_HASH_DEBUG
    DWORD hash;
#endif
} PROFREC_FINAL;

typedef struct tagPROFILE_SUMMARY {
    PROFREC_FINAL *recf;
    int count;       // number of profile records to print
    int pruned;      // number of functions pruned because time measured was <100us
    //int negative;
    int max_rcount;  // maximum stack depth of profiled functions
    double runtime;  // Total run time after first profiled function call
    int longestNameLen;  // characters in the longest function name, for table alignment
    int avgFileLen;  // for table alignment
    ULONG maxLineNum;  // for table alignment
    double collectTime;  // seconds spent in PrintProfileData(), before WriteProfileOutput().
    DWORD64 totalCycles, cpu;
} PROFILE_SUMMARY;

static PROFREC save[PROFILE_MAXRECORDS]; // records for all functions that have been called
static PROFREC_STACK stack[PROFILE_MAXSTACK+1];  // records on the current function stack
static PROFREC_STACK *pstack = stack+1;  // Track the current function record on the stack.
static int scount = 0;
static bool atexit_registered = 0;

#ifdef PROFILE_HASH_DEBUG
static int g_lookups = 0, g_collisions = 0, g_longest = 1;
DWORD g_longestChainHash;
#endif

BOOL lfaInit(HANDLE hProc, HMODULE hModule, DWORD64 &ModBase);
bool LineFromAddr(HANDLE hProc, DWORD64 SymAddr,
                  TCHAR *name, size_t nameLen,
                  DWORD64 *pLine, DWORD64 *pDisp, PCHAR *ppFileName = 0);
int lfaUnload(DWORD64 ModBase);
int asprintf(char **strp, const char *fmt, ...);  // Allocate a buffer and sprintf() to it.

LPCTSTR GetExeName()
{
    static TCHAR buffer[MAX_PATH];
    if (GetModuleFileName(GetModuleHandle(NULL), buffer, MAX_PATH) == 0)
        return NULL;
    return buffer;
}

static BOOL bProfile = RegConfig("Software\\Adam Bailey\\Profile").ReadInt(GetExeName(), 1, true);

void __declspec(naked) __fastcall ReadTimestampCounter(DWORD64 *now)
{
    _asm
    {
        rdtsc
        mov [ecx], eax  // with fastcall, first function argument is already in ecx.
        mov [ecx + 4], edx
        ret
    }
}

DWORD __declspec(naked) GetCurrentThreadId2()
{
    _asm {
        mov eax, fs:[0x24]  // In NT, the TIB stores the thread ID here.
        ret
    }
}

// Trying to measure the difference between tick counts on different cores.  AEB  2008-08-11
__int64 GetCPUTimeSkew(DWORD64 cpufreq)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    int nCPU = sysinfo.dwNumberOfProcessors;

    if (nCPU <= 1)
        return 0;

    LARGE_INTEGER perffreq, *perfcount = (LARGE_INTEGER*)malloc(nCPU * sizeof(LARGE_INTEGER));
    DWORD64 *pcount = (DWORD64*)malloc(nCPU * sizeof(DWORD64));
    __int64 maxdiff = 0;
    QueryPerformanceFrequency(&perffreq);
    HANDLE hProc = GetCurrentProcess();

    for (int cpu = 0; cpu < nCPU; cpu++)
    {
        SetProcessAffinityMask(hProc, 1 << cpu);
        Sleep(0);    //! Does this do any good?
        __asm cpuid  //! or this?  Probably not.
        QueryPerformanceCounter(&perfcount[cpu]);
        ReadTimestampCounter(&pcount[cpu]);
    }

    for (int cpu = 1; cpu < nCPU; cpu++)
    {
        __int64 real = (__int64)((double)(perfcount[cpu].QuadPart - perfcount[cpu-1].QuadPart) / perffreq.QuadPart * cpufreq);
        __int64 rdtsc = (__int64)(pcount[cpu] - pcount[cpu-1]);
        //maxdiff = max(maxdiff, real - rdtsc);
        if (_abs64(real - rdtsc) > _abs64(maxdiff))
            maxdiff = real - rdtsc;
    }

    delete [] perfcount;
    delete [] pcount;

    return maxdiff;
}

static DWORD64 startTime_tsc;
static LARGE_INTEGER startTime_hpc;

int cmpfunc(const void *p1, const void *p2)
{
#ifdef PROFILE_HASH_DEBUG
    const PROFREC_FINAL *pr1 = (const PROFREC_FINAL*)p1;
    const PROFREC_FINAL *pr2 = (const PROFREC_FINAL*)p2;
    if (pr1->rec->stepped_over > pr2->rec->stepped_over)    return 1;
    if (pr1->rec->stepped_over < pr2->rec->stepped_over)    return -1;
    if (pr1->rec->stepped_under > pr2->rec->stepped_under)  return 1;
    if (pr1->rec->stepped_under < pr2->rec->stepped_under)  return -1;
    if (pr1->rec->address > pr2->rec->address) return -1;  //! inverted because table prints backwards.
    if (pr1->rec->address < pr2->rec->address) return 1;
    return 0;
#else
    const PROFREC *r1 = ((const PROFREC_FINAL*)p1)->rec;
    const PROFREC *r2 = ((const PROFREC_FINAL*)p2)->rec;
    if (r1->functionTime > r2->functionTime) return 1;
    if (r1->functionTime < r2->functionTime) return -1;
    if (r1->childTime > r2->childTime) return 1;
    if (r1->childTime < r2->childTime) return -1;
    if (r1->count > r2->count) return 1;
    if (r1->count < r2->count) return -1;
    return 0;
#endif
}


//time        total     avg
//----------- ------    ------
//0.00012345       -         -
//0.0012345        -     0.001
//0.012345         -     0.012
//0.12345        0.1     0.123
//1.2345         1.2     1.235
//12.345        12.3     12.35
//123.45       123.5     123.5
//1234.5      1234.5      1234
//12345          12s     12.3s
//123450        123s      123s
//1234500      1234s     1234s

int sprintfTotal(char *buf, double ms)
{
    if (ms < 0.1) {
        buf[0] = 0;
        return 0;
    }
    else if (ms < 10000.0)
        return sprintf(buf, "%.1f", ms);
    else
        return sprintf(buf, "%.0fs", ms * .001);
}

int sprintfAvg(char *buf, double ms)
{
    if (ms < .001) {
        buf[0] = 0;
        return 0;
    }
    else if (ms < 10.0)
        return sprintf(buf, "%.3f", ms);
    else if (ms < 100.0)
        return sprintf(buf, "%.2f", ms);
    else if (ms < 1000.0)
        return sprintf(buf, "%.1f", ms);
    else if (ms < 100000.0)
        return sprintf(buf, "%.1fs", ms * .001);
    else
        return sprintf(buf, "%.0fs", ms * .001);
}

size_t GetDigits(ULONG num)
{
   int digits = 1;
   while (num >= 10)
      digits++, num /= 10;
   return digits;
}

#define WPO_GOODFIRST  1  // When writing to a file, put the interesting part first.
#define WPO_GOODLAST   0  // When writing to a console, put the interesting part last.
#define WPO_LONGFMT    2  // Print more information after the table.  Needs WPO_GOODFIRST.
#define WPO_SHORTFMT   0
#define WPO_FILENAME   8  // Print source file name and line number of profiled functions.

void WriteProfileOutput(FILE *f, PROFILE_SUMMARY *data, DWORD flags = 0)
{
    HANDLE hProc = GetCurrentProcess();
    char str_name[100], str_self[100], str_child[100], str_calls[20], str_savg[50], str_cavg[50], str_file[200];
    char str_over[20], str_under[20], str_addr[20], str_hash[20];
    fprintf(f, "Times are in milliseconds, unless noted.\n");
    char *name;
    bool longfmt = (flags & WPO_LONGFMT) != 0;
    bool goodfirst = (flags & WPO_GOODFIRST) != 0;
    bool bFileName = (flags & WPO_FILENAME) != 0;
    int lineDigits = GetDigits(data->maxLineNum);

    MyTable tab;

    if (longfmt) {
#ifdef PROFILE_HASH_DEBUG
        tab.Init(11, MyTable::RIGHT);
        tab.SetHeader("Self", "S.Avg", "Child", "C.Avg", "Calls", "Function", "File", "over", "under", "Address", "Hash");
#else
        tab.Init(7, MyTable::RIGHT);
        tab.SetHeader("Self", "S.Avg", "Child", "C.Avg", "Calls", "Function", "File");
#endif
        tab.Align(5, MyTable::LEFT);
        tab.Align(6, MyTable::LEFT);
    }
    else {
        tab.Init(4, MyTable::RIGHT);
        tab.SetHeader("Self", "Child", "Calls", "Function");
        tab.Align(3, MyTable::LEFT);
    }

    for (int i = 0; i < data->count; i++)
    {
        double self = data->recf[i].rec->functionTime * 0.1;
        double child = data->recf[i].rec->childTime * 0.1;

        if (data->recf[i].funcname)
            name = data->recf[i].funcname;
        else
            _sntprintf(name = str_name, 100, _T("%08X"), save[i].address);

        str_self[0] = str_child[0] = str_savg[0] = str_cavg[0] = 0;
        sprintfTotal(str_self, self);
        sprintfTotal(str_child, child);
        if (data->recf[i].rec->count < 1000000)
            sprintf(str_calls, "%d", data->recf[i].rec->count);
        else
            sprintf(str_calls, "%0.1fM", data->recf[i].rec->count * .000001);

        if (longfmt)
        {
            if (self && data->recf[i].rec->count > 1)
                sprintfAvg(str_savg, self / data->recf[i].rec->count);
            if (child && data->recf[i].rec->count > 1)
                sprintfAvg(str_cavg, child / data->recf[i].rec->count);

#ifdef PROFILE_HASH_DEBUG
            sprintf(str_over, "%d", data->recf[i].rec->stepped_over);
            sprintf(str_under, "%d", data->recf[i].rec->stepped_under);
            sprintf(str_addr, "%p", data->recf[i].rec->address);
            sprintf(str_hash, "%X", data->recf[i].hash);
#endif

            const char *file = data->recf[i].filename;
            if (bFileName && file)
            {
                // shorten line number if this file is longer than average
                int diff = max(0, (int)strlen(file) - data->avgFileLen);
                int n = max(lineDigits - diff, 0);

                _snprintf(str_file, 200, "%-*s %*d", 
                    data->avgFileLen, file,
                    n, (int)data->recf[i].sourceLine);
                tab.AddRow(str_self, str_savg, str_child, str_cavg, str_calls, name, str_file, str_over, str_under, str_addr, str_hash);
            }
            else
               tab.AddRow(str_self, str_savg, str_child, str_cavg, str_calls, name, "", str_over, str_under, str_addr, str_hash);
        }
        else
            tab.AddRow(str_self, str_child, str_calls, name);
    }

    tab.Print(f, goodfirst);

    if (goodfirst)
    {
        fprintf(f, "\n");
        //fprintf(f, "CPU frequency: %d MHz  (to check timer accuracy)\n", (int)(data->cpu / 1000000));
        fprintf(f, "Collected %d records.", data->count + data->pruned);
        if (data->pruned)
            fprintf(f, "  Pruned %d zero-time functions, leaving %d.\n", data->pruned, data->count);
        else
            fprintf(f, "\n");
        if (longfmt && data->max_rcount)
            fprintf(f, "Max stack depth: %d\n", data->max_rcount);
        if (data->totalCycles)
            fprintf(f, "Total function time: %0.3f seconds.\n", data->totalCycles / (double)data->cpu);
        if (data->runtime)
            fprintf(f, "Total run time after first profiled function call: %0.3f seconds.\n", data->runtime);
        //if (longfmt)
        //    fprintf(f, "Time spent in PrintProfileData(): %0.3f seconds.\n", data->collectTime);
#ifdef PROFILE_HASH_DEBUG
        fprintf(f, "\n");
        fprintf(f, "Hash table load factor: %0.3f\n", (double)(data->count + data->pruned) / PROFILE_MAXRECORDS);
        fprintf(f, "%d lookups, %d collisions; avg. chain length %0.3f\n",
           g_lookups, g_collisions, (double)(g_lookups + g_collisions) / g_lookups);
        fprintf(f, "Longest hash chain was %d entries.\n", g_longest);
        for (int idx = 0; idx < data->count; idx++)
            if (data->recf[idx].rec - save == g_longestChainHash) {
                fprintf(f, "End node: %s\n", data->recf[idx].funcname);
                break;
            }
#endif
        //fprintf(f, "Memory allocated for table printing: %d kB\n", tab.m_nMemAlloc / 1024);
#if 0
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        for (int i = 0; i < 10; i++)
            fprintf(f, "GetCPUTimeSkew() = %I64d cycles\n", GetCPUTimeSkew(freq.QuadPart));
#endif
    }
    else if (data->pruned)
        fprintf(f, "Pruned %d zero-time functions.\n", data->pruned);

    //fprintf(f, "Negative: %d\n", data->negative);
}

void FreeSummary(PROFILE_SUMMARY &data)
{
    for (int i = 0; i < data.count; i++)
    {
        free(data.recf[i].funcname);
        free(data.recf[i].filename);
    }
    delete [] data.recf;
}

void PrintProfileData()
{
    scount = PROFILE_MAXRECORDS;  // STOP RECORDING ALREADY!
    if (!bProfile)
        return;

    char name[200];
    int totalFileLen = 0, noFile = 0;

    // calculate CPU speed
    DWORD64 now_tsc;
    LARGE_INTEGER now_hpc, freq;
    ReadTimestampCounter(&now_tsc);
    QueryPerformanceCounter(&now_hpc);
    QueryPerformanceFrequency(&freq);

    PROFILE_SUMMARY data;  // store everything here to print out
    //data.negative = 0;
    data.pruned = 0;
    data.longestNameLen = 0;
    data.avgFileLen = 0;
    data.maxLineNum = 0;
    data.runtime = (now_hpc.QuadPart - startTime_hpc.QuadPart) / (double)freq.QuadPart;
    data.collectTime = 0;
    data.cpu = (DWORD64)((now_tsc - startTime_tsc) / data.runtime);
    data.totalCycles = 0;

    DWORD64 ModBase;
    HANDLE hProc = GetCurrentProcess();
    HMODULE hMod = GetModuleHandle(PROFILE_MODULE_NAME);
    //DWORD64 LoadedModBase = (DWORD64)hMod;  //! ... because the HMODULE value happens to be the base address.  Is that ugly or cool?
    MODULEINFO modinfo;
    GetModuleInformation(hProc, hMod, &modinfo, sizeof(modinfo));
    DWORD64 LoadedModBase = (DWORD64)modinfo.lpBaseOfDll;

    if (!lfaInit(hProc, hMod, ModBase))
    {
       printf("Couldn't load debug info.  You don't want a sea of addresses, so I quit.\n");
       return;
    }

    data.count = 0;
    for (int i = 0; i < PROFILE_MAXRECORDS; i++)
    {
        if (!save[i].address)
            continue;
        save[i].address += (ADDR)(ModBase - LoadedModBase);
        save[i].functionTime -= save[i].childTime;
        //save[i].functionTime -= 21 * save[i].count; //! overhead test
        //if ((INT64)save[i].functionTime <= save[i].count)
        //   data.negative++;
        data.totalCycles += save[i].functionTime;

        save[i].functionTime = save[i].functionTime * 10000 / data.cpu;
        save[i].childTime = save[i].childTime * 10000 / data.cpu;

#ifdef PROFILE_HASH_DEBUG
        if (i == g_longestChainHash)
            g_longestChainHash = data.count;
#else
        if (!save[i].functionTime /*&& !save[i].childTime*/)
        {
            data.pruned++;
            save[i].address = 0;
            continue;
        }
#endif
        //save[data.count] = save[i];  // move each record up, eliminating gaps.
        data.count++;
    }

    // Get debug info for each record we care about while we fill the sequential PROFREC_FINAL array.
    data.recf = new PROFREC_FINAL[data.count];
    for (int i = 0, j = 0; j < PROFILE_MAXRECORDS; j++)
    {
        if (!save[j].address)
            continue;
        data.recf[i].rec = &save[j];
        PROFILE_HDBG(data.recf[i].hash = j;)
        if (LineFromAddr(hProc, (DWORD64)save[j].address, name, 200,
            &data.recf[i].sourceLine, NULL, &data.recf[i].filename))
        {
            // Remove "std::" at the beginning of words.
            char * tag = name;
            while ((tag = strstr(tag, "std::")) != NULL)
            {
                if (tag == name || !isalnum(tag[-1]))
                    memmove(tag, tag+5, 1+strlen(tag+5));
                else
                    tag++;
            }

            // See if we can shorten class constructors and destructors.
            tag = strstr(name, "::");
            if (tag && tag[2] == '~' && !strncmp(name, tag+3, tag - name))
                strcpy(tag, "::~");  // truncate extra class name for destructor
            else if (tag && !strncmp(name, tag+2, tag - name))
                strcpy(tag, "::()");  // truncate extra class name for constructor

            tag = strstr(name, "::`scalar deleting destructor'");
            if (tag)
                strcpy(tag, "::~scalar");
            tag = strstr(name, "::`vector deleting destructor'");
            if (tag)
                strcpy(tag, "::~vector");

            // Change all "::" to ":".
            //tag = name;
            //while ((tag = strstr(tag, "::")) != NULL)
            //    memmove(tag, tag+1, 1+strlen(tag+1));

            // Remove all spaces.
            tag = name;
            while ((tag = strchr(tag, ' ')) != NULL)
                memmove(tag, tag+1, 1+strlen(tag+1));

            name[100] = 0;  // truncate it here, after condensing the name.

            data.recf[i].funcname = _strdup(name);
            data.longestNameLen = max(data.longestNameLen, (int)strlen(data.recf[i].funcname));

            // Get file name without path.
            char *file = data.recf[i].filename;
            int baselen = file ? strlen(file) : 0;
            if (baselen) {
                while (baselen > 0 && file[baselen-1] != '\\' && file[baselen-1] != '/')
                     baselen--;

                data.recf[i].filename = _strdup(file + baselen);
                totalFileLen += strlen(file + baselen);
            }
            else {
               data.recf[i].filename = NULL;
               noFile++;
            }

            data.maxLineNum = max(data.maxLineNum, (ULONG)data.recf[i].sourceLine);
        }
        else
        {
            data.recf[i].filename = NULL;
            data.recf[i].funcname = NULL;
            data.longestNameLen = max(data.longestNameLen, 8);  // "%08X"
        }
        i++;
    }

    if (data.count > noFile) {
        data.avgFileLen = (totalFileLen + data.count - 1) / (data.count - noFile);  // average, rounded up.
        data.avgFileLen += data.avgFileLen / 8;  // fudge factor.
    }

    // now we could sort by address, name, time, or whatever.
#ifndef PROFILE_HASH_DEBUG
    qsort(data.recf, data.count, sizeof(PROFREC_FINAL), cmpfunc);
#endif

    // max_rcount was being tracked inside _pexit(), but let's do it here instead.
    for (data.max_rcount = 0; data.max_rcount < PROFILE_MAXSTACK; data.max_rcount++)
    {
        if (stack[data.max_rcount+1].address == 0)
            break;
    }

    LARGE_INTEGER later;
    QueryPerformanceCounter(&later);
    data.collectTime = (double)(later.QuadPart - now_hpc.QuadPart) / freq.QuadPart;

    bool HaveConsole = false;//(GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE);
    if (HaveConsole)
    {
        WriteProfileOutput(stdout, &data);
        //PRINTF("Press 'F' to write to profile.log.");
    }

    //_getch(); // Returns -1 if no console attached
    //if (!HaveConsole || toupper(_getch()) == 'F')
    {
        FILE *f = fopen("profile.log", "w");
        if (f)
        {
            WriteProfileOutput(f, &data, WPO_GOODFIRST | WPO_LONGFMT | WPO_FILENAME);
            fclose(f);
        }
        else
        {
            const char *syserr = strerror(errno);
            char *msg, *caption;
            asprintf(&msg, "Can't write to profile.log: %s", syserr);
            asprintf(&caption, "Profile.cpp (PID %d)", GetCurrentProcessId());
            printf("%s   %s\n", caption, msg);
            MessageBoxA(NULL, msg, caption, MB_OK | MB_ICONERROR);
            free(msg);
            free(caption);
        }
    }
    lfaUnload(ModBase);

    FreeSummary(data);
}

void RegisterProfileExitHandler()
{
    // save starting time values to figure out CPU speed
    ReadTimestampCounter(&startTime_tsc);
    QueryPerformanceCounter(&startTime_hpc);
    atexit(PrintProfileData);
    atexit_registered = true;
}


extern "C" void __declspec(naked) _cdecl _penter( void )
{
    // Push function address and time onto call stack.

    // pstack->addr = ReturnAddress();
    // pstack->functionTime = Now();
    // pstack->childTime = 0;
    // pstack++;

    //! If using __fastcall, we have to preserve EDX.

#if 0  // These two implementations are about equally fast.
    _asm {
        mov [esp - 8], ebx  // EAX and EDX need not be saved (at least for __cdecl functions).
        mov [esp - 4], ecx  // ECX holds the "this" pointer for class member functions.

        mov ebx, [pstack]
        mov ecx, [esp]        // function starting address (our return address)
        rdtsc                 // read counter into EDX:EAX
        mov [ebx], ecx
        xor ecx, ecx
        mov [ebx + 0x04], eax // function time lower
        mov [ebx + 0x08], edx // function time upper
        mov [ebx + 0x0C], ecx // child time lower = 0
        mov [ebx + 0x10], ecx // child time upper = 0
        add [pstack], 0x14    // sizeof(PROFREC_STACK)

        mov ebx, [esp-8]
        mov ecx, [esp-4]
        ret
    }
#elif 1
    _asm {
        mov [esp-4], edi

        mov edi, [pstack]
        mov eax, [esp]                  // function starting address (our return address)
        mov [edi], eax
        rdtsc                           // read counter into EDX:EAX
        add [pstack], 0x14              // sizeof(PROFREC_STACK)
        mov dword ptr [edi + 0x0C], 0   // child time lower = 0
        mov dword ptr [edi + 0x10], 0   // child time upper = 0
        mov [edi + 0x04], eax           // function time lower
        mov [edi + 0x08], edx           // function time upper

        mov edi, [esp-4]
        ret
    }
#else
    _asm {
        mov [esp-4], edi

        mov eax, [pstack]
        mov edi, [esp]                  // function starting address (our return address)
        mov [eax], edi
        mov edi, eax
        rdtsc                           // read counter into EDX:EAX
        add [pstack], 0x14              // sizeof(PROFREC_STACK)
        mov dword ptr [edi + 0x0C], 0   // child time lower = 0
        mov dword ptr [edi + 0x10], 0   // child time upper = 0
        mov [edi + 0x04], eax           // function time lower
        mov [edi + 0x08], edx           // function time upper

        mov edi, [esp-4]
        ret
    }
#endif
}

extern "C" void __declspec(naked) _cdecl _pexit( void )
{
    UINT64 elapsed;
    register PROFREC *pSave;
    ADDR addr;
    int i, hash;
    // We set up the stack frame manually, so don't forget to change it if you add variables.

    _asm {                      // function prologue
        mov [esp -  4], ebp     // save base pointer
        //mov [pexit_save], ebp
        //mov [pexit_save+4], eax
        //mov [pexit_save+8], edx

        // save registers the caller needs unchanged.
        // Pushing 6 registers is faster than pushad/popad; would you believe 3% faster overall?  Me, neither.  But it is.
        mov [esp -  8], eax     // save profiled function's return code
        mov [esp - 12], edx     // save upper half of 64-bit return value
        //mov [esp - 16], ecx     // ECX is caller-save
        lea ebp, [esp-24]       // set up stack frame -- leave room for stored registers.
        rdtsc                   // read timestamp counter

#if 1 // save ebx and edi -- needed when compiler optimzation is turned on.
        mov [esp-16], ebx
        mov [esp-20], esi
        mov [esp-24], edi
#endif

        mov dword ptr [elapsed    ], eax    // store low word.  elapsed is now the TSC value.
        mov dword ptr [elapsed + 4], edx    // store high word
    }

    if (!atexit_registered)
    {
        _asm lea esp, [esp-52]       // 28 bytes for registers, 24 bytes for local variables.
        //RegisterProfileExitHandler();
        _asm call RegisterProfileExitHandler
        _asm lea esp, [esp+52]       // restore stack pointer
    }

    --pstack;
    // compute time spent inside function, including children
    pstack->functionTime = elapsed -= pstack->functionTime;

    addr = pstack->address;
    //hash = (((((DWORD)addr - 0x400000) >> 3) +
    //         (((DWORD)addr - 0x400000) >> 20)) * 1000003) & (PROFILE_MAXRECORDS-1);

    //! least profile overhead, according to some test with VecUtil.
    //hash = ((((DWORD)addr >> 3) +
    //         ((DWORD)addr >> 20)) * 1000003) & (PROFILE_MAXRECORDS-1);

    //! trying to avoid collisions in T.Hex, where profiled functions are far apart.
    //hash = ((((DWORD)addr >> 3) +
    //         ((DWORD)addr >> PROFILE_SAVE_BITCOUNT)) * 1000003) & (PROFILE_MAXRECORDS-1);

    hash = ((((DWORD)addr >> 4) +
             //((DWORD)addr >> 16) +
             ((DWORD)addr >> (PROFILE_SAVE_BITCOUNT+5))) * 1000003) & (PROFILE_MAXRECORDS-1);

    //hash = (((DWORD)addr >> 3) + ((DWORD)addr >> 20)) * 1000003;
    //hash = (hash ^ (hash >> 16)) & (PROFILE_MAXRECORDS - 1);

    // If the call stack is not empty, update parent.childTime
    //if (pstack != stack)  // pstack starts at stack[1], so pstack[-1] is always valid.
          pstack[-1].childTime += elapsed;

    i = 0;
    if (scount < PROFILE_MAXRECORDS)  // still room to store a record?
    {
        // Search the hash table for this address.
        while (1)  // loop through hash buckets
        {
            pSave = &save[hash];
            if (pSave->address == addr)  // found the function record matching this address?
            {
                pSave = &save[hash];
                pSave->functionTime += pstack->functionTime;
                pSave->childTime += pstack->childTime;
                pSave->count++;
                PROFILE_HDBG(pSave->stepped_over += i;)
                break;
            }
            if (pSave->address == 0)     // Found an unused spot.  Make a new record.
            {
                pSave = &save[hash];
                scount++;
                pSave->address = pstack->address;
                pSave->count = 1;
                PROFILE_HDBG(pSave->stepped_over = i;)
                PROFILE_HDBG(pSave->stepped_under = 0;)
                pSave->functionTime = pstack->functionTime;
                pSave->childTime = pstack->childTime;
                break;
            }
            i++;
            PROFILE_HDBG(pSave->stepped_under++;)
            hash = (hash + 1) & (PROFILE_MAXRECORDS - 1);
            //if (i == PROFILE_MAXRECORDS - hash)  // wrap at end of array
            //{
            //    pSave = save;
            //    hash = 0;
            //}
            //else
            //{
            //    pSave++;  // next hash index after collision
            //    hash++;
            //    //pSave = &save[hash = (hash + ((DWORD)addr >> 6)) & (PROFILE_MAXRECORDS-1)];  //! experimental
            //}
        }
    }

#ifdef PROFILE_HASH_DEBUG
    g_lookups++;
    if (i >= g_longest) {
        g_longest = i+1;
        g_longestChainHash = hash;
    }
    g_collisions += i;
#endif

    _asm {              // restore stack pointer and other registers
        //add esp, 52
        mov ebp, [esp-4]
        mov eax, [esp-8]
        mov edx, [esp-12]
        //mov ecx, [esp-16]  // ECX is caller-save
        //mov ebp, [pexit_save]
        //mov eax, [pexit_save+4]
        //mov edx, [pexit_save+8]
#if 1
        mov ebx, [esp-16]
        mov esi, [esp-20]
        mov edi, [esp-24]
#endif

        ret
    }
}


// vasprintf() allocates a buffer of the correct size and fills it with formatted output.
// asprintf() is the friendly version that takes a variable-length argument list.
// These two functions are part of GCC's C library, adapted for MSVC.
int vasprintf(char **strp, const char *fmt, va_list argptr)
{
    va_list save = argptr;
    if (strp == 0) return -1;
    int nSize = 1 + _vscprintf(fmt, argptr);
    *strp = (char*)malloc(nSize);
    if (!*strp)
       return -1;
    **strp = 0; // just in case _vsnprintf fails, I guess...
    return _vsnprintf(*strp, nSize, fmt, save);
}

int asprintf(char **strp, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vasprintf(strp, fmt, args);
}



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
//#include <windows.h>
//#include <tchar.h>

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
//#include <stdio.h>


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
	if (!SymInitialize(
	            hProc,  // Process handle of the current process
	            NULL,   // No user-defined search path -> use default
	            FALSE)) // Do not load symbols for modules in the current process
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
                  DWORD64 *pLine, DWORD64 *pDisp, PCHAR *ppFileName)
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

    if (pLine || pDisp || ppFileName)
    {
        // Obtain file name and line number for the address
        if (pLine)
            *pLine = 0;
        if (pDisp)
            *pDisp = 0;
        if (ppFileName)
           *ppFileName = "";

	    CImageHlpLine64 LineInfo;
	    DWORD LineDisplacement = 0; // Displacement from the beginning of the line

	    bRet = SymGetLineFromAddr64(
	              hProc,             // Process handle of the current process
	              SymAddr,           // Address
	              &LineDisplacement, // Displacement will be stored here by the function
	              &LineInfo          // File name / line information will be stored here
	            );

        if( !bRet )
        {
            _tprintf( _T("Line information not found for %s. Error code: %u \n"), sip.si.Name, ::GetLastError() );
            LineInfo.LineNumber = 0;
            LineDisplacement = 0;
            LineInfo.FileName = NULL;
        }

        if (pLine)
            *pLine = LineInfo.LineNumber;
        if (pDisp)
            *pDisp = LineDisplacement;
        if (ppFileName)
           *ppFileName = LineInfo.FileName;
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
		return false;


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
		return false;

	// Open the file
	HANDLE hFile = ::CreateFile(pFileName, GENERIC_READ, FILE_SHARE_READ,
	                            NULL, OPEN_EXISTING, 0, NULL );

	if( hFile == INVALID_HANDLE_VALUE )
	{
		_tprintf( _T("CreateFile() failed. Error: %u \n"), ::GetLastError() );
		return false;
	}

	// Obtain the size of the file.
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

#else // Is this a good idea?

//extern "C" void __declspec(naked) _cdecl _penter( void )
//{
//    _asm { ret }
//}

//extern "C" void __declspec(naked) _cdecl _pexit( void )
//{
//    _asm { ret }
//}

#endif // PROFILE
