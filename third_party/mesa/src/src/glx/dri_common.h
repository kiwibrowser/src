/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifndef _DRI_COMMON_H
#define _DRI_COMMON_H

#include <GL/internal/dri_interface.h>
#include <stdbool.h>

typedef struct __GLXDRIconfigPrivateRec __GLXDRIconfigPrivate;

struct __GLXDRIconfigPrivateRec
{
   struct glx_config base;
   const __DRIconfig *driConfig;
};

extern struct glx_config *driConvertConfigs(const __DRIcoreExtension * core,
                                           struct glx_config * modes,
                                           const __DRIconfig ** configs);

extern void driDestroyConfigs(const __DRIconfig **configs);

extern __GLXDRIdrawable *
driFetchDrawable(struct glx_context *gc, GLXDrawable glxDrawable);

extern void
driReleaseDrawables(struct glx_context *gc);

extern const __DRIsystemTimeExtension systemTimeExtension;

extern void InfoMessageF(const char *f, ...);

extern void ErrorMessageF(const char *f, ...);

extern void CriticalErrorMessageF(const char *f, ...);

extern void *driOpenDriver(const char *driverName);

extern bool
dri2_convert_glx_attribs(unsigned num_attribs, const uint32_t *attribs,
			 unsigned *major_ver, unsigned *minor_ver,
			 uint32_t *flags, unsigned *api, int *reset,
			 unsigned *error);

#endif /* _DRI_COMMON_H */
