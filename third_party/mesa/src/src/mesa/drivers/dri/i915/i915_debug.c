/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "main/imports.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_debug.h"

static bool
debug(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint i;
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   
   if (len == 0) {
      printf("Error - zero length packet (0x%08x)\n", stream->ptr[0]);
      assert(0);
      return false;
   }

   if (stream->print_addresses)
      printf("%08x:  ", stream->offset);


   printf("%s (%d dwords):\n", name, len);
   for (i = 0; i < len; i++)
      printf("\t0x%08x\n",  ptr[i]);   
   printf("\n");

   stream->offset += len * sizeof(GLuint);
   
   return true;
}


static const char *get_prim_name( GLuint val )
{
   switch (val & PRIM3D_MASK) {
   case PRIM3D_TRILIST: return "TRILIST"; break;
   case PRIM3D_TRISTRIP: return "TRISTRIP"; break;
   case PRIM3D_TRISTRIP_RVRSE: return "TRISTRIP_RVRSE"; break;
   case PRIM3D_TRIFAN: return "TRIFAN"; break;
   case PRIM3D_POLY: return "POLY"; break;
   case PRIM3D_LINELIST: return "LINELIST"; break;
   case PRIM3D_LINESTRIP: return "LINESTRIP"; break;
   case PRIM3D_RECTLIST: return "RECTLIST"; break;
   case PRIM3D_POINTLIST: return "POINTLIST"; break;
   case PRIM3D_DIB: return "DIB"; break;
   case PRIM3D_CLEAR_RECT: return "CLEAR_RECT"; break;
   case PRIM3D_ZONE_INIT: return "ZONE_INIT"; break;
   default: return "????"; break;
   }
}

static bool
debug_prim(struct debug_stream *stream, const char *name,
	   bool dump_floats, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   const char *prim = get_prim_name( ptr[0] );
   GLuint i;
   


   printf("%s %s (%d dwords):\n", name, prim, len);
   printf("\t0x%08x\n",  ptr[0]);   
   for (i = 1; i < len; i++) {
      if (dump_floats)
	 printf("\t0x%08x // %f\n",  ptr[i], *(GLfloat *)&ptr[i]);   
      else
	 printf("\t0x%08x\n",  ptr[i]);   
   }

      
   printf("\n");

   stream->offset += len * sizeof(GLuint);
   
   return true;
}
   



static bool
debug_program(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);

   if (len == 0) {
      printf("Error - zero length packet (0x%08x)\n", stream->ptr[0]);
      assert(0);
      return false;
   }

   if (stream->print_addresses)
      printf("%08x:  ", stream->offset);

   printf("%s (%d dwords):\n", name, len);
   i915_disassemble_program( ptr, len );

   stream->offset += len * sizeof(GLuint);
   return true;
}


static bool
debug_chain(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint old_offset = stream->offset + len * sizeof(GLuint);
   GLuint i;

   printf("%s (%d dwords):\n", name, len);
   for (i = 0; i < len; i++)
      printf("\t0x%08x\n",  ptr[i]);

   stream->offset = ptr[1] & ~0x3;
   
   if (stream->offset < old_offset)
      printf("\n... skipping backwards from 0x%x --> 0x%x ...\n\n", 
		   old_offset, stream->offset );
   else
      printf("\n... skipping from 0x%x --> 0x%x ...\n\n", 
		   old_offset, stream->offset );


   return true;
}


static bool
debug_variable_length_prim(struct debug_stream *stream)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   const char *prim = get_prim_name( ptr[0] );
   GLuint i, len;

   GLushort *idx = (GLushort *)(ptr+1);
   for (i = 0; idx[i] != 0xffff; i++)
      ;

   len = 1+(i+2)/2;

   printf("3DPRIM, %s variable length %d indicies (%d dwords):\n", prim, i, len);
   for (i = 0; i < len; i++)
      printf("\t0x%08x\n",  ptr[i]);
   printf("\n");

   stream->offset += len * sizeof(GLuint);
   return true;
}


#define BITS( dw, hi, lo, ... )				\
do {							\
   unsigned himask = 0xffffffffU >> (31 - (hi));		\
   printf("\t\t ");				\
   printf(__VA_ARGS__);			\
   printf(": 0x%x\n", ((dw) & himask) >> (lo));	\
} while (0)

#define MBZ( dw, hi, lo) do {							\
   unsigned x = (dw) >> (lo);				\
   unsigned lomask = (1 << (lo)) - 1;			\
   unsigned himask;					\
   himask = (1UL << (hi)) - 1;				\
   assert ((x & himask & ~lomask) == 0);	\
} while (0)

#define FLAG( dw, bit, ... )			\
do {							\
   if (((dw) >> (bit)) & 1) {				\
      printf("\t\t ");				\
      printf(__VA_ARGS__);			\
      printf("\n");				\
   }							\
} while (0)

static bool
debug_load_immediate(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint bits = (ptr[0] >> 4) & 0xff;
   GLuint j = 0;
   
   printf("%s (%d dwords, flags: %x):\n", name, len, bits);
   printf("\t0x%08x\n",  ptr[j++]);

   if (bits & (1<<0)) {
      printf("\t  LIS0: 0x%08x\n", ptr[j]);
      printf("\t vb address: 0x%08x\n", (ptr[j] & ~0x3));
      BITS(ptr[j], 0, 0, "vb invalidate disable");
      j++;
   }
   if (bits & (1<<1)) {
      printf("\t  LIS1: 0x%08x\n", ptr[j]);
      BITS(ptr[j], 29, 24, "vb dword width");
      BITS(ptr[j], 21, 16, "vb dword pitch");
      BITS(ptr[j], 15, 0, "vb max index");
      j++;
   }
   if (bits & (1<<2)) {
      int i;
      printf("\t  LIS2: 0x%08x\n", ptr[j]);
      for (i = 0; i < 8; i++) {
	 unsigned tc = (ptr[j] >> (i * 4)) & 0xf;
	 if (tc != 0xf)
	    BITS(tc, 3, 0, "tex coord %d", i);
      }
      j++;
   }
   if (bits & (1<<3)) {
      printf("\t  LIS3: 0x%08x\n", ptr[j]);
      j++;
   }
   if (bits & (1<<4)) {
      printf("\t  LIS4: 0x%08x\n", ptr[j]);
      BITS(ptr[j], 31, 23, "point width");
      BITS(ptr[j], 22, 19, "line width");
      FLAG(ptr[j], 18, "alpha flatshade");
      FLAG(ptr[j], 17, "fog flatshade");
      FLAG(ptr[j], 16, "spec flatshade");
      FLAG(ptr[j], 15, "rgb flatshade");
      BITS(ptr[j], 14, 13, "cull mode");
      FLAG(ptr[j], 12, "vfmt: point width");
      FLAG(ptr[j], 11, "vfmt: specular/fog");
      FLAG(ptr[j], 10, "vfmt: rgba");
      FLAG(ptr[j], 9, "vfmt: depth offset");
      BITS(ptr[j], 8, 6, "vfmt: position (2==xyzw)");
      FLAG(ptr[j], 5, "force dflt diffuse");
      FLAG(ptr[j], 4, "force dflt specular");
      FLAG(ptr[j], 3, "local depth offset enable");
      FLAG(ptr[j], 2, "vfmt: fp32 fog coord");
      FLAG(ptr[j], 1, "sprite point");
      FLAG(ptr[j], 0, "antialiasing");
      j++;
   }
   if (bits & (1<<5)) {
      printf("\t  LIS5: 0x%08x\n", ptr[j]);
      BITS(ptr[j], 31, 28, "rgba write disables");
      FLAG(ptr[j], 27,     "force dflt point width");
      FLAG(ptr[j], 26,     "last pixel enable");
      FLAG(ptr[j], 25,     "global z offset enable");
      FLAG(ptr[j], 24,     "fog enable");
      BITS(ptr[j], 23, 16, "stencil ref");
      BITS(ptr[j], 15, 13, "stencil test");
      BITS(ptr[j], 12, 10, "stencil fail op");
      BITS(ptr[j], 9, 7,   "stencil pass z fail op");
      BITS(ptr[j], 6, 4,   "stencil pass z pass op");
      FLAG(ptr[j], 3,      "stencil write enable");
      FLAG(ptr[j], 2,      "stencil test enable");
      FLAG(ptr[j], 1,      "color dither enable");
      FLAG(ptr[j], 0,      "logiop enable");
      j++;
   }
   if (bits & (1<<6)) {
      printf("\t  LIS6: 0x%08x\n", ptr[j]);
      FLAG(ptr[j], 31,      "alpha test enable");
      BITS(ptr[j], 30, 28,  "alpha func");
      BITS(ptr[j], 27, 20,  "alpha ref");
      FLAG(ptr[j], 19,      "depth test enable");
      BITS(ptr[j], 18, 16,  "depth func");
      FLAG(ptr[j], 15,      "blend enable");
      BITS(ptr[j], 14, 12,  "blend func");
      BITS(ptr[j], 11, 8,   "blend src factor");
      BITS(ptr[j], 7,  4,   "blend dst factor");
      FLAG(ptr[j], 3,       "depth write enable");
      FLAG(ptr[j], 2,       "color write enable");
      BITS(ptr[j], 1,  0,   "provoking vertex"); 
      j++;
   }


   printf("\n");

   assert(j == len);

   stream->offset += len * sizeof(GLuint);
   
   return true;
}
 


static bool
debug_load_indirect(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint bits = (ptr[0] >> 8) & 0x3f;
   GLuint i, j = 0;
   
   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j++]);

   for (i = 0; i < 6; i++) {
      if (bits & (1<<i)) {
	 switch (1<<(8+i)) {
	 case LI0_STATE_STATIC_INDIRECT:
	    printf("        STATIC: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_DYNAMIC_INDIRECT:
	    printf("       DYNAMIC: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    break;
	 case LI0_STATE_SAMPLER:
	    printf("       SAMPLER: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_MAP:
	    printf("           MAP: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_PROGRAM:
	    printf("       PROGRAM: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_CONSTANTS:
	    printf("     CONSTANTS: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    printf("                0x%08x\n", ptr[j++]);
	    break;
	 default:
	    assert(0);
	    break;
	 }
      }
   }

   if (bits == 0) {
      printf("\t  DUMMY: 0x%08x\n", ptr[j++]);
   }

   printf("\n");


   assert(j == len);

   stream->offset += len * sizeof(GLuint);
   
   return true;
}
 	
static void BR13( struct debug_stream *stream,
		  GLuint val )
{
   printf("\t0x%08x\n",  val);
   FLAG(val, 30, "clipping enable");
   BITS(val, 25, 24, "color depth (3==32bpp)");
   BITS(val, 23, 16, "raster op");
   BITS(val, 15, 0,  "dest pitch");
}


static void BR2223( struct debug_stream *stream,
		    GLuint val22, GLuint val23 )
{
   union { GLuint val; short field[2]; } BR22, BR23;

   BR22.val = val22;
   BR23.val = val23;

   printf("\t0x%08x\n",  val22);
   BITS(val22, 31, 16, "dest y1");
   BITS(val22, 15, 0,  "dest x1");

   printf("\t0x%08x\n",  val23);
   BITS(val23, 31, 16, "dest y2");
   BITS(val23, 15, 0,  "dest x2");

   /* The blit engine may produce unexpected results when these aren't met */
   assert(BR22.field[0] < BR23.field[0]);
   assert(BR22.field[1] < BR23.field[1]);
}

static void BR09( struct debug_stream *stream,
		  GLuint val )
{
   printf("\t0x%08x -- dest address\n",  val);
}

static void BR26( struct debug_stream *stream,
		  GLuint val )
{
   printf("\t0x%08x\n",  val);
   BITS(val, 31, 16, "src y1");
   BITS(val, 15, 0,  "src x1");
}

static void BR11( struct debug_stream *stream,
		  GLuint val )
{
   printf("\t0x%08x\n",  val);
   BITS(val, 15, 0,  "src pitch");
}

static void BR12( struct debug_stream *stream,
		  GLuint val )
{
   printf("\t0x%08x -- src address\n",  val);
}

static void BR16( struct debug_stream *stream,
		  GLuint val )
{
   printf("\t0x%08x -- color\n",  val);
}
   
static bool
debug_copy_blit(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   int j = 0;

   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j++]);
   
   BR13(stream, ptr[j++]);
   BR2223(stream, ptr[j], ptr[j+1]);
   j += 2;
   BR09(stream, ptr[j++]);
   BR26(stream, ptr[j++]);
   BR11(stream, ptr[j++]);
   BR12(stream, ptr[j++]);

   stream->offset += len * sizeof(GLuint);
   assert(j == len);
   return true;
}

static bool
debug_color_blit(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   int j = 0;

   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j++]);

   BR13(stream, ptr[j++]);
   BR2223(stream, ptr[j], ptr[j+1]);
   j += 2;
   BR09(stream, ptr[j++]);
   BR16(stream, ptr[j++]);

   stream->offset += len * sizeof(GLuint);
   assert(j == len);
   return true;
}

static bool
debug_modes4(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   int j = 0;

   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j]);
   BITS(ptr[j], 21, 18, "logicop func");
   FLAG(ptr[j], 17, "stencil test mask modify-enable");
   FLAG(ptr[j], 16, "stencil write mask modify-enable");
   BITS(ptr[j], 15, 8, "stencil test mask");
   BITS(ptr[j], 7, 0,  "stencil write mask");
   j++;

   stream->offset += len * sizeof(GLuint);
   assert(j == len);
   return true;
}

static bool
debug_map_state(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   int j = 0;

   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j++]);
   
   {
      printf("\t0x%08x\n",  ptr[j]);
      BITS(ptr[j], 15, 0,   "map mask");
      j++;
   }

   while (j < len) {
      {
	 printf("\t  TMn.0: 0x%08x\n", ptr[j]);
	 printf("\t map address: 0x%08x\n", (ptr[j] & ~0x3));
	 FLAG(ptr[j], 1, "vertical line stride");
	 FLAG(ptr[j], 0, "vertical line stride offset");
	 j++;
      }

      {
	 printf("\t  TMn.1: 0x%08x\n", ptr[j]);
	 BITS(ptr[j], 31, 21, "height");
	 BITS(ptr[j], 20, 10, "width");
	 BITS(ptr[j], 9, 7, "surface format");
	 BITS(ptr[j], 6, 3, "texel format");
	 FLAG(ptr[j], 2, "use fence regs");
	 FLAG(ptr[j], 1, "tiled surface");
	 FLAG(ptr[j], 0, "tile walk ymajor");
	 j++;
      }
      {
	 printf("\t  TMn.2: 0x%08x\n", ptr[j]);
	 BITS(ptr[j], 31, 21, "dword pitch");
	 BITS(ptr[j], 20, 15, "cube face enables");
	 BITS(ptr[j], 14, 9, "max lod");
	 FLAG(ptr[j], 8,     "mip layout right");
	 BITS(ptr[j], 7, 0, "depth");
	 j++;
      }
   }

   stream->offset += len * sizeof(GLuint);
   assert(j == len);
   return true;
}

static bool
debug_sampler_state(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   int j = 0;

   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j++]);
   
   {
      printf("\t0x%08x\n",  ptr[j]);
      BITS(ptr[j], 15, 0,   "sampler mask");
      j++;
   }

   while (j < len) {
      {
	 printf("\t  TSn.0: 0x%08x\n", ptr[j]);
	 FLAG(ptr[j], 31, "reverse gamma");
	 FLAG(ptr[j], 30, "planar to packed");
	 FLAG(ptr[j], 29, "yuv->rgb");
	 BITS(ptr[j], 28, 27, "chromakey index");
	 BITS(ptr[j], 26, 22, "base mip level");
	 BITS(ptr[j], 21, 20, "mip mode filter");
	 BITS(ptr[j], 19, 17, "mag mode filter");
	 BITS(ptr[j], 16, 14, "min mode filter");
	 BITS(ptr[j], 13, 5,  "lod bias (s4.4)");
	 FLAG(ptr[j], 4,      "shadow enable");
	 FLAG(ptr[j], 3,      "max-aniso-4");
	 BITS(ptr[j], 2, 0,   "shadow func");
	 j++;
      }

      {
	 printf("\t  TSn.1: 0x%08x\n", ptr[j]);
	 BITS(ptr[j], 31, 24, "min lod");
	 MBZ( ptr[j], 23, 18 );
	 FLAG(ptr[j], 17,     "kill pixel enable");
	 FLAG(ptr[j], 16,     "keyed tex filter mode");
	 FLAG(ptr[j], 15,     "chromakey enable");
	 BITS(ptr[j], 14, 12, "tcx wrap mode");
	 BITS(ptr[j], 11, 9,  "tcy wrap mode");
	 BITS(ptr[j], 8,  6,  "tcz wrap mode");
	 FLAG(ptr[j], 5,      "normalized coords");
	 BITS(ptr[j], 4,  1,  "map (surface) index");
	 FLAG(ptr[j], 0,      "EAST deinterlacer enable");
	 j++;
      }
      {
	 printf("\t  TSn.2: 0x%08x  (default color)\n", ptr[j]);
	 j++;
      }
   }

   stream->offset += len * sizeof(GLuint);
   assert(j == len);
   return true;
}

static bool
debug_dest_vars(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   int j = 0;

   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j++]);

   {
      printf("\t0x%08x\n",  ptr[j]);
      FLAG(ptr[j], 31,     "early classic ztest");
      FLAG(ptr[j], 30,     "opengl tex default color");
      FLAG(ptr[j], 29,     "bypass iz");
      FLAG(ptr[j], 28,     "lod preclamp");
      BITS(ptr[j], 27, 26, "dither pattern");
      FLAG(ptr[j], 25,     "linear gamma blend");
      FLAG(ptr[j], 24,     "debug dither");
      BITS(ptr[j], 23, 20, "dstorg x");
      BITS(ptr[j], 19, 16, "dstorg y");
      MBZ (ptr[j], 15, 15 );
      BITS(ptr[j], 14, 12, "422 write select");
      BITS(ptr[j], 11, 8,  "cbuf format");
      BITS(ptr[j], 3, 2,   "zbuf format");
      FLAG(ptr[j], 1,      "vert line stride");
      FLAG(ptr[j], 1,      "vert line stride offset");
      j++;
   }
   
   stream->offset += len * sizeof(GLuint);
   assert(j == len);
   return true;
}

static bool
debug_buf_info(struct debug_stream *stream, const char *name, GLuint len)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   int j = 0;

   printf("%s (%d dwords):\n", name, len);
   printf("\t0x%08x\n",  ptr[j++]);

   {
      printf("\t0x%08x\n",  ptr[j]);
      BITS(ptr[j], 28, 28, "aux buffer id");
      BITS(ptr[j], 27, 24, "buffer id (7=depth, 3=back)");
      FLAG(ptr[j], 23,     "use fence regs");
      FLAG(ptr[j], 22,     "tiled surface");
      FLAG(ptr[j], 21,     "tile walk ymajor");
      MBZ (ptr[j], 20, 14);
      BITS(ptr[j], 13, 2,  "dword pitch");
      MBZ (ptr[j], 2,  0);
      j++;
   }
   
   printf("\t0x%08x -- buffer base address\n",  ptr[j++]);

   stream->offset += len * sizeof(GLuint);
   assert(j == len);
   return true;
}

static bool
i915_debug_packet(struct debug_stream *stream)
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint cmd = *ptr;
   
   switch (((cmd >> 29) & 0x7)) {
   case 0x0:
      switch ((cmd >> 23) & 0x3f) {
      case 0x0:
	 return debug(stream, "MI_NOOP", 1);
      case 0x3:
	 return debug(stream, "MI_WAIT_FOR_EVENT", 1);
      case 0x4:
	 return debug(stream, "MI_FLUSH", 1);
      case 0xA:
	 debug(stream, "MI_BATCH_BUFFER_END", 1);
	 return false;
      case 0x22:
	 return debug(stream, "MI_LOAD_REGISTER_IMM", 3);
      case 0x31:
	 return debug_chain(stream, "MI_BATCH_BUFFER_START", 2);
      default:
	 break;
      }
      break;
   case 0x1:
      break;
   case 0x2:
      switch ((cmd >> 22) & 0xff) {	 
      case 0x50:
	 return debug_color_blit(stream, "XY_COLOR_BLT", (cmd & 0xff) + 2);
      case 0x53:
	 return debug_copy_blit(stream, "XY_SRC_COPY_BLT", (cmd & 0xff) + 2);
      default:
	 return debug(stream, "blit command", (cmd & 0xff) + 2);
      }
      break;
   case 0x3:
      switch ((cmd >> 24) & 0x1f) {	 
      case 0x6:
	 return debug(stream, "3DSTATE_ANTI_ALIASING", 1);
      case 0x7:
	 return debug(stream, "3DSTATE_RASTERIZATION_RULES", 1);
      case 0x8:
	 return debug(stream, "3DSTATE_BACKFACE_STENCIL_OPS", 2);
      case 0x9:
	 return debug(stream, "3DSTATE_BACKFACE_STENCIL_MASKS", 1);
      case 0xb:
	 return debug(stream, "3DSTATE_INDEPENDENT_ALPHA_BLEND", 1);
      case 0xc:
	 return debug(stream, "3DSTATE_MODES5", 1);	 
      case 0xd:
	 return debug_modes4(stream, "3DSTATE_MODES4", 1);
      case 0x15:
	 return debug(stream, "3DSTATE_FOG_COLOR", 1);
      case 0x16:
	 return debug(stream, "3DSTATE_COORD_SET_BINDINGS", 1);
      case 0x1c:
	 /* 3DState16NP */
	 switch((cmd >> 19) & 0x1f) {
	 case 0x10:
	    return debug(stream, "3DSTATE_SCISSOR_ENABLE", 1);
	 case 0x11:
	    return debug(stream, "3DSTATE_DEPTH_SUBRECTANGLE_DISABLE", 1);
	 default:
	    break;
	 }
	 break;
      case 0x1d:
	 /* 3DStateMW */
	 switch ((cmd >> 16) & 0xff) {
	 case 0x0:
	    return debug_map_state(stream, "3DSTATE_MAP_STATE", (cmd & 0x1f) + 2);
	 case 0x1:
	    return debug_sampler_state(stream, "3DSTATE_SAMPLER_STATE", (cmd & 0x1f) + 2);
	 case 0x4:
	    return debug_load_immediate(stream, "3DSTATE_LOAD_STATE_IMMEDIATE", (cmd & 0xf) + 2);
	 case 0x5:
	    return debug_program(stream, "3DSTATE_PIXEL_SHADER_PROGRAM", (cmd & 0x1ff) + 2);
	 case 0x6:
	    return debug(stream, "3DSTATE_PIXEL_SHADER_CONSTANTS", (cmd & 0xff) + 2);
	 case 0x7:
	    return debug_load_indirect(stream, "3DSTATE_LOAD_INDIRECT", (cmd & 0xff) + 2);
	 case 0x80:
	    return debug(stream, "3DSTATE_DRAWING_RECTANGLE", (cmd & 0xffff) + 2);
	 case 0x81:
	    return debug(stream, "3DSTATE_SCISSOR_RECTANGLE", (cmd & 0xffff) + 2);
	 case 0x83:
	    return debug(stream, "3DSTATE_SPAN_STIPPLE", (cmd & 0xffff) + 2);
	 case 0x85:
	    return debug_dest_vars(stream, "3DSTATE_DEST_BUFFER_VARS", (cmd & 0xffff) + 2);
	 case 0x88:
	    return debug(stream, "3DSTATE_CONSTANT_BLEND_COLOR", (cmd & 0xffff) + 2);
	 case 0x89:
	    return debug(stream, "3DSTATE_FOG_MODE", (cmd & 0xffff) + 2);
	 case 0x8e:
	    return debug_buf_info(stream, "3DSTATE_BUFFER_INFO", (cmd & 0xffff) + 2);
	 case 0x97:
	    return debug(stream, "3DSTATE_DEPTH_OFFSET_SCALE", (cmd & 0xffff) + 2);
	 case 0x98:
	    return debug(stream, "3DSTATE_DEFAULT_Z", (cmd & 0xffff) + 2);
	 case 0x99:
	    return debug(stream, "3DSTATE_DEFAULT_DIFFUSE", (cmd & 0xffff) + 2);
	 case 0x9a:
	    return debug(stream, "3DSTATE_DEFAULT_SPECULAR", (cmd & 0xffff) + 2);
	 case 0x9c:
	    return debug(stream, "3DSTATE_CLEAR_PARAMETERS", (cmd & 0xffff) + 2);
	 default:
	    assert(0);
	    return 0;
	 }
	 break;
      case 0x1e:
	 if (cmd & (1 << 23))
	    return debug(stream, "???", (cmd & 0xffff) + 1);
	 else
	    return debug(stream, "", 1);
	 break;
      case 0x1f:
	 if ((cmd & (1 << 23)) == 0)	
	    return debug_prim(stream, "3DPRIM (inline)", 1, (cmd & 0x1ffff) + 2);
	 else if (cmd & (1 << 17)) 
	 {
	    if ((cmd & 0xffff) == 0)
	       return debug_variable_length_prim(stream);
	    else
	       return debug_prim(stream, "3DPRIM (indexed)", 0, (((cmd & 0xffff) + 1) / 2) + 1);
	 }
	 else
	    return debug_prim(stream, "3DPRIM  (indirect sequential)", 0, 2); 
	 break;
      default:
	 return debug(stream, "", 0);
      }
      break;
   default:
      assert(0);
      return 0;
   }

   assert(0);
   return 0;
}



void
i915_dump_batchbuffer( GLuint *start,
		       GLuint *end )
{
   struct debug_stream stream;
   GLuint bytes = (end - start) * 4;
   bool done = false;

   printf("\n\nBATCH: (%d)\n", bytes / 4);

   stream.offset = 0;
   stream.ptr = (char *)start;
   stream.print_addresses = 0;

   while (!done &&
	  stream.offset < bytes &&
	  stream.offset >= 0)
   {
      if (!i915_debug_packet( &stream ))
	 break;

      assert(stream.offset <= bytes &&
	     stream.offset >= 0);
   }

   printf("END-BATCH\n\n\n");
}


