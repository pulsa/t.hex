#pragma once

#ifndef _THEX_H_
#define _THEX_H_

#define APPNAME _T("Tyrannosaurus Hex")

//#include "qstring.h"
//#include "atimer.h"

class HexWnd;
class thFrame;

class thApp : public wxApp
{
public:
    //thApp();
    bool OnInit();
    void DoEvents()
    {
        while (Pending())
            Dispatch();
    }

    // These are here because I didn't allow exceptions from config.vc.  Right?
    virtual bool OnExceptionInMainLoop() { return false; } //! right return code?
    //virtual void HandleEvent(wxEvtHandler *handler,
    //                         wxEventFunction func,
    //                         wxEvent& event) const
    //{ }
};

DECLARE_APP(thApp)

#include "defs.h"

//! quick hack for switching versions
//! wxRect::Inside() was changed to Contains(), with the old method deprecated.
#if wxCHECK_VERSION(2, 7, 0)
    #define InsideContains Contains
#else
    #define InsideContains Inside
#endif

#endif // _THEX_H_
