/* DO NOT EDIT - This file generated automatically by glX_proto_send.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2004, 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <GL/gl.h>
#include "indirect.h"
#include "glxclient.h"
#include "indirect_size.h"
#include "glapi.h"
#include "glthread.h"
#include <GL/glxproto.h>
#ifdef USE_XCB
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/glx.h>
#endif /* USE_XCB */

#define __GLX_PAD(n) (((n) + 3) & ~3)

#  if defined(__i386__) && defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#    define FASTCALL __attribute__((fastcall))
#  else
#    define FASTCALL
#  endif
#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define NOINLINE __attribute__((noinline))
#  else
#    define NOINLINE
#  endif

#ifndef __GNUC__
#  define __builtin_expect(x, y) x
#endif

/* If the size and opcode values are known at compile-time, this will, on
 * x86 at least, emit them with a single instruction.
 */
#define emit_header(dest, op, size)            \
    do { union { short s[2]; int i; } temp;    \
         temp.s[0] = (size); temp.s[1] = (op); \
         *((int *)(dest)) = temp.i; } while(0)

NOINLINE CARD32
__glXReadReply( Display *dpy, size_t size, void * dest, GLboolean reply_is_always_array )
{
    xGLXSingleReply reply;
    
    (void) _XReply(dpy, (xReply *) & reply, 0, False);
    if (size != 0) {
        if ((reply.length > 0) || reply_is_always_array) {
            const GLint bytes = (reply_is_always_array) 
              ? (4 * reply.length) : (reply.size * size);
            const GLint extra = 4 - (bytes & 3);

            _XRead(dpy, dest, bytes);
            if ( extra < 4 ) {
                _XEatData(dpy, extra);
            }
        }
        else {
            (void) memcpy( dest, &(reply.pad3), size);
        }
    }

    return reply.retval;
}

NOINLINE void
__glXReadPixelReply( Display *dpy, struct glx_context * gc, unsigned max_dim,
    GLint width, GLint height, GLint depth, GLenum format, GLenum type,
    void * dest, GLboolean dimensions_in_reply )
{
    xGLXSingleReply reply;
    GLint size;
    
    (void) _XReply(dpy, (xReply *) & reply, 0, False);

    if ( dimensions_in_reply ) {
        width  = reply.pad3;
        height = reply.pad4;
        depth  = reply.pad5;
	
	if ((height == 0) || (max_dim < 2)) { height = 1; }
	if ((depth  == 0) || (max_dim < 3)) { depth  = 1; }
    }

    size = reply.length * 4;
    if (size != 0) {
        void * buf = Xmalloc( size );

        if ( buf == NULL ) {
            _XEatData(dpy, size);
            __glXSetError(gc, GL_OUT_OF_MEMORY);
        }
        else {
            const GLint extra = 4 - (size & 3);

            _XRead(dpy, buf, size);
            if ( extra < 4 ) {
                _XEatData(dpy, extra);
            }

            __glEmptyImage(gc, 3, width, height, depth, format, type,
                           buf, dest);
            Xfree(buf);
        }
    }
}

#define X_GLXSingle 0

NOINLINE FASTCALL GLubyte *
__glXSetupSingleRequest( struct glx_context * gc, GLint sop, GLint cmdlen )
{
    xGLXSingleReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXSingle, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->contextTag = gc->currentContextTag;
    req->glxCode = sop;
    return (GLubyte *)(req) + sz_xGLXSingleReq;
}

NOINLINE FASTCALL GLubyte *
__glXSetupVendorRequest( struct glx_context * gc, GLint code, GLint vop, GLint cmdlen )
{
    xGLXVendorPrivateReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXVendorPrivate, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->glxCode = code;
    req->vendorCode = vop;
    req->contextTag = gc->currentContextTag;
    return (GLubyte *)(req) + sz_xGLXVendorPrivateReq;
}

const GLuint __glXDefaultPixelStore[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };

#define zero                        (__glXDefaultPixelStore+0)
#define one                         (__glXDefaultPixelStore+8)
#define default_pixel_store_1D      (__glXDefaultPixelStore+4)
#define default_pixel_store_1D_size 20
#define default_pixel_store_2D      (__glXDefaultPixelStore+4)
#define default_pixel_store_2D_size 20
#define default_pixel_store_3D      (__glXDefaultPixelStore+0)
#define default_pixel_store_3D_size 36
#define default_pixel_store_4D      (__glXDefaultPixelStore+0)
#define default_pixel_store_4D_size 36

static FASTCALL NOINLINE void
generic_3_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_4_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_6_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_8_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_12_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 12);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_16_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 16);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_24_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 24);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_32_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 32);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_NewList 101
void __indirect_glNewList(GLuint list, GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_new_list(c, gc->currentContextTag, list, mode);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_NewList, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&list), 4);
(void) memcpy((void *)(pc + 4), (void *)(&mode), 4);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_EndList 102
void __indirect_glEndList(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 0;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_end_list(c, gc->currentContextTag);
#else
        (void) __glXSetupSingleRequest(gc, X_GLsop_EndList, cmdlen);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLrop_CallList 1
void __indirect_glCallList(GLuint list)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_CallList, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&list), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CallLists 2
void __indirect_glCallLists(GLsizei n, GLenum type, const GLvoid * lists)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glCallLists_size(type);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * n));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (gc->currentDpy != NULL), 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_CallLists, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&type), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(lists), (compsize * n));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_CallLists;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(pc + 12), (void *)(&type), 4);
    __glXSendLargeCommand(gc, pc, 16, lists, (compsize * n));
}
    }
}

#define X_GLsop_DeleteLists 103
void __indirect_glDeleteLists(GLuint list, GLsizei range)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_delete_lists(c, gc->currentContextTag, list, range);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_DeleteLists, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&list), 4);
(void) memcpy((void *)(pc + 4), (void *)(&range), 4);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GenLists 104
GLuint __indirect_glGenLists(GLsizei range)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLuint retval = (GLuint) 0;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_gen_lists_reply_t *reply = xcb_glx_gen_lists_reply(c, xcb_glx_gen_lists(c, gc->currentContextTag, range), NULL);
        retval = reply->ret_val;
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GenLists, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&range), 4);
        retval = (GLuint) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return retval;
}

#define X_GLrop_ListBase 3
void __indirect_glListBase(GLuint base)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ListBase, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&base), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Begin 4
void __indirect_glBegin(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Begin, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Bitmap 5
void __indirect_glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (bitmap != NULL) ? __glImageSize(width, height, 1, GL_COLOR_INDEX, GL_BITMAP, 0) : 0;
    const GLuint cmdlen = 48 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_Bitmap, cmdlen);
(void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&xorig), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&yorig), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&xmove), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(&ymove), 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, 2, width, height, 1, GL_COLOR_INDEX, GL_BITMAP, bitmap, gc->pc + 48, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_2D, default_pixel_store_2D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_Bitmap;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 28), (void *)(&width), 4);
(void) memcpy((void *)(pc + 32), (void *)(&height), 4);
(void) memcpy((void *)(pc + 36), (void *)(&xorig), 4);
(void) memcpy((void *)(pc + 40), (void *)(&yorig), 4);
(void) memcpy((void *)(pc + 44), (void *)(&xmove), 4);
(void) memcpy((void *)(pc + 48), (void *)(&ymove), 4);
__glXSendLargeImage(gc, compsize, 2, width, height, 1, GL_COLOR_INDEX, GL_BITMAP, bitmap, pc + 52, pc + 8);
}
    }
}

#define X_GLrop_Color3bv 6
void __indirect_glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Color3bv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3bv 6
void __indirect_glColor3bv(const GLbyte * v)
{
    generic_3_byte( X_GLrop_Color3bv, v );
}

#define X_GLrop_Color3dv 7
void __indirect_glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_Color3dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&green), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&blue), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3dv 7
void __indirect_glColor3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_Color3dv, v );
}

#define X_GLrop_Color3fv 8
void __indirect_glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Color3fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3fv 8
void __indirect_glColor3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_Color3fv, v );
}

#define X_GLrop_Color3iv 9
void __indirect_glColor3i(GLint red, GLint green, GLint blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Color3iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3iv 9
void __indirect_glColor3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_Color3iv, v );
}

#define X_GLrop_Color3sv 10
void __indirect_glColor3s(GLshort red, GLshort green, GLshort blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Color3sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3sv 10
void __indirect_glColor3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_Color3sv, v );
}

#define X_GLrop_Color3ubv 11
void __indirect_glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Color3ubv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3ubv 11
void __indirect_glColor3ubv(const GLubyte * v)
{
    generic_3_byte( X_GLrop_Color3ubv, v );
}

#define X_GLrop_Color3uiv 12
void __indirect_glColor3ui(GLuint red, GLuint green, GLuint blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Color3uiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3uiv 12
void __indirect_glColor3uiv(const GLuint * v)
{
    generic_12_byte( X_GLrop_Color3uiv, v );
}

#define X_GLrop_Color3usv 13
void __indirect_glColor3us(GLushort red, GLushort green, GLushort blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Color3usv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3usv 13
void __indirect_glColor3usv(const GLushort * v)
{
    generic_6_byte( X_GLrop_Color3usv, v );
}

#define X_GLrop_Color4bv 14
void __indirect_glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Color4bv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
(void) memcpy((void *)(gc->pc + 7), (void *)(&alpha), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4bv 14
void __indirect_glColor4bv(const GLbyte * v)
{
    generic_4_byte( X_GLrop_Color4bv, v );
}

#define X_GLrop_Color4dv 15
void __indirect_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_Color4dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&green), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&blue), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&alpha), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4dv 15
void __indirect_glColor4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_Color4dv, v );
}

#define X_GLrop_Color4fv 16
void __indirect_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Color4fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4fv 16
void __indirect_glColor4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_Color4fv, v );
}

#define X_GLrop_Color4iv 17
void __indirect_glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Color4iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4iv 17
void __indirect_glColor4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_Color4iv, v );
}

#define X_GLrop_Color4sv 18
void __indirect_glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Color4sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&alpha), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4sv 18
void __indirect_glColor4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_Color4sv, v );
}

#define X_GLrop_Color4ubv 19
void __indirect_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Color4ubv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
(void) memcpy((void *)(gc->pc + 7), (void *)(&alpha), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4ubv 19
void __indirect_glColor4ubv(const GLubyte * v)
{
    generic_4_byte( X_GLrop_Color4ubv, v );
}

#define X_GLrop_Color4uiv 20
void __indirect_glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Color4uiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4uiv 20
void __indirect_glColor4uiv(const GLuint * v)
{
    generic_16_byte( X_GLrop_Color4uiv, v );
}

#define X_GLrop_Color4usv 21
void __indirect_glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Color4usv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&alpha), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4usv 21
void __indirect_glColor4usv(const GLushort * v)
{
    generic_8_byte( X_GLrop_Color4usv, v );
}

#define X_GLrop_EdgeFlagv 22
void __indirect_glEdgeFlag(GLboolean flag)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_EdgeFlagv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&flag), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EdgeFlagv 22
void __indirect_glEdgeFlagv(const GLboolean * flag)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_EdgeFlagv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(flag), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_End 23
void __indirect_glEnd(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
emit_header(gc->pc, X_GLrop_End, cmdlen);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexdv 24
void __indirect_glIndexd(GLdouble c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Indexdv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&c), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexdv 24
void __indirect_glIndexdv(const GLdouble * c)
{
    generic_8_byte( X_GLrop_Indexdv, c );
}

#define X_GLrop_Indexfv 25
void __indirect_glIndexf(GLfloat c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Indexfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&c), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexfv 25
void __indirect_glIndexfv(const GLfloat * c)
{
    generic_4_byte( X_GLrop_Indexfv, c );
}

#define X_GLrop_Indexiv 26
void __indirect_glIndexi(GLint c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Indexiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&c), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexiv 26
void __indirect_glIndexiv(const GLint * c)
{
    generic_4_byte( X_GLrop_Indexiv, c );
}

#define X_GLrop_Indexsv 27
void __indirect_glIndexs(GLshort c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Indexsv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&c), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexsv 27
void __indirect_glIndexsv(const GLshort * c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Indexsv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(c), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3bv 28
void __indirect_glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Normal3bv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&ny), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&nz), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3bv 28
void __indirect_glNormal3bv(const GLbyte * v)
{
    generic_3_byte( X_GLrop_Normal3bv, v );
}

#define X_GLrop_Normal3dv 29
void __indirect_glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_Normal3dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&ny), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&nz), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3dv 29
void __indirect_glNormal3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_Normal3dv, v );
}

#define X_GLrop_Normal3fv 30
void __indirect_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Normal3fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&ny), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&nz), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3fv 30
void __indirect_glNormal3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_Normal3fv, v );
}

#define X_GLrop_Normal3iv 31
void __indirect_glNormal3i(GLint nx, GLint ny, GLint nz)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Normal3iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&ny), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&nz), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3iv 31
void __indirect_glNormal3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_Normal3iv, v );
}

#define X_GLrop_Normal3sv 32
void __indirect_glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Normal3sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&ny), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&nz), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3sv 32
void __indirect_glNormal3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_Normal3sv, v );
}

#define X_GLrop_RasterPos2dv 33
void __indirect_glRasterPos2d(GLdouble x, GLdouble y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_RasterPos2dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2dv 33
void __indirect_glRasterPos2dv(const GLdouble * v)
{
    generic_16_byte( X_GLrop_RasterPos2dv, v );
}

#define X_GLrop_RasterPos2fv 34
void __indirect_glRasterPos2f(GLfloat x, GLfloat y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_RasterPos2fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2fv 34
void __indirect_glRasterPos2fv(const GLfloat * v)
{
    generic_8_byte( X_GLrop_RasterPos2fv, v );
}

#define X_GLrop_RasterPos2iv 35
void __indirect_glRasterPos2i(GLint x, GLint y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_RasterPos2iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2iv 35
void __indirect_glRasterPos2iv(const GLint * v)
{
    generic_8_byte( X_GLrop_RasterPos2iv, v );
}

#define X_GLrop_RasterPos2sv 36
void __indirect_glRasterPos2s(GLshort x, GLshort y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_RasterPos2sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2sv 36
void __indirect_glRasterPos2sv(const GLshort * v)
{
    generic_4_byte( X_GLrop_RasterPos2sv, v );
}

#define X_GLrop_RasterPos3dv 37
void __indirect_glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_RasterPos3dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3dv 37
void __indirect_glRasterPos3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_RasterPos3dv, v );
}

#define X_GLrop_RasterPos3fv 38
void __indirect_glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_RasterPos3fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3fv 38
void __indirect_glRasterPos3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_RasterPos3fv, v );
}

#define X_GLrop_RasterPos3iv 39
void __indirect_glRasterPos3i(GLint x, GLint y, GLint z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_RasterPos3iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3iv 39
void __indirect_glRasterPos3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_RasterPos3iv, v );
}

#define X_GLrop_RasterPos3sv 40
void __indirect_glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_RasterPos3sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3sv 40
void __indirect_glRasterPos3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_RasterPos3sv, v );
}

#define X_GLrop_RasterPos4dv 41
void __indirect_glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_RasterPos4dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&w), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4dv 41
void __indirect_glRasterPos4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_RasterPos4dv, v );
}

#define X_GLrop_RasterPos4fv 42
void __indirect_glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_RasterPos4fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4fv 42
void __indirect_glRasterPos4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_RasterPos4fv, v );
}

#define X_GLrop_RasterPos4iv 43
void __indirect_glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_RasterPos4iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4iv 43
void __indirect_glRasterPos4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_RasterPos4iv, v );
}

#define X_GLrop_RasterPos4sv 44
void __indirect_glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_RasterPos4sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&w), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4sv 44
void __indirect_glRasterPos4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_RasterPos4sv, v );
}

#define X_GLrop_Rectdv 45
void __indirect_glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_Rectdv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y1), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&x2), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&y2), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectdv 45
void __indirect_glRectdv(const GLdouble * v1, const GLdouble * v2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_Rectdv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v1), 16);
(void) memcpy((void *)(gc->pc + 20), (void *)(v2), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectfv 46
void __indirect_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Rectfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y1), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x2), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y2), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectfv 46
void __indirect_glRectfv(const GLfloat * v1, const GLfloat * v2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Rectfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v1), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(v2), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectiv 47
void __indirect_glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Rectiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y1), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x2), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y2), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectiv 47
void __indirect_glRectiv(const GLint * v1, const GLint * v2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Rectiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v1), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(v2), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectsv 48
void __indirect_glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Rectsv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&y1), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x2), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&y2), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectsv 48
void __indirect_glRectsv(const GLshort * v1, const GLshort * v2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Rectsv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v1), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v2), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1dv 49
void __indirect_glTexCoord1d(GLdouble s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_TexCoord1dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1dv 49
void __indirect_glTexCoord1dv(const GLdouble * v)
{
    generic_8_byte( X_GLrop_TexCoord1dv, v );
}

#define X_GLrop_TexCoord1fv 50
void __indirect_glTexCoord1f(GLfloat s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_TexCoord1fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1fv 50
void __indirect_glTexCoord1fv(const GLfloat * v)
{
    generic_4_byte( X_GLrop_TexCoord1fv, v );
}

#define X_GLrop_TexCoord1iv 51
void __indirect_glTexCoord1i(GLint s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_TexCoord1iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1iv 51
void __indirect_glTexCoord1iv(const GLint * v)
{
    generic_4_byte( X_GLrop_TexCoord1iv, v );
}

#define X_GLrop_TexCoord1sv 52
void __indirect_glTexCoord1s(GLshort s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_TexCoord1sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1sv 52
void __indirect_glTexCoord1sv(const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_TexCoord1sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2dv 53
void __indirect_glTexCoord2d(GLdouble s, GLdouble t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_TexCoord2dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2dv 53
void __indirect_glTexCoord2dv(const GLdouble * v)
{
    generic_16_byte( X_GLrop_TexCoord2dv, v );
}

#define X_GLrop_TexCoord2fv 54
void __indirect_glTexCoord2f(GLfloat s, GLfloat t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_TexCoord2fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2fv 54
void __indirect_glTexCoord2fv(const GLfloat * v)
{
    generic_8_byte( X_GLrop_TexCoord2fv, v );
}

#define X_GLrop_TexCoord2iv 55
void __indirect_glTexCoord2i(GLint s, GLint t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_TexCoord2iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2iv 55
void __indirect_glTexCoord2iv(const GLint * v)
{
    generic_8_byte( X_GLrop_TexCoord2iv, v );
}

#define X_GLrop_TexCoord2sv 56
void __indirect_glTexCoord2s(GLshort s, GLshort t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_TexCoord2sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&t), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2sv 56
void __indirect_glTexCoord2sv(const GLshort * v)
{
    generic_4_byte( X_GLrop_TexCoord2sv, v );
}

#define X_GLrop_TexCoord3dv 57
void __indirect_glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_TexCoord3dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3dv 57
void __indirect_glTexCoord3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_TexCoord3dv, v );
}

#define X_GLrop_TexCoord3fv 58
void __indirect_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexCoord3fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3fv 58
void __indirect_glTexCoord3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_TexCoord3fv, v );
}

#define X_GLrop_TexCoord3iv 59
void __indirect_glTexCoord3i(GLint s, GLint t, GLint r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexCoord3iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3iv 59
void __indirect_glTexCoord3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_TexCoord3iv, v );
}

#define X_GLrop_TexCoord3sv 60
void __indirect_glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_TexCoord3sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&t), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&r), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3sv 60
void __indirect_glTexCoord3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_TexCoord3sv, v );
}

#define X_GLrop_TexCoord4dv 61
void __indirect_glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_TexCoord4dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&q), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4dv 61
void __indirect_glTexCoord4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_TexCoord4dv, v );
}

#define X_GLrop_TexCoord4fv 62
void __indirect_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_TexCoord4fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&q), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4fv 62
void __indirect_glTexCoord4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_TexCoord4fv, v );
}

#define X_GLrop_TexCoord4iv 63
void __indirect_glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_TexCoord4iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&q), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4iv 63
void __indirect_glTexCoord4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_TexCoord4iv, v );
}

#define X_GLrop_TexCoord4sv 64
void __indirect_glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_TexCoord4sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&t), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&r), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&q), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4sv 64
void __indirect_glTexCoord4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_TexCoord4sv, v );
}

#define X_GLrop_Vertex2dv 65
void __indirect_glVertex2d(GLdouble x, GLdouble y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Vertex2dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2dv 65
void __indirect_glVertex2dv(const GLdouble * v)
{
    generic_16_byte( X_GLrop_Vertex2dv, v );
}

#define X_GLrop_Vertex2fv 66
void __indirect_glVertex2f(GLfloat x, GLfloat y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Vertex2fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2fv 66
void __indirect_glVertex2fv(const GLfloat * v)
{
    generic_8_byte( X_GLrop_Vertex2fv, v );
}

#define X_GLrop_Vertex2iv 67
void __indirect_glVertex2i(GLint x, GLint y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Vertex2iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2iv 67
void __indirect_glVertex2iv(const GLint * v)
{
    generic_8_byte( X_GLrop_Vertex2iv, v );
}

#define X_GLrop_Vertex2sv 68
void __indirect_glVertex2s(GLshort x, GLshort y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Vertex2sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2sv 68
void __indirect_glVertex2sv(const GLshort * v)
{
    generic_4_byte( X_GLrop_Vertex2sv, v );
}

#define X_GLrop_Vertex3dv 69
void __indirect_glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_Vertex3dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3dv 69
void __indirect_glVertex3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_Vertex3dv, v );
}

#define X_GLrop_Vertex3fv 70
void __indirect_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Vertex3fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3fv 70
void __indirect_glVertex3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_Vertex3fv, v );
}

#define X_GLrop_Vertex3iv 71
void __indirect_glVertex3i(GLint x, GLint y, GLint z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Vertex3iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3iv 71
void __indirect_glVertex3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_Vertex3iv, v );
}

#define X_GLrop_Vertex3sv 72
void __indirect_glVertex3s(GLshort x, GLshort y, GLshort z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Vertex3sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3sv 72
void __indirect_glVertex3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_Vertex3sv, v );
}

#define X_GLrop_Vertex4dv 73
void __indirect_glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_Vertex4dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&w), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4dv 73
void __indirect_glVertex4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_Vertex4dv, v );
}

#define X_GLrop_Vertex4fv 74
void __indirect_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Vertex4fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4fv 74
void __indirect_glVertex4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_Vertex4fv, v );
}

#define X_GLrop_Vertex4iv 75
void __indirect_glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Vertex4iv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4iv 75
void __indirect_glVertex4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_Vertex4iv, v );
}

#define X_GLrop_Vertex4sv 76
void __indirect_glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Vertex4sv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&w), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4sv 76
void __indirect_glVertex4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_Vertex4sv, v );
}

#define X_GLrop_ClipPlane 77
void __indirect_glClipPlane(GLenum plane, const GLdouble * equation)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_ClipPlane, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(equation), 32);
(void) memcpy((void *)(gc->pc + 36), (void *)(&plane), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorMaterial 78
void __indirect_glColorMaterial(GLenum face, GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_ColorMaterial, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CullFace 79
void __indirect_glCullFace(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_CullFace, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogf 80
void __indirect_glFogf(GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Fogf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogfv 81
void __indirect_glFogfv(GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glFogfv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_Fogfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogi 82
void __indirect_glFogi(GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Fogi, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogiv 83
void __indirect_glFogiv(GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glFogiv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_Fogiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FrontFace 84
void __indirect_glFrontFace(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_FrontFace, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Hint 85
void __indirect_glHint(GLenum target, GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Hint, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lightf 86
void __indirect_glLightf(GLenum light, GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Lightf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lightfv 87
void __indirect_glLightfv(GLenum light, GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_Lightfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lighti 88
void __indirect_glLighti(GLenum light, GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Lighti, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lightiv 89
void __indirect_glLightiv(GLenum light, GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightiv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_Lightiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModelf 90
void __indirect_glLightModelf(GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_LightModelf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModelfv 91
void __indirect_glLightModelfv(GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightModelfv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_LightModelfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModeli 92
void __indirect_glLightModeli(GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_LightModeli, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModeliv 93
void __indirect_glLightModeliv(GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightModeliv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_LightModeliv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LineStipple 94
void __indirect_glLineStipple(GLint factor, GLushort pattern)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_LineStipple, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&factor), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pattern), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LineWidth 95
void __indirect_glLineWidth(GLfloat width)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_LineWidth, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&width), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materialf 96
void __indirect_glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Materialf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materialfv 97
void __indirect_glMaterialfv(GLenum face, GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glMaterialfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_Materialfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materiali 98
void __indirect_glMateriali(GLenum face, GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Materiali, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materialiv 99
void __indirect_glMaterialiv(GLenum face, GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glMaterialiv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_Materialiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointSize 100
void __indirect_glPointSize(GLfloat size)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_PointSize, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&size), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PolygonMode 101
void __indirect_glPolygonMode(GLenum face, GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_PolygonMode, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PolygonStipple 102
void __indirect_glPolygonStipple(const GLubyte * mask)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (mask != NULL) ? __glImageSize(32, 32, 1, GL_COLOR_INDEX, GL_BITMAP, 0) : 0;
    const GLuint cmdlen = 24 + __GLX_PAD(compsize);
emit_header(gc->pc, X_GLrop_PolygonStipple, cmdlen);
if (compsize > 0) {
    (*gc->fillImage)(gc, 2, 32, 32, 1, GL_COLOR_INDEX, GL_BITMAP, mask, gc->pc + 24, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_2D, default_pixel_store_2D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Scissor 103
void __indirect_glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Scissor, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&height), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ShadeModel 104
void __indirect_glShadeModel(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ShadeModel, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameterf 105
void __indirect_glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexParameterf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameterfv 106
void __indirect_glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexParameterfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_TexParameterfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameteri 107
void __indirect_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexParameteri, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameteriv 108
void __indirect_glTexParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexParameteriv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_TexParameteriv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static void
__glx_TexImage_1D2D( unsigned opcode, unsigned dim, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glImageSize(width, height, 1, format, type, target);
    const GLuint cmdlen = 56 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, opcode, cmdlen);
(void) memcpy((void *)(gc->pc + 24), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(&border), 4);
(void) memcpy((void *)(gc->pc + 48), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 52), (void *)(&type), 4);
if ((compsize > 0) && (pixels != NULL)) {
    (*gc->fillImage)(gc, dim, width, height, 1, format, type, pixels, gc->pc + 56, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_2D, default_pixel_store_2D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = opcode;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 28), (void *)(&target), 4);
(void) memcpy((void *)(pc + 32), (void *)(&level), 4);
(void) memcpy((void *)(pc + 36), (void *)(&internalformat), 4);
(void) memcpy((void *)(pc + 40), (void *)(&width), 4);
(void) memcpy((void *)(pc + 44), (void *)(&height), 4);
(void) memcpy((void *)(pc + 48), (void *)(&border), 4);
(void) memcpy((void *)(pc + 52), (void *)(&format), 4);
(void) memcpy((void *)(pc + 56), (void *)(&type), 4);
__glXSendLargeImage(gc, compsize, dim, width, height, 1, format, type, pixels, pc + 60, pc + 8);
}
    }
}

#define X_GLrop_TexImage1D 109
void __indirect_glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    __glx_TexImage_1D2D(X_GLrop_TexImage1D, 1, target, level, internalformat, width, 1, border, format, type, pixels );
}

#define X_GLrop_TexImage2D 110
void __indirect_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    __glx_TexImage_1D2D(X_GLrop_TexImage2D, 2, target, level, internalformat, width, height, border, format, type, pixels );
}

#define X_GLrop_TexEnvf 111
void __indirect_glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexEnvf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexEnvfv 112
void __indirect_glTexEnvfv(GLenum target, GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexEnvfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_TexEnvfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexEnvi 113
void __indirect_glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexEnvi, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexEnviv 114
void __indirect_glTexEnviv(GLenum target, GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexEnviv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_TexEnviv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGend 115
void __indirect_glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_TexGend, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&param), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&coord), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&pname), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGendv 116
void __indirect_glTexGendv(GLenum coord, GLenum pname, const GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexGendv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 8));
emit_header(gc->pc, X_GLrop_TexGendv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 8));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGenf 117
void __indirect_glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexGenf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGenfv 118
void __indirect_glTexGenfv(GLenum coord, GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexGenfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_TexGenfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGeni 119
void __indirect_glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_TexGeni, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGeniv 120
void __indirect_glTexGeniv(GLenum coord, GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexGeniv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_TexGeniv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_InitNames 121
void __indirect_glInitNames(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
emit_header(gc->pc, X_GLrop_InitNames, cmdlen);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadName 122
void __indirect_glLoadName(GLuint name)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_LoadName, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&name), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PassThrough 123
void __indirect_glPassThrough(GLfloat token)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_PassThrough, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&token), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PopName 124
void __indirect_glPopName(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
emit_header(gc->pc, X_GLrop_PopName, cmdlen);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PushName 125
void __indirect_glPushName(GLuint name)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_PushName, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&name), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_DrawBuffer 126
void __indirect_glDrawBuffer(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_DrawBuffer, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Clear 127
void __indirect_glClear(GLbitfield mask)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Clear, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearAccum 128
void __indirect_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_ClearAccum, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearIndex 129
void __indirect_glClearIndex(GLfloat c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ClearIndex, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&c), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearColor 130
void __indirect_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_ClearColor, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearStencil 131
void __indirect_glClearStencil(GLint s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ClearStencil, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearDepth 132
void __indirect_glClearDepth(GLclampd depth)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_ClearDepth, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&depth), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_StencilMask 133
void __indirect_glStencilMask(GLuint mask)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_StencilMask, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorMask 134
void __indirect_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ColorMask, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
(void) memcpy((void *)(gc->pc + 7), (void *)(&alpha), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_DepthMask 135
void __indirect_glDepthMask(GLboolean flag)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_DepthMask, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&flag), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_IndexMask 136
void __indirect_glIndexMask(GLuint mask)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_IndexMask, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Accum 137
void __indirect_glAccum(GLenum op, GLfloat value)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_Accum, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&value), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PopAttrib 141
void __indirect_glPopAttrib(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
emit_header(gc->pc, X_GLrop_PopAttrib, cmdlen);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PushAttrib 142
void __indirect_glPushAttrib(GLbitfield mask)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_PushAttrib, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid1d 147
void __indirect_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_MapGrid1d, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&u1), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&un), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid1f 148
void __indirect_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MapGrid1f, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&un), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&u1), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid2d 149
void __indirect_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44;
emit_header(gc->pc, X_GLrop_MapGrid2d, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&u1), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&v1), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&v2), 8);
(void) memcpy((void *)(gc->pc + 36), (void *)(&un), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&vn), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid2f 150
void __indirect_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_MapGrid2f, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&un), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&u1), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&vn), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&v1), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&v2), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord1dv 151
void __indirect_glEvalCoord1d(GLdouble u)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_EvalCoord1dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&u), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord1dv 151
void __indirect_glEvalCoord1dv(const GLdouble * u)
{
    generic_8_byte( X_GLrop_EvalCoord1dv, u );
}

#define X_GLrop_EvalCoord1fv 152
void __indirect_glEvalCoord1f(GLfloat u)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_EvalCoord1fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&u), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord1fv 152
void __indirect_glEvalCoord1fv(const GLfloat * u)
{
    generic_4_byte( X_GLrop_EvalCoord1fv, u );
}

#define X_GLrop_EvalCoord2dv 153
void __indirect_glEvalCoord2d(GLdouble u, GLdouble v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_EvalCoord2dv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&u), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord2dv 153
void __indirect_glEvalCoord2dv(const GLdouble * u)
{
    generic_16_byte( X_GLrop_EvalCoord2dv, u );
}

#define X_GLrop_EvalCoord2fv 154
void __indirect_glEvalCoord2f(GLfloat u, GLfloat v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_EvalCoord2fv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&u), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord2fv 154
void __indirect_glEvalCoord2fv(const GLfloat * u)
{
    generic_8_byte( X_GLrop_EvalCoord2fv, u );
}

#define X_GLrop_EvalMesh1 155
void __indirect_glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_EvalMesh1, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&i1), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&i2), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalPoint1 156
void __indirect_glEvalPoint1(GLint i)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_EvalPoint1, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&i), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalMesh2 157
void __indirect_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_EvalMesh2, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&i1), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&i2), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&j1), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&j2), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalPoint2 158
void __indirect_glEvalPoint2(GLint i, GLint j)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_EvalPoint2, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&i), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&j), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_AlphaFunc 159
void __indirect_glAlphaFunc(GLenum func, GLclampf ref)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_AlphaFunc, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&func), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&ref), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BlendFunc 160
void __indirect_glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_BlendFunc, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&sfactor), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&dfactor), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LogicOp 161
void __indirect_glLogicOp(GLenum opcode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_LogicOp, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&opcode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_StencilFunc 162
void __indirect_glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_StencilFunc, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&func), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&ref), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&mask), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_StencilOp 163
void __indirect_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_StencilOp, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&fail), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&zfail), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&zpass), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_DepthFunc 164
void __indirect_glDepthFunc(GLenum func)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_DepthFunc, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&func), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelZoom 165
void __indirect_glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_PixelZoom, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&xfactor), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&yfactor), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelTransferf 166
void __indirect_glPixelTransferf(GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_PixelTransferf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelTransferi 167
void __indirect_glPixelTransferi(GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_PixelTransferi, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelMapfv 168
void __indirect_glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((mapsize * 4));
    if (mapsize < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((mapsize >= 0) && (gc->currentDpy != NULL), 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_PixelMapfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&map), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&mapsize), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(values), (mapsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_PixelMapfv;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 8), (void *)(&map), 4);
(void) memcpy((void *)(pc + 12), (void *)(&mapsize), 4);
    __glXSendLargeCommand(gc, pc, 16, values, (mapsize * 4));
}
    }
}

#define X_GLrop_PixelMapuiv 169
void __indirect_glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((mapsize * 4));
    if (mapsize < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((mapsize >= 0) && (gc->currentDpy != NULL), 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_PixelMapuiv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&map), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&mapsize), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(values), (mapsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_PixelMapuiv;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 8), (void *)(&map), 4);
(void) memcpy((void *)(pc + 12), (void *)(&mapsize), 4);
    __glXSendLargeCommand(gc, pc, 16, values, (mapsize * 4));
}
    }
}

#define X_GLrop_PixelMapusv 170
void __indirect_glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((mapsize * 2));
    if (mapsize < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((mapsize >= 0) && (gc->currentDpy != NULL), 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_PixelMapusv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&map), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&mapsize), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(values), (mapsize * 2));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_PixelMapusv;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 8), (void *)(&map), 4);
(void) memcpy((void *)(pc + 12), (void *)(&mapsize), 4);
    __glXSendLargeCommand(gc, pc, 16, values, (mapsize * 2));
}
    }
}

#define X_GLrop_ReadBuffer 171
void __indirect_glReadBuffer(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ReadBuffer, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyPixels 172
void __indirect_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_CopyPixels, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&type), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_ReadPixels 111
void __indirect_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 28;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_read_pixels_reply_t *reply = xcb_glx_read_pixels_reply(c, xcb_glx_read_pixels(c, gc->currentContextTag, x, y, width, height, format, type, state->storePack.swapEndian, 0), NULL);
        __glEmptyImage(gc, 3, width, height, 1, format, type, xcb_glx_read_pixels_data(reply), pixels);
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_ReadPixels, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&x), 4);
(void) memcpy((void *)(pc + 4), (void *)(&y), 4);
(void) memcpy((void *)(pc + 8), (void *)(&width), 4);
(void) memcpy((void *)(pc + 12), (void *)(&height), 4);
(void) memcpy((void *)(pc + 16), (void *)(&format), 4);
(void) memcpy((void *)(pc + 20), (void *)(&type), 4);
        *(int32_t *)(pc + 24) = 0;
        * (int8_t *)(pc + 24) = state->storePack.swapEndian;
        __glXReadPixelReply(dpy, gc, 2, width, height, 1, format, type, pixels, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLrop_DrawPixels 173
void __indirect_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (pixels != NULL) ? __glImageSize(width, height, 1, format, type, 0) : 0;
    const GLuint cmdlen = 40 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_DrawPixels, cmdlen);
(void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&type), 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, 2, width, height, 1, format, type, pixels, gc->pc + 40, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_2D, default_pixel_store_2D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_DrawPixels;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 28), (void *)(&width), 4);
(void) memcpy((void *)(pc + 32), (void *)(&height), 4);
(void) memcpy((void *)(pc + 36), (void *)(&format), 4);
(void) memcpy((void *)(pc + 40), (void *)(&type), 4);
__glXSendLargeImage(gc, compsize, 2, width, height, 1, format, type, pixels, pc + 44, pc + 8);
}
    }
}

#define X_GLsop_GetClipPlane 113
void __indirect_glGetClipPlane(GLenum plane, GLdouble * equation)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_clip_plane_reply_t *reply = xcb_glx_get_clip_plane_reply(c, xcb_glx_get_clip_plane(c, gc->currentContextTag, plane), NULL);
        (void)memcpy(equation, xcb_glx_get_clip_plane_data(reply), xcb_glx_get_clip_plane_data_length(reply) * sizeof(GLdouble));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetClipPlane, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&plane), 4);
        (void) __glXReadReply(dpy, 8, equation, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetLightfv 118
void __indirect_glGetLightfv(GLenum light, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_lightfv_reply_t *reply = xcb_glx_get_lightfv_reply(c, xcb_glx_get_lightfv(c, gc->currentContextTag, light, pname), NULL);
        if (xcb_glx_get_lightfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_lightfv_data(reply), xcb_glx_get_lightfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetLightfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&light), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetLightiv 119
void __indirect_glGetLightiv(GLenum light, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_lightiv_reply_t *reply = xcb_glx_get_lightiv_reply(c, xcb_glx_get_lightiv(c, gc->currentContextTag, light, pname), NULL);
        if (xcb_glx_get_lightiv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_lightiv_data(reply), xcb_glx_get_lightiv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetLightiv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&light), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetMapdv 120
void __indirect_glGetMapdv(GLenum target, GLenum query, GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_mapdv_reply_t *reply = xcb_glx_get_mapdv_reply(c, xcb_glx_get_mapdv(c, gc->currentContextTag, target, query), NULL);
        if (xcb_glx_get_mapdv_data_length(reply) == 0)
            (void)memcpy(v, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(v, xcb_glx_get_mapdv_data(reply), xcb_glx_get_mapdv_data_length(reply) * sizeof(GLdouble));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMapdv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&query), 4);
        (void) __glXReadReply(dpy, 8, v, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetMapfv 121
void __indirect_glGetMapfv(GLenum target, GLenum query, GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_mapfv_reply_t *reply = xcb_glx_get_mapfv_reply(c, xcb_glx_get_mapfv(c, gc->currentContextTag, target, query), NULL);
        if (xcb_glx_get_mapfv_data_length(reply) == 0)
            (void)memcpy(v, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(v, xcb_glx_get_mapfv_data(reply), xcb_glx_get_mapfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMapfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&query), 4);
        (void) __glXReadReply(dpy, 4, v, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetMapiv 122
void __indirect_glGetMapiv(GLenum target, GLenum query, GLint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_mapiv_reply_t *reply = xcb_glx_get_mapiv_reply(c, xcb_glx_get_mapiv(c, gc->currentContextTag, target, query), NULL);
        if (xcb_glx_get_mapiv_data_length(reply) == 0)
            (void)memcpy(v, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(v, xcb_glx_get_mapiv_data(reply), xcb_glx_get_mapiv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMapiv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&query), 4);
        (void) __glXReadReply(dpy, 4, v, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetMaterialfv 123
void __indirect_glGetMaterialfv(GLenum face, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_materialfv_reply_t *reply = xcb_glx_get_materialfv_reply(c, xcb_glx_get_materialfv(c, gc->currentContextTag, face, pname), NULL);
        if (xcb_glx_get_materialfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_materialfv_data(reply), xcb_glx_get_materialfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMaterialfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&face), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetMaterialiv 124
void __indirect_glGetMaterialiv(GLenum face, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_materialiv_reply_t *reply = xcb_glx_get_materialiv_reply(c, xcb_glx_get_materialiv(c, gc->currentContextTag, face, pname), NULL);
        if (xcb_glx_get_materialiv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_materialiv_data(reply), xcb_glx_get_materialiv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMaterialiv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&face), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetPixelMapfv 125
void __indirect_glGetPixelMapfv(GLenum map, GLfloat * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_pixel_mapfv_reply_t *reply = xcb_glx_get_pixel_mapfv_reply(c, xcb_glx_get_pixel_mapfv(c, gc->currentContextTag, map), NULL);
        if (xcb_glx_get_pixel_mapfv_data_length(reply) == 0)
            (void)memcpy(values, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(values, xcb_glx_get_pixel_mapfv_data(reply), xcb_glx_get_pixel_mapfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetPixelMapfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&map), 4);
        (void) __glXReadReply(dpy, 4, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetPixelMapuiv 126
void __indirect_glGetPixelMapuiv(GLenum map, GLuint * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_pixel_mapuiv_reply_t *reply = xcb_glx_get_pixel_mapuiv_reply(c, xcb_glx_get_pixel_mapuiv(c, gc->currentContextTag, map), NULL);
        if (xcb_glx_get_pixel_mapuiv_data_length(reply) == 0)
            (void)memcpy(values, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(values, xcb_glx_get_pixel_mapuiv_data(reply), xcb_glx_get_pixel_mapuiv_data_length(reply) * sizeof(GLuint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetPixelMapuiv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&map), 4);
        (void) __glXReadReply(dpy, 4, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetPixelMapusv 127
void __indirect_glGetPixelMapusv(GLenum map, GLushort * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_pixel_mapusv_reply_t *reply = xcb_glx_get_pixel_mapusv_reply(c, xcb_glx_get_pixel_mapusv(c, gc->currentContextTag, map), NULL);
        if (xcb_glx_get_pixel_mapusv_data_length(reply) == 0)
            (void)memcpy(values, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(values, xcb_glx_get_pixel_mapusv_data(reply), xcb_glx_get_pixel_mapusv_data_length(reply) * sizeof(GLushort));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetPixelMapusv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&map), 4);
        (void) __glXReadReply(dpy, 2, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetPolygonStipple 128
void __indirect_glGetPolygonStipple(GLubyte * mask)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_polygon_stipple_reply_t *reply = xcb_glx_get_polygon_stipple_reply(c, xcb_glx_get_polygon_stipple(c, gc->currentContextTag, 0), NULL);
        __glEmptyImage(gc, 3, 32, 32, 1, GL_COLOR_INDEX, GL_BITMAP, xcb_glx_get_polygon_stipple_data(reply), mask);
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetPolygonStipple, cmdlen);
        *(int32_t *)(pc + 0) = 0;
        __glXReadPixelReply(dpy, gc, 2, 32, 32, 1, GL_COLOR_INDEX, GL_BITMAP, mask, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexEnvfv 130
void __indirect_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_envfv_reply_t *reply = xcb_glx_get_tex_envfv_reply(c, xcb_glx_get_tex_envfv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_tex_envfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_envfv_data(reply), xcb_glx_get_tex_envfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexEnvfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexEnviv 131
void __indirect_glGetTexEnviv(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_enviv_reply_t *reply = xcb_glx_get_tex_enviv_reply(c, xcb_glx_get_tex_enviv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_tex_enviv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_enviv_data(reply), xcb_glx_get_tex_enviv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexEnviv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexGendv 132
void __indirect_glGetTexGendv(GLenum coord, GLenum pname, GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_gendv_reply_t *reply = xcb_glx_get_tex_gendv_reply(c, xcb_glx_get_tex_gendv(c, gc->currentContextTag, coord, pname), NULL);
        if (xcb_glx_get_tex_gendv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_gendv_data(reply), xcb_glx_get_tex_gendv_data_length(reply) * sizeof(GLdouble));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexGendv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&coord), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 8, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexGenfv 133
void __indirect_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_genfv_reply_t *reply = xcb_glx_get_tex_genfv_reply(c, xcb_glx_get_tex_genfv(c, gc->currentContextTag, coord, pname), NULL);
        if (xcb_glx_get_tex_genfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_genfv_data(reply), xcb_glx_get_tex_genfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexGenfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&coord), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexGeniv 134
void __indirect_glGetTexGeniv(GLenum coord, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_geniv_reply_t *reply = xcb_glx_get_tex_geniv_reply(c, xcb_glx_get_tex_geniv(c, gc->currentContextTag, coord, pname), NULL);
        if (xcb_glx_get_tex_geniv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_geniv_data(reply), xcb_glx_get_tex_geniv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexGeniv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&coord), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexImage 135
void __indirect_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 20;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_image_reply_t *reply = xcb_glx_get_tex_image_reply(c, xcb_glx_get_tex_image(c, gc->currentContextTag, target, level, format, type, state->storePack.swapEndian), NULL);
        if (reply->height == 0) { reply->height = 1; }
        if (reply->depth == 0) { reply->depth = 1; }
        __glEmptyImage(gc, 3, reply->width, reply->height, reply->depth, format, type, xcb_glx_get_tex_image_data(reply), pixels);
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexImage, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&level), 4);
(void) memcpy((void *)(pc + 8), (void *)(&format), 4);
(void) memcpy((void *)(pc + 12), (void *)(&type), 4);
        *(int32_t *)(pc + 16) = 0;
        * (int8_t *)(pc + 16) = state->storePack.swapEndian;
        __glXReadPixelReply(dpy, gc, 3, 0, 0, 0, format, type, pixels, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexParameterfv 136
void __indirect_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_parameterfv_reply_t *reply = xcb_glx_get_tex_parameterfv_reply(c, xcb_glx_get_tex_parameterfv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_tex_parameterfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_parameterfv_data(reply), xcb_glx_get_tex_parameterfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexParameterfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexParameteriv 137
void __indirect_glGetTexParameteriv(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_parameteriv_reply_t *reply = xcb_glx_get_tex_parameteriv_reply(c, xcb_glx_get_tex_parameteriv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_tex_parameteriv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_parameteriv_data(reply), xcb_glx_get_tex_parameteriv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexParameteriv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexLevelParameterfv 138
void __indirect_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 12;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_level_parameterfv_reply_t *reply = xcb_glx_get_tex_level_parameterfv_reply(c, xcb_glx_get_tex_level_parameterfv(c, gc->currentContextTag, target, level, pname), NULL);
        if (xcb_glx_get_tex_level_parameterfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_level_parameterfv_data(reply), xcb_glx_get_tex_level_parameterfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexLevelParameterfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&level), 4);
(void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetTexLevelParameteriv 139
void __indirect_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 12;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_tex_level_parameteriv_reply_t *reply = xcb_glx_get_tex_level_parameteriv_reply(c, xcb_glx_get_tex_level_parameteriv(c, gc->currentContextTag, target, level, pname), NULL);
        if (xcb_glx_get_tex_level_parameteriv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_tex_level_parameteriv_data(reply), xcb_glx_get_tex_level_parameteriv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetTexLevelParameteriv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&level), 4);
(void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_IsList 141
GLboolean __indirect_glIsList(GLuint list)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_is_list_reply_t *reply = xcb_glx_is_list_reply(c, xcb_glx_is_list(c, gc->currentContextTag, list), NULL);
        retval = reply->ret_val;
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_IsList, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&list), 4);
        retval = (GLboolean) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return retval;
}

#define X_GLrop_DepthRange 174
void __indirect_glDepthRange(GLclampd zNear, GLclampd zFar)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_DepthRange, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&zNear), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&zFar), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Frustum 175
void __indirect_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 52;
emit_header(gc->pc, X_GLrop_Frustum, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&left), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&right), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&bottom), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&top), 8);
(void) memcpy((void *)(gc->pc + 36), (void *)(&zNear), 8);
(void) memcpy((void *)(gc->pc + 44), (void *)(&zFar), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadIdentity 176
void __indirect_glLoadIdentity(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
emit_header(gc->pc, X_GLrop_LoadIdentity, cmdlen);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadMatrixf 177
void __indirect_glLoadMatrixf(const GLfloat * m)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 68;
emit_header(gc->pc, X_GLrop_LoadMatrixf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(m), 64);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadMatrixd 178
void __indirect_glLoadMatrixd(const GLdouble * m)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 132;
emit_header(gc->pc, X_GLrop_LoadMatrixd, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(m), 128);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MatrixMode 179
void __indirect_glMatrixMode(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_MatrixMode, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultMatrixf 180
void __indirect_glMultMatrixf(const GLfloat * m)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 68;
emit_header(gc->pc, X_GLrop_MultMatrixf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(m), 64);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultMatrixd 181
void __indirect_glMultMatrixd(const GLdouble * m)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 132;
emit_header(gc->pc, X_GLrop_MultMatrixd, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(m), 128);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Ortho 182
void __indirect_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 52;
emit_header(gc->pc, X_GLrop_Ortho, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&left), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&right), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&bottom), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&top), 8);
(void) memcpy((void *)(gc->pc + 36), (void *)(&zNear), 8);
(void) memcpy((void *)(gc->pc + 44), (void *)(&zFar), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PopMatrix 183
void __indirect_glPopMatrix(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
emit_header(gc->pc, X_GLrop_PopMatrix, cmdlen);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PushMatrix 184
void __indirect_glPushMatrix(void)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
emit_header(gc->pc, X_GLrop_PushMatrix, cmdlen);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rotated 185
void __indirect_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_Rotated, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&angle), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&z), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rotatef 186
void __indirect_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Rotatef, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&angle), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Scaled 187
void __indirect_glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_Scaled, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Scalef 188
void __indirect_glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Scalef, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Translated 189
void __indirect_glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_Translated, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Translatef 190
void __indirect_glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Translatef, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Viewport 191
void __indirect_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Viewport, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&height), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BindTexture 4117
void __indirect_glBindTexture(GLenum target, GLuint texture)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_BindTexture, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&texture), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexubv 194
void __indirect_glIndexub(GLubyte c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Indexubv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&c), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexubv 194
void __indirect_glIndexubv(const GLubyte * c)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_Indexubv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(c), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PolygonOffset 192
void __indirect_glPolygonOffset(GLfloat factor, GLfloat units)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_PolygonOffset, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&factor), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&units), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexImage1D 4119
void __indirect_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
emit_header(gc->pc, X_GLrop_CopyTexImage1D, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&border), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexImage2D 4120
void __indirect_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_CopyTexImage2D, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&border), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexSubImage1D 4121
void __indirect_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_CopyTexSubImage1D, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&xoffset), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexSubImage2D 4122
void __indirect_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
emit_header(gc->pc, X_GLrop_CopyTexSubImage2D, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&xoffset), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&yoffset), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&height), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_DeleteTextures 144
void __indirect_glDeleteTextures(GLsizei n, const GLuint * textures)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
#endif
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_delete_textures(c, gc->currentContextTag, n, textures);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_DeleteTextures, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
(void) memcpy((void *)(pc + 4), (void *)(textures), (n * 4));
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_DeleteTexturesEXT 12
void glDeleteTexturesEXT(GLsizei n, const GLuint * textures)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLDELETETEXTURESEXTPROC p =
            (PFNGLDELETETEXTURESEXTPROC) disp_table[327];
    p(n, textures);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivate, X_GLvop_DeleteTexturesEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
(void) memcpy((void *)(pc + 4), (void *)(textures), (n * 4));
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GenTextures 145
void __indirect_glGenTextures(GLsizei n, GLuint * textures)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_gen_textures_reply_t *reply = xcb_glx_gen_textures_reply(c, xcb_glx_gen_textures(c, gc->currentContextTag, n), NULL);
        (void)memcpy(textures, xcb_glx_gen_textures_data(reply), xcb_glx_gen_textures_data_length(reply) * sizeof(GLuint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GenTextures, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) __glXReadReply(dpy, 4, textures, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GenTexturesEXT 13
void glGenTexturesEXT(GLsizei n, GLuint * textures)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGENTEXTURESEXTPROC p =
            (PFNGLGENTEXTURESEXTPROC) disp_table[328];
    p(n, textures);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GenTexturesEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) __glXReadReply(dpy, 4, textures, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_IsTexture 146
GLboolean __indirect_glIsTexture(GLuint texture)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_is_texture_reply_t *reply = xcb_glx_is_texture_reply(c, xcb_glx_is_texture(c, gc->currentContextTag, texture), NULL);
        retval = reply->ret_val;
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_IsTexture, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&texture), 4);
        retval = (GLboolean) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return retval;
}

#define X_GLvop_IsTextureEXT 14
GLboolean glIsTextureEXT(GLuint texture)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLISTEXTUREEXTPROC p =
            (PFNGLISTEXTUREEXTPROC) disp_table[330];
    return p(texture);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_IsTextureEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&texture), 4);
        retval = (GLboolean) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}
}

#define X_GLrop_PrioritizeTextures 4118
void __indirect_glPrioritizeTextures(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8 + __GLX_PAD((n * 4)) + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_PrioritizeTextures, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(textures), (n * 4));
(void) memcpy((void *)(gc->pc + 8 + (n * 4)), (void *)(priorities), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

static void
__glx_TexSubImage_1D2D( unsigned opcode, unsigned dim, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (pixels != NULL) ? __glImageSize(width, height, 1, format, type, target) : 0;
    const GLuint cmdlen = 60 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, opcode, cmdlen);
(void) memcpy((void *)(gc->pc + 24), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&xoffset), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&yoffset), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 48), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 52), (void *)(&type), 4);
(void) memset((void *)(gc->pc + 56), 0, 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, dim, width, height, 1, format, type, pixels, gc->pc + 60, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_2D, default_pixel_store_2D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = opcode;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 28), (void *)(&target), 4);
(void) memcpy((void *)(pc + 32), (void *)(&level), 4);
(void) memcpy((void *)(pc + 36), (void *)(&xoffset), 4);
(void) memcpy((void *)(pc + 40), (void *)(&yoffset), 4);
(void) memcpy((void *)(pc + 44), (void *)(&width), 4);
(void) memcpy((void *)(pc + 48), (void *)(&height), 4);
(void) memcpy((void *)(pc + 52), (void *)(&format), 4);
(void) memcpy((void *)(pc + 56), (void *)(&type), 4);
(void) memset((void *)(pc + 60), 0, 4);
__glXSendLargeImage(gc, compsize, dim, width, height, 1, format, type, pixels, pc + 64, pc + 8);
}
    }
}

#define X_GLrop_TexSubImage1D 4099
void __indirect_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
    __glx_TexSubImage_1D2D(X_GLrop_TexSubImage1D, 1, target, level, xoffset, 1, width, 1, format, type, pixels );
}

#define X_GLrop_TexSubImage2D 4100
void __indirect_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    __glx_TexSubImage_1D2D(X_GLrop_TexSubImage2D, 2, target, level, xoffset, yoffset, width, height, format, type, pixels );
}

#define X_GLrop_BlendColor 4096
void __indirect_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_BlendColor, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BlendEquation 4097
void __indirect_glBlendEquation(GLenum mode)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_BlendEquation, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorTable 2053
void __indirect_glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (table != NULL) ? __glImageSize(width, 1, 1, format, type, target) : 0;
    const GLuint cmdlen = 44 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_ColorTable, cmdlen);
(void) memcpy((void *)(gc->pc + 24), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&type), 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, 1, width, 1, 1, format, type, table, gc->pc + 44, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_1D, default_pixel_store_1D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_ColorTable;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 28), (void *)(&target), 4);
(void) memcpy((void *)(pc + 32), (void *)(&internalformat), 4);
(void) memcpy((void *)(pc + 36), (void *)(&width), 4);
(void) memcpy((void *)(pc + 40), (void *)(&format), 4);
(void) memcpy((void *)(pc + 44), (void *)(&type), 4);
__glXSendLargeImage(gc, compsize, 1, width, 1, 1, format, type, table, pc + 48, pc + 8);
}
    }
}

#define X_GLrop_ColorTableParameterfv 2054
void __indirect_glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glColorTableParameterfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_ColorTableParameterfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorTableParameteriv 2055
void __indirect_glColorTableParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glColorTableParameteriv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_ColorTableParameteriv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyColorTable 2056
void __indirect_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_CopyColorTable, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_GetColorTable 147
void __indirect_glGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 16;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_color_table_reply_t *reply = xcb_glx_get_color_table_reply(c, xcb_glx_get_color_table(c, gc->currentContextTag, target, format, type, state->storePack.swapEndian), NULL);
        __glEmptyImage(gc, 3, reply->width, 1, 1, format, type, xcb_glx_get_color_table_data(reply), table);
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetColorTable, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        __glXReadPixelReply(dpy, gc, 1, 0, 0, 0, format, type, table, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetColorTableSGI 4098
void glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETCOLORTABLESGIPROC p =
            (PFNGLGETCOLORTABLESGIPROC) disp_table[343];
    p(target, format, type, table);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 16;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetColorTableSGI, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        __glXReadPixelReply(dpy, gc, 1, 0, 0, 0, format, type, table, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetColorTableParameterfv 148
void __indirect_glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_color_table_parameterfv_reply_t *reply = xcb_glx_get_color_table_parameterfv_reply(c, xcb_glx_get_color_table_parameterfv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_color_table_parameterfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_color_table_parameterfv_data(reply), xcb_glx_get_color_table_parameterfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetColorTableParameterfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetColorTableParameterfvSGI 4099
void glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETCOLORTABLEPARAMETERFVSGIPROC p =
            (PFNGLGETCOLORTABLEPARAMETERFVSGIPROC) disp_table[344];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetColorTableParameterfvSGI, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetColorTableParameteriv 149
void __indirect_glGetColorTableParameteriv(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_color_table_parameteriv_reply_t *reply = xcb_glx_get_color_table_parameteriv_reply(c, xcb_glx_get_color_table_parameteriv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_color_table_parameteriv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_color_table_parameteriv_data(reply), xcb_glx_get_color_table_parameteriv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetColorTableParameteriv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetColorTableParameterivSGI 4100
void glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETCOLORTABLEPARAMETERIVSGIPROC p =
            (PFNGLGETCOLORTABLEPARAMETERIVSGIPROC) disp_table[345];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetColorTableParameterivSGI, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLrop_ColorSubTable 195
void __indirect_glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (data != NULL) ? __glImageSize(count, 1, 1, format, type, target) : 0;
    const GLuint cmdlen = 44 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_ColorSubTable, cmdlen);
(void) memcpy((void *)(gc->pc + 24), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&start), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&count), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&type), 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, 1, count, 1, 1, format, type, data, gc->pc + 44, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_1D, default_pixel_store_1D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_ColorSubTable;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 28), (void *)(&target), 4);
(void) memcpy((void *)(pc + 32), (void *)(&start), 4);
(void) memcpy((void *)(pc + 36), (void *)(&count), 4);
(void) memcpy((void *)(pc + 40), (void *)(&format), 4);
(void) memcpy((void *)(pc + 44), (void *)(&type), 4);
__glXSendLargeImage(gc, compsize, 1, count, 1, 1, format, type, data, pc + 48, pc + 8);
}
    }
}

#define X_GLrop_CopyColorSubTable 196
void __indirect_glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_CopyColorSubTable, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&start), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static void
__glx_ConvolutionFilter_1D2D( unsigned opcode, unsigned dim, GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (image != NULL) ? __glImageSize(width, height, 1, format, type, target) : 0;
    const GLuint cmdlen = 48 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, opcode, cmdlen);
(void) memcpy((void *)(gc->pc + 24), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(&type), 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, dim, width, height, 1, format, type, image, gc->pc + 48, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_2D, default_pixel_store_2D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = opcode;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 28), (void *)(&target), 4);
(void) memcpy((void *)(pc + 32), (void *)(&internalformat), 4);
(void) memcpy((void *)(pc + 36), (void *)(&width), 4);
(void) memcpy((void *)(pc + 40), (void *)(&height), 4);
(void) memcpy((void *)(pc + 44), (void *)(&format), 4);
(void) memcpy((void *)(pc + 48), (void *)(&type), 4);
__glXSendLargeImage(gc, compsize, dim, width, height, 1, format, type, image, pc + 52, pc + 8);
}
    }
}

#define X_GLrop_ConvolutionFilter1D 4101
void __indirect_glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image)
{
    __glx_ConvolutionFilter_1D2D(X_GLrop_ConvolutionFilter1D, 1, target, internalformat, width, 1, format, type, image );
}

#define X_GLrop_ConvolutionFilter2D 4102
void __indirect_glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image)
{
    __glx_ConvolutionFilter_1D2D(X_GLrop_ConvolutionFilter2D, 2, target, internalformat, width, height, format, type, image );
}

#define X_GLrop_ConvolutionParameterf 4103
void __indirect_glConvolutionParameterf(GLenum target, GLenum pname, GLfloat params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_ConvolutionParameterf, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&params), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ConvolutionParameterfv 4104
void __indirect_glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glConvolutionParameterfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_ConvolutionParameterfv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ConvolutionParameteri 4105
void __indirect_glConvolutionParameteri(GLenum target, GLenum pname, GLint params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_ConvolutionParameteri, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&params), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ConvolutionParameteriv 4106
void __indirect_glConvolutionParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glConvolutionParameteriv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_ConvolutionParameteriv, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyConvolutionFilter1D 4107
void __indirect_glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_CopyConvolutionFilter1D, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyConvolutionFilter2D 4108
void __indirect_glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_CopyConvolutionFilter2D, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&height), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_GetConvolutionFilter 150
void __indirect_glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, GLvoid * image)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 16;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_convolution_filter_reply_t *reply = xcb_glx_get_convolution_filter_reply(c, xcb_glx_get_convolution_filter(c, gc->currentContextTag, target, format, type, state->storePack.swapEndian), NULL);
        if (reply->height == 0) { reply->height = 1; }
        __glEmptyImage(gc, 3, reply->width, reply->height, 1, format, type, xcb_glx_get_convolution_filter_data(reply), image);
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetConvolutionFilter, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        __glXReadPixelReply(dpy, gc, 2, 0, 0, 0, format, type, image, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetConvolutionFilterEXT 1
void gl_dispatch_stub_356(GLenum target, GLenum format, GLenum type, GLvoid * image)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETCONVOLUTIONFILTEREXTPROC p =
            (PFNGLGETCONVOLUTIONFILTEREXTPROC) disp_table[356];
    p(target, format, type, image);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 16;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetConvolutionFilterEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        __glXReadPixelReply(dpy, gc, 2, 0, 0, 0, format, type, image, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetConvolutionParameterfv 151
void __indirect_glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_convolution_parameterfv_reply_t *reply = xcb_glx_get_convolution_parameterfv_reply(c, xcb_glx_get_convolution_parameterfv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_convolution_parameterfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_convolution_parameterfv_data(reply), xcb_glx_get_convolution_parameterfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetConvolutionParameterfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetConvolutionParameterfvEXT 2
void gl_dispatch_stub_357(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETCONVOLUTIONPARAMETERFVEXTPROC p =
            (PFNGLGETCONVOLUTIONPARAMETERFVEXTPROC) disp_table[357];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetConvolutionParameterfvEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetConvolutionParameteriv 152
void __indirect_glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_convolution_parameteriv_reply_t *reply = xcb_glx_get_convolution_parameteriv_reply(c, xcb_glx_get_convolution_parameteriv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_convolution_parameteriv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_convolution_parameteriv_data(reply), xcb_glx_get_convolution_parameteriv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetConvolutionParameteriv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetConvolutionParameterivEXT 3
void gl_dispatch_stub_358(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETCONVOLUTIONPARAMETERIVEXTPROC p =
            (PFNGLGETCONVOLUTIONPARAMETERIVEXTPROC) disp_table[358];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetConvolutionParameterivEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetHistogram 154
void __indirect_glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 16;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_histogram_reply_t *reply = xcb_glx_get_histogram_reply(c, xcb_glx_get_histogram(c, gc->currentContextTag, target, reset, format, type, state->storePack.swapEndian), NULL);
        __glEmptyImage(gc, 3, reply->width, 1, 1, format, type, xcb_glx_get_histogram_data(reply), values);
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetHistogram, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        * (int8_t *)(pc + 13) = reset;
        __glXReadPixelReply(dpy, gc, 1, 0, 0, 0, format, type, values, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetHistogramEXT 5
void gl_dispatch_stub_361(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETHISTOGRAMEXTPROC p =
            (PFNGLGETHISTOGRAMEXTPROC) disp_table[361];
    p(target, reset, format, type, values);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 16;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetHistogramEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        * (int8_t *)(pc + 13) = reset;
        __glXReadPixelReply(dpy, gc, 1, 0, 0, 0, format, type, values, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetHistogramParameterfv 155
void __indirect_glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_histogram_parameterfv_reply_t *reply = xcb_glx_get_histogram_parameterfv_reply(c, xcb_glx_get_histogram_parameterfv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_histogram_parameterfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_histogram_parameterfv_data(reply), xcb_glx_get_histogram_parameterfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetHistogramParameterfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetHistogramParameterfvEXT 6
void gl_dispatch_stub_362(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETHISTOGRAMPARAMETERFVEXTPROC p =
            (PFNGLGETHISTOGRAMPARAMETERFVEXTPROC) disp_table[362];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetHistogramParameterfvEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetHistogramParameteriv 156
void __indirect_glGetHistogramParameteriv(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_histogram_parameteriv_reply_t *reply = xcb_glx_get_histogram_parameteriv_reply(c, xcb_glx_get_histogram_parameteriv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_histogram_parameteriv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_histogram_parameteriv_data(reply), xcb_glx_get_histogram_parameteriv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetHistogramParameteriv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetHistogramParameterivEXT 7
void gl_dispatch_stub_363(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETHISTOGRAMPARAMETERIVEXTPROC p =
            (PFNGLGETHISTOGRAMPARAMETERIVEXTPROC) disp_table[363];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetHistogramParameterivEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetMinmax 157
void __indirect_glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 16;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_minmax_reply_t *reply = xcb_glx_get_minmax_reply(c, xcb_glx_get_minmax(c, gc->currentContextTag, target, reset, format, type, state->storePack.swapEndian), NULL);
        __glEmptyImage(gc, 3, 2, 1, 1, format, type, xcb_glx_get_minmax_data(reply), values);
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMinmax, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        * (int8_t *)(pc + 13) = reset;
        __glXReadPixelReply(dpy, gc, 1, 2, 1, 1, format, type, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetMinmaxEXT 8
void gl_dispatch_stub_364(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETMINMAXEXTPROC p =
            (PFNGLGETMINMAXEXTPROC) disp_table[364];
    p(target, reset, format, type, values);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    const __GLXattribute * const state = gc->client_state_private;
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 16;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetMinmaxEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&format), 4);
(void) memcpy((void *)(pc + 8), (void *)(&type), 4);
        *(int32_t *)(pc + 12) = 0;
        * (int8_t *)(pc + 12) = state->storePack.swapEndian;
        * (int8_t *)(pc + 13) = reset;
        __glXReadPixelReply(dpy, gc, 1, 2, 1, 1, format, type, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetMinmaxParameterfv 158
void __indirect_glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_minmax_parameterfv_reply_t *reply = xcb_glx_get_minmax_parameterfv_reply(c, xcb_glx_get_minmax_parameterfv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_minmax_parameterfv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_minmax_parameterfv_data(reply), xcb_glx_get_minmax_parameterfv_data_length(reply) * sizeof(GLfloat));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMinmaxParameterfv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetMinmaxParameterfvEXT 9
void gl_dispatch_stub_365(GLenum target, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETMINMAXPARAMETERFVEXTPROC p =
            (PFNGLGETMINMAXPARAMETERFVEXTPROC) disp_table[365];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetMinmaxParameterfvEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLsop_GetMinmaxParameteriv 159
void __indirect_glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_minmax_parameteriv_reply_t *reply = xcb_glx_get_minmax_parameteriv_reply(c, xcb_glx_get_minmax_parameteriv(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_minmax_parameteriv_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_minmax_parameteriv_data(reply), xcb_glx_get_minmax_parameteriv_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetMinmaxParameteriv, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLvop_GetMinmaxParameterivEXT 10
void gl_dispatch_stub_366(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
    if (gc->isDirect) {
        const _glapi_proc *const disp_table = GET_DISPATCH();
        PFNGLGETMINMAXPARAMETERIVEXTPROC p =
            (PFNGLGETMINMAXPARAMETERIVEXTPROC) disp_table[366];
    p(target, pname, params);
    } else
#endif
    {
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetMinmaxParameterivEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}
}

#define X_GLrop_Histogram 4110
void __indirect_glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_Histogram, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&sink), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Minmax 4111
void __indirect_glMinmax(GLenum target, GLenum internalformat, GLboolean sink)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_Minmax, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&sink), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ResetHistogram 4112
void __indirect_glResetHistogram(GLenum target)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ResetHistogram, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ResetMinmax 4113
void __indirect_glResetMinmax(GLenum target)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ResetMinmax, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static void
__glx_TexImage_3D4D( unsigned opcode, unsigned dim, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLint border, GLenum format, GLenum type, const GLvoid * pixels )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (pixels != NULL) ? __glImageSize(width, height, depth, format, type, target) : 0;
    const GLuint cmdlen = 84 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, opcode, cmdlen);
(void) memcpy((void *)(gc->pc + 40), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 48), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 52), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 56), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 60), (void *)(&depth), 4);
(void) memcpy((void *)(gc->pc + 64), (void *)(&extent), 4);
(void) memcpy((void *)(gc->pc + 68), (void *)(&border), 4);
(void) memcpy((void *)(gc->pc + 72), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 76), (void *)(&type), 4);
(void) memcpy((void *)(gc->pc + 80), (void *)((pixels == NULL) ? one : zero), 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, dim, width, height, depth, format, type, pixels, gc->pc + 84, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_4D, default_pixel_store_4D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = opcode;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 44), (void *)(&target), 4);
(void) memcpy((void *)(pc + 48), (void *)(&level), 4);
(void) memcpy((void *)(pc + 52), (void *)(&internalformat), 4);
(void) memcpy((void *)(pc + 56), (void *)(&width), 4);
(void) memcpy((void *)(pc + 60), (void *)(&height), 4);
(void) memcpy((void *)(pc + 64), (void *)(&depth), 4);
(void) memcpy((void *)(pc + 68), (void *)(&extent), 4);
(void) memcpy((void *)(pc + 72), (void *)(&border), 4);
(void) memcpy((void *)(pc + 76), (void *)(&format), 4);
(void) memcpy((void *)(pc + 80), (void *)(&type), 4);
(void) memcpy((void *)(pc + 84), zero, 4);
__glXSendLargeImage(gc, compsize, dim, width, height, depth, format, type, pixels, pc + 88, pc + 8);
}
    }
}

#define X_GLrop_TexImage3D 4114
void __indirect_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    __glx_TexImage_3D4D(X_GLrop_TexImage3D, 3, target, level, internalformat, width, height, depth, 1, border, format, type, pixels );
}

static void
__glx_TexSubImage_3D4D( unsigned opcode, unsigned dim, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLenum format, GLenum type, const GLvoid * pixels )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = (pixels != NULL) ? __glImageSize(width, height, depth, format, type, target) : 0;
    const GLuint cmdlen = 92 + __GLX_PAD(compsize);
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, opcode, cmdlen);
(void) memcpy((void *)(gc->pc + 40), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 48), (void *)(&xoffset), 4);
(void) memcpy((void *)(gc->pc + 52), (void *)(&yoffset), 4);
(void) memcpy((void *)(gc->pc + 56), (void *)(&zoffset), 4);
(void) memcpy((void *)(gc->pc + 60), (void *)(&woffset), 4);
(void) memcpy((void *)(gc->pc + 64), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 68), (void *)(&height), 4);
(void) memcpy((void *)(gc->pc + 72), (void *)(&depth), 4);
(void) memcpy((void *)(gc->pc + 76), (void *)(&extent), 4);
(void) memcpy((void *)(gc->pc + 80), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 84), (void *)(&type), 4);
(void) memset((void *)(gc->pc + 88), 0, 4);
if (compsize > 0) {
    (*gc->fillImage)(gc, dim, width, height, depth, format, type, pixels, gc->pc + 92, gc->pc + 4);
} else {
    (void) memcpy( gc->pc + 4, default_pixel_store_4D, default_pixel_store_4D_size );
}
gc->pc += cmdlen;
if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = opcode;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 44), (void *)(&target), 4);
(void) memcpy((void *)(pc + 48), (void *)(&level), 4);
(void) memcpy((void *)(pc + 52), (void *)(&xoffset), 4);
(void) memcpy((void *)(pc + 56), (void *)(&yoffset), 4);
(void) memcpy((void *)(pc + 60), (void *)(&zoffset), 4);
(void) memcpy((void *)(pc + 64), (void *)(&woffset), 4);
(void) memcpy((void *)(pc + 68), (void *)(&width), 4);
(void) memcpy((void *)(pc + 72), (void *)(&height), 4);
(void) memcpy((void *)(pc + 76), (void *)(&depth), 4);
(void) memcpy((void *)(pc + 80), (void *)(&extent), 4);
(void) memcpy((void *)(pc + 84), (void *)(&format), 4);
(void) memcpy((void *)(pc + 88), (void *)(&type), 4);
(void) memset((void *)(pc + 92), 0, 4);
__glXSendLargeImage(gc, compsize, dim, width, height, depth, format, type, pixels, pc + 96, pc + 8);
}
    }
}

#define X_GLrop_TexSubImage3D 4115
void __indirect_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
{
    __glx_TexSubImage_3D4D(X_GLrop_TexSubImage3D, 3, target, level, xoffset, yoffset, zoffset, 1, width, height, depth, 1, format, type, pixels );
}

#define X_GLrop_CopyTexSubImage3D 4123
void __indirect_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_CopyTexSubImage3D, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&xoffset), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&yoffset), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&zoffset), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&height), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ActiveTextureARB 197
void __indirect_glActiveTextureARB(GLenum texture)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ActiveTextureARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&texture), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1dvARB 198
void __indirect_glMultiTexCoord1dARB(GLenum target, GLdouble s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord1dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1dvARB 198
void __indirect_glMultiTexCoord1dvARB(GLenum target, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord1dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1fvARB 199
void __indirect_glMultiTexCoord1fARB(GLenum target, GLfloat s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord1fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1fvARB 199
void __indirect_glMultiTexCoord1fvARB(GLenum target, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord1fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1ivARB 200
void __indirect_glMultiTexCoord1iARB(GLenum target, GLint s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord1ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1ivARB 200
void __indirect_glMultiTexCoord1ivARB(GLenum target, const GLint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord1ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1svARB 201
void __indirect_glMultiTexCoord1sARB(GLenum target, GLshort s)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord1svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1svARB 201
void __indirect_glMultiTexCoord1svARB(GLenum target, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord1svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2dvARB 202
void __indirect_glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_MultiTexCoord2dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2dvARB 202
void __indirect_glMultiTexCoord2dvARB(GLenum target, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_MultiTexCoord2dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v), 16);
(void) memcpy((void *)(gc->pc + 20), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2fvARB 203
void __indirect_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord2fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2fvARB 203
void __indirect_glMultiTexCoord2fvARB(GLenum target, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord2fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2ivARB 204
void __indirect_glMultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord2ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2ivARB 204
void __indirect_glMultiTexCoord2ivARB(GLenum target, const GLint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord2ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2svARB 205
void __indirect_glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord2svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&t), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2svARB 205
void __indirect_glMultiTexCoord2svARB(GLenum target, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_MultiTexCoord2svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3dvARB 206
void __indirect_glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
emit_header(gc->pc, X_GLrop_MultiTexCoord3dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3dvARB 206
void __indirect_glMultiTexCoord3dvARB(GLenum target, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
emit_header(gc->pc, X_GLrop_MultiTexCoord3dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v), 24);
(void) memcpy((void *)(gc->pc + 28), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3fvARB 207
void __indirect_glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_MultiTexCoord3fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3fvARB 207
void __indirect_glMultiTexCoord3fvARB(GLenum target, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_MultiTexCoord3fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 12);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3ivARB 208
void __indirect_glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_MultiTexCoord3ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3ivARB 208
void __indirect_glMultiTexCoord3ivARB(GLenum target, const GLint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_MultiTexCoord3ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 12);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3svARB 209
void __indirect_glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord3svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&t), 2);
(void) memcpy((void *)(gc->pc + 12), (void *)(&r), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3svARB 209
void __indirect_glMultiTexCoord3svARB(GLenum target, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord3svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 6);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4dvARB 210
void __indirect_glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_MultiTexCoord4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&q), 8);
(void) memcpy((void *)(gc->pc + 36), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4dvARB 210
void __indirect_glMultiTexCoord4dvARB(GLenum target, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_MultiTexCoord4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v), 32);
(void) memcpy((void *)(gc->pc + 36), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4fvARB 211
void __indirect_glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_MultiTexCoord4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&q), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4fvARB 211
void __indirect_glMultiTexCoord4fvARB(GLenum target, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_MultiTexCoord4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4ivARB 212
void __indirect_glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_MultiTexCoord4ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&q), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4ivARB 212
void __indirect_glMultiTexCoord4ivARB(GLenum target, const GLint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_MultiTexCoord4ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4svARB 213
void __indirect_glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord4svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&t), 2);
(void) memcpy((void *)(gc->pc + 12), (void *)(&r), 2);
(void) memcpy((void *)(gc->pc + 14), (void *)(&q), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4svARB 213
void __indirect_glMultiTexCoord4svARB(GLenum target, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_MultiTexCoord4svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SampleCoverageARB 229
void __indirect_glSampleCoverageARB(GLclampf value, GLboolean invert)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_SampleCoverageARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&value), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&invert), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_GetProgramStringARB 1308
void __indirect_glGetProgramStringARB(GLenum target, GLenum pname, GLvoid * string)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramStringARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 1, string, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetProgramivARB 1307
void __indirect_glGetProgramivARB(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramivARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLrop_ProgramEnvParameter4dvARB 4185
void __indirect_glProgramEnvParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44;
emit_header(gc->pc, X_GLrop_ProgramEnvParameter4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&z), 8);
(void) memcpy((void *)(gc->pc + 36), (void *)(&w), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramEnvParameter4dvARB 4185
void __indirect_glProgramEnvParameter4dvARB(GLenum target, GLuint index, const GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44;
emit_header(gc->pc, X_GLrop_ProgramEnvParameter4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), 32);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramEnvParameter4fvARB 4184
void __indirect_glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_ProgramEnvParameter4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramEnvParameter4fvARB 4184
void __indirect_glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_ProgramEnvParameter4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramLocalParameter4dvARB 4216
void __indirect_glProgramLocalParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44;
emit_header(gc->pc, X_GLrop_ProgramLocalParameter4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&z), 8);
(void) memcpy((void *)(gc->pc + 36), (void *)(&w), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramLocalParameter4dvARB 4216
void __indirect_glProgramLocalParameter4dvARB(GLenum target, GLuint index, const GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44;
emit_header(gc->pc, X_GLrop_ProgramLocalParameter4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), 32);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramLocalParameter4fvARB 4215
void __indirect_glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_ProgramLocalParameter4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramLocalParameter4fvARB 4215
void __indirect_glProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_ProgramLocalParameter4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ProgramStringARB 4217
void __indirect_glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid * string)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((len >= 0) && (gc->currentDpy != NULL), 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_ProgramStringARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&format), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&len), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(string), len);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_ProgramStringARB;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 8), (void *)(&target), 4);
(void) memcpy((void *)(pc + 12), (void *)(&format), 4);
(void) memcpy((void *)(pc + 16), (void *)(&len), 4);
    __glXSendLargeCommand(gc, pc, 20, string, len);
}
    }
}

#define X_GLrop_VertexAttrib1dvARB 4197
void __indirect_glVertexAttrib1dARB(GLuint index, GLdouble x)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib1dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1dvARB 4197
void __indirect_glVertexAttrib1dvARB(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib1dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1fvARB 4193
void __indirect_glVertexAttrib1fARB(GLuint index, GLfloat x)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1fvARB 4193
void __indirect_glVertexAttrib1fvARB(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1svARB 4189
void __indirect_glVertexAttrib1sARB(GLuint index, GLshort x)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1svARB 4189
void __indirect_glVertexAttrib1svARB(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2dvARB 4198
void __indirect_glVertexAttrib2dARB(GLuint index, GLdouble x, GLdouble y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib2dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2dvARB 4198
void __indirect_glVertexAttrib2dvARB(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib2dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2fvARB 4194
void __indirect_glVertexAttrib2fARB(GLuint index, GLfloat x, GLfloat y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib2fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2fvARB 4194
void __indirect_glVertexAttrib2fvARB(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib2fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2svARB 4190
void __indirect_glVertexAttrib2sARB(GLuint index, GLshort x, GLshort y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib2svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&y), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2svARB 4190
void __indirect_glVertexAttrib2svARB(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib2svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3dvARB 4199
void __indirect_glVertexAttrib3dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
emit_header(gc->pc, X_GLrop_VertexAttrib3dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 24), (void *)(&z), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3dvARB 4199
void __indirect_glVertexAttrib3dvARB(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
emit_header(gc->pc, X_GLrop_VertexAttrib3dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 24);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3fvARB 4195
void __indirect_glVertexAttrib3fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_VertexAttrib3fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3fvARB 4195
void __indirect_glVertexAttrib3fvARB(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_VertexAttrib3fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 12);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3svARB 4191
void __indirect_glVertexAttrib3sARB(GLuint index, GLshort x, GLshort y, GLshort z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib3svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3svARB 4191
void __indirect_glVertexAttrib3svARB(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib3svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 6);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4NbvARB 4235
void __indirect_glVertexAttrib4NbvARB(GLuint index, const GLbyte * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib4NbvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4NivARB 4237
void __indirect_glVertexAttrib4NivARB(GLuint index, const GLint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4NivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4NsvARB 4236
void __indirect_glVertexAttrib4NsvARB(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib4NsvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4NubvARB 4201
void __indirect_glVertexAttrib4NubARB(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib4NubvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 1);
(void) memcpy((void *)(gc->pc + 9), (void *)(&y), 1);
(void) memcpy((void *)(gc->pc + 10), (void *)(&z), 1);
(void) memcpy((void *)(gc->pc + 11), (void *)(&w), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4NubvARB 4201
void __indirect_glVertexAttrib4NubvARB(GLuint index, const GLubyte * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib4NubvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4NuivARB 4239
void __indirect_glVertexAttrib4NuivARB(GLuint index, const GLuint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4NuivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4NusvARB 4238
void __indirect_glVertexAttrib4NusvARB(GLuint index, const GLushort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib4NusvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4bvARB 4230
void __indirect_glVertexAttrib4bvARB(GLuint index, const GLbyte * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib4bvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4dvARB 4200
void __indirect_glVertexAttrib4dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_VertexAttrib4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 24), (void *)(&z), 8);
(void) memcpy((void *)(gc->pc + 32), (void *)(&w), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4dvARB 4200
void __indirect_glVertexAttrib4dvARB(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_VertexAttrib4dvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 32);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4fvARB 4196
void __indirect_glVertexAttrib4fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4fvARB 4196
void __indirect_glVertexAttrib4fvARB(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4fvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4ivARB 4231
void __indirect_glVertexAttrib4ivARB(GLuint index, const GLint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4ivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4svARB 4192
void __indirect_glVertexAttrib4sARB(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib4svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 2);
(void) memcpy((void *)(gc->pc + 14), (void *)(&w), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4svARB 4192
void __indirect_glVertexAttrib4svARB(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib4svARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4ubvARB 4232
void __indirect_glVertexAttrib4ubvARB(GLuint index, const GLubyte * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib4ubvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4uivARB 4234
void __indirect_glVertexAttrib4uivARB(GLuint index, const GLuint * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4uivARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4usvARB 4233
void __indirect_glVertexAttrib4usvARB(GLuint index, const GLushort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib4usvARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BeginQueryARB 231
void __indirect_glBeginQueryARB(GLenum target, GLuint id)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_BeginQueryARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&id), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_DeleteQueriesARB 161
void __indirect_glDeleteQueriesARB(GLsizei n, const GLuint * ids)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
#endif
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_delete_queries_arb(c, gc->currentContextTag, n, ids);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_DeleteQueriesARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
(void) memcpy((void *)(pc + 4), (void *)(ids), (n * 4));
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLrop_EndQueryARB 232
void __indirect_glEndQueryARB(GLenum target)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_EndQueryARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_GenQueriesARB 162
void __indirect_glGenQueriesARB(GLsizei n, GLuint * ids)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_gen_queries_arb_reply_t *reply = xcb_glx_gen_queries_arb_reply(c, xcb_glx_gen_queries_arb(c, gc->currentContextTag, n), NULL);
        (void)memcpy(ids, xcb_glx_gen_queries_arb_data(reply), xcb_glx_gen_queries_arb_data_length(reply) * sizeof(GLuint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GenQueriesARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) __glXReadReply(dpy, 4, ids, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetQueryObjectivARB 165
void __indirect_glGetQueryObjectivARB(GLuint id, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_query_objectiv_arb_reply_t *reply = xcb_glx_get_query_objectiv_arb_reply(c, xcb_glx_get_query_objectiv_arb(c, gc->currentContextTag, id, pname), NULL);
        if (xcb_glx_get_query_objectiv_arb_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_query_objectiv_arb_data(reply), xcb_glx_get_query_objectiv_arb_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetQueryObjectivARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&id), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetQueryObjectuivARB 166
void __indirect_glGetQueryObjectuivARB(GLuint id, GLenum pname, GLuint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_query_objectuiv_arb_reply_t *reply = xcb_glx_get_query_objectuiv_arb_reply(c, xcb_glx_get_query_objectuiv_arb(c, gc->currentContextTag, id, pname), NULL);
        if (xcb_glx_get_query_objectuiv_arb_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_query_objectuiv_arb_data(reply), xcb_glx_get_query_objectuiv_arb_data_length(reply) * sizeof(GLuint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetQueryObjectuivARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&id), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_GetQueryivARB 164
void __indirect_glGetQueryivARB(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
#ifndef USE_XCB
    const GLuint cmdlen = 8;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_get_queryiv_arb_reply_t *reply = xcb_glx_get_queryiv_arb_reply(c, xcb_glx_get_queryiv_arb(c, gc->currentContextTag, target, pname), NULL);
        if (xcb_glx_get_queryiv_arb_data_length(reply) == 0)
            (void)memcpy(params, &reply->datum, sizeof(reply->datum));
        else
            (void)memcpy(params, xcb_glx_get_queryiv_arb_data(reply), xcb_glx_get_queryiv_arb_data_length(reply) * sizeof(GLint));
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_GetQueryivARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return;
}

#define X_GLsop_IsQueryARB 163
GLboolean __indirect_glIsQueryARB(GLuint id)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
#ifndef USE_XCB
    const GLuint cmdlen = 4;
#endif
    if (__builtin_expect(dpy != NULL, 1)) {
#ifdef USE_XCB
        xcb_connection_t *c = XGetXCBConnection(dpy);
        (void) __glXFlushRenderBuffer(gc, gc->pc);
        xcb_glx_is_query_arb_reply_t *reply = xcb_glx_is_query_arb_reply(c, xcb_glx_is_query_arb(c, gc->currentContextTag, id), NULL);
        retval = reply->ret_val;
        free(reply);
#else
        GLubyte const * pc = __glXSetupSingleRequest(gc, X_GLsop_IsQueryARB, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&id), 4);
        retval = (GLboolean) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
#endif /* USE_XCB */
    }
    return retval;
}

#define X_GLrop_DrawBuffersARB 233
void __indirect_glDrawBuffersARB(GLsizei n, const GLenum * bufs)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (gc->currentDpy != NULL), 1)) {
if (cmdlen <= gc->maxSmallRenderCommandSize) {
    if ( (gc->pc + cmdlen) > gc->bufEnd ) {
        (void) __glXFlushRenderBuffer(gc, gc->pc);
    }
emit_header(gc->pc, X_GLrop_DrawBuffersARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(bufs), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
else {
const GLint op = X_GLrop_DrawBuffersARB;
const GLuint cmdlenLarge = cmdlen + 4;
GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);
(void) memcpy((void *)(pc + 4), (void *)(&op), 4);
(void) memcpy((void *)(pc + 8), (void *)(&n), 4);
    __glXSendLargeCommand(gc, pc, 12, bufs, (n * 4));
}
    }
}

#define X_GLrop_ClampColorARB 234
void __indirect_glClampColorARB(GLenum target, GLenum clamp)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_ClampColorARB, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&clamp), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RenderbufferStorageMultisample 4331
void __indirect_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_RenderbufferStorageMultisample, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&samples), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&height), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SampleMaskSGIS 2048
void __indirect_glSampleMaskSGIS(GLclampf value, GLboolean invert)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_SampleMaskSGIS, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&value), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&invert), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SamplePatternSGIS 2049
void __indirect_glSamplePatternSGIS(GLenum pattern)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_SamplePatternSGIS, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pattern), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointParameterfEXT 2065
void __indirect_glPointParameterfEXT(GLenum pname, GLfloat param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_PointParameterfEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointParameterfvEXT 2066
void __indirect_glPointParameterfvEXT(GLenum pname, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glPointParameterfvEXT_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_PointParameterfvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3bvEXT 4126
void __indirect_glSecondaryColor3bEXT(GLbyte red, GLbyte green, GLbyte blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_SecondaryColor3bvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3bvEXT 4126
void __indirect_glSecondaryColor3bvEXT(const GLbyte * v)
{
    generic_3_byte( X_GLrop_SecondaryColor3bvEXT, v );
}

#define X_GLrop_SecondaryColor3dvEXT 4130
void __indirect_glSecondaryColor3dEXT(GLdouble red, GLdouble green, GLdouble blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_SecondaryColor3dvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&green), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&blue), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3dvEXT 4130
void __indirect_glSecondaryColor3dvEXT(const GLdouble * v)
{
    generic_24_byte( X_GLrop_SecondaryColor3dvEXT, v );
}

#define X_GLrop_SecondaryColor3fvEXT 4129
void __indirect_glSecondaryColor3fEXT(GLfloat red, GLfloat green, GLfloat blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_SecondaryColor3fvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3fvEXT 4129
void __indirect_glSecondaryColor3fvEXT(const GLfloat * v)
{
    generic_12_byte( X_GLrop_SecondaryColor3fvEXT, v );
}

#define X_GLrop_SecondaryColor3ivEXT 4128
void __indirect_glSecondaryColor3iEXT(GLint red, GLint green, GLint blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_SecondaryColor3ivEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3ivEXT 4128
void __indirect_glSecondaryColor3ivEXT(const GLint * v)
{
    generic_12_byte( X_GLrop_SecondaryColor3ivEXT, v );
}

#define X_GLrop_SecondaryColor3svEXT 4127
void __indirect_glSecondaryColor3sEXT(GLshort red, GLshort green, GLshort blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_SecondaryColor3svEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3svEXT 4127
void __indirect_glSecondaryColor3svEXT(const GLshort * v)
{
    generic_6_byte( X_GLrop_SecondaryColor3svEXT, v );
}

#define X_GLrop_SecondaryColor3ubvEXT 4131
void __indirect_glSecondaryColor3ubEXT(GLubyte red, GLubyte green, GLubyte blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_SecondaryColor3ubvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
(void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
(void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3ubvEXT 4131
void __indirect_glSecondaryColor3ubvEXT(const GLubyte * v)
{
    generic_3_byte( X_GLrop_SecondaryColor3ubvEXT, v );
}

#define X_GLrop_SecondaryColor3uivEXT 4133
void __indirect_glSecondaryColor3uiEXT(GLuint red, GLuint green, GLuint blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_SecondaryColor3uivEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3uivEXT 4133
void __indirect_glSecondaryColor3uivEXT(const GLuint * v)
{
    generic_12_byte( X_GLrop_SecondaryColor3uivEXT, v );
}

#define X_GLrop_SecondaryColor3usvEXT 4132
void __indirect_glSecondaryColor3usEXT(GLushort red, GLushort green, GLushort blue)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_SecondaryColor3usvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
(void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
(void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3usvEXT 4132
void __indirect_glSecondaryColor3usvEXT(const GLushort * v)
{
    generic_6_byte( X_GLrop_SecondaryColor3usvEXT, v );
}

#define X_GLrop_FogCoorddvEXT 4125
void __indirect_glFogCoorddEXT(GLdouble coord)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_FogCoorddvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FogCoorddvEXT 4125
void __indirect_glFogCoorddvEXT(const GLdouble * coord)
{
    generic_8_byte( X_GLrop_FogCoorddvEXT, coord );
}

#define X_GLrop_FogCoordfvEXT 4124
void __indirect_glFogCoordfEXT(GLfloat coord)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_FogCoordfvEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FogCoordfvEXT 4124
void __indirect_glFogCoordfvEXT(const GLfloat * coord)
{
    generic_4_byte( X_GLrop_FogCoordfvEXT, coord );
}

#define X_GLrop_BlendFuncSeparateEXT 4134
void __indirect_glBlendFuncSeparateEXT(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_BlendFuncSeparateEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&sfactorRGB), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&dfactorRGB), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&sfactorAlpha), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&dfactorAlpha), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_WindowPos3fvMESA 230
void __indirect_glWindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_WindowPos3fvMESA, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_WindowPos3fvMESA 230
void __indirect_glWindowPos3fvMESA(const GLfloat * v)
{
    generic_12_byte( X_GLrop_WindowPos3fvMESA, v );
}

#define X_GLvop_AreProgramsResidentNV 1293
GLboolean __indirect_glAreProgramsResidentNV(GLsizei n, const GLuint * ids, GLboolean * residences)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return 0;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_AreProgramsResidentNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
(void) memcpy((void *)(pc + 4), (void *)(ids), (n * 4));
        retval = (GLboolean) __glXReadReply(dpy, 1, residences, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_BindProgramNV 4180
void __indirect_glBindProgramNV(GLenum target, GLuint program)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_BindProgramNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&program), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_DeleteProgramsNV 1294
void __indirect_glDeleteProgramsNV(GLsizei n, const GLuint * programs)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivate, X_GLvop_DeleteProgramsNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
(void) memcpy((void *)(pc + 4), (void *)(programs), (n * 4));
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLrop_ExecuteProgramNV 4181
void __indirect_glExecuteProgramNV(GLenum target, GLuint id, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_ExecuteProgramNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&id), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(params), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_GenProgramsNV 1295
void __indirect_glGenProgramsNV(GLsizei n, GLuint * programs)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GenProgramsNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) __glXReadReply(dpy, 4, programs, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetProgramParameterdvNV 1297
void __indirect_glGetProgramParameterdvNV(GLenum target, GLuint index, GLenum pname, GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 12;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramParameterdvNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 8, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetProgramParameterfvNV 1296
void __indirect_glGetProgramParameterfvNV(GLenum target, GLuint index, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 12;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramParameterfvNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetProgramStringNV 1299
void __indirect_glGetProgramStringNV(GLuint id, GLenum pname, GLubyte * program)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramStringNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&id), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 1, program, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetProgramivNV 1298
void __indirect_glGetProgramivNV(GLuint id, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramivNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&id), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetTrackMatrixivNV 1300
void __indirect_glGetTrackMatrixivNV(GLenum target, GLuint address, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 12;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetTrackMatrixivNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&address), 4);
(void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetVertexAttribdvNV 1301
void __indirect_glGetVertexAttribdvNV(GLuint index, GLenum pname, GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetVertexAttribdvNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&index), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 8, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetVertexAttribfvNV 1302
void __indirect_glGetVertexAttribfvNV(GLuint index, GLenum pname, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetVertexAttribfvNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&index), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetVertexAttribivNV 1303
void __indirect_glGetVertexAttribivNV(GLuint index, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetVertexAttribivNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&index), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_IsProgramNV 1304
GLboolean __indirect_glIsProgramNV(GLuint program)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_IsProgramNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&program), 4);
        retval = (GLboolean) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_LoadProgramNV 4183
void __indirect_glLoadProgramNV(GLenum target, GLuint id, GLsizei len, const GLubyte * program)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(len >= 0, 1)) {
emit_header(gc->pc, X_GLrop_LoadProgramNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&id), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&len), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(program), len);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_ProgramParameters4dvNV 4187
void __indirect_glProgramParameters4dvNV(GLenum target, GLuint index, GLsizei num, const GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16 + __GLX_PAD((num * 32));
    if (num < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(num >= 0, 1)) {
emit_header(gc->pc, X_GLrop_ProgramParameters4dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&num), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(params), (num * 32));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_ProgramParameters4fvNV 4186
void __indirect_glProgramParameters4fvNV(GLenum target, GLuint index, GLsizei num, const GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16 + __GLX_PAD((num * 16));
    if (num < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(num >= 0, 1)) {
emit_header(gc->pc, X_GLrop_ProgramParameters4fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&num), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(params), (num * 16));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_RequestResidentProgramsNV 4182
void __indirect_glRequestResidentProgramsNV(GLsizei n, const GLuint * ids)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_RequestResidentProgramsNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(ids), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_TrackMatrixNV 4188
void __indirect_glTrackMatrixNV(GLenum target, GLuint address, GLenum matrix, GLenum transform)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_TrackMatrixNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&address), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&matrix), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&transform), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1dvNV 4273
void __indirect_glVertexAttrib1dNV(GLuint index, GLdouble x)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib1dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1dvNV 4273
void __indirect_glVertexAttrib1dvNV(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib1dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1fvNV 4269
void __indirect_glVertexAttrib1fNV(GLuint index, GLfloat x)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1fvNV 4269
void __indirect_glVertexAttrib1fvNV(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1svNV 4265
void __indirect_glVertexAttrib1sNV(GLuint index, GLshort x)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib1svNV 4265
void __indirect_glVertexAttrib1svNV(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib1svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2dvNV 4274
void __indirect_glVertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib2dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2dvNV 4274
void __indirect_glVertexAttrib2dvNV(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib2dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2fvNV 4270
void __indirect_glVertexAttrib2fNV(GLuint index, GLfloat x, GLfloat y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib2fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2fvNV 4270
void __indirect_glVertexAttrib2fvNV(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib2fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2svNV 4266
void __indirect_glVertexAttrib2sNV(GLuint index, GLshort x, GLshort y)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib2svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&y), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib2svNV 4266
void __indirect_glVertexAttrib2svNV(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib2svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3dvNV 4275
void __indirect_glVertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
emit_header(gc->pc, X_GLrop_VertexAttrib3dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 24), (void *)(&z), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3dvNV 4275
void __indirect_glVertexAttrib3dvNV(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
emit_header(gc->pc, X_GLrop_VertexAttrib3dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 24);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3fvNV 4271
void __indirect_glVertexAttrib3fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_VertexAttrib3fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&z), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3fvNV 4271
void __indirect_glVertexAttrib3fvNV(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_VertexAttrib3fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 12);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3svNV 4267
void __indirect_glVertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib3svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib3svNV 4267
void __indirect_glVertexAttrib3svNV(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib3svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 6);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4dvNV 4276
void __indirect_glVertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_VertexAttrib4dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 24), (void *)(&z), 8);
(void) memcpy((void *)(gc->pc + 32), (void *)(&w), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4dvNV 4276
void __indirect_glVertexAttrib4dvNV(GLuint index, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
emit_header(gc->pc, X_GLrop_VertexAttrib4dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 32);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4fvNV 4272
void __indirect_glVertexAttrib4fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&w), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4fvNV 4272
void __indirect_glVertexAttrib4fvNV(GLuint index, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_VertexAttrib4fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4svNV 4268
void __indirect_glVertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib4svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 2);
(void) memcpy((void *)(gc->pc + 10), (void *)(&y), 2);
(void) memcpy((void *)(gc->pc + 12), (void *)(&z), 2);
(void) memcpy((void *)(gc->pc + 14), (void *)(&w), 2);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4svNV 4268
void __indirect_glVertexAttrib4svNV(GLuint index, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
emit_header(gc->pc, X_GLrop_VertexAttrib4svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4ubvNV 4277
void __indirect_glVertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib4ubvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&x), 1);
(void) memcpy((void *)(gc->pc + 9), (void *)(&y), 1);
(void) memcpy((void *)(gc->pc + 10), (void *)(&z), 1);
(void) memcpy((void *)(gc->pc + 11), (void *)(&w), 1);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttrib4ubvNV 4277
void __indirect_glVertexAttrib4ubvNV(GLuint index, const GLubyte * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_VertexAttrib4ubvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_VertexAttribs1dvNV 4210
void __indirect_glVertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 8));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs1dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 8));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs1fvNV 4206
void __indirect_glVertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs1fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs1svNV 4202
void __indirect_glVertexAttribs1svNV(GLuint index, GLsizei n, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 2));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs1svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 2));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs2dvNV 4211
void __indirect_glVertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 16));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs2dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 16));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs2fvNV 4207
void __indirect_glVertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 8));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs2fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 8));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs2svNV 4203
void __indirect_glVertexAttribs2svNV(GLuint index, GLsizei n, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs2svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs3dvNV 4212
void __indirect_glVertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 24));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs3dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 24));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs3fvNV 4208
void __indirect_glVertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 12));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs3fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 12));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs3svNV 4204
void __indirect_glVertexAttribs3svNV(GLuint index, GLsizei n, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 6));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs3svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 6));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs4dvNV 4213
void __indirect_glVertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 32));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs4dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 32));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs4fvNV 4209
void __indirect_glVertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 16));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs4fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 16));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs4svNV 4205
void __indirect_glVertexAttribs4svNV(GLuint index, GLsizei n, const GLshort * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 8));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs4svNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 8));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_VertexAttribs4ubvNV 4214
void __indirect_glVertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_VertexAttribs4ubvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&index), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_PointParameteriNV 4221
void __indirect_glPointParameteriNV(GLenum pname, GLint param)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_PointParameteriNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointParameterivNV 4222
void __indirect_glPointParameterivNV(GLenum pname, const GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glPointParameterivNV_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
emit_header(gc->pc, X_GLrop_PointParameterivNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ActiveStencilFaceEXT 4220
void __indirect_glActiveStencilFaceEXT(GLenum face)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_ActiveStencilFaceEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_GetProgramNamedParameterdvNV 1311
void __indirect_glGetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte * name, GLdouble * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((len >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramNamedParameterdvNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&id), 4);
(void) memcpy((void *)(pc + 4), (void *)(&len), 4);
(void) memcpy((void *)(pc + 8), (void *)(name), len);
        (void) __glXReadReply(dpy, 8, params, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetProgramNamedParameterfvNV 1310
void __indirect_glGetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte * name, GLfloat * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((len >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetProgramNamedParameterfvNV, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&id), 4);
(void) memcpy((void *)(pc + 4), (void *)(&len), 4);
(void) memcpy((void *)(pc + 8), (void *)(name), len);
        (void) __glXReadReply(dpy, 4, params, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLrop_ProgramNamedParameter4dvNV 4219
void __indirect_glProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(len >= 0, 1)) {
emit_header(gc->pc, X_GLrop_ProgramNamedParameter4dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
(void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
(void) memcpy((void *)(gc->pc + 28), (void *)(&w), 8);
(void) memcpy((void *)(gc->pc + 36), (void *)(&id), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&len), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(name), len);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_ProgramNamedParameter4dvNV 4219
void __indirect_glProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(len >= 0, 1)) {
emit_header(gc->pc, X_GLrop_ProgramNamedParameter4dvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(v), 32);
(void) memcpy((void *)(gc->pc + 36), (void *)(&id), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&len), 4);
(void) memcpy((void *)(gc->pc + 44), (void *)(name), len);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_ProgramNamedParameter4fvNV 4218
void __indirect_glProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(len >= 0, 1)) {
emit_header(gc->pc, X_GLrop_ProgramNamedParameter4fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&id), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&len), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&z), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&w), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(name), len);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_ProgramNamedParameter4fvNV 4218
void __indirect_glProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28 + __GLX_PAD(len);
    if (len < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(len >= 0, 1)) {
emit_header(gc->pc, X_GLrop_ProgramNamedParameter4fvNV, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&id), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&len), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(v), 16);
(void) memcpy((void *)(gc->pc + 28), (void *)(name), len);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_BlendEquationSeparateEXT 4228
void __indirect_glBlendEquationSeparateEXT(GLenum modeRGB, GLenum modeA)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_BlendEquationSeparateEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&modeRGB), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&modeA), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BindFramebufferEXT 4319
void __indirect_glBindFramebufferEXT(GLenum target, GLuint framebuffer)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_BindFramebufferEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&framebuffer), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BindRenderbufferEXT 4316
void __indirect_glBindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
emit_header(gc->pc, X_GLrop_BindRenderbufferEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&renderbuffer), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_CheckFramebufferStatusEXT 1427
GLenum __indirect_glCheckFramebufferStatusEXT(GLenum target)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLenum retval = (GLenum) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_CheckFramebufferStatusEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        retval = (GLenum) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_DeleteFramebuffersEXT 4320
void __indirect_glDeleteFramebuffersEXT(GLsizei n, const GLuint * framebuffers)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_DeleteFramebuffersEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(framebuffers), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_DeleteRenderbuffersEXT 4317
void __indirect_glDeleteRenderbuffersEXT(GLsizei n, const GLuint * renderbuffers)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8 + __GLX_PAD((n * 4));
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect(n >= 0, 1)) {
emit_header(gc->pc, X_GLrop_DeleteRenderbuffersEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(renderbuffers), (n * 4));
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_FramebufferRenderbufferEXT 4324
void __indirect_glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_FramebufferRenderbufferEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&attachment), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&renderbuffertarget), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&renderbuffer), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FramebufferTexture1DEXT 4321
void __indirect_glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_FramebufferTexture1DEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&attachment), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&textarget), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&texture), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&level), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FramebufferTexture2DEXT 4322
void __indirect_glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_FramebufferTexture2DEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&attachment), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&textarget), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&texture), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&level), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FramebufferTexture3DEXT 4323
void __indirect_glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
emit_header(gc->pc, X_GLrop_FramebufferTexture3DEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&attachment), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&textarget), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&texture), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&zoffset), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_GenFramebuffersEXT 1426
void __indirect_glGenFramebuffersEXT(GLsizei n, GLuint * framebuffers)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GenFramebuffersEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) __glXReadReply(dpy, 4, framebuffers, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GenRenderbuffersEXT 1423
void __indirect_glGenRenderbuffersEXT(GLsizei n, GLuint * renderbuffers)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (n < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GenRenderbuffersEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) __glXReadReply(dpy, 4, renderbuffers, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLrop_GenerateMipmapEXT 4325
void __indirect_glGenerateMipmapEXT(GLenum target)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
emit_header(gc->pc, X_GLrop_GenerateMipmapEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_GetFramebufferAttachmentParameterivEXT 1428
void __indirect_glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 12;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetFramebufferAttachmentParameterivEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&attachment), 4);
(void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_GetRenderbufferParameterivEXT 1424
void __indirect_glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint * params)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_GetRenderbufferParameterivEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&target), 4);
(void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) __glXReadReply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_IsFramebufferEXT 1425
GLboolean __indirect_glIsFramebufferEXT(GLuint framebuffer)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_IsFramebufferEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&framebuffer), 4);
        retval = (GLboolean) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLvop_IsRenderbufferEXT 1422
GLboolean __indirect_glIsRenderbufferEXT(GLuint renderbuffer)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply, X_GLvop_IsRenderbufferEXT, cmdlen);
(void) memcpy((void *)(pc + 0), (void *)(&renderbuffer), 4);
        retval = (GLboolean) __glXReadReply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_RenderbufferStorageEXT 4318
void __indirect_glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
emit_header(gc->pc, X_GLrop_RenderbufferStorageEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&width), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&height), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BlitFramebufferEXT 4330
void __indirect_glBlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44;
emit_header(gc->pc, X_GLrop_BlitFramebufferEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&srcX0), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&srcY0), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&srcX1), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&srcY1), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&dstX0), 4);
(void) memcpy((void *)(gc->pc + 24), (void *)(&dstY0), 4);
(void) memcpy((void *)(gc->pc + 28), (void *)(&dstX1), 4);
(void) memcpy((void *)(gc->pc + 32), (void *)(&dstY1), 4);
(void) memcpy((void *)(gc->pc + 36), (void *)(&mask), 4);
(void) memcpy((void *)(gc->pc + 40), (void *)(&filter), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FramebufferTextureLayerEXT 237
void __indirect_glFramebufferTextureLayerEXT(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
emit_header(gc->pc, X_GLrop_FramebufferTextureLayerEXT, cmdlen);
(void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
(void) memcpy((void *)(gc->pc + 8), (void *)(&attachment), 4);
(void) memcpy((void *)(gc->pc + 12), (void *)(&texture), 4);
(void) memcpy((void *)(gc->pc + 16), (void *)(&level), 4);
(void) memcpy((void *)(gc->pc + 20), (void *)(&layer), 4);
gc->pc += cmdlen;
if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}


#ifdef GLX_SHARED_GLAPI

static const struct proc_pair
{
   const char *name;
   _glapi_proc proc;
} proc_pairs[20] = {
   { "AreTexturesResidentEXT", (_glapi_proc) glAreTexturesResidentEXT },
   { "DeleteTexturesEXT", (_glapi_proc) glDeleteTexturesEXT },
   { "GenTexturesEXT", (_glapi_proc) glGenTexturesEXT },
   { "GetColorTableEXT", (_glapi_proc) glGetColorTableEXT },
   { "GetColorTableParameterfvEXT", (_glapi_proc) glGetColorTableParameterfvEXT },
   { "GetColorTableParameterfvSGI", (_glapi_proc) glGetColorTableParameterfvEXT },
   { "GetColorTableParameterivEXT", (_glapi_proc) glGetColorTableParameterivEXT },
   { "GetColorTableParameterivSGI", (_glapi_proc) glGetColorTableParameterivEXT },
   { "GetColorTableSGI", (_glapi_proc) glGetColorTableEXT },
   { "GetConvolutionFilterEXT", (_glapi_proc) gl_dispatch_stub_356 },
   { "GetConvolutionParameterfvEXT", (_glapi_proc) gl_dispatch_stub_357 },
   { "GetConvolutionParameterivEXT", (_glapi_proc) gl_dispatch_stub_358 },
   { "GetHistogramEXT", (_glapi_proc) gl_dispatch_stub_361 },
   { "GetHistogramParameterfvEXT", (_glapi_proc) gl_dispatch_stub_362 },
   { "GetHistogramParameterivEXT", (_glapi_proc) gl_dispatch_stub_363 },
   { "GetMinmaxEXT", (_glapi_proc) gl_dispatch_stub_364 },
   { "GetMinmaxParameterfvEXT", (_glapi_proc) gl_dispatch_stub_365 },
   { "GetMinmaxParameterivEXT", (_glapi_proc) gl_dispatch_stub_366 },
   { "GetSeparableFilterEXT", (_glapi_proc) gl_dispatch_stub_359 },
   { "IsTextureEXT", (_glapi_proc) glIsTextureEXT }
};

static int
__indirect_get_proc_compare(const void *key, const void *memb)
{
   const struct proc_pair *pair = (const struct proc_pair *) memb;
   return strcmp((const char *) key, pair->name);
}

_glapi_proc
__indirect_get_proc_address(const char *name)
{
   const struct proc_pair *pair;
   
   /* skip "gl" */
   name += 2;

   pair = (const struct proc_pair *) bsearch((const void *) name,
      (const void *) proc_pairs, ARRAY_SIZE(proc_pairs), sizeof(proc_pairs[0]),
      __indirect_get_proc_compare);

   return (pair) ? pair->proc : NULL;
}

#endif /* GLX_SHARED_GLAPI */


#  undef FASTCALL
#  undef NOINLINE
