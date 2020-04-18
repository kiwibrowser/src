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
             
#include "brw_context.h"
#include "brw_wm.h"
#include "brw_state.h"
#include "main/formats.h"
#include "main/fbobject.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"

#include "glsl/ralloc.h"

/** Return number of src args for given instruction */
GLuint brw_wm_nr_args( GLuint opcode )
{
   switch (opcode) {
   case WM_FRONTFACING:
   case WM_PIXELXY:
      return 0;
   case WM_CINTERP:
   case WM_WPOSXY:
   case WM_DELTAXY:
      return 1;
   case WM_LINTERP:
   case WM_PIXELW:
      return 2;
   case WM_FB_WRITE:
   case WM_PINTERP:
      return 3;
   default:
      assert(opcode < MAX_OPCODE);
      return _mesa_num_inst_src_regs(opcode);
   }
}


GLuint brw_wm_is_scalar_result( GLuint opcode )
{
   switch (opcode) {
   case OPCODE_COS:
   case OPCODE_EX2:
   case OPCODE_LG2:
   case OPCODE_POW:
   case OPCODE_RCP:
   case OPCODE_RSQ:
   case OPCODE_SIN:
   case OPCODE_DP2:
   case OPCODE_DP3:
   case OPCODE_DP4:
   case OPCODE_DPH:
   case OPCODE_DST:
      return 1;
      
   default:
      return 0;
   }
}


/**
 * Do GPU code generation for non-GLSL shader.  non-GLSL shaders have
 * no flow control instructions so we can more readily do SSA-style
 * optimizations.
 */
static void
brw_wm_non_glsl_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
   /* Augment fragment program.  Add instructions for pre- and
    * post-fragment-program tasks such as interpolation and fogging.
    */
   brw_wm_pass_fp(c);

   /* Translate to intermediate representation.  Build register usage
    * chains.
    */
   brw_wm_pass0(c);

   /* Dead code removal.
    */
   brw_wm_pass1(c);

   /* Register allocation.
    * Divide by two because we operate on 16 pixels at a time and require
    * two GRF entries for each logical shader register.
    */
   c->grf_limit = BRW_WM_MAX_GRF / 2;

   brw_wm_pass2(c);

   /* how many general-purpose registers are used */
   c->prog_data.reg_blocks = brw_register_blocks(c->max_wm_grf);

   /* Emit GEN4 code.
    */
   brw_wm_emit(c);
}


/**
 * Return a bitfield where bit n is set if barycentric interpolation mode n
 * (see enum brw_wm_barycentric_interp_mode) is needed by the fragment shader.
 */
static unsigned
brw_compute_barycentric_interp_modes(struct brw_context *brw,
                                     bool shade_model_flat,
                                     const struct gl_fragment_program *fprog)
{
   unsigned barycentric_interp_modes = 0;
   int attr;

   /* Loop through all fragment shader inputs to figure out what interpolation
    * modes are in use, and set the appropriate bits in
    * barycentric_interp_modes.
    */
   for (attr = 0; attr < FRAG_ATTRIB_MAX; ++attr) {
      enum glsl_interp_qualifier interp_qualifier =
         fprog->InterpQualifier[attr];
      bool is_centroid = fprog->IsCentroid & BITFIELD64_BIT(attr);
      bool is_gl_Color = attr == FRAG_ATTRIB_COL0 || attr == FRAG_ATTRIB_COL1;

      /* Ignore unused inputs. */
      if (!(fprog->Base.InputsRead & BITFIELD64_BIT(attr)))
         continue;

      /* Ignore WPOS and FACE, because they don't require interpolation. */
      if (attr == FRAG_ATTRIB_WPOS || attr == FRAG_ATTRIB_FACE)
         continue;

      /* Determine the set (or sets) of barycentric coordinates needed to
       * interpolate this variable.  Note that when
       * brw->needs_unlit_centroid_workaround is set, centroid interpolation
       * uses PIXEL interpolation for unlit pixels and CENTROID interpolation
       * for lit pixels, so we need both sets of barycentric coordinates.
       */
      if (interp_qualifier == INTERP_QUALIFIER_NOPERSPECTIVE) {
         if (is_centroid) {
            barycentric_interp_modes |=
               1 << BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC;
         }
         if (!is_centroid || brw->needs_unlit_centroid_workaround) {
            barycentric_interp_modes |=
               1 << BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC;
         }
      } else if (interp_qualifier == INTERP_QUALIFIER_SMOOTH ||
                 (!(shade_model_flat && is_gl_Color) &&
                  interp_qualifier == INTERP_QUALIFIER_NONE)) {
         if (is_centroid) {
            barycentric_interp_modes |=
               1 << BRW_WM_PERSPECTIVE_CENTROID_BARYCENTRIC;
         }
         if (!is_centroid || brw->needs_unlit_centroid_workaround) {
            barycentric_interp_modes |=
               1 << BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC;
         }
      }
   }

   return barycentric_interp_modes;
}


void
brw_wm_payload_setup(struct brw_context *brw,
		     struct brw_wm_compile *c)
{
   struct intel_context *intel = &brw->intel;
   bool uses_depth = (c->fp->program.Base.InputsRead &
		      (1 << FRAG_ATTRIB_WPOS)) != 0;
   unsigned barycentric_interp_modes = c->prog_data.barycentric_interp_modes;
   int i;

   if (intel->gen >= 6) {
      /* R0-1: masks, pixel X/Y coordinates. */
      c->nr_payload_regs = 2;
      /* R2: only for 32-pixel dispatch.*/

      /* R3-26: barycentric interpolation coordinates.  These appear in the
       * same order that they appear in the brw_wm_barycentric_interp_mode
       * enum.  Each set of coordinates occupies 2 registers if dispatch width
       * == 8 and 4 registers if dispatch width == 16.  Coordinates only
       * appear if they were enabled using the "Barycentric Interpolation
       * Mode" bits in WM_STATE.
       */
      for (i = 0; i < BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT; ++i) {
         if (barycentric_interp_modes & (1 << i)) {
            c->barycentric_coord_reg[i] = c->nr_payload_regs;
            c->nr_payload_regs += 2;
            if (c->dispatch_width == 16) {
               c->nr_payload_regs += 2;
            }
         }
      }

      /* R27: interpolated depth if uses source depth */
      if (uses_depth) {
	 c->source_depth_reg = c->nr_payload_regs;
	 c->nr_payload_regs++;
	 if (c->dispatch_width == 16) {
	    /* R28: interpolated depth if not 8-wide. */
	    c->nr_payload_regs++;
	 }
      }
      /* R29: interpolated W set if GEN6_WM_USES_SOURCE_W.
       */
      if (uses_depth) {
	 c->source_w_reg = c->nr_payload_regs;
	 c->nr_payload_regs++;
	 if (c->dispatch_width == 16) {
	    /* R30: interpolated W if not 8-wide. */
	    c->nr_payload_regs++;
	 }
      }
      /* R31: MSAA position offsets. */
      /* R32-: bary for 32-pixel. */
      /* R58-59: interp W for 32-pixel. */

      if (c->fp->program.Base.OutputsWritten &
	  BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
	 c->source_depth_to_render_target = true;
	 c->computes_depth = true;
      }
   } else {
      brw_wm_lookup_iz(intel, c);
   }
}

/**
 * All Mesa program -> GPU code generation goes through this function.
 * Depending on the instructions used (i.e. flow control instructions)
 * we'll use one of two code generators.
 */
bool do_wm_prog(struct brw_context *brw,
		struct gl_shader_program *prog,
		struct brw_fragment_program *fp,
		struct brw_wm_prog_key *key)
{
   struct intel_context *intel = &brw->intel;
   struct brw_wm_compile *c;
   const GLuint *program;
   GLuint program_size;

   c = brw->wm.compile_data;
   if (c == NULL) {
      brw->wm.compile_data = rzalloc(NULL, struct brw_wm_compile);
      c = brw->wm.compile_data;
      if (c == NULL) {
         /* Ouch - big out of memory problem.  Can't continue
          * without triggering a segfault, no way to signal,
          * so just return.
          */
         return false;
      }
   } else {
      void *instruction = c->instruction;
      void *prog_instructions = c->prog_instructions;
      void *vreg = c->vreg;
      void *refs = c->refs;
      memset(c, 0, sizeof(*brw->wm.compile_data));
      c->instruction = instruction;
      c->prog_instructions = prog_instructions;
      c->vreg = vreg;
      c->refs = refs;
   }
   memcpy(&c->key, key, sizeof(*key));

   c->fp = fp;
   c->env_param = brw->intel.ctx.FragmentProgram.Parameters;

   brw_init_compile(brw, &c->func, c);

   c->prog_data.barycentric_interp_modes =
      brw_compute_barycentric_interp_modes(brw, c->key.flat_shade,
                                           &fp->program);

   if (prog && prog->_LinkedShaders[MESA_SHADER_FRAGMENT]) {
      if (!brw_wm_fs_emit(brw, c, prog))
	 return false;
   } else {
      if (!c->instruction) {
	 c->instruction = rzalloc_array(c, struct brw_wm_instruction, BRW_WM_MAX_INSN);
	 c->prog_instructions = rzalloc_array(c, struct prog_instruction, BRW_WM_MAX_INSN);
	 c->vreg = rzalloc_array(c, struct brw_wm_value, BRW_WM_MAX_VREG);
	 c->refs = rzalloc_array(c, struct brw_wm_ref, BRW_WM_MAX_REF);
      }

      /* Fallback for fixed function and ARB_fp shaders. */
      c->dispatch_width = 16;
      brw_wm_payload_setup(brw, c);
      brw_wm_non_glsl_emit(brw, c);
      c->prog_data.dispatch_width = 16;
   }

   /* Scratch space is used for register spilling */
   if (c->last_scratch) {
      perf_debug("Fragment shader triggered register spilling.  "
                 "Try reducing the number of live scalar values to "
                 "improve performance.\n");

      c->prog_data.total_scratch = brw_get_scratch_size(c->last_scratch);

      brw_get_scratch_bo(intel, &brw->wm.scratch_bo,
			 c->prog_data.total_scratch * brw->max_wm_threads);
   }

   if (unlikely(INTEL_DEBUG & DEBUG_WM))
      fprintf(stderr, "\n");

   /* get the program
    */
   program = brw_get_program(&c->func, &program_size);

   brw_upload_cache(&brw->cache, BRW_WM_PROG,
		    &c->key, sizeof(c->key),
		    program, program_size,
		    &c->prog_data, sizeof(c->prog_data),
		    &brw->wm.prog_offset, &brw->wm.prog_data);

   return true;
}

static bool
key_debug(const char *name, int a, int b)
{
   if (a != b) {
      perf_debug("  %s %d->%d\n", name, a, b);
      return true;
   } else {
      return false;
   }
}

bool
brw_debug_recompile_sampler_key(const struct brw_sampler_prog_key_data *old_key,
                                const struct brw_sampler_prog_key_data *key)
{
   bool found = false;

   for (unsigned int i = 0; i < MAX_SAMPLERS; i++) {
      found |= key_debug("EXT_texture_swizzle or DEPTH_TEXTURE_MODE",
                         old_key->swizzles[i], key->swizzles[i]);
   }
   found |= key_debug("GL_CLAMP enabled on any texture unit's 1st coordinate",
                      old_key->gl_clamp_mask[0], key->gl_clamp_mask[0]);
   found |= key_debug("GL_CLAMP enabled on any texture unit's 2nd coordinate",
                      old_key->gl_clamp_mask[1], key->gl_clamp_mask[1]);
   found |= key_debug("GL_CLAMP enabled on any texture unit's 3rd coordinate",
                      old_key->gl_clamp_mask[2], key->gl_clamp_mask[2]);
   found |= key_debug("GL_MESA_ycbcr texturing\n",
                      old_key->yuvtex_mask, key->yuvtex_mask);
   found |= key_debug("GL_MESA_ycbcr UV swapping\n",
                      old_key->yuvtex_swap_mask, key->yuvtex_swap_mask);

   return found;
}

void
brw_wm_debug_recompile(struct brw_context *brw,
                       struct gl_shader_program *prog,
                       const struct brw_wm_prog_key *key)
{
   struct brw_cache_item *c = NULL;
   const struct brw_wm_prog_key *old_key = NULL;
   bool found = false;

   perf_debug("Recompiling fragment shader for program %d\n", prog->Name);

   for (unsigned int i = 0; i < brw->cache.size; i++) {
      for (c = brw->cache.items[i]; c; c = c->next) {
         if (c->cache_id == BRW_WM_PROG) {
            old_key = c->key;

            if (old_key->program_string_id == key->program_string_id)
               break;
         }
      }
      if (c)
         break;
   }

   if (!c) {
      perf_debug("  Didn't find previous compile in the shader cache for "
                 "debug\n");
      return;
   }

   found |= key_debug("alphatest, computed depth, depth test, or depth write",
                      old_key->iz_lookup, key->iz_lookup);
   found |= key_debug("depth statistics", old_key->stats_wm, key->stats_wm);
   found |= key_debug("flat shading", old_key->flat_shade, key->flat_shade);
   found |= key_debug("number of color buffers", old_key->nr_color_regions, key->nr_color_regions);
   found |= key_debug("rendering to FBO", old_key->render_to_fbo, key->render_to_fbo);
   found |= key_debug("fragment color clamping", old_key->clamp_fragment_color, key->clamp_fragment_color);
   found |= key_debug("line smoothing", old_key->line_aa, key->line_aa);
   found |= key_debug("proj_attrib_mask", old_key->proj_attrib_mask, key->proj_attrib_mask);
   found |= key_debug("renderbuffer height", old_key->drawable_height, key->drawable_height);
   found |= key_debug("vertex shader outputs", old_key->vp_outputs_written, key->vp_outputs_written);

   found |= brw_debug_recompile_sampler_key(&old_key->tex, &key->tex);

   if (!found) {
      perf_debug("  Something else\n");
   }
}

void
brw_populate_sampler_prog_key_data(struct gl_context *ctx,
				   const struct gl_program *prog,
				   struct brw_sampler_prog_key_data *key)
{
   struct intel_context *intel = intel_context(ctx);

   for (int s = 0; s < MAX_SAMPLERS; s++) {
      key->swizzles[s] = SWIZZLE_NOOP;

      if (!(prog->SamplersUsed & (1 << s)))
	 continue;

      int unit_id = prog->SamplerUnits[s];
      const struct gl_texture_unit *unit = &ctx->Texture.Unit[unit_id];

      if (unit->_ReallyEnabled && unit->_Current->Target != GL_TEXTURE_BUFFER) {
	 const struct gl_texture_object *t = unit->_Current;
	 const struct gl_texture_image *img = t->Image[0][t->BaseLevel];
	 struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit_id);

         const bool alpha_depth = t->DepthMode == GL_ALPHA &&
            (img->_BaseFormat == GL_DEPTH_COMPONENT ||
             img->_BaseFormat == GL_DEPTH_STENCIL);

         /* Haswell handles texture swizzling as surface format overrides
          * (except for GL_ALPHA); all other platforms need MOVs in the shader.
          */
         if (!intel->is_haswell || alpha_depth)
            key->swizzles[s] = brw_get_texture_swizzle(t);

	 if (img->InternalFormat == GL_YCBCR_MESA) {
	    key->yuvtex_mask |= 1 << s;
	    if (img->TexFormat == MESA_FORMAT_YCBCR)
		key->yuvtex_swap_mask |= 1 << s;
	 }

	 if (sampler->MinFilter != GL_NEAREST &&
	     sampler->MagFilter != GL_NEAREST) {
	    if (sampler->WrapS == GL_CLAMP)
	       key->gl_clamp_mask[0] |= 1 << s;
	    if (sampler->WrapT == GL_CLAMP)
	       key->gl_clamp_mask[1] |= 1 << s;
	    if (sampler->WrapR == GL_CLAMP)
	       key->gl_clamp_mask[2] |= 1 << s;
	 }
      }
   }
}

static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct brw_fragment_program *fp = 
      (struct brw_fragment_program *)brw->fragment_program;
   const struct gl_program *prog = (struct gl_program *) brw->fragment_program;
   GLuint lookup = 0;
   GLuint line_aa;
   bool program_uses_dfdy = fp->program.UsesDFdy;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   if (intel->gen < 6) {
      /* _NEW_COLOR */
      if (fp->program.UsesKill || ctx->Color.AlphaEnabled)
	 lookup |= IZ_PS_KILL_ALPHATEST_BIT;

      if (fp->program.Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
	 lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

      /* _NEW_DEPTH */
      if (ctx->Depth.Test)
	 lookup |= IZ_DEPTH_TEST_ENABLE_BIT;

      if (ctx->Depth.Test && ctx->Depth.Mask) /* ?? */
	 lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

      /* _NEW_STENCIL */
      if (ctx->Stencil._Enabled) {
	 lookup |= IZ_STENCIL_TEST_ENABLE_BIT;

	 if (ctx->Stencil.WriteMask[0] ||
	     ctx->Stencil.WriteMask[ctx->Stencil._BackFace])
	    lookup |= IZ_STENCIL_WRITE_ENABLE_BIT;
      }
      key->iz_lookup = lookup;
   }

   line_aa = AA_NEVER;

   /* _NEW_LINE, _NEW_POLYGON, BRW_NEW_REDUCED_PRIMITIVE */
   if (ctx->Line.SmoothFlag) {
      if (brw->intel.reduced_primitive == GL_LINES) {
	 line_aa = AA_ALWAYS;
      }
      else if (brw->intel.reduced_primitive == GL_TRIANGLES) {
	 if (ctx->Polygon.FrontMode == GL_LINE) {
	    line_aa = AA_SOMETIMES;

	    if (ctx->Polygon.BackMode == GL_LINE ||
		(ctx->Polygon.CullFlag &&
		 ctx->Polygon.CullFaceMode == GL_BACK))
	       line_aa = AA_ALWAYS;
	 }
	 else if (ctx->Polygon.BackMode == GL_LINE) {
	    line_aa = AA_SOMETIMES;

	    if ((ctx->Polygon.CullFlag &&
		 ctx->Polygon.CullFaceMode == GL_FRONT))
	       line_aa = AA_ALWAYS;
	 }
      }
   }

   key->line_aa = line_aa;

   if (intel->gen < 6)
      key->stats_wm = brw->intel.stats_wm;

   /* BRW_NEW_WM_INPUT_DIMENSIONS */
   /* Only set this for fixed function.  The optimization it enables isn't
    * useful for programs using shaders.
    */
   if (ctx->Shader.CurrentFragmentProgram)
      key->proj_attrib_mask = 0xffffffff;
   else
      key->proj_attrib_mask = brw->wm.input_size_masks[4-1];

   /* _NEW_LIGHT */
   key->flat_shade = (ctx->Light.ShadeModel == GL_FLAT);

   /* _NEW_FRAG_CLAMP | _NEW_BUFFERS */
   key->clamp_fragment_color = ctx->Color._ClampFragmentColor;

   /* _NEW_TEXTURE */
   brw_populate_sampler_prog_key_data(ctx, prog, &key->tex);

   /* _NEW_BUFFERS */
   /*
    * Include the draw buffer origin and height so that we can calculate
    * fragment position values relative to the bottom left of the drawable,
    * from the incoming screen origin relative position we get as part of our
    * payload.
    *
    * This is only needed for the WM_WPOSXY opcode when the fragment program
    * uses the gl_FragCoord input.
    *
    * We could avoid recompiling by including this as a constant referenced by
    * our program, but if we were to do that it would also be nice to handle
    * getting that constant updated at batchbuffer submit time (when we
    * hold the lock and know where the buffer really is) rather than at emit
    * time when we don't hold the lock and are just guessing.  We could also
    * just avoid using this as key data if the program doesn't use
    * fragment.position.
    *
    * For DRI2 the origin_x/y will always be (0,0) but we still need the
    * drawable height in order to invert the Y axis.
    */
   if (fp->program.Base.InputsRead & FRAG_BIT_WPOS) {
      key->drawable_height = ctx->DrawBuffer->Height;
   }

   if ((fp->program.Base.InputsRead & FRAG_BIT_WPOS) || program_uses_dfdy) {
      key->render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   }

   /* _NEW_BUFFERS */
   key->nr_color_regions = ctx->DrawBuffer->_NumColorDrawBuffers;
  /* _NEW_MULTISAMPLE */
   key->sample_alpha_to_coverage = ctx->Multisample.SampleAlphaToCoverage;

   /* CACHE_NEW_VS_PROG */
   if (intel->gen < 6)
      key->vp_outputs_written = brw->vs.prog_data->outputs_written;

   /* The unique fragment program ID */
   key->program_string_id = fp->id;
}


static void
brw_upload_wm_prog(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct brw_wm_prog_key key;
   struct brw_fragment_program *fp = (struct brw_fragment_program *)
      brw->fragment_program;

   brw_wm_populate_key(brw, &key);

   if (!brw_search_cache(&brw->cache, BRW_WM_PROG,
			 &key, sizeof(key),
			 &brw->wm.prog_offset, &brw->wm.prog_data)) {
      bool success = do_wm_prog(brw, ctx->Shader._CurrentFragmentProgram, fp,
				&key);
      (void) success;
      assert(success);
   }
}


const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .mesa  = (_NEW_COLOR |
		_NEW_DEPTH |
		_NEW_STENCIL |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_LIGHT |
		_NEW_FRAG_CLAMP |
		_NEW_BUFFERS |
		_NEW_TEXTURE |
		_NEW_MULTISAMPLE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = brw_upload_wm_prog
};

