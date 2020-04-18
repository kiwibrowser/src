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
  
#include "main/imports.h"
#include "main/enums.h"
#include "main/shaderobj.h"
#include "program/prog_parameter.h"
#include "program/program.h"
#include "program/programopt.h"
#include "tnl/tnl.h"
#include "glsl/ralloc.h"

#include "brw_context.h"
#include "brw_wm.h"

static void brwBindProgram( struct gl_context *ctx,
			    GLenum target, 
			    struct gl_program *prog )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: 
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      break;
   }
}

static struct gl_program *brwNewProgram( struct gl_context *ctx,
				      GLenum target, 
				      GLuint id )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct brw_vertex_program *prog = CALLOC_STRUCT(brw_vertex_program);
      if (prog) {
	 prog->id = brw->program_id++;

	 return _mesa_init_vertex_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   case GL_FRAGMENT_PROGRAM_ARB: {
      struct brw_fragment_program *prog = CALLOC_STRUCT(brw_fragment_program);
      if (prog) {
	 prog->id = brw->program_id++;

	 return _mesa_init_fragment_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   default:
      return _mesa_new_program(ctx, target, id);
   }
}

static void brwDeleteProgram( struct gl_context *ctx,
			      struct gl_program *prog )
{
   _mesa_delete_program( ctx, prog );
}


static GLboolean
brwIsProgramNative(struct gl_context *ctx,
		   GLenum target,
		   struct gl_program *prog)
{
   return true;
}

static void
shader_error(struct gl_context *ctx, struct gl_program *prog, const char *msg)
{
   struct gl_shader_program *shader;

   shader = _mesa_lookup_shader_program(ctx, prog->Id);

   if (shader) {
      ralloc_strcat(&shader->InfoLog, msg);
      shader->LinkStatus = false;
   }
}

static GLboolean
brwProgramStringNotify(struct gl_context *ctx,
		       GLenum target,
		       struct gl_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   int i;

   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct gl_fragment_program *fprog = (struct gl_fragment_program *) prog;
      struct brw_fragment_program *newFP = brw_fragment_program(fprog);
      const struct brw_fragment_program *curFP =
         brw_fragment_program_const(brw->fragment_program);
      struct gl_shader_program *shader_program;

      if (newFP == curFP)
	 brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      newFP->id = brw->program_id++;      

      /* Don't reject fragment shaders for their Mesa IR state when we're
       * using the new FS backend.
       */
      shader_program = _mesa_lookup_shader_program(ctx, prog->Id);
      if (shader_program
	  && shader_program->_LinkedShaders[MESA_SHADER_FRAGMENT]) {
	 return true;
      }
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct gl_vertex_program *vprog = (struct gl_vertex_program *) prog;
      struct brw_vertex_program *newVP = brw_vertex_program(vprog);
      const struct brw_vertex_program *curVP =
         brw_vertex_program_const(brw->vertex_program);

      if (newVP == curVP)
	 brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      if (newVP->program.IsPositionInvariant) {
	 _mesa_insert_mvp_code(ctx, &newVP->program);
      }
      newVP->id = brw->program_id++;      

      /* Also tell tnl about it:
       */
      _tnl_program_string(ctx, target, prog);
   }

   /* Reject programs with subroutines, which are totally broken at the moment
    * (all program flows return when any program flow returns, and
    * the VS also hangs if a function call calls a function.
    *
    * See piglit glsl-{vs,fs}-functions-[23] tests.
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      int r;

      if (prog->Instructions[i].Opcode == OPCODE_CAL) {
	 shader_error(ctx, prog,
		      "i965 driver doesn't yet support uninlined function "
		      "calls.  Move to using a single return statement at "
		      "the end of the function to work around it.\n");
	 return false;
      }

      if (prog->Instructions[i].Opcode == OPCODE_RET) {
	 shader_error(ctx, prog,
		      "i965 driver doesn't yet support \"return\" "
		      "from main().\n");
	 return false;
      }

      for (r = 0; r < _mesa_num_inst_src_regs(inst->Opcode); r++) {
	 if (prog->Instructions[i].SrcReg[r].RelAddr &&
	     prog->Instructions[i].SrcReg[r].File == PROGRAM_INPUT) {
	    shader_error(ctx, prog,
			 "Variable indexing of shader inputs unsupported\n");
	    return false;
	 }
      }

      if (target == GL_FRAGMENT_PROGRAM_ARB &&
	  prog->Instructions[i].DstReg.RelAddr &&
	  prog->Instructions[i].DstReg.File == PROGRAM_OUTPUT) {
	 shader_error(ctx, prog,
		      "Variable indexing of FS outputs unsupported\n");
	 return false;
      }
      if (target == GL_FRAGMENT_PROGRAM_ARB) {
	 if ((prog->Instructions[i].DstReg.RelAddr &&
	      prog->Instructions[i].DstReg.File == PROGRAM_TEMPORARY) ||
	     (prog->Instructions[i].SrcReg[0].RelAddr &&
	      prog->Instructions[i].SrcReg[0].File == PROGRAM_TEMPORARY) ||
	     (prog->Instructions[i].SrcReg[1].RelAddr &&
	      prog->Instructions[i].SrcReg[1].File == PROGRAM_TEMPORARY) ||
	     (prog->Instructions[i].SrcReg[2].RelAddr &&
	      prog->Instructions[i].SrcReg[2].File == PROGRAM_TEMPORARY)) {
	    shader_error(ctx, prog,
			 "Variable indexing of variable arrays in the FS "
			 "unsupported\n");
	    return false;
	 }
      }
   }

   return true;
}

/* Per-thread scratch space is a power-of-two multiple of 1KB. */
int
brw_get_scratch_size(int size)
{
   int i;

   for (i = 1024; i < size; i *= 2)
      ;

   return i;
}

void
brw_get_scratch_bo(struct intel_context *intel,
		   drm_intel_bo **scratch_bo, int size)
{
   drm_intel_bo *old_bo = *scratch_bo;

   if (old_bo && old_bo->size < size) {
      drm_intel_bo_unreference(old_bo);
      old_bo = NULL;
   }

   if (!old_bo) {
      *scratch_bo = drm_intel_bo_alloc(intel->bufmgr, "scratch bo", size, 4096);
   }
}

void brwInitFragProgFuncs( struct dd_function_table *functions )
{
   assert(functions->ProgramStringNotify == _tnl_program_string); 

   functions->BindProgram = brwBindProgram;
   functions->NewProgram = brwNewProgram;
   functions->DeleteProgram = brwDeleteProgram;
   functions->IsProgramNative = brwIsProgramNative;
   functions->ProgramStringNotify = brwProgramStringNotify;

   functions->NewShader = brw_new_shader;
   functions->NewShaderProgram = brw_new_shader_program;
   functions->LinkShader = brw_link_shader;
}

