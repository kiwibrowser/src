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
              

#ifndef BRW_WM_H
#define BRW_WM_H

#include <stdbool.h>

#include "program/prog_instruction.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_program.h"

#define SATURATE (1<<5)

/* A big lookup table is used to figure out which and how many
 * additional regs will inserted before the main payload in the WM
 * program execution.  These mainly relate to depth and stencil
 * processing and the early-depth-test optimization.
 */
#define IZ_PS_KILL_ALPHATEST_BIT    0x1
#define IZ_PS_COMPUTES_DEPTH_BIT    0x2
#define IZ_DEPTH_WRITE_ENABLE_BIT   0x4
#define IZ_DEPTH_TEST_ENABLE_BIT    0x8
#define IZ_STENCIL_WRITE_ENABLE_BIT 0x10
#define IZ_STENCIL_TEST_ENABLE_BIT  0x20
#define IZ_BIT_MAX                  0x40

#define AA_NEVER     0
#define AA_SOMETIMES 1
#define AA_ALWAYS    2

struct brw_wm_prog_key {
   uint8_t iz_lookup;
   GLuint stats_wm:1;
   GLuint flat_shade:1;
   GLuint nr_color_regions:5;
   GLuint sample_alpha_to_coverage:1;
   GLuint render_to_fbo:1;
   GLuint clamp_fragment_color:1;
   GLuint line_aa:2;

   GLbitfield proj_attrib_mask; /**< one bit per fragment program attribute */

   GLushort drawable_height;
   GLbitfield64 vp_outputs_written;
   GLuint program_string_id:32;

   struct brw_sampler_prog_key_data tex;
};


/* A bit of a glossary:
 *
 * brw_wm_value: A computed value or program input.  Values are
 * constant, they are created once and are never modified.  When a
 * fragment program register is written or overwritten, new values are
 * created fresh, preserving the rule that values are constant.
 *
 * brw_wm_ref: A reference to a value.  Wherever a value used is by an
 * instruction or as a program output, that is tracked with an
 * instance of this struct.  All references to a value occur after it
 * is created.  After the last reference, a value is dead and can be
 * discarded.
 *
 * brw_wm_grf: Represents a physical hardware register.  May be either
 * empty or hold a value.  Register allocation is the process of
 * assigning values to grf registers.  This occurs in pass2 and the
 * brw_wm_grf struct is not used before that.
 *
 * Fragment program registers: These are time-varying constructs that
 * are hard to reason about and which we translate away in pass0.  A
 * single fragment program register element (eg. temp[0].x) will be
 * translated to one or more brw_wm_value structs, one for each time
 * that temp[0].x is written to during the program. 
 */



/* Used in pass2 to track register allocation.
 */
struct brw_wm_grf {
   struct brw_wm_value *value;
   GLuint nextuse;
};

struct brw_wm_value {
   struct brw_reg hw_reg;	/* emitted to this reg, may not always be there */
   struct brw_wm_ref *lastuse;
   struct brw_wm_grf *resident; 
   GLuint contributes_to_output:1;
   GLuint spill_slot:16;	/* if non-zero, spill immediately after calculation */
};

struct brw_wm_ref {
   struct brw_reg hw_reg;	/* nr filled in in pass2, everything else, pass0 */
   struct brw_wm_value *value;
   struct brw_wm_ref *prevuse;
   GLuint unspill_reg:7;	/* unspill to reg */
   GLuint emitted:1;
   GLuint insn:24;
};

struct brw_wm_constref {
   const struct brw_wm_ref *ref;
   GLfloat constval;
};


struct brw_wm_instruction {
   struct brw_wm_value *dst[4];
   struct brw_wm_ref *src[3][4];
   GLuint opcode:8;
   GLuint saturate:1;
   GLuint writemask:4;
   GLuint tex_unit:4;   /* texture unit for TEX, TXD, TXP instructions */
   GLuint tex_idx:4;    /* TEXTURE_1D,2D,3D,CUBE,RECT_INDEX source target */
   GLuint tex_shadow:1; /* do shadow comparison? */
   GLuint eot:1;    	/* End of thread indicator for FB_WRITE*/
   GLuint target:10;    /* target binding table index for FB_WRITE*/
};


#define BRW_WM_MAX_INSN  (MAX_PROGRAM_INSTRUCTIONS*3 + FRAG_ATTRIB_MAX + 3)
#define BRW_WM_MAX_GRF   128		/* hardware limit */
#define BRW_WM_MAX_VREG  (BRW_WM_MAX_INSN * 4)
#define BRW_WM_MAX_REF   (BRW_WM_MAX_INSN * 12)
#define BRW_WM_MAX_PARAM 256
#define BRW_WM_MAX_CONST 256
#define BRW_WM_MAX_SUBROUTINE 16

/* used in masks next to WRITEMASK_*. */
#define SATURATE (1<<5)


/* New opcodes to track internal operations required for WM unit.
 * These are added early so that the registers used can be tracked,
 * freed and reused like those of other instructions.
 */
#define WM_PIXELXY        (MAX_OPCODE)
#define WM_DELTAXY        (MAX_OPCODE + 1)
#define WM_PIXELW         (MAX_OPCODE + 2)
#define WM_LINTERP        (MAX_OPCODE + 3)
#define WM_PINTERP        (MAX_OPCODE + 4)
#define WM_CINTERP        (MAX_OPCODE + 5)
#define WM_WPOSXY         (MAX_OPCODE + 6)
#define WM_FB_WRITE       (MAX_OPCODE + 7)
#define WM_FRONTFACING    (MAX_OPCODE + 8)
#define MAX_WM_OPCODE     (MAX_OPCODE + 9)

#define PROGRAM_PAYLOAD   (PROGRAM_FILE_MAX)
#define NUM_FILES	  (PROGRAM_PAYLOAD + 1)

#define PAYLOAD_DEPTH     (FRAG_ATTRIB_MAX)
#define PAYLOAD_W         (FRAG_ATTRIB_MAX + 1)
#define PAYLOAD_FP_REG_MAX (FRAG_ATTRIB_MAX + 2)

struct brw_wm_compile {
   struct brw_compile func;
   struct brw_wm_prog_key key;
   struct brw_wm_prog_data prog_data;

   struct brw_fragment_program *fp;

   GLfloat (*env_param)[4];

   enum {
      START,
      PASS2_DONE
   } state;

   uint8_t source_depth_reg;
   uint8_t source_w_reg;
   uint8_t aa_dest_stencil_reg;
   uint8_t dest_depth_reg;
   uint8_t barycentric_coord_reg[BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT];
   uint8_t nr_payload_regs;
   GLuint computes_depth:1;	/* could be derived from program string */
   GLuint source_depth_to_render_target:1;
   GLuint runtime_check_aads_emit:1;

   /* Initial pass - translate fp instructions to fp instructions,
    * simplifying and adding instructions for interpolation and
    * framebuffer writes.
    */
   struct prog_instruction *prog_instructions;
   GLuint nr_fp_insns;
   GLuint fp_temp;
   GLuint fp_interp_emitted;

   struct prog_src_register pixel_xy;
   struct prog_src_register delta_xy;
   struct prog_src_register pixel_w;


   struct brw_wm_value *vreg;
   GLuint nr_vreg;

   struct brw_wm_value creg[BRW_WM_MAX_PARAM];
   GLuint nr_creg;

   struct {
      struct brw_wm_value depth[4]; /* includes r0/r1 */
      struct brw_wm_value input_interp[FRAG_ATTRIB_MAX];
   } payload;


   const struct brw_wm_ref *pass0_fp_reg[NUM_FILES][256][4];

   struct brw_wm_ref undef_ref;
   struct brw_wm_value undef_value;

   struct brw_wm_ref *refs;
   GLuint nr_refs;

   struct brw_wm_instruction *instruction;
   GLuint nr_insns;

   struct brw_wm_constref constref[BRW_WM_MAX_CONST];
   GLuint nr_constrefs;

   struct brw_wm_grf pass2_grf[BRW_WM_MAX_GRF/2];

   GLuint grf_limit;
   GLuint max_wm_grf;
   GLuint last_scratch;

   GLuint cur_inst;  /**< index of current instruction */

   bool out_of_regs;  /**< ran out of GRF registers? */

   /** Mapping from Mesa registers to hardware registers */
   struct {
      bool inited;
      struct brw_reg reg;
   } wm_regs[NUM_FILES][256][4];

   bool used_grf[BRW_WM_MAX_GRF];
   GLuint first_free_grf;
   struct brw_reg stack;
   struct brw_reg emit_mask_reg;
   GLuint tmp_regs[BRW_WM_MAX_GRF];
   GLuint tmp_index;
   GLuint tmp_max;
   GLuint subroutines[BRW_WM_MAX_SUBROUTINE];
   GLuint dispatch_width;

   /** we may need up to 3 constants per instruction (if use_const_buffer) */
   struct {
      GLint index;
      struct brw_reg reg;
   } current_const[3];
};


/** Bits for prog_instruction::Aux field */
#define INST_AUX_EOT      0x1
#define INST_AUX_TARGET(T)  (T << 1)
#define INST_AUX_GET_TARGET(AUX) ((AUX) >> 1)


GLuint brw_wm_nr_args( GLuint opcode );
GLuint brw_wm_is_scalar_result( GLuint opcode );

void brw_wm_pass_fp( struct brw_wm_compile *c );
void brw_wm_pass0( struct brw_wm_compile *c );
void brw_wm_pass1( struct brw_wm_compile *c );
void brw_wm_pass2( struct brw_wm_compile *c );
void brw_wm_emit( struct brw_wm_compile *c );
bool brw_wm_arg_can_be_immediate(enum prog_opcode, int arg);
void brw_wm_print_value( struct brw_wm_compile *c,
			 struct brw_wm_value *value );

void brw_wm_print_ref( struct brw_wm_compile *c,
		       struct brw_wm_ref *ref );

void brw_wm_print_insn( struct brw_wm_compile *c,
			struct brw_wm_instruction *inst );

void brw_wm_print_program( struct brw_wm_compile *c,
			   const char *stage );

void brw_wm_lookup_iz(struct intel_context *intel,
		      struct brw_wm_compile *c);

bool brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c,
		    struct gl_shader_program *prog);

/* brw_wm_emit.c */
void emit_alu1(struct brw_compile *p,
	       struct brw_instruction *(*func)(struct brw_compile *,
					       struct brw_reg,
					       struct brw_reg),
	       const struct brw_reg *dst,
	       GLuint mask,
	       const struct brw_reg *arg0);
void emit_alu2(struct brw_compile *p,
	       struct brw_instruction *(*func)(struct brw_compile *,
					       struct brw_reg,
					       struct brw_reg,
					       struct brw_reg),
	       const struct brw_reg *dst,
	       GLuint mask,
	       const struct brw_reg *arg0,
	       const struct brw_reg *arg1);
void emit_cinterp(struct brw_compile *p,
		  const struct brw_reg *dst,
		  GLuint mask,
		  const struct brw_reg *arg0);
void emit_cmp(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1,
	      const struct brw_reg *arg2);
void emit_ddxy(struct brw_compile *p,
	       const struct brw_reg *dst,
	       GLuint mask,
	       bool is_ddx,
	       const struct brw_reg *arg0,
	       bool negate_value);
void emit_delta_xy(struct brw_compile *p,
		   const struct brw_reg *dst,
		   GLuint mask,
		   const struct brw_reg *arg0);
void emit_dp2(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);
void emit_dp3(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);
void emit_dp4(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);
void emit_dph(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);
void emit_fb_write(struct brw_wm_compile *c,
		   struct brw_reg *arg0,
		   struct brw_reg *arg1,
		   struct brw_reg *arg2,
		   GLuint target,
		   GLuint eot);
void emit_frontfacing(struct brw_compile *p,
		      const struct brw_reg *dst,
		      GLuint mask);
void emit_linterp(struct brw_compile *p,
		  const struct brw_reg *dst,
		  GLuint mask,
		  const struct brw_reg *arg0,
		  const struct brw_reg *deltas);
void emit_lrp(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1,
	      const struct brw_reg *arg2);
void emit_mad(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1,
	      const struct brw_reg *arg2);
void emit_math1(struct brw_wm_compile *c,
		GLuint function,
		const struct brw_reg *dst,
		GLuint mask,
		const struct brw_reg *arg0);
void emit_math2(struct brw_wm_compile *c,
		GLuint function,
		const struct brw_reg *dst,
		GLuint mask,
		const struct brw_reg *arg0,
		const struct brw_reg *arg1);
void emit_min(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);
void emit_max(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);
void emit_pinterp(struct brw_compile *p,
		  const struct brw_reg *dst,
		  GLuint mask,
		  const struct brw_reg *arg0,
		  const struct brw_reg *deltas,
		  const struct brw_reg *w);
void emit_pixel_xy(struct brw_wm_compile *c,
		   const struct brw_reg *dst,
		   GLuint mask);
void emit_pixel_w(struct brw_wm_compile *c,
		  const struct brw_reg *dst,
		  GLuint mask,
		  const struct brw_reg *arg0,
		  const struct brw_reg *deltas);
void emit_sop(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      GLuint cond,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);
void emit_sign(struct brw_compile *p,
	       const struct brw_reg *dst,
	       GLuint mask,
	       const struct brw_reg *arg0);
void emit_tex(struct brw_wm_compile *c,
	      struct brw_reg *dst,
	      GLuint dst_flags,
	      struct brw_reg *arg,
	      struct brw_reg depth_payload,
	      GLuint tex_idx,
	      GLuint sampler,
	      bool shadow);
void emit_txb(struct brw_wm_compile *c,
	      struct brw_reg *dst,
	      GLuint dst_flags,
	      struct brw_reg *arg,
	      struct brw_reg depth_payload,
	      GLuint tex_idx,
	      GLuint sampler);
void emit_wpos_xy(struct brw_wm_compile *c,
		  const struct brw_reg *dst,
		  GLuint mask,
		  const struct brw_reg *arg0);
void emit_xpd(struct brw_compile *p,
	      const struct brw_reg *dst,
	      GLuint mask,
	      const struct brw_reg *arg0,
	      const struct brw_reg *arg1);

GLboolean brw_link_shader(struct gl_context *ctx, struct gl_shader_program *prog);
struct gl_shader *brw_new_shader(struct gl_context *ctx, GLuint name, GLuint type);
struct gl_shader_program *brw_new_shader_program(struct gl_context *ctx, GLuint name);

bool brw_color_buffer_write_enabled(struct brw_context *brw);
bool brw_render_target_supported(struct intel_context *intel,
				 struct gl_renderbuffer *rb);
void brw_wm_payload_setup(struct brw_context *brw,
			  struct brw_wm_compile *c);
bool do_wm_prog(struct brw_context *brw,
		struct gl_shader_program *prog,
		struct brw_fragment_program *fp,
		struct brw_wm_prog_key *key);
void brw_wm_debug_recompile(struct brw_context *brw,
                            struct gl_shader_program *prog,
                            const struct brw_wm_prog_key *key);

#endif
