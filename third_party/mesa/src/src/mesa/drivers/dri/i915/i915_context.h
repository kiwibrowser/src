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

#ifndef I915CONTEXT_INC
#define I915CONTEXT_INC

#include "intel_context.h"

#define I915_FALLBACK_TEXTURE		 0x1000
#define I915_FALLBACK_COLORMASK		 0x2000
#define I915_FALLBACK_STENCIL		 0x4000
#define I915_FALLBACK_STIPPLE		 0x8000
#define I915_FALLBACK_PROGRAM		 0x10000
#define I915_FALLBACK_LOGICOP		 0x20000
#define I915_FALLBACK_POLYGON_SMOOTH	 0x40000
#define I915_FALLBACK_POINT_SMOOTH	 0x80000
#define I915_FALLBACK_POINT_SPRITE_COORD_ORIGIN	 0x100000
#define I915_FALLBACK_DRAW_OFFSET	 0x200000
#define I915_FALLBACK_COORD_REPLACE	 0x400000

#define I915_UPLOAD_CTX              0x1
#define I915_UPLOAD_BUFFERS          0x2
#define I915_UPLOAD_STIPPLE          0x4
#define I915_UPLOAD_PROGRAM          0x8
#define I915_UPLOAD_CONSTANTS        0x10
#define I915_UPLOAD_INVARIENT        0x40
#define I915_UPLOAD_DEFAULTS         0x80
#define I915_UPLOAD_RASTER_RULES     0x100
#define I915_UPLOAD_BLEND            0x200
#define I915_UPLOAD_TEX(i)           (0x00010000<<(i))
#define I915_UPLOAD_TEX_ALL          (0x00ff0000)
#define I915_UPLOAD_TEX_0_SHIFT      16


/* State structure offsets - these will probably disappear.
 */
#define I915_DESTREG_CBUFADDR0 0
#define I915_DESTREG_CBUFADDR1 1
#define I915_DESTREG_DBUFADDR0 3
#define I915_DESTREG_DBUFADDR1 4
#define I915_DESTREG_DV0 6
#define I915_DESTREG_DV1 7
#define I915_DESTREG_SENABLE 8
#define I915_DESTREG_SR0 9
#define I915_DESTREG_SR1 10
#define I915_DESTREG_SR2 11
#define I915_DESTREG_DRAWRECT0 12
#define I915_DESTREG_DRAWRECT1 13
#define I915_DESTREG_DRAWRECT2 14
#define I915_DESTREG_DRAWRECT3 15
#define I915_DESTREG_DRAWRECT4 16
#define I915_DESTREG_DRAWRECT5 17
#define I915_DEST_SETUP_SIZE 18

#define I915_CTXREG_STATE4		0
#define I915_CTXREG_LI			1
#define I915_CTXREG_LIS2		2
#define I915_CTXREG_LIS4		3
#define I915_CTXREG_LIS5		4
#define I915_CTXREG_LIS6		5
#define I915_CTXREG_BF_STENCIL_OPS	6
#define I915_CTXREG_BF_STENCIL_MASKS	7
#define I915_CTX_SETUP_SIZE		8

#define I915_BLENDREG_IAB		0
#define I915_BLENDREG_BLENDCOLOR0	1
#define I915_BLENDREG_BLENDCOLOR1	2
#define I915_BLEND_SETUP_SIZE		3

#define I915_STPREG_ST0        0
#define I915_STPREG_ST1        1
#define I915_STP_SETUP_SIZE    2

#define I915_TEXREG_MS3        1
#define I915_TEXREG_MS4        2
#define I915_TEXREG_SS2        3
#define I915_TEXREG_SS3        4
#define I915_TEXREG_SS4        5
#define I915_TEX_SETUP_SIZE    6

#define I915_DEFREG_C0    0
#define I915_DEFREG_C1    1
#define I915_DEFREG_S0    2
#define I915_DEFREG_S1    3
#define I915_DEFREG_Z0    4
#define I915_DEFREG_Z1    5
#define I915_DEF_SETUP_SIZE    6

enum {
   I915_RASTER_RULES,
   I915_RASTER_RULES_SETUP_SIZE,
};

#define I915_MAX_CONSTANT      32
#define I915_CONSTANT_SIZE     (2+(4*I915_MAX_CONSTANT))

#define I915_MAX_TEX_INDIRECT 4
#define I915_MAX_TEX_INSN     32
#define I915_MAX_ALU_INSN     64
#define I915_MAX_DECL_INSN    27
#define I915_MAX_TEMPORARY    16

#define I915_MAX_INSN          (I915_MAX_DECL_INSN + \
				I915_MAX_TEX_INSN + \
				I915_MAX_ALU_INSN)

/* Maximum size of the program packet, which matches the limits on
 * decl, tex, and ALU instructions.
 */
#define I915_PROGRAM_SIZE      (I915_MAX_INSN * 3 + 1)

/* Hardware version of a parsed fragment program.  "Derived" from the
 * mesa fragment_program struct.
 */
struct i915_fragment_program
{
   struct gl_fragment_program FragProg;

   bool translated;
   bool params_uptodate;
   bool on_hardware;
   bool error;             /* If program is malformed for any reason. */

   /** Record of which phases R registers were last written in. */
   GLuint register_phases[16];
   GLuint indirections;
   GLuint nr_tex_indirect;
   GLuint nr_tex_insn;
   GLuint nr_alu_insn;
   GLuint nr_decl_insn;




   /* TODO: split between the stored representation of a program and
    * the state used to build that representation.
    */
   struct gl_context *ctx;

   /* declarations contains the packet header. */
   GLuint declarations[I915_MAX_DECL_INSN * 3 + 1];
   GLuint program[(I915_MAX_TEX_INSN + I915_MAX_ALU_INSN) * 3];

   GLfloat constant[I915_MAX_CONSTANT][4];
   GLuint constant_flags[I915_MAX_CONSTANT];
   GLuint nr_constants;

   GLuint *csr;                 /* Cursor, points into program.
                                 */

   GLuint *decl;                /* Cursor, points into declarations.
                                 */

   GLuint decl_s;               /* flags for which s regs need to be decl'd */
   GLuint decl_t;               /* flags for which t regs need to be decl'd */

   GLuint temp_flag;            /* Tracks temporary regs which are in
                                 * use.
                                 */

   GLuint utemp_flag;           /* Tracks TYPE_U temporary regs which are in
                                 * use.
                                 */


   /* Track which R registers are "live" for each instruction.
    * A register is live between the time it's written to and the last time
    * it's read. */
   GLuint usedRegs[I915_MAX_INSN];

   /* Helpers for i915_fragprog.c:
    */
   GLuint wpos_tex;
   bool depth_written;

   struct
   {
      GLuint reg;               /* Hardware constant idx */
      const GLfloat *values;    /* Pointer to tracked values */
   } param[I915_MAX_CONSTANT];
   GLuint nr_params;
};







#define I915_TEX_UNITS 8


struct i915_hw_state
{
   GLuint Ctx[I915_CTX_SETUP_SIZE];
   GLuint Blend[I915_BLEND_SETUP_SIZE];
   GLuint Buffer[I915_DEST_SETUP_SIZE];
   GLuint Stipple[I915_STP_SETUP_SIZE];
   GLuint Defaults[I915_DEF_SETUP_SIZE];
   GLuint RasterRules[I915_RASTER_RULES_SETUP_SIZE];
   GLuint Tex[I915_TEX_UNITS][I915_TEX_SETUP_SIZE];
   GLuint Constant[I915_CONSTANT_SIZE];
   GLuint ConstantSize;
   GLuint Program[I915_PROGRAM_SIZE];
   GLuint ProgramSize;

   /* Region pointers for relocation: 
    */
   struct intel_region *draw_region;
   struct intel_region *depth_region;
/*    struct intel_region *tex_region[I915_TEX_UNITS]; */

   /* Regions aren't actually that appropriate here as the memory may
    * be from a PBO or FBO.  Will have to do this for draw and depth for
    * FBO's...
    */
   drm_intel_bo *tex_buffer[I915_TEX_UNITS];
   GLuint tex_offset[I915_TEX_UNITS];


   GLuint active;               /* I915_UPLOAD_* */
   GLuint emitted;              /* I915_UPLOAD_* */
};

struct i915_context
{
   struct intel_context intel;

   GLuint last_ReallyEnabled;
   GLuint lodbias_ss2[MAX_TEXTURE_UNITS];


   struct i915_fragment_program *current_program;

   drm_intel_bo *current_vb_bo;
   unsigned int current_vertex_size;

   struct i915_hw_state state;
   uint32_t last_draw_offset;
   GLuint last_sampler;
};


#define I915_STATECHANGE(i915, flag)					\
do {									\
   INTEL_FIREVERTICES( &(i915)->intel );					\
   (i915)->state.emitted &= ~(flag);					\
} while (0)

#define I915_ACTIVESTATE(i915, flag, mode)			\
do {								\
   INTEL_FIREVERTICES( &(i915)->intel );				\
   if (mode)							\
      (i915)->state.active |= (flag);				\
   else								\
      (i915)->state.active &= ~(flag);				\
} while (0)


/*======================================================================
 * i915_vtbl.c
 */
extern void i915InitVtbl(struct i915_context *i915);

extern void
i915_state_draw_region(struct intel_context *intel,
                       struct i915_hw_state *state,
                       struct intel_region *color_region,
                       struct intel_region *depth_region);



#define SZ_TO_HW(sz)  ((sz-2)&0x3)
#define EMIT_SZ(sz)   (EMIT_1F + (sz) - 1)
#define EMIT_ATTR( ATTR, STYLE, S4, SZ )				\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = (ATTR);	\
   intel->vertex_attrs[intel->vertex_attr_count].format = (STYLE);	\
   s4 |= S4;								\
   intel->vertex_attr_count++;						\
   offset += (SZ);							\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = 0;		\
   intel->vertex_attrs[intel->vertex_attr_count].format = EMIT_PAD;	\
   intel->vertex_attrs[intel->vertex_attr_count].offset = (N);		\
   intel->vertex_attr_count++;						\
   offset += (N);							\
} while (0)



/*======================================================================
 * i915_context.c
 */
extern bool i915CreateContext(int api,
			      const struct gl_config * mesaVis,
			      __DRIcontext * driContextPriv,
                              unsigned major_version,
                              unsigned minor_version,
                              unsigned *error,
			      void *sharedContextPrivate);


/*======================================================================
 * i915_debug.c
 */
extern void i915_disassemble_program(const GLuint * program, GLuint sz);
extern void i915_print_ureg(const char *msg, GLuint ureg);


/*======================================================================
 * i915_state.c
 */
extern void i915InitStateFunctions(struct dd_function_table *functions);
extern void i915InitState(struct i915_context *i915);
extern void i915_update_stencil(struct gl_context * ctx);
extern void i915_update_provoking_vertex(struct gl_context *ctx);
extern void i915_update_sprite_point_enable(struct gl_context *ctx);


/*======================================================================
 * i915_tex.c
 */
extern void i915UpdateTextureState(struct intel_context *intel);
extern void i915InitTextureFuncs(struct dd_function_table *functions);

/*======================================================================
 * i915_fragprog.c
 */
extern void i915ValidateFragmentProgram(struct i915_context *i915);
extern void i915InitFragProgFuncs(struct dd_function_table *functions);

/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct i915_context *
i915_context(struct gl_context * ctx)
{
   return (struct i915_context *) ctx;
}



#define I915_CONTEXT(ctx)	i915_context(ctx)



#endif
