/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef VG_TRANSLATE_H
#define VG_TRANSLATE_H

#include "VG/openvg.h"
#include "vg_context.h"

/*FIXME: we really should be using translate module
 * but pipe_format can't express some of the VG formats
 * (the premultiplied ones) so currently it won't work */

void _vega_pack_rgba_span_float(struct vg_context *ctx,
                                VGuint n, VGfloat rgba[][4],
                                VGImageFormat dstFormat,
                                void *dstAddr);
void _vega_unpack_float_span_rgba(struct vg_context *ctx,
                                  VGuint n,
                                  VGuint offset,
                                  const void * data,
                                  VGImageFormat dataFormat,
                                  VGfloat rgba[][4]);
VGint _vega_size_for_format(VGImageFormat format);

#endif
