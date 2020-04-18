/**************************************************************************
 *
 * Copyright 2007-2010 VMware, Inc.
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
 * Rasterization for binned triangles within a tile
 */



/**
 * Prototype for a 8 plane rasterizer function.  Will codegenerate
 * several of these.
 *
 * XXX: Varients for more/fewer planes.
 * XXX: Need ways of dropping planes as we descend.
 * XXX: SIMD
 */
static void
TAG(do_block_4)(struct lp_rasterizer_task *task,
                const struct lp_rast_triangle *tri,
                const struct lp_rast_plane *plane,
                int x, int y,
                const int *c)
{
   unsigned mask = 0xffff;
   int j;

   for (j = 0; j < NR_PLANES; j++) {
      mask &= ~build_mask_linear(c[j] - 1, 
				 -plane[j].dcdx,
				 plane[j].dcdy);
   }

   /* Now pass to the shader:
    */
   if (mask)
      lp_rast_shade_quads_mask(task, &tri->inputs, x, y, mask);
}

/**
 * Evaluate a 16x16 block of pixels to determine which 4x4 subblocks are in/out
 * of the triangle's bounds.
 */
static void
TAG(do_block_16)(struct lp_rasterizer_task *task,
                 const struct lp_rast_triangle *tri,
                 const struct lp_rast_plane *plane,
                 int x, int y,
                 const int *c)
{
   unsigned outmask, inmask, partmask, partial_mask;
   unsigned j;

   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   for (j = 0; j < NR_PLANES; j++) {
      const int dcdx = -plane[j].dcdx * 4;
      const int dcdy = plane[j].dcdy * 4;
      const int cox = plane[j].eo * 4;
      const int ei = plane[j].dcdy - plane[j].dcdx - plane[j].eo;
      const int cio = ei * 4 - 1;

      build_masks(c[j] + cox,
		  cio - cox,
		  dcdx, dcdy, 
		  &outmask,   /* sign bits from c[i][0..15] + cox */
		  &partmask); /* sign bits from c[i][0..15] + cio */
   }

   if (outmask == 0xffff)
      return;

   /* Mask of sub-blocks which are inside all trivial accept planes:
    */
   inmask = ~partmask & 0xffff;

   /* Mask of sub-blocks which are inside all trivial reject planes,
    * but outside at least one trivial accept plane:
    */
   partial_mask = partmask & ~outmask;

   assert((partial_mask & inmask) == 0);

   LP_COUNT_ADD(nr_empty_4, util_bitcount(0xffff & ~(partial_mask | inmask)));

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int ix = (i & 3) * 4;
      int iy = (i >> 2) * 4;
      int px = x + ix;
      int py = y + iy; 
      int cx[NR_PLANES];

      partial_mask &= ~(1 << i);

      LP_COUNT(nr_partially_covered_4);

      for (j = 0; j < NR_PLANES; j++)
         cx[j] = (c[j] 
		  - plane[j].dcdx * ix
		  + plane[j].dcdy * iy);

      TAG(do_block_4)(task, tri, plane, px, py, cx);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int ix = (i & 3) * 4;
      int iy = (i >> 2) * 4;
      int px = x + ix;
      int py = y + iy; 

      inmask &= ~(1 << i);

      LP_COUNT(nr_fully_covered_4);
      block_full_4(task, tri, px, py);
   }
}


/**
 * Scan the tile in chunks and figure out which pixels to rasterize
 * for this triangle.
 */
void
TAG(lp_rast_triangle)(struct lp_rasterizer_task *task,
                      const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_triangle *tri = arg.triangle.tri;
   unsigned plane_mask = arg.triangle.plane_mask;
   const struct lp_rast_plane *tri_plane = GET_PLANES(tri);
   const int x = task->x, y = task->y;
   struct lp_rast_plane plane[NR_PLANES];
   int c[NR_PLANES];
   unsigned outmask, inmask, partmask, partial_mask;
   unsigned j = 0;

   if (tri->inputs.disable) {
      /* This triangle was partially binned and has been disabled */
      return;
   }

   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   while (plane_mask) {
      int i = ffs(plane_mask) - 1;
      plane[j] = tri_plane[i];
      plane_mask &= ~(1 << i);
      c[j] = plane[j].c + plane[j].dcdy * y - plane[j].dcdx * x;

      {
	 const int dcdx = -plane[j].dcdx * 16;
	 const int dcdy = plane[j].dcdy * 16;
	 const int cox = plane[j].eo * 16;
         const int ei = plane[j].dcdy - plane[j].dcdx - plane[j].eo;
         const int cio = ei * 16 - 1;

	 build_masks(c[j] + cox,
		     cio - cox,
		     dcdx, dcdy, 
		     &outmask,   /* sign bits from c[i][0..15] + cox */
		     &partmask); /* sign bits from c[i][0..15] + cio */
      }

      j++;
   }

   if (outmask == 0xffff)
      return;

   /* Mask of sub-blocks which are inside all trivial accept planes:
    */
   inmask = ~partmask & 0xffff;

   /* Mask of sub-blocks which are inside all trivial reject planes,
    * but outside at least one trivial accept plane:
    */
   partial_mask = partmask & ~outmask;

   assert((partial_mask & inmask) == 0);

   LP_COUNT_ADD(nr_empty_16, util_bitcount(0xffff & ~(partial_mask | inmask)));

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int ix = (i & 3) * 16;
      int iy = (i >> 2) * 16;
      int px = x + ix;
      int py = y + iy;
      int cx[NR_PLANES];

      for (j = 0; j < NR_PLANES; j++)
         cx[j] = (c[j]
		  - plane[j].dcdx * ix
		  + plane[j].dcdy * iy);

      partial_mask &= ~(1 << i);

      LP_COUNT(nr_partially_covered_16);
      TAG(do_block_16)(task, tri, plane, px, py, cx);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int ix = (i & 3) * 16;
      int iy = (i >> 2) * 16;
      int px = x + ix;
      int py = y + iy;

      inmask &= ~(1 << i);

      LP_COUNT(nr_fully_covered_16);
      block_full_16(task, tri, px, py);
   }
}

#if defined(PIPE_ARCH_SSE) && defined(TRI_16)
/* XXX: special case this when intersection is not required.
 *      - tile completely within bbox,
 *      - bbox completely within tile.
 */
void
TRI_16(struct lp_rasterizer_task *task,
       const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_triangle *tri = arg.triangle.tri;
   const struct lp_rast_plane *plane = GET_PLANES(tri);
   unsigned mask = arg.triangle.plane_mask;
   unsigned outmask, partial_mask;
   unsigned j;
   __m128i cstep4[NR_PLANES][4];

   int x = (mask & 0xff);
   int y = (mask >> 8);

   outmask = 0;                 /* outside one or more trivial reject planes */
   
   x += task->x;
   y += task->y;

   for (j = 0; j < NR_PLANES; j++) {
      const int dcdx = -plane[j].dcdx * 4;
      const int dcdy = plane[j].dcdy * 4;
      __m128i xdcdy = _mm_set1_epi32(dcdy);

      cstep4[j][0] = _mm_setr_epi32(0, dcdx, dcdx*2, dcdx*3);
      cstep4[j][1] = _mm_add_epi32(cstep4[j][0], xdcdy);
      cstep4[j][2] = _mm_add_epi32(cstep4[j][1], xdcdy);
      cstep4[j][3] = _mm_add_epi32(cstep4[j][2], xdcdy);

      {
	 const int c = plane[j].c + plane[j].dcdy * y - plane[j].dcdx * x;
	 const int cox = plane[j].eo * 4;

	 outmask |= sign_bits4(cstep4[j], c + cox);
      }
   }

   if (outmask == 0xffff)
      return;


   /* Mask of sub-blocks which are inside all trivial reject planes,
    * but outside at least one trivial accept plane:
    */
   partial_mask = 0xffff & ~outmask;

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int ix = (i & 3) * 4;
      int iy = (i >> 2) * 4;
      int px = x + ix;
      int py = y + iy; 
      unsigned mask = 0xffff;

      partial_mask &= ~(1 << i);

      for (j = 0; j < NR_PLANES; j++) {
         const int cx = (plane[j].c - 1
			 - plane[j].dcdx * px
			 + plane[j].dcdy * py) * 4;

	 mask &= ~sign_bits4(cstep4[j], cx);
      }

      if (mask)
	 lp_rast_shade_quads_mask(task, &tri->inputs, px, py, mask);
   }
}
#endif

#if defined(PIPE_ARCH_SSE) && defined(TRI_4)
void
TRI_4(struct lp_rasterizer_task *task,
      const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_triangle *tri = arg.triangle.tri;
   const struct lp_rast_plane *plane = GET_PLANES(tri);
   unsigned mask = arg.triangle.plane_mask;
   const int x = task->x + (mask & 0xff);
   const int y = task->y + (mask >> 8);
   unsigned j;

   /* Iterate over partials:
    */
   {
      unsigned mask = 0xffff;

      for (j = 0; j < NR_PLANES; j++) {
	 const int cx = (plane[j].c 
			 - plane[j].dcdx * x
			 + plane[j].dcdy * y);

	 const int dcdx = -plane[j].dcdx;
	 const int dcdy = plane[j].dcdy;
	 __m128i xdcdy = _mm_set1_epi32(dcdy);

	 __m128i cstep0 = _mm_setr_epi32(cx, cx + dcdx, cx + dcdx*2, cx + dcdx*3);
	 __m128i cstep1 = _mm_add_epi32(cstep0, xdcdy);
	 __m128i cstep2 = _mm_add_epi32(cstep1, xdcdy);
	 __m128i cstep3 = _mm_add_epi32(cstep2, xdcdy);

	 __m128i cstep01 = _mm_packs_epi32(cstep0, cstep1);
	 __m128i cstep23 = _mm_packs_epi32(cstep2, cstep3);
	 __m128i result = _mm_packs_epi16(cstep01, cstep23);

	 /* Extract the sign bits
	  */
	 mask &= ~_mm_movemask_epi8(result);
      }

      if (mask)
	 lp_rast_shade_quads_mask(task, &tri->inputs, x, y, mask);
   }
}
#endif



#undef TAG
#undef TRI_4
#undef TRI_16
#undef NR_PLANES

