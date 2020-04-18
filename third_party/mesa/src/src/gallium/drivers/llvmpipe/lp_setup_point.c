/**************************************************************************
 *
 * Copyright 2010, VMware Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Binning code for points
 */

#include "lp_setup_context.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_perf.h"
#include "lp_rast.h"
#include "lp_state_fs.h"
#include "lp_state_setup.h"
#include "tgsi/tgsi_scan.h"

#define NUM_CHANNELS 4

struct point_info {
   /* x,y deltas */
   int dy01, dy12;
   int dx01, dx12;

   const float (*v0)[4];

   float (*a0)[4];
   float (*dadx)[4];
   float (*dady)[4];
};   


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 */
static void
constant_coef(struct lp_setup_context *setup,
              struct point_info *info,
              unsigned slot,
              const float value,
              unsigned i)
{
   info->a0[slot][i] = value;
   info->dadx[slot][i] = 0.0f;
   info->dady[slot][i] = 0.0f;
}


static void
point_persp_coeff(struct lp_setup_context *setup,
                  const struct point_info *info,
                  unsigned slot,
                  unsigned i)
{
   /*
    * Fragment shader expects pre-multiplied w for LP_INTERP_PERSPECTIVE. A
    * better stratergy would be to take the primitive in consideration when
    * generating the fragment shader key, and therefore avoid the per-fragment
    * perspective divide.
    */

   float w0 = info->v0[0][3];

   assert(i < 4);

   info->a0[slot][i] = info->v0[slot][i]*w0;
   info->dadx[slot][i] = 0.0f;
   info->dady[slot][i] = 0.0f;
}


/**
 * Setup automatic texcoord coefficients (for sprite rendering).
 * \param slot  the vertex attribute slot to setup
 * \param i  the attribute channel in [0,3]
 * \param sprite_coord_origin  one of PIPE_SPRITE_COORD_x
 * \param perspective  does the shader expects pre-multiplied w, i.e.,
 *    LP_INTERP_PERSPECTIVE is specified in the shader key
 */
static void
texcoord_coef(struct lp_setup_context *setup,
              const struct point_info *info,
              unsigned slot,
              unsigned i,
              unsigned sprite_coord_origin,
              boolean perspective)
{
   float w0 = info->v0[0][3];

   assert(i < 4);

   if (i == 0) {
      float dadx = FIXED_ONE / (float)info->dx12;
      float dady =  0.0f;
      float x0 = info->v0[0][0] - setup->pixel_offset;
      float y0 = info->v0[0][1] - setup->pixel_offset;

      info->dadx[slot][0] = dadx;
      info->dady[slot][0] = dady;
      info->a0[slot][0] = 0.5 - (dadx * x0 + dady * y0);

      if (perspective) {
         info->dadx[slot][0] *= w0;
         info->dady[slot][0] *= w0;
         info->a0[slot][0] *= w0;
      }
   }
   else if (i == 1) {
      float dadx = 0.0f;
      float dady = FIXED_ONE / (float)info->dx12;
      float x0 = info->v0[0][0] - setup->pixel_offset;
      float y0 = info->v0[0][1] - setup->pixel_offset;

      if (sprite_coord_origin == PIPE_SPRITE_COORD_LOWER_LEFT) {
         dady = -dady;
      }

      info->dadx[slot][1] = dadx;
      info->dady[slot][1] = dady;
      info->a0[slot][1] = 0.5 - (dadx * x0 + dady * y0);

      if (perspective) {
         info->dadx[slot][1] *= w0;
         info->dady[slot][1] *= w0;
         info->a0[slot][1] *= w0;
      }
   }
   else if (i == 2) {
      info->a0[slot][2] = 0.0f;
      info->dadx[slot][2] = 0.0f;
      info->dady[slot][2] = 0.0f;
   }
   else {
      info->a0[slot][3] = perspective ? w0 : 1.0f;
      info->dadx[slot][3] = 0.0f;
      info->dady[slot][3] = 0.0f;
   }
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial
 * Z and W are copied from position_coef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_point_fragcoord_coef(struct lp_setup_context *setup,
                           struct point_info *info,
                           unsigned slot,
                           unsigned usage_mask)
{
   /*X*/
   if (usage_mask & TGSI_WRITEMASK_X) {
      info->a0[slot][0] = 0.0;
      info->dadx[slot][0] = 1.0;
      info->dady[slot][0] = 0.0;
   }

   /*Y*/
   if (usage_mask & TGSI_WRITEMASK_Y) {
      info->a0[slot][1] = 0.0;
      info->dadx[slot][1] = 0.0;
      info->dady[slot][1] = 1.0;
   }

   /*Z*/
   if (usage_mask & TGSI_WRITEMASK_Z) {
      constant_coef(setup, info, slot, info->v0[0][2], 2);
   }

   /*W*/
   if (usage_mask & TGSI_WRITEMASK_W) {
      constant_coef(setup, info, slot, info->v0[0][3], 3);
   }
}


/**
 * Compute the point->coef[] array dadx, dady, a0 values.
 */
static void   
setup_point_coefficients( struct lp_setup_context *setup,
                          struct point_info *info)
{
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   const struct lp_fragment_shader *shader = setup->fs.current.variant->shader;
   unsigned fragcoord_usage_mask = TGSI_WRITEMASK_XYZ;
   unsigned slot;

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < key->num_inputs; slot++) {
      unsigned vert_attr = key->inputs[slot].src_index;
      unsigned usage_mask = key->inputs[slot].usage_mask;
      enum lp_interp interp = key->inputs[slot].interp;
      boolean perspective = !!(interp == LP_INTERP_PERSPECTIVE);
      unsigned i;

      if (perspective & usage_mask) {
         fragcoord_usage_mask |= TGSI_WRITEMASK_W;
      }
      
      switch (interp) {
      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0, so all need to ensure that the usage mask is covers all
          * usages.
          */
         fragcoord_usage_mask |= usage_mask;
         break;

      case LP_INTERP_LINEAR:
         /* Sprite tex coords may use linear interpolation someday */
         /* fall-through */
      case LP_INTERP_PERSPECTIVE:
         /* check if the sprite coord flag is set for this attribute.
          * If so, set it up so it up so x and y vary from 0 to 1.
          */
         if (shader->info.base.input_semantic_name[slot] == TGSI_SEMANTIC_GENERIC) {
            unsigned semantic_index = shader->info.base.input_semantic_index[slot];
            /* Note that sprite_coord enable is a bitfield of
             * PIPE_MAX_SHADER_OUTPUTS bits.
             */
            if (semantic_index < PIPE_MAX_SHADER_OUTPUTS &&
                (setup->sprite_coord_enable & (1 << semantic_index))) {
               for (i = 0; i < NUM_CHANNELS; i++) {
                  if (usage_mask & (1 << i)) {
                     texcoord_coef(setup, info, slot + 1, i,
                                   setup->sprite_coord_origin,
                                   perspective);
                  }
               }
               break;
            }
         }
         /* fall-through */
      case LP_INTERP_CONSTANT:
         for (i = 0; i < NUM_CHANNELS; i++) {
            if (usage_mask & (1 << i)) {
               if (perspective) {
                  point_persp_coeff(setup, info, slot+1, i);
               }
               else {
                  constant_coef(setup, info, slot+1, info->v0[vert_attr][i], i);
               }
            }
         }
         break;

      case LP_INTERP_FACING:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               constant_coef(setup, info, slot+1, 1.0, i);
         break;

      default:
         assert(0);
         break;
      }
   }

   /* The internal position input is in slot zero:
    */
   setup_point_fragcoord_coef(setup, info, 0,
                              fragcoord_usage_mask);
}


static INLINE int
subpixel_snap(float a)
{
   return util_iround(FIXED_ONE * a);
}


static boolean
try_setup_point( struct lp_setup_context *setup,
                 const float (*v0)[4] )
{
   /* x/y positions in fixed point */
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   const int sizeAttr = setup->psize;
   const float size
      = (setup->point_size_per_vertex && sizeAttr > 0) ? v0[sizeAttr][0]
      : setup->point_size;
   
   /* Point size as fixed point integer, remove rounding errors 
    * and gives minimum width for very small points
    */
   int fixed_width = MAX2(FIXED_ONE,
                          (subpixel_snap(size) + FIXED_ONE/2 - 1) & ~(FIXED_ONE-1));

   const int x0 = subpixel_snap(v0[0][0] - setup->pixel_offset) - fixed_width/2;
   const int y0 = subpixel_snap(v0[0][1] - setup->pixel_offset) - fixed_width/2;
     
   struct lp_scene *scene = setup->scene;
   struct lp_rast_triangle *point;
   unsigned bytes;
   struct u_rect bbox;
   unsigned nr_planes = 4;
   struct point_info info;


   /* Bounding rectangle (in pixels) */
   {
      /* Yes this is necessary to accurately calculate bounding boxes
       * with the two fill-conventions we support.  GL (normally) ends
       * up needing a bottom-left fill convention, which requires
       * slightly different rounding.
       */
      int adj = (setup->pixel_offset != 0) ? 1 : 0;

      bbox.x0 = (x0 + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      bbox.x1 = (x0 + fixed_width + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      bbox.y0 = (y0 + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.y1 = (y0 + fixed_width + (FIXED_ONE-1)) >> FIXED_ORDER;

      /* Inclusive coordinates:
       */
      bbox.x1--;
      bbox.y1--;
   }
   
   if (!u_rect_test_intersection(&setup->draw_region, &bbox)) {
      if (0) debug_printf("offscreen\n");
      LP_COUNT(nr_culled_tris);
      return TRUE;
   }

   u_rect_find_intersection(&setup->draw_region, &bbox);

   point = lp_setup_alloc_triangle(scene,
                                   key->num_inputs,
                                   nr_planes,
                                   &bytes);
   if (!point)
      return FALSE;

#ifdef DEBUG
   point->v[0][0] = v0[0][0];
   point->v[0][1] = v0[0][1];
#endif

   info.v0 = v0;
   info.dx01 = 0;
   info.dx12 = fixed_width;
   info.dy01 = fixed_width;
   info.dy12 = 0;
   info.a0 = GET_A0(&point->inputs);
   info.dadx = GET_DADX(&point->inputs);
   info.dady = GET_DADY(&point->inputs);
   
   /* Setup parameter interpolants:
    */
   setup_point_coefficients(setup, &info);

   point->inputs.frontfacing = TRUE;
   point->inputs.disable = FALSE;
   point->inputs.opaque = FALSE;

   {
      struct lp_rast_plane *plane = GET_PLANES(point);

      plane[0].dcdx = -1;
      plane[0].dcdy = 0;
      plane[0].c = 1-bbox.x0;
      plane[0].eo = 1;

      plane[1].dcdx = 1;
      plane[1].dcdy = 0;
      plane[1].c = bbox.x1+1;
      plane[1].eo = 0;

      plane[2].dcdx = 0;
      plane[2].dcdy = 1;
      plane[2].c = 1-bbox.y0;
      plane[2].eo = 1;

      plane[3].dcdx = 0;
      plane[3].dcdy = -1;
      plane[3].c = bbox.y1+1;
      plane[3].eo = 0;
   }

   return lp_setup_bin_triangle(setup, point, &bbox, nr_planes);
}


static void 
lp_setup_point(struct lp_setup_context *setup,
               const float (*v0)[4])
{
   if (!try_setup_point( setup, v0 ))
   {
      if (!lp_setup_flush_and_restart(setup))
         return;

      if (!try_setup_point( setup, v0 ))
         return;
   }
}


void 
lp_setup_choose_point( struct lp_setup_context *setup )
{
   setup->point = lp_setup_point;
}


