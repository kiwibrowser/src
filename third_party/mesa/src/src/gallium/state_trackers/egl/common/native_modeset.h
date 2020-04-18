/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _NATIVE_MODESET_H_
#define _NATIVE_MODESET_H_

#include "pipe/p_compiler.h"

struct native_display;
struct native_surface;
struct native_config;

struct native_connector {
   int dummy;
};

struct native_mode {
   const char *desc;
   int width, height;
   int refresh_rate; /* HZ * 1000 */
};

/**
 * Mode setting interface of the native display.  It exposes the mode setting
 * capabilities of the underlying graphics hardware.
 */
struct native_display_modeset {
   /**
    * Get the available physical connectors and the number of CRTCs.
    */
   const struct native_connector **(*get_connectors)(struct native_display *ndpy,
                                                     int *num_connectors,
                                                     int *num_crtcs);

   /**
    * Get the current supported modes of a connector.  The returned modes may
    * change every time this function is called and those from previous calls
    * might become invalid.
    */
   const struct native_mode **(*get_modes)(struct native_display *ndpy,
                                           const struct native_connector *nconn,
                                           int *num_modes);

   /**
    * Create a scan-out surface.  Required unless no config has scanout_bit
    * set.
    */
   struct native_surface *(*create_scanout_surface)(struct native_display *ndpy,
                                                    const struct native_config *nconf,
                                                    uint width, uint height);

   /**
    * Program the CRTC to output the surface to the given connectors with the
    * given mode.  When surface is not given, the CRTC is disabled.
    *
    * This interface does not export a way to query capabilities of the CRTCs.
    * The native display usually needs to dynamically map the index to a CRTC
    * that supports the given connectors.
    */
   boolean (*program)(struct native_display *ndpy, int crtc_idx,
                      struct native_surface *nsurf, uint x, uint y,
                      const struct native_connector **nconns, int num_nconns,
                      const struct native_mode *nmode);
};

#endif /* _NATIVE_MODESET_H_ */
