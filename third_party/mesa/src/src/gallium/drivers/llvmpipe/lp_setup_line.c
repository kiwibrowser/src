/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/*
 * Binning code for lines
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_perf.h"
#include "lp_setup_context.h"
#include "lp_rast.h"
#include "lp_state_fs.h"
#include "lp_state_setup.h"

#define NUM_CHANNELS 4

struct lp_line_info {

   float dx;
   float dy;
   float oneoverarea;

   const float (*v1)[4];
   const float (*v2)[4];

   float (*a0)[4];
   float (*dadx)[4];
   float (*dady)[4];
};


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 */
static void constant_coef( struct lp_setup_context *setup,
                           struct lp_line_info *info,
                           unsigned slot,
                           const float value,
                           unsigned i )
{
   info->a0[slot][i] = value;
   info->dadx[slot][i] = 0.0f;
   info->dady[slot][i] = 0.0f;
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void linear_coef( struct lp_setup_context *setup,
                         struct lp_line_info *info,
                         unsigned slot,
                         unsigned vert_attr,
                         unsigned i)
{
   float a1 = info->v1[vert_attr][i]; 
   float a2 = info->v2[vert_attr][i];
      
   float da21 = a1 - a2;   
   float dadx = da21 * info->dx * info->oneoverarea;
   float dady = da21 * info->dy * info->oneoverarea;

   info->dadx[slot][i] = dadx;
   info->dady[slot][i] = dady;  
   
   info->a0[slot][i] = (a1 -
                              (dadx * (info->v1[0][0] - setup->pixel_offset) +
                               dady * (info->v1[0][1] - setup->pixel_offset)));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void perspective_coef( struct lp_setup_context *setup,
                              struct lp_line_info *info,
                              unsigned slot,
                              unsigned vert_attr,
                              unsigned i)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   float a1 = info->v1[vert_attr][i] * info->v1[0][3];
   float a2 = info->v2[vert_attr][i] * info->v2[0][3];

   float da21 = a1 - a2;   
   float dadx = da21 * info->dx * info->oneoverarea;
   float dady = da21 * info->dy * info->oneoverarea;

   info->dadx[slot][i] = dadx;
   info->dady[slot][i] = dady;
   
   info->a0[slot][i] = (a1 -
                        (dadx * (info->v1[0][0] - setup->pixel_offset) +
                         dady * (info->v1[0][1] - setup->pixel_offset)));
}

static void
setup_fragcoord_coef( struct lp_setup_context *setup,
                      struct lp_line_info *info,
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
      linear_coef(setup, info, slot, 0, 2);
   }

   /*W*/
   if (usage_mask & TGSI_WRITEMASK_W) {
      linear_coef(setup, info, slot, 0, 3);
   }
}

/**
 * Compute the tri->coef[] array dadx, dady, a0 values.
 */
static void setup_line_coefficients( struct lp_setup_context *setup,
                                     struct lp_line_info *info)
{
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   unsigned fragcoord_usage_mask = TGSI_WRITEMASK_XYZ;
   unsigned slot;

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < key->num_inputs; slot++) {
      unsigned vert_attr = key->inputs[slot].src_index;
      unsigned usage_mask = key->inputs[slot].usage_mask;
      unsigned i;
           
      switch (key->inputs[slot].interp) {
      case LP_INTERP_CONSTANT:
         if (key->flatshade_first) {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(setup, info, slot+1, info->v1[vert_attr][i], i);
         }
         else {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(setup, info, slot+1, info->v2[vert_attr][i], i);
         }
         break;

      case LP_INTERP_LINEAR:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               linear_coef(setup, info, slot+1, vert_attr, i);
         break;

      case LP_INTERP_PERSPECTIVE:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               perspective_coef(setup, info, slot+1, vert_attr, i);
         fragcoord_usage_mask |= TGSI_WRITEMASK_W;
         break;

      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0, so all need to ensure that the usage mask is covers all
          * usages.
          */
         fragcoord_usage_mask |= usage_mask;
         break;

      case LP_INTERP_FACING:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               constant_coef(setup, info, slot+1, 1.0, i);
         break;

      default:
         assert(0);
      }
   }

   /* The internal position input is in slot zero:
    */
   setup_fragcoord_coef(setup, info, 0,
                        fragcoord_usage_mask);
}



static INLINE int subpixel_snap( float a )
{
   return util_iround(FIXED_ONE * a);
}


/**
 * Print line vertex attribs (for debug).
 */
static void
print_line(struct lp_setup_context *setup,
           const float (*v1)[4],
           const float (*v2)[4])
{
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   uint i;

   debug_printf("llvmpipe line\n");
   for (i = 0; i < 1 + key->num_inputs; i++) {
      debug_printf("  v1[%d]:  %f %f %f %f\n", i,
                   v1[i][0], v1[i][1], v1[i][2], v1[i][3]);
   }
   for (i = 0; i < 1 + key->num_inputs; i++) {
      debug_printf("  v2[%d]:  %f %f %f %f\n", i,
                   v2[i][0], v2[i][1], v2[i][2], v2[i][3]);
   }
}


static INLINE boolean sign(float x){
   return x >= 0;  
}  


/* Used on positive floats only:
 */
static INLINE float fracf(float f)
{
   return f - floorf(f);
}



static boolean
try_setup_line( struct lp_setup_context *setup,
               const float (*v1)[4],
               const float (*v2)[4])
{
   struct lp_scene *scene = setup->scene;
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   struct lp_rast_triangle *line;
   struct lp_rast_plane *plane;
   struct lp_line_info info;
   float width = MAX2(1.0, setup->line_width);
   struct u_rect bbox;
   unsigned tri_bytes;
   int x[4]; 
   int y[4];
   int i;
   int nr_planes = 4;
   
   /* linewidth should be interpreted as integer */
   int fixed_width = util_iround(width) * FIXED_ONE;

   float x_offset=0;
   float y_offset=0;
   float x_offset_end=0;
   float y_offset_end=0;
      
   float x1diff;
   float y1diff;
   float x2diff;
   float y2diff;
   float dx, dy;
   float area;

   boolean draw_start;
   boolean draw_end;
   boolean will_draw_start;
   boolean will_draw_end;

   if (0)
      print_line(setup, v1, v2);

   if (setup->scissor_test) {
      nr_planes = 8;
   }
   else {
      nr_planes = 4;
   }


   dx = v1[0][0] - v2[0][0];
   dy = v1[0][1] - v2[0][1];
   area = (dx * dx  + dy * dy);
   if (area == 0) {
      LP_COUNT(nr_culled_tris);
      return TRUE;
   }

   info.oneoverarea = 1.0f / area;
   info.dx = dx;
   info.dy = dy;
   info.v1 = v1;
   info.v2 = v2;

  
   /* X-MAJOR LINE */
   if (fabsf(dx) >= fabsf(dy)) {
      float dydx = dy / dx;

      x1diff = v1[0][0] - (float) floor(v1[0][0]) - 0.5;
      y1diff = v1[0][1] - (float) floor(v1[0][1]) - 0.5;
      x2diff = v2[0][0] - (float) floor(v2[0][0]) - 0.5;
      y2diff = v2[0][1] - (float) floor(v2[0][1]) - 0.5;

      if (y2diff==-0.5 && dy<0){
         y2diff = 0.5;
      }
      
      /* 
       * Diamond exit rule test for starting point 
       */    
      if (fabsf(x1diff) + fabsf(y1diff) < 0.5) {
         draw_start = TRUE;
      }
      else if (sign(x1diff) == sign(-dx)) {
         draw_start = FALSE;
      }
      else if (sign(-y1diff) != sign(dy)) {
         draw_start = TRUE;
      }
      else {
         /* do intersection test */
         float yintersect = fracf(v1[0][1]) + x1diff * dydx;
         draw_start = (yintersect < 1.0 && yintersect > 0.0);
      }


      /* 
       * Diamond exit rule test for ending point 
       */    
      if (fabsf(x2diff) + fabsf(y2diff) < 0.5) {
         draw_end = FALSE;
      }
      else if (sign(x2diff) != sign(-dx)) {
         draw_end = FALSE;
      }
      else if (sign(-y2diff) == sign(dy)) {
         draw_end = TRUE;
      }
      else {
         /* do intersection test */
         float yintersect = fracf(v2[0][1]) + x2diff * dydx;
         draw_end = (yintersect < 1.0 && yintersect > 0.0);
      }

      /* Are we already drawing start/end?
       */
      will_draw_start = sign(-x1diff) != sign(dx);
      will_draw_end = (sign(x2diff) == sign(-dx)) || x2diff==0;

      if (dx < 0) {
         /* if v2 is to the right of v1, swap pointers */
         const float (*temp)[4] = v1;
         v1 = v2;
         v2 = temp;
         dx = -dx;
         dy = -dy;
         /* Otherwise shift planes appropriately */
         if (will_draw_start != draw_start) {
            x_offset_end = - x1diff - 0.5;
            y_offset_end = x_offset_end * dydx;

         }
         if (will_draw_end != draw_end) {
            x_offset = - x2diff - 0.5;
            y_offset = x_offset * dydx;
         }

      }
      else{
         /* Otherwise shift planes appropriately */
         if (will_draw_start != draw_start) {
            x_offset = - x1diff + 0.5;
            y_offset = x_offset * dydx;
         }
         if (will_draw_end != draw_end) {
            x_offset_end = - x2diff + 0.5;
            y_offset_end = x_offset_end * dydx;
         }
      }
  
      /* x/y positions in fixed point */
      x[0] = subpixel_snap(v1[0][0] + x_offset     - setup->pixel_offset);
      x[1] = subpixel_snap(v2[0][0] + x_offset_end - setup->pixel_offset);
      x[2] = subpixel_snap(v2[0][0] + x_offset_end - setup->pixel_offset);
      x[3] = subpixel_snap(v1[0][0] + x_offset     - setup->pixel_offset);
      
      y[0] = subpixel_snap(v1[0][1] + y_offset     - setup->pixel_offset) - fixed_width/2;
      y[1] = subpixel_snap(v2[0][1] + y_offset_end - setup->pixel_offset) - fixed_width/2;
      y[2] = subpixel_snap(v2[0][1] + y_offset_end - setup->pixel_offset) + fixed_width/2;
      y[3] = subpixel_snap(v1[0][1] + y_offset     - setup->pixel_offset) + fixed_width/2;
      
   }
   else {
      const float dxdy = dx / dy;

      /* Y-MAJOR LINE */      
      x1diff = v1[0][0] - (float) floor(v1[0][0]) - 0.5;
      y1diff = v1[0][1] - (float) floor(v1[0][1]) - 0.5;
      x2diff = v2[0][0] - (float) floor(v2[0][0]) - 0.5;
      y2diff = v2[0][1] - (float) floor(v2[0][1]) - 0.5;

      if (x2diff==-0.5 && dx<0) {
         x2diff = 0.5;
      }

      /* 
       * Diamond exit rule test for starting point 
       */    
      if (fabsf(x1diff) + fabsf(y1diff) < 0.5) {
         draw_start = TRUE;
      }
      else if (sign(-y1diff) == sign(dy)) {
         draw_start = FALSE;
      }
      else if (sign(x1diff) != sign(-dx)) {
         draw_start = TRUE;
      }
      else {
         /* do intersection test */
         float xintersect = fracf(v1[0][0]) + y1diff * dxdy;
         draw_start = (xintersect < 1.0 && xintersect > 0.0);
      }

      /* 
       * Diamond exit rule test for ending point 
       */    
      if (fabsf(x2diff) + fabsf(y2diff) < 0.5) {
         draw_end = FALSE;
      }
      else if (sign(-y2diff) != sign(dy) ) {
         draw_end = FALSE;
      }
      else if (sign(x2diff) == sign(-dx) ) {
         draw_end = TRUE;
      }
      else {
         /* do intersection test */
         float xintersect = fracf(v2[0][0]) + y2diff * dxdy;
         draw_end = (xintersect < 1.0 && xintersect >= 0.0);
      }

      /* Are we already drawing start/end?
       */
      will_draw_start = sign(y1diff) == sign(dy);
      will_draw_end = (sign(-y2diff) == sign(dy)) || y2diff==0;

      if (dy > 0) {
         /* if v2 is on top of v1, swap pointers */
         const float (*temp)[4] = v1;
         v1 = v2;
         v2 = temp; 
         dx = -dx;
         dy = -dy;

         /* Otherwise shift planes appropriately */
         if (will_draw_start != draw_start) {
            y_offset_end = - y1diff + 0.5;
            x_offset_end = y_offset_end * dxdy;
         }
         if (will_draw_end != draw_end) {
            y_offset = - y2diff + 0.5;
            x_offset = y_offset * dxdy;
         }
      }
      else {
         /* Otherwise shift planes appropriately */
         if (will_draw_start != draw_start) {
            y_offset = - y1diff - 0.5;
            x_offset = y_offset * dxdy;
                     
         }
         if (will_draw_end != draw_end) {
            y_offset_end = - y2diff - 0.5;
            x_offset_end = y_offset_end * dxdy;
         }
      }

      /* x/y positions in fixed point */
      x[0] = subpixel_snap(v1[0][0] + x_offset     - setup->pixel_offset) - fixed_width/2;
      x[1] = subpixel_snap(v2[0][0] + x_offset_end - setup->pixel_offset) - fixed_width/2;
      x[2] = subpixel_snap(v2[0][0] + x_offset_end - setup->pixel_offset) + fixed_width/2;
      x[3] = subpixel_snap(v1[0][0] + x_offset     - setup->pixel_offset) + fixed_width/2;
     
      y[0] = subpixel_snap(v1[0][1] + y_offset     - setup->pixel_offset); 
      y[1] = subpixel_snap(v2[0][1] + y_offset_end - setup->pixel_offset);
      y[2] = subpixel_snap(v2[0][1] + y_offset_end - setup->pixel_offset);
      y[3] = subpixel_snap(v1[0][1] + y_offset     - setup->pixel_offset);
   }



   LP_COUNT(nr_tris);

 
   /* Bounding rectangle (in pixels) */
   {
      /* Yes this is necessary to accurately calculate bounding boxes
       * with the two fill-conventions we support.  GL (normally) ends
       * up needing a bottom-left fill convention, which requires
       * slightly different rounding.
       */
      int adj = (setup->pixel_offset != 0) ? 1 : 0;

      bbox.x0 = (MIN4(x[0], x[1], x[2], x[3]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.x1 = (MAX4(x[0], x[1], x[2], x[3]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.y0 = (MIN4(y[0], y[1], y[2], y[3]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      bbox.y1 = (MAX4(y[0], y[1], y[2], y[3]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;

      /* Inclusive coordinates:
       */
      bbox.x1--;
      bbox.y1--;
   }

   if (bbox.x1 < bbox.x0 ||
       bbox.y1 < bbox.y0) {
      if (0) debug_printf("empty bounding box\n");
      LP_COUNT(nr_culled_tris);
      return TRUE;
   }

   if (!u_rect_test_intersection(&setup->draw_region, &bbox)) {
      if (0) debug_printf("offscreen\n");
      LP_COUNT(nr_culled_tris);
      return TRUE;
   }

   /* Can safely discard negative regions:
    */
   bbox.x0 = MAX2(bbox.x0, 0);
   bbox.y0 = MAX2(bbox.y0, 0);

   line = lp_setup_alloc_triangle(scene,
                                  key->num_inputs,
                                  nr_planes,
                                  &tri_bytes);
   if (!line)
      return FALSE;

#ifdef DEBUG
   line->v[0][0] = v1[0][0];
   line->v[1][0] = v2[0][0];   
   line->v[0][1] = v1[0][1];
   line->v[1][1] = v2[0][1];
#endif

   /* calculate the deltas */
   plane = GET_PLANES(line);
   plane[0].dcdy = x[0] - x[1];
   plane[1].dcdy = x[1] - x[2];
   plane[2].dcdy = x[2] - x[3];
   plane[3].dcdy = x[3] - x[0];

   plane[0].dcdx = y[0] - y[1];
   plane[1].dcdx = y[1] - y[2];
   plane[2].dcdx = y[2] - y[3];
   plane[3].dcdx = y[3] - y[0];


   /* Setup parameter interpolants:
    */
   info.a0 = GET_A0(&line->inputs);
   info.dadx = GET_DADX(&line->inputs);
   info.dady = GET_DADY(&line->inputs);
   setup_line_coefficients(setup, &info); 

   line->inputs.frontfacing = TRUE;
   line->inputs.disable = FALSE;
   line->inputs.opaque = FALSE;

   for (i = 0; i < 4; i++) {

      /* half-edge constants, will be interated over the whole render
       * target.
       */
      plane[i].c = plane[i].dcdx * x[i] - plane[i].dcdy * y[i];

      
      /* correct for top-left vs. bottom-left fill convention.  
       *
       * note that we're overloading gl_rasterization_rules to mean
       * both (0.5,0.5) pixel centers *and* bottom-left filling
       * convention.
       *
       * GL actually has a top-left filling convention, but GL's
       * notion of "top" differs from gallium's...
       *
       * Also, sometimes (in FBO cases) GL will render upside down
       * to its usual method, in which case it will probably want
       * to use the opposite, top-left convention.
       */         
      if (plane[i].dcdx < 0) {
         /* both fill conventions want this - adjust for left edges */
         plane[i].c++;            
      }
      else if (plane[i].dcdx == 0) {
         if (setup->pixel_offset == 0) {
            /* correct for top-left fill convention:
             */
            if (plane[i].dcdy > 0) plane[i].c++;
         }
         else {
            /* correct for bottom-left fill convention:
             */
            if (plane[i].dcdy < 0) plane[i].c++;
         }
      }

      plane[i].dcdx *= FIXED_ONE;
      plane[i].dcdy *= FIXED_ONE;

      /* find trivial reject offsets for each edge for a single-pixel
       * sized block.  These will be scaled up at each recursive level to
       * match the active blocksize.  Scaling in this way works best if
       * the blocks are square.
       */
      plane[i].eo = 0;
      if (plane[i].dcdx < 0) plane[i].eo -= plane[i].dcdx;
      if (plane[i].dcdy > 0) plane[i].eo += plane[i].dcdy;
   }


   /* 
    * When rasterizing scissored tris, use the intersection of the
    * triangle bounding box and the scissor rect to generate the
    * scissor planes.
    *
    * This permits us to cut off the triangle "tails" that are present
    * in the intermediate recursive levels caused when two of the
    * triangles edges don't diverge quickly enough to trivially reject
    * exterior blocks from the triangle.
    *
    * It's not really clear if it's worth worrying about these tails,
    * but since we generate the planes for each scissored tri, it's
    * free to trim them in this case.
    * 
    * Note that otherwise, the scissor planes only vary in 'C' value,
    * and even then only on state-changes.  Could alternatively store
    * these planes elsewhere.
    */
   if (nr_planes == 8) {
      const struct u_rect *scissor = &setup->scissor;

      plane[4].dcdx = -1;
      plane[4].dcdy = 0;
      plane[4].c = 1-scissor->x0;
      plane[4].eo = 1;

      plane[5].dcdx = 1;
      plane[5].dcdy = 0;
      plane[5].c = scissor->x1+1;
      plane[5].eo = 0;

      plane[6].dcdx = 0;
      plane[6].dcdy = 1;
      plane[6].c = 1-scissor->y0;
      plane[6].eo = 1;

      plane[7].dcdx = 0;
      plane[7].dcdy = -1;
      plane[7].c = scissor->y1+1;
      plane[7].eo = 0;
   }

   return lp_setup_bin_triangle(setup, line, &bbox, nr_planes);
}


static void lp_setup_line( struct lp_setup_context *setup,
                           const float (*v0)[4],
                           const float (*v1)[4] )
{
   if (!try_setup_line( setup, v0, v1 ))
   {
      if (!lp_setup_flush_and_restart(setup))
         return;

      if (!try_setup_line( setup, v0, v1 ))
         return;
   }
}


void lp_setup_choose_line( struct lp_setup_context *setup ) 
{ 
   setup->line = lp_setup_line;
}


