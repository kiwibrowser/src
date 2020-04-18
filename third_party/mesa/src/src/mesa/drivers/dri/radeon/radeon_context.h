/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

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
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __RADEON_CONTEXT_H__
#define __RADEON_CONTEXT_H__

#include "tnl/t_vertex.h"
#include "dri_util.h"
#include "drm.h"
#include "radeon_drm.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "radeon_screen.h"

#include "radeon_common.h"


struct r100_context;
typedef struct r100_context r100ContextRec;
typedef struct r100_context *r100ContextPtr;



#define R100_TEX_ALL 0x7

/* used for both tcl_vtx and vc_frmt tex bits (they are identical) */
#define RADEON_ST_BIT(unit) \
(unit == 0 ? RADEON_CP_VC_FRMT_ST0 : (RADEON_CP_VC_FRMT_ST1 >> 2) << (2 * unit))

#define RADEON_Q_BIT(unit) \
(unit == 0 ? RADEON_CP_VC_FRMT_Q0 : (RADEON_CP_VC_FRMT_Q1 >> 2) << (2 * unit))

struct radeon_texture_env_state {
	radeonTexObjPtr texobj;
	GLenum format;
	GLenum envMode;
};

struct radeon_texture_state {
	struct radeon_texture_env_state unit[RADEON_MAX_TEXTURE_UNITS];
};

/* Trying to keep these relatively short as the variables are becoming
 * extravagently long.  Drop the driver name prefix off the front of
 * everything - I think we know which driver we're in by now, and keep the
 * prefix to 3 letters unless absolutely impossible.  
 */

#define CTX_CMD_0             0
#define CTX_PP_MISC           1
#define CTX_PP_FOG_COLOR      2
#define CTX_RE_SOLID_COLOR    3
#define CTX_RB3D_BLENDCNTL    4
#define CTX_RB3D_DEPTHOFFSET  5
#define CTX_RB3D_DEPTHPITCH   6
#define CTX_RB3D_ZSTENCILCNTL 7
#define CTX_CMD_1             8
#define CTX_PP_CNTL           9
#define CTX_RB3D_CNTL         10
#define CTX_RB3D_COLOROFFSET  11
#define CTX_CMD_2             12
#define CTX_RB3D_COLORPITCH   13
#define CTX_STATE_SIZE        14

#define SET_CMD_0               0
#define SET_SE_CNTL             1
#define SET_SE_COORDFMT         2
#define SET_CMD_1               3
#define SET_SE_CNTL_STATUS      4
#define SET_STATE_SIZE          5

#define LIN_CMD_0               0
#define LIN_RE_LINE_PATTERN     1
#define LIN_RE_LINE_STATE       2
#define LIN_CMD_1               3
#define LIN_SE_LINE_WIDTH       4
#define LIN_STATE_SIZE          5

#define MSK_CMD_0               0
#define MSK_RB3D_STENCILREFMASK 1
#define MSK_RB3D_ROPCNTL        2
#define MSK_RB3D_PLANEMASK      3
#define MSK_STATE_SIZE          4

#define VPT_CMD_0           0
#define VPT_SE_VPORT_XSCALE          1
#define VPT_SE_VPORT_XOFFSET         2
#define VPT_SE_VPORT_YSCALE          3
#define VPT_SE_VPORT_YOFFSET         4
#define VPT_SE_VPORT_ZSCALE          5
#define VPT_SE_VPORT_ZOFFSET         6
#define VPT_STATE_SIZE      7

#define MSC_CMD_0               0
#define MSC_RE_MISC             1
#define MSC_STATE_SIZE          2

#define TEX_CMD_0                   0
#define TEX_PP_TXFILTER             1
#define TEX_PP_TXFORMAT             2
#define TEX_PP_TXOFFSET             3
#define TEX_PP_TXCBLEND             4
#define TEX_PP_TXABLEND             5
#define TEX_PP_TFACTOR              6
#define TEX_CMD_1                   7
#define TEX_PP_BORDER_COLOR         8
#define TEX_STATE_SIZE              9

#define TXR_CMD_0                   0	/* rectangle textures */
#define TXR_PP_TEX_SIZE             1	/* 0x1d04, 0x1d0c for NPOT! */
#define TXR_PP_TEX_PITCH            2	/* 0x1d08, 0x1d10 for NPOT! */
#define TXR_STATE_SIZE              3

#define CUBE_CMD_0                  0
#define CUBE_PP_CUBIC_FACES         1
#define CUBE_CMD_1                  2
#define CUBE_PP_CUBIC_OFFSET_0      3
#define CUBE_PP_CUBIC_OFFSET_1      4
#define CUBE_PP_CUBIC_OFFSET_2      5
#define CUBE_PP_CUBIC_OFFSET_3      6
#define CUBE_PP_CUBIC_OFFSET_4      7
#define CUBE_STATE_SIZE             8

#define ZBS_CMD_0              0
#define ZBS_SE_ZBIAS_FACTOR             1
#define ZBS_SE_ZBIAS_CONSTANT           2
#define ZBS_STATE_SIZE         3

#define TCL_CMD_0                        0
#define TCL_OUTPUT_VTXFMT         1
#define TCL_OUTPUT_VTXSEL         2
#define TCL_MATRIX_SELECT_0       3
#define TCL_MATRIX_SELECT_1       4
#define TCL_UCP_VERT_BLEND_CTL    5
#define TCL_TEXTURE_PROC_CTL      6
#define TCL_LIGHT_MODEL_CTL       7
#define TCL_PER_LIGHT_CTL_0       8
#define TCL_PER_LIGHT_CTL_1       9
#define TCL_PER_LIGHT_CTL_2       10
#define TCL_PER_LIGHT_CTL_3       11
#define TCL_STATE_SIZE                   12

#define MTL_CMD_0            0
#define MTL_EMMISSIVE_RED    1
#define MTL_EMMISSIVE_GREEN  2
#define MTL_EMMISSIVE_BLUE   3
#define MTL_EMMISSIVE_ALPHA  4
#define MTL_AMBIENT_RED      5
#define MTL_AMBIENT_GREEN    6
#define MTL_AMBIENT_BLUE     7
#define MTL_AMBIENT_ALPHA    8
#define MTL_DIFFUSE_RED      9
#define MTL_DIFFUSE_GREEN    10
#define MTL_DIFFUSE_BLUE     11
#define MTL_DIFFUSE_ALPHA    12
#define MTL_SPECULAR_RED     13
#define MTL_SPECULAR_GREEN   14
#define MTL_SPECULAR_BLUE    15
#define MTL_SPECULAR_ALPHA   16
#define MTL_SHININESS        17
#define MTL_STATE_SIZE       18

#define VTX_CMD_0              0
#define VTX_SE_COORD_FMT       1
#define VTX_STATE_SIZE         2

#define MAT_CMD_0              0
#define MAT_ELT_0              1
#define MAT_STATE_SIZE         17

#define GRD_CMD_0                  0
#define GRD_VERT_GUARD_CLIP_ADJ    1
#define GRD_VERT_GUARD_DISCARD_ADJ 2
#define GRD_HORZ_GUARD_CLIP_ADJ    3
#define GRD_HORZ_GUARD_DISCARD_ADJ 4
#define GRD_STATE_SIZE             5

/* position changes frequently when lighting in modelpos - separate
 * out to new state item?  
 */
#define LIT_CMD_0                  0
#define LIT_AMBIENT_RED            1
#define LIT_AMBIENT_GREEN          2
#define LIT_AMBIENT_BLUE           3
#define LIT_AMBIENT_ALPHA          4
#define LIT_DIFFUSE_RED            5
#define LIT_DIFFUSE_GREEN          6
#define LIT_DIFFUSE_BLUE           7
#define LIT_DIFFUSE_ALPHA          8
#define LIT_SPECULAR_RED           9
#define LIT_SPECULAR_GREEN         10
#define LIT_SPECULAR_BLUE          11
#define LIT_SPECULAR_ALPHA         12
#define LIT_POSITION_X             13
#define LIT_POSITION_Y             14
#define LIT_POSITION_Z             15
#define LIT_POSITION_W             16
#define LIT_DIRECTION_X            17
#define LIT_DIRECTION_Y            18
#define LIT_DIRECTION_Z            19
#define LIT_DIRECTION_W            20
#define LIT_ATTEN_QUADRATIC        21
#define LIT_ATTEN_LINEAR           22
#define LIT_ATTEN_CONST            23
#define LIT_ATTEN_XXX              24
#define LIT_CMD_1                  25
#define LIT_SPOT_DCD               26
#define LIT_SPOT_EXPONENT          27
#define LIT_SPOT_CUTOFF            28
#define LIT_SPECULAR_THRESH        29
#define LIT_RANGE_CUTOFF           30	/* ? */
#define LIT_ATTEN_CONST_INV        31
#define LIT_STATE_SIZE             32

/* Fog
 */
#define FOG_CMD_0      0
#define FOG_R          1
#define FOG_C          2
#define FOG_D          3
#define FOG_PAD        4
#define FOG_STATE_SIZE 5

/* UCP
 */
#define UCP_CMD_0      0
#define UCP_X          1
#define UCP_Y          2
#define UCP_Z          3
#define UCP_W          4
#define UCP_STATE_SIZE 5

/* GLT - Global ambient
 */
#define GLT_CMD_0      0
#define GLT_RED        1
#define GLT_GREEN      2
#define GLT_BLUE       3
#define GLT_ALPHA      4
#define GLT_STATE_SIZE 5

/* EYE
 */
#define EYE_CMD_0          0
#define EYE_X              1
#define EYE_Y              2
#define EYE_Z              3
#define EYE_RESCALE_FACTOR 4
#define EYE_STATE_SIZE     5

#define SHN_CMD_0          0
#define SHN_SHININESS      1
#define SHN_STATE_SIZE     2

#define R100_QUERYOBJ_CMD_0  0
#define R100_QUERYOBJ_DATA_0 1
#define R100_QUERYOBJ_CMDSIZE  2

#define STP_CMD_0 0
#define STP_DATA_0 1
#define STP_CMD_1 2
#define STP_STATE_SIZE 35

struct r100_hw_state {
	/* Hardware state, stored as cmdbuf commands:  
	 *   -- Need to doublebuffer for
	 *           - eliding noop statechange loops? (except line stipple count)
	 */
	struct radeon_state_atom ctx;
	struct radeon_state_atom set;
	struct radeon_state_atom lin;
	struct radeon_state_atom msk;
	struct radeon_state_atom vpt;
	struct radeon_state_atom tcl;
	struct radeon_state_atom msc;
	struct radeon_state_atom tex[3];
	struct radeon_state_atom cube[3];
	struct radeon_state_atom zbs;
	struct radeon_state_atom mtl;
	struct radeon_state_atom mat[6];
	struct radeon_state_atom lit[8];	/* includes vec, scl commands */
	struct radeon_state_atom ucp[6];
	struct radeon_state_atom eye;	/* eye pos */
	struct radeon_state_atom grd;	/* guard band clipping */
	struct radeon_state_atom fog;
	struct radeon_state_atom glt;
	struct radeon_state_atom txr[3];	/* for NPOT */
	struct radeon_state_atom stp;
};

struct radeon_stipple_state {
	GLuint mask[32];
};

struct r100_state {
	struct radeon_stipple_state stipple;
	struct radeon_texture_state texture;
};

#define RADEON_CMD_BUF_SZ  (8*1024)
#define R200_ELT_BUF_SZ  (8*1024)
/* radeon_tcl.c
 */
struct r100_tcl_info {
	GLuint vertex_format;
	GLuint hw_primitive;

	/* Temporary for cases where incoming vertex data is incompatible
	 * with maos code.
	 */
	GLvector4f ObjClean;

	GLuint *Elts;

        int elt_cmd_offset;
	int elt_cmd_start;
        int elt_used;
};

/* radeon_swtcl.c
 */
struct r100_swtcl_info {
	GLuint vertex_format;

	GLubyte *verts;

	/* Fallback rasterization functions
	 */
	radeon_point_func draw_point;
	radeon_line_func draw_line;
	radeon_tri_func draw_tri;

   /**
    * Offset of the 4UB color data within a hardware (swtcl) vertex.
    */
	GLuint coloroffset;

   /**
    * Offset of the 3UB specular color data within a hardware (swtcl) vertex.
    */
	GLuint specoffset;

	GLboolean needproj;
};



/* A maximum total of 20 elements per vertex:  3 floats for position, 3
 * floats for normal, 4 floats for color, 4 bytes for secondary color,
 * 3 floats for each texture unit (9 floats total).
 * 
 * The position data is never actually stored here, so 3 elements could be
 * trimmed out of the buffer. This number is only valid for vtxfmt!
 */
#define RADEON_MAX_VERTEX_SIZE 20

struct r100_context {
        struct radeon_context radeon;

	/* Driver and hardware state management
	 */
	struct r100_hw_state hw;
	struct r100_state state;

	/* Vertex buffers
	 */
	struct radeon_ioctl ioctl;
	struct radeon_store store;

	/* TCL stuff
	 */
	GLmatrix TexGenMatrix[RADEON_MAX_TEXTURE_UNITS];
	GLboolean recheck_texgen[RADEON_MAX_TEXTURE_UNITS];
	GLboolean TexGenNeedNormals[RADEON_MAX_TEXTURE_UNITS];
	GLuint TexGenEnabled;
	GLuint NeedTexMatrix;
	GLuint TexMatColSwap;
	GLmatrix tmpmat[RADEON_MAX_TEXTURE_UNITS];
	GLuint last_ReallyEnabled;

	/* radeon_tcl.c
	 */
	struct r100_tcl_info tcl;

	/* radeon_swtcl.c
	 */
	struct r100_swtcl_info swtcl;

	GLboolean using_hyperz;
	GLboolean texmicrotile;

	/* Performance counters
	 */
	GLuint boxes;		/* Draw performance boxes */
	GLuint hardwareWentIdle;
	GLuint c_clears;
	GLuint c_drawWaits;
	GLuint c_textureSwaps;
	GLuint c_textureBytes;
	GLuint c_vertexBuffers;

};


#define R100_CONTEXT(ctx)		((r100ContextPtr)(ctx->DriverCtx))


#define RADEON_OLD_PACKETS 1

extern GLboolean r100CreateContext( gl_api api,
				    const struct gl_config *glVisual,
				    __DRIcontext *driContextPriv,
				    unsigned major_version,
				    unsigned minor_version,
				    uint32_t flags,
				    unsigned *error,
				    void *sharedContextPrivate);


#endif				/* __RADEON_CONTEXT_H__ */
