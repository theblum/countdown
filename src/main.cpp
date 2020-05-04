/* ===========================================================================
 * File: main.cpp
 * Date: 01 May 2020
 * Creator: Brian Blumberg <blum@disroot.org>
 * Notice: Copyright (c) 2019 Brian Blumberg. All Rights Reserved.
 * ===========================================================================
 */

#include <stdio.h>
#include <string.h>

#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "defs.h"

global b32 globalRunning;
global b32 globalMapped;
global clockid_t globalClockType;

struct WindowInfo {
    s32 width, height;
    s32 x, y;
};

internal void
getScreenDimensions(Display *display, s32 screen, s32 *width, s32 *height)
{
    s32 numMonitors = 0;
    XRRMonitorInfo *monitorInfo = XRRGetMonitors(display,
                                                 DefaultRootWindow(display),
                                                 True,
                                                 &numMonitors);
    if(numMonitors) {
        *width  = monitorInfo[0].width;
        *height = monitorInfo[0].height;
    } else {
        /* @Note: Xrandr not supported, assume only one monitor */
        *width  = WidthOfScreen(ScreenOfDisplay(display, screen));
        *height = HeightOfScreen(ScreenOfDisplay(display, screen));
    }
}

internal Window
createWindow(Display *display, s32 screen, char *name, WindowInfo *windowInfo)
{
    Window window = 0;

    XVisualInfo visualInfo = {};
    if(XMatchVisualInfo(display, screen, 32, TrueColor, &visualInfo)) {
        XSetWindowAttributes windowAttribs = {};
        windowAttribs.border_pixel = 0x0;
        windowAttribs.colormap = XCreateColormap(display,
                                                 DefaultRootWindow(display),
                                                 visualInfo.visual,
                                                 AllocNone);
        windowAttribs.event_mask = 0;
        windowAttribs.override_redirect = True;
        u64 windowAttribsMask = CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

        window = XCreateWindow(display,
                               DefaultRootWindow(display),
                               windowInfo->x, windowInfo->y,
                               windowInfo->width, windowInfo->height,
                               0,
                               visualInfo.depth,
                               InputOutput,
                               visualInfo.visual,
                               windowAttribsMask,
                               &windowAttribs);

        XTextProperty windowName = {};
        windowName.value = (u8 *)name;
        windowName.encoding = XA_STRING;
        windowName.format = 8;
        windowName.nitems = strlen(name);

        XClassHint classHint = {};
        classHint.res_name = name;
        classHint.res_class = name;

        XSizeHints sizeHints = {};
        sizeHints.flags = PSize | PMinSize;
        sizeHints.width = sizeHints.base_width = sizeHints.min_width
            = sizeHints.max_width = windowInfo->width;
        sizeHints.height = sizeHints.base_height = sizeHints.min_height
            = sizeHints.max_height = windowInfo->height;

        XSetWMProperties(display,
                         window,
                         &windowName,
                         0, 0, 0, 
                         &sizeHints,
                         0,
                         &classHint);

        char *atomNames[] = {
            "_NET_WM_WINDOW_TYPE",
            "_NET_WM_WINDOW_TYPE_DOCK",
        };
        Atom atoms[2];
        XInternAtoms(display,
                     atomNames,
                     2,
                     False,
                     atoms);
        XTextProperty windowType = {};
        windowType.value = (u8 *)(atoms + 1);
        windowType.encoding = XA_ATOM;
        windowType.format = 32;
        windowType.nitems = 1;
        XSetTextProperty(display, window, &windowType, atoms[0]);
    } else {
        /* @Todo: Error handling -- no matching visual found */
    }

    return window;
}

internal void
fadeIn(Display *display, Window window, WindowInfo *windowInfo, GC gc)
{
    for(s32 i = 0x00; i < 0xff; ++i) {
        XGCValues gcValues = {};
        gcValues.foreground = (u32)(i << 24);
        XChangeGC(display, gc, GCForeground, &gcValues);
        XFillRectangle(display,
                       window,
                       gc,
                       0, 0,
                       windowInfo->width, windowInfo->height);
        XFlush(display);

        struct timespec quickSleep = {};
        quickSleep.tv_nsec = (s64)(5*MILLISECS_TO_NANOSECS);
        clock_nanosleep(globalClockType, 0, &quickSleep, 0);
    }
}

internal void
fadeOut(Display *display, Window window, WindowInfo *windowInfo, GC gc)
{
    for(s32 i = 0xff-1; i >= 0x00; --i) {
        XGCValues gcValues = {};
        gcValues.foreground = (u32)(i << 24);
        XChangeGC(display, gc, GCForeground, &gcValues);
        XFillRectangle(display,
                       window,
                       gc,
                       0, 0,
                       windowInfo->width, windowInfo->height);
        XFlush(display);

        struct timespec quickSleep = {};
        quickSleep.tv_nsec = (s64)(5*MILLISECS_TO_NANOSECS);
        clock_nanosleep(globalClockType, 0, &quickSleep, 0);
    }
}

internal void
handleEvents(Display *display, Window window, WindowInfo *windowInfo, GC gc)
{
    s32 lastEventType = 0;
    XEvent event = {};

    while(XPending(display)) {
        XNextEvent(display, &event);
        switch(event.type) {
            case KeyRelease: {
                if((lastEventType != KeyPress) && (lastEventType != KeyRelease)) {
                    XKeyEvent *kevent = (XKeyEvent *)&event;
                    KeySym key = XLookupKeysym(kevent, 0);
                    if((key == XK_Tab) && (~(Mod1Mask ^ ~kevent->state) == 0)) {
                        if(globalMapped) {
                            fadeOut(display, window, windowInfo, gc);
                            XUnmapWindow(display, window);
                            globalMapped = false;
                        } else {
                            XMapWindow(display, window);
                            fadeIn(display, window, windowInfo, gc);
                            globalMapped = true;
                        }
                    } else if((key == XK_Tab) && (~((Mod1Mask|ShiftMask) ^ ~kevent->state) == 0)) {
                        if(globalMapped) fadeOut(display, window, windowInfo, gc);
                        globalRunning = false;
                    }
                }
            } break;

            default: {
                /* @Note: do nothing */
            } break;
        }

        lastEventType = event.type;
    }
}

internal void
drawGradient(XImage *ximage)
{

}

internal void
blitImage(Display *display, Window window, WindowInfo *windowInfo, XImage *ximage)
{

}

internal f32
getElapsedMSPF(struct timespec *startClock, struct timespec *endClock)
{
    f32 result =
        ((f32)(endClock->tv_sec - startClock->tv_sec)*SECS_TO_MILLISECS) +
        ((f32)(endClock->tv_nsec - startClock->tv_nsec)*NANOSECS_TO_MILLISECS);
    return result;
}

int
main(int argc, char **argv)
{
    Display *display = XOpenDisplay(0);
    if(display) {
        s32 screen = DefaultScreen(display);
        s32 screenWidth = 0;
        s32 screenHeight = 0;
        getScreenDimensions(display, screen, &screenWidth, &screenHeight);

        WindowInfo windowInfo = {};
        windowInfo.width = screenWidth;
        windowInfo.height = (s32)(screenHeight * 0.2f);
        windowInfo.x = 0;
        windowInfo.y = screenHeight - windowInfo.height;

        Window window = createWindow(display, screen, PROGRAM_NAME, &windowInfo);
        if(window) {
            globalClockType = CLOCK_MONOTONIC;

            XGrabKey(display,
                     XKeysymToKeycode(display, XK_Tab),
                     Mod1Mask,
                     DefaultRootWindow(display),
                     True,
                     GrabModeAsync,
                     GrabModeAsync);

            XGrabKey(display,
                     XKeysymToKeycode(display, XK_Tab),
                     Mod1Mask | ShiftMask,
                     DefaultRootWindow(display),
                     True,
                     GrabModeAsync,
                     GrabModeAsync);

            XImage *ximage = 0;// createXImage(display, &visualInfo);

            GC gc = XCreateGC(display, window, 0, 0);
            XMapWindow(display, window);
            fadeIn(display, window, &windowInfo, gc);
            globalMapped = true;

            XSync(display, False);

            struct timespec startClock = {};
            struct timespec endClock = {};
            f32 targetMSPF = 1000.0f / 10.0f;
            f32 elapsedMSPF = 0.0f;
            clock_gettime(globalClockType, &startClock);

            globalRunning = true;
            while(globalRunning) {
                handleEvents(display, window, &windowInfo, gc);

                drawGradient(ximage);

                clock_gettime(globalClockType, &endClock);
                elapsedMSPF = getElapsedMSPF(&startClock, &endClock);
                if(targetMSPF - elapsedMSPF > F32_EPSILON) {
                    while(targetMSPF - elapsedMSPF > F32_EPSILON) {
                        struct timespec requestedSleep = {};
                        requestedSleep.tv_nsec =
                            (u64)((targetMSPF - elapsedMSPF)*MILLISECS_TO_NANOSECS);
                        clock_nanosleep(globalClockType, 0, &requestedSleep, 0);
                        clock_gettime(globalClockType, &endClock);
                        elapsedMSPF = getElapsedMSPF(&startClock, &endClock);
                    }
                } else {
                    /* @Todo: Frame running behind... */
                }
                startClock = endClock;
                // printf("MSPF: %.3f, FPS: %.3f\n", elapsedMSPF, 1000.0f / elapsedMSPF);

                blitImage(display, window, &windowInfo, ximage);
            }

            XFreeGC(display, gc);
            XDestroyWindow(display, window);
        }

        XCloseDisplay(display);
    } else {
        /* @Todo: Error handling -- couldn't open display */
    }

    return 0;
}
