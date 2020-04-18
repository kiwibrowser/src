/*
 * (C) Copyright IBM Corporation 2002, 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glxextensions.c
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "glxclient.h"
#include <X11/extensions/extutil.h>
#include <X11/extensions/Xext.h>
#include <string.h>
#include "glxextensions.h"


#define SET_BIT(m,b)   (m[ (b) / 8 ] |=  (1U << ((b) % 8)))
#define CLR_BIT(m,b)   (m[ (b) / 8 ] &= ~(1U << ((b) % 8)))
#define IS_SET(m,b)    ((m[ (b) / 8 ] & (1U << ((b) % 8))) != 0)
#define CONCAT(a,b) a ## b
#define GLX(n) "GLX_" # n, 4 + sizeof( # n ) - 1, CONCAT(n,_bit)
#define GL(n)  "GL_" # n,  3 + sizeof( # n ) - 1, GL_ ## n ## _bit
#define VER(a,b)  a, b
#define Y  1
#define N  0
#define EXT_ENABLED(bit,supported) (IS_SET( supported, bit ))


struct extension_info
{
   const char *const name;
   unsigned name_len;

   unsigned char bit;

   /* This is the lowest version of GLX that "requires" this extension.
    * For example, GLX 1.3 requires SGIX_fbconfig, SGIX_pbuffer, and
    * SGI_make_current_read.  If the extension is not required by any known
    * version of GLX, use 0, 0.
    */
   unsigned char version_major;
   unsigned char version_minor;
   unsigned char client_support;
   unsigned char direct_support;
   unsigned char client_only;        /** Is the extension client-side only? */
   unsigned char direct_only;        /** Is the extension for direct
				      * contexts only?
				      */
};

/* *INDENT-OFF* */
static const struct extension_info known_glx_extensions[] = {
   { GLX(ARB_create_context),          VER(0,0), Y, N, N, N },
   { GLX(ARB_create_context_profile),  VER(0,0), Y, N, N, N },
   { GLX(ARB_create_context_robustness), VER(0,0), Y, N, N, N },
   { GLX(ARB_get_proc_address),        VER(1,4), Y, N, Y, N },
   { GLX(ARB_multisample),             VER(1,4), Y, Y, N, N },
   { GLX(ATI_pixel_format_float),      VER(0,0), N, N, N, N },
   { GLX(EXT_import_context),          VER(0,0), Y, Y, N, N },
   { GLX(EXT_visual_info),             VER(0,0), Y, Y, N, N },
   { GLX(EXT_visual_rating),           VER(0,0), Y, Y, N, N },
   { GLX(EXT_framebuffer_sRGB),        VER(0,0), Y, Y, N, N },
   { GLX(EXT_create_context_es2_profile), VER(0,0), Y, N, N, Y },
   { GLX(MESA_copy_sub_buffer),        VER(0,0), Y, N, N, N },
   { GLX(MESA_multithread_makecurrent),VER(0,0), Y, N, Y, N },
   { GLX(MESA_swap_control),           VER(0,0), Y, N, N, Y },
   { GLX(NV_float_buffer),             VER(0,0), N, N, N, N },
   { GLX(OML_swap_method),             VER(0,0), Y, Y, N, N },
   { GLX(OML_sync_control),            VER(0,0), Y, N, N, Y },
   { GLX(SGI_make_current_read),       VER(1,3), Y, N, N, N },
   { GLX(SGI_swap_control),            VER(0,0), Y, N, N, N },
   { GLX(SGI_video_sync),              VER(0,0), Y, N, N, Y },
   { GLX(SGIS_multisample),            VER(0,0), Y, Y, N, N },
   { GLX(SGIX_fbconfig),               VER(1,3), Y, Y, N, N },
   { GLX(SGIX_pbuffer),                VER(1,3), Y, Y, N, N },
   { GLX(SGIX_swap_barrier),           VER(0,0), N, N, N, N },
   { GLX(SGIX_swap_group),             VER(0,0), N, N, N, N },
   { GLX(SGIX_visual_select_group),    VER(0,0), Y, Y, N, N },
   { GLX(EXT_texture_from_pixmap),     VER(0,0), Y, N, N, N },
   { GLX(INTEL_swap_event),            VER(0,0), Y, N, N, N },
   { NULL }
};

static const struct extension_info known_gl_extensions[] = {
   { GL(ARB_depth_texture),              VER(1,4), Y, N, N, N },
   { GL(ARB_draw_buffers),               VER(0,0), Y, N, N, N },
   { GL(ARB_fragment_program),           VER(0,0), Y, N, N, N },
   { GL(ARB_fragment_program_shadow),    VER(0,0), Y, N, N, N },
   { GL(ARB_framebuffer_object),         VER(0,0), Y, N, N, N },
   { GL(ARB_imaging),                    VER(0,0), Y, N, N, N },
   { GL(ARB_multisample),                VER(1,3), Y, N, N, N },
   { GL(ARB_multitexture),               VER(1,3), Y, N, N, N },
   { GL(ARB_occlusion_query),            VER(1,5), Y, N, N, N },
   { GL(ARB_point_parameters),           VER(1,4), Y, N, N, N },
   { GL(ARB_point_sprite),               VER(0,0), Y, N, N, N },
   { GL(ARB_shadow),                     VER(1,4), Y, N, N, N },
   { GL(ARB_shadow_ambient),             VER(0,0), Y, N, N, N },
   { GL(ARB_texture_border_clamp),       VER(1,3), Y, N, N, N },
   { GL(ARB_texture_compression),        VER(1,3), Y, N, N, N },
   { GL(ARB_texture_cube_map),           VER(1,3), Y, N, N, N },
   { GL(ARB_texture_env_add),            VER(1,3), Y, N, N, N },
   { GL(ARB_texture_env_combine),        VER(1,3), Y, N, N, N },
   { GL(ARB_texture_env_crossbar),       VER(1,4), Y, N, N, N },
   { GL(ARB_texture_env_dot3),           VER(1,3), Y, N, N, N },
   { GL(ARB_texture_mirrored_repeat),    VER(1,4), Y, N, N, N },
   { GL(ARB_texture_non_power_of_two),   VER(1,5), Y, N, N, N },
   { GL(ARB_texture_rectangle),          VER(0,0), Y, N, N, N },
   { GL(ARB_texture_rg),                 VER(0,0), Y, N, N, N },
   { GL(ARB_transpose_matrix),           VER(1,3), Y, N, Y, N },
   { GL(ARB_vertex_buffer_object),       VER(1,5), N, N, N, N },
   { GL(ARB_vertex_program),             VER(0,0), Y, N, N, N },
   { GL(ARB_window_pos),                 VER(1,4), Y, N, N, N },
   { GL(EXT_abgr),                       VER(0,0), Y, N, N, N },
   { GL(EXT_bgra),                       VER(1,2), Y, N, N, N },
   { GL(EXT_blend_color),                VER(1,4), Y, N, N, N },
   { GL(EXT_blend_equation_separate),    VER(0,0), Y, N, N, N },
   { GL(EXT_blend_func_separate),        VER(1,4), Y, N, N, N },
   { GL(EXT_blend_logic_op),             VER(1,4), Y, N, N, N },
   { GL(EXT_blend_minmax),               VER(1,4), Y, N, N, N },
   { GL(EXT_blend_subtract),             VER(1,4), Y, N, N, N },
   { GL(EXT_clip_volume_hint),           VER(0,0), Y, N, N, N },
   { GL(EXT_compiled_vertex_array),      VER(0,0), N, N, N, N },
   { GL(EXT_convolution),                VER(0,0), N, N, N, N },
   { GL(EXT_copy_texture),               VER(1,1), Y, N, N, N },
   { GL(EXT_cull_vertex),                VER(0,0), N, N, N, N },
   { GL(EXT_depth_bounds_test),          VER(0,0), N, N, N, N },
   { GL(EXT_draw_range_elements),        VER(1,2), Y, N, Y, N },
   { GL(EXT_fog_coord),                  VER(1,4), Y, N, N, N },
   { GL(EXT_framebuffer_blit),           VER(0,0), Y, N, N, N },
   { GL(EXT_framebuffer_multisample),    VER(0,0), Y, N, N, N },
   { GL(EXT_framebuffer_object),         VER(0,0), Y, N, N, N },
   { GL(EXT_framebuffer_sRGB),           VER(0,0), Y, N, N, N },
   { GL(EXT_multi_draw_arrays),          VER(1,4), Y, N, Y, N },
   { GL(EXT_packed_depth_stencil),       VER(0,0), Y, N, N, N },
   { GL(EXT_packed_pixels),              VER(1,2), Y, N, N, N },
   { GL(EXT_paletted_texture),           VER(0,0), Y, N, N, N },
   { GL(EXT_pixel_buffer_object),        VER(0,0), N, N, N, N },
   { GL(EXT_point_parameters),           VER(1,4), Y, N, N, N },
   { GL(EXT_polygon_offset),             VER(1,1), Y, N, N, N },
   { GL(EXT_rescale_normal),             VER(1,2), Y, N, N, N },
   { GL(EXT_secondary_color),            VER(1,4), Y, N, N, N },
   { GL(EXT_separate_specular_color),    VER(1,2), Y, N, N, N },
   { GL(EXT_shadow_funcs),               VER(1,5), Y, N, N, N },
   { GL(EXT_shared_texture_palette),     VER(0,0), Y, N, N, N },
   { GL(EXT_stencil_two_side),           VER(0,0), Y, N, N, N },
   { GL(EXT_stencil_wrap),               VER(1,4), Y, N, N, N },
   { GL(EXT_subtexture),                 VER(1,1), Y, N, N, N },
   { GL(EXT_texture),                    VER(1,1), Y, N, N, N },
   { GL(EXT_texture3D),                  VER(1,2), Y, N, N, N },
   { GL(EXT_texture_compression_dxt1),   VER(0,0), Y, N, N, N },
   { GL(EXT_texture_compression_s3tc),   VER(0,0), Y, N, N, N },
   { GL(EXT_texture_edge_clamp),         VER(1,2), Y, N, N, N },
   { GL(EXT_texture_env_add),            VER(1,3), Y, N, N, N },
   { GL(EXT_texture_env_combine),        VER(1,3), Y, N, N, N },
   { GL(EXT_texture_env_dot3),           VER(0,0), Y, N, N, N },
   { GL(EXT_texture_filter_anisotropic), VER(0,0), Y, N, N, N },
   { GL(EXT_texture_lod),                VER(1,2), Y, N, N, N },
   { GL(EXT_texture_lod_bias),           VER(1,4), Y, N, N, N },
   { GL(EXT_texture_mirror_clamp),       VER(0,0), Y, N, N, N },
   { GL(EXT_texture_object),             VER(1,1), Y, N, N, N },
   { GL(EXT_texture_rectangle),          VER(0,0), Y, N, N, N },
   { GL(EXT_vertex_array),               VER(0,0), Y, N, N, N },
   { GL(3DFX_texture_compression_FXT1),  VER(0,0), Y, N, N, N },
   { GL(APPLE_packed_pixels),            VER(1,2), Y, N, N, N },
   { GL(APPLE_ycbcr_422),                VER(0,0), Y, N, N, N },
   { GL(ATI_draw_buffers),               VER(0,0), Y, N, N, N },
   { GL(ATI_text_fragment_shader),       VER(0,0), Y, N, N, N },
   { GL(ATI_texture_env_combine3),       VER(0,0), Y, N, N, N },
   { GL(ATI_texture_float),              VER(0,0), Y, N, N, N },
   { GL(ATI_texture_mirror_once),        VER(0,0), Y, N, N, N },
   { GL(ATIX_texture_env_combine3),      VER(0,0), Y, N, N, N },
   { GL(HP_convolution_border_modes),    VER(0,0), Y, N, N, N },
   { GL(HP_occlusion_test),              VER(0,0), Y, N, N, N },
   { GL(IBM_cull_vertex),                VER(0,0), Y, N, N, N },
   { GL(IBM_pixel_filter_hint),          VER(0,0), Y, N, N, N },
   { GL(IBM_rasterpos_clip),             VER(0,0), Y, N, N, N },
   { GL(IBM_texture_clamp_nodraw),       VER(0,0), Y, N, N, N },
   { GL(IBM_texture_mirrored_repeat),    VER(0,0), Y, N, N, N },
   { GL(INGR_blend_func_separate),       VER(0,0), Y, N, N, N },
   { GL(INGR_interlace_read),            VER(0,0), Y, N, N, N },
   { GL(MESA_pack_invert),               VER(0,0), Y, N, N, N },
   { GL(MESA_ycbcr_texture),             VER(0,0), Y, N, N, N },
   { GL(NV_blend_square),                VER(1,4), Y, N, N, N },
   { GL(NV_copy_depth_to_color),         VER(0,0), Y, N, N, N },
   { GL(NV_depth_clamp),                 VER(0,0), Y, N, N, N },
   { GL(NV_fog_distance),                VER(0,0), Y, N, N, N },
   { GL(NV_fragment_program),            VER(0,0), Y, N, N, N },
   { GL(NV_fragment_program_option),     VER(0,0), Y, N, N, N },
   { GL(NV_fragment_program2),           VER(0,0), Y, N, N, N },
   { GL(NV_light_max_exponent),          VER(0,0), Y, N, N, N },
   { GL(NV_multisample_filter_hint),     VER(0,0), Y, N, N, N },
   { GL(NV_packed_depth_stencil),        VER(0,0), Y, N, N, N },
   { GL(NV_point_sprite),                VER(0,0), Y, N, N, N },
   { GL(NV_texgen_reflection),           VER(0,0), Y, N, N, N },
   { GL(NV_texture_compression_vtc),     VER(0,0), Y, N, N, N },
   { GL(NV_texture_env_combine4),        VER(0,0), Y, N, N, N },
   { GL(NV_texture_rectangle),           VER(0,0), Y, N, N, N },
   { GL(NV_vertex_program),              VER(0,0), Y, N, N, N },
   { GL(NV_vertex_program1_1),           VER(0,0), Y, N, N, N },
   { GL(NV_vertex_program2),             VER(0,0), Y, N, N, N },
   { GL(NV_vertex_program2_option),      VER(0,0), Y, N, N, N },
   { GL(NV_vertex_program3),             VER(0,0), Y, N, N, N },
   { GL(OES_read_format),                VER(0,0), Y, N, N, N },
   { GL(OES_compressed_paletted_texture),VER(0,0), Y, N, N, N },
   { GL(SGI_color_matrix),               VER(0,0), Y, N, N, N },
   { GL(SGI_color_table),                VER(0,0), Y, N, N, N },
   { GL(SGI_texture_color_table),        VER(0,0), Y, N, N, N },
   { GL(SGIS_generate_mipmap),           VER(1,4), Y, N, N, N },
   { GL(SGIS_multisample),               VER(0,0), Y, N, N, N },
   { GL(SGIS_texture_border_clamp),      VER(1,3), Y, N, N, N },
   { GL(SGIS_texture_edge_clamp),        VER(1,2), Y, N, N, N },
   { GL(SGIS_texture_lod),               VER(1,2), Y, N, N, N },
   { GL(SGIX_blend_alpha_minmax),        VER(0,0), Y, N, N, N },
   { GL(SGIX_clipmap),                   VER(0,0), Y, N, N, N },
   { GL(SGIX_depth_texture),             VER(0,0), Y, N, N, N },
   { GL(SGIX_fog_offset),                VER(0,0), Y, N, N, N },
   { GL(SGIX_shadow),                    VER(0,0), Y, N, N, N },
   { GL(SGIX_shadow_ambient),            VER(0,0), Y, N, N, N },
   { GL(SGIX_texture_coordinate_clamp),  VER(0,0), Y, N, N, N },
   { GL(SGIX_texture_lod_bias),          VER(0,0), Y, N, N, N },
   { GL(SGIX_texture_range),             VER(0,0), Y, N, N, N },
   { GL(SGIX_texture_scale_bias),        VER(0,0), Y, N, N, N },
   { GL(SGIX_vertex_preclip),            VER(0,0), Y, N, N, N },
   { GL(SGIX_vertex_preclip_hint),       VER(0,0), Y, N, N, N },
   { GL(SGIX_ycrcb),                     VER(0,0), Y, N, N, N },
   { GL(SUN_convolution_border_modes),   VER(0,0), Y, N, N, N },
   { GL(SUN_multi_draw_arrays),          VER(0,0), Y, N, Y, N },
   { GL(SUN_slice_accum),                VER(0,0), Y, N, N, N },
   { NULL }
};
/* *INDENT-ON* */


/* global bit-fields of available extensions and their characteristics */
static unsigned char client_glx_support[8];
static unsigned char client_glx_only[8];
static unsigned char direct_glx_only[8];
static unsigned char client_gl_support[__GL_EXT_BYTES];
static unsigned char client_gl_only[__GL_EXT_BYTES];

/**
 * Bits representing the set of extensions that are enabled by default in all
 * direct rendering drivers.
 */
static unsigned char direct_glx_support[8];

/**
 * Highest core GL version that can be supported for indirect rendering.
 */
static const unsigned gl_major = 1;
static const unsigned gl_minor = 4;

/* client extensions string */
static const char *__glXGLXClientExtensions = NULL;

static void __glXExtensionsCtr(void);
static void __glXExtensionsCtrScreen(struct glx_screen * psc);
static void __glXProcessServerString(const struct extension_info *ext,
                                     const char *server_string,
                                     unsigned char *server_support);

/**
 * Set the state of a GLX extension.
 *
 * \param name      Name of the extension.
 * \param name_len  Length, in characters, of the extension name.
 * \param state     New state (either enabled or disabled) of the extension.
 * \param supported Table in which the state of the extension is to be set.
 */
static void
set_glx_extension(const struct extension_info *ext,
                  const char *name, unsigned name_len, GLboolean state,
                  unsigned char *supported)
{
   unsigned i;


   for (i = 0; ext[i].name != NULL; i++) {
      if ((name_len == ext[i].name_len)
          && (strncmp(ext[i].name, name, name_len) == 0)) {
         if (state) {
            SET_BIT(supported, ext[i].bit);
         }
         else {
            CLR_BIT(supported, ext[i].bit);
         }

         return;
      }
   }
}


#define NUL '\0'
#define SEPARATOR ' '

/**
 * Convert the server's extension string to a bit-field.
 *
 * \param server_string   GLX extension string from the server.
 * \param server_support  Bit-field of supported extensions.
 *
 * \note
 * This function is used to process both GLX and GL extension strings.  The
 * bit-fields used to track each of these have different sizes.  Therefore,
 * the data pointed by \c server_support must be preinitialized to zero.
 */
static void
__glXProcessServerString(const struct extension_info *ext,
                         const char *server_string,
                         unsigned char *server_support)
{
   unsigned base;
   unsigned len;

   for (base = 0; server_string[base] != NUL; /* empty */ ) {
      /* Determine the length of the next extension name.
       */
      for (len = 0; (server_string[base + len] != SEPARATOR)
           && (server_string[base + len] != NUL); len++) {
         /* empty */
      }

      /* Set the bit for the extension in the server_support table.
       */
      set_glx_extension(ext, &server_string[base], len, GL_TRUE,
                        server_support);


      /* Advance to the next extension string.  This means that we skip
       * over the previous string and any trialing white-space.
       */
      for (base += len; (server_string[base] == SEPARATOR)
           && (server_string[base] != NUL); base++) {
         /* empty */
      }
   }
}

void
__glXEnableDirectExtension(struct glx_screen * psc, const char *name)
{
   __glXExtensionsCtr();
   __glXExtensionsCtrScreen(psc);

   set_glx_extension(known_glx_extensions,
                     name, strlen(name), GL_TRUE, psc->direct_support);
}

/**
 * Initialize global extension support tables.
 */

static void
__glXExtensionsCtr(void)
{
   unsigned i;
   static GLboolean ext_list_first_time = GL_TRUE;


   if (ext_list_first_time) {
      ext_list_first_time = GL_FALSE;

      (void) memset(client_glx_support, 0, sizeof(client_glx_support));
      (void) memset(direct_glx_support, 0, sizeof(direct_glx_support));
      (void) memset(client_glx_only, 0, sizeof(client_glx_only));
      (void) memset(direct_glx_only, 0, sizeof(direct_glx_only));

      (void) memset(client_gl_support, 0, sizeof(client_gl_support));
      (void) memset(client_gl_only, 0, sizeof(client_gl_only));

      for (i = 0; known_glx_extensions[i].name != NULL; i++) {
         const unsigned bit = known_glx_extensions[i].bit;

         if (known_glx_extensions[i].client_support) {
            SET_BIT(client_glx_support, bit);
         }

         if (known_glx_extensions[i].direct_support) {
            SET_BIT(direct_glx_support, bit);
         }

         if (known_glx_extensions[i].client_only) {
            SET_BIT(client_glx_only, bit);
         }

         if (known_glx_extensions[i].direct_only) {
            SET_BIT(direct_glx_only, bit);
         }
      }

      for (i = 0; known_gl_extensions[i].name != NULL; i++) {
         const unsigned bit = known_gl_extensions[i].bit;

         if (known_gl_extensions[i].client_support) {
            SET_BIT(client_gl_support, bit);
         }

         if (known_gl_extensions[i].client_only) {
            SET_BIT(client_gl_only, bit);
         }
      }

#if 0
      fprintf(stderr, "[%s:%u] Maximum client library version: %u.%u\n",
              __func__, __LINE__, gl_major, gl_minor);
#endif
   }
}


/**
 * Make sure that per-screen direct-support table is initialized.
 *
 * \param psc  Pointer to GLX per-screen record.
 */

static void
__glXExtensionsCtrScreen(struct glx_screen * psc)
{
   if (psc->ext_list_first_time) {
      psc->ext_list_first_time = GL_FALSE;
      (void) memcpy(psc->direct_support, direct_glx_support,
                    sizeof(direct_glx_support));
   }
}


/**
 * Check if a certain extension is enabled on a given screen.
 *
 * \param psc  Pointer to GLX per-screen record.
 * \param bit  Bit index in the direct-support table.
 * \returns If the extension bit is enabled for the screen, \c GL_TRUE is
 *          returned.  If the extension bit is not enabled or if \c psc is
 *          \c NULL, then \c GL_FALSE is returned.
 */
GLboolean
__glXExtensionBitIsEnabled(struct glx_screen * psc, unsigned bit)
{
   GLboolean enabled = GL_FALSE;

   if (psc != NULL) {
      __glXExtensionsCtr();
      __glXExtensionsCtrScreen(psc);
      enabled = EXT_ENABLED(bit, psc->direct_support);
   }

   return enabled;
}


/**
 * Check if a certain extension is enabled in a given context.
 *
 */
GLboolean
__glExtensionBitIsEnabled(struct glx_context *gc, unsigned bit)
{
   GLboolean enabled = GL_FALSE;

   if (gc != NULL) {
      enabled = EXT_ENABLED(bit, gc->gl_extension_bits);
   }

   return enabled;
}



/**
 * Convert a bit-field to a string of supported extensions.
 */
static char *
__glXGetStringFromTable(const struct extension_info *ext,
                        const unsigned char *supported)
{
   unsigned i;
   unsigned ext_str_len;
   char *ext_str;
   char *point;


   ext_str_len = 0;
   for (i = 0; ext[i].name != NULL; i++) {
      if (EXT_ENABLED(ext[i].bit, supported)) {
         ext_str_len += ext[i].name_len + 1;
      }
   }

   ext_str = Xmalloc(ext_str_len + 1);
   if (ext_str != NULL) {
      point = ext_str;

      for (i = 0; ext[i].name != NULL; i++) {
         if (EXT_ENABLED(ext[i].bit, supported)) {
            (void) memcpy(point, ext[i].name, ext[i].name_len);
            point += ext[i].name_len;

            *point = ' ';
            point++;
         }
      }

      *point = '\0';
   }

   return ext_str;
}


/**
 * Get the string of client library supported extensions.
 */
const char *
__glXGetClientExtensions(void)
{
   if (__glXGLXClientExtensions == NULL) {
      __glXExtensionsCtr();
      __glXGLXClientExtensions = __glXGetStringFromTable(known_glx_extensions,
                                                         client_glx_support);
   }

   return __glXGLXClientExtensions;
}


/**
 * Calculate the list of application usable extensions.  The resulting
 * string is stored in \c psc->effectiveGLXexts.
 *
 * \param psc                        Pointer to GLX per-screen record.
 * \param display_is_direct_capable  True if the display is capable of
 *                                   direct rendering.
 * \param minor_version              GLX minor version from the server.
 */

void
__glXCalculateUsableExtensions(struct glx_screen * psc,
                               GLboolean display_is_direct_capable,
                               int minor_version)
{
   unsigned char server_support[8];
   unsigned char usable[8];
   unsigned i;

   __glXExtensionsCtr();
   __glXExtensionsCtrScreen(psc);

   (void) memset(server_support, 0, sizeof(server_support));
   __glXProcessServerString(known_glx_extensions,
                            psc->serverGLXexts, server_support);


   /* This is a hack.  Some servers support GLX 1.3 but don't export
    * all of the extensions implied by GLX 1.3.  If the server claims
    * support for GLX 1.3, enable support for the extensions that can be
    * "emulated" as well.
    */
#ifndef GLX_USE_APPLEGL
   if (minor_version >= 3) {
      SET_BIT(server_support, EXT_visual_info_bit);
      SET_BIT(server_support, EXT_visual_rating_bit);
      SET_BIT(server_support, SGI_make_current_read_bit);
      SET_BIT(server_support, SGIX_fbconfig_bit);
      SET_BIT(server_support, SGIX_pbuffer_bit);

      /* This one is a little iffy.  GLX 1.3 doesn't incorporate all of this
       * extension.  However, the only part that is not strictly client-side
       * is shared.  That's the glXQueryContext / glXQueryContextInfoEXT
       * function.
       */

      SET_BIT(server_support, EXT_import_context_bit);
   }
#endif

   /* An extension is supported if the client-side (i.e., libGL) supports
    * it and the "server" supports it.  In this case that means that either
    * the true server supports it or it is only for direct-rendering and
    * the direct rendering driver supports it.
    *
    * If the display is not capable of direct rendering, then the extension
    * is enabled if and only if the client-side library and the server
    * support it.
    */

   if (display_is_direct_capable) {
      for (i = 0; i < 8; i++) {
         usable[i] = (client_glx_support[i] & client_glx_only[i])
            | (client_glx_support[i] & psc->direct_support[i] &
               server_support[i])
            | (client_glx_support[i] & psc->direct_support[i] &
               direct_glx_only[i]);
      }
   }
   else {
      for (i = 0; i < 8; i++) {
         usable[i] = (client_glx_support[i] & client_glx_only[i])
            | (client_glx_support[i] & server_support[i]);
      }
   }

   /* This hack is necessary because GLX_ARB_create_context_profile depends on
    * server support, but GLX_EXT_create_context_es2_profile is direct-only.
    * Without this hack, it would be possible to advertise
    * GLX_EXT_create_context_es2_profile without
    * GLX_ARB_create_context_profile.  That would be a problem.
    */
   if (!IS_SET(server_support, ARB_create_context_profile_bit)) {
      CLR_BIT(usable, EXT_create_context_es2_profile_bit);
   }

   psc->effectiveGLXexts = __glXGetStringFromTable(known_glx_extensions,
                                                   usable);
}


/**
 * Calculate the list of application usable extensions.  The resulting
 * string is stored in \c gc->extensions.
 *
 * \param gc             Pointer to GLX context.
 * \param server_string  Extension string from the server.
 * \param major_version  GL major version from the server.
 * \param minor_version  GL minor version from the server.
 */

void
__glXCalculateUsableGLExtensions(struct glx_context * gc,
                                 const char *server_string,
                                 int major_version, int minor_version)
{
   unsigned char server_support[__GL_EXT_BYTES];
   unsigned char usable[__GL_EXT_BYTES];
   unsigned i;


   __glXExtensionsCtr();

   (void) memset(server_support, 0, sizeof(server_support));
   __glXProcessServerString(known_gl_extensions, server_string,
                            server_support);


   /* Handle lazy servers that don't export all the extensions strings that
    * are part of the GL core version that they support.
    */

   for (i = 0; i < __GL_EXT_BYTES; i++) {
      if ((known_gl_extensions[i].version_major != 0)
          && ((major_version > known_gl_extensions[i].version_major)
              || ((major_version == known_gl_extensions[i].version_major)
                  && (minor_version >=
                      known_gl_extensions[i].version_minor)))) {
         SET_BIT(server_support, known_gl_extensions[i].bit);
      }
   }


   /* An extension is supported if the client-side (i.e., libGL) supports
    * it and the server supports it or the client-side library supports it
    * and it only needs client-side support.
    */

   for (i = 0; i < __GL_EXT_BYTES; i++) {
      usable[i] = (client_gl_support[i] & client_gl_only[i])
         | (client_gl_support[i] & server_support[i]);
   }

   gc->extensions = (unsigned char *)
      __glXGetStringFromTable(known_gl_extensions, usable);
   (void) memcpy(gc->gl_extension_bits, usable, sizeof(usable));
}


/**
 * Calculates the maximum core GL version that can be supported for indirect
 * rendering.
 */
void
__glXGetGLVersion(int *major_version, int *minor_version)
{
   __glXExtensionsCtr();
   *major_version = gl_major;
   *minor_version = gl_minor;
}


/**
 * Get a string representing the set of extensions supported by the client
 * library.  This is currently only used to send the list of extensions
 * supported by the client to the server.
 */
char *
__glXGetClientGLExtensionString(void)
{
   __glXExtensionsCtr();
   return __glXGetStringFromTable(known_gl_extensions, client_gl_support);
}
