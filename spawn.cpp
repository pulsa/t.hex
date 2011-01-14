#include "precomp.h"
#include "thex.h"
#include "thframe.h"
#include "spawn.h"
#include "hexwnd.h"
#include "hexdoc.h"

#define new New

SpawnHandler::SpawnHandler(thFrame *frame)
{
    this->frame = frame;
    writeBuffer = new uint8[BLOCKSIZE];
}

SpawnHandler::~SpawnHandler()
{
    delete [] writeBuffer;
}

static VOID SessionReadShellThreadFn(LPVOID param)
{
    ((SpawnHandler*)param)->SessionReadShellThreadFn();
}

static VOID SessionWriteShellThreadFn(LPVOID param)
{
    ((SpawnHandler*)param)->SessionWriteShellThreadFn();
}


// **********************************************************************
// SessionReadShellThreadFn
//
// The read thread procedure. Reads from the pipe connected to the shell
// process, writes to console output.
//

void SpawnHandler::SessionReadShellThreadFn()
{
    HANDLE hMyStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    const int BUFFER_SIZE = 8192;
    BYTE    Buffer[BUFFER_SIZE];
    //BYTE    Buffer2[BUFFER_SIZE * 2];
    BYTE *Buffer2 = Buffer;
    DWORD   BytesRead;
    bool bNeedCR = false;

    // This bogus peek is here because win32 won't let me close the pipe if it is
    // waiting for input on a read.
    while (PeekNamedPipe(hChildStdout, Buffer, sizeof(Buffer), 
                    &BytesRead, NULL, NULL)) 
    {
        DWORD /*BufferCnt,*/ BytesToWrite;
        BYTE PrevChar = 0;

        if (BytesRead == 0)
        {
            Sleep(50);
            continue;
        }

        ReadFile(hChildStdout, Buffer, sizeof(Buffer), &BytesRead, NULL);

        //
        // Process the data we got from the shell:  replace any naked LF's
        // with CR-LF pairs.
        // (not needed for true console output)
        //for (BufferCnt = 0, BytesToWrite = 0; BufferCnt < BytesRead; BufferCnt++) {
        //    if (Buffer[BufferCnt] == '\n' && PrevChar != '\r')
        //        Buffer2[BytesToWrite++] = '\r';
        //    PrevChar = Buffer2[BytesToWrite++] = Buffer[BufferCnt];
        //}
        BytesToWrite = BytesRead;

        //! Someday we're going to have to deal with the GUI thread here.
        // worry about that later.

        WriteFile(hMyStdout, Buffer2, BytesToWrite, &BytesToWrite, NULL);
        if (Buffer2[BytesToWrite - 1] != '\n')
            bNeedCR = true;
    }

    if (bNeedCR)
        printf("\n");

    if (GetLastError() != ERROR_BROKEN_PIPE)
        printf("SessionReadShellThreadFn exited, error = %d\n", GetLastError());

    mutex.Lock();
    if (hChildStdout)
        CloseHandle(hChildStdout);
    hChildStdout = NULL;
    mutex.Unlock();

	ExitThread(0);
}

// **********************************************************************
// SessionWriteShellThreadFn
//
// The write thread procedure. Reads from the HexDoc, writes to the pipe.
//
//

void SpawnHandler::SessionWriteShellThreadFn()
{
    HANDLE hMyStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD dummy;
    THSIZE offset;
    if (bSelOnly)
        offset = frame->GetHexWnd()->GetSelection().GetFirst();
    else
        offset = 0;

    while (bytesWritten < bytesTotal)
    {
        mutex.Lock();
        THSIZE block = wxMin(bytesTotal - bytesWritten, BLOCKSIZE);
        frame->GetHexWnd()->doc->Read(offset, block, writeBuffer);
        PRINTF(_T("Writing %I64X bytes at offset %I64X... "), block, bytesWritten);
        mutex.Unlock();
        if (!hChildStdin)
            break;

        BOOL rc = WriteFile(hChildStdin, writeBuffer, block, &dummy, NULL);
        if (rc)
        {
            PRINTF(_T("OK.\n"));
        }
        else
        {
            PRINTF(_T("Failed.\n"));
        }

        // See if the process rejected our input and quit early.
        //! A process could close its stdin and keep running.
        //  Do we need to worry about weird stuff like that?
        //  Or if it leaves stdin open and keeps running?  What do we do?
        //if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &dummy, NULL))
        //    break;
        GetExitCodeProcess(hChildProc, &dummy);
        if (dummy != STILL_ACTIVE)
        {
            PRINTF(_T("Child process didn't want input (writing 0x%I64X bytes at offset 0x%I64X.)\n"), block, bytesWritten);
            break;
        }

        mutex.Lock();
        offset += block;
        bytesWritten += block;
        mutex.Unlock();
    }

    mutex.Lock();
    if (hChildStdin)
        CloseHandle(hChildStdin);
    hChildStdin = 0;
    mutex.Unlock();

    ExitThread(0);
}

bool SpawnHandler::Start(wxString file, wxString args, wxString dir, bool bSelOnly)
{
    STARTUPINFO si;
    BOOL Result;
    HANDLE hMyProc = GetCurrentProcess();

    this->bSelOnly = bSelOnly;
    this->bComplete = false;

    // Find the full path of the file to execute.
    if (!wxFile::Exists(file))
    {
        wxPathList pathList;
        pathList.AddEnvList(_T("PATH"));
        pathList.Add(dir);
        wxString tmpFile = pathList.FindAbsoluteValidPath(file);
        if (!tmpFile)
        {
            wxMessageBox(_T("Couldn't find ") + file);
            return false;
        }
        file = tmpFile;
    }

    //
    // Initialize process startup info
    //
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    //si.lpReserved = NULL;
    //si.lpTitle = NULL;
    //si.lpDesktop = NULL;
    //si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    //si.wShowWindow = 0;
    //si.lpReserved2 = NULL;
    //si.cbReserved2 = 0;

    si.dwFlags = STARTF_USESTDHANDLES;

    // Create the I/O pipes for the shell.
    // Child process must not inherit the write handle to its stdin.
    // Otherwise ReadFile(stdin) never returns 0, and the input never ends.
    HANDLE hNoInheritStdin;
    HANDLE hNoInheritStdout; // might as well keep this clean too
    if (!CreatePipe(&hNoInheritStdin, &hChildStdin, NULL, 0) ||
        !CreatePipe(&hChildStdout, &hNoInheritStdout, NULL, 0))
    {
        printf("Failed to create shell pipes, error = %d\n", GetLastError());
        goto Failure;
    }

    DuplicateHandle(hMyProc, hNoInheritStdin,
                    hMyProc, &si.hStdInput,
                    0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    DuplicateHandle(hMyProc, hNoInheritStdout,
                    hMyProc, &si.hStdOutput,
                    0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    DuplicateHandle(hMyProc, si.hStdOutput,
                    hMyProc, &si.hStdError,
                    0, TRUE, DUPLICATE_SAME_ACCESS);

    // start read thread for stdout/stderr
    DWORD ThreadId;
    hReadThread = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE) ::SessionReadShellThreadFn, 
        this, 0, &ThreadId);

    //! todo: separate write thread in case child proc hangs

    if (args.Len())
    {
        // argv[0] should be the EXE file
        args = _T('\"') + file + _T("\" ") + args;
    }

    LPCTSTR lpfile = file.Len() ? (LPCTSTR)file.c_str() : NULL;
    LPTSTR lpargs = args.Len() ? _tcsdup((LPCTSTR)args.c_str()) : NULL;
    LPCTSTR lpdir = dir.Len() ? (LPCTSTR)dir.c_str() : NULL;

#ifdef _DEBUG
    {
    PRINTF(_T("\n"));
    PRINTF(_T("*** Starting process ***\n"));
    PRINTF(_T("  Source: %s"), frame->GetHexWnd()->doc->info.c_str());
    Selection sel = frame->GetHexWnd()->GetSelection();
    if (bSelOnly)
        PRINTF(_T(", bytes %I64X - %I64X\n"), sel.GetFirst(), sel.GetLast());
    else
        PRINTF(_T(" (whole file)\n"));
    PRINTF(_T("  Size: %I64X bytes\n"), sel.GetSize());
    PRINTF(_T("  File: %s\n"), lpfile);
    PRINTF(_T("  Command line: %s\n"), lpargs);
    PRINTF(_T("  Directory: %s\n"), lpdir);
    }
#endif

    Result = CreateProcess(lpfile, lpargs, NULL, NULL, TRUE,
        //CREATE_NO_WINDOW |
        //CREATE_NEW_CONSOLE,
        CREATE_SUSPENDED |
        CREATE_NEW_PROCESS_GROUP |  // so we can send it a Ctrl+C event
        0,
        NULL, lpdir, &si, &pi);
    DWORD dwCreateProcessError = GetLastError();

    ResumeThread(pi.hThread); //! this is silly
    hChildProc = pi.hProcess;

    hWriteThread = CreateThread(NULL, 0,
       (LPTHREAD_START_ROUTINE) ::SessionWriteShellThreadFn,
       this, 0, &ThreadId);

    // We're done with these now.
    CloseHandle(si.hStdInput);
    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);
    CloseHandle(pi.hThread);

    if (Result)
    {
        if (bSelOnly)
            bytesTotal = frame->GetHexWnd()->GetSelection().GetSize();
        else
            bytesTotal = frame->GetHexWnd()->doc->GetSize();
        bytesWritten = 0;
        wxTimer::Start(500); // semi-auto, fire twice a second

        progress = new thProgressDialog(bytesTotal, frame, _T("Writing"), _T("Writing to pipe"));
        //progress->SetUpdateInterval(100);

        // I/O thread will close hReadPipe (stdout) when it finishes.
    }
    else
    {
        PRINTF(_T("CreateProcess('%s', '%s', '%s') failed.  Code %d\n"),
           file.c_str(), args.c_str(), dir.c_str(), dwCreateProcessError);
    }

Failure:
    ;

    return true;
}

void SpawnHandler::Notify()
{
    wxMutexLocker lock(mutex);

    // See if either thread has stopped.
    HANDLE handles[2] = {hWriteThread, hReadThread};
    DWORD rc = WaitForMultipleObjects(2, handles, FALSE, 0);

    if (bytesWritten == bytesTotal ||
        !progress->Update(bytesWritten) ||
        (rc >= WAIT_OBJECT_0 && rc < WAIT_OBJECT_0 + 2))
    {
        Stop();
        delete progress;
        progress = NULL;

        CloseHandle(hReadThread);
        CloseHandle(hWriteThread);

        // We can send an event to the child process like this:
        //GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pi.dwProcessId);

        // Give child proc a chance to finish, in case it does something weird.
        //! I don't know if we need this, but it doesn't hurt.
        DWORD wait = WaitForSingleObject(hChildProc, 200);
        if (wait == WAIT_OBJECT_0)
        {
            if (GetExitCodeProcess(hChildProc, &exitCode))
                bComplete = true;
            else
                PRINTF(_T("GetExitCodeProcess: error code %d\n"), GetLastError());
        }

        CloseHandle(hChildProc);

        //! clean up pipes.  Shouldn't have to do this.
        // Debugger may complain.
        if (hChildStdout)
            CloseHandle(hChildStdout);
        if (hChildStdin)
            CloseHandle(hChildStdin);
        hChildStdout = hChildStdin = 0;

        frame->OnPipeComplete();
    }
}
