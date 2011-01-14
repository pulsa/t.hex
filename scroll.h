#pragma once

#ifndef _SCROLL_H_
#define _SCROLL_H_

#define SO_RADIUS          15   // radius of scroll origin window
#define SO_MIDPOINT_RADIUS 10   // radius of dead zone
#define SO_WNDCLASS _T("AhedScrollOrigin")

class HexWnd;
BOOL CreateScrollOriginWnd(HexWnd *phw, int x, int y);

#endif // _SCROLL_H_