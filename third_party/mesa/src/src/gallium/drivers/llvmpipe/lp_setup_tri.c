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
 * Binning code for triangles
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_sse.h"
#include "lp_perf.h"
#include "lp_setup_context.h"
#include "lp_rast.h"
#include "lp_state_fs.h"
#include "lp_state_setup.h"

#define NUM_CHANNELS 4

#if defined(PIPE_ARCH_SSE)
#include <emmintrin.h>
#endif
   
static INLINE int
subpixel_snap(float a)
{
   return util_iround(FIXED_ONE * a);
}

static INLINE float
fixed_to_float(int a)
{
   return a * (1.0 / FIXED_ONE);
}


/* Position and area in fixed point coordinates */
struct fixed_position {
   int x[4];
   int y[4];
   int area;
   int dx01;
   int dy01;
   int dx20;
   int dy20;
};


/**
 * Alloc space for a new triangle plus the input.a0/dadx/dady arrays
 * immediately after it.
 * The memory is allocated from the per-scene pool, not per-tile.
 * \param tri_size  returns number of bytes allocated
 * \param num_inputs  number of fragment shader inputs
 * \return pointer to triangle space
 */
struct lp_rast_triangle *
lp_setup_alloc_triangle(struct lp_scene *scene,
                        unsigned nr_inputs,
                        unsigned nr_planes,
                        unsigned *tri_size)
{
   unsigned input_array_sz = NUM_CHANNELS * (nr_inputs + 1) * sizeof(float);
   unsigned plane_sz = nr_planes * sizeof(struct lp_rast_plane);
   struct lp_rast_triangle *tri;

   *tri_size = (sizeof(struct lp_rast_triangle) +
                3 * input_array_sz +
                plane_sz);

   tri = lp_scene_alloc_aligned( scene, *tri_size, 16 );
   if (tri == NULL)
      return NULL;

   tri->inputs.stride = input_array_sz;

   {
      char *a = (char *)tri;
      char *b = (char *)&GET_PLANES(tri)[nr_planes];
      assert(b - a == *tri_size);
   }

   return tri;
}

void
lp_setup_print_vertex(struct lp_setup_context *setup,
                      const char *name,
                      const float (*v)[4])
{
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   int i, j;

   debug_printf("   wpos (%s[0]) xyzw %f %f %f %f\n",
                name,
                v[0][0], v[0][1], v[0][2], v[0][3]);

   for (i = 0; i < key->num_inputs; i++) {
      const float *in = v[key->inputs[i].src_index];

      debug_printf("  in[%d] (%s[%d]) %s%s%s%s ",
                   i, 
                   name, key->inputs[i].src_index,
                   (key->inputs[i].usage_mask & 0x1) ? "x" : " ",
                   (key->inputs[i].usage_mask & 0x2) ? "y" : " ",
                   (key->inputs[i].usage_mask & 0x4) ? "z" : " ",
                   (key->inputs[i].usage_mask & 0x8) ? "w" : " ");

      for (j = 0; j < 4; j++)
         if (key->inputs[i].usage_mask & (1<<j))
            debug_printf("%.5f ", in[j]);

      debug_printf("\n");
   }
}


/**
 * Print triangle vertex attribs (for debug).
 */
void
lp_setup_print_triangle(struct lp_setup_context *setup,
                        const float (*v0)[4],
                        const float (*v1)[4],
                        const float (*v2)[4])
{
   debug_printf("triangle\n");

   {
      const float ex = v0[0][0] - v2[0][0];
      const float ey = v0[0][1] - v2[0][1];
      const float fx = v1[0][0] - v2[0][0];
      const float fy = v1[0][1] - v2[0][1];

      /* det = cross(e,f).z */
      const float det = ex * fy - ey * fx;
      if (det < 0.0f) 
         debug_printf("   - ccw\n");
      else if (det > 0.0f)
         debug_printf("   - cw\n");
      else
         debug_printf("   - zero area\n");
   }

   lp_setup_print_vertex(setup, "v0", v0);
   lp_setup_print_vertex(setup, "v1", v1);
   lp_setup_print_vertex(setup, "v2", v2);
}


#define MAX_PLANES 8
static unsigned
lp_rast_tri_tab[MAX_PLANES+1] = {
   0,               /* should be impossible */
   LP_RAST_OP_TRIANGLE_1,
   LP_RAST_OP_TRIANGLE_2,
   LP_RAST_OP_TRIANGLE_3,
   LP_RAST_OP_TRIANGLE_4,
   LP_RAST_OP_TRIANGLE_5,
   LP_RAST_OP_TRIANGLE_6,
   LP_RAST_OP_TRIANGLE_7,
   LP_RAST_OP_TRIANGLE_8
};



/**
 * The primitive covers the whole tile- shade whole tile.
 *
 * \param tx, ty  the tile position in tiles, not pixels
 */
static boolean
lp_setup_whole_tile(struct lp_setup_context *setup,
                    const struct lp_rast_shader_inputs *inputs,
                    int tx, int ty)
{
   struct lp_scene *scene = setup->scene;

   LP_COUNT(nr_fully_covered_64);

   /* if variant is opaque and scissor doesn't effect the tile */
   if (inputs->opaque) {
      if (!scene->fb.zsbuf) {
         /*
          * All previous rendering will be overwritten so reset the bin.
          */
         lp_scene_bin_reset( scene, tx, ty );
      }

      LP_COUNT(nr_shade_opaque_64);
      return lp_scene_bin_cmd_with_state( scene, tx, ty,
                                          setup->fs.stored,
                                          LP_RAST_OP_SHADE_TILE_OPAQUE,
                                          lp_rast_arg_inputs(inputs) );
   } else {
      LP_COUNT(nr_shade_64);
      return lp_scene_bin_cmd_with_state( scene, tx, ty,
                                          setup->fs.stored, 
                                          LP_RAST_OP_SHADE_TILE,
                                          lp_rast_arg_inputs(inputs) );
   }
}


/**
 * Do basic setup for triangle rasterization and determine which
 * framebuffer tiles are touched.  Put the triangle in the scene's
 * bins for the tiles which we overlap.
 */
static boolean
do_triangle_ccw(struct lp_setup_context *setup,
                struct fixed_position* position,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4],
                boolean frontfacing )
{
   struct lp_scene *scene = setup->scene;
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   struct lp_rast_triangle *tri;
   struct lp_rast_plane *plane;
   struct u_rect bbox;
   unsigned tri_bytes;
   int nr_planes = 3;

   /* Area should always be positive here */
   assert(position->area > 0);

   if (0)
      lp_setup_print_triangle(setup, v0, v1, v2);

   if (setup->scissor_test) {
      nr_planes = 7;
   }
   else {
      nr_planes = 3;
   }

   /* Bounding rectangle (in pixels) */
   {
      /* Yes this is necessary to accurately calculate bounding boxes
       * with the two fill-conventions we support.  GL (normally) ends
       * up needing a bottom-left fill convention, which requires
       * slightly different rounding.
       */
      int adj = (setup->pixel_offset != 0) ? 1 : 0;

      /* Inclusive x0, exclusive x1 */
      bbox.x0 =  MIN3(position->x[0], position->x[1], position->x[2]) >> FIXED_ORDER;
      bbox.x1 = (MAX3(position->x[0], position->x[1], position->x[2]) - 1) >> FIXED_ORDER;

      /* Inclusive / exclusive depending upon adj (bottom-left or top-right) */
      bbox.y0 = (MIN3(position->y[0], position->y[1], position->y[2]) + adj) >> FIXED_ORDER;
      bbox.y1 = (MAX3(position->y[0], position->y[1], position->y[2]) - 1 + adj) >> FIXED_ORDER;
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

   /* Can safely discard negative regions, but need to keep hold of
    * information about when the triangle extends past screen
    * boundaries.  See trimmed_box in lp_setup_bin_triangle().
    */
   bbox.x0 = MAX2(bbox.x0, 0);
   bbox.y0 = MAX2(bbox.y0, 0);

   tri = lp_setup_alloc_triangle(scene,
                                 key->num_inputs,
                                 nr_planes,
                                 &tri_bytes);
   if (!tri)
      return FALSE;

#if 0
   tri->v[0][0] = v0[0][0];
   tri->v[1][0] = v1[0][0];
   tri->v[2][0] = v2[0][0];
   tri->v[0][1] = v0[0][1];
   tri->v[1][1] = v1[0][1];
   tri->v[2][1] = v2[0][1];
#endif

   LP_COUNT(nr_tris);

   /* Setup parameter interpolants:
    */
   setup->setup.variant->jit_function( v0,
				       v1,
				       v2,
				       frontfacing,
				       GET_A0(&tri->inputs),
				       GET_DADX(&tri->inputs),
				       GET_DADY(&tri->inputs) );

   tri->inputs.frontfacing = frontfacing;
   tri->inputs.disable = FALSE;
   tri->inputs.opaque = setup->fs.current.variant->opaque;

   if (0)
      lp_dump_setup_coef(&setup->setup.variant->key,
			 (const float (*)[4])GET_A0(&tri->inputs),
			 (const float (*)[4])GET_DADX(&tri->inputs),
			 (const float (*)[4])GET_DADY(&tri->inputs));

   plane = GET_PLANES(tri);

#if defined(PIPE_ARCH_SSE)
   {
      __m128i vertx, verty;
      __m128i shufx, shufy;
      __m128i dcdx, dcdy, c;
      __m128i unused;
      __m128i dcdx_neg_mask;
      __m128i dcdy_neg_mask;
      __m128i dcdx_zero_mask;
      __m128i top_left_flag;
      __m128i c_inc_mask, c_inc;
      __m128i eo, p0, p1, p2;
      __m128i zero = _mm_setzero_si128();

      vertx = _mm_loadu_si128((__m128i *)position->x); /* vertex x coords */
      verty = _mm_loadu_si128((__m128i *)position->y); /* vertex y coords */

      shufx = _mm_shuffle_epi32(vertx, _MM_SHUFFLE(3,0,2,1));
      shufy = _mm_shuffle_epi32(verty, _MM_SHUFFLE(3,0,2,1));

      dcdx = _mm_sub_epi32(verty, shufy);
      dcdy = _mm_sub_epi32(vertx, shufx);

      dcdx_neg_mask = _mm_srai_epi32(dcdx, 31);
      dcdx_zero_mask = _mm_cmpeq_epi32(dcdx, zero);
      dcdy_neg_mask = _mm_srai_epi32(dcdy, 31);

      top_left_flag = _mm_set1_epi32((setup->pixel_offset == 0) ? ~0 : 0);

      c_inc_mask = _mm_or_si128(dcdx_neg_mask,
                                _mm_and_si128(dcdx_zero_mask,
                                              _mm_xor_si128(dcdy_neg_mask,
                                                            top_left_flag)));

      c_inc = _mm_srli_epi32(c_inc_mask, 31);

      c = _mm_sub_epi32(mm_mullo_epi32(dcdx, vertx),
                        mm_mullo_epi32(dcdy, verty));

      c = _mm_add_epi32(c, c_inc);

      /* Scale up to match c:
       */
      dcdx = _mm_slli_epi32(dcdx, FIXED_ORDER);
      dcdy = _mm_slli_epi32(dcdy, FIXED_ORDER);

      /* Calculate trivial reject values:
       */
      eo = _mm_sub_epi32(_mm_andnot_si128(dcdy_neg_mask, dcdy),
                         _mm_and_si128(dcdx_neg_mask, dcdx));

      /* ei = _mm_sub_epi32(_mm_sub_epi32(dcdy, dcdx), eo); */

      /* Pointless transpose which gets undone immediately in
       * rasterization:
       */
      transpose4_epi32(&c, &dcdx, &dcdy, &eo,
                       &p0, &p1, &p2, &unused);

      _mm_store_si128((__m128i *)&plane[0], p0);
      _mm_store_si128((__m128i *)&plane[1], p1);
      _mm_store_si128((__m128i *)&plane[2], p2);
   }
#else
   {
      int i;
      plane[0].dcdy = position->dx01;
      plane[1].dcdy = position->x[1] - position->x[2];
      plane[2].dcdy = position->dx20;
      plane[0].dcdx = position->dy01;
      plane[1].dcdx = position->y[1] - position->y[2];
      plane[2].dcdx = position->dy20;
  
      for (i = 0; i < 3; i++) {
         /* half-edge constants, will be interated over the whole render
          * target.
          */
         plane[i].c = plane[i].dcdx * position->x[i] - plane[i].dcdy * position->y[i];

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
   }
#endif

   if (0) {
      debug_printf("p0: %08x/%08x/%08x/%08x\n",
                   plane[0].c,
                   plane[0].dcdx,
                   plane[0].dcdy,
                   plane[0].eo);
      
      debug_printf("p1: %08x/%08x/%08x/%08x\n",
                   plane[1].c,
                   plane[1].dcdx,
                   plane[1].dcdy,
                   plane[1].eo);
      
      debug_printf("p0: %08x/%08x/%08x/%08x\n",
                   plane[2].c,
                   plane[2].dcdx,
                   plane[2].dcdy,
                   plane[2].eo);
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
   if (nr_planes == 7) {
      const struct u_rect *scissor = &setup->scissor;

      plane[3].dcdx = -1;
      plane[3].dcdy = 0;
      plane[3].c = 1-scissor->x0;
      plane[3].eo = 1;

      plane[4].dcdx = 1;
      plane[4].dcdy = 0;
      plane[4].c = scissor->x1+1;
      plane[4].eo = 0;

      plane[5].dcdx = 0;
      plane[5].dcdy = 1;
      plane[5].c = 1-scissor->y0;
      plane[5].eo = 1;

      plane[6].dcdx = 0;
      plane[6].dcdy = -1;
      plane[6].c = scissor->y1+1;
      plane[6].eo = 0;
   }

   return lp_setup_bin_triangle( setup, tri, &bbox, nr_planes );
}

/*
 * Round to nearest less or equal power of two of the input.
 *
 * Undefined if no bit set exists, so code should check against 0 first.
 */
static INLINE uint32_t 
floor_pot(uint32_t n)
{
#if defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86)
   if (n == 0)
      return 0;

   __asm__("bsr %1,%0"
          : "=r" (n)
          : "rm" (n));
   return 1 << n;
#else
   n |= (n >>  1);
   n |= (n >>  2);
   n |= (n >>  4);
   n |= (n >>  8);
   n |= (n >> 16);
   return n - (n >> 1);
#endif
}


boolean
lp_setup_bin_triangle( struct lp_setup_context *setup,
                       struct lp_rast_triangle *tri,
                       const struct u_rect *bbox,
                       int nr_planes )
{
   struct lp_scene *scene = setup->scene;
   struct u_rect trimmed_box = *bbox;   
   int i;

   /* What is the largest power-of-two boundary this triangle crosses:
    */
   int dx = floor_pot((bbox->x0 ^ bbox->x1) |
		      (bbox->y0 ^ bbox->y1));

   /* The largest dimension of the rasterized area of the triangle
    * (aligned to a 4x4 grid), rounded down to the nearest power of two:
    */
   int sz = floor_pot((bbox->x1 - (bbox->x0 & ~3)) |
		      (bbox->y1 - (bbox->y0 & ~3)));

   /* Now apply scissor, etc to the bounding box.  Could do this
    * earlier, but it confuses the logic for tri-16 and would force
    * the rasterizer to also respect scissor, etc, just for the rare
    * cases where a small triangle extends beyond the scissor.
    */
   u_rect_find_intersection(&setup->draw_region, &trimmed_box);

   /* Determine which tile(s) intersect the triangle's bounding box
    */
   if (dx < TILE_SIZE)
   {
      int ix0 = bbox->x0 / TILE_SIZE;
      int iy0 = bbox->y0 / TILE_SIZE;
      unsigned px = bbox->x0 & 63 & ~3;
      unsigned py = bbox->y0 & 63 & ~3;

      assert(iy0 == bbox->y1 / TILE_SIZE &&
	     ix0 == bbox->x1 / TILE_SIZE);

      if (nr_planes == 3) {
         if (sz < 4)
         {
            /* Triangle is contained in a single 4x4 stamp:
             */
            assert(px + 4 <= TILE_SIZE);
            assert(py + 4 <= TILE_SIZE);
            return lp_scene_bin_cmd_with_state( scene, ix0, iy0,
                                                setup->fs.stored,
                                                LP_RAST_OP_TRIANGLE_3_4,
                                                lp_rast_arg_triangle_contained(tri, px, py) );
         }

         if (sz < 16)
         {
            /* Triangle is contained in a single 16x16 block:
             */

            /*
             * The 16x16 block is only 4x4 aligned, and can exceed the tile
             * dimensions if the triangle is 16 pixels in one dimension but 4
             * in the other. So budge the 16x16 back inside the tile.
             */
            px = MIN2(px, TILE_SIZE - 16);
            py = MIN2(py, TILE_SIZE - 16);

            assert(px + 16 <= TILE_SIZE);
            assert(py + 16 <= TILE_SIZE);

            return lp_scene_bin_cmd_with_state( scene, ix0, iy0,
                                                setup->fs.stored,
                                                LP_RAST_OP_TRIANGLE_3_16,
                                                lp_rast_arg_triangle_contained(tri, px, py) );
         }
      }
      else if (nr_planes == 4 && sz < 16) 
      {
         px = MIN2(px, TILE_SIZE - 16);
         py = MIN2(py, TILE_SIZE - 16);

         assert(px + 16 <= TILE_SIZE);
         assert(py + 16 <= TILE_SIZE);

         return lp_scene_bin_cmd_with_state(scene, ix0, iy0,
                                            setup->fs.stored,
                                            LP_RAST_OP_TRIANGLE_4_16,
                                            lp_rast_arg_triangle_contained(tri, px, py));
      }


      /* Triangle is contained in a single tile:
       */
      return lp_scene_bin_cmd_with_state( scene, ix0, iy0, setup->fs.stored,
                                          lp_rast_tri_tab[nr_planes], 
                                          lp_rast_arg_triangle(tri, (1<<nr_planes)-1) );
   }
   else
   {
      struct lp_rast_plane *plane = GET_PLANES(tri);
      int c[MAX_PLANES];
      int ei[MAX_PLANES];

      int eo[MAX_PLANES];
      int xstep[MAX_PLANES];
      int ystep[MAX_PLANES];
      int x, y;

      int ix0 = trimmed_box.x0 / TILE_SIZE;
      int iy0 = trimmed_box.y0 / TILE_SIZE;
      int ix1 = trimmed_box.x1 / TILE_SIZE;
      int iy1 = trimmed_box.y1 / TILE_SIZE;
      
      for (i = 0; i < nr_planes; i++) {
         c[i] = (plane[i].c + 
                 plane[i].dcdy * iy0 * TILE_SIZE - 
                 plane[i].dcdx * ix0 * TILE_SIZE);

         ei[i] = (plane[i].dcdy - 
                  plane[i].dcdx - 
                  plane[i].eo) << TILE_ORDER;

         eo[i] = plane[i].eo << TILE_ORDER;
         xstep[i] = -(plane[i].dcdx << TILE_ORDER);
         ystep[i] = plane[i].dcdy << TILE_ORDER;
      }



      /* Test tile-sized blocks against the triangle.
       * Discard blocks fully outside the tri.  If the block is fully
       * contained inside the tri, bin an lp_rast_shade_tile command.
       * Else, bin a lp_rast_triangle command.
       */
      for (y = iy0; y <= iy1; y++)
      {
	 boolean in = FALSE;  /* are we inside the triangle? */
	 int cx[MAX_PLANES];

         for (i = 0; i < nr_planes; i++)
            cx[i] = c[i];

	 for (x = ix0; x <= ix1; x++)
	 {
            int out = 0;
            int partial = 0;

            for (i = 0; i < nr_planes; i++) {
               int planeout = cx[i] + eo[i];
               int planepartial = cx[i] + ei[i] - 1;
               out |= (planeout >> 31);
               partial |= (planepartial >> 31) & (1<<i);
            }

            if (out) {
               /* do nothing */
               if (in)
                  break;  /* exiting triangle, all done with this row */
               LP_COUNT(nr_empty_64);
            }
            else if (partial) {
               /* Not trivially accepted by at least one plane - 
                * rasterize/shade partial tile
                */
               int count = util_bitcount(partial);
               in = TRUE;
               
               if (!lp_scene_bin_cmd_with_state( scene, x, y,
                                                 setup->fs.stored,
                                                 lp_rast_tri_tab[count], 
                                                 lp_rast_arg_triangle(tri, partial) ))
                  goto fail;

               LP_COUNT(nr_partially_covered_64);
            }
            else {
               /* triangle covers the whole tile- shade whole tile */
               LP_COUNT(nr_fully_covered_64);
               in = TRUE;
               if (!lp_setup_whole_tile(setup, &tri->inputs, x, y))
                  goto fail;
            }

	    /* Iterate cx values across the region:
	     */
            for (i = 0; i < nr_planes; i++)
               cx[i] += xstep[i];
	 }
      
	 /* Iterate c values down the region:
	  */
         for (i = 0; i < nr_planes; i++)
            c[i] += ystep[i];
      }
   }

   return TRUE;

fail:
   /* Need to disable any partially binned triangle.  This is easier
    * than trying to locate all the triangle, shade-tile, etc,
    * commands which may have been binned.
    */
   tri->inputs.disable = TRUE;
   return FALSE;
}


/**
 * Try to draw the triangle, restart the scene on failure.
 */
static void retry_triangle_ccw( struct lp_setup_context *setup,
                                struct fixed_position* position,
                                const float (*v0)[4],
                                const float (*v1)[4],
                                const float (*v2)[4],
                                boolean front)
{
   if (!do_triangle_ccw( setup, position, v0, v1, v2, front ))
   {
      if (!lp_setup_flush_and_restart(setup))
         return;

      if (!do_triangle_ccw( setup, position, v0, v1, v2, front ))
         return;
   }
}


/**
 * Calculate fixed position data for a triangle
 */
static INLINE void
calc_fixed_position( struct lp_setup_context *setup,
                     struct fixed_position* position,
                     const float (*v0)[4],
                     const float (*v1)[4],
                     const float (*v2)[4])
{
   position->x[0] = subpixel_snap(v0[0][0] - setup->pixel_offset);
   position->x[1] = subpixel_snap(v1[0][0] - setup->pixel_offset);
   position->x[2] = subpixel_snap(v2[0][0] - setup->pixel_offset);
   position->x[3] = 0;

   position->y[0] = subpixel_snap(v0[0][1] - setup->pixel_offset);
   position->y[1] = subpixel_snap(v1[0][1] - setup->pixel_offset);
   position->y[2] = subpixel_snap(v2[0][1] - setup->pixel_offset);
   position->y[3] = 0;

   position->dx01 = position->x[0] - position->x[1];
   position->dy01 = position->y[0] - position->y[1];

   position->dx20 = position->x[2] - position->x[0];
   position->dy20 = position->y[2] - position->y[0];

   position->area = position->dx01 * position->dy20 - position->dx20 * position->dy01;
}


/**
 * Rotate a triangle, flipping its clockwise direction,
 * Swaps values for xy[0] and xy[1]
 */
static INLINE void
rotate_fixed_position_01( struct fixed_position* position )
{
   int x, y;

   x = position->x[1];
   y = position->y[1];
   position->x[1] = position->x[0];
   position->y[1] = position->y[0];
   position->x[0] = x;
   position->y[0] = y;

   position->dx01 = -position->dx01;
   position->dy01 = -position->dy01;
   position->dx20 = position->x[2] - position->x[0];
   position->dy20 = position->y[2] - position->y[0];

   position->area = -position->area;
}


/**
 * Rotate a triangle, flipping its clockwise direction,
 * Swaps values for xy[1] and xy[2]
 */
static INLINE void
rotate_fixed_position_12( struct fixed_position* position )
{
   int x, y;

   x = position->x[2];
   y = position->y[2];
   position->x[2] = position->x[1];
   position->y[2] = position->y[1];
   position->x[1] = x;
   position->y[1] = y;

   x = position->dx01;
   y = position->dy01;
   position->dx01 = -position->dx20;
   position->dy01 = -position->dy20;
   position->dx20 = -x;
   position->dy20 = -y;

   position->area = -position->area;
}


/**
 * Draw triangle if it's CW, cull otherwise.
 */
static void triangle_cw( struct lp_setup_context *setup,
			 const float (*v0)[4],
			 const float (*v1)[4],
			 const float (*v2)[4] )
{
   struct fixed_position position;
   calc_fixed_position(setup, &position, v0, v1, v2);

   if (position.area < 0) {
      if (setup->flatshade_first) {
         rotate_fixed_position_12(&position);
         retry_triangle_ccw(setup, &position, v0, v2, v1, !setup->ccw_is_frontface);
      } else {
         rotate_fixed_position_01(&position);
         retry_triangle_ccw(setup, &position, v1, v0, v2, !setup->ccw_is_frontface);
      }
   }
}


static void triangle_ccw( struct lp_setup_context *setup,
                          const float (*v0)[4],
                          const float (*v1)[4],
                          const float (*v2)[4])
{
   struct fixed_position position;
   calc_fixed_position(setup, &position, v0, v1, v2);

   if (position.area > 0)
      retry_triangle_ccw(setup, &position, v0, v1, v2, setup->ccw_is_frontface);
}

/**
 * Draw triangle whether it's CW or CCW.
 */
static void triangle_both( struct lp_setup_context *setup,
			   const float (*v0)[4],
			   const float (*v1)[4],
			   const float (*v2)[4] )
{
   struct fixed_position position;
   calc_fixed_position(setup, &position, v0, v1, v2);

   if (0) {
      assert(!util_is_inf_or_nan(v0[0][0]));
      assert(!util_is_inf_or_nan(v0[0][1]));
      assert(!util_is_inf_or_nan(v1[0][0]));
      assert(!util_is_inf_or_nan(v1[0][1]));
      assert(!util_is_inf_or_nan(v2[0][0]));
      assert(!util_is_inf_or_nan(v2[0][1]));
   }

   if (position.area > 0)
      retry_triangle_ccw( setup, &position, v0, v1, v2, setup->ccw_is_frontface );
   else if (position.area < 0) {
      if (setup->flatshade_first) {
         rotate_fixed_position_12( &position );
         retry_triangle_ccw( setup, &position, v0, v2, v1, !setup->ccw_is_frontface );
      } else {
         rotate_fixed_position_01( &position );
         retry_triangle_ccw( setup, &position, v1, v0, v2, !setup->ccw_is_frontface );
      }
   }
}


static void triangle_nop( struct lp_setup_context *setup,
			  const float (*v0)[4],
			  const float (*v1)[4],
			  const float (*v2)[4] )
{
}


void 
lp_setup_choose_triangle( struct lp_setup_context *setup )
{
   switch (setup->cullmode) {
   case PIPE_FACE_NONE:
      setup->triangle = triangle_both;
      break;
   case PIPE_FACE_BACK:
      setup->triangle = setup->ccw_is_frontface ? triangle_ccw : triangle_cw;
      break;
   case PIPE_FACE_FRONT:
      setup->triangle = setup->ccw_is_frontface ? triangle_cw : triangle_ccw;
      break;
   default:
      setup->triangle = triangle_nop;
      break;
   }
}
