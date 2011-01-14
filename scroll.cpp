#include "precomp.h"
#include "scroll.h"
#include "resource.h"
#include "hexwnd.h"

#define new New

#define USE_WS_CHILD

typedef struct {
    HWND hParentWnd;
    bool done;
    HWND hScrollOriginWnd;
    HexWnd *phw;
    int delay;
    double subX, subY;
    bool hScroll, vScroll;
    DWORD nextActionTime;
    bool bMovedAnywhere;
} SCROLLORIGINPARAMS, UNALIGNED *PSCROLLORIGINPARAMS;

static HCURSOR cursors[11] = {0};

void DoScroll(PSCROLLORIGINPARAMS psop)
{
    if (psop->done)
        return;

    POINT pt;
    RECT rcWindow;
    int dx = 0, dy = 0;
    GetCursorPos(&pt);
    GetWindowRect(psop->hScrollOriginWnd, &rcWindow);
    const int x = rcWindow.left + SO_RADIUS;
    const int y = rcWindow.top + SO_RADIUS;
    if (psop->hScroll)
        dx = pt.x - x;
    if (psop->vScroll)
        dy = pt.y - y;
    //if (dx * dx + dy * dy < SO_RADIUS * SO_RADIUS)
    //    dx = dy = 0;

    if (dy > 0)
        dy = max(0, dy - SO_MIDPOINT_RADIUS);
    else
        dy = min(0, dy + SO_MIDPOINT_RADIUS);

    if (dx > 0)
        dx = max(0, dx - SO_MIDPOINT_RADIUS);
    else
        dx = min(0, dx + SO_MIDPOINT_RADIUS);

    int cursor;
    if (dx > 0)
    {
        if (dy > 0)
            cursor = IDC_DR;
        else if (dy < 0)
            cursor = IDC_UR;
        else
            cursor = IDC_R;
    }
    else if (dx < 0)
    {
        if (dy > 0)
            cursor = IDC_DL;
        else if (dy < 0)
            cursor = IDC_UL;
        else
            cursor = IDC_L;
    }
    else
    {
        if (dy > 0)
            cursor = IDC_D;
        else if (dy < 0)
            cursor = IDC_U;
        else
        {
            if (psop->hScroll && psop->vScroll)
                cursor = IDC_UDLR;
            else if (psop->hScroll)
                cursor = IDC_LR;
            else
                cursor = IDC_UD;
        }
    }
    SetCursor(cursors[cursor - IDC_SCROLL1]);

    if (!dx && !dy)
    {
        //psop->delay = 10; // do nothing until the mouse moves
        psop->nextActionTime = timeGetTime() + 10;
        return;
    }

    int mx = abs(dx), my = abs(dy);
    int sx = SGN(dx), sy = SGN(dy);
    int moveX = 0, moveY = 0;

    //psop->delay = 10; //! 10;
    psop->nextActionTime = timeGetTime() + 10;
    psop->subX += sx * pow(1.6, (mx + 19) * .05);
    psop->subY += sy * pow(1.6, (my + 19) * .05);
    //psop->subY += sy * (int)(200.0 * pow(1.4, (my + 19) * .05) - 200);

    moveX = (int)psop->subX / 10; // truncation is intentional
    moveY = (int)psop->subY / 10;

    psop->subX = fmod(psop->subX, 10.0);
    psop->subY = fmod(psop->subY, 10.0);

    if (!moveX && !moveY)
        return;

    // ah, just repaint the whole damn circle.
    pt.x = rcWindow.left;
    pt.y = rcWindow.top;
    ScreenToClient((HWND)psop->phw->GetHWND(), &pt);
    psop->phw->RefreshRect(wxRect(pt.x, pt.y, 2 * SO_RADIUS + 1, 2 * SO_RADIUS + 1), false);

    //PRINTF("x=%3d y=%3d   delay=%d\n", moveX, moveY, psop->delay);
    psop->phw->OnSmoothScroll(moveX, moveY);
    psop->bMovedAnywhere = true;
}

LRESULT WINAPI ScrollOriginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PSCROLLORIGINPARAMS psop = (PSCROLLORIGINPARAMS)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
    case WM_CREATE: {
        psop = (PSCROLLORIGINPARAMS)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (UINT_PTR)psop);
        psop->hScrollOriginWnd = hWnd;
        } break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        DrawIcon(hdc, 0, 0, (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SIZE1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
        EndPaint(hWnd, &ps);
        break;
    case WM_MOUSEMOVE:
        DoScroll(psop);
        break;
    case WM_MBUTTONUP:
        if (!psop->bMovedAnywhere) // Middle button released without leaving origin window
            break;
        // else: button released after moving.  Stop moving and destory the origin window.
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_CAPTURECHANGED:
    case WM_SYSKEYDOWN:
    case WM_KILLFOCUS:
        psop->done = true;
        DestroyWindow(hWnd);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

BOOL CreateScrollOriginWnd(HexWnd *phw, int x, int y)
{
    WNDCLASS wc;
    MSG msg;
    SCROLLORIGINPARAMS sop = { (HWND)phw->GetHWND(), false };
    HRGN hRgn;
    HWND hWnd;
    BOOL rc = false;
    POINT pt = {x, y}; // use this if we can't use WS_CHILD
    DWORD parentStyle = GetWindowLong(sop.hParentWnd, GWL_STYLE);

    sop.phw = phw;
    sop.subX = sop.subY = 0;
    sop.hScroll = !!(parentStyle & WS_HSCROLL);
    sop.vScroll = !!(parentStyle & WS_VSCROLL);
    sop.bMovedAnywhere = false;

    if (!sop.hScroll && !sop.vScroll)
        return true; // can't scroll in any direction

    memset(&wc, 0, sizeof(wc));
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = SO_WNDCLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpfnWndProc = ScrollOriginWndProc;
    wc.cbWndExtra = sizeof(PSCROLLORIGINPARAMS);
    if (!RegisterClass(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PRINTF(_T("RegisterClass() failed.  Code %d\n"), GetLastError());
        return false;
    }

    if (cursors[0] == NULL)
    {
        for (int i = 0; i < 11; i++)
        {
            cursors[i] = (HCURSOR)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_SCROLL1 + i), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
        }
    }

#ifdef USE_WS_CHILD
    x = pt.x;
    y = pt.y;

    //HWND hTmpParent = GetParent(sop.hParentWnd);

    hWnd = CreateWindowEx(0,
        SO_WNDCLASS, _T("Ahed scroll origin window"),
        WS_CHILD, // not visible, we call ShowWindow() later
        x - SO_RADIUS, y - SO_RADIUS,
        SO_RADIUS * 2 + 1, SO_RADIUS * 2 + 1,
        sop.hParentWnd, NULL, wc.hInstance, &sop);

#else
    ClientToScreen(sop.hParentWnd, &pt);
    x = pt.x;
    y = pt.y;

    hWnd = CreateWindowEx(WS_EX_LAYERED,
        SO_WNDCLASS, "Ahed scroll origin window",
        WS_POPUP | WS_VISIBLE,
        x - SO_RADIUS, y - SO_RADIUS,
        SO_RADIUS * 2 + 1, SO_RADIUS * 2 + 1,
        sop.hParentWnd, NULL, wc.hInstance, &sop);
#endif

    if (!hWnd)
    {
        PRINTF(_T("CreateWindowEx() failed.  Code %d\n"), GetLastError());
        goto unregister;
    }

    hRgn = CreateEllipticRgn(0, 0, SO_RADIUS * 2 + 2, SO_RADIUS * 2 + 2);
    SetWindowRgn(hWnd, hRgn, TRUE);

#ifdef USE_WS_CHILD
    ShowWindow(hWnd, SW_SHOWNA);
    SetFocus(hWnd);
#else
    SetLayeredWindowAttributes(hWnd, 0, 180, LWA_ALPHA);
#endif

    SetCapture(hWnd);
    timeBeginPeriod(1);

    HANDLE hProc = GetCurrentProcess();
    //BOOL ret = SetProcessPriorityBoost(hProc, FALSE);
    DWORD dwOldPriority = GetPriorityClass(hProc);
    BOOL ret = SetPriorityClass(hProc, ABOVE_NORMAL_PRIORITY_CLASS); //! WinNT only
    if (!ret)
        PRINTF(_T("Couldn't boost process priority.\n"));

    while (!sop.done)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                ExitProcess(0); //! I hope this never happens.
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        //if (sop.delay)
        //{
        //    Sleep(1);
        //    sop.delay--;
        //}
        if (timeGetTime() < sop.nextActionTime)
            Sleep(1);
        else
            DoScroll(&sop);
    }

    // new scope block to get rid of compiler warning (yeah, it's dirty.)
    //! why do we need this block at all?  2006-08-11
    // Ah, it's here because of unsophisticated painting code, which has returned.
    {
        RECT rcErase = {x - SO_RADIUS, y - SO_RADIUS, x + SO_RADIUS + 1, y + SO_RADIUS + 1};
        InvalidateRect(sop.hParentWnd, &rcErase, FALSE);
        //UpdateWindow(sop.hParentWnd);
    }

    sop.phw->FinishSmoothScroll();

    rc = TRUE;
    timeEndPeriod(1);

    //SetProcessPriorityBoost(hProc, TRUE);
    SetPriorityClass(hProc, dwOldPriority);

unregister:
    UnregisterClass(SO_WNDCLASS, wc.hInstance);
    return rc;
}
