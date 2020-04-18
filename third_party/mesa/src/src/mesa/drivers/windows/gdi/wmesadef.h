#ifndef WMESADEF_H
#define WMESADEF_H

#include <windows.h>

#include "main/context.h"


/**
 * The Windows Mesa rendering context, derived from struct gl_context.
 */
struct wmesa_context {
    struct gl_context           gl_ctx;	        /* The core GL/Mesa context */
    HDC                 hDC;
    COLORREF		clearColorRef;
    HPEN                clearPen;
    HBRUSH              clearBrush;
};


/**
 * Windows framebuffer, derived from gl_framebuffer
 */
struct wmesa_framebuffer
{
    struct gl_framebuffer Base;
    HDC                 hDC;
    int			pixelformat;
    GLuint		ScanWidth;
    int			cColorBits;
    /* back buffer DIB fields */
    HDC                 dib_hDC;
    BITMAPINFO          bmi;
    HBITMAP             hbmDIB;
    HBITMAP             hOldBitmap;
    PBYTE               pbPixels;
    struct wmesa_framebuffer *next;
};

typedef struct wmesa_framebuffer *WMesaFramebuffer;


#endif /* WMESADEF_H */
