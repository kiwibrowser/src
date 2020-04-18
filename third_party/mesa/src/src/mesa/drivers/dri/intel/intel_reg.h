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

#define CMD_MI				(0x0 << 29)
#define CMD_2D				(0x2 << 29)
#define CMD_3D				(0x3 << 29)

#define MI_NOOP				(CMD_MI | 0)

#define MI_BATCH_BUFFER_END		(CMD_MI | 0xA << 23)

#define MI_FLUSH			(CMD_MI | (4 << 23))
#define FLUSH_MAP_CACHE				(1 << 0)
#define INHIBIT_FLUSH_RENDER_CACHE		(1 << 2)

#define MI_FLUSH_DW			(CMD_MI | (0x26 << 23) | 2)

/* Stalls command execution waiting for the given events to have occurred. */
#define MI_WAIT_FOR_EVENT               (CMD_MI | (0x3 << 23))
#define MI_WAIT_FOR_PLANE_B_FLIP        (1<<6)
#define MI_WAIT_FOR_PLANE_A_FLIP        (1<<2)

#define MI_STORE_REGISTER_MEM		(CMD_MI | (0x24 << 23))
# define MI_STORE_REGISTER_MEM_USE_GGTT		(1 << 22)

/* p189 */
#define _3DSTATE_LOAD_STATE_IMMEDIATE_1   (CMD_3D | (0x1d<<24) | (0x04<<16))
#define I1_LOAD_S(n)                      (1<<(4+n))

#define _3DSTATE_DRAWRECT_INFO		(CMD_3D | (0x1d<<24) | (0x80<<16) | 0x3)

/** @{
 *
 * PIPE_CONTROL operation, a combination MI_FLUSH and register write with
 * additional flushing control.
 */
#define _3DSTATE_PIPE_CONTROL		(CMD_3D | (3 << 27) | (2 << 24))
#define PIPE_CONTROL_CS_STALL		(1 << 20)
#define PIPE_CONTROL_GLOBAL_SNAPSHOT_COUNT_RESET	(1 << 19)
#define PIPE_CONTROL_TLB_INVALIDATE	(1 << 18)
#define PIPE_CONTROL_SYNC_GFDT		(1 << 17)
#define PIPE_CONTROL_MEDIA_STATE_CLEAR	(1 << 16)
#define PIPE_CONTROL_NO_WRITE		(0 << 14)
#define PIPE_CONTROL_WRITE_IMMEDIATE	(1 << 14)
#define PIPE_CONTROL_WRITE_DEPTH_COUNT	(2 << 14)
#define PIPE_CONTROL_WRITE_TIMESTAMP	(3 << 14)
#define PIPE_CONTROL_DEPTH_STALL	(1 << 13)
#define PIPE_CONTROL_WRITE_FLUSH	(1 << 12)
#define PIPE_CONTROL_INSTRUCTION_FLUSH	(1 << 11)
#define PIPE_CONTROL_TC_FLUSH		(1 << 10) /* GM45+ only */
#define PIPE_CONTROL_ISP_DIS		(1 << 9)
#define PIPE_CONTROL_INTERRUPT_ENABLE	(1 << 8)
/* GT */
#define PIPE_CONTROL_VF_CACHE_INVALIDATE	(1 << 4)
#define PIPE_CONTROL_CONST_CACHE_INVALIDATE	(1 << 3)
#define PIPE_CONTROL_STATE_CACHE_INVALIDATE	(1 << 2)
#define PIPE_CONTROL_STALL_AT_SCOREBOARD	(1 << 1)
#define PIPE_CONTROL_DEPTH_CACHE_FLUSH		(1 << 0)
#define PIPE_CONTROL_PPGTT_WRITE	(0 << 2)
#define PIPE_CONTROL_GLOBAL_GTT_WRITE	(1 << 2)

/** @} */

/** @{
 * 915 definitions
 *
 * 915 documents say that bits 31:28 and 1 are "undefined, must be zero."
 */
#define S0_VB_OFFSET_MASK		0x0ffffffc
#define S0_AUTO_CACHE_INV_DISABLE	(1<<0)
/** @} */

/** @{
 * 830 definitions
 */
#define S0_VB_OFFSET_MASK_830		0xffffff80
#define S0_VB_PITCH_SHIFT_830		1
#define S0_VB_ENABLE_830		(1<<0)
/** @} */

#define S1_VERTEX_WIDTH_SHIFT          24
#define S1_VERTEX_WIDTH_MASK           (0x3f<<24)
#define S1_VERTEX_PITCH_SHIFT          16
#define S1_VERTEX_PITCH_MASK           (0x3f<<16)

#define TEXCOORDFMT_2D                 0x0
#define TEXCOORDFMT_3D                 0x1
#define TEXCOORDFMT_4D                 0x2
#define TEXCOORDFMT_1D                 0x3
#define TEXCOORDFMT_2D_16              0x4
#define TEXCOORDFMT_4D_16              0x5
#define TEXCOORDFMT_NOT_PRESENT        0xf
#define S2_TEXCOORD_FMT0_MASK            0xf
#define S2_TEXCOORD_FMT1_SHIFT           4
#define S2_TEXCOORD_FMT(unit, type)    ((type)<<(unit*4))
#define S2_TEXCOORD_NONE               (~0)
#define S2_TEX_COUNT_SHIFT_830		12
#define S2_VERTEX_1_WIDTH_SHIFT_830	0
#define S2_VERTEX_0_WIDTH_SHIFT_830	6
/* S3 not interesting */

#define S4_POINT_WIDTH_SHIFT           23
#define S4_POINT_WIDTH_MASK            (0x1ff<<23)
#define S4_LINE_WIDTH_SHIFT            19
#define S4_LINE_WIDTH_ONE              (0x2<<19)
#define S4_LINE_WIDTH_MASK             (0xf<<19)
#define S4_FLATSHADE_ALPHA             (1<<18)
#define S4_FLATSHADE_FOG               (1<<17)
#define S4_FLATSHADE_SPECULAR          (1<<16)
#define S4_FLATSHADE_COLOR             (1<<15)
#define S4_CULLMODE_BOTH	       (0<<13)
#define S4_CULLMODE_NONE	       (1<<13)
#define S4_CULLMODE_CW		       (2<<13)
#define S4_CULLMODE_CCW		       (3<<13)
#define S4_CULLMODE_MASK	       (3<<13)
#define S4_VFMT_POINT_WIDTH            (1<<12)
#define S4_VFMT_SPEC_FOG               (1<<11)
#define S4_VFMT_COLOR                  (1<<10)
#define S4_VFMT_DEPTH_OFFSET           (1<<9)
#define S4_VFMT_XYZ     	       (1<<6)
#define S4_VFMT_XYZW     	       (2<<6)
#define S4_VFMT_XY     		       (3<<6)
#define S4_VFMT_XYW     	       (4<<6)
#define S4_VFMT_XYZW_MASK              (7<<6)
#define S4_FORCE_DEFAULT_DIFFUSE       (1<<5)
#define S4_FORCE_DEFAULT_SPECULAR      (1<<4)
#define S4_LOCAL_DEPTH_OFFSET_ENABLE   (1<<3)
#define S4_VFMT_FOG_PARAM              (1<<2)
#define S4_SPRITE_POINT_ENABLE         (1<<1)
#define S4_LINE_ANTIALIAS_ENABLE       (1<<0)

#define S4_VFMT_MASK (S4_VFMT_POINT_WIDTH   | 	\
		      S4_VFMT_SPEC_FOG      |	\
		      S4_VFMT_COLOR         |	\
		      S4_VFMT_DEPTH_OFFSET  |	\
		      S4_VFMT_XYZW_MASK     |	\
		      S4_VFMT_FOG_PARAM)


#define S5_WRITEDISABLE_ALPHA          (1<<31)
#define S5_WRITEDISABLE_RED            (1<<30)
#define S5_WRITEDISABLE_GREEN          (1<<29)
#define S5_WRITEDISABLE_BLUE           (1<<28)
#define S5_WRITEDISABLE_MASK           (0xf<<28)
#define S5_FORCE_DEFAULT_POINT_SIZE    (1<<27)
#define S5_LAST_PIXEL_ENABLE           (1<<26)
#define S5_GLOBAL_DEPTH_OFFSET_ENABLE  (1<<25)
#define S5_FOG_ENABLE                  (1<<24)
#define S5_STENCIL_REF_SHIFT           16
#define S5_STENCIL_REF_MASK            (0xff<<16)
#define S5_STENCIL_TEST_FUNC_SHIFT     13
#define S5_STENCIL_TEST_FUNC_MASK      (0x7<<13)
#define S5_STENCIL_FAIL_SHIFT          10
#define S5_STENCIL_FAIL_MASK           (0x7<<10)
#define S5_STENCIL_PASS_Z_FAIL_SHIFT   7
#define S5_STENCIL_PASS_Z_FAIL_MASK    (0x7<<7)
#define S5_STENCIL_PASS_Z_PASS_SHIFT   4
#define S5_STENCIL_PASS_Z_PASS_MASK    (0x7<<4)
#define S5_STENCIL_WRITE_ENABLE        (1<<3)
#define S5_STENCIL_TEST_ENABLE         (1<<2)
#define S5_COLOR_DITHER_ENABLE         (1<<1)
#define S5_LOGICOP_ENABLE              (1<<0)


#define S6_ALPHA_TEST_ENABLE           (1<<31)
#define S6_ALPHA_TEST_FUNC_SHIFT       28
#define S6_ALPHA_TEST_FUNC_MASK        (0x7<<28)
#define S6_ALPHA_REF_SHIFT             20
#define S6_ALPHA_REF_MASK              (0xff<<20)
#define S6_DEPTH_TEST_ENABLE           (1<<19)
#define S6_DEPTH_TEST_FUNC_SHIFT       16
#define S6_DEPTH_TEST_FUNC_MASK        (0x7<<16)
#define S6_CBUF_BLEND_ENABLE           (1<<15)
#define S6_CBUF_BLEND_FUNC_SHIFT       12
#define S6_CBUF_BLEND_FUNC_MASK        (0x7<<12)
#define S6_CBUF_SRC_BLEND_FACT_SHIFT   8
#define S6_CBUF_SRC_BLEND_FACT_MASK    (0xf<<8)
#define S6_CBUF_DST_BLEND_FACT_SHIFT   4
#define S6_CBUF_DST_BLEND_FACT_MASK    (0xf<<4)
#define S6_DEPTH_WRITE_ENABLE          (1<<3)
#define S6_COLOR_WRITE_ENABLE          (1<<2)
#define S6_TRISTRIP_PV_SHIFT           0
#define S6_TRISTRIP_PV_MASK            (0x3<<0)

#define S7_DEPTH_OFFSET_CONST_MASK     ~0

/* p143 */
#define _3DSTATE_BUF_INFO_CMD	(CMD_3D | (0x1d<<24) | (0x8e<<16) | 1)
/* Dword 1 */
#define BUF_3D_ID_COLOR_BACK	(0x3<<24)
#define BUF_3D_ID_DEPTH 	(0x7<<24)
#define BUF_3D_USE_FENCE	(1<<23)
#define BUF_3D_TILED_SURFACE	(1<<22)
#define BUF_3D_TILE_WALK_X	0
#define BUF_3D_TILE_WALK_Y	(1<<21)
#define BUF_3D_PITCH(x)         (((x)/4)<<2)
/* Dword 2 */
#define BUF_3D_ADDR(x)		((x) & ~0x3)

/* Primitive dispatch on 830-945 */
#define _3DPRIMITIVE			(CMD_3D | (0x1f << 24))
#define PRIM_INDIRECT            (1<<23)
#define PRIM_INLINE              (0<<23)
#define PRIM_INDIRECT_SEQUENTIAL (0<<17)
#define PRIM_INDIRECT_ELTS       (1<<17)

#define PRIM3D_TRILIST		(0x0<<18)
#define PRIM3D_TRISTRIP 	(0x1<<18)
#define PRIM3D_TRISTRIP_RVRSE	(0x2<<18)
#define PRIM3D_TRIFAN		(0x3<<18)
#define PRIM3D_POLY		(0x4<<18)
#define PRIM3D_LINELIST 	(0x5<<18)
#define PRIM3D_LINESTRIP	(0x6<<18)
#define PRIM3D_RECTLIST 	(0x7<<18)
#define PRIM3D_POINTLIST	(0x8<<18)
#define PRIM3D_DIB		(0x9<<18)
#define PRIM3D_MASK		(0x1f<<18)

#define XY_SETUP_BLT_CMD		(CMD_2D | (0x01 << 22) | 6)

#define XY_COLOR_BLT_CMD		(CMD_2D | (0x50 << 22) | 4)

#define XY_SRC_COPY_BLT_CMD             (CMD_2D | (0x53 << 22) | 6)

#define XY_TEXT_IMMEDIATE_BLIT_CMD	(CMD_2D | (0x31 << 22))
# define XY_TEXT_BYTE_PACKED		(1 << 16)

/* BR00 */
#define XY_BLT_WRITE_ALPHA	(1 << 21)
#define XY_BLT_WRITE_RGB	(1 << 20)
#define XY_SRC_TILED		(1 << 15)
#define XY_DST_TILED		(1 << 11)

/* BR13 */
#define BR13_8			(0x0 << 24)
#define BR13_565		(0x1 << 24)
#define BR13_8888		(0x3 << 24)

#define FENCE_LINEAR 0
#define FENCE_XMAJOR 1
#define FENCE_YMAJOR 2

#define SO_NUM_PRIM_STORAGE_NEEDED	0x2280
#define SO_PRIM_STORAGE_NEEDED0_IVB	0x5240
#define SO_PRIM_STORAGE_NEEDED1_IVB	0x5248
#define SO_PRIM_STORAGE_NEEDED2_IVB	0x5250
#define SO_PRIM_STORAGE_NEEDED3_IVB	0x5258

#define SO_NUM_PRIMS_WRITTEN		0x2288
#define SO_NUM_PRIMS_WRITTEN0_IVB	0x5200
#define SO_NUM_PRIMS_WRITTEN1_IVB	0x5208
#define SO_NUM_PRIMS_WRITTEN2_IVB	0x5210
#define SO_NUM_PRIMS_WRITTEN3_IVB	0x5218

#define TIMESTAMP                       0x2358
