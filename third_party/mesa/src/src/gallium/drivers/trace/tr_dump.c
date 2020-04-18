/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/**
 * @file
 * Trace dumping functions.
 *
 * For now we just use standard XML for dumping the trace calls, as this is
 * simple to write, parse, and visually inspect, but the actual representation
 * is abstracted out of this file, so that we can switch to a binary
 * representation if/when it becomes justified.
 *
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#include "pipe/p_config.h"

#include <stdio.h>
#include <stdlib.h>

#include "pipe/p_compiler.h"
#include "os/os_thread.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_string.h"
#include "util/u_math.h"
#include "util/u_format.h"

#include "tr_dump.h"
#include "tr_screen.h"
#include "tr_texture.h"


static FILE *stream = NULL;
static unsigned refcount = 0;
pipe_static_mutex(call_mutex);
static long unsigned call_no = 0;
static boolean dumping = FALSE;


static INLINE void
trace_dump_write(const char *buf, size_t size)
{
   if (stream) {
      fwrite(buf, size, 1, stream);
   }
}


static INLINE void
trace_dump_writes(const char *s)
{
   trace_dump_write(s, strlen(s));
}


static INLINE void
trace_dump_writef(const char *format, ...)
{
   static char buf[1024];
   unsigned len;
   va_list ap;
   va_start(ap, format);
   len = util_vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);
   trace_dump_write(buf, len);
}


static INLINE void
trace_dump_escape(const char *str)
{
   const unsigned char *p = (const unsigned char *)str;
   unsigned char c;
   while((c = *p++) != 0) {
      if(c == '<')
         trace_dump_writes("&lt;");
      else if(c == '>')
         trace_dump_writes("&gt;");
      else if(c == '&')
         trace_dump_writes("&amp;");
      else if(c == '\'')
         trace_dump_writes("&apos;");
      else if(c == '\"')
         trace_dump_writes("&quot;");
      else if(c >= 0x20 && c <= 0x7e)
         trace_dump_writef("%c", c);
      else
         trace_dump_writef("&#%u;", c);
   }
}


static INLINE void
trace_dump_indent(unsigned level)
{
   unsigned i;
   for(i = 0; i < level; ++i)
      trace_dump_writes("\t");
}


static INLINE void
trace_dump_newline(void)
{
   trace_dump_writes("\n");
}


static INLINE void
trace_dump_tag(const char *name)
{
   trace_dump_writes("<");
   trace_dump_writes(name);
   trace_dump_writes("/>");
}


static INLINE void
trace_dump_tag_begin(const char *name)
{
   trace_dump_writes("<");
   trace_dump_writes(name);
   trace_dump_writes(">");
}

static INLINE void
trace_dump_tag_begin1(const char *name,
                      const char *attr1, const char *value1)
{
   trace_dump_writes("<");
   trace_dump_writes(name);
   trace_dump_writes(" ");
   trace_dump_writes(attr1);
   trace_dump_writes("='");
   trace_dump_escape(value1);
   trace_dump_writes("'>");
}


static INLINE void
trace_dump_tag_begin2(const char *name,
                      const char *attr1, const char *value1,
                      const char *attr2, const char *value2)
{
   trace_dump_writes("<");
   trace_dump_writes(name);
   trace_dump_writes(" ");
   trace_dump_writes(attr1);
   trace_dump_writes("=\'");
   trace_dump_escape(value1);
   trace_dump_writes("\' ");
   trace_dump_writes(attr2);
   trace_dump_writes("=\'");
   trace_dump_escape(value2);
   trace_dump_writes("\'>");
}


static INLINE void
trace_dump_tag_begin3(const char *name,
                      const char *attr1, const char *value1,
                      const char *attr2, const char *value2,
                      const char *attr3, const char *value3)
{
   trace_dump_writes("<");
   trace_dump_writes(name);
   trace_dump_writes(" ");
   trace_dump_writes(attr1);
   trace_dump_writes("=\'");
   trace_dump_escape(value1);
   trace_dump_writes("\' ");
   trace_dump_writes(attr2);
   trace_dump_writes("=\'");
   trace_dump_escape(value2);
   trace_dump_writes("\' ");
   trace_dump_writes(attr3);
   trace_dump_writes("=\'");
   trace_dump_escape(value3);
   trace_dump_writes("\'>");
}


static INLINE void
trace_dump_tag_end(const char *name)
{
   trace_dump_writes("</");
   trace_dump_writes(name);
   trace_dump_writes(">");
}

static void
trace_dump_trace_close(void)
{
   if(stream) {
      trace_dump_writes("</trace>\n");
      fclose(stream);
      stream = NULL;
      refcount = 0;
      call_no = 0;
   }
}

boolean trace_dump_trace_begin()
{
   const char *filename;

   filename = debug_get_option("GALLIUM_TRACE", NULL);
   if(!filename)
      return FALSE;

   if(!stream) {

      stream = fopen(filename, "wt");
      if(!stream)
         return FALSE;

      trace_dump_writes("<?xml version='1.0' encoding='UTF-8'?>\n");
      trace_dump_writes("<?xml-stylesheet type='text/xsl' href='trace.xsl'?>\n");
      trace_dump_writes("<trace version='0.1'>\n");

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE)
      /* Linux applications rarely cleanup GL / Gallium resources so catch
       * application exit here */
      atexit(trace_dump_trace_close);
#endif
   }

   ++refcount;

   return TRUE;
}

boolean trace_dump_trace_enabled(void)
{
   return stream ? TRUE : FALSE;
}

void trace_dump_trace_end(void)
{
   if(stream)
      if(!--refcount)
         trace_dump_trace_close();
}

/*
 * Call lock
 */

void trace_dump_call_lock(void)
{
   pipe_mutex_lock(call_mutex);
}

void trace_dump_call_unlock(void)
{
   pipe_mutex_unlock(call_mutex);
}

/*
 * Dumping control
 */

void trace_dumping_start_locked(void)
{
   dumping = TRUE;
}

void trace_dumping_stop_locked(void)
{
   dumping = FALSE;
}

boolean trace_dumping_enabled_locked(void)
{
   return dumping;
}

void trace_dumping_start(void)
{
   pipe_mutex_lock(call_mutex);
   trace_dumping_start_locked();
   pipe_mutex_unlock(call_mutex);
}

void trace_dumping_stop(void)
{
   pipe_mutex_lock(call_mutex);
   trace_dumping_stop_locked();
   pipe_mutex_unlock(call_mutex);
}

boolean trace_dumping_enabled(void)
{
   boolean ret;
   pipe_mutex_lock(call_mutex);
   ret = trace_dumping_enabled_locked();
   pipe_mutex_unlock(call_mutex);
   return ret;
}

/*
 * Dump functions
 */

void trace_dump_call_begin_locked(const char *klass, const char *method)
{
   if (!dumping)
      return;

   ++call_no;
   trace_dump_indent(1);
   trace_dump_writes("<call no=\'");
   trace_dump_writef("%lu", call_no);
   trace_dump_writes("\' class=\'");
   trace_dump_escape(klass);
   trace_dump_writes("\' method=\'");
   trace_dump_escape(method);
   trace_dump_writes("\'>");
   trace_dump_newline();
}

void trace_dump_call_end_locked(void)
{
   if (!dumping)
      return;

   trace_dump_indent(1);
   trace_dump_tag_end("call");
   trace_dump_newline();
   fflush(stream);
}

void trace_dump_call_begin(const char *klass, const char *method)
{
   pipe_mutex_lock(call_mutex);
   trace_dump_call_begin_locked(klass, method);
}

void trace_dump_call_end(void)
{
   trace_dump_call_end_locked();
   pipe_mutex_unlock(call_mutex);
}

void trace_dump_arg_begin(const char *name)
{
   if (!dumping)
      return;

   trace_dump_indent(2);
   trace_dump_tag_begin1("arg", "name", name);
}

void trace_dump_arg_end(void)
{
   if (!dumping)
      return;

   trace_dump_tag_end("arg");
   trace_dump_newline();
}

void trace_dump_ret_begin(void)
{
   if (!dumping)
      return;

   trace_dump_indent(2);
   trace_dump_tag_begin("ret");
}

void trace_dump_ret_end(void)
{
   if (!dumping)
      return;

   trace_dump_tag_end("ret");
   trace_dump_newline();
}

void trace_dump_bool(int value)
{
   if (!dumping)
      return;

   trace_dump_writef("<bool>%c</bool>", value ? '1' : '0');
}

void trace_dump_int(long long int value)
{
   if (!dumping)
      return;

   trace_dump_writef("<int>%lli</int>", value);
}

void trace_dump_uint(long long unsigned value)
{
   if (!dumping)
      return;

   trace_dump_writef("<uint>%llu</uint>", value);
}

void trace_dump_float(double value)
{
   if (!dumping)
      return;

   trace_dump_writef("<float>%g</float>", value);
}

void trace_dump_bytes(const void *data,
                      size_t size)
{
   static const char hex_table[16] = "0123456789ABCDEF";
   const uint8_t *p = data;
   size_t i;

   if (!dumping)
      return;

   trace_dump_writes("<bytes>");
   for(i = 0; i < size; ++i) {
      uint8_t byte = *p++;
      char hex[2];
      hex[0] = hex_table[byte >> 4];
      hex[1] = hex_table[byte & 0xf];
      trace_dump_write(hex, 2);
   }
   trace_dump_writes("</bytes>");
}

void trace_dump_box_bytes(const void *data,
			  enum pipe_format format,
			  const struct pipe_box *box,
			  unsigned stride,
			  unsigned slice_stride)
{
   size_t size;

   if (slice_stride)
      size = box->depth * slice_stride;
   else if (stride)
      size = util_format_get_nblocksy(format, box->height) * stride;
   else {
      size = util_format_get_nblocksx(format, box->width) * util_format_get_blocksize(format);
   }

   trace_dump_bytes(data, size);
}

void trace_dump_string(const char *str)
{
   if (!dumping)
      return;

   trace_dump_writes("<string>");
   trace_dump_escape(str);
   trace_dump_writes("</string>");
}

void trace_dump_enum(const char *value)
{
   if (!dumping)
      return;

   trace_dump_writes("<enum>");
   trace_dump_escape(value);
   trace_dump_writes("</enum>");
}

void trace_dump_array_begin(void)
{
   if (!dumping)
      return;

   trace_dump_writes("<array>");
}

void trace_dump_array_end(void)
{
   if (!dumping)
      return;

   trace_dump_writes("</array>");
}

void trace_dump_elem_begin(void)
{
   if (!dumping)
      return;

   trace_dump_writes("<elem>");
}

void trace_dump_elem_end(void)
{
   if (!dumping)
      return;

   trace_dump_writes("</elem>");
}

void trace_dump_struct_begin(const char *name)
{
   if (!dumping)
      return;

   trace_dump_writef("<struct name='%s'>", name);
}

void trace_dump_struct_end(void)
{
   if (!dumping)
      return;

   trace_dump_writes("</struct>");
}

void trace_dump_member_begin(const char *name)
{
   if (!dumping)
      return;

   trace_dump_writef("<member name='%s'>", name);
}

void trace_dump_member_end(void)
{
   if (!dumping)
      return;

   trace_dump_writes("</member>");
}

void trace_dump_null(void)
{
   if (!dumping)
      return;

   trace_dump_writes("<null/>");
}

void trace_dump_ptr(const void *value)
{
   if (!dumping)
      return;

   if(value)
      trace_dump_writef("<ptr>0x%08lx</ptr>", (unsigned long)(uintptr_t)value);
   else
      trace_dump_null();
}


void trace_dump_resource_ptr(struct pipe_resource *_resource)
{
   if (!dumping)
      return;

   if (_resource) {
      struct trace_resource *tr_resource = trace_resource(_resource);
      trace_dump_ptr(tr_resource->resource);
   } else {
      trace_dump_null();
   }
}

void trace_dump_surface_ptr(struct pipe_surface *_surface)
{
   if (!dumping)
      return;

   if (_surface) {
      struct trace_surface *tr_surf = trace_surface(_surface);
      trace_dump_ptr(tr_surf->surface);
   } else {
      trace_dump_null();
   }
}

void trace_dump_transfer_ptr(struct pipe_transfer *_transfer)
{
   if (!dumping)
      return;

   if (_transfer) {
      struct trace_transfer *tr_tran = trace_transfer(_transfer);
      trace_dump_ptr(tr_tran->transfer);
   } else {
      trace_dump_null();
   }
}
