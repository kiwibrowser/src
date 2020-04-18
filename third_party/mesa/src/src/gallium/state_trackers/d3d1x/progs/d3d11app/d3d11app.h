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

#ifndef D3D11APP_H
#define D3D11APP_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <d3d11.h>
#include <assert.h>
#include <stdio.h>
#include <float.h>

#define ensure(x) do {HRESULT __hr = (x); if(!SUCCEEDED(__hr)) {fprintf(stderr, "COM error %08x\n", __hr); abort();}} while(0)

struct d3d11_application
{
	virtual ~d3d11_application() {}

	virtual void draw(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv, unsigned width, unsigned height, double time) = 0;
	virtual bool init(ID3D11Device* dev, int argc, char** argv) = 0;
};

/* this is the entry point you must provide */
extern "C" d3d11_application* d3d11_application_create();

#endif
