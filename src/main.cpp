/* ===========================================================================
 * File: main.cpp
 * Date: 01 May 2020
 * Creator: Brian Blumberg <blum@disroot.org>
 * Notice: Copyright (c) 2019 Brian Blumberg. All Rights Reserved.
 * ===========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>

#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "defs.h"
#include "main.h"

internal void
getScreenDimensions(X11State *x11, s32 *width, s32 *height)
{
    s32 numMonitors = 0;
    XRRMonitorInfo *monitorInfo = XRRGetMonitors(x11->display,
                                                 DefaultRootWindow(x11->display),
                                                 True,
                                                 &numMonitors);
    if(numMonitors) {
        *width  = monitorInfo[0].width;
        *height = monitorInfo[0].height;
    } else {
        /* @Note: Xrandr not supported, assume only one monitor */
        *width  = WidthOfScreen(ScreenOfDisplay(x11->display, x11->screen));
        *height = HeightOfScreen(ScreenOfDisplay(x11->display, x11->screen));
    }
}

internal Window
createWindow(X11State *x11, char *name)
{
    assert(x11->visualInfo);

    Window window = 0;

    if(XMatchVisualInfo(x11->display, x11->screen, 32, TrueColor, x11->visualInfo)) {
        XSetWindowAttributes windowAttribs = {};
        windowAttribs.background_pixel = 0x0;
        windowAttribs.border_pixel = 0x0;
        windowAttribs.colormap = XCreateColormap(x11->display,
                                                 DefaultRootWindow(x11->display),
                                                 x11->visualInfo->visual,
                                                 AllocNone);
        windowAttribs.override_redirect = True;
        /* @Note: We don't need to request KeyEvents as the XGrabKey function automatically sends
         * KeyPress and KeyRelease events to us.
         */
        u64 windowAttribsMask = CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect;

        window = XCreateWindow(x11->display,
                               DefaultRootWindow(x11->display),
                               x11->windowInfo->x, x11->windowInfo->y,
                               x11->windowInfo->width, x11->windowInfo->height,
                               0,
                               x11->visualInfo->depth,
                               InputOutput,
                               x11->visualInfo->visual,
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
            = sizeHints.max_width = x11->windowInfo->width;
        sizeHints.height = sizeHints.base_height = sizeHints.min_height
            = sizeHints.max_height = x11->windowInfo->height;

        XSetWMProperties(x11->display,
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
        XInternAtoms(x11->display,
                     atomNames,
                     2,
                     False,
                     atoms);
        XTextProperty windowType = {};
        windowType.value = (u8 *)(atoms + 1);
        windowType.encoding = XA_ATOM;
        windowType.format = 32;
        windowType.nitems = 1;
        XSetTextProperty(x11->display, window, &windowType, atoms[0]);
    } else {
        /* @Todo: Error handling -- no matching visual found */
    }

    return window;
}

internal XImage *
createXImage(X11State *x11)
{
    XImage *ximage = 0;

    u8 *data = (u8 *)malloc(x11->windowInfo->width * x11->windowInfo->height * sizeof(u32));
    if(data) {
        ximage = XCreateImage(x11->display,
                              x11->visualInfo->visual,
                              x11->visualInfo->depth,
                              ZPixmap,
                              0,
                              (char *)data,
                              x11->windowInfo->width,
                              x11->windowInfo->height,
                              sizeof(u32)*8,
                              0);
    }

    return ximage;
}

internal void
drawGradient(X11State *x11, u32 color, f32 delta, s32 fade)
{
    /* @Todo: Optimize all this! */

    /* @Todo: This gradient doesn't look good on my monitor.  It's not very smooth. */
    f32 halfPI = M_PI / 2.0f;
    s32 width = x11->ximage->width;
    s32 height = x11->ximage->height;
    u32 *data = (u32 *)x11->ximage->data;
    for(s32 y = 0; y < height; ++y) {
        for(s32 x = 0; x < width; ++x) {
            f32 alphaMult = CLAMP(1.0 - ((f32)x/width) * ((f32)y/height));
            u32 *pixel = &data[y*width + x];
            u8 alpha = (u8)(((color & 0xff000000) >> 24) * alphaMult);
            u8 red   = (u8)(((color & 0x00ff0000) >> 16) * alphaMult);
            u8 green = (u8)(((color & 0x0000ff00) >> 8)  * alphaMult);
            u8 blue  = (u8)(((color & 0x000000ff) >> 0)  * alphaMult);
            *pixel = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
    }

    if(fade != 0) {
        CLAMP(delta);
        f32 alphaMult = 0.0f;
        if(fade > 0) {
            /* @Note: sin(0 ... pi/2) == 0 ... 1 */
            alphaMult = sin(delta * halfPI);
        } else {
            /* @Note: sin(pi/2 ... pi) == 1 ... 0 */
            alphaMult = sin((delta * halfPI) + halfPI);
        }

        for(s32 y = 0; y < height; ++y) {
            u32 *pixel = 0;
            for(s32 x = 0; x < width; ++x) {
                pixel = &data[y*width + x];
                u8 alpha = (u8)(((*pixel & 0xff000000) >> 24) * alphaMult);
                u8 red   = (u8)(((*pixel & 0x00ff0000) >> 16) * alphaMult);
                u8 green = (u8)(((*pixel & 0x0000ff00) >> 8)  * alphaMult);
                u8 blue  = (u8)(((*pixel & 0x000000ff) >> 0)  * alphaMult);
                *pixel = (alpha << 24) | (red << 16) | (green << 8) | blue;
            }
        }
    }
}

#if 0
internal void
fade(Display *display, Window window, WindowInfo *windowInfo, GC gc, s32 start, s32 end, s32 step)
{
    for(s32 i = start; i != end; i += step) {
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
        clock_nanosleep(globalClockType, 0, &quickSleep,0);
    }
}

internal void
fadeIn(Display *display, Window window, WindowInfo *windowInfo, GC gc)
{
    fade(display, window, windowInfo, gc, 0x00, 0xff, 1);
}

internal void
fadeOut(Display *display, Window window, WindowInfo *windowInfo, GC gc)
{
    fade(display, window, windowInfo, gc, 0xff, 0x00, -1);
}
#endif

internal void
handleEvents(X11State *x11, s32 *fade, b32 *shouldClose)
{
    XEvent event = {};

    while(XPending(x11->display)) {
        XNextEvent(x11->display, &event);
        switch(event.type) {
            case KeyRelease: {
                XKeyEvent *kevent = (XKeyEvent *)&event;
                KeySym key = XLookupKeysym(kevent, 0);
                if((key == XK_Tab) && (~(Mod1Mask ^ ~kevent->state) == 0)) {
                    XWindowAttributes windowAttribs = {};
                    XGetWindowAttributes(x11->display, x11->window, &windowAttribs);
                    if(windowAttribs.map_state == IsUnmapped) {
                        XMapWindow(x11->display, x11->window);
                        *fade = 1;
                    } else {
                        *fade = -1;
                    }
                } else if((key == XK_Tab) && (~((Mod1Mask|ShiftMask) ^ ~kevent->state) == 0)) {
                    *fade = -1;
                    *shouldClose = true;
                }
            } break;

            default: {
                /* @Note: do nothing */
            } break;
        }
    }
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
    X11State x11 = {};
    x11.display = XOpenDisplay(0);
    if(x11.display) {
        x11.screen = DefaultScreen(x11.display);
        s32 screenWidth = 0;
        s32 screenHeight = 0;
        getScreenDimensions(&x11, &screenWidth, &screenHeight);

        WindowInfo windowInfo = {};
        windowInfo.width = screenWidth;
        windowInfo.height = (s32)(screenHeight * 0.2f);
        windowInfo.x = 0;
        windowInfo.y = screenHeight - windowInfo.height;
        x11.windowInfo = &windowInfo;

        XVisualInfo visualInfo = {};
        x11.visualInfo = &visualInfo;
        x11.window = createWindow(&x11, PROGRAM_NAME);
        if(x11.window) {
            XGrabKey(x11.display,
                     XKeysymToKeycode(x11.display, XK_Tab),
                     Mod1Mask,
                     DefaultRootWindow(x11.display),
                     True,
                     GrabModeAsync,
                     GrabModeAsync);

            XGrabKey(x11.display,
                     XKeysymToKeycode(x11.display, XK_Tab),
                     Mod1Mask | ShiftMask,
                     DefaultRootWindow(x11.display),
                     True,
                     GrabModeAsync,
                     GrabModeAsync);

            x11.ximage = createXImage(&x11);
            if(x11.ximage) {
                x11.pixmap = XCreatePixmap(x11.display,
                                           x11.window,
                                           x11.windowInfo->width,
                                           x11.windowInfo->height,
                                           x11.visualInfo->depth);

                x11.gc = XCreateGC(x11.display, x11.window, 0, 0);

                XMapWindow(x11.display, x11.window);

                XSync(x11.display, False);

                struct timespec startClock = {};
                struct timespec endClock = {};
                clockid_t clockType = CLOCK_MONOTONIC;
                f32 targetMSPF = 1000.0f / 30.0f;
                f32 elapsedMSPF = 0.0f;
                clock_gettime(clockType, &startClock);

                s32 fade = 1;
                f32 delta = 0.0f;
                b32 shouldClose = false;
                b32 running = true;
                while(running) {
                    handleEvents(&x11, &fade, &shouldClose);

                    if(fade != 0) delta += 0.5f * (elapsedMSPF/1000.0f);
                    /* @Todo: Separate out the gradient and fade code so we can begin to enter
                     * text in above the gradient.
                     */
                    drawGradient(&x11, 0xff222222, delta, fade);
                    if(delta >= 1.0f) {
                        delta = 0.0f;
                        if(fade < 0) XUnmapWindow(x11.display, x11.window);
                        if(shouldClose) running = false;
                        fade = 0;
                    }

                    clock_gettime(clockType, &endClock);
                    elapsedMSPF = getElapsedMSPF(&startClock, &endClock);
                    if(targetMSPF - elapsedMSPF > F32_EPSILON) {
                        while(targetMSPF - elapsedMSPF > F32_EPSILON) {
                            struct timespec requestedSleep = {};
                            requestedSleep.tv_nsec =
                                (u64)((targetMSPF - elapsedMSPF)*MILLISECS_TO_NANOSECS);
                            clock_nanosleep(clockType, 0, &requestedSleep, 0);
                            clock_gettime(clockType, &endClock);
                            elapsedMSPF = getElapsedMSPF(&startClock, &endClock);
                        }
                    } else {
                        /* @Todo: Frame running behind... */
                    }
                    startClock = endClock;

#if 0
                    printf("MSPF: %.3f, FPS: %.3f\n", elapsedMSPF, 1000.0f / elapsedMSPF);
#endif

                    XPutImage(x11.display,
                              x11.pixmap,
                              x11.gc,
                              x11.ximage,
                              0, 0,
                              0, 0,
                              x11.ximage->width, x11.ximage->height);

                    XCopyArea(x11.display,
                              x11.pixmap,
                              x11.window,
                              x11.gc,
                              0, 0,
                              x11.windowInfo->width, x11.windowInfo->height,
                              0, 0);
                }

                XFreePixmap(x11.display, x11.pixmap);
                XFreeGC(x11.display, x11.gc);
                XDestroyImage(x11.ximage);
            } else {
                /* @Todo: Error handling -- couldn't create ximage  */
            }

            XDestroyWindow(x11.display, x11.window);
        }

        XCloseDisplay(x11.display);
    } else {
        /* @Todo: Error handling -- couldn't open display */
    }

    return 0;
}
