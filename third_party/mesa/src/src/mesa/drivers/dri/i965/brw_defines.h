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

#define INTEL_MASK(high, low) (((1<<((high)-(low)+1))-1)<<(low))
#define SET_FIELD(value, field) (((value) << field ## _SHIFT) & field ## _MASK)
#define GET_FIELD(word, field) (((word)  & field ## _MASK) >> field ## _SHIFT)

#ifndef BRW_DEFINES_H
#define BRW_DEFINES_H

/* 3D state:
 */
#define PIPE_CONTROL_NOWRITE          0x00
#define PIPE_CONTROL_WRITEIMMEDIATE   0x01
#define PIPE_CONTROL_WRITEDEPTH       0x02
#define PIPE_CONTROL_WRITETIMESTAMP   0x03

#define PIPE_CONTROL_GTTWRITE_PROCESS_LOCAL 0x00
#define PIPE_CONTROL_GTTWRITE_GLOBAL        0x01

#define CMD_3D_PRIM                                 0x7b00 /* 3DPRIMITIVE */
/* DW0 */
# define GEN4_3DPRIM_TOPOLOGY_TYPE_SHIFT            10
# define GEN4_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL (0 << 15)
# define GEN4_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM     (1 << 15)
/* DW1 */
# define GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL (0 << 8)
# define GEN7_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM     (1 << 8)

#define _3DPRIM_POINTLIST         0x01
#define _3DPRIM_LINELIST          0x02
#define _3DPRIM_LINESTRIP         0x03
#define _3DPRIM_TRILIST           0x04
#define _3DPRIM_TRISTRIP          0x05
#define _3DPRIM_TRIFAN            0x06
#define _3DPRIM_QUADLIST          0x07
#define _3DPRIM_QUADSTRIP         0x08
#define _3DPRIM_LINELIST_ADJ      0x09
#define _3DPRIM_LINESTRIP_ADJ     0x0A
#define _3DPRIM_TRILIST_ADJ       0x0B
#define _3DPRIM_TRISTRIP_ADJ      0x0C
#define _3DPRIM_TRISTRIP_REVERSE  0x0D
#define _3DPRIM_POLYGON           0x0E
#define _3DPRIM_RECTLIST          0x0F
#define _3DPRIM_LINELOOP          0x10
#define _3DPRIM_POINTLIST_BF      0x11
#define _3DPRIM_LINESTRIP_CONT    0x12
#define _3DPRIM_LINESTRIP_BF      0x13
#define _3DPRIM_LINESTRIP_CONT_BF 0x14
#define _3DPRIM_TRIFAN_NOSTIPPLE  0x15

#define BRW_ANISORATIO_2     0 
#define BRW_ANISORATIO_4     1 
#define BRW_ANISORATIO_6     2 
#define BRW_ANISORATIO_8     3 
#define BRW_ANISORATIO_10    4 
#define BRW_ANISORATIO_12    5 
#define BRW_ANISORATIO_14    6 
#define BRW_ANISORATIO_16    7

#define BRW_BLENDFACTOR_ONE                 0x1
#define BRW_BLENDFACTOR_SRC_COLOR           0x2
#define BRW_BLENDFACTOR_SRC_ALPHA           0x3
#define BRW_BLENDFACTOR_DST_ALPHA           0x4
#define BRW_BLENDFACTOR_DST_COLOR           0x5
#define BRW_BLENDFACTOR_SRC_ALPHA_SATURATE  0x6
#define BRW_BLENDFACTOR_CONST_COLOR         0x7
#define BRW_BLENDFACTOR_CONST_ALPHA         0x8
#define BRW_BLENDFACTOR_SRC1_COLOR          0x9
#define BRW_BLENDFACTOR_SRC1_ALPHA          0x0A
#define BRW_BLENDFACTOR_ZERO                0x11
#define BRW_BLENDFACTOR_INV_SRC_COLOR       0x12
#define BRW_BLENDFACTOR_INV_SRC_ALPHA       0x13
#define BRW_BLENDFACTOR_INV_DST_ALPHA       0x14
#define BRW_BLENDFACTOR_INV_DST_COLOR       0x15
#define BRW_BLENDFACTOR_INV_CONST_COLOR     0x17
#define BRW_BLENDFACTOR_INV_CONST_ALPHA     0x18
#define BRW_BLENDFACTOR_INV_SRC1_COLOR      0x19
#define BRW_BLENDFACTOR_INV_SRC1_ALPHA      0x1A

#define BRW_BLENDFUNCTION_ADD               0
#define BRW_BLENDFUNCTION_SUBTRACT          1
#define BRW_BLENDFUNCTION_REVERSE_SUBTRACT  2
#define BRW_BLENDFUNCTION_MIN               3
#define BRW_BLENDFUNCTION_MAX               4

#define BRW_ALPHATEST_FORMAT_UNORM8         0
#define BRW_ALPHATEST_FORMAT_FLOAT32        1

#define BRW_CHROMAKEY_KILL_ON_ANY_MATCH  0
#define BRW_CHROMAKEY_REPLACE_BLACK      1

#define BRW_CLIP_API_OGL     0
#define BRW_CLIP_API_DX      1

#define BRW_CLIPMODE_NORMAL              0
#define BRW_CLIPMODE_CLIP_ALL            1
#define BRW_CLIPMODE_CLIP_NON_REJECTED   2
#define BRW_CLIPMODE_REJECT_ALL          3
#define BRW_CLIPMODE_ACCEPT_ALL          4
#define BRW_CLIPMODE_KERNEL_CLIP         5

#define BRW_CLIP_NDCSPACE     0
#define BRW_CLIP_SCREENSPACE  1

#define BRW_COMPAREFUNCTION_ALWAYS       0
#define BRW_COMPAREFUNCTION_NEVER        1
#define BRW_COMPAREFUNCTION_LESS         2
#define BRW_COMPAREFUNCTION_EQUAL        3
#define BRW_COMPAREFUNCTION_LEQUAL       4
#define BRW_COMPAREFUNCTION_GREATER      5
#define BRW_COMPAREFUNCTION_NOTEQUAL     6
#define BRW_COMPAREFUNCTION_GEQUAL       7

#define BRW_COVERAGE_PIXELS_HALF     0
#define BRW_COVERAGE_PIXELS_1        1
#define BRW_COVERAGE_PIXELS_2        2
#define BRW_COVERAGE_PIXELS_4        3

#define BRW_CULLMODE_BOTH        0
#define BRW_CULLMODE_NONE        1
#define BRW_CULLMODE_FRONT       2
#define BRW_CULLMODE_BACK        3

#define BRW_DEFAULTCOLOR_R8G8B8A8_UNORM      0
#define BRW_DEFAULTCOLOR_R32G32B32A32_FLOAT  1

#define BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT     0
#define BRW_DEPTHFORMAT_D32_FLOAT                1
#define BRW_DEPTHFORMAT_D24_UNORM_S8_UINT        2
#define BRW_DEPTHFORMAT_D24_UNORM_X8_UINT        3 /* GEN5 */
#define BRW_DEPTHFORMAT_D16_UNORM                5

#define BRW_FLOATING_POINT_IEEE_754        0
#define BRW_FLOATING_POINT_NON_IEEE_754    1

#define BRW_FRONTWINDING_CW      0
#define BRW_FRONTWINDING_CCW     1

#define BRW_SPRITE_POINT_ENABLE  16

#define BRW_CUT_INDEX_ENABLE     (1 << 10)

#define BRW_INDEX_BYTE     0
#define BRW_INDEX_WORD     1
#define BRW_INDEX_DWORD    2

#define BRW_LOGICOPFUNCTION_CLEAR            0
#define BRW_LOGICOPFUNCTION_NOR              1
#define BRW_LOGICOPFUNCTION_AND_INVERTED     2
#define BRW_LOGICOPFUNCTION_COPY_INVERTED    3
#define BRW_LOGICOPFUNCTION_AND_REVERSE      4
#define BRW_LOGICOPFUNCTION_INVERT           5
#define BRW_LOGICOPFUNCTION_XOR              6
#define BRW_LOGICOPFUNCTION_NAND             7
#define BRW_LOGICOPFUNCTION_AND              8
#define BRW_LOGICOPFUNCTION_EQUIV            9
#define BRW_LOGICOPFUNCTION_NOOP             10
#define BRW_LOGICOPFUNCTION_OR_INVERTED      11
#define BRW_LOGICOPFUNCTION_COPY             12
#define BRW_LOGICOPFUNCTION_OR_REVERSE       13
#define BRW_LOGICOPFUNCTION_OR               14
#define BRW_LOGICOPFUNCTION_SET              15  

#define BRW_MAPFILTER_NEAREST        0x0 
#define BRW_MAPFILTER_LINEAR         0x1 
#define BRW_MAPFILTER_ANISOTROPIC    0x2

#define BRW_MIPFILTER_NONE        0   
#define BRW_MIPFILTER_NEAREST     1   
#define BRW_MIPFILTER_LINEAR      3

#define BRW_ADDRESS_ROUNDING_ENABLE_U_MAG	0x20
#define BRW_ADDRESS_ROUNDING_ENABLE_U_MIN	0x10
#define BRW_ADDRESS_ROUNDING_ENABLE_V_MAG	0x08
#define BRW_ADDRESS_ROUNDING_ENABLE_V_MIN	0x04
#define BRW_ADDRESS_ROUNDING_ENABLE_R_MAG	0x02
#define BRW_ADDRESS_ROUNDING_ENABLE_R_MIN	0x01

#define BRW_POLYGON_FRONT_FACING     0
#define BRW_POLYGON_BACK_FACING      1

#define BRW_PREFILTER_ALWAYS     0x0 
#define BRW_PREFILTER_NEVER      0x1
#define BRW_PREFILTER_LESS       0x2
#define BRW_PREFILTER_EQUAL      0x3
#define BRW_PREFILTER_LEQUAL     0x4
#define BRW_PREFILTER_GREATER    0x5
#define BRW_PREFILTER_NOTEQUAL   0x6
#define BRW_PREFILTER_GEQUAL     0x7

#define BRW_PROVOKING_VERTEX_0    0
#define BRW_PROVOKING_VERTEX_1    1 
#define BRW_PROVOKING_VERTEX_2    2

#define BRW_RASTRULE_UPPER_LEFT  0    
#define BRW_RASTRULE_UPPER_RIGHT 1
/* These are listed as "Reserved, but not seen as useful"
 * in Intel documentation (page 212, "Point Rasterization Rule",
 * section 7.4 "SF Pipeline State Summary", of document
 * "Intel® 965 Express Chipset Family and Intel® G35 Express
 * Chipset Graphics Controller Programmer's Reference Manual,
 * Volume 2: 3D/Media", Revision 1.0b as of January 2008,
 * available at 
 *     http://intellinuxgraphics.org/documentation.html
 * at the time of this writing).
 *
 * These appear to be supported on at least some
 * i965-family devices, and the BRW_RASTRULE_LOWER_RIGHT
 * is useful when using OpenGL to render to a FBO
 * (which has the pixel coordinate Y orientation inverted
 * with respect to the normal OpenGL pixel coordinate system).
 */
#define BRW_RASTRULE_LOWER_LEFT  2
#define BRW_RASTRULE_LOWER_RIGHT 3

#define BRW_RENDERTARGET_CLAMPRANGE_UNORM    0
#define BRW_RENDERTARGET_CLAMPRANGE_SNORM    1
#define BRW_RENDERTARGET_CLAMPRANGE_FORMAT   2

#define BRW_STENCILOP_KEEP               0
#define BRW_STENCILOP_ZERO               1
#define BRW_STENCILOP_REPLACE            2
#define BRW_STENCILOP_INCRSAT            3
#define BRW_STENCILOP_DECRSAT            4
#define BRW_STENCILOP_INCR               5
#define BRW_STENCILOP_DECR               6
#define BRW_STENCILOP_INVERT             7

/* Surface state DW0 */
#define BRW_SURFACE_RC_READ_WRITE	(1 << 8)
#define BRW_SURFACE_MIPLAYOUT_SHIFT	10
#define BRW_SURFACE_MIPMAPLAYOUT_BELOW   0
#define BRW_SURFACE_MIPMAPLAYOUT_RIGHT   1
#define BRW_SURFACE_CUBEFACE_ENABLES	0x3f
#define BRW_SURFACE_BLEND_ENABLED	(1 << 13)
#define BRW_SURFACE_WRITEDISABLE_B_SHIFT	14
#define BRW_SURFACE_WRITEDISABLE_G_SHIFT	15
#define BRW_SURFACE_WRITEDISABLE_R_SHIFT	16
#define BRW_SURFACE_WRITEDISABLE_A_SHIFT	17

#define BRW_SURFACEFORMAT_R32G32B32A32_FLOAT             0x000 
#define BRW_SURFACEFORMAT_R32G32B32A32_SINT              0x001 
#define BRW_SURFACEFORMAT_R32G32B32A32_UINT              0x002 
#define BRW_SURFACEFORMAT_R32G32B32A32_UNORM             0x003 
#define BRW_SURFACEFORMAT_R32G32B32A32_SNORM             0x004 
#define BRW_SURFACEFORMAT_R64G64_FLOAT                   0x005 
#define BRW_SURFACEFORMAT_R32G32B32X32_FLOAT             0x006 
#define BRW_SURFACEFORMAT_R32G32B32A32_SSCALED           0x007
#define BRW_SURFACEFORMAT_R32G32B32A32_USCALED           0x008
#define BRW_SURFACEFORMAT_R32G32B32_FLOAT                0x040 
#define BRW_SURFACEFORMAT_R32G32B32_SINT                 0x041 
#define BRW_SURFACEFORMAT_R32G32B32_UINT                 0x042 
#define BRW_SURFACEFORMAT_R32G32B32_UNORM                0x043 
#define BRW_SURFACEFORMAT_R32G32B32_SNORM                0x044 
#define BRW_SURFACEFORMAT_R32G32B32_SSCALED              0x045 
#define BRW_SURFACEFORMAT_R32G32B32_USCALED              0x046 
#define BRW_SURFACEFORMAT_R16G16B16A16_UNORM             0x080 
#define BRW_SURFACEFORMAT_R16G16B16A16_SNORM             0x081 
#define BRW_SURFACEFORMAT_R16G16B16A16_SINT              0x082 
#define BRW_SURFACEFORMAT_R16G16B16A16_UINT              0x083 
#define BRW_SURFACEFORMAT_R16G16B16A16_FLOAT             0x084 
#define BRW_SURFACEFORMAT_R32G32_FLOAT                   0x085 
#define BRW_SURFACEFORMAT_R32G32_SINT                    0x086 
#define BRW_SURFACEFORMAT_R32G32_UINT                    0x087 
#define BRW_SURFACEFORMAT_R32_FLOAT_X8X24_TYPELESS       0x088 
#define BRW_SURFACEFORMAT_X32_TYPELESS_G8X24_UINT        0x089 
#define BRW_SURFACEFORMAT_L32A32_FLOAT                   0x08A 
#define BRW_SURFACEFORMAT_R32G32_UNORM                   0x08B 
#define BRW_SURFACEFORMAT_R32G32_SNORM                   0x08C 
#define BRW_SURFACEFORMAT_R64_FLOAT                      0x08D 
#define BRW_SURFACEFORMAT_R16G16B16X16_UNORM             0x08E 
#define BRW_SURFACEFORMAT_R16G16B16X16_FLOAT             0x08F 
#define BRW_SURFACEFORMAT_A32X32_FLOAT                   0x090 
#define BRW_SURFACEFORMAT_L32X32_FLOAT                   0x091 
#define BRW_SURFACEFORMAT_I32X32_FLOAT                   0x092 
#define BRW_SURFACEFORMAT_R16G16B16A16_SSCALED           0x093
#define BRW_SURFACEFORMAT_R16G16B16A16_USCALED           0x094
#define BRW_SURFACEFORMAT_R32G32_SSCALED                 0x095
#define BRW_SURFACEFORMAT_R32G32_USCALED                 0x096
#define BRW_SURFACEFORMAT_B8G8R8A8_UNORM                 0x0C0 
#define BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB            0x0C1 
#define BRW_SURFACEFORMAT_R10G10B10A2_UNORM              0x0C2 
#define BRW_SURFACEFORMAT_R10G10B10A2_UNORM_SRGB         0x0C3 
#define BRW_SURFACEFORMAT_R10G10B10A2_UINT               0x0C4 
#define BRW_SURFACEFORMAT_R10G10B10_SNORM_A2_UNORM       0x0C5 
#define BRW_SURFACEFORMAT_R8G8B8A8_UNORM                 0x0C7 
#define BRW_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB            0x0C8 
#define BRW_SURFACEFORMAT_R8G8B8A8_SNORM                 0x0C9 
#define BRW_SURFACEFORMAT_R8G8B8A8_SINT                  0x0CA 
#define BRW_SURFACEFORMAT_R8G8B8A8_UINT                  0x0CB 
#define BRW_SURFACEFORMAT_R16G16_UNORM                   0x0CC 
#define BRW_SURFACEFORMAT_R16G16_SNORM                   0x0CD 
#define BRW_SURFACEFORMAT_R16G16_SINT                    0x0CE 
#define BRW_SURFACEFORMAT_R16G16_UINT                    0x0CF 
#define BRW_SURFACEFORMAT_R16G16_FLOAT                   0x0D0 
#define BRW_SURFACEFORMAT_B10G10R10A2_UNORM              0x0D1 
#define BRW_SURFACEFORMAT_B10G10R10A2_UNORM_SRGB         0x0D2 
#define BRW_SURFACEFORMAT_R11G11B10_FLOAT                0x0D3 
#define BRW_SURFACEFORMAT_R32_SINT                       0x0D6 
#define BRW_SURFACEFORMAT_R32_UINT                       0x0D7 
#define BRW_SURFACEFORMAT_R32_FLOAT                      0x0D8 
#define BRW_SURFACEFORMAT_R24_UNORM_X8_TYPELESS          0x0D9 
#define BRW_SURFACEFORMAT_X24_TYPELESS_G8_UINT           0x0DA 
#define BRW_SURFACEFORMAT_L16A16_UNORM                   0x0DF 
#define BRW_SURFACEFORMAT_I24X8_UNORM                    0x0E0 
#define BRW_SURFACEFORMAT_L24X8_UNORM                    0x0E1 
#define BRW_SURFACEFORMAT_A24X8_UNORM                    0x0E2 
#define BRW_SURFACEFORMAT_I32_FLOAT                      0x0E3 
#define BRW_SURFACEFORMAT_L32_FLOAT                      0x0E4 
#define BRW_SURFACEFORMAT_A32_FLOAT                      0x0E5 
#define BRW_SURFACEFORMAT_B8G8R8X8_UNORM                 0x0E9 
#define BRW_SURFACEFORMAT_B8G8R8X8_UNORM_SRGB            0x0EA 
#define BRW_SURFACEFORMAT_R8G8B8X8_UNORM                 0x0EB 
#define BRW_SURFACEFORMAT_R8G8B8X8_UNORM_SRGB            0x0EC 
#define BRW_SURFACEFORMAT_R9G9B9E5_SHAREDEXP             0x0ED 
#define BRW_SURFACEFORMAT_B10G10R10X2_UNORM              0x0EE 
#define BRW_SURFACEFORMAT_L16A16_FLOAT                   0x0F0 
#define BRW_SURFACEFORMAT_R32_UNORM                      0x0F1 
#define BRW_SURFACEFORMAT_R32_SNORM                      0x0F2 
#define BRW_SURFACEFORMAT_R10G10B10X2_USCALED            0x0F3
#define BRW_SURFACEFORMAT_R8G8B8A8_SSCALED               0x0F4
#define BRW_SURFACEFORMAT_R8G8B8A8_USCALED               0x0F5
#define BRW_SURFACEFORMAT_R16G16_SSCALED                 0x0F6
#define BRW_SURFACEFORMAT_R16G16_USCALED                 0x0F7
#define BRW_SURFACEFORMAT_R32_SSCALED                    0x0F8
#define BRW_SURFACEFORMAT_R32_USCALED                    0x0F9
#define BRW_SURFACEFORMAT_B5G6R5_UNORM                   0x100 
#define BRW_SURFACEFORMAT_B5G6R5_UNORM_SRGB              0x101 
#define BRW_SURFACEFORMAT_B5G5R5A1_UNORM                 0x102 
#define BRW_SURFACEFORMAT_B5G5R5A1_UNORM_SRGB            0x103 
#define BRW_SURFACEFORMAT_B4G4R4A4_UNORM                 0x104 
#define BRW_SURFACEFORMAT_B4G4R4A4_UNORM_SRGB            0x105 
#define BRW_SURFACEFORMAT_R8G8_UNORM                     0x106 
#define BRW_SURFACEFORMAT_R8G8_SNORM                     0x107 
#define BRW_SURFACEFORMAT_R8G8_SINT                      0x108 
#define BRW_SURFACEFORMAT_R8G8_UINT                      0x109 
#define BRW_SURFACEFORMAT_R16_UNORM                      0x10A 
#define BRW_SURFACEFORMAT_R16_SNORM                      0x10B 
#define BRW_SURFACEFORMAT_R16_SINT                       0x10C 
#define BRW_SURFACEFORMAT_R16_UINT                       0x10D 
#define BRW_SURFACEFORMAT_R16_FLOAT                      0x10E 
#define BRW_SURFACEFORMAT_I16_UNORM                      0x111 
#define BRW_SURFACEFORMAT_L16_UNORM                      0x112 
#define BRW_SURFACEFORMAT_A16_UNORM                      0x113 
#define BRW_SURFACEFORMAT_L8A8_UNORM                     0x114 
#define BRW_SURFACEFORMAT_I16_FLOAT                      0x115
#define BRW_SURFACEFORMAT_L16_FLOAT                      0x116
#define BRW_SURFACEFORMAT_A16_FLOAT                      0x117
#define BRW_SURFACEFORMAT_L8A8_UNORM_SRGB                0x118
#define BRW_SURFACEFORMAT_R5G5_SNORM_B6_UNORM            0x119
#define BRW_SURFACEFORMAT_B5G5R5X1_UNORM                 0x11A
#define BRW_SURFACEFORMAT_B5G5R5X1_UNORM_SRGB            0x11B
#define BRW_SURFACEFORMAT_R8G8_SSCALED                   0x11C
#define BRW_SURFACEFORMAT_R8G8_USCALED                   0x11D
#define BRW_SURFACEFORMAT_R16_SSCALED                    0x11E
#define BRW_SURFACEFORMAT_R16_USCALED                    0x11F
#define BRW_SURFACEFORMAT_R8_UNORM                       0x140 
#define BRW_SURFACEFORMAT_R8_SNORM                       0x141 
#define BRW_SURFACEFORMAT_R8_SINT                        0x142 
#define BRW_SURFACEFORMAT_R8_UINT                        0x143 
#define BRW_SURFACEFORMAT_A8_UNORM                       0x144 
#define BRW_SURFACEFORMAT_I8_UNORM                       0x145 
#define BRW_SURFACEFORMAT_L8_UNORM                       0x146 
#define BRW_SURFACEFORMAT_P4A4_UNORM                     0x147 
#define BRW_SURFACEFORMAT_A4P4_UNORM                     0x148
#define BRW_SURFACEFORMAT_R8_SSCALED                     0x149
#define BRW_SURFACEFORMAT_R8_USCALED                     0x14A
#define BRW_SURFACEFORMAT_L8_UNORM_SRGB                  0x14C
#define BRW_SURFACEFORMAT_DXT1_RGB_SRGB                  0x180
#define BRW_SURFACEFORMAT_R1_UINT                        0x181 
#define BRW_SURFACEFORMAT_YCRCB_NORMAL                   0x182 
#define BRW_SURFACEFORMAT_YCRCB_SWAPUVY                  0x183 
#define BRW_SURFACEFORMAT_BC1_UNORM                      0x186 
#define BRW_SURFACEFORMAT_BC2_UNORM                      0x187 
#define BRW_SURFACEFORMAT_BC3_UNORM                      0x188 
#define BRW_SURFACEFORMAT_BC4_UNORM                      0x189 
#define BRW_SURFACEFORMAT_BC5_UNORM                      0x18A 
#define BRW_SURFACEFORMAT_BC1_UNORM_SRGB                 0x18B 
#define BRW_SURFACEFORMAT_BC2_UNORM_SRGB                 0x18C 
#define BRW_SURFACEFORMAT_BC3_UNORM_SRGB                 0x18D 
#define BRW_SURFACEFORMAT_MONO8                          0x18E 
#define BRW_SURFACEFORMAT_YCRCB_SWAPUV                   0x18F 
#define BRW_SURFACEFORMAT_YCRCB_SWAPY                    0x190 
#define BRW_SURFACEFORMAT_DXT1_RGB                       0x191 
#define BRW_SURFACEFORMAT_FXT1                           0x192 
#define BRW_SURFACEFORMAT_R8G8B8_UNORM                   0x193 
#define BRW_SURFACEFORMAT_R8G8B8_SNORM                   0x194 
#define BRW_SURFACEFORMAT_R8G8B8_SSCALED                 0x195 
#define BRW_SURFACEFORMAT_R8G8B8_USCALED                 0x196 
#define BRW_SURFACEFORMAT_R64G64B64A64_FLOAT             0x197 
#define BRW_SURFACEFORMAT_R64G64B64_FLOAT                0x198 
#define BRW_SURFACEFORMAT_BC4_SNORM                      0x199 
#define BRW_SURFACEFORMAT_BC5_SNORM                      0x19A 
#define BRW_SURFACEFORMAT_R16G16B16_UNORM                0x19C 
#define BRW_SURFACEFORMAT_R16G16B16_SNORM                0x19D 
#define BRW_SURFACEFORMAT_R16G16B16_SSCALED              0x19E 
#define BRW_SURFACEFORMAT_R16G16B16_USCALED              0x19F
#define BRW_SURFACE_FORMAT_SHIFT	18
#define BRW_SURFACE_FORMAT_MASK		INTEL_MASK(26, 18)

#define BRW_SURFACERETURNFORMAT_FLOAT32  0
#define BRW_SURFACERETURNFORMAT_S1       1

#define BRW_SURFACE_TYPE_SHIFT		29
#define BRW_SURFACE_TYPE_MASK		INTEL_MASK(31, 29)
#define BRW_SURFACE_1D      0
#define BRW_SURFACE_2D      1
#define BRW_SURFACE_3D      2
#define BRW_SURFACE_CUBE    3
#define BRW_SURFACE_BUFFER  4
#define BRW_SURFACE_NULL    7

#define GEN7_SURFACE_ARYSPC_FULL	0
#define GEN7_SURFACE_ARYSPC_LOD0	1

/* Surface state DW2 */
#define BRW_SURFACE_HEIGHT_SHIFT	19
#define BRW_SURFACE_HEIGHT_MASK		INTEL_MASK(31, 19)
#define BRW_SURFACE_WIDTH_SHIFT		6
#define BRW_SURFACE_WIDTH_MASK		INTEL_MASK(18, 6)
#define BRW_SURFACE_LOD_SHIFT		2
#define BRW_SURFACE_LOD_MASK		INTEL_MASK(5, 2)

/* Surface state DW3 */
#define BRW_SURFACE_DEPTH_SHIFT		21
#define BRW_SURFACE_DEPTH_MASK		INTEL_MASK(31, 21)
#define BRW_SURFACE_PITCH_SHIFT		3
#define BRW_SURFACE_PITCH_MASK		INTEL_MASK(19, 3)
#define BRW_SURFACE_TILED		(1 << 1)
#define BRW_SURFACE_TILED_Y		(1 << 0)

/* Surface state DW4 */
#define BRW_SURFACE_MIN_LOD_SHIFT	28
#define BRW_SURFACE_MIN_LOD_MASK	INTEL_MASK(31, 28)
#define BRW_SURFACE_MULTISAMPLECOUNT_1  (0 << 4)
#define BRW_SURFACE_MULTISAMPLECOUNT_4  (2 << 4)
#define GEN7_SURFACE_MULTISAMPLECOUNT_1 0
#define GEN7_SURFACE_MULTISAMPLECOUNT_4 2
#define GEN7_SURFACE_MULTISAMPLECOUNT_8 3
#define GEN7_SURFACE_MSFMT_MSS			0
#define GEN7_SURFACE_MSFMT_DEPTH_STENCIL	1

/* Surface state DW5 */
#define BRW_SURFACE_X_OFFSET_SHIFT		25
#define BRW_SURFACE_X_OFFSET_MASK		INTEL_MASK(31, 25)
#define BRW_SURFACE_VERTICAL_ALIGN_ENABLE	(1 << 24)
#define BRW_SURFACE_Y_OFFSET_SHIFT		20
#define BRW_SURFACE_Y_OFFSET_MASK		INTEL_MASK(23, 20)

/* Surface state DW7 */
#define HSW_SCS_ZERO                     0
#define HSW_SCS_ONE                      1
#define HSW_SCS_RED                      4
#define HSW_SCS_GREEN                    5
#define HSW_SCS_BLUE                     6
#define HSW_SCS_ALPHA                    7

#define BRW_TEXCOORDMODE_WRAP            0
#define BRW_TEXCOORDMODE_MIRROR          1
#define BRW_TEXCOORDMODE_CLAMP           2
#define BRW_TEXCOORDMODE_CUBE            3
#define BRW_TEXCOORDMODE_CLAMP_BORDER    4
#define BRW_TEXCOORDMODE_MIRROR_ONCE     5

#define BRW_THREAD_PRIORITY_NORMAL   0
#define BRW_THREAD_PRIORITY_HIGH     1

#define BRW_TILEWALK_XMAJOR                 0
#define BRW_TILEWALK_YMAJOR                 1

#define BRW_VERTEX_SUBPIXEL_PRECISION_8BITS  0
#define BRW_VERTEX_SUBPIXEL_PRECISION_4BITS  1

/* Execution Unit (EU) defines
 */

#define BRW_ALIGN_1   0
#define BRW_ALIGN_16  1

#define BRW_ADDRESS_DIRECT                        0
#define BRW_ADDRESS_REGISTER_INDIRECT_REGISTER    1

#define BRW_CHANNEL_X     0
#define BRW_CHANNEL_Y     1
#define BRW_CHANNEL_Z     2
#define BRW_CHANNEL_W     3

enum brw_compression {
   BRW_COMPRESSION_NONE       = 0,
   BRW_COMPRESSION_2NDHALF    = 1,
   BRW_COMPRESSION_COMPRESSED = 2,
};

#define GEN6_COMPRESSION_1Q		0
#define GEN6_COMPRESSION_2Q		1
#define GEN6_COMPRESSION_3Q		2
#define GEN6_COMPRESSION_4Q		3
#define GEN6_COMPRESSION_1H		0
#define GEN6_COMPRESSION_2H		2

#define BRW_CONDITIONAL_NONE  0
#define BRW_CONDITIONAL_Z     1
#define BRW_CONDITIONAL_NZ    2
#define BRW_CONDITIONAL_EQ    1	/* Z */
#define BRW_CONDITIONAL_NEQ   2	/* NZ */
#define BRW_CONDITIONAL_G     3
#define BRW_CONDITIONAL_GE    4
#define BRW_CONDITIONAL_L     5
#define BRW_CONDITIONAL_LE    6
#define BRW_CONDITIONAL_R     7
#define BRW_CONDITIONAL_O     8
#define BRW_CONDITIONAL_U     9

#define BRW_DEBUG_NONE        0
#define BRW_DEBUG_BREAKPOINT  1

#define BRW_DEPENDENCY_NORMAL         0
#define BRW_DEPENDENCY_NOTCLEARED     1
#define BRW_DEPENDENCY_NOTCHECKED     2
#define BRW_DEPENDENCY_DISABLE        3

#define BRW_EXECUTE_1     0
#define BRW_EXECUTE_2     1
#define BRW_EXECUTE_4     2
#define BRW_EXECUTE_8     3
#define BRW_EXECUTE_16    4
#define BRW_EXECUTE_32    5

#define BRW_HORIZONTAL_STRIDE_0   0
#define BRW_HORIZONTAL_STRIDE_1   1
#define BRW_HORIZONTAL_STRIDE_2   2
#define BRW_HORIZONTAL_STRIDE_4   3

#define BRW_INSTRUCTION_NORMAL    0
#define BRW_INSTRUCTION_SATURATE  1

#define BRW_MASK_ENABLE   0
#define BRW_MASK_DISABLE  1

/** @{
 *
 * Gen6 has replaced "mask enable/disable" with WECtrl, which is
 * effectively the same but much simpler to think about.  Now, there
 * are two contributors ANDed together to whether channels are
 * executed: The predication on the instruction, and the channel write
 * enable.
 */
/**
 * This is the default value.  It means that a channel's write enable is set
 * if the per-channel IP is pointing at this instruction.
 */
#define BRW_WE_NORMAL		0
/**
 * This is used like BRW_MASK_DISABLE, and causes all channels to have
 * their write enable set.  Note that predication still contributes to
 * whether the channel actually gets written.
 */
#define BRW_WE_ALL		1
/** @} */

enum opcode {
   /* These are the actual hardware opcodes. */
   BRW_OPCODE_MOV =	1,
   BRW_OPCODE_SEL =	2,
   BRW_OPCODE_NOT =	4,
   BRW_OPCODE_AND =	5,
   BRW_OPCODE_OR =	6,
   BRW_OPCODE_XOR =	7,
   BRW_OPCODE_SHR =	8,
   BRW_OPCODE_SHL =	9,
   BRW_OPCODE_RSR =	10,
   BRW_OPCODE_RSL =	11,
   BRW_OPCODE_ASR =	12,
   BRW_OPCODE_CMP =	16,
   BRW_OPCODE_CMPN =	17,
   BRW_OPCODE_JMPI =	32,
   BRW_OPCODE_IF =	34,
   BRW_OPCODE_IFF =	35,
   BRW_OPCODE_ELSE =	36,
   BRW_OPCODE_ENDIF =	37,
   BRW_OPCODE_DO =	38,
   BRW_OPCODE_WHILE =	39,
   BRW_OPCODE_BREAK =	40,
   BRW_OPCODE_CONTINUE = 41,
   BRW_OPCODE_HALT =	42,
   BRW_OPCODE_MSAVE =	44,
   BRW_OPCODE_MRESTORE = 45,
   BRW_OPCODE_PUSH =	46,
   BRW_OPCODE_POP =	47,
   BRW_OPCODE_WAIT =	48,
   BRW_OPCODE_SEND =	49,
   BRW_OPCODE_SENDC =	50,
   BRW_OPCODE_MATH =	56,
   BRW_OPCODE_ADD =	64,
   BRW_OPCODE_MUL =	65,
   BRW_OPCODE_AVG =	66,
   BRW_OPCODE_FRC =	67,
   BRW_OPCODE_RNDU =	68,
   BRW_OPCODE_RNDD =	69,
   BRW_OPCODE_RNDE =	70,
   BRW_OPCODE_RNDZ =	71,
   BRW_OPCODE_MAC =	72,
   BRW_OPCODE_MACH =	73,
   BRW_OPCODE_LZD =	74,
   BRW_OPCODE_SAD2 =	80,
   BRW_OPCODE_SADA2 =	81,
   BRW_OPCODE_DP4 =	84,
   BRW_OPCODE_DPH =	85,
   BRW_OPCODE_DP3 =	86,
   BRW_OPCODE_DP2 =	87,
   BRW_OPCODE_DPA2 =	88,
   BRW_OPCODE_LINE =	89,
   BRW_OPCODE_PLN =	90,
   BRW_OPCODE_MAD =	91,
   BRW_OPCODE_NOP =	126,

   /* These are compiler backend opcodes that get translated into other
    * instructions.
    */
   FS_OPCODE_FB_WRITE = 128,
   SHADER_OPCODE_RCP,
   SHADER_OPCODE_RSQ,
   SHADER_OPCODE_SQRT,
   SHADER_OPCODE_EXP2,
   SHADER_OPCODE_LOG2,
   SHADER_OPCODE_POW,
   SHADER_OPCODE_INT_QUOTIENT,
   SHADER_OPCODE_INT_REMAINDER,
   SHADER_OPCODE_SIN,
   SHADER_OPCODE_COS,

   SHADER_OPCODE_TEX,
   SHADER_OPCODE_TXD,
   SHADER_OPCODE_TXF,
   SHADER_OPCODE_TXL,
   SHADER_OPCODE_TXS,
   FS_OPCODE_TXB,

   FS_OPCODE_DDX,
   FS_OPCODE_DDY,
   FS_OPCODE_PIXEL_X,
   FS_OPCODE_PIXEL_Y,
   FS_OPCODE_CINTERP,
   FS_OPCODE_LINTERP,
   FS_OPCODE_DISCARD,
   FS_OPCODE_SPILL,
   FS_OPCODE_UNSPILL,
   FS_OPCODE_PULL_CONSTANT_LOAD,
   FS_OPCODE_MOV_DISPATCH_TO_FLAGS,

   VS_OPCODE_URB_WRITE,
   VS_OPCODE_SCRATCH_READ,
   VS_OPCODE_SCRATCH_WRITE,
   VS_OPCODE_PULL_CONSTANT_LOAD,
};

#define BRW_PREDICATE_NONE             0
#define BRW_PREDICATE_NORMAL           1
#define BRW_PREDICATE_ALIGN1_ANYV             2
#define BRW_PREDICATE_ALIGN1_ALLV             3
#define BRW_PREDICATE_ALIGN1_ANY2H            4
#define BRW_PREDICATE_ALIGN1_ALL2H            5
#define BRW_PREDICATE_ALIGN1_ANY4H            6
#define BRW_PREDICATE_ALIGN1_ALL4H            7
#define BRW_PREDICATE_ALIGN1_ANY8H            8
#define BRW_PREDICATE_ALIGN1_ALL8H            9
#define BRW_PREDICATE_ALIGN1_ANY16H           10
#define BRW_PREDICATE_ALIGN1_ALL16H           11
#define BRW_PREDICATE_ALIGN16_REPLICATE_X     2
#define BRW_PREDICATE_ALIGN16_REPLICATE_Y     3
#define BRW_PREDICATE_ALIGN16_REPLICATE_Z     4
#define BRW_PREDICATE_ALIGN16_REPLICATE_W     5
#define BRW_PREDICATE_ALIGN16_ANY4H           6
#define BRW_PREDICATE_ALIGN16_ALL4H           7

#define BRW_ARCHITECTURE_REGISTER_FILE    0
#define BRW_GENERAL_REGISTER_FILE         1
#define BRW_MESSAGE_REGISTER_FILE         2
#define BRW_IMMEDIATE_VALUE               3

#define BRW_REGISTER_TYPE_UD  0
#define BRW_REGISTER_TYPE_D   1
#define BRW_REGISTER_TYPE_UW  2
#define BRW_REGISTER_TYPE_W   3
#define BRW_REGISTER_TYPE_UB  4
#define BRW_REGISTER_TYPE_B   5
#define BRW_REGISTER_TYPE_VF  5	/* packed float vector, immediates only? */
#define BRW_REGISTER_TYPE_HF  6
#define BRW_REGISTER_TYPE_V   6	/* packed int vector, immediates only, uword dest only */
#define BRW_REGISTER_TYPE_F   7

#define BRW_ARF_NULL                  0x00
#define BRW_ARF_ADDRESS               0x10
#define BRW_ARF_ACCUMULATOR           0x20   
#define BRW_ARF_FLAG                  0x30
#define BRW_ARF_MASK                  0x40
#define BRW_ARF_MASK_STACK            0x50
#define BRW_ARF_MASK_STACK_DEPTH      0x60
#define BRW_ARF_STATE                 0x70
#define BRW_ARF_CONTROL               0x80
#define BRW_ARF_NOTIFICATION_COUNT    0x90
#define BRW_ARF_IP                    0xA0

#define BRW_MRF_COMPR4			(1 << 7)

#define BRW_AMASK   0
#define BRW_IMASK   1
#define BRW_LMASK   2
#define BRW_CMASK   3



#define BRW_THREAD_NORMAL     0
#define BRW_THREAD_ATOMIC     1
#define BRW_THREAD_SWITCH     2

#define BRW_VERTICAL_STRIDE_0                 0
#define BRW_VERTICAL_STRIDE_1                 1
#define BRW_VERTICAL_STRIDE_2                 2
#define BRW_VERTICAL_STRIDE_4                 3
#define BRW_VERTICAL_STRIDE_8                 4
#define BRW_VERTICAL_STRIDE_16                5
#define BRW_VERTICAL_STRIDE_32                6
#define BRW_VERTICAL_STRIDE_64                7
#define BRW_VERTICAL_STRIDE_128               8
#define BRW_VERTICAL_STRIDE_256               9
#define BRW_VERTICAL_STRIDE_ONE_DIMENSIONAL   0xF

#define BRW_WIDTH_1       0
#define BRW_WIDTH_2       1
#define BRW_WIDTH_4       2
#define BRW_WIDTH_8       3
#define BRW_WIDTH_16      4

#define BRW_STATELESS_BUFFER_BOUNDARY_1K      0
#define BRW_STATELESS_BUFFER_BOUNDARY_2K      1
#define BRW_STATELESS_BUFFER_BOUNDARY_4K      2
#define BRW_STATELESS_BUFFER_BOUNDARY_8K      3
#define BRW_STATELESS_BUFFER_BOUNDARY_16K     4
#define BRW_STATELESS_BUFFER_BOUNDARY_32K     5
#define BRW_STATELESS_BUFFER_BOUNDARY_64K     6
#define BRW_STATELESS_BUFFER_BOUNDARY_128K    7
#define BRW_STATELESS_BUFFER_BOUNDARY_256K    8
#define BRW_STATELESS_BUFFER_BOUNDARY_512K    9
#define BRW_STATELESS_BUFFER_BOUNDARY_1M      10
#define BRW_STATELESS_BUFFER_BOUNDARY_2M      11

#define BRW_POLYGON_FACING_FRONT      0
#define BRW_POLYGON_FACING_BACK       1

/**
 * Message target: Shared Function ID for where to SEND a message.
 *
 * These are enumerated in the ISA reference under "send - Send Message".
 * In particular, see the following tables:
 * - G45 PRM, Volume 4, Table 14-15 "Message Descriptor Definition"
 * - Sandybridge PRM, Volume 4 Part 2, Table 8-16 "Extended Message Descriptor"
 * - BSpec, Volume 1a (GPU Overview) / Graphics Processing Engine (GPE) /
 *   Overview / GPE Function IDs
 */
enum brw_message_target {
   BRW_SFID_NULL                     = 0,
   BRW_SFID_MATH                     = 1, /* Only valid on Gen4-5 */
   BRW_SFID_SAMPLER                  = 2,
   BRW_SFID_MESSAGE_GATEWAY          = 3,
   BRW_SFID_DATAPORT_READ            = 4,
   BRW_SFID_DATAPORT_WRITE           = 5,
   BRW_SFID_URB                      = 6,
   BRW_SFID_THREAD_SPAWNER           = 7,

   GEN6_SFID_DATAPORT_SAMPLER_CACHE  = 4,
   GEN6_SFID_DATAPORT_RENDER_CACHE   = 5,
   GEN6_SFID_DATAPORT_CONSTANT_CACHE = 9,

   GEN7_SFID_DATAPORT_DATA_CACHE     = 10,
};

#define GEN7_MESSAGE_TARGET_DP_DATA_CACHE     10

#define BRW_SAMPLER_RETURN_FORMAT_FLOAT32     0
#define BRW_SAMPLER_RETURN_FORMAT_UINT32      2
#define BRW_SAMPLER_RETURN_FORMAT_SINT32      3

#define BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE              0
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE             0
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS        0
#define BRW_SAMPLER_MESSAGE_SIMD8_KILLPIX             1
#define BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_LOD        1
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_LOD         1
#define BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_GRADIENTS  2
#define BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_GRADIENTS    2
#define BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_COMPARE    0
#define BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_COMPARE     2
#define BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_BIAS_COMPARE 0
#define BRW_SAMPLER_MESSAGE_SIMD4X2_SAMPLE_LOD_COMPARE 1
#define BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_LOD_COMPARE  1
#define BRW_SAMPLER_MESSAGE_SIMD4X2_RESINFO           2
#define BRW_SAMPLER_MESSAGE_SIMD16_RESINFO            2
#define BRW_SAMPLER_MESSAGE_SIMD4X2_LD                3
#define BRW_SAMPLER_MESSAGE_SIMD8_LD                  3
#define BRW_SAMPLER_MESSAGE_SIMD16_LD                 3

#define GEN5_SAMPLER_MESSAGE_SAMPLE              0
#define GEN5_SAMPLER_MESSAGE_SAMPLE_BIAS         1
#define GEN5_SAMPLER_MESSAGE_SAMPLE_LOD          2
#define GEN5_SAMPLER_MESSAGE_SAMPLE_COMPARE      3
#define GEN5_SAMPLER_MESSAGE_SAMPLE_DERIVS       4
#define GEN5_SAMPLER_MESSAGE_SAMPLE_BIAS_COMPARE 5
#define GEN5_SAMPLER_MESSAGE_SAMPLE_LOD_COMPARE  6
#define GEN5_SAMPLER_MESSAGE_SAMPLE_LD           7
#define GEN5_SAMPLER_MESSAGE_SAMPLE_RESINFO      10
#define HSW_SAMPLER_MESSAGE_SAMPLE_DERIV_COMPARE 20
#define GEN7_SAMPLER_MESSAGE_SAMPLE_LD_MCS       29
#define GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DMS       30
#define GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DSS       31

/* for GEN5 only */
#define BRW_SAMPLER_SIMD_MODE_SIMD4X2                   0
#define BRW_SAMPLER_SIMD_MODE_SIMD8                     1
#define BRW_SAMPLER_SIMD_MODE_SIMD16                    2
#define BRW_SAMPLER_SIMD_MODE_SIMD32_64                 3

#define BRW_DATAPORT_OWORD_BLOCK_1_OWORDLOW   0
#define BRW_DATAPORT_OWORD_BLOCK_1_OWORDHIGH  1
#define BRW_DATAPORT_OWORD_BLOCK_2_OWORDS     2
#define BRW_DATAPORT_OWORD_BLOCK_4_OWORDS     3
#define BRW_DATAPORT_OWORD_BLOCK_8_OWORDS     4

#define BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD     0
#define BRW_DATAPORT_OWORD_DUAL_BLOCK_4OWORDS    2

#define BRW_DATAPORT_DWORD_SCATTERED_BLOCK_8DWORDS   2
#define BRW_DATAPORT_DWORD_SCATTERED_BLOCK_16DWORDS  3

/* This one stays the same across generations. */
#define BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ          0
/* GEN4 */
#define BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ     1
#define BRW_DATAPORT_READ_MESSAGE_MEDIA_BLOCK_READ          2
#define BRW_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ      3
/* G45, GEN5 */
#define G45_DATAPORT_READ_MESSAGE_RENDER_UNORM_READ	    1
#define G45_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ     2
#define G45_DATAPORT_READ_MESSAGE_AVC_LOOP_FILTER_READ	    3
#define G45_DATAPORT_READ_MESSAGE_MEDIA_BLOCK_READ          4
#define G45_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ      6
/* GEN6 */
#define GEN6_DATAPORT_READ_MESSAGE_RENDER_UNORM_READ	    1
#define GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ     2
#define GEN6_DATAPORT_READ_MESSAGE_MEDIA_BLOCK_READ          4
#define GEN6_DATAPORT_READ_MESSAGE_OWORD_UNALIGN_BLOCK_READ  5
#define GEN6_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ      6

#define BRW_DATAPORT_READ_TARGET_DATA_CACHE      0
#define BRW_DATAPORT_READ_TARGET_RENDER_CACHE    1
#define BRW_DATAPORT_READ_TARGET_SAMPLER_CACHE   2

#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE                0
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE_REPLICATED     1
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_DUAL_SOURCE_SUBSPAN01         2
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_DUAL_SOURCE_SUBSPAN23         3
#define BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_SINGLE_SOURCE_SUBSPAN01       4

#define BRW_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE                0
#define BRW_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE           1
#define BRW_DATAPORT_WRITE_MESSAGE_MEDIA_BLOCK_WRITE                2
#define BRW_DATAPORT_WRITE_MESSAGE_DWORD_SCATTERED_WRITE            3
#define BRW_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE              4
#define BRW_DATAPORT_WRITE_MESSAGE_STREAMED_VERTEX_BUFFER_WRITE     5
#define BRW_DATAPORT_WRITE_MESSAGE_FLUSH_RENDER_CACHE               7

/* GEN6 */
#define GEN6_DATAPORT_WRITE_MESSAGE_DWORD_ATOMIC_WRITE              7
#define GEN6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE               8
#define GEN6_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE          9
#define GEN6_DATAPORT_WRITE_MESSAGE_MEDIA_BLOCK_WRITE               10
#define GEN6_DATAPORT_WRITE_MESSAGE_DWORD_SCATTERED_WRITE           11
#define GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE             12
#define GEN6_DATAPORT_WRITE_MESSAGE_STREAMED_VB_WRITE               13
#define GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_UNORM_WRITE       14

/* GEN7 */
#define GEN7_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE          10

#define BRW_MATH_FUNCTION_INV                              1
#define BRW_MATH_FUNCTION_LOG                              2
#define BRW_MATH_FUNCTION_EXP                              3
#define BRW_MATH_FUNCTION_SQRT                             4
#define BRW_MATH_FUNCTION_RSQ                              5
#define BRW_MATH_FUNCTION_SIN                              6 /* was 7 */
#define BRW_MATH_FUNCTION_COS                              7 /* was 8 */
#define BRW_MATH_FUNCTION_SINCOS                           8 /* was 6 */
#define BRW_MATH_FUNCTION_TAN                              9 /* gen4 */
#define BRW_MATH_FUNCTION_FDIV                             9 /* gen6+ */
#define BRW_MATH_FUNCTION_POW                              10
#define BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER   11
#define BRW_MATH_FUNCTION_INT_DIV_QUOTIENT                 12
#define BRW_MATH_FUNCTION_INT_DIV_REMAINDER                13

#define BRW_MATH_INTEGER_UNSIGNED     0
#define BRW_MATH_INTEGER_SIGNED       1

#define BRW_MATH_PRECISION_FULL        0
#define BRW_MATH_PRECISION_PARTIAL     1

#define BRW_MATH_SATURATE_NONE         0
#define BRW_MATH_SATURATE_SATURATE     1

#define BRW_MATH_DATA_VECTOR  0
#define BRW_MATH_DATA_SCALAR  1

#define BRW_URB_OPCODE_WRITE  0

#define BRW_URB_SWIZZLE_NONE          0
#define BRW_URB_SWIZZLE_INTERLEAVE    1
#define BRW_URB_SWIZZLE_TRANSPOSE     2

#define BRW_SCRATCH_SPACE_SIZE_1K     0
#define BRW_SCRATCH_SPACE_SIZE_2K     1
#define BRW_SCRATCH_SPACE_SIZE_4K     2
#define BRW_SCRATCH_SPACE_SIZE_8K     3
#define BRW_SCRATCH_SPACE_SIZE_16K    4
#define BRW_SCRATCH_SPACE_SIZE_32K    5
#define BRW_SCRATCH_SPACE_SIZE_64K    6
#define BRW_SCRATCH_SPACE_SIZE_128K   7
#define BRW_SCRATCH_SPACE_SIZE_256K   8
#define BRW_SCRATCH_SPACE_SIZE_512K   9
#define BRW_SCRATCH_SPACE_SIZE_1M     10
#define BRW_SCRATCH_SPACE_SIZE_2M     11




#define CMD_URB_FENCE                 0x6000
#define CMD_CS_URB_STATE              0x6001
#define CMD_CONST_BUFFER              0x6002

#define CMD_STATE_BASE_ADDRESS        0x6101
#define CMD_STATE_SIP                 0x6102
#define CMD_PIPELINE_SELECT_965       0x6104
#define CMD_PIPELINE_SELECT_GM45      0x6904

#define _3DSTATE_PIPELINED_POINTERS		0x7800
#define _3DSTATE_BINDING_TABLE_POINTERS		0x7801
# define GEN6_BINDING_TABLE_MODIFY_VS	(1 << 8)
# define GEN6_BINDING_TABLE_MODIFY_GS	(1 << 9)
# define GEN6_BINDING_TABLE_MODIFY_PS	(1 << 12)

#define _3DSTATE_BINDING_TABLE_POINTERS_VS	0x7826 /* GEN7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_HS	0x7827 /* GEN7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_DS	0x7828 /* GEN7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_GS	0x7829 /* GEN7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_PS	0x782A /* GEN7+ */

#define _3DSTATE_SAMPLER_STATE_POINTERS		0x7802 /* GEN6+ */
# define PS_SAMPLER_STATE_CHANGE				(1 << 12)
# define GS_SAMPLER_STATE_CHANGE				(1 << 9)
# define VS_SAMPLER_STATE_CHANGE				(1 << 8)
/* DW1: VS */
/* DW2: GS */
/* DW3: PS */

#define _3DSTATE_SAMPLER_STATE_POINTERS_VS	0x782B /* GEN7+ */
#define _3DSTATE_SAMPLER_STATE_POINTERS_GS	0x782E /* GEN7+ */
#define _3DSTATE_SAMPLER_STATE_POINTERS_PS	0x782F /* GEN7+ */

#define _3DSTATE_VERTEX_BUFFERS       0x7808
# define BRW_VB0_INDEX_SHIFT		27
# define GEN6_VB0_INDEX_SHIFT		26
# define BRW_VB0_ACCESS_VERTEXDATA	(0 << 26)
# define BRW_VB0_ACCESS_INSTANCEDATA	(1 << 26)
# define GEN6_VB0_ACCESS_VERTEXDATA	(0 << 20)
# define GEN6_VB0_ACCESS_INSTANCEDATA	(1 << 20)
# define GEN7_VB0_ADDRESS_MODIFYENABLE  (1 << 14)
# define BRW_VB0_PITCH_SHIFT		0

#define _3DSTATE_VERTEX_ELEMENTS      0x7809
# define BRW_VE0_INDEX_SHIFT		27
# define GEN6_VE0_INDEX_SHIFT		26
# define BRW_VE0_FORMAT_SHIFT		16
# define BRW_VE0_VALID			(1 << 26)
# define GEN6_VE0_VALID			(1 << 25)
# define GEN6_VE0_EDGE_FLAG_ENABLE	(1 << 15)
# define BRW_VE0_SRC_OFFSET_SHIFT	0
# define BRW_VE1_COMPONENT_NOSTORE	0
# define BRW_VE1_COMPONENT_STORE_SRC	1
# define BRW_VE1_COMPONENT_STORE_0	2
# define BRW_VE1_COMPONENT_STORE_1_FLT	3
# define BRW_VE1_COMPONENT_STORE_1_INT	4
# define BRW_VE1_COMPONENT_STORE_VID	5
# define BRW_VE1_COMPONENT_STORE_IID	6
# define BRW_VE1_COMPONENT_STORE_PID	7
# define BRW_VE1_COMPONENT_0_SHIFT	28
# define BRW_VE1_COMPONENT_1_SHIFT	24
# define BRW_VE1_COMPONENT_2_SHIFT	20
# define BRW_VE1_COMPONENT_3_SHIFT	16
# define BRW_VE1_DST_OFFSET_SHIFT	0

#define CMD_INDEX_BUFFER              0x780a
#define GEN4_3DSTATE_VF_STATISTICS		0x780b
#define GM45_3DSTATE_VF_STATISTICS		0x680b
#define _3DSTATE_CC_STATE_POINTERS		0x780e /* GEN6+ */
#define _3DSTATE_BLEND_STATE_POINTERS		0x7824 /* GEN7+ */
#define _3DSTATE_DEPTH_STENCIL_STATE_POINTERS	0x7825 /* GEN7+ */

#define _3DSTATE_URB				0x7805 /* GEN6 */
# define GEN6_URB_VS_SIZE_SHIFT				16
# define GEN6_URB_VS_ENTRIES_SHIFT			0
# define GEN6_URB_GS_ENTRIES_SHIFT			8
# define GEN6_URB_GS_SIZE_SHIFT				0

#define _3DSTATE_VF                             0x780c /* GEN7.5+ */
#define HSW_CUT_INDEX_ENABLE                            (1 << 8)

#define _3DSTATE_URB_VS                         0x7830 /* GEN7+ */
#define _3DSTATE_URB_HS                         0x7831 /* GEN7+ */
#define _3DSTATE_URB_DS                         0x7832 /* GEN7+ */
#define _3DSTATE_URB_GS                         0x7833 /* GEN7+ */
# define GEN7_URB_ENTRY_SIZE_SHIFT                      16
# define GEN7_URB_STARTING_ADDRESS_SHIFT                25

#define _3DSTATE_PUSH_CONSTANT_ALLOC_VS         0x7912 /* GEN7+ */
#define _3DSTATE_PUSH_CONSTANT_ALLOC_PS         0x7916 /* GEN7+ */
# define GEN7_PUSH_CONSTANT_BUFFER_OFFSET_SHIFT         16

#define _3DSTATE_VIEWPORT_STATE_POINTERS	0x780d /* GEN6+ */
# define GEN6_CC_VIEWPORT_MODIFY			(1 << 12)
# define GEN6_SF_VIEWPORT_MODIFY			(1 << 11)
# define GEN6_CLIP_VIEWPORT_MODIFY			(1 << 10)

#define _3DSTATE_VIEWPORT_STATE_POINTERS_CC	0x7823 /* GEN7+ */
#define _3DSTATE_VIEWPORT_STATE_POINTERS_SF_CL	0x7821 /* GEN7+ */

#define _3DSTATE_SCISSOR_STATE_POINTERS		0x780f /* GEN6+ */

#define _3DSTATE_VS				0x7810 /* GEN6+ */
/* DW2 */
# define GEN6_VS_SPF_MODE				(1 << 31)
# define GEN6_VS_VECTOR_MASK_ENABLE			(1 << 30)
# define GEN6_VS_SAMPLER_COUNT_SHIFT			27
# define GEN6_VS_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GEN6_VS_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GEN6_VS_FLOATING_POINT_MODE_ALT		(1 << 16)
/* DW4 */
# define GEN6_VS_DISPATCH_START_GRF_SHIFT		20
# define GEN6_VS_URB_READ_LENGTH_SHIFT			11
# define GEN6_VS_URB_ENTRY_READ_OFFSET_SHIFT		4
/* DW5 */
# define GEN6_VS_MAX_THREADS_SHIFT			25
# define HSW_VS_MAX_THREADS_SHIFT			23
# define GEN6_VS_STATISTICS_ENABLE			(1 << 10)
# define GEN6_VS_CACHE_DISABLE				(1 << 1)
# define GEN6_VS_ENABLE					(1 << 0)

#define _3DSTATE_GS		      		0x7811 /* GEN6+ */
/* DW2 */
# define GEN6_GS_SPF_MODE				(1 << 31)
# define GEN6_GS_VECTOR_MASK_ENABLE			(1 << 30)
# define GEN6_GS_SAMPLER_COUNT_SHIFT			27
# define GEN6_GS_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GEN6_GS_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GEN6_GS_FLOATING_POINT_MODE_ALT		(1 << 16)
/* DW4 */
# define GEN6_GS_URB_READ_LENGTH_SHIFT			11
# define GEN7_GS_INCLUDE_VERTEX_HANDLES		        (1 << 10)
# define GEN6_GS_URB_ENTRY_READ_OFFSET_SHIFT		4
# define GEN6_GS_DISPATCH_START_GRF_SHIFT		0
/* DW5 */
# define GEN6_GS_MAX_THREADS_SHIFT			25
# define GEN6_GS_STATISTICS_ENABLE			(1 << 10)
# define GEN6_GS_SO_STATISTICS_ENABLE			(1 << 9)
# define GEN6_GS_RENDERING_ENABLE			(1 << 8)
# define GEN7_GS_ENABLE					(1 << 0)
/* DW6 */
# define GEN6_GS_REORDER				(1 << 30)
# define GEN6_GS_DISCARD_ADJACENCY			(1 << 29)
# define GEN6_GS_SVBI_PAYLOAD_ENABLE			(1 << 28)
# define GEN6_GS_SVBI_POSTINCREMENT_ENABLE		(1 << 27)
# define GEN6_GS_SVBI_POSTINCREMENT_VALUE_SHIFT		16
# define GEN6_GS_SVBI_POSTINCREMENT_VALUE_MASK		INTEL_MASK(25, 16)
# define GEN6_GS_ENABLE					(1 << 15)

# define BRW_GS_EDGE_INDICATOR_0			(1 << 8)
# define BRW_GS_EDGE_INDICATOR_1			(1 << 9)

#define _3DSTATE_HS                             0x781B /* GEN7+ */
#define _3DSTATE_TE                             0x781C /* GEN7+ */
#define _3DSTATE_DS                             0x781D /* GEN7+ */

#define _3DSTATE_CLIP				0x7812 /* GEN6+ */
/* DW1 */
# define GEN7_CLIP_WINDING_CW                           (0 << 20)
# define GEN7_CLIP_WINDING_CCW                          (1 << 20)
# define GEN7_CLIP_VERTEX_SUBPIXEL_PRECISION_8          (0 << 19)
# define GEN7_CLIP_VERTEX_SUBPIXEL_PRECISION_4          (1 << 19)
# define GEN7_CLIP_EARLY_CULL                           (1 << 18)
# define GEN7_CLIP_CULLMODE_BOTH                        (0 << 16)
# define GEN7_CLIP_CULLMODE_NONE                        (1 << 16)
# define GEN7_CLIP_CULLMODE_FRONT                       (2 << 16)
# define GEN7_CLIP_CULLMODE_BACK                        (3 << 16)
# define GEN6_CLIP_STATISTICS_ENABLE			(1 << 10)
/**
 * Just does cheap culling based on the clip distance.  Bits must be
 * disjoint with USER_CLIP_CLIP_DISTANCE bits.
 */
# define GEN6_USER_CLIP_CULL_DISTANCES_SHIFT		0
/* DW2 */
# define GEN6_CLIP_ENABLE				(1 << 31)
# define GEN6_CLIP_API_OGL				(0 << 30)
# define GEN6_CLIP_API_D3D				(1 << 30)
# define GEN6_CLIP_XY_TEST				(1 << 28)
# define GEN6_CLIP_Z_TEST				(1 << 27)
# define GEN6_CLIP_GB_TEST				(1 << 26)
/** 8-bit field of which user clip distances to clip aganist. */
# define GEN6_USER_CLIP_CLIP_DISTANCES_SHIFT		16
# define GEN6_CLIP_MODE_NORMAL				(0 << 13)
# define GEN6_CLIP_MODE_REJECT_ALL			(3 << 13)
# define GEN6_CLIP_MODE_ACCEPT_ALL			(4 << 13)
# define GEN6_CLIP_PERSPECTIVE_DIVIDE_DISABLE		(1 << 9)
# define GEN6_CLIP_NON_PERSPECTIVE_BARYCENTRIC_ENABLE	(1 << 8)
# define GEN6_CLIP_TRI_PROVOKE_SHIFT			4
# define GEN6_CLIP_LINE_PROVOKE_SHIFT			2
# define GEN6_CLIP_TRIFAN_PROVOKE_SHIFT			0
/* DW3 */
# define GEN6_CLIP_MIN_POINT_WIDTH_SHIFT		17
# define GEN6_CLIP_MAX_POINT_WIDTH_SHIFT		6
# define GEN6_CLIP_FORCE_ZERO_RTAINDEX			(1 << 5)

#define _3DSTATE_SF				0x7813 /* GEN6+ */
/* DW1 (for gen6) */
# define GEN6_SF_NUM_OUTPUTS_SHIFT			22
# define GEN6_SF_SWIZZLE_ENABLE				(1 << 21)
# define GEN6_SF_POINT_SPRITE_UPPERLEFT			(0 << 20)
# define GEN6_SF_POINT_SPRITE_LOWERLEFT			(1 << 20)
# define GEN6_SF_URB_ENTRY_READ_LENGTH_SHIFT		11
# define GEN6_SF_URB_ENTRY_READ_OFFSET_SHIFT		4
/* DW2 */
# define GEN6_SF_LEGACY_GLOBAL_DEPTH_BIAS		(1 << 11)
# define GEN6_SF_STATISTICS_ENABLE			(1 << 10)
# define GEN6_SF_GLOBAL_DEPTH_OFFSET_SOLID		(1 << 9)
# define GEN6_SF_GLOBAL_DEPTH_OFFSET_WIREFRAME		(1 << 8)
# define GEN6_SF_GLOBAL_DEPTH_OFFSET_POINT		(1 << 7)
# define GEN6_SF_FRONT_SOLID				(0 << 5)
# define GEN6_SF_FRONT_WIREFRAME			(1 << 5)
# define GEN6_SF_FRONT_POINT				(2 << 5)
# define GEN6_SF_BACK_SOLID				(0 << 3)
# define GEN6_SF_BACK_WIREFRAME				(1 << 3)
# define GEN6_SF_BACK_POINT				(2 << 3)
# define GEN6_SF_VIEWPORT_TRANSFORM_ENABLE		(1 << 1)
# define GEN6_SF_WINDING_CCW				(1 << 0)
/* DW3 */
# define GEN6_SF_LINE_AA_ENABLE				(1 << 31)
# define GEN6_SF_CULL_BOTH				(0 << 29)
# define GEN6_SF_CULL_NONE				(1 << 29)
# define GEN6_SF_CULL_FRONT				(2 << 29)
# define GEN6_SF_CULL_BACK				(3 << 29)
# define GEN6_SF_LINE_WIDTH_SHIFT			18 /* U3.7 */
# define GEN6_SF_LINE_END_CAP_WIDTH_0_5			(0 << 16)
# define GEN6_SF_LINE_END_CAP_WIDTH_1_0			(1 << 16)
# define GEN6_SF_LINE_END_CAP_WIDTH_2_0			(2 << 16)
# define GEN6_SF_LINE_END_CAP_WIDTH_4_0			(3 << 16)
# define GEN6_SF_SCISSOR_ENABLE				(1 << 11)
# define GEN6_SF_MSRAST_OFF_PIXEL			(0 << 8)
# define GEN6_SF_MSRAST_OFF_PATTERN			(1 << 8)
# define GEN6_SF_MSRAST_ON_PIXEL			(2 << 8)
# define GEN6_SF_MSRAST_ON_PATTERN			(3 << 8)
/* DW4 */
# define GEN6_SF_TRI_PROVOKE_SHIFT			29
# define GEN6_SF_LINE_PROVOKE_SHIFT			27
# define GEN6_SF_TRIFAN_PROVOKE_SHIFT			25
# define GEN6_SF_LINE_AA_MODE_MANHATTAN			(0 << 14)
# define GEN6_SF_LINE_AA_MODE_TRUE			(1 << 14)
# define GEN6_SF_VERTEX_SUBPIXEL_8BITS			(0 << 12)
# define GEN6_SF_VERTEX_SUBPIXEL_4BITS			(1 << 12)
# define GEN6_SF_USE_STATE_POINT_WIDTH			(1 << 11)
# define GEN6_SF_POINT_WIDTH_SHIFT			0 /* U8.3 */
/* DW5: depth offset constant */
/* DW6: depth offset scale */
/* DW7: depth offset clamp */
/* DW8 */
# define ATTRIBUTE_1_OVERRIDE_W				(1 << 31)
# define ATTRIBUTE_1_OVERRIDE_Z				(1 << 30)
# define ATTRIBUTE_1_OVERRIDE_Y				(1 << 29)
# define ATTRIBUTE_1_OVERRIDE_X				(1 << 28)
# define ATTRIBUTE_1_CONST_SOURCE_SHIFT			25
# define ATTRIBUTE_1_SWIZZLE_SHIFT			22
# define ATTRIBUTE_1_SOURCE_SHIFT			16
# define ATTRIBUTE_0_OVERRIDE_W				(1 << 15)
# define ATTRIBUTE_0_OVERRIDE_Z				(1 << 14)
# define ATTRIBUTE_0_OVERRIDE_Y				(1 << 13)
# define ATTRIBUTE_0_OVERRIDE_X				(1 << 12)
# define ATTRIBUTE_0_CONST_SOURCE_SHIFT			9
# define ATTRIBUTE_0_SWIZZLE_SHIFT			6
# define ATTRIBUTE_0_SOURCE_SHIFT			0

# define ATTRIBUTE_SWIZZLE_INPUTATTR                    0
# define ATTRIBUTE_SWIZZLE_INPUTATTR_FACING             1
# define ATTRIBUTE_SWIZZLE_INPUTATTR_W                  2
# define ATTRIBUTE_SWIZZLE_INPUTATTR_FACING_W           3
# define ATTRIBUTE_SWIZZLE_SHIFT                        6

/* DW16: Point sprite texture coordinate enables */
/* DW17: Constant interpolation enables */
/* DW18: attr 0-7 wrap shortest enables */
/* DW19: attr 8-16 wrap shortest enables */

/* On GEN7, many fields of 3DSTATE_SF were split out into a new command:
 * 3DSTATE_SBE.  The remaining fields live in different DWords, but retain
 * the same bit-offset.  The only new field:
 */
/* GEN7/DW1: */
# define GEN7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT	12
/* GEN7/DW2: */
# define HSW_SF_LINE_STIPPLE_ENABLE			14

#define _3DSTATE_SBE				0x781F /* GEN7+ */
/* DW1 */
# define GEN7_SBE_SWIZZLE_CONTROL_MODE			(1 << 28)
# define GEN7_SBE_NUM_OUTPUTS_SHIFT			22
# define GEN7_SBE_SWIZZLE_ENABLE			(1 << 21)
# define GEN7_SBE_POINT_SPRITE_LOWERLEFT		(1 << 20)
# define GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT		11
# define GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT		4
/* DW2-9: Attribute setup (same as DW8-15 of gen6 _3DSTATE_SF) */
/* DW10: Point sprite texture coordinate enables */
/* DW11: Constant interpolation enables */
/* DW12: attr 0-7 wrap shortest enables */
/* DW13: attr 8-16 wrap shortest enables */

enum brw_wm_barycentric_interp_mode {
   BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC		= 0,
   BRW_WM_PERSPECTIVE_CENTROID_BARYCENTRIC	= 1,
   BRW_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC	= 2,
   BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC	= 3,
   BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC	= 4,
   BRW_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC	= 5,
   BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT  = 6
};
#define BRW_WM_NONPERSPECTIVE_BARYCENTRIC_BITS \
   ((1 << BRW_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC) | \
    (1 << BRW_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC) | \
    (1 << BRW_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC))

#define _3DSTATE_WM				0x7814 /* GEN6+ */
/* DW1: kernel pointer */
/* DW2 */
# define GEN6_WM_SPF_MODE				(1 << 31)
# define GEN6_WM_VECTOR_MASK_ENABLE			(1 << 30)
# define GEN6_WM_SAMPLER_COUNT_SHIFT			27
# define GEN6_WM_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GEN6_WM_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GEN6_WM_FLOATING_POINT_MODE_ALT		(1 << 16)
/* DW3: scratch space */
/* DW4 */
# define GEN6_WM_STATISTICS_ENABLE			(1 << 31)
# define GEN6_WM_DEPTH_CLEAR				(1 << 30)
# define GEN6_WM_DEPTH_RESOLVE				(1 << 28)
# define GEN6_WM_HIERARCHICAL_DEPTH_RESOLVE		(1 << 27)
# define GEN6_WM_DISPATCH_START_GRF_SHIFT_0		16
# define GEN6_WM_DISPATCH_START_GRF_SHIFT_1		8
# define GEN6_WM_DISPATCH_START_GRF_SHIFT_2		0
/* DW5 */
# define GEN6_WM_MAX_THREADS_SHIFT			25
# define GEN6_WM_KILL_ENABLE				(1 << 22)
# define GEN6_WM_COMPUTED_DEPTH				(1 << 21)
# define GEN6_WM_USES_SOURCE_DEPTH			(1 << 20)
# define GEN6_WM_DISPATCH_ENABLE			(1 << 19)
# define GEN6_WM_LINE_END_CAP_AA_WIDTH_0_5		(0 << 16)
# define GEN6_WM_LINE_END_CAP_AA_WIDTH_1_0		(1 << 16)
# define GEN6_WM_LINE_END_CAP_AA_WIDTH_2_0		(2 << 16)
# define GEN6_WM_LINE_END_CAP_AA_WIDTH_4_0		(3 << 16)
# define GEN6_WM_LINE_AA_WIDTH_0_5			(0 << 14)
# define GEN6_WM_LINE_AA_WIDTH_1_0			(1 << 14)
# define GEN6_WM_LINE_AA_WIDTH_2_0			(2 << 14)
# define GEN6_WM_LINE_AA_WIDTH_4_0			(3 << 14)
# define GEN6_WM_POLYGON_STIPPLE_ENABLE			(1 << 13)
# define GEN6_WM_LINE_STIPPLE_ENABLE			(1 << 11)
# define GEN6_WM_OMASK_TO_RENDER_TARGET			(1 << 9)
# define GEN6_WM_USES_SOURCE_W				(1 << 8)
# define GEN6_WM_DUAL_SOURCE_BLEND_ENABLE		(1 << 7)
# define GEN6_WM_32_DISPATCH_ENABLE			(1 << 2)
# define GEN6_WM_16_DISPATCH_ENABLE			(1 << 1)
# define GEN6_WM_8_DISPATCH_ENABLE			(1 << 0)
/* DW6 */
# define GEN6_WM_NUM_SF_OUTPUTS_SHIFT			20
# define GEN6_WM_POSOFFSET_NONE				(0 << 18)
# define GEN6_WM_POSOFFSET_CENTROID			(2 << 18)
# define GEN6_WM_POSOFFSET_SAMPLE			(3 << 18)
# define GEN6_WM_POSITION_ZW_PIXEL			(0 << 16)
# define GEN6_WM_POSITION_ZW_CENTROID			(2 << 16)
# define GEN6_WM_POSITION_ZW_SAMPLE			(3 << 16)
# define GEN6_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC	(1 << 15)
# define GEN6_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 14)
# define GEN6_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC	(1 << 13)
# define GEN6_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC		(1 << 12)
# define GEN6_WM_PERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 11)
# define GEN6_WM_PERSPECTIVE_PIXEL_BARYCENTRIC		(1 << 10)
# define GEN6_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT   10
# define GEN6_WM_POINT_RASTRULE_UPPER_RIGHT		(1 << 9)
# define GEN6_WM_MSRAST_OFF_PIXEL			(0 << 1)
# define GEN6_WM_MSRAST_OFF_PATTERN			(1 << 1)
# define GEN6_WM_MSRAST_ON_PIXEL			(2 << 1)
# define GEN6_WM_MSRAST_ON_PATTERN			(3 << 1)
# define GEN6_WM_MSDISPMODE_PERSAMPLE			(0 << 0)
# define GEN6_WM_MSDISPMODE_PERPIXEL			(1 << 0)
/* DW7: kernel 1 pointer */
/* DW8: kernel 2 pointer */

#define _3DSTATE_CONSTANT_VS		      0x7815 /* GEN6+ */
#define _3DSTATE_CONSTANT_GS		      0x7816 /* GEN6+ */
#define _3DSTATE_CONSTANT_PS		      0x7817 /* GEN6+ */
# define GEN6_CONSTANT_BUFFER_3_ENABLE			(1 << 15)
# define GEN6_CONSTANT_BUFFER_2_ENABLE			(1 << 14)
# define GEN6_CONSTANT_BUFFER_1_ENABLE			(1 << 13)
# define GEN6_CONSTANT_BUFFER_0_ENABLE			(1 << 12)

#define _3DSTATE_CONSTANT_HS                  0x7819 /* GEN7+ */
#define _3DSTATE_CONSTANT_DS                  0x781A /* GEN7+ */

#define _3DSTATE_STREAMOUT                    0x781e /* GEN7+ */
/* DW1 */
# define SO_FUNCTION_ENABLE				(1 << 31)
# define SO_RENDERING_DISABLE				(1 << 30)
/* This selects which incoming rendering stream goes down the pipeline.  The
 * rendering stream is 0 if not defined by special cases in the GS state.
 */
# define SO_RENDER_STREAM_SELECT_SHIFT			27
# define SO_RENDER_STREAM_SELECT_MASK			INTEL_MASK(28, 27)
/* Controls reordering of TRISTRIP_* elements in stream output (not rendering).
 */
# define SO_REORDER_TRAILING				(1 << 26)
/* Controls SO_NUM_PRIMS_WRITTEN_* and SO_PRIM_STORAGE_* */
# define SO_STATISTICS_ENABLE				(1 << 25)
# define SO_BUFFER_ENABLE(n)				(1 << (8 + (n)))
/* DW2 */
# define SO_STREAM_3_VERTEX_READ_OFFSET_SHIFT		29
# define SO_STREAM_3_VERTEX_READ_OFFSET_MASK		INTEL_MASK(29, 29)
# define SO_STREAM_3_VERTEX_READ_LENGTH_SHIFT		24
# define SO_STREAM_3_VERTEX_READ_LENGTH_MASK		INTEL_MASK(28, 24)
# define SO_STREAM_2_VERTEX_READ_OFFSET_SHIFT		21
# define SO_STREAM_2_VERTEX_READ_OFFSET_MASK		INTEL_MASK(21, 21)
# define SO_STREAM_2_VERTEX_READ_LENGTH_SHIFT		16
# define SO_STREAM_2_VERTEX_READ_LENGTH_MASK		INTEL_MASK(20, 16)
# define SO_STREAM_1_VERTEX_READ_OFFSET_SHIFT		13
# define SO_STREAM_1_VERTEX_READ_OFFSET_MASK		INTEL_MASK(13, 13)
# define SO_STREAM_1_VERTEX_READ_LENGTH_SHIFT		8
# define SO_STREAM_1_VERTEX_READ_LENGTH_MASK		INTEL_MASK(12, 8)
# define SO_STREAM_0_VERTEX_READ_OFFSET_SHIFT		5
# define SO_STREAM_0_VERTEX_READ_OFFSET_MASK		INTEL_MASK(5, 5)
# define SO_STREAM_0_VERTEX_READ_LENGTH_SHIFT		0
# define SO_STREAM_0_VERTEX_READ_LENGTH_MASK		INTEL_MASK(4, 0)

/* 3DSTATE_WM for Gen7 */
/* DW1 */
# define GEN7_WM_STATISTICS_ENABLE			(1 << 31)
# define GEN7_WM_DEPTH_CLEAR				(1 << 30)
# define GEN7_WM_DISPATCH_ENABLE			(1 << 29)
# define GEN7_WM_DEPTH_RESOLVE				(1 << 28)
# define GEN7_WM_HIERARCHICAL_DEPTH_RESOLVE		(1 << 27)
# define GEN7_WM_KILL_ENABLE				(1 << 25)
# define GEN7_WM_PSCDEPTH_OFF			        (0 << 23)
# define GEN7_WM_PSCDEPTH_ON			        (1 << 23)
# define GEN7_WM_PSCDEPTH_ON_GE			        (2 << 23)
# define GEN7_WM_PSCDEPTH_ON_LE			        (3 << 23)
# define GEN7_WM_USES_SOURCE_DEPTH			(1 << 20)
# define GEN7_WM_USES_SOURCE_W			        (1 << 19)
# define GEN7_WM_POSITION_ZW_PIXEL			(0 << 17)
# define GEN7_WM_POSITION_ZW_CENTROID			(2 << 17)
# define GEN7_WM_POSITION_ZW_SAMPLE			(3 << 17)
# define GEN7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT   11
# define GEN7_WM_USES_INPUT_COVERAGE_MASK	        (1 << 10)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_0_5		(0 << 8)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_1_0		(1 << 8)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_2_0		(2 << 8)
# define GEN7_WM_LINE_END_CAP_AA_WIDTH_4_0		(3 << 8)
# define GEN7_WM_LINE_AA_WIDTH_0_5			(0 << 6)
# define GEN7_WM_LINE_AA_WIDTH_1_0			(1 << 6)
# define GEN7_WM_LINE_AA_WIDTH_2_0			(2 << 6)
# define GEN7_WM_LINE_AA_WIDTH_4_0			(3 << 6)
# define GEN7_WM_POLYGON_STIPPLE_ENABLE			(1 << 4)
# define GEN7_WM_LINE_STIPPLE_ENABLE			(1 << 3)
# define GEN7_WM_POINT_RASTRULE_UPPER_RIGHT		(1 << 2)
# define GEN7_WM_MSRAST_OFF_PIXEL			(0 << 0)
# define GEN7_WM_MSRAST_OFF_PATTERN			(1 << 0)
# define GEN7_WM_MSRAST_ON_PIXEL			(2 << 0)
# define GEN7_WM_MSRAST_ON_PATTERN			(3 << 0)
/* DW2 */
# define GEN7_WM_MSDISPMODE_PERSAMPLE			(0 << 31)
# define GEN7_WM_MSDISPMODE_PERPIXEL			(1 << 31)

#define _3DSTATE_PS				0x7820 /* GEN7+ */
/* DW1: kernel pointer */
/* DW2 */
# define GEN7_PS_SPF_MODE				(1 << 31)
# define GEN7_PS_VECTOR_MASK_ENABLE			(1 << 30)
# define GEN7_PS_SAMPLER_COUNT_SHIFT			27
# define GEN7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GEN7_PS_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GEN7_PS_FLOATING_POINT_MODE_ALT		(1 << 16)
/* DW3: scratch space */
/* DW4 */
# define IVB_PS_MAX_THREADS_SHIFT			24
# define HSW_PS_MAX_THREADS_SHIFT			23
# define HSW_PS_SAMPLE_MASK_SHIFT		        12
# define HSW_PS_SAMPLE_MASK_MASK			INTEL_MASK(19, 12)
# define GEN7_PS_PUSH_CONSTANT_ENABLE		        (1 << 11)
# define GEN7_PS_ATTRIBUTE_ENABLE		        (1 << 10)
# define GEN7_PS_OMASK_TO_RENDER_TARGET			(1 << 9)
# define GEN7_PS_DUAL_SOURCE_BLEND_ENABLE		(1 << 7)
# define GEN7_PS_POSOFFSET_NONE				(0 << 3)
# define GEN7_PS_POSOFFSET_CENTROID			(2 << 3)
# define GEN7_PS_POSOFFSET_SAMPLE			(3 << 3)
# define GEN7_PS_32_DISPATCH_ENABLE			(1 << 2)
# define GEN7_PS_16_DISPATCH_ENABLE			(1 << 1)
# define GEN7_PS_8_DISPATCH_ENABLE			(1 << 0)
/* DW5 */
# define GEN7_PS_DISPATCH_START_GRF_SHIFT_0		16
# define GEN7_PS_DISPATCH_START_GRF_SHIFT_1		8
# define GEN7_PS_DISPATCH_START_GRF_SHIFT_2		0
/* DW6: kernel 1 pointer */
/* DW7: kernel 2 pointer */

#define _3DSTATE_SAMPLE_MASK			0x7818 /* GEN6+ */

#define _3DSTATE_DRAWING_RECTANGLE		0x7900
#define _3DSTATE_BLEND_CONSTANT_COLOR		0x7901
#define _3DSTATE_CHROMA_KEY			0x7904
#define _3DSTATE_DEPTH_BUFFER			0x7905 /* GEN4-6 */
#define _3DSTATE_POLY_STIPPLE_OFFSET		0x7906
#define _3DSTATE_POLY_STIPPLE_PATTERN		0x7907
#define _3DSTATE_LINE_STIPPLE_PATTERN		0x7908
#define _3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP	0x7909
#define _3DSTATE_AA_LINE_PARAMETERS		0x790a /* G45+ */

#define _3DSTATE_GS_SVB_INDEX			0x790b /* CTG+ */
/* DW1 */
# define SVB_INDEX_SHIFT				29
# define SVB_LOAD_INTERNAL_VERTEX_COUNT			(1 << 0) /* SNB+ */
/* DW2: SVB index */
/* DW3: SVB maximum index */

#define _3DSTATE_MULTISAMPLE			0x790d /* GEN6+ */
/* DW1 */
# define MS_PIXEL_LOCATION_CENTER			(0 << 4)
# define MS_PIXEL_LOCATION_UPPER_LEFT			(1 << 4)
# define MS_NUMSAMPLES_1				(0 << 1)
# define MS_NUMSAMPLES_4				(2 << 1)
# define MS_NUMSAMPLES_8				(3 << 1)

#define _3DSTATE_STENCIL_BUFFER			0x790e /* ILK, SNB */
#define _3DSTATE_HIER_DEPTH_BUFFER		0x790f /* ILK, SNB */

#define GEN7_3DSTATE_CLEAR_PARAMS		0x7804
#define GEN7_3DSTATE_DEPTH_BUFFER		0x7805
#define GEN7_3DSTATE_STENCIL_BUFFER		0x7806
# define HSW_STENCIL_ENABLED                            (1 << 31)
#define GEN7_3DSTATE_HIER_DEPTH_BUFFER		0x7807

#define _3DSTATE_CLEAR_PARAMS			0x7910 /* ILK, SNB */
# define GEN5_DEPTH_CLEAR_VALID				(1 << 15)
/* DW1: depth clear value */
/* DW2 */
# define GEN7_DEPTH_CLEAR_VALID				(1 << 0)

#define _3DSTATE_SO_DECL_LIST			0x7917 /* GEN7+ */
/* DW1 */
# define SO_STREAM_TO_BUFFER_SELECTS_3_SHIFT		12
# define SO_STREAM_TO_BUFFER_SELECTS_3_MASK		INTEL_MASK(15, 12)
# define SO_STREAM_TO_BUFFER_SELECTS_2_SHIFT		8
# define SO_STREAM_TO_BUFFER_SELECTS_2_MASK		INTEL_MASK(11, 8)
# define SO_STREAM_TO_BUFFER_SELECTS_1_SHIFT		4
# define SO_STREAM_TO_BUFFER_SELECTS_1_MASK		INTEL_MASK(7, 4)
# define SO_STREAM_TO_BUFFER_SELECTS_0_SHIFT		0
# define SO_STREAM_TO_BUFFER_SELECTS_0_MASK		INTEL_MASK(3, 0)
/* DW2 */
# define SO_NUM_ENTRIES_3_SHIFT				24
# define SO_NUM_ENTRIES_3_MASK				INTEL_MASK(31, 24)
# define SO_NUM_ENTRIES_2_SHIFT				16
# define SO_NUM_ENTRIES_2_MASK				INTEL_MASK(23, 16)
# define SO_NUM_ENTRIES_1_SHIFT				8
# define SO_NUM_ENTRIES_1_MASK				INTEL_MASK(15, 8)
# define SO_NUM_ENTRIES_0_SHIFT				0
# define SO_NUM_ENTRIES_0_MASK				INTEL_MASK(7, 0)

/* SO_DECL DW0 */
# define SO_DECL_OUTPUT_BUFFER_SLOT_SHIFT		12
# define SO_DECL_OUTPUT_BUFFER_SLOT_MASK		INTEL_MASK(13, 12)
# define SO_DECL_HOLE_FLAG				(1 << 11)
# define SO_DECL_REGISTER_INDEX_SHIFT			4
# define SO_DECL_REGISTER_INDEX_MASK			INTEL_MASK(9, 4)
# define SO_DECL_COMPONENT_MASK_SHIFT			0
# define SO_DECL_COMPONENT_MASK_MASK			INTEL_MASK(3, 0)

#define _3DSTATE_SO_BUFFER                    0x7918 /* GEN7+ */
/* DW1 */
# define SO_BUFFER_INDEX_SHIFT				29
# define SO_BUFFER_INDEX_MASK				INTEL_MASK(30, 29)
# define SO_BUFFER_PITCH_SHIFT				0
# define SO_BUFFER_PITCH_MASK				INTEL_MASK(11, 0)
/* DW2: start address */
/* DW3: end address. */

#define CMD_PIPE_CONTROL              0x7a00

#define CMD_MI_FLUSH                  0x0200


/* Bitfields for the URB_WRITE message, DW2 of message header: */
#define URB_WRITE_PRIM_END		0x1
#define URB_WRITE_PRIM_START		0x2
#define URB_WRITE_PRIM_TYPE_SHIFT	2


/* Maximum number of entries that can be addressed using a binding table
 * pointer of type SURFTYPE_BUFFER
 */
#define BRW_MAX_NUM_BUFFER_ENTRIES	(1 << 27)

#include "intel_chipset.h"

#endif
