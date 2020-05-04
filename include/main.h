#if !defined(MAIN_H)
#define MAIN_H
/* ===========================================================================
 * File: main.h
 * Date: 04 May 2020
 * Creator: Brian Blumberg <blum@disroot.org>
 * Notice: Copyright (c) 2020 Brian Blumberg. All Rights Reserved.
 * ===========================================================================
 */

struct WindowInfo {
    s32 width, height;
    s32 x, y;
};

struct X11State {
    Display *display;
    s32 screen;
    Window window;
    XImage *ximage;
    Pixmap pixmap;
    WindowInfo *windowInfo;
    XVisualInfo *visualInfo;
    GC gc;
};

#endif
