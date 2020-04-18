/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef D3D1X_PRIVATE_H_
#define D3D1X_PRIVATE_H_

#include <algorithm>
#include <vector>
#include <string>
#include <float.h>

#include "dxbc.h"
#include "sm4.h"
#include "sm4_to_tgsi.h"

#include "d3d1xstutil.h"

#include <d3d11.h>
#include <d3d11shader.h>

extern "C"
{
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_caps.h"
#include "util/u_debug.h"
#include "os/os_thread.h"
}

#include "galliumdxgi.h"
#include "galliumd3d10_1.h"
#include "galliumd3d11.h"

#ifdef CHECK
#define invalid(x) unlikely(x)
#else
#define invalid(x) (0)
#endif

#define D3D10_STAGE_VS 0
#define D3D10_STAGE_PS 1
#define D3D10_STAGE_GS 2
#define D3D10_STAGES 3

#define D3D11_STAGE_VS 0
#define D3D11_STAGE_PS 1
#define D3D11_STAGE_GS 2
#define D3D11_STAGE_HS 3
#define D3D11_STAGE_DS 4
#define D3D11_STAGE_CS 5
#define D3D11_STAGES 6

#define D3D10_BLEND_COUNT 20
#define D3D11_BLEND_COUNT 20
extern unsigned d3d11_to_pipe_blend[D3D11_BLEND_COUNT];

#define D3D11_USAGE_COUNT 4
extern unsigned d3d11_to_pipe_usage[D3D11_USAGE_COUNT];

#define D3D10_STENCIL_OP_COUNT 9
#define D3D11_STENCIL_OP_COUNT 9
extern unsigned d3d11_to_pipe_stencil_op[D3D11_STENCIL_OP_COUNT];

#define D3D11_TEXTURE_ADDRESS_COUNT 6
extern unsigned d3d11_to_pipe_wrap[D3D11_TEXTURE_ADDRESS_COUNT];

#define D3D11_QUERY_COUNT 16
extern unsigned d3d11_to_pipe_query[D3D11_QUERY_COUNT];
extern unsigned d3d11_query_size[D3D11_QUERY_COUNT];

#endif /* D3D1X_H_ */
