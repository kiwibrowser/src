/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include <stdbool.h>
#include "main/glheader.h"
#include "main/api_arrayelt.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "main/imports.h"
#include "main/extensions.h"
#include "main/mfeatures.h"
#include "main/version.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_tex.h"
#include "r200_swtcl.h"
#include "r200_tcl.h"
#include "r200_vertprog.h"
#include "radeon_queryobj.h"
#include "r200_blit.h"
#include "radeon_fog.h"

#include "radeon_span.h"

#include "utils.h"
#include "xmlpool.h" /* for symbolic values of enum-type options */

/* Return various strings for glGetString().
 */
static const GLubyte *r200GetString( struct gl_context *ctx, GLenum name )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   GLuint agp_mode = (rmesa->radeon.radeonScreen->card_type == RADEON_CARD_PCI)? 0 :
      rmesa->radeon.radeonScreen->AGPMode;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"Tungsten Graphics, Inc.";

   case GL_RENDERER:
      offset = driGetRendererString( buffer, "R200", agp_mode );

      sprintf( & buffer[ offset ], " %sTCL",
	       !(rmesa->radeon.TclFallback & R200_TCL_FALLBACK_TCL_DISABLE)
	       ? "" : "NO-" );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}


extern const struct tnl_pipeline_stage _r200_render_stage;
extern const struct tnl_pipeline_stage _r200_tcl_stage;

static const struct tnl_pipeline_stage *r200_pipeline[] = {

   /* Try and go straight to t&l
    */
   &_r200_tcl_stage,  

   /* Catch any t&l fallbacks
    */
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
   &_tnl_vertex_program_stage,
   /* Try again to go to tcl? 
    *     - no good for asymmetric-twoside (do with multipass)
    *     - no good for asymmetric-unfilled (do with multipass)
    *     - good for material
    *     - good for texgen
    *     - need to manipulate a bit of state
    *
    * - worth it/not worth it?
    */
			
   /* Else do them here.
    */
/*    &_r200_render_stage,  */ /* FIXME: bugs with ut2003 */
   &_tnl_render_stage,		/* FALLBACK:  */
   NULL,
};



/* Initialize the driver's misc functions.
 */
static void r200InitDriverFuncs( struct dd_function_table *functions )
{
    functions->GetBufferSize		= NULL; /* OBSOLETE */
    functions->GetString		= r200GetString;
}


static void r200_get_lock(radeonContextPtr radeon)
{
   r200ContextPtr rmesa = (r200ContextPtr)radeon;
   drm_radeon_sarea_t *sarea = radeon->sarea;

   R200_STATECHANGE( rmesa, ctx );
   if (rmesa->radeon.sarea->tiling_enabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |= R200_COLOR_TILE_ENABLE;
   }
   else rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] &= ~R200_COLOR_TILE_ENABLE;

   if ( sarea->ctx_owner != rmesa->radeon.dri.hwContext ) {
      sarea->ctx_owner = rmesa->radeon.dri.hwContext;
   }

}

static void r200_vtbl_emit_cs_header(struct radeon_cs *cs, radeonContextPtr rmesa)
{
}

static void r200_emit_query_finish(radeonContextPtr radeon)
{
   BATCH_LOCALS(radeon);
   struct radeon_query_object *query = radeon->query.current;

   BEGIN_BATCH_NO_AUTOSTATE(4);
   OUT_BATCH(CP_PACKET0(RADEON_RB3D_ZPASS_ADDR, 0));
   OUT_BATCH_RELOC(0, query->bo, query->curr_offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
   END_BATCH();
   query->curr_offset += sizeof(uint32_t);
   assert(query->curr_offset < RADEON_QUERY_PAGE_SIZE);
   query->emitted_begin = GL_FALSE;
}

static void r200_init_vtbl(radeonContextPtr radeon)
{
   radeon->vtbl.get_lock = r200_get_lock;
   radeon->vtbl.update_viewport_offset = r200UpdateViewportOffset;
   radeon->vtbl.emit_cs_header = r200_vtbl_emit_cs_header;
   radeon->vtbl.swtcl_flush = r200_swtcl_flush;
   radeon->vtbl.fallback = r200Fallback;
   radeon->vtbl.update_scissor = r200_vtbl_update_scissor;
   radeon->vtbl.emit_query_finish = r200_emit_query_finish;
   radeon->vtbl.check_blit = r200_check_blit;
   radeon->vtbl.blit = r200_blit;
   radeon->vtbl.is_format_renderable = radeonIsFormatRenderable;
}


/* Create the device specific rendering context.
 */
GLboolean r200CreateContext( gl_api api,
			     const struct gl_config *glVisual,
			     __DRIcontext *driContextPriv,
			     unsigned major_version,
			     unsigned minor_version,
			     uint32_t flags,
			     unsigned *error,
			     void *sharedContextPrivate)
{
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   radeonScreenPtr screen = (radeonScreenPtr)(sPriv->driverPrivate);
   struct dd_function_table functions;
   r200ContextPtr rmesa;
   struct gl_context *ctx;
   int i;
   int tcl_mode;

   switch (api) {
   case API_OPENGL:
      if (major_version > 1 || minor_version > 3) {
         *error = __DRI_CTX_ERROR_BAD_VERSION;
         return GL_FALSE;
      }
      break;
   case API_OPENGLES:
      break;
   default:
      *error = __DRI_CTX_ERROR_BAD_API;
      return GL_FALSE;
   }

   /* Flag filtering is handled in dri2CreateContextAttribs.
    */
   (void) flags;

   assert(glVisual);
   assert(driContextPriv);
   assert(screen);

   /* Allocate the R200 context */
   rmesa = (r200ContextPtr) CALLOC( sizeof(*rmesa) );
   if ( !rmesa ) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      return GL_FALSE;
   }

   rmesa->radeon.radeonScreen = screen;
   r200_init_vtbl(&rmesa->radeon);
   /* init exp fog table data */
   radeonInitStaticFogData();

   /* Parse configuration files.
    * Do this here so that initialMaxAnisotropy is set before we create
    * the default textures.
    */
   driParseConfigFiles (&rmesa->radeon.optionCache, &screen->optionCache,
			screen->driScreen->myNum, "r200");
   rmesa->radeon.initialMaxAnisotropy = driQueryOptionf(&rmesa->radeon.optionCache,
							"def_max_anisotropy");

   if ( sPriv->drm_version.major == 1
       && driQueryOptionb( &rmesa->radeon.optionCache, "hyperz" ) ) {
      if ( sPriv->drm_version.minor < 13 )
	 fprintf( stderr, "DRM version 1.%d too old to support HyperZ, "
			  "disabling.\n", sPriv->drm_version.minor );
      else
	 rmesa->using_hyperz = GL_TRUE;
   }
 
   if ( sPriv->drm_version.minor >= 15 )
      rmesa->texmicrotile = GL_TRUE;

   /* Init default driver functions then plug in our R200-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions(&functions);
   r200InitDriverFuncs(&functions);
   r200InitIoctlFuncs(&functions);
   r200InitStateFuncs(&rmesa->radeon, &functions);
   r200InitTextureFuncs(&rmesa->radeon, &functions);
   r200InitShaderFuncs(&functions);
   radeonInitQueryObjFunctions(&functions);

   if (!radeonInitContext(&rmesa->radeon, &functions,
			  glVisual, driContextPriv,
			  sharedContextPrivate)) {
     FREE(rmesa);
     *error = __DRI_CTX_ERROR_NO_MEMORY;
     return GL_FALSE;
   }

   rmesa->radeon.swtcl.RenderIndex = ~0;
   rmesa->radeon.hw.all_dirty = 1;

   ctx = rmesa->radeon.glCtx;
   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _ae_create_context( ctx );

   /* Set the maximum texture size small enough that we can guarentee that
    * all texture units can bind a maximal texture and have all of them in
    * texturable memory at once. Depending on the allow_large_textures driconf
    * setting allow larger textures.
    */
   ctx->Const.MaxTextureUnits = driQueryOptioni (&rmesa->radeon.optionCache,
						 "texture_units");
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

   ctx->Const.MaxCombinedTextureImageUnits = ctx->Const.MaxTextureUnits;

   ctx->Const.StripTextureBorder = GL_TRUE;

   i = driQueryOptioni( &rmesa->radeon.optionCache, "allow_large_textures");

   /* FIXME: When no memory manager is available we should set this 
    * to some reasonable value based on texture memory pool size */
   ctx->Const.MaxTextureLevels = 12;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 12;
   ctx->Const.MaxTextureRectSize = 2048;
   ctx->Const.MaxRenderbufferSize = 2048;

   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* No wide AA points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;
   ctx->Const.PointSizeGranularity = 0.0625;
   ctx->Const.MaxPointSize = 2047.0;

   /* mesa initialization problem - _mesa_init_point was already called */
   ctx->Point.MaxSize = ctx->Const.MaxPointSize;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 0.0625;

   ctx->Const.VertexProgram.MaxNativeInstructions = R200_VSF_MAX_INST;
   ctx->Const.VertexProgram.MaxNativeAttribs = 12;
   ctx->Const.VertexProgram.MaxNativeTemps = R200_VSF_MAX_TEMPS;
   ctx->Const.VertexProgram.MaxNativeParameters = R200_VSF_MAX_PARAM;
   ctx->Const.VertexProgram.MaxNativeAddressRegs = 1;

   ctx->Const.MaxDrawBuffers = 1;
   ctx->Const.MaxColorAttachments = 1;

   _mesa_set_mvp_with_dp4( ctx, GL_TRUE );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, r200_pipeline );

   /* Try and keep materials and vertices separate:
    */
/*    _tnl_isolate_materials( ctx, GL_TRUE ); */


   /* Configure swrast and TNL to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );


   for ( i = 0 ; i < R200_MAX_TEXTURE_UNITS ; i++ ) {
      _math_matrix_ctr( &rmesa->TexGenMatrix[i] );
      _math_matrix_set_identity( &rmesa->TexGenMatrix[i] );
   }
   _math_matrix_ctr( &rmesa->tmpmat );
   _math_matrix_set_identity( &rmesa->tmpmat );

   ctx->Extensions.ARB_half_float_pixel = true;
   ctx->Extensions.ARB_occlusion_query = true;
   ctx->Extensions.ARB_texture_border_clamp = true;
   ctx->Extensions.ARB_texture_env_combine = true;
   ctx->Extensions.ARB_texture_env_dot3 = true;
   ctx->Extensions.ARB_texture_env_crossbar = true;
   ctx->Extensions.EXT_blend_color = true;
   ctx->Extensions.EXT_blend_minmax = true;
   ctx->Extensions.EXT_fog_coord = true;
   ctx->Extensions.EXT_packed_depth_stencil = true;
   ctx->Extensions.EXT_secondary_color = true;
   ctx->Extensions.EXT_texture_env_dot3 = true;
   ctx->Extensions.EXT_texture_filter_anisotropic = true;
   ctx->Extensions.EXT_texture_mirror_clamp = true;
   ctx->Extensions.ATI_texture_env_combine3 = true;
   ctx->Extensions.ATI_texture_mirror_once = true;
   ctx->Extensions.MESA_pack_invert = true;
   ctx->Extensions.NV_blend_square = true;
   ctx->Extensions.NV_texture_rectangle = true;
#if FEATURE_OES_EGL_image
   ctx->Extensions.OES_EGL_image = true;
#endif

   ctx->Extensions.EXT_framebuffer_object = true;
   ctx->Extensions.ARB_occlusion_query = true;

   if (!(rmesa->radeon.radeonScreen->chip_flags & R200_CHIPSET_YCBCR_BROKEN)) {
     /* yuv textures don't work with some chips - R200 / rv280 okay so far
	others get the bit ordering right but don't actually do YUV-RGB conversion */
      ctx->Extensions.MESA_ycbcr_texture = true;
   }
   if (rmesa->radeon.glCtx->Mesa_DXTn) {
      ctx->Extensions.EXT_texture_compression_s3tc = true;
      ctx->Extensions.S3_s3tc = true;
   }
   else if (driQueryOptionb (&rmesa->radeon.optionCache, "force_s3tc_enable")) {
      ctx->Extensions.EXT_texture_compression_s3tc = true;
   }

   ctx->Extensions.ARB_texture_cube_map = true;

   ctx->Extensions.EXT_blend_equation_separate = true;
   ctx->Extensions.EXT_blend_func_separate = true;

   ctx->Extensions.ARB_vertex_program = true;
   ctx->Extensions.EXT_gpu_program_parameters = true;

   ctx->Extensions.NV_vertex_program =
      driQueryOptionb(&rmesa->radeon.optionCache, "nv_vertex_program");

   ctx->Extensions.ATI_fragment_shader = (ctx->Const.MaxTextureUnits == 6);

   ctx->Extensions.ARB_point_sprite = true;
   ctx->Extensions.EXT_point_parameters = true;

#if 0
   r200InitDriverFuncs( ctx );
   r200InitIoctlFuncs( ctx );
   r200InitStateFuncs( ctx );
   r200InitTextureFuncs( ctx );
#endif
   /* plug in a few more device driver functions */
   /* XXX these should really go right after _mesa_init_driver_functions() */
   radeon_fbo_init(&rmesa->radeon);
   radeonInitSpanFuncs( ctx );
   r200InitTnlFuncs( ctx );
   r200InitState( rmesa );
   r200InitSwtcl( ctx );

   rmesa->prefer_gart_client_texturing = 
      (getenv("R200_GART_CLIENT_TEXTURES") != 0);

   tcl_mode = driQueryOptioni(&rmesa->radeon.optionCache, "tcl_mode");
   if (driQueryOptionb(&rmesa->radeon.optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(rmesa, R200_FALLBACK_DISABLE, 1);
   }
   else if (tcl_mode == DRI_CONF_TCL_SW || getenv("R200_NO_TCL") ||
	    !(rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
      if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	 rmesa->radeon.radeonScreen->chip_flags &= ~RADEON_CHIPSET_TCL;
	 fprintf(stderr, "Disabling HW TCL support\n");
      }
      TCL_FALLBACK(rmesa->radeon.glCtx, R200_TCL_FALLBACK_TCL_DISABLE, 1);
   }

   _mesa_compute_version(ctx);

   *error = __DRI_CTX_ERROR_SUCCESS;
   return GL_TRUE;
}


void r200DestroyContext( __DRIcontext *driContextPriv )
{
	int i;
	r200ContextPtr rmesa = (r200ContextPtr)driContextPriv->driverPrivate;
	if (rmesa)
	{
		for ( i = 0 ; i < R200_MAX_TEXTURE_UNITS ; i++ ) {
			_math_matrix_dtr( &rmesa->TexGenMatrix[i] );
		}
	}
	radeonDestroyContext(driContextPriv);
}
