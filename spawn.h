#pragma once

#ifndef _SPAWN_H_
#define _SPAWN_H_

class thFrame;
class thProgressDialog;

class SpawnHandler : public wxTimer
{
public:
    SpawnHandler(thFrame *frame);
    ~SpawnHandler();

    thFrame *frame;
    thProgressDialog *progress;
    wxMutex mutex;

    THSIZE bytesWritten, bytesTotal;

    HANDLE hChildStdout; // also includes stderr.  //! todo: separate?
    HANDLE hChildStdin;
    HANDLE hChildProc;
    HANDLE hReadThread, hWriteThread;
    bool bSelOnly;
    DWORD exitCode;
    bool bComplete; // did the child process exit?
    PROCESS_INFORMATION pi;

    enum { BLOCKSIZE = 0x4000 };
    uint8* writeBuffer;

    bool Start(wxString file, wxString args, wxString dir, bool bSelOnly);
    virtual void Notify(); // called when the timer is triggered?

    void SessionReadShellThreadFn();
    void SessionWriteShellThreadFn();
};

#endif //_SPAWN_H_
