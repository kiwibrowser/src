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

#ifndef __R200_CONTEXT_H__
#define __R200_CONTEXT_H__

#include "tnl/t_vertex.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"

#include "main/macros.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "r200_reg.h"
#include "r200_vertprog.h"

#ifndef R200_EMIT_VAP_PVS_CNTL
#error This driver requires a newer libdrm to compile
#endif

#include "radeon_screen.h"
#include "radeon_common.h"

struct r200_context;
typedef struct r200_context r200ContextRec;
typedef struct r200_context *r200ContextPtr;

#include "main/mm.h"

struct r200_vertex_program {
        struct gl_vertex_program mesa_program; /* Must be first */
        int translated;
        /* need excess instr: 1 for late loop checking, 2 for 
           additional instr due to instr/attr, 3 for fog */
        VERTEX_SHADER_INSTRUCTION instr[R200_VSF_MAX_INST + 6];
        int pos_end;
        int inputs[VERT_ATTRIB_MAX];
        GLubyte inputmap_rev[16];
        int native;
        int fogpidx;
        int fogmode;
};

#define R200_TEX_ALL 0x3f


struct r200_texture_env_state {
   radeonTexObjPtr texobj;
   GLuint outputreg;
   GLuint unitneeded;
};

#define R200_MAX_TEXTURE_UNITS 6

struct r200_texture_state {
   struct r200_texture_env_state unit[R200_MAX_TEXTURE_UNITS];
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
#define CTX_CMD_2             12 /* why */
#define CTX_RB3D_COLORPITCH   13 /* why */
#define CTX_STATE_SIZE_OLDDRM 14
#define CTX_CMD_3             14
#define CTX_RB3D_BLENDCOLOR   15
#define CTX_RB3D_ABLENDCNTL   16
#define CTX_RB3D_CBLENDCNTL   17
#define CTX_STATE_SIZE_NEWDRM 18

#define SET_CMD_0               0
#define SET_SE_CNTL             1
#define SET_RE_CNTL             2 /* replace se_coord_fmt */
#define SET_STATE_SIZE          3

#define VTE_CMD_0               0
#define VTE_SE_VTE_CNTL         1
#define VTE_STATE_SIZE          2

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

#define ZBS_CMD_0               0
#define ZBS_SE_ZBIAS_FACTOR     1
#define ZBS_SE_ZBIAS_CONSTANT   2
#define ZBS_STATE_SIZE          3

#define MSC_CMD_0               0
#define MSC_RE_MISC             1
#define MSC_STATE_SIZE          2

#define TAM_CMD_0               0
#define TAM_DEBUG3              1
#define TAM_STATE_SIZE          2

#define TEX_CMD_0                   0
#define TEX_PP_TXFILTER             1  /*2c00*/
#define TEX_PP_TXFORMAT             2  /*2c04*/
#define TEX_PP_TXFORMAT_X           3  /*2c08*/
#define TEX_PP_TXSIZE               4  /*2c0c*/
#define TEX_PP_TXPITCH              5  /*2c10*/
#define TEX_PP_BORDER_COLOR         6  /*2c14*/
#define TEX_CMD_1_OLDDRM            7
#define TEX_PP_TXOFFSET_OLDDRM      8  /*2d00 */
#define TEX_STATE_SIZE_OLDDRM       9
#define TEX_PP_CUBIC_FACES          7
#define TEX_PP_TXMULTI_CTL          8
#define TEX_CMD_1_NEWDRM            9
#define TEX_PP_TXOFFSET_NEWDRM     10
#define TEX_STATE_SIZE_NEWDRM      11

#define CUBE_CMD_0                  0  /* 1 register follows */ /* this command unnecessary */
#define CUBE_PP_CUBIC_FACES         1  /* 0x2c18 */             /* with new enough drm */
#define CUBE_CMD_1                  2  /* 5 registers follow */
#define CUBE_PP_CUBIC_OFFSET_F1     3  /* 0x2d04 */
#define CUBE_PP_CUBIC_OFFSET_F2     4  /* 0x2d08 */
#define CUBE_PP_CUBIC_OFFSET_F3     5  /* 0x2d0c */
#define CUBE_PP_CUBIC_OFFSET_F4     6  /* 0x2d10 */
#define CUBE_PP_CUBIC_OFFSET_F5     7  /* 0x2d14 */
#define CUBE_STATE_SIZE             8

#define PIX_CMD_0                   0
#define PIX_PP_TXCBLEND             1
#define PIX_PP_TXCBLEND2            2
#define PIX_PP_TXABLEND             3
#define PIX_PP_TXABLEND2            4
#define PIX_STATE_SIZE              5

#define TF_CMD_0                    0
#define TF_TFACTOR_0                1
#define TF_TFACTOR_1                2
#define TF_TFACTOR_2                3
#define TF_TFACTOR_3                4
#define TF_TFACTOR_4                5
#define TF_TFACTOR_5                6
#define TF_STATE_SIZE               7

#define ATF_CMD_0                   0
#define ATF_TFACTOR_0               1
#define ATF_TFACTOR_1               2
#define ATF_TFACTOR_2               3
#define ATF_TFACTOR_3               4
#define ATF_TFACTOR_4               5
#define ATF_TFACTOR_5               6
#define ATF_TFACTOR_6               7
#define ATF_TFACTOR_7               8
#define ATF_STATE_SIZE              9

/* ATI_FRAGMENT_SHADER */
#define AFS_CMD_0                 0
#define AFS_IC0                   1 /* 2f00 */
#define AFS_IC1                   2 /* 2f04 */
#define AFS_IA0                   3 /* 2f08 */
#define AFS_IA1                   4 /* 2f0c */
#define AFS_STATE_SIZE           33

#define PVS_CMD_0                 0
#define PVS_CNTL_1                1
#define PVS_CNTL_2                2
#define PVS_STATE_SIZE            3

/* those are quite big... */
#define VPI_CMD_0                 0
#define VPI_OPDST_0               1
#define VPI_SRC0_0                2
#define VPI_SRC1_0                3
#define VPI_SRC2_0                4
#define VPI_OPDST_63              253
#define VPI_SRC0_63               254
#define VPI_SRC1_63               255
#define VPI_SRC2_63               256
#define VPI_STATE_SIZE            257

#define VPP_CMD_0                0
#define VPP_PARAM0_0             1
#define VPP_PARAM1_0             2
#define VPP_PARAM2_0             3
#define VPP_PARAM3_0             4
#define VPP_PARAM0_95            381
#define VPP_PARAM1_95            382
#define VPP_PARAM2_95            383
#define VPP_PARAM3_95            384
#define VPP_STATE_SIZE           385

#define TCL_CMD_0                 0
#define TCL_LIGHT_MODEL_CTL_0     1
#define TCL_LIGHT_MODEL_CTL_1     2
#define TCL_PER_LIGHT_CTL_0       3
#define TCL_PER_LIGHT_CTL_1       4
#define TCL_PER_LIGHT_CTL_2       5
#define TCL_PER_LIGHT_CTL_3       6
#define TCL_CMD_1                 7
#define TCL_UCP_VERT_BLEND_CTL    8
#define TCL_STATE_SIZE            9

#define MSL_CMD_0                     0
#define MSL_MATRIX_SELECT_0           1
#define MSL_MATRIX_SELECT_1           2
#define MSL_MATRIX_SELECT_2           3
#define MSL_MATRIX_SELECT_3           4
#define MSL_MATRIX_SELECT_4           5
#define MSL_STATE_SIZE                6

#define TCG_CMD_0                 0
#define TCG_TEX_PROC_CTL_2            1
#define TCG_TEX_PROC_CTL_3            2
#define TCG_TEX_PROC_CTL_0            3
#define TCG_TEX_PROC_CTL_1            4
#define TCG_TEX_CYL_WRAP_CTL      5
#define TCG_STATE_SIZE            6

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
#define MTL_CMD_1            17
#define MTL_SHININESS        18
#define MTL_STATE_SIZE       19

#define VAP_CMD_0                   0
#define VAP_SE_VAP_CNTL             1
#define VAP_STATE_SIZE              2

/* Replaces a lot of packet info from radeon
 */
#define VTX_CMD_0                   0
#define VTX_VTXFMT_0            1
#define VTX_VTXFMT_1            2
#define VTX_TCL_OUTPUT_VTXFMT_0 3
#define VTX_TCL_OUTPUT_VTXFMT_1 4
#define VTX_CMD_1               5
#define VTX_TCL_OUTPUT_COMPSEL  6
#define VTX_CMD_2               7
#define VTX_STATE_CNTL          8
#define VTX_STATE_SIZE          9

/* SPR - point sprite state
 */
#define SPR_CMD_0              0
#define SPR_POINT_SPRITE_CNTL  1
#define SPR_STATE_SIZE         2

#define PTP_CMD_0              0
#define PTP_VPORT_SCALE_0      1
#define PTP_VPORT_SCALE_1      2
#define PTP_VPORT_SCALE_PTSIZE 3
#define PTP_VPORT_SCALE_3      4
#define PTP_CMD_1              5
#define PTP_ATT_CONST_QUAD     6
#define PTP_ATT_CONST_LIN      7
#define PTP_ATT_CONST_CON      8
#define PTP_ATT_CONST_3        9
#define PTP_EYE_X             10
#define PTP_EYE_Y             11
#define PTP_EYE_Z             12
#define PTP_EYE_3             13
#define PTP_CLAMP_MIN         14
#define PTP_CLAMP_MAX         15
#define PTP_CLAMP_2           16
#define PTP_CLAMP_3           17
#define PTP_STATE_SIZE        18

#define VTX_COLOR(v,n)   (((v)>>(R200_VTX_COLOR_0_SHIFT+(n)*2))&\
                         R200_VTX_COLOR_MASK)

/**
 * Given the \c R200_SE_VTX_FMT_1 for the current vertex state, determine
 * how many components are in texture coordinate \c n.
 */
#define VTX_TEXn_COUNT(v,n)   (((v) >> (3 * n)) & 0x07)

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
#define LIT_SPOT_DCM               27
#define LIT_SPOT_EXPONENT          28
#define LIT_SPOT_CUTOFF            29
#define LIT_SPECULAR_THRESH        30
#define LIT_RANGE_CUTOFF           31 /* ? */
#define LIT_ATTEN_CONST_INV        32
#define LIT_STATE_SIZE             33

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

/* CST - constant state
 */
#define CST_CMD_0                             0
#define CST_PP_CNTL_X                         1
#define CST_CMD_1                             2
#define CST_RB3D_DEPTHXY_OFFSET               3
#define CST_CMD_2                             4
#define CST_RE_AUX_SCISSOR_CNTL               5
#define CST_CMD_4                             6
#define CST_SE_VAP_CNTL_STATUS                7
#define CST_CMD_5                             8
#define CST_RE_POINTSIZE                      9
#define CST_CMD_6                             10
#define CST_SE_TCL_INPUT_VTX_0                11
#define CST_SE_TCL_INPUT_VTX_1                12
#define CST_SE_TCL_INPUT_VTX_2                13
#define CST_SE_TCL_INPUT_VTX_3                14
#define CST_STATE_SIZE                        15

#define PRF_CMD_0         0
#define PRF_PP_TRI_PERF   1
#define PRF_PP_PERF_CNTL  2
#define PRF_STATE_SIZE    3


#define SCI_CMD_1         0
#define SCI_XY_1          1
#define SCI_CMD_2         2
#define SCI_XY_2          3
#define SCI_STATE_SIZE    4

#define R200_QUERYOBJ_CMD_0  0
#define R200_QUERYOBJ_DATA_0 1
#define R200_QUERYOBJ_CMDSIZE  2

#define STP_CMD_0 0
#define STP_DATA_0 1
#define STP_CMD_1 2
#define STP_STATE_SIZE 35

struct r200_hw_state {
   /* Hardware state, stored as cmdbuf commands:  
    *   -- Need to doublebuffer for
    *           - reviving state after loss of context
    *           - eliding noop statechange loops? (except line stipple count)
    */
   struct radeon_state_atom ctx;
   struct radeon_state_atom set;
   struct radeon_state_atom sci;
   struct radeon_state_atom vte;
   struct radeon_state_atom lin;
   struct radeon_state_atom msk;
   struct radeon_state_atom vpt;
   struct radeon_state_atom vap;
   struct radeon_state_atom vtx;
   struct radeon_state_atom tcl;
   struct radeon_state_atom msl;
   struct radeon_state_atom tcg;
   struct radeon_state_atom msc;
   struct radeon_state_atom cst;
   struct radeon_state_atom tam;
   struct radeon_state_atom tf;
   struct radeon_state_atom tex[6];
   struct radeon_state_atom cube[6];
   struct radeon_state_atom zbs;
   struct radeon_state_atom mtl[2];
   struct radeon_state_atom mat[9];
   struct radeon_state_atom lit[8]; /* includes vec, scl commands */
   struct radeon_state_atom ucp[6];
   struct radeon_state_atom pix[6]; /* pixshader stages */
   struct radeon_state_atom eye; /* eye pos */
   struct radeon_state_atom grd; /* guard band clipping */
   struct radeon_state_atom fog;
   struct radeon_state_atom glt;
   struct radeon_state_atom prf;
   struct radeon_state_atom afs[2];
   struct radeon_state_atom pvs;
   struct radeon_state_atom vpi[2];
   struct radeon_state_atom vpp[2];
   struct radeon_state_atom atf;
   struct radeon_state_atom spr;
   struct radeon_state_atom ptp;
   struct radeon_state_atom stp;
};

struct r200_state {
   /* Derived state for internal purposes:
    */
   struct r200_texture_state texture;
   GLuint envneeded;
};

#define R200_CMD_BUF_SZ  (16*1024) 

#define R200_ELT_BUF_SZ  (16*1024) 
/* r200_tcl.c
 */
struct r200_tcl_info {
   GLuint hw_primitive;

   int elt_used;

};


/* r200_swtcl.c
 */
struct r200_swtcl_info {


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

   /**
    * Should Mesa project vertex data or will the hardware do it?
    */
   GLboolean needproj;
};




   /* A maximum total of 29 elements per vertex:  3 floats for position, 3
    * floats for normal, 4 floats for color, 4 bytes for secondary color,
    * 3 floats for each texture unit (18 floats total).
    * 
    * we maybe need add. 4 to prevent segfault if someone specifies
    * GL_TEXTURE6/GL_TEXTURE7 (esp. for the codegen-path) (FIXME: )
    * 
    * The position data is never actually stored here, so 3 elements could be
    * trimmed out of the buffer.
    */

#define R200_MAX_VERTEX_SIZE ((3*6)+11)

struct r200_context {
   struct radeon_context radeon;

   /* Driver and hardware state management
    */
   struct r200_hw_state hw;
   struct r200_state state;
   struct r200_vertex_program *curr_vp_hw;

   /* Vertex buffers
    */
   struct radeon_ioctl ioctl;
   struct radeon_store store;

   /* Clientdata textures;
    */
   GLuint prefer_gart_client_texturing;

   /* TCL stuff
    */
   GLmatrix TexGenMatrix[R200_MAX_TEXTURE_UNITS];
   GLboolean recheck_texgen[R200_MAX_TEXTURE_UNITS];
   GLboolean TexGenNeedNormals[R200_MAX_TEXTURE_UNITS];
   GLuint TexMatEnabled;
   GLuint TexMatCompSel;
   GLuint TexGenEnabled;
   GLuint TexGenCompSel;
   GLmatrix tmpmat;

   /* r200_tcl.c
    */
   struct r200_tcl_info tcl;

   /* r200_swtcl.c
    */
   struct r200_swtcl_info swtcl;

   GLboolean using_hyperz;
   GLboolean texmicrotile;

  struct ati_fragment_shader *afs_loaded;
};

#define R200_CONTEXT(ctx)		((r200ContextPtr)(ctx->DriverCtx))


extern void r200DestroyContext( __DRIcontext *driContextPriv );
extern GLboolean r200CreateContext( gl_api api,
				    const struct gl_config *glVisual,
				    __DRIcontext *driContextPriv,
				    unsigned major_version,
				    unsigned minor_version,
				    uint32_t flags,
				    unsigned *error,
				    void *sharedContextPrivate);
extern GLboolean r200MakeCurrent( __DRIcontext *driContextPriv,
				  __DRIdrawable *driDrawPriv,
				  __DRIdrawable *driReadPriv );
extern GLboolean r200UnbindContext( __DRIcontext *driContextPriv );

extern void r200_init_texcopy_functions(struct dd_function_table *table);

/* ================================================================
 * Debugging:
 */

#define R200_DEBUG RADEON_DEBUG



#endif /* __R200_CONTEXT_H__ */
