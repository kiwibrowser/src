/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
   

#ifndef BRW_EU_H
#define BRW_EU_H

#include <stdbool.h>
#include "brw_structs.h"
#include "brw_defines.h"
#include "program/prog_instruction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BRW_SWIZZLE4(a,b,c,d) (((a)<<0) | ((b)<<2) | ((c)<<4) | ((d)<<6))
#define BRW_GET_SWZ(swz, idx) (((swz) >> ((idx)*2)) & 0x3)

#define BRW_SWIZZLE_NOOP      BRW_SWIZZLE4(0,1,2,3)
#define BRW_SWIZZLE_XYZW      BRW_SWIZZLE4(0,1,2,3)
#define BRW_SWIZZLE_XXXX      BRW_SWIZZLE4(0,0,0,0)
#define BRW_SWIZZLE_YYYY      BRW_SWIZZLE4(1,1,1,1)
#define BRW_SWIZZLE_ZZZZ      BRW_SWIZZLE4(2,2,2,2)
#define BRW_SWIZZLE_WWWW      BRW_SWIZZLE4(3,3,3,3)
#define BRW_SWIZZLE_XYXY      BRW_SWIZZLE4(0,1,0,1)

static inline bool brw_is_single_value_swizzle(int swiz)
{
   return (swiz == BRW_SWIZZLE_XXXX ||
	   swiz == BRW_SWIZZLE_YYYY ||
	   swiz == BRW_SWIZZLE_ZZZZ ||
	   swiz == BRW_SWIZZLE_WWWW);
}

#define REG_SIZE (8*4)


/* These aren't hardware structs, just something useful for us to pass around:
 *
 * Align1 operation has a lot of control over input ranges.  Used in
 * WM programs to implement shaders decomposed into "channel serial"
 * or "structure of array" form:
 */
struct brw_reg
{
   GLuint type:4;
   GLuint file:2;
   GLuint nr:8;
   GLuint subnr:5;		/* :1 in align16 */
   GLuint negate:1;		/* source only */
   GLuint abs:1;		/* source only */
   GLuint vstride:4;		/* source only */
   GLuint width:3;		/* src only, align1 only */
   GLuint hstride:2;   		/* align1 only */
   GLuint address_mode:1;	/* relative addressing, hopefully! */
   GLuint pad0:1;

   union {      
      struct {
	 GLuint swizzle:8;		/* src only, align16 only */
	 GLuint writemask:4;		/* dest only, align16 only */
	 GLint  indirect_offset:10;	/* relative addressing offset */
	 GLuint pad1:10;		/* two dwords total */
      } bits;

      GLfloat f;
      GLint   d;
      GLuint ud;
   } dw1;      
};


struct brw_indirect {
   GLuint addr_subnr:4;
   GLint addr_offset:10;
   GLuint pad:18;
};


#define BRW_EU_MAX_INSN_STACK 5

struct brw_compile {
   struct brw_instruction *store;
   int store_size;
   GLuint nr_insn;

   void *mem_ctx;

   /* Allow clients to push/pop instruction state:
    */
   struct brw_instruction stack[BRW_EU_MAX_INSN_STACK];
   bool compressed_stack[BRW_EU_MAX_INSN_STACK];
   struct brw_instruction *current;

   GLuint flag_value;
   bool single_program_flow;
   bool compressed;
   struct brw_context *brw;

   /* Control flow stacks:
    * - if_stack contains IF and ELSE instructions which must be patched
    *   (and popped) once the matching ENDIF instruction is encountered.
    *
    *   Just store the instruction pointer(an index).
    */
   int *if_stack;
   int if_stack_depth;
   int if_stack_array_size;

   /**
    * loop_stack contains the instruction pointers of the starts of loops which
    * must be patched (and popped) once the matching WHILE instruction is
    * encountered.
    */
   int *loop_stack;
   /**
    * pre-gen6, the BREAK and CONT instructions had to tell how many IF/ENDIF
    * blocks they were popping out of, to fix up the mask stack.  This tracks
    * the IF/ENDIF nesting in each current nested loop level.
    */
   int *if_depth_in_loop;
   int loop_stack_depth;
   int loop_stack_array_size;
};

static INLINE int type_sz( GLuint type )
{
   switch( type ) {
   case BRW_REGISTER_TYPE_UD:
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_F:
      return 4;
   case BRW_REGISTER_TYPE_HF:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_W:
      return 2;
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_B:
      return 1;
   default:
      return 0;
   }
}

/**
 * Construct a brw_reg.
 * \param file  one of the BRW_x_REGISTER_FILE values
 * \param nr  register number/index
 * \param subnr  register sub number
 * \param type  one of BRW_REGISTER_TYPE_x
 * \param vstride  one of BRW_VERTICAL_STRIDE_x
 * \param width  one of BRW_WIDTH_x
 * \param hstride  one of BRW_HORIZONTAL_STRIDE_x
 * \param swizzle  one of BRW_SWIZZLE_x
 * \param writemask  WRITEMASK_X/Y/Z/W bitfield
 */
static INLINE struct brw_reg brw_reg( GLuint file,
                                      GLuint nr,
                                      GLuint subnr,
                                      GLuint type,
                                      GLuint vstride,
                                      GLuint width,
                                      GLuint hstride,
                                      GLuint swizzle,
                                      GLuint writemask )
{
   struct brw_reg reg;
   if (file == BRW_GENERAL_REGISTER_FILE)
      assert(nr < BRW_MAX_GRF);
   else if (file == BRW_MESSAGE_REGISTER_FILE)
      assert((nr & ~(1 << 7)) < BRW_MAX_MRF);
   else if (file == BRW_ARCHITECTURE_REGISTER_FILE)
      assert(nr <= BRW_ARF_IP);

   reg.type = type;
   reg.file = file;
   reg.nr = nr;
   reg.subnr = subnr * type_sz(type);
   reg.negate = 0;
   reg.abs = 0;
   reg.vstride = vstride;
   reg.width = width;
   reg.hstride = hstride;
   reg.address_mode = BRW_ADDRESS_DIRECT;
   reg.pad0 = 0;

   /* Could do better: If the reg is r5.3<0;1,0>, we probably want to
    * set swizzle and writemask to W, as the lower bits of subnr will
    * be lost when converted to align16.  This is probably too much to
    * keep track of as you'd want it adjusted by suboffset(), etc.
    * Perhaps fix up when converting to align16?
    */
   reg.dw1.bits.swizzle = swizzle;
   reg.dw1.bits.writemask = writemask;
   reg.dw1.bits.indirect_offset = 0;
   reg.dw1.bits.pad1 = 0;
   return reg;
}

/** Construct float[16] register */
static INLINE struct brw_reg brw_vec16_reg( GLuint file,
					      GLuint nr,
					      GLuint subnr )
{
   return brw_reg(file,
		  nr,
		  subnr,
		  BRW_REGISTER_TYPE_F,
		  BRW_VERTICAL_STRIDE_16,
		  BRW_WIDTH_16,
		  BRW_HORIZONTAL_STRIDE_1,
		  BRW_SWIZZLE_XYZW,
		  WRITEMASK_XYZW);
}

/** Construct float[8] register */
static INLINE struct brw_reg brw_vec8_reg( GLuint file,
					     GLuint nr,
					     GLuint subnr )
{
   return brw_reg(file,
		  nr,
		  subnr,
		  BRW_REGISTER_TYPE_F,
		  BRW_VERTICAL_STRIDE_8,
		  BRW_WIDTH_8,
		  BRW_HORIZONTAL_STRIDE_1,
		  BRW_SWIZZLE_XYZW,
		  WRITEMASK_XYZW);
}

/** Construct float[4] register */
static INLINE struct brw_reg brw_vec4_reg( GLuint file,
					      GLuint nr,
					      GLuint subnr )
{
   return brw_reg(file,
		  nr,
		  subnr,
		  BRW_REGISTER_TYPE_F,
		  BRW_VERTICAL_STRIDE_4,
		  BRW_WIDTH_4,
		  BRW_HORIZONTAL_STRIDE_1,
		  BRW_SWIZZLE_XYZW,
		  WRITEMASK_XYZW);
}

/** Construct float[2] register */
static INLINE struct brw_reg brw_vec2_reg( GLuint file,
					      GLuint nr,
					      GLuint subnr )
{
   return brw_reg(file,
		  nr,
		  subnr,
		  BRW_REGISTER_TYPE_F,
		  BRW_VERTICAL_STRIDE_2,
		  BRW_WIDTH_2,
		  BRW_HORIZONTAL_STRIDE_1,
		  BRW_SWIZZLE_XYXY,
		  WRITEMASK_XY);
}

/** Construct float[1] register */
static INLINE struct brw_reg brw_vec1_reg( GLuint file,
					     GLuint nr,
					     GLuint subnr )
{
   return brw_reg(file,
		  nr,
		  subnr,
		  BRW_REGISTER_TYPE_F,
		  BRW_VERTICAL_STRIDE_0,
		  BRW_WIDTH_1,
		  BRW_HORIZONTAL_STRIDE_0,
		  BRW_SWIZZLE_XXXX,
		  WRITEMASK_X);
}


static INLINE struct brw_reg retype( struct brw_reg reg,
				       GLuint type )
{
   reg.type = type;
   return reg;
}

static inline struct brw_reg
sechalf(struct brw_reg reg)
{
   if (reg.vstride)
      reg.nr++;
   return reg;
}

static INLINE struct brw_reg suboffset( struct brw_reg reg,
					  GLuint delta )
{   
   reg.subnr += delta * type_sz(reg.type);
   return reg;
}


static INLINE struct brw_reg offset( struct brw_reg reg,
				       GLuint delta )
{
   reg.nr += delta;
   return reg;
}


static INLINE struct brw_reg byte_offset( struct brw_reg reg,
					    GLuint bytes )
{
   GLuint newoffset = reg.nr * REG_SIZE + reg.subnr + bytes;
   reg.nr = newoffset / REG_SIZE;
   reg.subnr = newoffset % REG_SIZE;
   return reg;
}
   

/** Construct unsigned word[16] register */
static INLINE struct brw_reg brw_uw16_reg( GLuint file,
					     GLuint nr,
					     GLuint subnr )
{
   return suboffset(retype(brw_vec16_reg(file, nr, 0), BRW_REGISTER_TYPE_UW), subnr);
}

/** Construct unsigned word[8] register */
static INLINE struct brw_reg brw_uw8_reg( GLuint file,
					    GLuint nr,
					    GLuint subnr )
{
   return suboffset(retype(brw_vec8_reg(file, nr, 0), BRW_REGISTER_TYPE_UW), subnr);
}

/** Construct unsigned word[1] register */
static INLINE struct brw_reg brw_uw1_reg( GLuint file,
					    GLuint nr,
					    GLuint subnr )
{
   return suboffset(retype(brw_vec1_reg(file, nr, 0), BRW_REGISTER_TYPE_UW), subnr);
}

static INLINE struct brw_reg brw_imm_reg( GLuint type )
{
   return brw_reg( BRW_IMMEDIATE_VALUE,
		   0,
		   0,
		   type,
		   BRW_VERTICAL_STRIDE_0,
		   BRW_WIDTH_1,
		   BRW_HORIZONTAL_STRIDE_0,
		   0,
		   0);      
}

/** Construct float immediate register */
static INLINE struct brw_reg brw_imm_f( GLfloat f )
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_F);
   imm.dw1.f = f;
   return imm;
}

/** Construct integer immediate register */
static INLINE struct brw_reg brw_imm_d( GLint d )
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_D);
   imm.dw1.d = d;
   return imm;
}

/** Construct uint immediate register */
static INLINE struct brw_reg brw_imm_ud( GLuint ud )
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_UD);
   imm.dw1.ud = ud;
   return imm;
}

/** Construct ushort immediate register */
static INLINE struct brw_reg brw_imm_uw( GLushort uw )
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_UW);
   imm.dw1.ud = uw | (uw << 16);
   return imm;
}

/** Construct short immediate register */
static INLINE struct brw_reg brw_imm_w( GLshort w )
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_W);
   imm.dw1.d = w | (w << 16);
   return imm;
}

/* brw_imm_b and brw_imm_ub aren't supported by hardware - the type
 * numbers alias with _V and _VF below:
 */

/** Construct vector of eight signed half-byte values */
static INLINE struct brw_reg brw_imm_v( GLuint v )
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_V);
   imm.vstride = BRW_VERTICAL_STRIDE_0;
   imm.width = BRW_WIDTH_8;
   imm.hstride = BRW_HORIZONTAL_STRIDE_1;
   imm.dw1.ud = v;
   return imm;
}

/** Construct vector of four 8-bit float values */
static INLINE struct brw_reg brw_imm_vf( GLuint v )
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_VF);
   imm.vstride = BRW_VERTICAL_STRIDE_0;
   imm.width = BRW_WIDTH_4;
   imm.hstride = BRW_HORIZONTAL_STRIDE_1;
   imm.dw1.ud = v;
   return imm;
}

#define VF_ZERO 0x0
#define VF_ONE  0x30
#define VF_NEG  (1<<7)

static INLINE struct brw_reg brw_imm_vf4( GLuint v0, 
					    GLuint v1, 
					    GLuint v2,
					    GLuint v3)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_VF);
   imm.vstride = BRW_VERTICAL_STRIDE_0;
   imm.width = BRW_WIDTH_4;
   imm.hstride = BRW_HORIZONTAL_STRIDE_1;
   imm.dw1.ud = ((v0 << 0) |
		 (v1 << 8) |
		 (v2 << 16) |
		 (v3 << 24));
   return imm;
}


static INLINE struct brw_reg brw_address( struct brw_reg reg )
{
   return brw_imm_uw(reg.nr * REG_SIZE + reg.subnr);
}

/** Construct float[1] general-purpose register */
static INLINE struct brw_reg brw_vec1_grf( GLuint nr, GLuint subnr )
{
   return brw_vec1_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

/** Construct float[2] general-purpose register */
static INLINE struct brw_reg brw_vec2_grf( GLuint nr, GLuint subnr )
{
   return brw_vec2_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

/** Construct float[4] general-purpose register */
static INLINE struct brw_reg brw_vec4_grf( GLuint nr, GLuint subnr )
{
   return brw_vec4_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

/** Construct float[8] general-purpose register */
static INLINE struct brw_reg brw_vec8_grf( GLuint nr, GLuint subnr )
{
   return brw_vec8_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}


static INLINE struct brw_reg brw_uw8_grf( GLuint nr, GLuint subnr )
{
   return brw_uw8_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

static INLINE struct brw_reg brw_uw16_grf( GLuint nr, GLuint subnr )
{
   return brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}


/** Construct null register (usually used for setting condition codes) */
static INLINE struct brw_reg brw_null_reg( void )
{
   return brw_vec8_reg(BRW_ARCHITECTURE_REGISTER_FILE, 
		       BRW_ARF_NULL, 
		       0);
}

static INLINE struct brw_reg brw_address_reg( GLuint subnr )
{
   return brw_uw1_reg(BRW_ARCHITECTURE_REGISTER_FILE, 
		      BRW_ARF_ADDRESS, 
		      subnr);
}

/* If/else instructions break in align16 mode if writemask & swizzle
 * aren't xyzw.  This goes against the convention for other scalar
 * regs:
 */
static INLINE struct brw_reg brw_ip_reg( void )
{
   return brw_reg(BRW_ARCHITECTURE_REGISTER_FILE, 
		  BRW_ARF_IP, 
		  0,
		  BRW_REGISTER_TYPE_UD,
		  BRW_VERTICAL_STRIDE_4, /* ? */
		  BRW_WIDTH_1,
		  BRW_HORIZONTAL_STRIDE_0,
		  BRW_SWIZZLE_XYZW, /* NOTE! */
		  WRITEMASK_XYZW); /* NOTE! */
}

static INLINE struct brw_reg brw_acc_reg( void )
{
   return brw_vec8_reg(BRW_ARCHITECTURE_REGISTER_FILE, 
		       BRW_ARF_ACCUMULATOR, 
		       0);
}

static INLINE struct brw_reg brw_notification_1_reg(void)
{

   return brw_reg(BRW_ARCHITECTURE_REGISTER_FILE,
		  BRW_ARF_NOTIFICATION_COUNT,
		  1,
		  BRW_REGISTER_TYPE_UD,
		  BRW_VERTICAL_STRIDE_0,
		  BRW_WIDTH_1,
		  BRW_HORIZONTAL_STRIDE_0,
		  BRW_SWIZZLE_XXXX,
		  WRITEMASK_X);
}


static INLINE struct brw_reg brw_flag_reg( void )
{
   return brw_uw1_reg(BRW_ARCHITECTURE_REGISTER_FILE,
		      BRW_ARF_FLAG,
		      0);
}


static INLINE struct brw_reg brw_mask_reg( GLuint subnr )
{
   return brw_uw1_reg(BRW_ARCHITECTURE_REGISTER_FILE,
		      BRW_ARF_MASK,
		      subnr);
}

static INLINE struct brw_reg brw_message_reg( GLuint nr )
{
   assert((nr & ~(1 << 7)) < BRW_MAX_MRF);
   return brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE,
		       nr,
		       0);
}




/* This is almost always called with a numeric constant argument, so
 * make things easy to evaluate at compile time:
 */
static INLINE GLuint cvt( GLuint val )
{
   switch (val) {
   case 0: return 0;
   case 1: return 1;
   case 2: return 2;
   case 4: return 3;
   case 8: return 4;
   case 16: return 5;
   case 32: return 6;
   }
   return 0;
}

static INLINE struct brw_reg stride( struct brw_reg reg,
				       GLuint vstride,
				       GLuint width,
				       GLuint hstride )
{
   reg.vstride = cvt(vstride);
   reg.width = cvt(width) - 1;
   reg.hstride = cvt(hstride);
   return reg;
}


static INLINE struct brw_reg vec16( struct brw_reg reg )
{
   return stride(reg, 16,16,1);
}

static INLINE struct brw_reg vec8( struct brw_reg reg )
{
   return stride(reg, 8,8,1);
}

static INLINE struct brw_reg vec4( struct brw_reg reg )
{
   return stride(reg, 4,4,1);
}

static INLINE struct brw_reg vec2( struct brw_reg reg )
{
   return stride(reg, 2,2,1);
}

static INLINE struct brw_reg vec1( struct brw_reg reg )
{
   return stride(reg, 0,1,0);
}


static INLINE struct brw_reg get_element( struct brw_reg reg, GLuint elt )
{
   return vec1(suboffset(reg, elt));
}

static INLINE struct brw_reg get_element_ud( struct brw_reg reg, GLuint elt )
{
   return vec1(suboffset(retype(reg, BRW_REGISTER_TYPE_UD), elt));
}

static INLINE struct brw_reg get_element_d( struct brw_reg reg, GLuint elt )
{
   return vec1(suboffset(retype(reg, BRW_REGISTER_TYPE_D), elt));
}


static INLINE struct brw_reg brw_swizzle( struct brw_reg reg,
					    GLuint x,
					    GLuint y, 
					    GLuint z,
					    GLuint w)
{
   assert(reg.file != BRW_IMMEDIATE_VALUE);

   reg.dw1.bits.swizzle = BRW_SWIZZLE4(BRW_GET_SWZ(reg.dw1.bits.swizzle, x),
				       BRW_GET_SWZ(reg.dw1.bits.swizzle, y),
				       BRW_GET_SWZ(reg.dw1.bits.swizzle, z),
				       BRW_GET_SWZ(reg.dw1.bits.swizzle, w));
   return reg;
}


static INLINE struct brw_reg brw_swizzle1( struct brw_reg reg,
					     GLuint x )
{
   return brw_swizzle(reg, x, x, x, x);
}

static INLINE struct brw_reg brw_writemask( struct brw_reg reg,
					      GLuint mask )
{
   assert(reg.file != BRW_IMMEDIATE_VALUE);
   reg.dw1.bits.writemask &= mask;
   return reg;
}

static INLINE struct brw_reg brw_set_writemask( struct brw_reg reg,
						  GLuint mask )
{
   assert(reg.file != BRW_IMMEDIATE_VALUE);
   reg.dw1.bits.writemask = mask;
   return reg;
}

static INLINE struct brw_reg negate( struct brw_reg reg )
{
   reg.negate ^= 1;
   return reg;
}

static INLINE struct brw_reg brw_abs( struct brw_reg reg )
{
   reg.abs = 1;
   reg.negate = 0;
   return reg;
}

/***********************************************************************
 */
static INLINE struct brw_reg brw_vec4_indirect( GLuint subnr,
						  GLint offset )
{
   struct brw_reg reg =  brw_vec4_grf(0, 0);
   reg.subnr = subnr;
   reg.address_mode = BRW_ADDRESS_REGISTER_INDIRECT_REGISTER;
   reg.dw1.bits.indirect_offset = offset;
   return reg;
}

static INLINE struct brw_reg brw_vec1_indirect( GLuint subnr,
						  GLint offset )
{
   struct brw_reg reg =  brw_vec1_grf(0, 0);
   reg.subnr = subnr;
   reg.address_mode = BRW_ADDRESS_REGISTER_INDIRECT_REGISTER;
   reg.dw1.bits.indirect_offset = offset;
   return reg;
}

static INLINE struct brw_reg deref_4f(struct brw_indirect ptr, GLint offset)
{
   return brw_vec4_indirect(ptr.addr_subnr, ptr.addr_offset + offset);
}

static INLINE struct brw_reg deref_1f(struct brw_indirect ptr, GLint offset)
{
   return brw_vec1_indirect(ptr.addr_subnr, ptr.addr_offset + offset);
}

static INLINE struct brw_reg deref_4b(struct brw_indirect ptr, GLint offset)
{
   return retype(deref_4f(ptr, offset), BRW_REGISTER_TYPE_B);
}

static INLINE struct brw_reg deref_1uw(struct brw_indirect ptr, GLint offset)
{
   return retype(deref_1f(ptr, offset), BRW_REGISTER_TYPE_UW);
}

static INLINE struct brw_reg deref_1d(struct brw_indirect ptr, GLint offset)
{
   return retype(deref_1f(ptr, offset), BRW_REGISTER_TYPE_D);
}

static INLINE struct brw_reg deref_1ud(struct brw_indirect ptr, GLint offset)
{
   return retype(deref_1f(ptr, offset), BRW_REGISTER_TYPE_UD);
}

static INLINE struct brw_reg get_addr_reg(struct brw_indirect ptr)
{
   return brw_address_reg(ptr.addr_subnr);
}

static INLINE struct brw_indirect brw_indirect_offset( struct brw_indirect ptr, GLint offset )
{
   ptr.addr_offset += offset;
   return ptr;
}

static INLINE struct brw_indirect brw_indirect( GLuint addr_subnr, GLint offset )
{
   struct brw_indirect ptr;
   ptr.addr_subnr = addr_subnr;
   ptr.addr_offset = offset;
   ptr.pad = 0;
   return ptr;
}

/** Do two brw_regs refer to the same register? */
static INLINE bool
brw_same_reg(struct brw_reg r1, struct brw_reg r2)
{
   return r1.file == r2.file && r1.nr == r2.nr;
}

static INLINE struct brw_instruction *current_insn( struct brw_compile *p)
{
   return &p->store[p->nr_insn];
}

void brw_pop_insn_state( struct brw_compile *p );
void brw_push_insn_state( struct brw_compile *p );
void brw_set_mask_control( struct brw_compile *p, GLuint value );
void brw_set_saturate( struct brw_compile *p, bool enable );
void brw_set_access_mode( struct brw_compile *p, GLuint access_mode );
void brw_set_compression_control(struct brw_compile *p, enum brw_compression c);
void brw_set_predicate_control_flag_value( struct brw_compile *p, GLuint value );
void brw_set_predicate_control( struct brw_compile *p, GLuint pc );
void brw_set_predicate_inverse(struct brw_compile *p, bool predicate_inverse);
void brw_set_conditionalmod( struct brw_compile *p, GLuint conditional );
void brw_set_acc_write_control(struct brw_compile *p, GLuint value);

void brw_init_compile(struct brw_context *, struct brw_compile *p,
		      void *mem_ctx);
const GLuint *brw_get_program( struct brw_compile *p, GLuint *sz );

struct brw_instruction *brw_next_insn(struct brw_compile *p, GLuint opcode);
void brw_set_dest(struct brw_compile *p, struct brw_instruction *insn,
		  struct brw_reg dest);
void brw_set_src0(struct brw_compile *p, struct brw_instruction *insn,
		  struct brw_reg reg);

void gen6_resolve_implied_move(struct brw_compile *p,
			       struct brw_reg *src,
			       GLuint msg_reg_nr);

/* Helpers for regular instructions:
 */
#define ALU1(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0);

#define ALU2(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1);

#define ALU3(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1,			\
	      struct brw_reg src2);

#define ROUND(OP) \
void brw_##OP(struct brw_compile *p, struct brw_reg dest, struct brw_reg src0);

ALU1(MOV)
ALU2(SEL)
ALU1(NOT)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(SHR)
ALU2(SHL)
ALU2(RSR)
ALU2(RSL)
ALU2(ASR)
ALU2(JMPI)
ALU2(ADD)
ALU2(AVG)
ALU2(MUL)
ALU1(FRC)
ALU1(RNDD)
ALU2(MAC)
ALU2(MACH)
ALU1(LZD)
ALU2(DP4)
ALU2(DPH)
ALU2(DP3)
ALU2(DP2)
ALU2(LINE)
ALU2(PLN)
ALU3(MAD)

ROUND(RNDZ)
ROUND(RNDE)

#undef ALU1
#undef ALU2
#undef ALU3
#undef ROUND


/* Helpers for SEND instruction:
 */
void brw_set_sampler_message(struct brw_compile *p,
                             struct brw_instruction *insn,
                             GLuint binding_table_index,
                             GLuint sampler,
                             GLuint msg_type,
                             GLuint response_length,
                             GLuint msg_length,
                             GLuint header_present,
                             GLuint simd_mode,
                             GLuint return_format);

void brw_set_dp_read_message(struct brw_compile *p,
			     struct brw_instruction *insn,
			     GLuint binding_table_index,
			     GLuint msg_control,
			     GLuint msg_type,
			     GLuint target_cache,
			     GLuint msg_length,
			     GLuint response_length);

void brw_set_dp_write_message(struct brw_compile *p,
			      struct brw_instruction *insn,
			      GLuint binding_table_index,
			      GLuint msg_control,
			      GLuint msg_type,
			      GLuint msg_length,
			      bool header_present,
			      GLuint last_render_target,
			      GLuint response_length,
			      GLuint end_of_thread,
			      GLuint send_commit_msg);

void brw_urb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   bool used,
		   GLuint msg_length,
		   GLuint response_length,
		   bool eot,
		   bool writes_complete,
		   GLuint offset,
		   GLuint swizzle);

void brw_ff_sync(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   GLuint response_length,
		   bool eot);

void brw_svb_write(struct brw_compile *p,
                   struct brw_reg dest,
                   GLuint msg_reg_nr,
                   struct brw_reg src0,
                   GLuint binding_table_index,
                   bool   send_commit_msg);

void brw_fb_WRITE(struct brw_compile *p,
		  int dispatch_width,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   GLuint msg_control,
		   GLuint binding_table_index,
		   GLuint msg_length,
		   GLuint response_length,
		   bool eot,
		   bool header_present);

void brw_SAMPLE(struct brw_compile *p,
		struct brw_reg dest,
		GLuint msg_reg_nr,
		struct brw_reg src0,
		GLuint binding_table_index,
		GLuint sampler,
		GLuint writemask,
		GLuint msg_type,
		GLuint response_length,
		GLuint msg_length,
		GLuint header_present,
		GLuint simd_mode,
		GLuint return_format);

void brw_math_16( struct brw_compile *p,
		  struct brw_reg dest,
		  GLuint function,
		  GLuint msg_reg_nr,
		  struct brw_reg src,
		  GLuint precision );

void brw_math( struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       GLuint msg_reg_nr,
	       struct brw_reg src,
	       GLuint data_type,
	       GLuint precision );

void brw_math2(struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       struct brw_reg src0,
	       struct brw_reg src1);

void brw_oword_block_read(struct brw_compile *p,
			  struct brw_reg dest,
			  struct brw_reg mrf,
			  uint32_t offset,
			  uint32_t bind_table_index);

void brw_oword_block_read_scratch(struct brw_compile *p,
				  struct brw_reg dest,
				  struct brw_reg mrf,
				  int num_regs,
				  GLuint offset);

void brw_oword_block_write_scratch(struct brw_compile *p,
				   struct brw_reg mrf,
				   int num_regs,
				   GLuint offset);

void brw_dword_scattered_read(struct brw_compile *p,
			      struct brw_reg dest,
			      struct brw_reg mrf,
			      uint32_t bind_table_index);

void brw_dp_READ_4_vs( struct brw_compile *p,
                       struct brw_reg dest,
                       GLuint location,
                       GLuint bind_table_index );

void brw_dp_READ_4_vs_relative(struct brw_compile *p,
			       struct brw_reg dest,
			       struct brw_reg addrReg,
			       GLuint offset,
			       GLuint bind_table_index);

/* If/else/endif.  Works by manipulating the execution flags on each
 * channel.
 */
struct brw_instruction *brw_IF(struct brw_compile *p, 
			       GLuint execute_size);
struct brw_instruction *gen6_IF(struct brw_compile *p, uint32_t conditional,
				struct brw_reg src0, struct brw_reg src1);

void brw_ELSE(struct brw_compile *p);
void brw_ENDIF(struct brw_compile *p);

/* DO/WHILE loops:
 */
struct brw_instruction *brw_DO(struct brw_compile *p,
			       GLuint execute_size);

struct brw_instruction *brw_WHILE(struct brw_compile *p);

struct brw_instruction *brw_BREAK(struct brw_compile *p);
struct brw_instruction *brw_CONT(struct brw_compile *p);
struct brw_instruction *gen6_CONT(struct brw_compile *p);
/* Forward jumps:
 */
void brw_land_fwd_jump(struct brw_compile *p, int jmp_insn_idx);



void brw_NOP(struct brw_compile *p);

void brw_WAIT(struct brw_compile *p);

/* Special case: there is never a destination, execution size will be
 * taken from src0:
 */
void brw_CMP(struct brw_compile *p,
	     struct brw_reg dest,
	     GLuint conditional,
	     struct brw_reg src0,
	     struct brw_reg src1);

void brw_print_reg( struct brw_reg reg );


/*********************************************************************** 
 * brw_eu_util.c:
 */

void brw_copy_indirect_to_indirect(struct brw_compile *p,
				   struct brw_indirect dst_ptr,
				   struct brw_indirect src_ptr,
				   GLuint count);

void brw_copy_from_indirect(struct brw_compile *p,
			    struct brw_reg dst,
			    struct brw_indirect ptr,
			    GLuint count);

void brw_copy4(struct brw_compile *p,
	       struct brw_reg dst,
	       struct brw_reg src,
	       GLuint count);

void brw_copy8(struct brw_compile *p,
	       struct brw_reg dst,
	       struct brw_reg src,
	       GLuint count);

void brw_math_invert( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg src);

void brw_set_src1(struct brw_compile *p,
		  struct brw_instruction *insn,
		  struct brw_reg reg);

void brw_set_uip_jip(struct brw_compile *p);

uint32_t brw_swap_cmod(uint32_t cmod);

/* brw_optimize.c */
void brw_optimize(struct brw_compile *p);
void brw_remove_duplicate_mrf_moves(struct brw_compile *p);
void brw_remove_grf_to_mrf_moves(struct brw_compile *p);

#ifdef __cplusplus
}
#endif

#endif
