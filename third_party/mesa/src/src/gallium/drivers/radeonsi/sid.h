/*
 * Southern Islands Register documentation
 *
 * Copyright (C) 2011  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SID_H
#define SID_H

/* si values */
#define SI_CONFIG_REG_OFFSET                 0x00008000
#define SI_CONFIG_REG_END                    0x0000B000
#define SI_SH_REG_OFFSET                     0x0000B000
#define SI_SH_REG_END                        0x0000C000
#define SI_CONTEXT_REG_OFFSET                0x00028000
#define SI_CONTEXT_REG_END                   0x00029000

#define EVENT_TYPE_PS_PARTIAL_FLUSH            0x10
#define EVENT_TYPE_CACHE_FLUSH_AND_INV_TS_EVENT 0x14
#define EVENT_TYPE_ZPASS_DONE                  0x15
#define EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT   0x16
#define EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH	0x1f
#define EVENT_TYPE_SAMPLE_STREAMOUTSTATS	0x20
#define		EVENT_TYPE(x)                           ((x) << 0)
#define		EVENT_INDEX(x)                          ((x) << 8)
                /* 0 - any non-TS event
		 * 1 - ZPASS_DONE
		 * 2 - SAMPLE_PIPELINESTAT
		 * 3 - SAMPLE_STREAMOUTSTAT*
		 * 4 - *S_PARTIAL_FLUSH
		 * 5 - TS events
		 */

#define PREDICATION_OP_CLEAR 0x0
#define PREDICATION_OP_ZPASS 0x1
#define PREDICATION_OP_PRIMCOUNT 0x2

#define PRED_OP(x) ((x) << 16)

#define PREDICATION_CONTINUE (1 << 31)

#define PREDICATION_HINT_WAIT (0 << 12)
#define PREDICATION_HINT_NOWAIT_DRAW (1 << 12)

#define PREDICATION_DRAW_NOT_VISIBLE (0 << 8)
#define PREDICATION_DRAW_VISIBLE (1 << 8)

#define R600_TEXEL_PITCH_ALIGNMENT_MASK        0x7

#define PKT3_NOP                               0x10
#define PKT3_SET_PREDICATION                   0x20
#define PKT3_COND_EXEC                         0x22
#define PKT3_PRED_EXEC                         0x23
#define PKT3_START_3D_CMDBUF                   0x24
#define PKT3_DRAW_INDEX_2                      0x27
#define PKT3_CONTEXT_CONTROL                   0x28
#define PKT3_INDEX_TYPE                        0x2A
#define PKT3_DRAW_INDEX                        0x2B
#define PKT3_DRAW_INDEX_AUTO                   0x2D
#define PKT3_DRAW_INDEX_IMMD                   0x2E
#define PKT3_NUM_INSTANCES                     0x2F
#define PKT3_STRMOUT_BUFFER_UPDATE             0x34
#define PKT3_MEM_SEMAPHORE                     0x39
#define PKT3_MPEG_INDEX                        0x3A
#define PKT3_WAIT_REG_MEM                      0x3C
#define		WAIT_REG_MEM_EQUAL		3
#define PKT3_MEM_WRITE                         0x3D
#define PKT3_INDIRECT_BUFFER                   0x32
#define PKT3_SURFACE_SYNC                      0x43
#define PKT3_ME_INITIALIZE                     0x44
#define PKT3_COND_WRITE                        0x45
#define PKT3_EVENT_WRITE                       0x46
#define PKT3_EVENT_WRITE_EOP                   0x47
#define PKT3_EVENT_WRITE_EOS                   0x48
#define PKT3_ONE_REG_WRITE                     0x57
#define PKT3_SET_CONFIG_REG                    0x68
#define PKT3_SET_CONTEXT_REG                   0x69
#define PKT3_SET_SH_REG                        0x76
#define PKT3_SET_SH_REG_OFFSET                 0x77

#define PKT_TYPE_S(x)                   (((x) & 0x3) << 30)
#define PKT_TYPE_G(x)                   (((x) >> 30) & 0x3)
#define PKT_TYPE_C                      0x3FFFFFFF
#define PKT_COUNT_S(x)                  (((x) & 0x3FFF) << 16)
#define PKT_COUNT_G(x)                  (((x) >> 16) & 0x3FFF)
#define PKT_COUNT_C                     0xC000FFFF
#define PKT0_BASE_INDEX_S(x)            (((x) & 0xFFFF) << 0)
#define PKT0_BASE_INDEX_G(x)            (((x) >> 0) & 0xFFFF)
#define PKT0_BASE_INDEX_C               0xFFFF0000
#define PKT3_IT_OPCODE_S(x)             (((x) & 0xFF) << 8)
#define PKT3_IT_OPCODE_G(x)             (((x) >> 8) & 0xFF)
#define PKT3_IT_OPCODE_C                0xFFFF00FF
#define PKT3_PREDICATE(x)               (((x) >> 0) & 0x1)
#define PKT0(index, count) (PKT_TYPE_S(0) | PKT0_BASE_INDEX_S(index) | PKT_COUNT_S(count))
#define PKT3(op, count, predicate) (PKT_TYPE_S(3) | PKT3_IT_OPCODE_S(op) | PKT_COUNT_S(count) | PKT3_PREDICATE(predicate))

#define R_0084FC_CP_STRMOUT_CNTL		                        0x0084FC
#define   S_0084FC_OFFSET_UPDATE_DONE(x)		              (((x) & 0x1) << 0)
#define R_0085F0_CP_COHER_CNTL                                          0x0085F0
#define   S_0085F0_DEST_BASE_0_ENA(x)                                 (((x) & 0x1) << 0)
#define   G_0085F0_DEST_BASE_0_ENA(x)                                 (((x) >> 0) & 0x1)
#define   C_0085F0_DEST_BASE_0_ENA                                    0xFFFFFFFE
#define   S_0085F0_DEST_BASE_1_ENA(x)                                 (((x) & 0x1) << 1)
#define   G_0085F0_DEST_BASE_1_ENA(x)                                 (((x) >> 1) & 0x1)
#define   C_0085F0_DEST_BASE_1_ENA                                    0xFFFFFFFD
#define   S_0085F0_CB0_DEST_BASE_ENA_SHIFT             	              6
#define   S_0085F0_CB0_DEST_BASE_ENA(x)                               (((x) & 0x1) << 6)
#define   G_0085F0_CB0_DEST_BASE_ENA(x)                               (((x) >> 6) & 0x1)
#define   C_0085F0_CB0_DEST_BASE_ENA                                  0xFFFFFFBF
#define   S_0085F0_CB1_DEST_BASE_ENA(x)                               (((x) & 0x1) << 7)
#define   G_0085F0_CB1_DEST_BASE_ENA(x)                               (((x) >> 7) & 0x1)
#define   C_0085F0_CB1_DEST_BASE_ENA                                  0xFFFFFF7F
#define   S_0085F0_CB2_DEST_BASE_ENA(x)                               (((x) & 0x1) << 8)
#define   G_0085F0_CB2_DEST_BASE_ENA(x)                               (((x) >> 8) & 0x1)
#define   C_0085F0_CB2_DEST_BASE_ENA                                  0xFFFFFEFF
#define   S_0085F0_CB3_DEST_BASE_ENA(x)                               (((x) & 0x1) << 9)
#define   G_0085F0_CB3_DEST_BASE_ENA(x)                               (((x) >> 9) & 0x1)
#define   C_0085F0_CB3_DEST_BASE_ENA                                  0xFFFFFDFF
#define   S_0085F0_CB4_DEST_BASE_ENA(x)                               (((x) & 0x1) << 10)
#define   G_0085F0_CB4_DEST_BASE_ENA(x)                               (((x) >> 10) & 0x1)
#define   C_0085F0_CB4_DEST_BASE_ENA                                  0xFFFFFBFF
#define   S_0085F0_CB5_DEST_BASE_ENA(x)                               (((x) & 0x1) << 11)
#define   G_0085F0_CB5_DEST_BASE_ENA(x)                               (((x) >> 11) & 0x1)
#define   C_0085F0_CB5_DEST_BASE_ENA                                  0xFFFFF7FF
#define   S_0085F0_CB6_DEST_BASE_ENA(x)                               (((x) & 0x1) << 12)
#define   G_0085F0_CB6_DEST_BASE_ENA(x)                               (((x) >> 12) & 0x1)
#define   C_0085F0_CB6_DEST_BASE_ENA                                  0xFFFFEFFF
#define   S_0085F0_CB7_DEST_BASE_ENA(x)                               (((x) & 0x1) << 13)
#define   G_0085F0_CB7_DEST_BASE_ENA(x)                               (((x) >> 13) & 0x1)
#define   C_0085F0_CB7_DEST_BASE_ENA                                  0xFFFFDFFF
#define   S_0085F0_DB_DEST_BASE_ENA(x)                                (((x) & 0x1) << 14)
#define   G_0085F0_DB_DEST_BASE_ENA(x)                                (((x) >> 14) & 0x1)
#define   C_0085F0_DB_DEST_BASE_ENA                                   0xFFFFBFFF
#define   S_0085F0_DEST_BASE_2_ENA(x)                                 (((x) & 0x1) << 19)
#define   G_0085F0_DEST_BASE_2_ENA(x)                                 (((x) >> 19) & 0x1)
#define   C_0085F0_DEST_BASE_2_ENA                                    0xFFF7FFFF
#define   S_0085F0_DEST_BASE_3_ENA(x)                                 (((x) & 0x1) << 21)
#define   G_0085F0_DEST_BASE_3_ENA(x)                                 (((x) >> 21) & 0x1)
#define   C_0085F0_DEST_BASE_3_ENA                                    0xFFDFFFFF
#define   S_0085F0_TCL1_ACTION_ENA(x)                                 (((x) & 0x1) << 22)
#define   G_0085F0_TCL1_ACTION_ENA(x)                                 (((x) >> 22) & 0x1)
#define   C_0085F0_TCL1_ACTION_ENA                                    0xFFBFFFFF
#define   S_0085F0_TC_ACTION_ENA(x)                                   (((x) & 0x1) << 23)
#define   G_0085F0_TC_ACTION_ENA(x)                                   (((x) >> 23) & 0x1)
#define   C_0085F0_TC_ACTION_ENA                                      0xFF7FFFFF
#define   S_0085F0_CB_ACTION_ENA(x)                                   (((x) & 0x1) << 25)
#define   G_0085F0_CB_ACTION_ENA(x)                                   (((x) >> 25) & 0x1)
#define   C_0085F0_CB_ACTION_ENA                                      0xFDFFFFFF
#define   S_0085F0_DB_ACTION_ENA(x)                                   (((x) & 0x1) << 26)
#define   G_0085F0_DB_ACTION_ENA(x)                                   (((x) >> 26) & 0x1)
#define   C_0085F0_DB_ACTION_ENA                                      0xFBFFFFFF
#define   S_0085F0_SH_KCACHE_ACTION_ENA(x)                            (((x) & 0x1) << 27)
#define   G_0085F0_SH_KCACHE_ACTION_ENA(x)                            (((x) >> 27) & 0x1)
#define   C_0085F0_SH_KCACHE_ACTION_ENA                               0xF7FFFFFF
#define   S_0085F0_SH_ICACHE_ACTION_ENA(x)                            (((x) & 0x1) << 29)
#define   G_0085F0_SH_ICACHE_ACTION_ENA(x)                            (((x) >> 29) & 0x1)
#define   C_0085F0_SH_ICACHE_ACTION_ENA                               0xDFFFFFFF
#define R_0085F4_CP_COHER_SIZE                                          0x0085F4
#define R_0085F8_CP_COHER_BASE                                          0x0085F8
#define R_0088B0_VGT_VTX_VECT_EJECT_REG                                 0x0088B0
#define   S_0088B0_PRIM_COUNT(x)                                      (((x) & 0x3FF) << 0)
#define   G_0088B0_PRIM_COUNT(x)                                      (((x) >> 0) & 0x3FF)
#define   C_0088B0_PRIM_COUNT                                         0xFFFFFC00
#define R_0088C4_VGT_CACHE_INVALIDATION                                 0x0088C4
#define   S_0088C4_VS_NO_EXTRA_BUFFER(x)                              (((x) & 0x1) << 5)
#define   G_0088C4_VS_NO_EXTRA_BUFFER(x)                              (((x) >> 5) & 0x1)
#define   C_0088C4_VS_NO_EXTRA_BUFFER                                 0xFFFFFFDF
#define   S_0088C4_STREAMOUT_FULL_FLUSH(x)                            (((x) & 0x1) << 13)
#define   G_0088C4_STREAMOUT_FULL_FLUSH(x)                            (((x) >> 13) & 0x1)
#define   C_0088C4_STREAMOUT_FULL_FLUSH                               0xFFFFDFFF
#define   S_0088C4_ES_LIMIT(x)                                        (((x) & 0x1F) << 16)
#define   G_0088C4_ES_LIMIT(x)                                        (((x) >> 16) & 0x1F)
#define   C_0088C4_ES_LIMIT                                           0xFFE0FFFF
#define R_0088C8_VGT_ESGS_RING_SIZE                                     0x0088C8
#define R_0088CC_VGT_GSVS_RING_SIZE                                     0x0088CC
#define R_0088D4_VGT_GS_VERTEX_REUSE                                    0x0088D4
#define   S_0088D4_VERT_REUSE(x)                                      (((x) & 0x1F) << 0)
#define   G_0088D4_VERT_REUSE(x)                                      (((x) >> 0) & 0x1F)
#define   C_0088D4_VERT_REUSE                                         0xFFFFFFE0
#define R_008958_VGT_PRIMITIVE_TYPE                                     0x008958
#define   S_008958_PRIM_TYPE(x)                                       (((x) & 0x3F) << 0)
#define   G_008958_PRIM_TYPE(x)                                       (((x) >> 0) & 0x3F)
#define   C_008958_PRIM_TYPE                                          0xFFFFFFC0
#define     V_008958_DI_PT_NONE                                     0x00
#define     V_008958_DI_PT_POINTLIST                                0x01
#define     V_008958_DI_PT_LINELIST                                 0x02
#define     V_008958_DI_PT_LINESTRIP                                0x03
#define     V_008958_DI_PT_TRILIST                                  0x04
#define     V_008958_DI_PT_TRIFAN                                   0x05
#define     V_008958_DI_PT_TRISTRIP                                 0x06
#define     V_008958_DI_PT_UNUSED_0                                 0x07
#define     V_008958_DI_PT_UNUSED_1                                 0x08
#define     V_008958_DI_PT_PATCH                                    0x09
#define     V_008958_DI_PT_LINELIST_ADJ                             0x0A
#define     V_008958_DI_PT_LINESTRIP_ADJ                            0x0B
#define     V_008958_DI_PT_TRILIST_ADJ                              0x0C
#define     V_008958_DI_PT_TRISTRIP_ADJ                             0x0D
#define     V_008958_DI_PT_UNUSED_3                                 0x0E
#define     V_008958_DI_PT_UNUSED_4                                 0x0F
#define     V_008958_DI_PT_TRI_WITH_WFLAGS                          0x10
#define     V_008958_DI_PT_RECTLIST                                 0x11
#define     V_008958_DI_PT_LINELOOP                                 0x12
#define     V_008958_DI_PT_QUADLIST                                 0x13
#define     V_008958_DI_PT_QUADSTRIP                                0x14
#define     V_008958_DI_PT_POLYGON                                  0x15
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V0                     0x16
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V1                     0x17
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V2                     0x18
#define     V_008958_DI_PT_2D_COPY_RECT_LIST_V3                     0x19
#define     V_008958_DI_PT_2D_FILL_RECT_LIST                        0x1A
#define     V_008958_DI_PT_2D_LINE_STRIP                            0x1B
#define     V_008958_DI_PT_2D_TRI_STRIP                             0x1C
#define R_00895C_VGT_INDEX_TYPE                                         0x00895C
#define   S_00895C_INDEX_TYPE(x)                                      (((x) & 0x03) << 0)
#define   G_00895C_INDEX_TYPE(x)                                      (((x) >> 0) & 0x03)
#define   C_00895C_INDEX_TYPE                                         0xFFFFFFFC
#define     V_00895C_DI_INDEX_SIZE_16_BIT                           0x00
#define     V_00895C_DI_INDEX_SIZE_32_BIT                           0x01
#define R_008960_VGT_STRMOUT_BUFFER_FILLED_SIZE_0                       0x008960
#define R_008964_VGT_STRMOUT_BUFFER_FILLED_SIZE_1                       0x008964
#define R_008968_VGT_STRMOUT_BUFFER_FILLED_SIZE_2                       0x008968
#define R_00896C_VGT_STRMOUT_BUFFER_FILLED_SIZE_3                       0x00896C
#define R_008970_VGT_NUM_INDICES                                        0x008970
#define R_008974_VGT_NUM_INSTANCES                                      0x008974
#define R_008988_VGT_TF_RING_SIZE                                       0x008988
#define   S_008988_SIZE(x)                                            (((x) & 0xFFFF) << 0)
#define   G_008988_SIZE(x)                                            (((x) >> 0) & 0xFFFF)
#define   C_008988_SIZE                                               0xFFFF0000
#define R_0089B0_VGT_HS_OFFCHIP_PARAM                                   0x0089B0
#define   S_0089B0_OFFCHIP_BUFFERING(x)                               (((x) & 0x7F) << 0)
#define   G_0089B0_OFFCHIP_BUFFERING(x)                               (((x) >> 0) & 0x7F)
#define   C_0089B0_OFFCHIP_BUFFERING                                  0xFFFFFF80
#define R_0089B8_VGT_TF_MEMORY_BASE                                     0x0089B8
#define R_008A14_PA_CL_ENHANCE                                          0x008A14
#define   S_008A14_CLIP_VTX_REORDER_ENA(x)                            (((x) & 0x1) << 0)
#define   G_008A14_CLIP_VTX_REORDER_ENA(x)                            (((x) >> 0) & 0x1)
#define   C_008A14_CLIP_VTX_REORDER_ENA                               0xFFFFFFFE
#define   S_008A14_NUM_CLIP_SEQ(x)                                    (((x) & 0x03) << 1)
#define   G_008A14_NUM_CLIP_SEQ(x)                                    (((x) >> 1) & 0x03)
#define   C_008A14_NUM_CLIP_SEQ                                       0xFFFFFFF9
#define   S_008A14_CLIPPED_PRIM_SEQ_STALL(x)                          (((x) & 0x1) << 3)
#define   G_008A14_CLIPPED_PRIM_SEQ_STALL(x)                          (((x) >> 3) & 0x1)
#define   C_008A14_CLIPPED_PRIM_SEQ_STALL                             0xFFFFFFF7
#define   S_008A14_VE_NAN_PROC_DISABLE(x)                             (((x) & 0x1) << 4)
#define   G_008A14_VE_NAN_PROC_DISABLE(x)                             (((x) >> 4) & 0x1)
#define   C_008A14_VE_NAN_PROC_DISABLE                                0xFFFFFFEF
#define R_008A60_PA_SU_LINE_STIPPLE_VALUE                               0x008A60
#define   S_008A60_LINE_STIPPLE_VALUE(x)                              (((x) & 0xFFFFFF) << 0)
#define   G_008A60_LINE_STIPPLE_VALUE(x)                              (((x) >> 0) & 0xFFFFFF)
#define   C_008A60_LINE_STIPPLE_VALUE                                 0xFF000000
#define R_008B10_PA_SC_LINE_STIPPLE_STATE                               0x008B10
#define   S_008B10_CURRENT_PTR(x)                                     (((x) & 0x0F) << 0)
#define   G_008B10_CURRENT_PTR(x)                                     (((x) >> 0) & 0x0F)
#define   C_008B10_CURRENT_PTR                                        0xFFFFFFF0
#define   S_008B10_CURRENT_COUNT(x)                                   (((x) & 0xFF) << 8)
#define   G_008B10_CURRENT_COUNT(x)                                   (((x) >> 8) & 0xFF)
#define   C_008B10_CURRENT_COUNT                                      0xFFFF00FF
#define R_008BF0_PA_SC_ENHANCE                                          0x008BF0
#define   S_008BF0_ENABLE_PA_SC_OUT_OF_ORDER(x)                       (((x) & 0x1) << 0)
#define   G_008BF0_ENABLE_PA_SC_OUT_OF_ORDER(x)                       (((x) >> 0) & 0x1)
#define   C_008BF0_ENABLE_PA_SC_OUT_OF_ORDER                          0xFFFFFFFE
#define   S_008BF0_DISABLE_SC_DB_TILE_FIX(x)                          (((x) & 0x1) << 1)
#define   G_008BF0_DISABLE_SC_DB_TILE_FIX(x)                          (((x) >> 1) & 0x1)
#define   C_008BF0_DISABLE_SC_DB_TILE_FIX                             0xFFFFFFFD
#define   S_008BF0_DISABLE_AA_MASK_FULL_FIX(x)                        (((x) & 0x1) << 2)
#define   G_008BF0_DISABLE_AA_MASK_FULL_FIX(x)                        (((x) >> 2) & 0x1)
#define   C_008BF0_DISABLE_AA_MASK_FULL_FIX                           0xFFFFFFFB
#define   S_008BF0_ENABLE_1XMSAA_SAMPLE_LOCATIONS(x)                  (((x) & 0x1) << 3)
#define   G_008BF0_ENABLE_1XMSAA_SAMPLE_LOCATIONS(x)                  (((x) >> 3) & 0x1)
#define   C_008BF0_ENABLE_1XMSAA_SAMPLE_LOCATIONS                     0xFFFFFFF7
#define   S_008BF0_ENABLE_1XMSAA_SAMPLE_LOC_CENTROID(x)               (((x) & 0x1) << 4)
#define   G_008BF0_ENABLE_1XMSAA_SAMPLE_LOC_CENTROID(x)               (((x) >> 4) & 0x1)
#define   C_008BF0_ENABLE_1XMSAA_SAMPLE_LOC_CENTROID                  0xFFFFFFEF
#define   S_008BF0_DISABLE_SCISSOR_FIX(x)                             (((x) & 0x1) << 5)
#define   G_008BF0_DISABLE_SCISSOR_FIX(x)                             (((x) >> 5) & 0x1)
#define   C_008BF0_DISABLE_SCISSOR_FIX                                0xFFFFFFDF
#define   S_008BF0_DISABLE_PW_BUBBLE_COLLAPSE(x)                      (((x) & 0x03) << 6)
#define   G_008BF0_DISABLE_PW_BUBBLE_COLLAPSE(x)                      (((x) >> 6) & 0x03)
#define   C_008BF0_DISABLE_PW_BUBBLE_COLLAPSE                         0xFFFFFF3F
#define   S_008BF0_SEND_UNLIT_STILES_TO_PACKER(x)                     (((x) & 0x1) << 8)
#define   G_008BF0_SEND_UNLIT_STILES_TO_PACKER(x)                     (((x) >> 8) & 0x1)
#define   C_008BF0_SEND_UNLIT_STILES_TO_PACKER                        0xFFFFFEFF
#define   S_008BF0_DISABLE_DUALGRAD_PERF_OPTIMIZATION(x)              (((x) & 0x1) << 9)
#define   G_008BF0_DISABLE_DUALGRAD_PERF_OPTIMIZATION(x)              (((x) >> 9) & 0x1)
#define   C_008BF0_DISABLE_DUALGRAD_PERF_OPTIMIZATION                 0xFFFFFDFF
#define R_008C08_SQC_CACHES                                             0x008C08
#define   S_008C08_INST_INVALIDATE(x)                                 (((x) & 0x1) << 0)
#define   G_008C08_INST_INVALIDATE(x)                                 (((x) >> 0) & 0x1)
#define   C_008C08_INST_INVALIDATE                                    0xFFFFFFFE
#define   S_008C08_DATA_INVALIDATE(x)                                 (((x) & 0x1) << 1)
#define   G_008C08_DATA_INVALIDATE(x)                                 (((x) >> 1) & 0x1)
#define   C_008C08_DATA_INVALIDATE                                    0xFFFFFFFD
#define R_008C0C_SQ_RANDOM_WAVE_PRI                                     0x008C0C
#define   S_008C0C_RET(x)                                             (((x) & 0x7F) << 0)
#define   G_008C0C_RET(x)                                             (((x) >> 0) & 0x7F)
#define   C_008C0C_RET                                                0xFFFFFF80
#define   S_008C0C_RUI(x)                                             (((x) & 0x07) << 7)
#define   G_008C0C_RUI(x)                                             (((x) >> 7) & 0x07)
#define   C_008C0C_RUI                                                0xFFFFFC7F
#define   S_008C0C_RNG(x)                                             (((x) & 0x7FF) << 10)
#define   G_008C0C_RNG(x)                                             (((x) >> 10) & 0x7FF)
#define   C_008C0C_RNG                                                0xFFE003FF
#if 0
#define R_008DFC_SQ_INST                                                0x008DFC
#define R_008DFC_SQ_VOP1                                                0x008DFC
#define   S_008DFC_SRC0(x)                                            (((x) & 0x1FF) << 0)
#define   G_008DFC_SRC0(x)                                            (((x) >> 0) & 0x1FF)
#define   C_008DFC_SRC0                                               0xFFFFFE00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define     V_008DFC_SQ_SRC_VGPR                                    0x100
#define   S_008DFC_OP(x)                                              (((x) & 0xFF) << 9)
#define   G_008DFC_OP(x)                                              (((x) >> 9) & 0xFF)
#define   C_008DFC_OP                                                 0xFFFE01FF
#define     V_008DFC_SQ_V_NOP                                       0x00
#define     V_008DFC_SQ_V_MOV_B32                                   0x01
#define     V_008DFC_SQ_V_READFIRSTLANE_B32                         0x02
#define     V_008DFC_SQ_V_CVT_I32_F64                               0x03
#define     V_008DFC_SQ_V_CVT_F64_I32                               0x04
#define     V_008DFC_SQ_V_CVT_F32_I32                               0x05
#define     V_008DFC_SQ_V_CVT_F32_U32                               0x06
#define     V_008DFC_SQ_V_CVT_U32_F32                               0x07
#define     V_008DFC_SQ_V_CVT_I32_F32                               0x08
#define     V_008DFC_SQ_V_MOV_FED_B32                               0x09
#define     V_008DFC_SQ_V_CVT_F16_F32                               0x0A
#define     V_008DFC_SQ_V_CVT_F32_F16                               0x0B
#define     V_008DFC_SQ_V_CVT_RPI_I32_F32                           0x0C
#define     V_008DFC_SQ_V_CVT_FLR_I32_F32                           0x0D
#define     V_008DFC_SQ_V_CVT_OFF_F32_I4                            0x0E
#define     V_008DFC_SQ_V_CVT_F32_F64                               0x0F
#define     V_008DFC_SQ_V_CVT_F64_F32                               0x10
#define     V_008DFC_SQ_V_CVT_F32_UBYTE0                            0x11
#define     V_008DFC_SQ_V_CVT_F32_UBYTE1                            0x12
#define     V_008DFC_SQ_V_CVT_F32_UBYTE2                            0x13
#define     V_008DFC_SQ_V_CVT_F32_UBYTE3                            0x14
#define     V_008DFC_SQ_V_CVT_U32_F64                               0x15
#define     V_008DFC_SQ_V_CVT_F64_U32                               0x16
#define     V_008DFC_SQ_V_FRACT_F32                                 0x20
#define     V_008DFC_SQ_V_TRUNC_F32                                 0x21
#define     V_008DFC_SQ_V_CEIL_F32                                  0x22
#define     V_008DFC_SQ_V_RNDNE_F32                                 0x23
#define     V_008DFC_SQ_V_FLOOR_F32                                 0x24
#define     V_008DFC_SQ_V_EXP_F32                                   0x25
#define     V_008DFC_SQ_V_LOG_CLAMP_F32                             0x26
#define     V_008DFC_SQ_V_LOG_F32                                   0x27
#define     V_008DFC_SQ_V_RCP_CLAMP_F32                             0x28
#define     V_008DFC_SQ_V_RCP_LEGACY_F32                            0x29
#define     V_008DFC_SQ_V_RCP_F32                                   0x2A
#define     V_008DFC_SQ_V_RCP_IFLAG_F32                             0x2B
#define     V_008DFC_SQ_V_RSQ_CLAMP_F32                             0x2C
#define     V_008DFC_SQ_V_RSQ_LEGACY_F32                            0x2D
#define     V_008DFC_SQ_V_RSQ_F32                                   0x2E
#define     V_008DFC_SQ_V_RCP_F64                                   0x2F
#define     V_008DFC_SQ_V_RCP_CLAMP_F64                             0x30
#define     V_008DFC_SQ_V_RSQ_F64                                   0x31
#define     V_008DFC_SQ_V_RSQ_CLAMP_F64                             0x32
#define     V_008DFC_SQ_V_SQRT_F32                                  0x33
#define     V_008DFC_SQ_V_SQRT_F64                                  0x34
#define     V_008DFC_SQ_V_SIN_F32                                   0x35
#define     V_008DFC_SQ_V_COS_F32                                   0x36
#define     V_008DFC_SQ_V_NOT_B32                                   0x37
#define     V_008DFC_SQ_V_BFREV_B32                                 0x38
#define     V_008DFC_SQ_V_FFBH_U32                                  0x39
#define     V_008DFC_SQ_V_FFBL_B32                                  0x3A
#define     V_008DFC_SQ_V_FFBH_I32                                  0x3B
#define     V_008DFC_SQ_V_FREXP_EXP_I32_F64                         0x3C
#define     V_008DFC_SQ_V_FREXP_MANT_F64                            0x3D
#define     V_008DFC_SQ_V_FRACT_F64                                 0x3E
#define     V_008DFC_SQ_V_FREXP_EXP_I32_F32                         0x3F
#define     V_008DFC_SQ_V_FREXP_MANT_F32                            0x40
#define     V_008DFC_SQ_V_CLREXCP                                   0x41
#define     V_008DFC_SQ_V_MOVRELD_B32                               0x42
#define     V_008DFC_SQ_V_MOVRELS_B32                               0x43
#define     V_008DFC_SQ_V_MOVRELSD_B32                              0x44
#define   S_008DFC_VDST(x)                                            (((x) & 0xFF) << 17)
#define   G_008DFC_VDST(x)                                            (((x) >> 17) & 0xFF)
#define   C_008DFC_VDST                                               0xFE01FFFF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x7F) << 25)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 25) & 0x7F)
#define   C_008DFC_ENCODING                                           0x01FFFFFF
#define     V_008DFC_SQ_ENC_VOP1_FIELD                              0x3F
#define R_008DFC_SQ_MIMG_1                                              0x008DFC
#define   S_008DFC_VADDR(x)                                           (((x) & 0xFF) << 0)
#define   G_008DFC_VADDR(x)                                           (((x) >> 0) & 0xFF)
#define   C_008DFC_VADDR                                              0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VDATA(x)                                           (((x) & 0xFF) << 8)
#define   G_008DFC_VDATA(x)                                           (((x) >> 8) & 0xFF)
#define   C_008DFC_VDATA                                              0xFFFF00FF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_SRSRC(x)                                           (((x) & 0x1F) << 16)
#define   G_008DFC_SRSRC(x)                                           (((x) >> 16) & 0x1F)
#define   C_008DFC_SRSRC                                              0xFFE0FFFF
#define   S_008DFC_SSAMP(x)                                           (((x) & 0x1F) << 21)
#define   G_008DFC_SSAMP(x)                                           (((x) >> 21) & 0x1F)
#define   C_008DFC_SSAMP                                              0xFC1FFFFF
#define R_008DFC_SQ_VOP3_1                                              0x008DFC
#define   S_008DFC_SRC0(x)                                            (((x) & 0x1FF) << 0)
#define   G_008DFC_SRC0(x)                                            (((x) >> 0) & 0x1FF)
#define   C_008DFC_SRC0                                               0xFFFFFE00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define     V_008DFC_SQ_SRC_VGPR                                    0x100
#define   S_008DFC_SRC1(x)                                            (((x) & 0x1FF) << 9)
#define   G_008DFC_SRC1(x)                                            (((x) >> 9) & 0x1FF)
#define   C_008DFC_SRC1                                               0xFFFC01FF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define     V_008DFC_SQ_SRC_VGPR                                    0x100
#define   S_008DFC_SRC2(x)                                            (((x) & 0x1FF) << 18)
#define   G_008DFC_SRC2(x)                                            (((x) >> 18) & 0x1FF)
#define   C_008DFC_SRC2                                               0xF803FFFF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define     V_008DFC_SQ_SRC_VGPR                                    0x100
#define   S_008DFC_OMOD(x)                                            (((x) & 0x03) << 27)
#define   G_008DFC_OMOD(x)                                            (((x) >> 27) & 0x03)
#define   C_008DFC_OMOD                                               0xE7FFFFFF
#define     V_008DFC_SQ_OMOD_OFF                                    0x00
#define     V_008DFC_SQ_OMOD_M2                                     0x01
#define     V_008DFC_SQ_OMOD_M4                                     0x02
#define     V_008DFC_SQ_OMOD_D2                                     0x03
#define   S_008DFC_NEG(x)                                             (((x) & 0x07) << 29)
#define   G_008DFC_NEG(x)                                             (((x) >> 29) & 0x07)
#define   C_008DFC_NEG                                                0x1FFFFFFF
#define R_008DFC_SQ_MUBUF_1                                             0x008DFC
#define   S_008DFC_VADDR(x)                                           (((x) & 0xFF) << 0)
#define   G_008DFC_VADDR(x)                                           (((x) >> 0) & 0xFF)
#define   C_008DFC_VADDR                                              0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VDATA(x)                                           (((x) & 0xFF) << 8)
#define   G_008DFC_VDATA(x)                                           (((x) >> 8) & 0xFF)
#define   C_008DFC_VDATA                                              0xFFFF00FF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_SRSRC(x)                                           (((x) & 0x1F) << 16)
#define   G_008DFC_SRSRC(x)                                           (((x) >> 16) & 0x1F)
#define   C_008DFC_SRSRC                                              0xFFE0FFFF
#define   S_008DFC_SLC(x)                                             (((x) & 0x1) << 22)
#define   G_008DFC_SLC(x)                                             (((x) >> 22) & 0x1)
#define   C_008DFC_SLC                                                0xFFBFFFFF
#define   S_008DFC_TFE(x)                                             (((x) & 0x1) << 23)
#define   G_008DFC_TFE(x)                                             (((x) >> 23) & 0x1)
#define   C_008DFC_TFE                                                0xFF7FFFFF
#define   S_008DFC_SOFFSET(x)                                         (((x) & 0xFF) << 24)
#define   G_008DFC_SOFFSET(x)                                         (((x) >> 24) & 0xFF)
#define   C_008DFC_SOFFSET                                            0x00FFFFFF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define R_008DFC_SQ_DS_0                                                0x008DFC
#define   S_008DFC_OFFSET0(x)                                         (((x) & 0xFF) << 0)
#define   G_008DFC_OFFSET0(x)                                         (((x) >> 0) & 0xFF)
#define   C_008DFC_OFFSET0                                            0xFFFFFF00
#define   S_008DFC_OFFSET1(x)                                         (((x) & 0xFF) << 8)
#define   G_008DFC_OFFSET1(x)                                         (((x) >> 8) & 0xFF)
#define   C_008DFC_OFFSET1                                            0xFFFF00FF
#define   S_008DFC_GDS(x)                                             (((x) & 0x1) << 17)
#define   G_008DFC_GDS(x)                                             (((x) >> 17) & 0x1)
#define   C_008DFC_GDS                                                0xFFFDFFFF
#define   S_008DFC_OP(x)                                              (((x) & 0xFF) << 18)
#define   G_008DFC_OP(x)                                              (((x) >> 18) & 0xFF)
#define   C_008DFC_OP                                                 0xFC03FFFF
#define     V_008DFC_SQ_DS_ADD_U32                                  0x00
#define     V_008DFC_SQ_DS_SUB_U32                                  0x01
#define     V_008DFC_SQ_DS_RSUB_U32                                 0x02
#define     V_008DFC_SQ_DS_INC_U32                                  0x03
#define     V_008DFC_SQ_DS_DEC_U32                                  0x04
#define     V_008DFC_SQ_DS_MIN_I32                                  0x05
#define     V_008DFC_SQ_DS_MAX_I32                                  0x06
#define     V_008DFC_SQ_DS_MIN_U32                                  0x07
#define     V_008DFC_SQ_DS_MAX_U32                                  0x08
#define     V_008DFC_SQ_DS_AND_B32                                  0x09
#define     V_008DFC_SQ_DS_OR_B32                                   0x0A
#define     V_008DFC_SQ_DS_XOR_B32                                  0x0B
#define     V_008DFC_SQ_DS_MSKOR_B32                                0x0C
#define     V_008DFC_SQ_DS_WRITE_B32                                0x0D
#define     V_008DFC_SQ_DS_WRITE2_B32                               0x0E
#define     V_008DFC_SQ_DS_WRITE2ST64_B32                           0x0F
#define     V_008DFC_SQ_DS_CMPST_B32                                0x10
#define     V_008DFC_SQ_DS_CMPST_F32                                0x11
#define     V_008DFC_SQ_DS_MIN_F32                                  0x12
#define     V_008DFC_SQ_DS_MAX_F32                                  0x13
#define     V_008DFC_SQ_DS_GWS_INIT                                 0x19
#define     V_008DFC_SQ_DS_GWS_SEMA_V                               0x1A
#define     V_008DFC_SQ_DS_GWS_SEMA_BR                              0x1B
#define     V_008DFC_SQ_DS_GWS_SEMA_P                               0x1C
#define     V_008DFC_SQ_DS_GWS_BARRIER                              0x1D
#define     V_008DFC_SQ_DS_WRITE_B8                                 0x1E
#define     V_008DFC_SQ_DS_WRITE_B16                                0x1F
#define     V_008DFC_SQ_DS_ADD_RTN_U32                              0x20
#define     V_008DFC_SQ_DS_SUB_RTN_U32                              0x21
#define     V_008DFC_SQ_DS_RSUB_RTN_U32                             0x22
#define     V_008DFC_SQ_DS_INC_RTN_U32                              0x23
#define     V_008DFC_SQ_DS_DEC_RTN_U32                              0x24
#define     V_008DFC_SQ_DS_MIN_RTN_I32                              0x25
#define     V_008DFC_SQ_DS_MAX_RTN_I32                              0x26
#define     V_008DFC_SQ_DS_MIN_RTN_U32                              0x27
#define     V_008DFC_SQ_DS_MAX_RTN_U32                              0x28
#define     V_008DFC_SQ_DS_AND_RTN_B32                              0x29
#define     V_008DFC_SQ_DS_OR_RTN_B32                               0x2A
#define     V_008DFC_SQ_DS_XOR_RTN_B32                              0x2B
#define     V_008DFC_SQ_DS_MSKOR_RTN_B32                            0x2C
#define     V_008DFC_SQ_DS_WRXCHG_RTN_B32                           0x2D
#define     V_008DFC_SQ_DS_WRXCHG2_RTN_B32                          0x2E
#define     V_008DFC_SQ_DS_WRXCHG2ST64_RTN_B32                      0x2F
#define     V_008DFC_SQ_DS_CMPST_RTN_B32                            0x30
#define     V_008DFC_SQ_DS_CMPST_RTN_F32                            0x31
#define     V_008DFC_SQ_DS_MIN_RTN_F32                              0x32
#define     V_008DFC_SQ_DS_MAX_RTN_F32                              0x33
#define     V_008DFC_SQ_DS_SWIZZLE_B32                              0x35
#define     V_008DFC_SQ_DS_READ_B32                                 0x36
#define     V_008DFC_SQ_DS_READ2_B32                                0x37
#define     V_008DFC_SQ_DS_READ2ST64_B32                            0x38
#define     V_008DFC_SQ_DS_READ_I8                                  0x39
#define     V_008DFC_SQ_DS_READ_U8                                  0x3A
#define     V_008DFC_SQ_DS_READ_I16                                 0x3B
#define     V_008DFC_SQ_DS_READ_U16                                 0x3C
#define     V_008DFC_SQ_DS_CONSUME                                  0x3D
#define     V_008DFC_SQ_DS_APPEND                                   0x3E
#define     V_008DFC_SQ_DS_ORDERED_COUNT                            0x3F
#define     V_008DFC_SQ_DS_ADD_U64                                  0x40
#define     V_008DFC_SQ_DS_SUB_U64                                  0x41
#define     V_008DFC_SQ_DS_RSUB_U64                                 0x42
#define     V_008DFC_SQ_DS_INC_U64                                  0x43
#define     V_008DFC_SQ_DS_DEC_U64                                  0x44
#define     V_008DFC_SQ_DS_MIN_I64                                  0x45
#define     V_008DFC_SQ_DS_MAX_I64                                  0x46
#define     V_008DFC_SQ_DS_MIN_U64                                  0x47
#define     V_008DFC_SQ_DS_MAX_U64                                  0x48
#define     V_008DFC_SQ_DS_AND_B64                                  0x49
#define     V_008DFC_SQ_DS_OR_B64                                   0x4A
#define     V_008DFC_SQ_DS_XOR_B64                                  0x4B
#define     V_008DFC_SQ_DS_MSKOR_B64                                0x4C
#define     V_008DFC_SQ_DS_WRITE_B64                                0x4D
#define     V_008DFC_SQ_DS_WRITE2_B64                               0x4E
#define     V_008DFC_SQ_DS_WRITE2ST64_B64                           0x4F
#define     V_008DFC_SQ_DS_CMPST_B64                                0x50
#define     V_008DFC_SQ_DS_CMPST_F64                                0x51
#define     V_008DFC_SQ_DS_MIN_F64                                  0x52
#define     V_008DFC_SQ_DS_MAX_F64                                  0x53
#define     V_008DFC_SQ_DS_ADD_RTN_U64                              0x60
#define     V_008DFC_SQ_DS_SUB_RTN_U64                              0x61
#define     V_008DFC_SQ_DS_RSUB_RTN_U64                             0x62
#define     V_008DFC_SQ_DS_INC_RTN_U64                              0x63
#define     V_008DFC_SQ_DS_DEC_RTN_U64                              0x64
#define     V_008DFC_SQ_DS_MIN_RTN_I64                              0x65
#define     V_008DFC_SQ_DS_MAX_RTN_I64                              0x66
#define     V_008DFC_SQ_DS_MIN_RTN_U64                              0x67
#define     V_008DFC_SQ_DS_MAX_RTN_U64                              0x68
#define     V_008DFC_SQ_DS_AND_RTN_B64                              0x69
#define     V_008DFC_SQ_DS_OR_RTN_B64                               0x6A
#define     V_008DFC_SQ_DS_XOR_RTN_B64                              0x6B
#define     V_008DFC_SQ_DS_MSKOR_RTN_B64                            0x6C
#define     V_008DFC_SQ_DS_WRXCHG_RTN_B64                           0x6D
#define     V_008DFC_SQ_DS_WRXCHG2_RTN_B64                          0x6E
#define     V_008DFC_SQ_DS_WRXCHG2ST64_RTN_B64                      0x6F
#define     V_008DFC_SQ_DS_CMPST_RTN_B64                            0x70
#define     V_008DFC_SQ_DS_CMPST_RTN_F64                            0x71
#define     V_008DFC_SQ_DS_MIN_RTN_F64                              0x72
#define     V_008DFC_SQ_DS_MAX_RTN_F64                              0x73
#define     V_008DFC_SQ_DS_READ_B64                                 0x76
#define     V_008DFC_SQ_DS_READ2_B64                                0x77
#define     V_008DFC_SQ_DS_READ2ST64_B64                            0x78
#define     V_008DFC_SQ_DS_ADD_SRC2_U32                             0x80
#define     V_008DFC_SQ_DS_SUB_SRC2_U32                             0x81
#define     V_008DFC_SQ_DS_RSUB_SRC2_U32                            0x82
#define     V_008DFC_SQ_DS_INC_SRC2_U32                             0x83
#define     V_008DFC_SQ_DS_DEC_SRC2_U32                             0x84
#define     V_008DFC_SQ_DS_MIN_SRC2_I32                             0x85
#define     V_008DFC_SQ_DS_MAX_SRC2_I32                             0x86
#define     V_008DFC_SQ_DS_MIN_SRC2_U32                             0x87
#define     V_008DFC_SQ_DS_MAX_SRC2_U32                             0x88
#define     V_008DFC_SQ_DS_AND_SRC2_B32                             0x89
#define     V_008DFC_SQ_DS_OR_SRC2_B32                              0x8A
#define     V_008DFC_SQ_DS_XOR_SRC2_B32                             0x8B
#define     V_008DFC_SQ_DS_WRITE_SRC2_B32                           0x8D
#define     V_008DFC_SQ_DS_MIN_SRC2_F32                             0x92
#define     V_008DFC_SQ_DS_MAX_SRC2_F32                             0x93
#define     V_008DFC_SQ_DS_ADD_SRC2_U64                             0xC0
#define     V_008DFC_SQ_DS_SUB_SRC2_U64                             0xC1
#define     V_008DFC_SQ_DS_RSUB_SRC2_U64                            0xC2
#define     V_008DFC_SQ_DS_INC_SRC2_U64                             0xC3
#define     V_008DFC_SQ_DS_DEC_SRC2_U64                             0xC4
#define     V_008DFC_SQ_DS_MIN_SRC2_I64                             0xC5
#define     V_008DFC_SQ_DS_MAX_SRC2_I64                             0xC6
#define     V_008DFC_SQ_DS_MIN_SRC2_U64                             0xC7
#define     V_008DFC_SQ_DS_MAX_SRC2_U64                             0xC8
#define     V_008DFC_SQ_DS_AND_SRC2_B64                             0xC9
#define     V_008DFC_SQ_DS_OR_SRC2_B64                              0xCA
#define     V_008DFC_SQ_DS_XOR_SRC2_B64                             0xCB
#define     V_008DFC_SQ_DS_WRITE_SRC2_B64                           0xCD
#define     V_008DFC_SQ_DS_MIN_SRC2_F64                             0xD2
#define     V_008DFC_SQ_DS_MAX_SRC2_F64                             0xD3
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_DS_FIELD                                0x36
#define R_008DFC_SQ_SOPC                                                0x008DFC
#define   S_008DFC_SSRC0(x)                                           (((x) & 0xFF) << 0)
#define   G_008DFC_SSRC0(x)                                           (((x) >> 0) & 0xFF)
#define   C_008DFC_SSRC0                                              0xFFFFFF00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define   S_008DFC_SSRC1(x)                                           (((x) & 0xFF) << 8)
#define   G_008DFC_SSRC1(x)                                           (((x) >> 8) & 0xFF)
#define   C_008DFC_SSRC1                                              0xFFFF00FF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define   S_008DFC_OP(x)                                              (((x) & 0x7F) << 16)
#define   G_008DFC_OP(x)                                              (((x) >> 16) & 0x7F)
#define   C_008DFC_OP                                                 0xFF80FFFF
#define     V_008DFC_SQ_S_CMP_EQ_I32                                0x00
#define     V_008DFC_SQ_S_CMP_LG_I32                                0x01
#define     V_008DFC_SQ_S_CMP_GT_I32                                0x02
#define     V_008DFC_SQ_S_CMP_GE_I32                                0x03
#define     V_008DFC_SQ_S_CMP_LT_I32                                0x04
#define     V_008DFC_SQ_S_CMP_LE_I32                                0x05
#define     V_008DFC_SQ_S_CMP_EQ_U32                                0x06
#define     V_008DFC_SQ_S_CMP_LG_U32                                0x07
#define     V_008DFC_SQ_S_CMP_GT_U32                                0x08
#define     V_008DFC_SQ_S_CMP_GE_U32                                0x09
#define     V_008DFC_SQ_S_CMP_LT_U32                                0x0A
#define     V_008DFC_SQ_S_CMP_LE_U32                                0x0B
#define     V_008DFC_SQ_S_BITCMP0_B32                               0x0C
#define     V_008DFC_SQ_S_BITCMP1_B32                               0x0D
#define     V_008DFC_SQ_S_BITCMP0_B64                               0x0E
#define     V_008DFC_SQ_S_BITCMP1_B64                               0x0F
#define     V_008DFC_SQ_S_SETVSKIP                                  0x10
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x1FF) << 23)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 23) & 0x1FF)
#define   C_008DFC_ENCODING                                           0x007FFFFF
#define     V_008DFC_SQ_ENC_SOPC_FIELD                              0x17E
#endif
#define R_008DFC_SQ_EXP_0                                               0x008DFC
#define   S_008DFC_EN(x)                                              (((x) & 0x0F) << 0)
#define   G_008DFC_EN(x)                                              (((x) >> 0) & 0x0F)
#define   C_008DFC_EN                                                 0xFFFFFFF0
#define   S_008DFC_TGT(x)                                             (((x) & 0x3F) << 4)
#define   G_008DFC_TGT(x)                                             (((x) >> 4) & 0x3F)
#define   C_008DFC_TGT                                                0xFFFFFC0F
#define     V_008DFC_SQ_EXP_MRT                                     0x00
#define     V_008DFC_SQ_EXP_MRTZ                                    0x08
#define     V_008DFC_SQ_EXP_NULL                                    0x09
#define     V_008DFC_SQ_EXP_POS                                     0x0C
#define     V_008DFC_SQ_EXP_PARAM                                   0x20
#define   S_008DFC_COMPR(x)                                           (((x) & 0x1) << 10)
#define   G_008DFC_COMPR(x)                                           (((x) >> 10) & 0x1)
#define   C_008DFC_COMPR                                              0xFFFFFBFF
#define   S_008DFC_DONE(x)                                            (((x) & 0x1) << 11)
#define   G_008DFC_DONE(x)                                            (((x) >> 11) & 0x1)
#define   C_008DFC_DONE                                               0xFFFFF7FF
#define   S_008DFC_VM(x)                                              (((x) & 0x1) << 12)
#define   G_008DFC_VM(x)                                              (((x) >> 12) & 0x1)
#define   C_008DFC_VM                                                 0xFFFFEFFF
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_EXP_FIELD                               0x3E
#if 0
#define R_008DFC_SQ_MIMG_0                                              0x008DFC
#define   S_008DFC_DMASK(x)                                           (((x) & 0x0F) << 8)
#define   G_008DFC_DMASK(x)                                           (((x) >> 8) & 0x0F)
#define   C_008DFC_DMASK                                              0xFFFFF0FF
#define   S_008DFC_UNORM(x)                                           (((x) & 0x1) << 12)
#define   G_008DFC_UNORM(x)                                           (((x) >> 12) & 0x1)
#define   C_008DFC_UNORM                                              0xFFFFEFFF
#define   S_008DFC_GLC(x)                                             (((x) & 0x1) << 13)
#define   G_008DFC_GLC(x)                                             (((x) >> 13) & 0x1)
#define   C_008DFC_GLC                                                0xFFFFDFFF
#define   S_008DFC_DA(x)                                              (((x) & 0x1) << 14)
#define   G_008DFC_DA(x)                                              (((x) >> 14) & 0x1)
#define   C_008DFC_DA                                                 0xFFFFBFFF
#define   S_008DFC_R128(x)                                            (((x) & 0x1) << 15)
#define   G_008DFC_R128(x)                                            (((x) >> 15) & 0x1)
#define   C_008DFC_R128                                               0xFFFF7FFF
#define   S_008DFC_TFE(x)                                             (((x) & 0x1) << 16)
#define   G_008DFC_TFE(x)                                             (((x) >> 16) & 0x1)
#define   C_008DFC_TFE                                                0xFFFEFFFF
#define   S_008DFC_LWE(x)                                             (((x) & 0x1) << 17)
#define   G_008DFC_LWE(x)                                             (((x) >> 17) & 0x1)
#define   C_008DFC_LWE                                                0xFFFDFFFF
#define   S_008DFC_OP(x)                                              (((x) & 0x7F) << 18)
#define   G_008DFC_OP(x)                                              (((x) >> 18) & 0x7F)
#define   C_008DFC_OP                                                 0xFE03FFFF
#define     V_008DFC_SQ_IMAGE_LOAD                                  0x00
#define     V_008DFC_SQ_IMAGE_LOAD_MIP                              0x01
#define     V_008DFC_SQ_IMAGE_LOAD_PCK                              0x02
#define     V_008DFC_SQ_IMAGE_LOAD_PCK_SGN                          0x03
#define     V_008DFC_SQ_IMAGE_LOAD_MIP_PCK                          0x04
#define     V_008DFC_SQ_IMAGE_LOAD_MIP_PCK_SGN                      0x05
#define     V_008DFC_SQ_IMAGE_STORE                                 0x08
#define     V_008DFC_SQ_IMAGE_STORE_MIP                             0x09
#define     V_008DFC_SQ_IMAGE_STORE_PCK                             0x0A
#define     V_008DFC_SQ_IMAGE_STORE_MIP_PCK                         0x0B
#define     V_008DFC_SQ_IMAGE_GET_RESINFO                           0x0E
#define     V_008DFC_SQ_IMAGE_ATOMIC_SWAP                           0x0F
#define     V_008DFC_SQ_IMAGE_ATOMIC_CMPSWAP                        0x10
#define     V_008DFC_SQ_IMAGE_ATOMIC_ADD                            0x11
#define     V_008DFC_SQ_IMAGE_ATOMIC_SUB                            0x12
#define     V_008DFC_SQ_IMAGE_ATOMIC_RSUB                           0x13
#define     V_008DFC_SQ_IMAGE_ATOMIC_SMIN                           0x14
#define     V_008DFC_SQ_IMAGE_ATOMIC_UMIN                           0x15
#define     V_008DFC_SQ_IMAGE_ATOMIC_SMAX                           0x16
#define     V_008DFC_SQ_IMAGE_ATOMIC_UMAX                           0x17
#define     V_008DFC_SQ_IMAGE_ATOMIC_AND                            0x18
#define     V_008DFC_SQ_IMAGE_ATOMIC_OR                             0x19
#define     V_008DFC_SQ_IMAGE_ATOMIC_XOR                            0x1A
#define     V_008DFC_SQ_IMAGE_ATOMIC_INC                            0x1B
#define     V_008DFC_SQ_IMAGE_ATOMIC_DEC                            0x1C
#define     V_008DFC_SQ_IMAGE_ATOMIC_FCMPSWAP                       0x1D
#define     V_008DFC_SQ_IMAGE_ATOMIC_FMIN                           0x1E
#define     V_008DFC_SQ_IMAGE_ATOMIC_FMAX                           0x1F
#define     V_008DFC_SQ_IMAGE_SAMPLE                                0x20
#define     V_008DFC_SQ_IMAGE_SAMPLE_CL                             0x21
#define     V_008DFC_SQ_IMAGE_SAMPLE_D                              0x22
#define     V_008DFC_SQ_IMAGE_SAMPLE_D_CL                           0x23
#define     V_008DFC_SQ_IMAGE_SAMPLE_L                              0x24
#define     V_008DFC_SQ_IMAGE_SAMPLE_B                              0x25
#define     V_008DFC_SQ_IMAGE_SAMPLE_B_CL                           0x26
#define     V_008DFC_SQ_IMAGE_SAMPLE_LZ                             0x27
#define     V_008DFC_SQ_IMAGE_SAMPLE_C                              0x28
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_CL                           0x29
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_D                            0x2A
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_D_CL                         0x2B
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_L                            0x2C
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_B                            0x2D
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_B_CL                         0x2E
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_LZ                           0x2F
#define     V_008DFC_SQ_IMAGE_SAMPLE_O                              0x30
#define     V_008DFC_SQ_IMAGE_SAMPLE_CL_O                           0x31
#define     V_008DFC_SQ_IMAGE_SAMPLE_D_O                            0x32
#define     V_008DFC_SQ_IMAGE_SAMPLE_D_CL_O                         0x33
#define     V_008DFC_SQ_IMAGE_SAMPLE_L_O                            0x34
#define     V_008DFC_SQ_IMAGE_SAMPLE_B_O                            0x35
#define     V_008DFC_SQ_IMAGE_SAMPLE_B_CL_O                         0x36
#define     V_008DFC_SQ_IMAGE_SAMPLE_LZ_O                           0x37
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_O                            0x38
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_CL_O                         0x39
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_D_O                          0x3A
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_D_CL_O                       0x3B
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_L_O                          0x3C
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_B_O                          0x3D
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_B_CL_O                       0x3E
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_LZ_O                         0x3F
#define     V_008DFC_SQ_IMAGE_GATHER4                               0x40
#define     V_008DFC_SQ_IMAGE_GATHER4_CL                            0x41
#define     V_008DFC_SQ_IMAGE_GATHER4_L                             0x44
#define     V_008DFC_SQ_IMAGE_GATHER4_B                             0x45
#define     V_008DFC_SQ_IMAGE_GATHER4_B_CL                          0x46
#define     V_008DFC_SQ_IMAGE_GATHER4_LZ                            0x47
#define     V_008DFC_SQ_IMAGE_GATHER4_C                             0x48
#define     V_008DFC_SQ_IMAGE_GATHER4_C_CL                          0x49
#define     V_008DFC_SQ_IMAGE_GATHER4_C_L                           0x4C
#define     V_008DFC_SQ_IMAGE_GATHER4_C_B                           0x4D
#define     V_008DFC_SQ_IMAGE_GATHER4_C_B_CL                        0x4E
#define     V_008DFC_SQ_IMAGE_GATHER4_C_LZ                          0x4F
#define     V_008DFC_SQ_IMAGE_GATHER4_O                             0x50
#define     V_008DFC_SQ_IMAGE_GATHER4_CL_O                          0x51
#define     V_008DFC_SQ_IMAGE_GATHER4_L_O                           0x54
#define     V_008DFC_SQ_IMAGE_GATHER4_B_O                           0x55
#define     V_008DFC_SQ_IMAGE_GATHER4_B_CL_O                        0x56
#define     V_008DFC_SQ_IMAGE_GATHER4_LZ_O                          0x57
#define     V_008DFC_SQ_IMAGE_GATHER4_C_O                           0x58
#define     V_008DFC_SQ_IMAGE_GATHER4_C_CL_O                        0x59
#define     V_008DFC_SQ_IMAGE_GATHER4_C_L_O                         0x5C
#define     V_008DFC_SQ_IMAGE_GATHER4_C_B_O                         0x5D
#define     V_008DFC_SQ_IMAGE_GATHER4_C_B_CL_O                      0x5E
#define     V_008DFC_SQ_IMAGE_GATHER4_C_LZ_O                        0x5F
#define     V_008DFC_SQ_IMAGE_GET_LOD                               0x60
#define     V_008DFC_SQ_IMAGE_SAMPLE_CD                             0x68
#define     V_008DFC_SQ_IMAGE_SAMPLE_CD_CL                          0x69
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_CD                           0x6A
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_CD_CL                        0x6B
#define     V_008DFC_SQ_IMAGE_SAMPLE_CD_O                           0x6C
#define     V_008DFC_SQ_IMAGE_SAMPLE_CD_CL_O                        0x6D
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_CD_O                         0x6E
#define     V_008DFC_SQ_IMAGE_SAMPLE_C_CD_CL_O                      0x6F
#define     V_008DFC_SQ_IMAGE_RSRC256                               0x7E
#define     V_008DFC_SQ_IMAGE_SAMPLER                               0x7F
#define   S_008DFC_SLC(x)                                             (((x) & 0x1) << 25)
#define   G_008DFC_SLC(x)                                             (((x) >> 25) & 0x1)
#define   C_008DFC_SLC                                                0xFDFFFFFF
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_MIMG_FIELD                              0x3C
#define R_008DFC_SQ_SOPP                                                0x008DFC
#define   S_008DFC_SIMM16(x)                                          (((x) & 0xFFFF) << 0)
#define   G_008DFC_SIMM16(x)                                          (((x) >> 0) & 0xFFFF)
#define   C_008DFC_SIMM16                                             0xFFFF0000
#define   S_008DFC_OP(x)                                              (((x) & 0x7F) << 16)
#define   G_008DFC_OP(x)                                              (((x) >> 16) & 0x7F)
#define   C_008DFC_OP                                                 0xFF80FFFF
#define     V_008DFC_SQ_S_NOP                                       0x00
#define     V_008DFC_SQ_S_ENDPGM                                    0x01
#define     V_008DFC_SQ_S_BRANCH                                    0x02
#define     V_008DFC_SQ_S_CBRANCH_SCC0                              0x04
#define     V_008DFC_SQ_S_CBRANCH_SCC1                              0x05
#define     V_008DFC_SQ_S_CBRANCH_VCCZ                              0x06
#define     V_008DFC_SQ_S_CBRANCH_VCCNZ                             0x07
#define     V_008DFC_SQ_S_CBRANCH_EXECZ                             0x08
#define     V_008DFC_SQ_S_CBRANCH_EXECNZ                            0x09
#define     V_008DFC_SQ_S_BARRIER                                   0x0A
#define     V_008DFC_SQ_S_WAITCNT                                   0x0C
#define     V_008DFC_SQ_S_SETHALT                                   0x0D
#define     V_008DFC_SQ_S_SLEEP                                     0x0E
#define     V_008DFC_SQ_S_SETPRIO                                   0x0F
#define     V_008DFC_SQ_S_SENDMSG                                   0x10
#define     V_008DFC_SQ_S_SENDMSGHALT                               0x11
#define     V_008DFC_SQ_S_TRAP                                      0x12
#define     V_008DFC_SQ_S_ICACHE_INV                                0x13
#define     V_008DFC_SQ_S_INCPERFLEVEL                              0x14
#define     V_008DFC_SQ_S_DECPERFLEVEL                              0x15
#define     V_008DFC_SQ_S_TTRACEDATA                                0x16
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x1FF) << 23)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 23) & 0x1FF)
#define   C_008DFC_ENCODING                                           0x007FFFFF
#define     V_008DFC_SQ_ENC_SOPP_FIELD                              0x17F
#define R_008DFC_SQ_VINTRP                                              0x008DFC
#define   S_008DFC_VSRC(x)                                            (((x) & 0xFF) << 0)
#define   G_008DFC_VSRC(x)                                            (((x) >> 0) & 0xFF)
#define   C_008DFC_VSRC                                               0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_ATTRCHAN(x)                                        (((x) & 0x03) << 8)
#define   G_008DFC_ATTRCHAN(x)                                        (((x) >> 8) & 0x03)
#define   C_008DFC_ATTRCHAN                                           0xFFFFFCFF
#define     V_008DFC_SQ_CHAN_X                                      0x00
#define     V_008DFC_SQ_CHAN_Y                                      0x01
#define     V_008DFC_SQ_CHAN_Z                                      0x02
#define     V_008DFC_SQ_CHAN_W                                      0x03
#define   S_008DFC_ATTR(x)                                            (((x) & 0x3F) << 10)
#define   G_008DFC_ATTR(x)                                            (((x) >> 10) & 0x3F)
#define   C_008DFC_ATTR                                               0xFFFF03FF
#define     V_008DFC_SQ_ATTR                                        0x00
#define   S_008DFC_OP(x)                                              (((x) & 0x03) << 16)
#define   G_008DFC_OP(x)                                              (((x) >> 16) & 0x03)
#define   C_008DFC_OP                                                 0xFFFCFFFF
#define     V_008DFC_SQ_V_INTERP_P1_F32                             0x00
#define     V_008DFC_SQ_V_INTERP_P2_F32                             0x01
#define     V_008DFC_SQ_V_INTERP_MOV_F32                            0x02
#define   S_008DFC_VDST(x)                                            (((x) & 0xFF) << 18)
#define   G_008DFC_VDST(x)                                            (((x) >> 18) & 0xFF)
#define   C_008DFC_VDST                                               0xFC03FFFF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_VINTRP_FIELD                            0x32
#define R_008DFC_SQ_MTBUF_0                                             0x008DFC
#define   S_008DFC_OFFSET(x)                                          (((x) & 0xFFF) << 0)
#define   G_008DFC_OFFSET(x)                                          (((x) >> 0) & 0xFFF)
#define   C_008DFC_OFFSET                                             0xFFFFF000
#define   S_008DFC_OFFEN(x)                                           (((x) & 0x1) << 12)
#define   G_008DFC_OFFEN(x)                                           (((x) >> 12) & 0x1)
#define   C_008DFC_OFFEN                                              0xFFFFEFFF
#define   S_008DFC_IDXEN(x)                                           (((x) & 0x1) << 13)
#define   G_008DFC_IDXEN(x)                                           (((x) >> 13) & 0x1)
#define   C_008DFC_IDXEN                                              0xFFFFDFFF
#define   S_008DFC_GLC(x)                                             (((x) & 0x1) << 14)
#define   G_008DFC_GLC(x)                                             (((x) >> 14) & 0x1)
#define   C_008DFC_GLC                                                0xFFFFBFFF
#define   S_008DFC_ADDR64(x)                                          (((x) & 0x1) << 15)
#define   G_008DFC_ADDR64(x)                                          (((x) >> 15) & 0x1)
#define   C_008DFC_ADDR64                                             0xFFFF7FFF
#define   S_008DFC_OP(x)                                              (((x) & 0x07) << 16)
#define   G_008DFC_OP(x)                                              (((x) >> 16) & 0x07)
#define   C_008DFC_OP                                                 0xFFF8FFFF
#define     V_008DFC_SQ_TBUFFER_LOAD_FORMAT_X                       0x00
#define     V_008DFC_SQ_TBUFFER_LOAD_FORMAT_XY                      0x01
#define     V_008DFC_SQ_TBUFFER_LOAD_FORMAT_XYZ                     0x02
#define     V_008DFC_SQ_TBUFFER_LOAD_FORMAT_XYZW                    0x03
#define     V_008DFC_SQ_TBUFFER_STORE_FORMAT_X                      0x04
#define     V_008DFC_SQ_TBUFFER_STORE_FORMAT_XY                     0x05
#define     V_008DFC_SQ_TBUFFER_STORE_FORMAT_XYZ                    0x06
#define     V_008DFC_SQ_TBUFFER_STORE_FORMAT_XYZW                   0x07
#define   S_008DFC_DFMT(x)                                            (((x) & 0x0F) << 19)
#define   G_008DFC_DFMT(x)                                            (((x) >> 19) & 0x0F)
#define   C_008DFC_DFMT                                               0xFF87FFFF
#define   S_008DFC_NFMT(x)                                            (((x) & 0x07) << 23)
#define   G_008DFC_NFMT(x)                                            (((x) >> 23) & 0x07)
#define   C_008DFC_NFMT                                               0xFC7FFFFF
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_MTBUF_FIELD                             0x3A
#define R_008DFC_SQ_SMRD                                                0x008DFC
#define   S_008DFC_OFFSET(x)                                          (((x) & 0xFF) << 0)
#define   G_008DFC_OFFSET(x)                                          (((x) >> 0) & 0xFF)
#define   C_008DFC_OFFSET                                             0xFFFFFF00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define   S_008DFC_IMM(x)                                             (((x) & 0x1) << 8)
#define   G_008DFC_IMM(x)                                             (((x) >> 8) & 0x1)
#define   C_008DFC_IMM                                                0xFFFFFEFF
#define   S_008DFC_SBASE(x)                                           (((x) & 0x3F) << 9)
#define   G_008DFC_SBASE(x)                                           (((x) >> 9) & 0x3F)
#define   C_008DFC_SBASE                                              0xFFFF81FF
#define   S_008DFC_SDST(x)                                            (((x) & 0x7F) << 15)
#define   G_008DFC_SDST(x)                                            (((x) >> 15) & 0x7F)
#define   C_008DFC_SDST                                               0xFFC07FFF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define   S_008DFC_OP(x)                                              (((x) & 0x1F) << 22)
#define   G_008DFC_OP(x)                                              (((x) >> 22) & 0x1F)
#define   C_008DFC_OP                                                 0xF83FFFFF
#define     V_008DFC_SQ_S_LOAD_DWORD                                0x00
#define     V_008DFC_SQ_S_LOAD_DWORDX2                              0x01
#define     V_008DFC_SQ_S_LOAD_DWORDX4                              0x02
#define     V_008DFC_SQ_S_LOAD_DWORDX8                              0x03
#define     V_008DFC_SQ_S_LOAD_DWORDX16                             0x04
#define     V_008DFC_SQ_S_BUFFER_LOAD_DWORD                         0x08
#define     V_008DFC_SQ_S_BUFFER_LOAD_DWORDX2                       0x09
#define     V_008DFC_SQ_S_BUFFER_LOAD_DWORDX4                       0x0A
#define     V_008DFC_SQ_S_BUFFER_LOAD_DWORDX8                       0x0B
#define     V_008DFC_SQ_S_BUFFER_LOAD_DWORDX16                      0x0C
#define     V_008DFC_SQ_S_MEMTIME                                   0x1E
#define     V_008DFC_SQ_S_DCACHE_INV                                0x1F
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x1F) << 27)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 27) & 0x1F)
#define   C_008DFC_ENCODING                                           0x07FFFFFF
#define     V_008DFC_SQ_ENC_SMRD_FIELD                              0x18
#define R_008DFC_SQ_EXP_1                                               0x008DFC
#define   S_008DFC_VSRC0(x)                                           (((x) & 0xFF) << 0)
#define   G_008DFC_VSRC0(x)                                           (((x) >> 0) & 0xFF)
#define   C_008DFC_VSRC0                                              0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VSRC1(x)                                           (((x) & 0xFF) << 8)
#define   G_008DFC_VSRC1(x)                                           (((x) >> 8) & 0xFF)
#define   C_008DFC_VSRC1                                              0xFFFF00FF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VSRC2(x)                                           (((x) & 0xFF) << 16)
#define   G_008DFC_VSRC2(x)                                           (((x) >> 16) & 0xFF)
#define   C_008DFC_VSRC2                                              0xFF00FFFF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VSRC3(x)                                           (((x) & 0xFF) << 24)
#define   G_008DFC_VSRC3(x)                                           (((x) >> 24) & 0xFF)
#define   C_008DFC_VSRC3                                              0x00FFFFFF
#define     V_008DFC_SQ_VGPR                                        0x00
#define R_008DFC_SQ_DS_1                                                0x008DFC
#define   S_008DFC_ADDR(x)                                            (((x) & 0xFF) << 0)
#define   G_008DFC_ADDR(x)                                            (((x) >> 0) & 0xFF)
#define   C_008DFC_ADDR                                               0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_DATA0(x)                                           (((x) & 0xFF) << 8)
#define   G_008DFC_DATA0(x)                                           (((x) >> 8) & 0xFF)
#define   C_008DFC_DATA0                                              0xFFFF00FF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_DATA1(x)                                           (((x) & 0xFF) << 16)
#define   G_008DFC_DATA1(x)                                           (((x) >> 16) & 0xFF)
#define   C_008DFC_DATA1                                              0xFF00FFFF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VDST(x)                                            (((x) & 0xFF) << 24)
#define   G_008DFC_VDST(x)                                            (((x) >> 24) & 0xFF)
#define   C_008DFC_VDST                                               0x00FFFFFF
#define     V_008DFC_SQ_VGPR                                        0x00
#define R_008DFC_SQ_VOPC                                                0x008DFC
#define   S_008DFC_SRC0(x)                                            (((x) & 0x1FF) << 0)
#define   G_008DFC_SRC0(x)                                            (((x) >> 0) & 0x1FF)
#define   C_008DFC_SRC0                                               0xFFFFFE00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define     V_008DFC_SQ_SRC_VGPR                                    0x100
#define   S_008DFC_VSRC1(x)                                           (((x) & 0xFF) << 9)
#define   G_008DFC_VSRC1(x)                                           (((x) >> 9) & 0xFF)
#define   C_008DFC_VSRC1                                              0xFFFE01FF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_OP(x)                                              (((x) & 0xFF) << 17)
#define   G_008DFC_OP(x)                                              (((x) >> 17) & 0xFF)
#define   C_008DFC_OP                                                 0xFE01FFFF
#define     V_008DFC_SQ_V_CMP_F_F32                                 0x00
#define     V_008DFC_SQ_V_CMP_LT_F32                                0x01
#define     V_008DFC_SQ_V_CMP_EQ_F32                                0x02
#define     V_008DFC_SQ_V_CMP_LE_F32                                0x03
#define     V_008DFC_SQ_V_CMP_GT_F32                                0x04
#define     V_008DFC_SQ_V_CMP_LG_F32                                0x05
#define     V_008DFC_SQ_V_CMP_GE_F32                                0x06
#define     V_008DFC_SQ_V_CMP_O_F32                                 0x07
#define     V_008DFC_SQ_V_CMP_U_F32                                 0x08
#define     V_008DFC_SQ_V_CMP_NGE_F32                               0x09
#define     V_008DFC_SQ_V_CMP_NLG_F32                               0x0A
#define     V_008DFC_SQ_V_CMP_NGT_F32                               0x0B
#define     V_008DFC_SQ_V_CMP_NLE_F32                               0x0C
#define     V_008DFC_SQ_V_CMP_NEQ_F32                               0x0D
#define     V_008DFC_SQ_V_CMP_NLT_F32                               0x0E
#define     V_008DFC_SQ_V_CMP_TRU_F32                               0x0F
#define     V_008DFC_SQ_V_CMPX_F_F32                                0x10
#define     V_008DFC_SQ_V_CMPX_LT_F32                               0x11
#define     V_008DFC_SQ_V_CMPX_EQ_F32                               0x12
#define     V_008DFC_SQ_V_CMPX_LE_F32                               0x13
#define     V_008DFC_SQ_V_CMPX_GT_F32                               0x14
#define     V_008DFC_SQ_V_CMPX_LG_F32                               0x15
#define     V_008DFC_SQ_V_CMPX_GE_F32                               0x16
#define     V_008DFC_SQ_V_CMPX_O_F32                                0x17
#define     V_008DFC_SQ_V_CMPX_U_F32                                0x18
#define     V_008DFC_SQ_V_CMPX_NGE_F32                              0x19
#define     V_008DFC_SQ_V_CMPX_NLG_F32                              0x1A
#define     V_008DFC_SQ_V_CMPX_NGT_F32                              0x1B
#define     V_008DFC_SQ_V_CMPX_NLE_F32                              0x1C
#define     V_008DFC_SQ_V_CMPX_NEQ_F32                              0x1D
#define     V_008DFC_SQ_V_CMPX_NLT_F32                              0x1E
#define     V_008DFC_SQ_V_CMPX_TRU_F32                              0x1F
#define     V_008DFC_SQ_V_CMP_F_F64                                 0x20
#define     V_008DFC_SQ_V_CMP_LT_F64                                0x21
#define     V_008DFC_SQ_V_CMP_EQ_F64                                0x22
#define     V_008DFC_SQ_V_CMP_LE_F64                                0x23
#define     V_008DFC_SQ_V_CMP_GT_F64                                0x24
#define     V_008DFC_SQ_V_CMP_LG_F64                                0x25
#define     V_008DFC_SQ_V_CMP_GE_F64                                0x26
#define     V_008DFC_SQ_V_CMP_O_F64                                 0x27
#define     V_008DFC_SQ_V_CMP_U_F64                                 0x28
#define     V_008DFC_SQ_V_CMP_NGE_F64                               0x29
#define     V_008DFC_SQ_V_CMP_NLG_F64                               0x2A
#define     V_008DFC_SQ_V_CMP_NGT_F64                               0x2B
#define     V_008DFC_SQ_V_CMP_NLE_F64                               0x2C
#define     V_008DFC_SQ_V_CMP_NEQ_F64                               0x2D
#define     V_008DFC_SQ_V_CMP_NLT_F64                               0x2E
#define     V_008DFC_SQ_V_CMP_TRU_F64                               0x2F
#define     V_008DFC_SQ_V_CMPX_F_F64                                0x30
#define     V_008DFC_SQ_V_CMPX_LT_F64                               0x31
#define     V_008DFC_SQ_V_CMPX_EQ_F64                               0x32
#define     V_008DFC_SQ_V_CMPX_LE_F64                               0x33
#define     V_008DFC_SQ_V_CMPX_GT_F64                               0x34
#define     V_008DFC_SQ_V_CMPX_LG_F64                               0x35
#define     V_008DFC_SQ_V_CMPX_GE_F64                               0x36
#define     V_008DFC_SQ_V_CMPX_O_F64                                0x37
#define     V_008DFC_SQ_V_CMPX_U_F64                                0x38
#define     V_008DFC_SQ_V_CMPX_NGE_F64                              0x39
#define     V_008DFC_SQ_V_CMPX_NLG_F64                              0x3A
#define     V_008DFC_SQ_V_CMPX_NGT_F64                              0x3B
#define     V_008DFC_SQ_V_CMPX_NLE_F64                              0x3C
#define     V_008DFC_SQ_V_CMPX_NEQ_F64                              0x3D
#define     V_008DFC_SQ_V_CMPX_NLT_F64                              0x3E
#define     V_008DFC_SQ_V_CMPX_TRU_F64                              0x3F
#define     V_008DFC_SQ_V_CMPS_F_F32                                0x40
#define     V_008DFC_SQ_V_CMPS_LT_F32                               0x41
#define     V_008DFC_SQ_V_CMPS_EQ_F32                               0x42
#define     V_008DFC_SQ_V_CMPS_LE_F32                               0x43
#define     V_008DFC_SQ_V_CMPS_GT_F32                               0x44
#define     V_008DFC_SQ_V_CMPS_LG_F32                               0x45
#define     V_008DFC_SQ_V_CMPS_GE_F32                               0x46
#define     V_008DFC_SQ_V_CMPS_O_F32                                0x47
#define     V_008DFC_SQ_V_CMPS_U_F32                                0x48
#define     V_008DFC_SQ_V_CMPS_NGE_F32                              0x49
#define     V_008DFC_SQ_V_CMPS_NLG_F32                              0x4A
#define     V_008DFC_SQ_V_CMPS_NGT_F32                              0x4B
#define     V_008DFC_SQ_V_CMPS_NLE_F32                              0x4C
#define     V_008DFC_SQ_V_CMPS_NEQ_F32                              0x4D
#define     V_008DFC_SQ_V_CMPS_NLT_F32                              0x4E
#define     V_008DFC_SQ_V_CMPS_TRU_F32                              0x4F
#define     V_008DFC_SQ_V_CMPSX_F_F32                               0x50
#define     V_008DFC_SQ_V_CMPSX_LT_F32                              0x51
#define     V_008DFC_SQ_V_CMPSX_EQ_F32                              0x52
#define     V_008DFC_SQ_V_CMPSX_LE_F32                              0x53
#define     V_008DFC_SQ_V_CMPSX_GT_F32                              0x54
#define     V_008DFC_SQ_V_CMPSX_LG_F32                              0x55
#define     V_008DFC_SQ_V_CMPSX_GE_F32                              0x56
#define     V_008DFC_SQ_V_CMPSX_O_F32                               0x57
#define     V_008DFC_SQ_V_CMPSX_U_F32                               0x58
#define     V_008DFC_SQ_V_CMPSX_NGE_F32                             0x59
#define     V_008DFC_SQ_V_CMPSX_NLG_F32                             0x5A
#define     V_008DFC_SQ_V_CMPSX_NGT_F32                             0x5B
#define     V_008DFC_SQ_V_CMPSX_NLE_F32                             0x5C
#define     V_008DFC_SQ_V_CMPSX_NEQ_F32                             0x5D
#define     V_008DFC_SQ_V_CMPSX_NLT_F32                             0x5E
#define     V_008DFC_SQ_V_CMPSX_TRU_F32                             0x5F
#define     V_008DFC_SQ_V_CMPS_F_F64                                0x60
#define     V_008DFC_SQ_V_CMPS_LT_F64                               0x61
#define     V_008DFC_SQ_V_CMPS_EQ_F64                               0x62
#define     V_008DFC_SQ_V_CMPS_LE_F64                               0x63
#define     V_008DFC_SQ_V_CMPS_GT_F64                               0x64
#define     V_008DFC_SQ_V_CMPS_LG_F64                               0x65
#define     V_008DFC_SQ_V_CMPS_GE_F64                               0x66
#define     V_008DFC_SQ_V_CMPS_O_F64                                0x67
#define     V_008DFC_SQ_V_CMPS_U_F64                                0x68
#define     V_008DFC_SQ_V_CMPS_NGE_F64                              0x69
#define     V_008DFC_SQ_V_CMPS_NLG_F64                              0x6A
#define     V_008DFC_SQ_V_CMPS_NGT_F64                              0x6B
#define     V_008DFC_SQ_V_CMPS_NLE_F64                              0x6C
#define     V_008DFC_SQ_V_CMPS_NEQ_F64                              0x6D
#define     V_008DFC_SQ_V_CMPS_NLT_F64                              0x6E
#define     V_008DFC_SQ_V_CMPS_TRU_F64                              0x6F
#define     V_008DFC_SQ_V_CMPSX_F_F64                               0x70
#define     V_008DFC_SQ_V_CMPSX_LT_F64                              0x71
#define     V_008DFC_SQ_V_CMPSX_EQ_F64                              0x72
#define     V_008DFC_SQ_V_CMPSX_LE_F64                              0x73
#define     V_008DFC_SQ_V_CMPSX_GT_F64                              0x74
#define     V_008DFC_SQ_V_CMPSX_LG_F64                              0x75
#define     V_008DFC_SQ_V_CMPSX_GE_F64                              0x76
#define     V_008DFC_SQ_V_CMPSX_O_F64                               0x77
#define     V_008DFC_SQ_V_CMPSX_U_F64                               0x78
#define     V_008DFC_SQ_V_CMPSX_NGE_F64                             0x79
#define     V_008DFC_SQ_V_CMPSX_NLG_F64                             0x7A
#define     V_008DFC_SQ_V_CMPSX_NGT_F64                             0x7B
#define     V_008DFC_SQ_V_CMPSX_NLE_F64                             0x7C
#define     V_008DFC_SQ_V_CMPSX_NEQ_F64                             0x7D
#define     V_008DFC_SQ_V_CMPSX_NLT_F64                             0x7E
#define     V_008DFC_SQ_V_CMPSX_TRU_F64                             0x7F
#define     V_008DFC_SQ_V_CMP_F_I32                                 0x80
#define     V_008DFC_SQ_V_CMP_LT_I32                                0x81
#define     V_008DFC_SQ_V_CMP_EQ_I32                                0x82
#define     V_008DFC_SQ_V_CMP_LE_I32                                0x83
#define     V_008DFC_SQ_V_CMP_GT_I32                                0x84
#define     V_008DFC_SQ_V_CMP_NE_I32                                0x85
#define     V_008DFC_SQ_V_CMP_GE_I32                                0x86
#define     V_008DFC_SQ_V_CMP_T_I32                                 0x87
#define     V_008DFC_SQ_V_CMP_CLASS_F32                             0x88
#define     V_008DFC_SQ_V_CMPX_F_I32                                0x90
#define     V_008DFC_SQ_V_CMPX_LT_I32                               0x91
#define     V_008DFC_SQ_V_CMPX_EQ_I32                               0x92
#define     V_008DFC_SQ_V_CMPX_LE_I32                               0x93
#define     V_008DFC_SQ_V_CMPX_GT_I32                               0x94
#define     V_008DFC_SQ_V_CMPX_NE_I32                               0x95
#define     V_008DFC_SQ_V_CMPX_GE_I32                               0x96
#define     V_008DFC_SQ_V_CMPX_T_I32                                0x97
#define     V_008DFC_SQ_V_CMPX_CLASS_F32                            0x98
#define     V_008DFC_SQ_V_CMP_F_I64                                 0xA0
#define     V_008DFC_SQ_V_CMP_LT_I64                                0xA1
#define     V_008DFC_SQ_V_CMP_EQ_I64                                0xA2
#define     V_008DFC_SQ_V_CMP_LE_I64                                0xA3
#define     V_008DFC_SQ_V_CMP_GT_I64                                0xA4
#define     V_008DFC_SQ_V_CMP_NE_I64                                0xA5
#define     V_008DFC_SQ_V_CMP_GE_I64                                0xA6
#define     V_008DFC_SQ_V_CMP_T_I64                                 0xA7
#define     V_008DFC_SQ_V_CMP_CLASS_F64                             0xA8
#define     V_008DFC_SQ_V_CMPX_F_I64                                0xB0
#define     V_008DFC_SQ_V_CMPX_LT_I64                               0xB1
#define     V_008DFC_SQ_V_CMPX_EQ_I64                               0xB2
#define     V_008DFC_SQ_V_CMPX_LE_I64                               0xB3
#define     V_008DFC_SQ_V_CMPX_GT_I64                               0xB4
#define     V_008DFC_SQ_V_CMPX_NE_I64                               0xB5
#define     V_008DFC_SQ_V_CMPX_GE_I64                               0xB6
#define     V_008DFC_SQ_V_CMPX_T_I64                                0xB7
#define     V_008DFC_SQ_V_CMPX_CLASS_F64                            0xB8
#define     V_008DFC_SQ_V_CMP_F_U32                                 0xC0
#define     V_008DFC_SQ_V_CMP_LT_U32                                0xC1
#define     V_008DFC_SQ_V_CMP_EQ_U32                                0xC2
#define     V_008DFC_SQ_V_CMP_LE_U32                                0xC3
#define     V_008DFC_SQ_V_CMP_GT_U32                                0xC4
#define     V_008DFC_SQ_V_CMP_NE_U32                                0xC5
#define     V_008DFC_SQ_V_CMP_GE_U32                                0xC6
#define     V_008DFC_SQ_V_CMP_T_U32                                 0xC7
#define     V_008DFC_SQ_V_CMPX_F_U32                                0xD0
#define     V_008DFC_SQ_V_CMPX_LT_U32                               0xD1
#define     V_008DFC_SQ_V_CMPX_EQ_U32                               0xD2
#define     V_008DFC_SQ_V_CMPX_LE_U32                               0xD3
#define     V_008DFC_SQ_V_CMPX_GT_U32                               0xD4
#define     V_008DFC_SQ_V_CMPX_NE_U32                               0xD5
#define     V_008DFC_SQ_V_CMPX_GE_U32                               0xD6
#define     V_008DFC_SQ_V_CMPX_T_U32                                0xD7
#define     V_008DFC_SQ_V_CMP_F_U64                                 0xE0
#define     V_008DFC_SQ_V_CMP_LT_U64                                0xE1
#define     V_008DFC_SQ_V_CMP_EQ_U64                                0xE2
#define     V_008DFC_SQ_V_CMP_LE_U64                                0xE3
#define     V_008DFC_SQ_V_CMP_GT_U64                                0xE4
#define     V_008DFC_SQ_V_CMP_NE_U64                                0xE5
#define     V_008DFC_SQ_V_CMP_GE_U64                                0xE6
#define     V_008DFC_SQ_V_CMP_T_U64                                 0xE7
#define     V_008DFC_SQ_V_CMPX_F_U64                                0xF0
#define     V_008DFC_SQ_V_CMPX_LT_U64                               0xF1
#define     V_008DFC_SQ_V_CMPX_EQ_U64                               0xF2
#define     V_008DFC_SQ_V_CMPX_LE_U64                               0xF3
#define     V_008DFC_SQ_V_CMPX_GT_U64                               0xF4
#define     V_008DFC_SQ_V_CMPX_NE_U64                               0xF5
#define     V_008DFC_SQ_V_CMPX_GE_U64                               0xF6
#define     V_008DFC_SQ_V_CMPX_T_U64                                0xF7
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x7F) << 25)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 25) & 0x7F)
#define   C_008DFC_ENCODING                                           0x01FFFFFF
#define     V_008DFC_SQ_ENC_VOPC_FIELD                              0x3E
#define R_008DFC_SQ_SOP1                                                0x008DFC
#define   S_008DFC_SSRC0(x)                                           (((x) & 0xFF) << 0)
#define   G_008DFC_SSRC0(x)                                           (((x) >> 0) & 0xFF)
#define   C_008DFC_SSRC0                                              0xFFFFFF00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define   S_008DFC_OP(x)                                              (((x) & 0xFF) << 8)
#define   G_008DFC_OP(x)                                              (((x) >> 8) & 0xFF)
#define   C_008DFC_OP                                                 0xFFFF00FF
#define     V_008DFC_SQ_S_MOV_B32                                   0x03
#define     V_008DFC_SQ_S_MOV_B64                                   0x04
#define     V_008DFC_SQ_S_CMOV_B32                                  0x05
#define     V_008DFC_SQ_S_CMOV_B64                                  0x06
#define     V_008DFC_SQ_S_NOT_B32                                   0x07
#define     V_008DFC_SQ_S_NOT_B64                                   0x08
#define     V_008DFC_SQ_S_WQM_B32                                   0x09
#define     V_008DFC_SQ_S_WQM_B64                                   0x0A
#define     V_008DFC_SQ_S_BREV_B32                                  0x0B
#define     V_008DFC_SQ_S_BREV_B64                                  0x0C
#define     V_008DFC_SQ_S_BCNT0_I32_B32                             0x0D
#define     V_008DFC_SQ_S_BCNT0_I32_B64                             0x0E
#define     V_008DFC_SQ_S_BCNT1_I32_B32                             0x0F
#define     V_008DFC_SQ_S_BCNT1_I32_B64                             0x10
#define     V_008DFC_SQ_S_FF0_I32_B32                               0x11
#define     V_008DFC_SQ_S_FF0_I32_B64                               0x12
#define     V_008DFC_SQ_S_FF1_I32_B32                               0x13
#define     V_008DFC_SQ_S_FF1_I32_B64                               0x14
#define     V_008DFC_SQ_S_FLBIT_I32_B32                             0x15
#define     V_008DFC_SQ_S_FLBIT_I32_B64                             0x16
#define     V_008DFC_SQ_S_FLBIT_I32                                 0x17
#define     V_008DFC_SQ_S_FLBIT_I32_I64                             0x18
#define     V_008DFC_SQ_S_SEXT_I32_I8                               0x19
#define     V_008DFC_SQ_S_SEXT_I32_I16                              0x1A
#define     V_008DFC_SQ_S_BITSET0_B32                               0x1B
#define     V_008DFC_SQ_S_BITSET0_B64                               0x1C
#define     V_008DFC_SQ_S_BITSET1_B32                               0x1D
#define     V_008DFC_SQ_S_BITSET1_B64                               0x1E
#define     V_008DFC_SQ_S_GETPC_B64                                 0x1F
#define     V_008DFC_SQ_S_SETPC_B64                                 0x20
#define     V_008DFC_SQ_S_SWAPPC_B64                                0x21
#define     V_008DFC_SQ_S_RFE_B64                                   0x22
#define     V_008DFC_SQ_S_AND_SAVEEXEC_B64                          0x24
#define     V_008DFC_SQ_S_OR_SAVEEXEC_B64                           0x25
#define     V_008DFC_SQ_S_XOR_SAVEEXEC_B64                          0x26
#define     V_008DFC_SQ_S_ANDN2_SAVEEXEC_B64                        0x27
#define     V_008DFC_SQ_S_ORN2_SAVEEXEC_B64                         0x28
#define     V_008DFC_SQ_S_NAND_SAVEEXEC_B64                         0x29
#define     V_008DFC_SQ_S_NOR_SAVEEXEC_B64                          0x2A
#define     V_008DFC_SQ_S_XNOR_SAVEEXEC_B64                         0x2B
#define     V_008DFC_SQ_S_QUADMASK_B32                              0x2C
#define     V_008DFC_SQ_S_QUADMASK_B64                              0x2D
#define     V_008DFC_SQ_S_MOVRELS_B32                               0x2E
#define     V_008DFC_SQ_S_MOVRELS_B64                               0x2F
#define     V_008DFC_SQ_S_MOVRELD_B32                               0x30
#define     V_008DFC_SQ_S_MOVRELD_B64                               0x31
#define     V_008DFC_SQ_S_CBRANCH_JOIN                              0x32
#define     V_008DFC_SQ_S_MOV_REGRD_B32                             0x33
#define     V_008DFC_SQ_S_ABS_I32                                   0x34
#define     V_008DFC_SQ_S_MOV_FED_B32                               0x35
#define   S_008DFC_SDST(x)                                            (((x) & 0x7F) << 16)
#define   G_008DFC_SDST(x)                                            (((x) >> 16) & 0x7F)
#define   C_008DFC_SDST                                               0xFF80FFFF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x1FF) << 23)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 23) & 0x1FF)
#define   C_008DFC_ENCODING                                           0x007FFFFF
#define     V_008DFC_SQ_ENC_SOP1_FIELD                              0x17D
#define R_008DFC_SQ_MTBUF_1                                             0x008DFC
#define   S_008DFC_VADDR(x)                                           (((x) & 0xFF) << 0)
#define   G_008DFC_VADDR(x)                                           (((x) >> 0) & 0xFF)
#define   C_008DFC_VADDR                                              0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VDATA(x)                                           (((x) & 0xFF) << 8)
#define   G_008DFC_VDATA(x)                                           (((x) >> 8) & 0xFF)
#define   C_008DFC_VDATA                                              0xFFFF00FF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_SRSRC(x)                                           (((x) & 0x1F) << 16)
#define   G_008DFC_SRSRC(x)                                           (((x) >> 16) & 0x1F)
#define   C_008DFC_SRSRC                                              0xFFE0FFFF
#define   S_008DFC_SLC(x)                                             (((x) & 0x1) << 22)
#define   G_008DFC_SLC(x)                                             (((x) >> 22) & 0x1)
#define   C_008DFC_SLC                                                0xFFBFFFFF
#define   S_008DFC_TFE(x)                                             (((x) & 0x1) << 23)
#define   G_008DFC_TFE(x)                                             (((x) >> 23) & 0x1)
#define   C_008DFC_TFE                                                0xFF7FFFFF
#define   S_008DFC_SOFFSET(x)                                         (((x) & 0xFF) << 24)
#define   G_008DFC_SOFFSET(x)                                         (((x) >> 24) & 0xFF)
#define   C_008DFC_SOFFSET                                            0x00FFFFFF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define R_008DFC_SQ_SOP2                                                0x008DFC
#define   S_008DFC_SSRC0(x)                                           (((x) & 0xFF) << 0)
#define   G_008DFC_SSRC0(x)                                           (((x) >> 0) & 0xFF)
#define   C_008DFC_SSRC0                                              0xFFFFFF00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define   S_008DFC_SSRC1(x)                                           (((x) & 0xFF) << 8)
#define   G_008DFC_SSRC1(x)                                           (((x) >> 8) & 0xFF)
#define   C_008DFC_SSRC1                                              0xFFFF00FF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define   S_008DFC_SDST(x)                                            (((x) & 0x7F) << 16)
#define   G_008DFC_SDST(x)                                            (((x) >> 16) & 0x7F)
#define   C_008DFC_SDST                                               0xFF80FFFF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define   S_008DFC_OP(x)                                              (((x) & 0x7F) << 23)
#define   G_008DFC_OP(x)                                              (((x) >> 23) & 0x7F)
#define   C_008DFC_OP                                                 0xC07FFFFF
#define     V_008DFC_SQ_S_ADD_U32                                   0x00
#define     V_008DFC_SQ_S_SUB_U32                                   0x01
#define     V_008DFC_SQ_S_ADD_I32                                   0x02
#define     V_008DFC_SQ_S_SUB_I32                                   0x03
#define     V_008DFC_SQ_S_ADDC_U32                                  0x04
#define     V_008DFC_SQ_S_SUBB_U32                                  0x05
#define     V_008DFC_SQ_S_MIN_I32                                   0x06
#define     V_008DFC_SQ_S_MIN_U32                                   0x07
#define     V_008DFC_SQ_S_MAX_I32                                   0x08
#define     V_008DFC_SQ_S_MAX_U32                                   0x09
#define     V_008DFC_SQ_S_CSELECT_B32                               0x0A
#define     V_008DFC_SQ_S_CSELECT_B64                               0x0B
#define     V_008DFC_SQ_S_AND_B32                                   0x0E
#define     V_008DFC_SQ_S_AND_B64                                   0x0F
#define     V_008DFC_SQ_S_OR_B32                                    0x10
#define     V_008DFC_SQ_S_OR_B64                                    0x11
#define     V_008DFC_SQ_S_XOR_B32                                   0x12
#define     V_008DFC_SQ_S_XOR_B64                                   0x13
#define     V_008DFC_SQ_S_ANDN2_B32                                 0x14
#define     V_008DFC_SQ_S_ANDN2_B64                                 0x15
#define     V_008DFC_SQ_S_ORN2_B32                                  0x16
#define     V_008DFC_SQ_S_ORN2_B64                                  0x17
#define     V_008DFC_SQ_S_NAND_B32                                  0x18
#define     V_008DFC_SQ_S_NAND_B64                                  0x19
#define     V_008DFC_SQ_S_NOR_B32                                   0x1A
#define     V_008DFC_SQ_S_NOR_B64                                   0x1B
#define     V_008DFC_SQ_S_XNOR_B32                                  0x1C
#define     V_008DFC_SQ_S_XNOR_B64                                  0x1D
#define     V_008DFC_SQ_S_LSHL_B32                                  0x1E
#define     V_008DFC_SQ_S_LSHL_B64                                  0x1F
#define     V_008DFC_SQ_S_LSHR_B32                                  0x20
#define     V_008DFC_SQ_S_LSHR_B64                                  0x21
#define     V_008DFC_SQ_S_ASHR_I32                                  0x22
#define     V_008DFC_SQ_S_ASHR_I64                                  0x23
#define     V_008DFC_SQ_S_BFM_B32                                   0x24
#define     V_008DFC_SQ_S_BFM_B64                                   0x25
#define     V_008DFC_SQ_S_MUL_I32                                   0x26
#define     V_008DFC_SQ_S_BFE_U32                                   0x27
#define     V_008DFC_SQ_S_BFE_I32                                   0x28
#define     V_008DFC_SQ_S_BFE_U64                                   0x29
#define     V_008DFC_SQ_S_BFE_I64                                   0x2A
#define     V_008DFC_SQ_S_CBRANCH_G_FORK                            0x2B
#define     V_008DFC_SQ_S_ABSDIFF_I32                               0x2C
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x03) << 30)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 30) & 0x03)
#define   C_008DFC_ENCODING                                           0x3FFFFFFF
#define     V_008DFC_SQ_ENC_SOP2_FIELD                              0x02
#define R_008DFC_SQ_SOPK                                                0x008DFC
#define   S_008DFC_SIMM16(x)                                          (((x) & 0xFFFF) << 0)
#define   G_008DFC_SIMM16(x)                                          (((x) >> 0) & 0xFFFF)
#define   C_008DFC_SIMM16                                             0xFFFF0000
#define   S_008DFC_SDST(x)                                            (((x) & 0x7F) << 16)
#define   G_008DFC_SDST(x)                                            (((x) >> 16) & 0x7F)
#define   C_008DFC_SDST                                               0xFF80FFFF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define   S_008DFC_OP(x)                                              (((x) & 0x1F) << 23)
#define   G_008DFC_OP(x)                                              (((x) >> 23) & 0x1F)
#define   C_008DFC_OP                                                 0xF07FFFFF
#define     V_008DFC_SQ_S_MOVK_I32                                  0x00
#define     V_008DFC_SQ_S_CMOVK_I32                                 0x02
#define     V_008DFC_SQ_S_CMPK_EQ_I32                               0x03
#define     V_008DFC_SQ_S_CMPK_LG_I32                               0x04
#define     V_008DFC_SQ_S_CMPK_GT_I32                               0x05
#define     V_008DFC_SQ_S_CMPK_GE_I32                               0x06
#define     V_008DFC_SQ_S_CMPK_LT_I32                               0x07
#define     V_008DFC_SQ_S_CMPK_LE_I32                               0x08
#define     V_008DFC_SQ_S_CMPK_EQ_U32                               0x09
#define     V_008DFC_SQ_S_CMPK_LG_U32                               0x0A
#define     V_008DFC_SQ_S_CMPK_GT_U32                               0x0B
#define     V_008DFC_SQ_S_CMPK_GE_U32                               0x0C
#define     V_008DFC_SQ_S_CMPK_LT_U32                               0x0D
#define     V_008DFC_SQ_S_CMPK_LE_U32                               0x0E
#define     V_008DFC_SQ_S_ADDK_I32                                  0x0F
#define     V_008DFC_SQ_S_MULK_I32                                  0x10
#define     V_008DFC_SQ_S_CBRANCH_I_FORK                            0x11
#define     V_008DFC_SQ_S_GETREG_B32                                0x12
#define     V_008DFC_SQ_S_SETREG_B32                                0x13
#define     V_008DFC_SQ_S_GETREG_REGRD_B32                          0x14
#define     V_008DFC_SQ_S_SETREG_IMM32_B32                          0x15
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x0F) << 28)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 28) & 0x0F)
#define   C_008DFC_ENCODING                                           0x0FFFFFFF
#define     V_008DFC_SQ_ENC_SOPK_FIELD                              0x0B
#define R_008DFC_SQ_VOP3_0                                              0x008DFC
#define   S_008DFC_VDST(x)                                            (((x) & 0xFF) << 0)
#define   G_008DFC_VDST(x)                                            (((x) >> 0) & 0xFF)
#define   C_008DFC_VDST                                               0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_ABS(x)                                             (((x) & 0x07) << 8)
#define   G_008DFC_ABS(x)                                             (((x) >> 8) & 0x07)
#define   C_008DFC_ABS                                                0xFFFFF8FF
#define   S_008DFC_CLAMP(x)                                           (((x) & 0x1) << 11)
#define   G_008DFC_CLAMP(x)                                           (((x) >> 11) & 0x1)
#define   C_008DFC_CLAMP                                              0xFFFFF7FF
#define   S_008DFC_OP(x)                                              (((x) & 0x1FF) << 17)
#define   G_008DFC_OP(x)                                              (((x) >> 17) & 0x1FF)
#define   C_008DFC_OP                                                 0xFC01FFFF
#define     V_008DFC_SQ_V_OPC_OFFSET                                0x00
#define     V_008DFC_SQ_V_OP2_OFFSET                                0x100
#define     V_008DFC_SQ_V_MAD_LEGACY_F32                            0x140
#define     V_008DFC_SQ_V_MAD_F32                                   0x141
#define     V_008DFC_SQ_V_MAD_I32_I24                               0x142
#define     V_008DFC_SQ_V_MAD_U32_U24                               0x143
#define     V_008DFC_SQ_V_CUBEID_F32                                0x144
#define     V_008DFC_SQ_V_CUBESC_F32                                0x145
#define     V_008DFC_SQ_V_CUBETC_F32                                0x146
#define     V_008DFC_SQ_V_CUBEMA_F32                                0x147
#define     V_008DFC_SQ_V_BFE_U32                                   0x148
#define     V_008DFC_SQ_V_BFE_I32                                   0x149
#define     V_008DFC_SQ_V_BFI_B32                                   0x14A
#define     V_008DFC_SQ_V_FMA_F32                                   0x14B
#define     V_008DFC_SQ_V_FMA_F64                                   0x14C
#define     V_008DFC_SQ_V_LERP_U8                                   0x14D
#define     V_008DFC_SQ_V_ALIGNBIT_B32                              0x14E
#define     V_008DFC_SQ_V_ALIGNBYTE_B32                             0x14F
#define     V_008DFC_SQ_V_MULLIT_F32                                0x150
#define     V_008DFC_SQ_V_MIN3_F32                                  0x151
#define     V_008DFC_SQ_V_MIN3_I32                                  0x152
#define     V_008DFC_SQ_V_MIN3_U32                                  0x153
#define     V_008DFC_SQ_V_MAX3_F32                                  0x154
#define     V_008DFC_SQ_V_MAX3_I32                                  0x155
#define     V_008DFC_SQ_V_MAX3_U32                                  0x156
#define     V_008DFC_SQ_V_MED3_F32                                  0x157
#define     V_008DFC_SQ_V_MED3_I32                                  0x158
#define     V_008DFC_SQ_V_MED3_U32                                  0x159
#define     V_008DFC_SQ_V_SAD_U8                                    0x15A
#define     V_008DFC_SQ_V_SAD_HI_U8                                 0x15B
#define     V_008DFC_SQ_V_SAD_U16                                   0x15C
#define     V_008DFC_SQ_V_SAD_U32                                   0x15D
#define     V_008DFC_SQ_V_CVT_PK_U8_F32                             0x15E
#define     V_008DFC_SQ_V_DIV_FIXUP_F32                             0x15F
#define     V_008DFC_SQ_V_DIV_FIXUP_F64                             0x160
#define     V_008DFC_SQ_V_LSHL_B64                                  0x161
#define     V_008DFC_SQ_V_LSHR_B64                                  0x162
#define     V_008DFC_SQ_V_ASHR_I64                                  0x163
#define     V_008DFC_SQ_V_ADD_F64                                   0x164
#define     V_008DFC_SQ_V_MUL_F64                                   0x165
#define     V_008DFC_SQ_V_MIN_F64                                   0x166
#define     V_008DFC_SQ_V_MAX_F64                                   0x167
#define     V_008DFC_SQ_V_LDEXP_F64                                 0x168
#define     V_008DFC_SQ_V_MUL_LO_U32                                0x169
#define     V_008DFC_SQ_V_MUL_HI_U32                                0x16A
#define     V_008DFC_SQ_V_MUL_LO_I32                                0x16B
#define     V_008DFC_SQ_V_MUL_HI_I32                                0x16C
#define     V_008DFC_SQ_V_DIV_SCALE_F32                             0x16D
#define     V_008DFC_SQ_V_DIV_SCALE_F64                             0x16E
#define     V_008DFC_SQ_V_DIV_FMAS_F32                              0x16F
#define     V_008DFC_SQ_V_DIV_FMAS_F64                              0x170
#define     V_008DFC_SQ_V_MSAD_U8                                   0x171
#define     V_008DFC_SQ_V_QSAD_U8                                   0x172
#define     V_008DFC_SQ_V_MQSAD_U8                                  0x173
#define     V_008DFC_SQ_V_TRIG_PREOP_F64                            0x174
#define     V_008DFC_SQ_V_OP1_OFFSET                                0x180
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_VOP3_FIELD                              0x34
#define R_008DFC_SQ_VOP2                                                0x008DFC
#define   S_008DFC_SRC0(x)                                            (((x) & 0x1FF) << 0)
#define   G_008DFC_SRC0(x)                                            (((x) >> 0) & 0x1FF)
#define   C_008DFC_SRC0                                               0xFFFFFE00
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define     V_008DFC_SQ_M0                                          0x7C
#define     V_008DFC_SQ_EXEC_LO                                     0x7E
#define     V_008DFC_SQ_EXEC_HI                                     0x7F
#define     V_008DFC_SQ_SRC_0                                       0x80
#define     V_008DFC_SQ_SRC_1_INT                                   0x81
#define     V_008DFC_SQ_SRC_2_INT                                   0x82
#define     V_008DFC_SQ_SRC_3_INT                                   0x83
#define     V_008DFC_SQ_SRC_4_INT                                   0x84
#define     V_008DFC_SQ_SRC_5_INT                                   0x85
#define     V_008DFC_SQ_SRC_6_INT                                   0x86
#define     V_008DFC_SQ_SRC_7_INT                                   0x87
#define     V_008DFC_SQ_SRC_8_INT                                   0x88
#define     V_008DFC_SQ_SRC_9_INT                                   0x89
#define     V_008DFC_SQ_SRC_10_INT                                  0x8A
#define     V_008DFC_SQ_SRC_11_INT                                  0x8B
#define     V_008DFC_SQ_SRC_12_INT                                  0x8C
#define     V_008DFC_SQ_SRC_13_INT                                  0x8D
#define     V_008DFC_SQ_SRC_14_INT                                  0x8E
#define     V_008DFC_SQ_SRC_15_INT                                  0x8F
#define     V_008DFC_SQ_SRC_16_INT                                  0x90
#define     V_008DFC_SQ_SRC_17_INT                                  0x91
#define     V_008DFC_SQ_SRC_18_INT                                  0x92
#define     V_008DFC_SQ_SRC_19_INT                                  0x93
#define     V_008DFC_SQ_SRC_20_INT                                  0x94
#define     V_008DFC_SQ_SRC_21_INT                                  0x95
#define     V_008DFC_SQ_SRC_22_INT                                  0x96
#define     V_008DFC_SQ_SRC_23_INT                                  0x97
#define     V_008DFC_SQ_SRC_24_INT                                  0x98
#define     V_008DFC_SQ_SRC_25_INT                                  0x99
#define     V_008DFC_SQ_SRC_26_INT                                  0x9A
#define     V_008DFC_SQ_SRC_27_INT                                  0x9B
#define     V_008DFC_SQ_SRC_28_INT                                  0x9C
#define     V_008DFC_SQ_SRC_29_INT                                  0x9D
#define     V_008DFC_SQ_SRC_30_INT                                  0x9E
#define     V_008DFC_SQ_SRC_31_INT                                  0x9F
#define     V_008DFC_SQ_SRC_32_INT                                  0xA0
#define     V_008DFC_SQ_SRC_33_INT                                  0xA1
#define     V_008DFC_SQ_SRC_34_INT                                  0xA2
#define     V_008DFC_SQ_SRC_35_INT                                  0xA3
#define     V_008DFC_SQ_SRC_36_INT                                  0xA4
#define     V_008DFC_SQ_SRC_37_INT                                  0xA5
#define     V_008DFC_SQ_SRC_38_INT                                  0xA6
#define     V_008DFC_SQ_SRC_39_INT                                  0xA7
#define     V_008DFC_SQ_SRC_40_INT                                  0xA8
#define     V_008DFC_SQ_SRC_41_INT                                  0xA9
#define     V_008DFC_SQ_SRC_42_INT                                  0xAA
#define     V_008DFC_SQ_SRC_43_INT                                  0xAB
#define     V_008DFC_SQ_SRC_44_INT                                  0xAC
#define     V_008DFC_SQ_SRC_45_INT                                  0xAD
#define     V_008DFC_SQ_SRC_46_INT                                  0xAE
#define     V_008DFC_SQ_SRC_47_INT                                  0xAF
#define     V_008DFC_SQ_SRC_48_INT                                  0xB0
#define     V_008DFC_SQ_SRC_49_INT                                  0xB1
#define     V_008DFC_SQ_SRC_50_INT                                  0xB2
#define     V_008DFC_SQ_SRC_51_INT                                  0xB3
#define     V_008DFC_SQ_SRC_52_INT                                  0xB4
#define     V_008DFC_SQ_SRC_53_INT                                  0xB5
#define     V_008DFC_SQ_SRC_54_INT                                  0xB6
#define     V_008DFC_SQ_SRC_55_INT                                  0xB7
#define     V_008DFC_SQ_SRC_56_INT                                  0xB8
#define     V_008DFC_SQ_SRC_57_INT                                  0xB9
#define     V_008DFC_SQ_SRC_58_INT                                  0xBA
#define     V_008DFC_SQ_SRC_59_INT                                  0xBB
#define     V_008DFC_SQ_SRC_60_INT                                  0xBC
#define     V_008DFC_SQ_SRC_61_INT                                  0xBD
#define     V_008DFC_SQ_SRC_62_INT                                  0xBE
#define     V_008DFC_SQ_SRC_63_INT                                  0xBF
#define     V_008DFC_SQ_SRC_64_INT                                  0xC0
#define     V_008DFC_SQ_SRC_M_1_INT                                 0xC1
#define     V_008DFC_SQ_SRC_M_2_INT                                 0xC2
#define     V_008DFC_SQ_SRC_M_3_INT                                 0xC3
#define     V_008DFC_SQ_SRC_M_4_INT                                 0xC4
#define     V_008DFC_SQ_SRC_M_5_INT                                 0xC5
#define     V_008DFC_SQ_SRC_M_6_INT                                 0xC6
#define     V_008DFC_SQ_SRC_M_7_INT                                 0xC7
#define     V_008DFC_SQ_SRC_M_8_INT                                 0xC8
#define     V_008DFC_SQ_SRC_M_9_INT                                 0xC9
#define     V_008DFC_SQ_SRC_M_10_INT                                0xCA
#define     V_008DFC_SQ_SRC_M_11_INT                                0xCB
#define     V_008DFC_SQ_SRC_M_12_INT                                0xCC
#define     V_008DFC_SQ_SRC_M_13_INT                                0xCD
#define     V_008DFC_SQ_SRC_M_14_INT                                0xCE
#define     V_008DFC_SQ_SRC_M_15_INT                                0xCF
#define     V_008DFC_SQ_SRC_M_16_INT                                0xD0
#define     V_008DFC_SQ_SRC_0_5                                     0xF0
#define     V_008DFC_SQ_SRC_M_0_5                                   0xF1
#define     V_008DFC_SQ_SRC_1                                       0xF2
#define     V_008DFC_SQ_SRC_M_1                                     0xF3
#define     V_008DFC_SQ_SRC_2                                       0xF4
#define     V_008DFC_SQ_SRC_M_2                                     0xF5
#define     V_008DFC_SQ_SRC_4                                       0xF6
#define     V_008DFC_SQ_SRC_M_4                                     0xF7
#define     V_008DFC_SQ_SRC_VCCZ                                    0xFB
#define     V_008DFC_SQ_SRC_EXECZ                                   0xFC
#define     V_008DFC_SQ_SRC_SCC                                     0xFD
#define     V_008DFC_SQ_SRC_LDS_DIRECT                              0xFE
#define     V_008DFC_SQ_SRC_VGPR                                    0x100
#define   S_008DFC_VSRC1(x)                                           (((x) & 0xFF) << 9)
#define   G_008DFC_VSRC1(x)                                           (((x) >> 9) & 0xFF)
#define   C_008DFC_VSRC1                                              0xFFFE01FF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_VDST(x)                                            (((x) & 0xFF) << 17)
#define   G_008DFC_VDST(x)                                            (((x) >> 17) & 0xFF)
#define   C_008DFC_VDST                                               0xFE01FFFF
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_OP(x)                                              (((x) & 0x3F) << 25)
#define   G_008DFC_OP(x)                                              (((x) >> 25) & 0x3F)
#define   C_008DFC_OP                                                 0x81FFFFFF
#define     V_008DFC_SQ_V_CNDMASK_B32                               0x00
#define     V_008DFC_SQ_V_READLANE_B32                              0x01
#define     V_008DFC_SQ_V_WRITELANE_B32                             0x02
#define     V_008DFC_SQ_V_ADD_F32                                   0x03
#define     V_008DFC_SQ_V_SUB_F32                                   0x04
#define     V_008DFC_SQ_V_SUBREV_F32                                0x05
#define     V_008DFC_SQ_V_MAC_LEGACY_F32                            0x06
#define     V_008DFC_SQ_V_MUL_LEGACY_F32                            0x07
#define     V_008DFC_SQ_V_MUL_F32                                   0x08
#define     V_008DFC_SQ_V_MUL_I32_I24                               0x09
#define     V_008DFC_SQ_V_MUL_HI_I32_I24                            0x0A
#define     V_008DFC_SQ_V_MUL_U32_U24                               0x0B
#define     V_008DFC_SQ_V_MUL_HI_U32_U24                            0x0C
#define     V_008DFC_SQ_V_MIN_LEGACY_F32                            0x0D
#define     V_008DFC_SQ_V_MAX_LEGACY_F32                            0x0E
#define     V_008DFC_SQ_V_MIN_F32                                   0x0F
#define     V_008DFC_SQ_V_MAX_F32                                   0x10
#define     V_008DFC_SQ_V_MIN_I32                                   0x11
#define     V_008DFC_SQ_V_MAX_I32                                   0x12
#define     V_008DFC_SQ_V_MIN_U32                                   0x13
#define     V_008DFC_SQ_V_MAX_U32                                   0x14
#define     V_008DFC_SQ_V_LSHR_B32                                  0x15
#define     V_008DFC_SQ_V_LSHRREV_B32                               0x16
#define     V_008DFC_SQ_V_ASHR_I32                                  0x17
#define     V_008DFC_SQ_V_ASHRREV_I32                               0x18
#define     V_008DFC_SQ_V_LSHL_B32                                  0x19
#define     V_008DFC_SQ_V_LSHLREV_B32                               0x1A
#define     V_008DFC_SQ_V_AND_B32                                   0x1B
#define     V_008DFC_SQ_V_OR_B32                                    0x1C
#define     V_008DFC_SQ_V_XOR_B32                                   0x1D
#define     V_008DFC_SQ_V_BFM_B32                                   0x1E
#define     V_008DFC_SQ_V_MAC_F32                                   0x1F
#define     V_008DFC_SQ_V_MADMK_F32                                 0x20
#define     V_008DFC_SQ_V_MADAK_F32                                 0x21
#define     V_008DFC_SQ_V_BCNT_U32_B32                              0x22
#define     V_008DFC_SQ_V_MBCNT_LO_U32_B32                          0x23
#define     V_008DFC_SQ_V_MBCNT_HI_U32_B32                          0x24
#define     V_008DFC_SQ_V_ADD_I32                                   0x25
#define     V_008DFC_SQ_V_SUB_I32                                   0x26
#define     V_008DFC_SQ_V_SUBREV_I32                                0x27
#define     V_008DFC_SQ_V_ADDC_U32                                  0x28
#define     V_008DFC_SQ_V_SUBB_U32                                  0x29
#define     V_008DFC_SQ_V_SUBBREV_U32                               0x2A
#define     V_008DFC_SQ_V_LDEXP_F32                                 0x2B
#define     V_008DFC_SQ_V_CVT_PKACCUM_U8_F32                        0x2C
#define     V_008DFC_SQ_V_CVT_PKNORM_I16_F32                        0x2D
#define     V_008DFC_SQ_V_CVT_PKNORM_U16_F32                        0x2E
#define     V_008DFC_SQ_V_CVT_PKRTZ_F16_F32                         0x2F
#define     V_008DFC_SQ_V_CVT_PK_U16_U32                            0x30
#define     V_008DFC_SQ_V_CVT_PK_I16_I32                            0x31
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x1) << 31)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 31) & 0x1)
#define   C_008DFC_ENCODING                                           0x7FFFFFFF
#define R_008DFC_SQ_VOP3_0_SDST_ENC                                     0x008DFC
#define   S_008DFC_VDST(x)                                            (((x) & 0xFF) << 0)
#define   G_008DFC_VDST(x)                                            (((x) >> 0) & 0xFF)
#define   C_008DFC_VDST                                               0xFFFFFF00
#define     V_008DFC_SQ_VGPR                                        0x00
#define   S_008DFC_SDST(x)                                            (((x) & 0x7F) << 8)
#define   G_008DFC_SDST(x)                                            (((x) >> 8) & 0x7F)
#define   C_008DFC_SDST                                               0xFFFF80FF
#define     V_008DFC_SQ_SGPR                                        0x00
#define     V_008DFC_SQ_VCC_LO                                      0x6A
#define     V_008DFC_SQ_VCC_HI                                      0x6B
#define     V_008DFC_SQ_TBA_LO                                      0x6C
#define     V_008DFC_SQ_TBA_HI                                      0x6D
#define     V_008DFC_SQ_TMA_LO                                      0x6E
#define     V_008DFC_SQ_TMA_HI                                      0x6F
#define     V_008DFC_SQ_TTMP0                                       0x70
#define     V_008DFC_SQ_TTMP1                                       0x71
#define     V_008DFC_SQ_TTMP2                                       0x72
#define     V_008DFC_SQ_TTMP3                                       0x73
#define     V_008DFC_SQ_TTMP4                                       0x74
#define     V_008DFC_SQ_TTMP5                                       0x75
#define     V_008DFC_SQ_TTMP6                                       0x76
#define     V_008DFC_SQ_TTMP7                                       0x77
#define     V_008DFC_SQ_TTMP8                                       0x78
#define     V_008DFC_SQ_TTMP9                                       0x79
#define     V_008DFC_SQ_TTMP10                                      0x7A
#define     V_008DFC_SQ_TTMP11                                      0x7B
#define   S_008DFC_OP(x)                                              (((x) & 0x1FF) << 17)
#define   G_008DFC_OP(x)                                              (((x) >> 17) & 0x1FF)
#define   C_008DFC_OP                                                 0xFC01FFFF
#define     V_008DFC_SQ_V_OPC_OFFSET                                0x00
#define     V_008DFC_SQ_V_OP2_OFFSET                                0x100
#define     V_008DFC_SQ_V_MAD_LEGACY_F32                            0x140
#define     V_008DFC_SQ_V_MAD_F32                                   0x141
#define     V_008DFC_SQ_V_MAD_I32_I24                               0x142
#define     V_008DFC_SQ_V_MAD_U32_U24                               0x143
#define     V_008DFC_SQ_V_CUBEID_F32                                0x144
#define     V_008DFC_SQ_V_CUBESC_F32                                0x145
#define     V_008DFC_SQ_V_CUBETC_F32                                0x146
#define     V_008DFC_SQ_V_CUBEMA_F32                                0x147
#define     V_008DFC_SQ_V_BFE_U32                                   0x148
#define     V_008DFC_SQ_V_BFE_I32                                   0x149
#define     V_008DFC_SQ_V_BFI_B32                                   0x14A
#define     V_008DFC_SQ_V_FMA_F32                                   0x14B
#define     V_008DFC_SQ_V_FMA_F64                                   0x14C
#define     V_008DFC_SQ_V_LERP_U8                                   0x14D
#define     V_008DFC_SQ_V_ALIGNBIT_B32                              0x14E
#define     V_008DFC_SQ_V_ALIGNBYTE_B32                             0x14F
#define     V_008DFC_SQ_V_MULLIT_F32                                0x150
#define     V_008DFC_SQ_V_MIN3_F32                                  0x151
#define     V_008DFC_SQ_V_MIN3_I32                                  0x152
#define     V_008DFC_SQ_V_MIN3_U32                                  0x153
#define     V_008DFC_SQ_V_MAX3_F32                                  0x154
#define     V_008DFC_SQ_V_MAX3_I32                                  0x155
#define     V_008DFC_SQ_V_MAX3_U32                                  0x156
#define     V_008DFC_SQ_V_MED3_F32                                  0x157
#define     V_008DFC_SQ_V_MED3_I32                                  0x158
#define     V_008DFC_SQ_V_MED3_U32                                  0x159
#define     V_008DFC_SQ_V_SAD_U8                                    0x15A
#define     V_008DFC_SQ_V_SAD_HI_U8                                 0x15B
#define     V_008DFC_SQ_V_SAD_U16                                   0x15C
#define     V_008DFC_SQ_V_SAD_U32                                   0x15D
#define     V_008DFC_SQ_V_CVT_PK_U8_F32                             0x15E
#define     V_008DFC_SQ_V_DIV_FIXUP_F32                             0x15F
#define     V_008DFC_SQ_V_DIV_FIXUP_F64                             0x160
#define     V_008DFC_SQ_V_LSHL_B64                                  0x161
#define     V_008DFC_SQ_V_LSHR_B64                                  0x162
#define     V_008DFC_SQ_V_ASHR_I64                                  0x163
#define     V_008DFC_SQ_V_ADD_F64                                   0x164
#define     V_008DFC_SQ_V_MUL_F64                                   0x165
#define     V_008DFC_SQ_V_MIN_F64                                   0x166
#define     V_008DFC_SQ_V_MAX_F64                                   0x167
#define     V_008DFC_SQ_V_LDEXP_F64                                 0x168
#define     V_008DFC_SQ_V_MUL_LO_U32                                0x169
#define     V_008DFC_SQ_V_MUL_HI_U32                                0x16A
#define     V_008DFC_SQ_V_MUL_LO_I32                                0x16B
#define     V_008DFC_SQ_V_MUL_HI_I32                                0x16C
#define     V_008DFC_SQ_V_DIV_SCALE_F32                             0x16D
#define     V_008DFC_SQ_V_DIV_SCALE_F64                             0x16E
#define     V_008DFC_SQ_V_DIV_FMAS_F32                              0x16F
#define     V_008DFC_SQ_V_DIV_FMAS_F64                              0x170
#define     V_008DFC_SQ_V_MSAD_U8                                   0x171
#define     V_008DFC_SQ_V_QSAD_U8                                   0x172
#define     V_008DFC_SQ_V_MQSAD_U8                                  0x173
#define     V_008DFC_SQ_V_TRIG_PREOP_F64                            0x174
#define     V_008DFC_SQ_V_OP1_OFFSET                                0x180
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_VOP3_FIELD                              0x34
#define R_008DFC_SQ_MUBUF_0                                             0x008DFC
#define   S_008DFC_OFFSET(x)                                          (((x) & 0xFFF) << 0)
#define   G_008DFC_OFFSET(x)                                          (((x) >> 0) & 0xFFF)
#define   C_008DFC_OFFSET                                             0xFFFFF000
#define   S_008DFC_OFFEN(x)                                           (((x) & 0x1) << 12)
#define   G_008DFC_OFFEN(x)                                           (((x) >> 12) & 0x1)
#define   C_008DFC_OFFEN                                              0xFFFFEFFF
#define   S_008DFC_IDXEN(x)                                           (((x) & 0x1) << 13)
#define   G_008DFC_IDXEN(x)                                           (((x) >> 13) & 0x1)
#define   C_008DFC_IDXEN                                              0xFFFFDFFF
#define   S_008DFC_GLC(x)                                             (((x) & 0x1) << 14)
#define   G_008DFC_GLC(x)                                             (((x) >> 14) & 0x1)
#define   C_008DFC_GLC                                                0xFFFFBFFF
#define   S_008DFC_ADDR64(x)                                          (((x) & 0x1) << 15)
#define   G_008DFC_ADDR64(x)                                          (((x) >> 15) & 0x1)
#define   C_008DFC_ADDR64                                             0xFFFF7FFF
#define   S_008DFC_LDS(x)                                             (((x) & 0x1) << 16)
#define   G_008DFC_LDS(x)                                             (((x) >> 16) & 0x1)
#define   C_008DFC_LDS                                                0xFFFEFFFF
#define   S_008DFC_OP(x)                                              (((x) & 0x7F) << 18)
#define   G_008DFC_OP(x)                                              (((x) >> 18) & 0x7F)
#define   C_008DFC_OP                                                 0xFE03FFFF
#define     V_008DFC_SQ_BUFFER_LOAD_FORMAT_X                        0x00
#define     V_008DFC_SQ_BUFFER_LOAD_FORMAT_XY                       0x01
#define     V_008DFC_SQ_BUFFER_LOAD_FORMAT_XYZ                      0x02
#define     V_008DFC_SQ_BUFFER_LOAD_FORMAT_XYZW                     0x03
#define     V_008DFC_SQ_BUFFER_STORE_FORMAT_X                       0x04
#define     V_008DFC_SQ_BUFFER_STORE_FORMAT_XY                      0x05
#define     V_008DFC_SQ_BUFFER_STORE_FORMAT_XYZ                     0x06
#define     V_008DFC_SQ_BUFFER_STORE_FORMAT_XYZW                    0x07
#define     V_008DFC_SQ_BUFFER_LOAD_UBYTE                           0x08
#define     V_008DFC_SQ_BUFFER_LOAD_SBYTE                           0x09
#define     V_008DFC_SQ_BUFFER_LOAD_USHORT                          0x0A
#define     V_008DFC_SQ_BUFFER_LOAD_SSHORT                          0x0B
#define     V_008DFC_SQ_BUFFER_LOAD_DWORD                           0x0C
#define     V_008DFC_SQ_BUFFER_LOAD_DWORDX2                         0x0D
#define     V_008DFC_SQ_BUFFER_LOAD_DWORDX4                         0x0E
#define     V_008DFC_SQ_BUFFER_STORE_BYTE                           0x18
#define     V_008DFC_SQ_BUFFER_STORE_SHORT                          0x1A
#define     V_008DFC_SQ_BUFFER_STORE_DWORD                          0x1C
#define     V_008DFC_SQ_BUFFER_STORE_DWORDX2                        0x1D
#define     V_008DFC_SQ_BUFFER_STORE_DWORDX4                        0x1E
#define     V_008DFC_SQ_BUFFER_ATOMIC_SWAP                          0x30
#define     V_008DFC_SQ_BUFFER_ATOMIC_CMPSWAP                       0x31
#define     V_008DFC_SQ_BUFFER_ATOMIC_ADD                           0x32
#define     V_008DFC_SQ_BUFFER_ATOMIC_SUB                           0x33
#define     V_008DFC_SQ_BUFFER_ATOMIC_RSUB                          0x34
#define     V_008DFC_SQ_BUFFER_ATOMIC_SMIN                          0x35
#define     V_008DFC_SQ_BUFFER_ATOMIC_UMIN                          0x36
#define     V_008DFC_SQ_BUFFER_ATOMIC_SMAX                          0x37
#define     V_008DFC_SQ_BUFFER_ATOMIC_UMAX                          0x38
#define     V_008DFC_SQ_BUFFER_ATOMIC_AND                           0x39
#define     V_008DFC_SQ_BUFFER_ATOMIC_OR                            0x3A
#define     V_008DFC_SQ_BUFFER_ATOMIC_XOR                           0x3B
#define     V_008DFC_SQ_BUFFER_ATOMIC_INC                           0x3C
#define     V_008DFC_SQ_BUFFER_ATOMIC_DEC                           0x3D
#define     V_008DFC_SQ_BUFFER_ATOMIC_FCMPSWAP                      0x3E
#define     V_008DFC_SQ_BUFFER_ATOMIC_FMIN                          0x3F
#define     V_008DFC_SQ_BUFFER_ATOMIC_FMAX                          0x40
#define     V_008DFC_SQ_BUFFER_ATOMIC_SWAP_X2                       0x50
#define     V_008DFC_SQ_BUFFER_ATOMIC_CMPSWAP_X2                    0x51
#define     V_008DFC_SQ_BUFFER_ATOMIC_ADD_X2                        0x52
#define     V_008DFC_SQ_BUFFER_ATOMIC_SUB_X2                        0x53
#define     V_008DFC_SQ_BUFFER_ATOMIC_RSUB_X2                       0x54
#define     V_008DFC_SQ_BUFFER_ATOMIC_SMIN_X2                       0x55
#define     V_008DFC_SQ_BUFFER_ATOMIC_UMIN_X2                       0x56
#define     V_008DFC_SQ_BUFFER_ATOMIC_SMAX_X2                       0x57
#define     V_008DFC_SQ_BUFFER_ATOMIC_UMAX_X2                       0x58
#define     V_008DFC_SQ_BUFFER_ATOMIC_AND_X2                        0x59
#define     V_008DFC_SQ_BUFFER_ATOMIC_OR_X2                         0x5A
#define     V_008DFC_SQ_BUFFER_ATOMIC_XOR_X2                        0x5B
#define     V_008DFC_SQ_BUFFER_ATOMIC_INC_X2                        0x5C
#define     V_008DFC_SQ_BUFFER_ATOMIC_DEC_X2                        0x5D
#define     V_008DFC_SQ_BUFFER_ATOMIC_FCMPSWAP_X2                   0x5E
#define     V_008DFC_SQ_BUFFER_ATOMIC_FMIN_X2                       0x5F
#define     V_008DFC_SQ_BUFFER_ATOMIC_FMAX_X2                       0x60
#define     V_008DFC_SQ_BUFFER_WBINVL1_SC                           0x70
#define     V_008DFC_SQ_BUFFER_WBINVL1                              0x71
#define   S_008DFC_ENCODING(x)                                        (((x) & 0x3F) << 26)
#define   G_008DFC_ENCODING(x)                                        (((x) >> 26) & 0x3F)
#define   C_008DFC_ENCODING                                           0x03FFFFFF
#define     V_008DFC_SQ_ENC_MUBUF_FIELD                             0x38
#endif
#define R_008F00_SQ_BUF_RSRC_WORD0                                      0x008F00
#define R_008F04_SQ_BUF_RSRC_WORD1                                      0x008F04
#define   S_008F04_BASE_ADDRESS_HI(x)                                 (((x) & 0xFFFF) << 0)
#define   G_008F04_BASE_ADDRESS_HI(x)                                 (((x) >> 0) & 0xFFFF)
#define   C_008F04_BASE_ADDRESS_HI                                    0xFFFF0000
#define   S_008F04_STRIDE(x)                                          (((x) & 0x3FFF) << 16)
#define   G_008F04_STRIDE(x)                                          (((x) >> 16) & 0x3FFF)
#define   C_008F04_STRIDE                                             0xC000FFFF
#define   S_008F04_CACHE_SWIZZLE(x)                                   (((x) & 0x1) << 30)
#define   G_008F04_CACHE_SWIZZLE(x)                                   (((x) >> 30) & 0x1)
#define   C_008F04_CACHE_SWIZZLE                                      0xBFFFFFFF
#define   S_008F04_SWIZZLE_ENABLE(x)                                  (((x) & 0x1) << 31)
#define   G_008F04_SWIZZLE_ENABLE(x)                                  (((x) >> 31) & 0x1)
#define   C_008F04_SWIZZLE_ENABLE                                     0x7FFFFFFF
#define R_008F08_SQ_BUF_RSRC_WORD2                                      0x008F08
#define R_008F0C_SQ_BUF_RSRC_WORD3                                      0x008F0C
#define   S_008F0C_DST_SEL_X(x)                                       (((x) & 0x07) << 0)
#define   G_008F0C_DST_SEL_X(x)                                       (((x) >> 0) & 0x07)
#define   C_008F0C_DST_SEL_X                                          0xFFFFFFF8
#define     V_008F0C_SQ_SEL_0                                       0x00
#define     V_008F0C_SQ_SEL_1                                       0x01
#define     V_008F0C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F0C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F0C_SQ_SEL_X                                       0x04
#define     V_008F0C_SQ_SEL_Y                                       0x05
#define     V_008F0C_SQ_SEL_Z                                       0x06
#define     V_008F0C_SQ_SEL_W                                       0x07
#define   S_008F0C_DST_SEL_Y(x)                                       (((x) & 0x07) << 3)
#define   G_008F0C_DST_SEL_Y(x)                                       (((x) >> 3) & 0x07)
#define   C_008F0C_DST_SEL_Y                                          0xFFFFFFC7
#define     V_008F0C_SQ_SEL_0                                       0x00
#define     V_008F0C_SQ_SEL_1                                       0x01
#define     V_008F0C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F0C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F0C_SQ_SEL_X                                       0x04
#define     V_008F0C_SQ_SEL_Y                                       0x05
#define     V_008F0C_SQ_SEL_Z                                       0x06
#define     V_008F0C_SQ_SEL_W                                       0x07
#define   S_008F0C_DST_SEL_Z(x)                                       (((x) & 0x07) << 6)
#define   G_008F0C_DST_SEL_Z(x)                                       (((x) >> 6) & 0x07)
#define   C_008F0C_DST_SEL_Z                                          0xFFFFFE3F
#define     V_008F0C_SQ_SEL_0                                       0x00
#define     V_008F0C_SQ_SEL_1                                       0x01
#define     V_008F0C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F0C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F0C_SQ_SEL_X                                       0x04
#define     V_008F0C_SQ_SEL_Y                                       0x05
#define     V_008F0C_SQ_SEL_Z                                       0x06
#define     V_008F0C_SQ_SEL_W                                       0x07
#define   S_008F0C_DST_SEL_W(x)                                       (((x) & 0x07) << 9)
#define   G_008F0C_DST_SEL_W(x)                                       (((x) >> 9) & 0x07)
#define   C_008F0C_DST_SEL_W                                          0xFFFFF1FF
#define     V_008F0C_SQ_SEL_0                                       0x00
#define     V_008F0C_SQ_SEL_1                                       0x01
#define     V_008F0C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F0C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F0C_SQ_SEL_X                                       0x04
#define     V_008F0C_SQ_SEL_Y                                       0x05
#define     V_008F0C_SQ_SEL_Z                                       0x06
#define     V_008F0C_SQ_SEL_W                                       0x07
#define   S_008F0C_NUM_FORMAT(x)                                      (((x) & 0x07) << 12)
#define   G_008F0C_NUM_FORMAT(x)                                      (((x) >> 12) & 0x07)
#define   C_008F0C_NUM_FORMAT                                         0xFFFF8FFF
#define     V_008F0C_BUF_NUM_FORMAT_UNORM                           0x00
#define     V_008F0C_BUF_NUM_FORMAT_SNORM                           0x01
#define     V_008F0C_BUF_NUM_FORMAT_USCALED                         0x02
#define     V_008F0C_BUF_NUM_FORMAT_SSCALED                         0x03
#define     V_008F0C_BUF_NUM_FORMAT_UINT                            0x04
#define     V_008F0C_BUF_NUM_FORMAT_SINT                            0x05
#define     V_008F0C_BUF_NUM_FORMAT_SNORM_OGL                       0x06
#define     V_008F0C_BUF_NUM_FORMAT_FLOAT                           0x07
#define   S_008F0C_DATA_FORMAT(x)                                     (((x) & 0x0F) << 15)
#define   G_008F0C_DATA_FORMAT(x)                                     (((x) >> 15) & 0x0F)
#define   C_008F0C_DATA_FORMAT                                        0xFFF87FFF
#define     V_008F0C_BUF_DATA_FORMAT_INVALID                        0x00
#define     V_008F0C_BUF_DATA_FORMAT_8                              0x01
#define     V_008F0C_BUF_DATA_FORMAT_16                             0x02
#define     V_008F0C_BUF_DATA_FORMAT_8_8                            0x03
#define     V_008F0C_BUF_DATA_FORMAT_32                             0x04
#define     V_008F0C_BUF_DATA_FORMAT_16_16                          0x05
#define     V_008F0C_BUF_DATA_FORMAT_10_11_11                       0x06
#define     V_008F0C_BUF_DATA_FORMAT_11_11_10                       0x07
#define     V_008F0C_BUF_DATA_FORMAT_10_10_10_2                     0x08
#define     V_008F0C_BUF_DATA_FORMAT_2_10_10_10                     0x09
#define     V_008F0C_BUF_DATA_FORMAT_8_8_8_8                        0x0A
#define     V_008F0C_BUF_DATA_FORMAT_32_32                          0x0B
#define     V_008F0C_BUF_DATA_FORMAT_16_16_16_16                    0x0C
#define     V_008F0C_BUF_DATA_FORMAT_32_32_32                       0x0D
#define     V_008F0C_BUF_DATA_FORMAT_32_32_32_32                    0x0E
#define     V_008F0C_BUF_DATA_FORMAT_RESERVED_15                    0x0F
#define   S_008F0C_ELEMENT_SIZE(x)                                    (((x) & 0x03) << 19)
#define   G_008F0C_ELEMENT_SIZE(x)                                    (((x) >> 19) & 0x03)
#define   C_008F0C_ELEMENT_SIZE                                       0xFFE7FFFF
#define   S_008F0C_INDEX_STRIDE(x)                                    (((x) & 0x03) << 21)
#define   G_008F0C_INDEX_STRIDE(x)                                    (((x) >> 21) & 0x03)
#define   C_008F0C_INDEX_STRIDE                                       0xFF9FFFFF
#define   S_008F0C_ADD_TID_ENABLE(x)                                  (((x) & 0x1) << 23)
#define   G_008F0C_ADD_TID_ENABLE(x)                                  (((x) >> 23) & 0x1)
#define   C_008F0C_ADD_TID_ENABLE                                     0xFF7FFFFF
#define   S_008F0C_HASH_ENABLE(x)                                     (((x) & 0x1) << 25)
#define   G_008F0C_HASH_ENABLE(x)                                     (((x) >> 25) & 0x1)
#define   C_008F0C_HASH_ENABLE                                        0xFDFFFFFF
#define   S_008F0C_HEAP(x)                                            (((x) & 0x1) << 26)
#define   G_008F0C_HEAP(x)                                            (((x) >> 26) & 0x1)
#define   C_008F0C_HEAP                                               0xFBFFFFFF
#define   S_008F0C_TYPE(x)                                            (((x) & 0x03) << 30)
#define   G_008F0C_TYPE(x)                                            (((x) >> 30) & 0x03)
#define   C_008F0C_TYPE                                               0x3FFFFFFF
#define     V_008F0C_SQ_RSRC_BUF                                    0x00
#define     V_008F0C_SQ_RSRC_BUF_RSVD_1                             0x01
#define     V_008F0C_SQ_RSRC_BUF_RSVD_2                             0x02
#define     V_008F0C_SQ_RSRC_BUF_RSVD_3                             0x03
#define R_008F10_SQ_IMG_RSRC_WORD0                                      0x008F10
#define R_008F14_SQ_IMG_RSRC_WORD1                                      0x008F14
#define   S_008F14_BASE_ADDRESS_HI(x)                                 (((x) & 0xFF) << 0)
#define   G_008F14_BASE_ADDRESS_HI(x)                                 (((x) >> 0) & 0xFF)
#define   C_008F14_BASE_ADDRESS_HI                                    0xFFFFFF00
#define   S_008F14_MIN_LOD(x)                                         (((x) & 0xFFF) << 8)
#define   G_008F14_MIN_LOD(x)                                         (((x) >> 8) & 0xFFF)
#define   C_008F14_MIN_LOD                                            0xFFF000FF
#define   S_008F14_DATA_FORMAT(x)                                     (((x) & 0x3F) << 20)
#define   G_008F14_DATA_FORMAT(x)                                     (((x) >> 20) & 0x3F)
#define   C_008F14_DATA_FORMAT                                        0xFC0FFFFF
#define     V_008F14_IMG_DATA_FORMAT_INVALID                        0x00
#define     V_008F14_IMG_DATA_FORMAT_8                              0x01
#define     V_008F14_IMG_DATA_FORMAT_16                             0x02
#define     V_008F14_IMG_DATA_FORMAT_8_8                            0x03
#define     V_008F14_IMG_DATA_FORMAT_32                             0x04
#define     V_008F14_IMG_DATA_FORMAT_16_16                          0x05
#define     V_008F14_IMG_DATA_FORMAT_10_11_11                       0x06
#define     V_008F14_IMG_DATA_FORMAT_11_11_10                       0x07
#define     V_008F14_IMG_DATA_FORMAT_10_10_10_2                     0x08
#define     V_008F14_IMG_DATA_FORMAT_2_10_10_10                     0x09
#define     V_008F14_IMG_DATA_FORMAT_8_8_8_8                        0x0A
#define     V_008F14_IMG_DATA_FORMAT_32_32                          0x0B
#define     V_008F14_IMG_DATA_FORMAT_16_16_16_16                    0x0C
#define     V_008F14_IMG_DATA_FORMAT_32_32_32                       0x0D
#define     V_008F14_IMG_DATA_FORMAT_32_32_32_32                    0x0E
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_15                    0x0F
#define     V_008F14_IMG_DATA_FORMAT_5_6_5                          0x10
#define     V_008F14_IMG_DATA_FORMAT_1_5_5_5                        0x11
#define     V_008F14_IMG_DATA_FORMAT_5_5_5_1                        0x12
#define     V_008F14_IMG_DATA_FORMAT_4_4_4_4                        0x13
#define     V_008F14_IMG_DATA_FORMAT_8_24                           0x14
#define     V_008F14_IMG_DATA_FORMAT_24_8                           0x15
#define     V_008F14_IMG_DATA_FORMAT_X24_8_32                       0x16
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_23                    0x17
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_24                    0x18
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_25                    0x19
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_26                    0x1A
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_27                    0x1B
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_28                    0x1C
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_29                    0x1D
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_30                    0x1E
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_31                    0x1F
#define     V_008F14_IMG_DATA_FORMAT_GB_GR                          0x20
#define     V_008F14_IMG_DATA_FORMAT_BG_RG                          0x21
#define     V_008F14_IMG_DATA_FORMAT_5_9_9_9                        0x22
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_42                    0x2A
#define     V_008F14_IMG_DATA_FORMAT_RESERVED_43                    0x2B
#define     V_008F14_IMG_DATA_FORMAT_FMASK8_S2_F1                   0x2C
#define     V_008F14_IMG_DATA_FORMAT_FMASK8_S4_F1                   0x2D
#define     V_008F14_IMG_DATA_FORMAT_FMASK8_S8_F1                   0x2E
#define     V_008F14_IMG_DATA_FORMAT_FMASK8_S2_F2                   0x2F
#define     V_008F14_IMG_DATA_FORMAT_FMASK8_S4_F2                   0x30
#define     V_008F14_IMG_DATA_FORMAT_FMASK8_S4_F4                   0x31
#define     V_008F14_IMG_DATA_FORMAT_FMASK16_S16_F1                 0x32
#define     V_008F14_IMG_DATA_FORMAT_FMASK16_S8_F2                  0x33
#define     V_008F14_IMG_DATA_FORMAT_FMASK32_S16_F2                 0x34
#define     V_008F14_IMG_DATA_FORMAT_FMASK32_S8_F4                  0x35
#define     V_008F14_IMG_DATA_FORMAT_FMASK32_S8_F8                  0x36
#define     V_008F14_IMG_DATA_FORMAT_FMASK64_S16_F4                 0x37
#define     V_008F14_IMG_DATA_FORMAT_FMASK64_S16_F8                 0x38
#define     V_008F14_IMG_DATA_FORMAT_4_4                            0x39
#define     V_008F14_IMG_DATA_FORMAT_6_5_5                          0x3A
#define     V_008F14_IMG_DATA_FORMAT_1                              0x3B
#define     V_008F14_IMG_DATA_FORMAT_1_REVERSED                     0x3C
#define     V_008F14_IMG_DATA_FORMAT_32_AS_8                        0x3D
#define     V_008F14_IMG_DATA_FORMAT_32_AS_8_8                      0x3E
#define     V_008F14_IMG_DATA_FORMAT_32_AS_32_32_32_32              0x3F
#define   S_008F14_NUM_FORMAT(x)                                      (((x) & 0x0F) << 26)
#define   G_008F14_NUM_FORMAT(x)                                      (((x) >> 26) & 0x0F)
#define   C_008F14_NUM_FORMAT                                         0xC3FFFFFF
#define     V_008F14_IMG_NUM_FORMAT_UNORM                           0x00
#define     V_008F14_IMG_NUM_FORMAT_SNORM                           0x01
#define     V_008F14_IMG_NUM_FORMAT_USCALED                         0x02
#define     V_008F14_IMG_NUM_FORMAT_SSCALED                         0x03
#define     V_008F14_IMG_NUM_FORMAT_UINT                            0x04
#define     V_008F14_IMG_NUM_FORMAT_SINT                            0x05
#define     V_008F14_IMG_NUM_FORMAT_SNORM_OGL                       0x06
#define     V_008F14_IMG_NUM_FORMAT_FLOAT                           0x07
#define     V_008F14_IMG_NUM_FORMAT_RESERVED_8                      0x08
#define     V_008F14_IMG_NUM_FORMAT_SRGB                            0x09
#define     V_008F14_IMG_NUM_FORMAT_UBNORM                          0x0A
#define     V_008F14_IMG_NUM_FORMAT_UBNORM_OGL                      0x0B
#define     V_008F14_IMG_NUM_FORMAT_UBINT                           0x0C
#define     V_008F14_IMG_NUM_FORMAT_UBSCALED                        0x0D
#define     V_008F14_IMG_NUM_FORMAT_RESERVED_14                     0x0E
#define     V_008F14_IMG_NUM_FORMAT_RESERVED_15                     0x0F
#define R_008F18_SQ_IMG_RSRC_WORD2                                      0x008F18
#define   S_008F18_WIDTH(x)                                           (((x) & 0x3FFF) << 0)
#define   G_008F18_WIDTH(x)                                           (((x) >> 0) & 0x3FFF)
#define   C_008F18_WIDTH                                              0xFFFFC000
#define   S_008F18_HEIGHT(x)                                          (((x) & 0x3FFF) << 14)
#define   G_008F18_HEIGHT(x)                                          (((x) >> 14) & 0x3FFF)
#define   C_008F18_HEIGHT                                             0xF0003FFF
#define   S_008F18_PERF_MOD(x)                                        (((x) & 0x07) << 28)
#define   G_008F18_PERF_MOD(x)                                        (((x) >> 28) & 0x07)
#define   C_008F18_PERF_MOD                                           0x8FFFFFFF
#define   S_008F18_INTERLACED(x)                                      (((x) & 0x1) << 31)
#define   G_008F18_INTERLACED(x)                                      (((x) >> 31) & 0x1)
#define   C_008F18_INTERLACED                                         0x7FFFFFFF
#define R_008F1C_SQ_IMG_RSRC_WORD3                                      0x008F1C
#define   S_008F1C_DST_SEL_X(x)                                       (((x) & 0x07) << 0)
#define   G_008F1C_DST_SEL_X(x)                                       (((x) >> 0) & 0x07)
#define   C_008F1C_DST_SEL_X                                          0xFFFFFFF8
#define     V_008F1C_SQ_SEL_0                                       0x00
#define     V_008F1C_SQ_SEL_1                                       0x01
#define     V_008F1C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F1C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F1C_SQ_SEL_X                                       0x04
#define     V_008F1C_SQ_SEL_Y                                       0x05
#define     V_008F1C_SQ_SEL_Z                                       0x06
#define     V_008F1C_SQ_SEL_W                                       0x07
#define   S_008F1C_DST_SEL_Y(x)                                       (((x) & 0x07) << 3)
#define   G_008F1C_DST_SEL_Y(x)                                       (((x) >> 3) & 0x07)
#define   C_008F1C_DST_SEL_Y                                          0xFFFFFFC7
#define     V_008F1C_SQ_SEL_0                                       0x00
#define     V_008F1C_SQ_SEL_1                                       0x01
#define     V_008F1C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F1C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F1C_SQ_SEL_X                                       0x04
#define     V_008F1C_SQ_SEL_Y                                       0x05
#define     V_008F1C_SQ_SEL_Z                                       0x06
#define     V_008F1C_SQ_SEL_W                                       0x07
#define   S_008F1C_DST_SEL_Z(x)                                       (((x) & 0x07) << 6)
#define   G_008F1C_DST_SEL_Z(x)                                       (((x) >> 6) & 0x07)
#define   C_008F1C_DST_SEL_Z                                          0xFFFFFE3F
#define     V_008F1C_SQ_SEL_0                                       0x00
#define     V_008F1C_SQ_SEL_1                                       0x01
#define     V_008F1C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F1C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F1C_SQ_SEL_X                                       0x04
#define     V_008F1C_SQ_SEL_Y                                       0x05
#define     V_008F1C_SQ_SEL_Z                                       0x06
#define     V_008F1C_SQ_SEL_W                                       0x07
#define   S_008F1C_DST_SEL_W(x)                                       (((x) & 0x07) << 9)
#define   G_008F1C_DST_SEL_W(x)                                       (((x) >> 9) & 0x07)
#define   C_008F1C_DST_SEL_W                                          0xFFFFF1FF
#define     V_008F1C_SQ_SEL_0                                       0x00
#define     V_008F1C_SQ_SEL_1                                       0x01
#define     V_008F1C_SQ_SEL_RESERVED_0                              0x02
#define     V_008F1C_SQ_SEL_RESERVED_1                              0x03
#define     V_008F1C_SQ_SEL_X                                       0x04
#define     V_008F1C_SQ_SEL_Y                                       0x05
#define     V_008F1C_SQ_SEL_Z                                       0x06
#define     V_008F1C_SQ_SEL_W                                       0x07
#define   S_008F1C_BASE_LEVEL(x)                                      (((x) & 0x0F) << 12)
#define   G_008F1C_BASE_LEVEL(x)                                      (((x) >> 12) & 0x0F)
#define   C_008F1C_BASE_LEVEL                                         0xFFFF0FFF
#define   S_008F1C_LAST_LEVEL(x)                                      (((x) & 0x0F) << 16)
#define   G_008F1C_LAST_LEVEL(x)                                      (((x) >> 16) & 0x0F)
#define   C_008F1C_LAST_LEVEL                                         0xFFF0FFFF
#define   S_008F1C_TILING_INDEX(x)                                    (((x) & 0x1F) << 20)
#define   G_008F1C_TILING_INDEX(x)                                    (((x) >> 20) & 0x1F)
#define   C_008F1C_TILING_INDEX                                       0xFE0FFFFF
#define   S_008F1C_POW2_PAD(x)                                        (((x) & 0x1) << 25)
#define   G_008F1C_POW2_PAD(x)                                        (((x) >> 25) & 0x1)
#define   C_008F1C_POW2_PAD                                           0xFDFFFFFF
#define   S_008F1C_TYPE(x)                                            (((x) & 0x0F) << 28)
#define   G_008F1C_TYPE(x)                                            (((x) >> 28) & 0x0F)
#define   C_008F1C_TYPE                                               0x0FFFFFFF
#define     V_008F1C_SQ_RSRC_IMG_RSVD_0                             0x00
#define     V_008F1C_SQ_RSRC_IMG_RSVD_1                             0x01
#define     V_008F1C_SQ_RSRC_IMG_RSVD_2                             0x02
#define     V_008F1C_SQ_RSRC_IMG_RSVD_3                             0x03
#define     V_008F1C_SQ_RSRC_IMG_RSVD_4                             0x04
#define     V_008F1C_SQ_RSRC_IMG_RSVD_5                             0x05
#define     V_008F1C_SQ_RSRC_IMG_RSVD_6                             0x06
#define     V_008F1C_SQ_RSRC_IMG_RSVD_7                             0x07
#define     V_008F1C_SQ_RSRC_IMG_1D                                 0x08
#define     V_008F1C_SQ_RSRC_IMG_2D                                 0x09
#define     V_008F1C_SQ_RSRC_IMG_3D                                 0x0A
#define     V_008F1C_SQ_RSRC_IMG_CUBE                               0x0B
#define     V_008F1C_SQ_RSRC_IMG_1D_ARRAY                           0x0C
#define     V_008F1C_SQ_RSRC_IMG_2D_ARRAY                           0x0D
#define     V_008F1C_SQ_RSRC_IMG_2D_MSAA                            0x0E
#define     V_008F1C_SQ_RSRC_IMG_2D_MSAA_ARRAY                      0x0F
#define R_008F20_SQ_IMG_RSRC_WORD4                                      0x008F20
#define   S_008F20_DEPTH(x)                                           (((x) & 0x1FFF) << 0)
#define   G_008F20_DEPTH(x)                                           (((x) >> 0) & 0x1FFF)
#define   C_008F20_DEPTH                                              0xFFFFE000
#define   S_008F20_PITCH(x)                                           (((x) & 0x3FFF) << 13)
#define   G_008F20_PITCH(x)                                           (((x) >> 13) & 0x3FFF)
#define   C_008F20_PITCH                                              0xF8001FFF
#define R_008F24_SQ_IMG_RSRC_WORD5                                      0x008F24
#define   S_008F24_BASE_ARRAY(x)                                      (((x) & 0x1FFF) << 0)
#define   G_008F24_BASE_ARRAY(x)                                      (((x) >> 0) & 0x1FFF)
#define   C_008F24_BASE_ARRAY                                         0xFFFFE000
#define   S_008F24_LAST_ARRAY(x)                                      (((x) & 0x1FFF) << 13)
#define   G_008F24_LAST_ARRAY(x)                                      (((x) >> 13) & 0x1FFF)
#define   C_008F24_LAST_ARRAY                                         0xFC001FFF
#define R_008F28_SQ_IMG_RSRC_WORD6                                      0x008F28
#define   S_008F28_MIN_LOD_WARN(x)                                    (((x) & 0xFFF) << 0)
#define   G_008F28_MIN_LOD_WARN(x)                                    (((x) >> 0) & 0xFFF)
#define   C_008F28_MIN_LOD_WARN                                       0xFFFFF000
#define R_008F2C_SQ_IMG_RSRC_WORD7                                      0x008F2C
#define R_008F30_SQ_IMG_SAMP_WORD0                                      0x008F30
#define   S_008F30_CLAMP_X(x)                                         (((x) & 0x07) << 0)
#define   G_008F30_CLAMP_X(x)                                         (((x) >> 0) & 0x07)
#define   C_008F30_CLAMP_X                                            0xFFFFFFF8
#define     V_008F30_SQ_TEX_WRAP                                    0x00
#define     V_008F30_SQ_TEX_MIRROR                                  0x01
#define     V_008F30_SQ_TEX_CLAMP_LAST_TEXEL                        0x02
#define     V_008F30_SQ_TEX_MIRROR_ONCE_LAST_TEXEL                  0x03
#define     V_008F30_SQ_TEX_CLAMP_HALF_BORDER                       0x04
#define     V_008F30_SQ_TEX_MIRROR_ONCE_HALF_BORDER                 0x05
#define     V_008F30_SQ_TEX_CLAMP_BORDER                            0x06
#define     V_008F30_SQ_TEX_MIRROR_ONCE_BORDER                      0x07
#define   S_008F30_CLAMP_Y(x)                                         (((x) & 0x07) << 3)
#define   G_008F30_CLAMP_Y(x)                                         (((x) >> 3) & 0x07)
#define   C_008F30_CLAMP_Y                                            0xFFFFFFC7
#define     V_008F30_SQ_TEX_WRAP                                    0x00
#define     V_008F30_SQ_TEX_MIRROR                                  0x01
#define     V_008F30_SQ_TEX_CLAMP_LAST_TEXEL                        0x02
#define     V_008F30_SQ_TEX_MIRROR_ONCE_LAST_TEXEL                  0x03
#define     V_008F30_SQ_TEX_CLAMP_HALF_BORDER                       0x04
#define     V_008F30_SQ_TEX_MIRROR_ONCE_HALF_BORDER                 0x05
#define     V_008F30_SQ_TEX_CLAMP_BORDER                            0x06
#define     V_008F30_SQ_TEX_MIRROR_ONCE_BORDER                      0x07
#define   S_008F30_CLAMP_Z(x)                                         (((x) & 0x07) << 6)
#define   G_008F30_CLAMP_Z(x)                                         (((x) >> 6) & 0x07)
#define   C_008F30_CLAMP_Z                                            0xFFFFFE3F
#define     V_008F30_SQ_TEX_WRAP                                    0x00
#define     V_008F30_SQ_TEX_MIRROR                                  0x01
#define     V_008F30_SQ_TEX_CLAMP_LAST_TEXEL                        0x02
#define     V_008F30_SQ_TEX_MIRROR_ONCE_LAST_TEXEL                  0x03
#define     V_008F30_SQ_TEX_CLAMP_HALF_BORDER                       0x04
#define     V_008F30_SQ_TEX_MIRROR_ONCE_HALF_BORDER                 0x05
#define     V_008F30_SQ_TEX_CLAMP_BORDER                            0x06
#define     V_008F30_SQ_TEX_MIRROR_ONCE_BORDER                      0x07
#define   S_008F30_DEPTH_COMPARE_FUNC(x)                              (((x) & 0x07) << 12)
#define   G_008F30_DEPTH_COMPARE_FUNC(x)                              (((x) >> 12) & 0x07)
#define   C_008F30_DEPTH_COMPARE_FUNC                                 0xFFFF8FFF
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_NEVER                     0x00
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_LESS                      0x01
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_EQUAL                     0x02
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_LESSEQUAL                 0x03
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_GREATER                   0x04
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_NOTEQUAL                  0x05
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_GREATEREQUAL              0x06
#define     V_008F30_SQ_TEX_DEPTH_COMPARE_ALWAYS                    0x07
#define   S_008F30_FORCE_UNNORMALIZED(x)                              (((x) & 0x1) << 15)
#define   G_008F30_FORCE_UNNORMALIZED(x)                              (((x) >> 15) & 0x1)
#define   C_008F30_FORCE_UNNORMALIZED                                 0xFFFF7FFF
#define   S_008F30_MC_COORD_TRUNC(x)                                  (((x) & 0x1) << 19)
#define   G_008F30_MC_COORD_TRUNC(x)                                  (((x) >> 19) & 0x1)
#define   C_008F30_MC_COORD_TRUNC                                     0xFFF7FFFF
#define   S_008F30_FORCE_DEGAMMA(x)                                   (((x) & 0x1) << 20)
#define   G_008F30_FORCE_DEGAMMA(x)                                   (((x) >> 20) & 0x1)
#define   C_008F30_FORCE_DEGAMMA                                      0xFFEFFFFF
#define   S_008F30_TRUNC_COORD(x)                                     (((x) & 0x1) << 27)
#define   G_008F30_TRUNC_COORD(x)                                     (((x) >> 27) & 0x1)
#define   C_008F30_TRUNC_COORD                                        0xF7FFFFFF
#define   S_008F30_DISABLE_CUBE_WRAP(x)                               (((x) & 0x1) << 28)
#define   G_008F30_DISABLE_CUBE_WRAP(x)                               (((x) >> 28) & 0x1)
#define   C_008F30_DISABLE_CUBE_WRAP                                  0xEFFFFFFF
#define   S_008F30_FILTER_MODE(x)                                     (((x) & 0x03) << 29)
#define   G_008F30_FILTER_MODE(x)                                     (((x) >> 29) & 0x03)
#define   C_008F30_FILTER_MODE                                        0x9FFFFFFF
#define R_008F34_SQ_IMG_SAMP_WORD1                                      0x008F34
#define   S_008F34_MIN_LOD(x)                                         (((x) & 0xFFF) << 0)
#define   G_008F34_MIN_LOD(x)                                         (((x) >> 0) & 0xFFF)
#define   C_008F34_MIN_LOD                                            0xFFFFF000
#define   S_008F34_MAX_LOD(x)                                         (((x) & 0xFFF) << 12)
#define   G_008F34_MAX_LOD(x)                                         (((x) >> 12) & 0xFFF)
#define   C_008F34_MAX_LOD                                            0xFF000FFF
#define   S_008F34_PERF_MIP(x)                                        (((x) & 0x0F) << 24)
#define   G_008F34_PERF_MIP(x)                                        (((x) >> 24) & 0x0F)
#define   C_008F34_PERF_MIP                                           0xF0FFFFFF
#define   S_008F34_PERF_Z(x)                                          (((x) & 0x0F) << 28)
#define   G_008F34_PERF_Z(x)                                          (((x) >> 28) & 0x0F)
#define   C_008F34_PERF_Z                                             0x0FFFFFFF
#define R_008F38_SQ_IMG_SAMP_WORD2                                      0x008F38
#define   S_008F38_LOD_BIAS(x)                                        (((x) & 0x3FFF) << 0)
#define   G_008F38_LOD_BIAS(x)                                        (((x) >> 0) & 0x3FFF)
#define   C_008F38_LOD_BIAS                                           0xFFFFC000
#define   S_008F38_LOD_BIAS_SEC(x)                                    (((x) & 0x3F) << 14)
#define   G_008F38_LOD_BIAS_SEC(x)                                    (((x) >> 14) & 0x3F)
#define   C_008F38_LOD_BIAS_SEC                                       0xFFF03FFF
#define   S_008F38_XY_MAG_FILTER(x)                                   (((x) & 0x03) << 20)
#define   G_008F38_XY_MAG_FILTER(x)                                   (((x) >> 20) & 0x03)
#define   C_008F38_XY_MAG_FILTER                                      0xFFCFFFFF
#define     V_008F38_SQ_TEX_XY_FILTER_POINT                         0x00
#define     V_008F38_SQ_TEX_XY_FILTER_BILINEAR                      0x01
#define   S_008F38_XY_MIN_FILTER(x)                                   (((x) & 0x03) << 22)
#define   G_008F38_XY_MIN_FILTER(x)                                   (((x) >> 22) & 0x03)
#define   C_008F38_XY_MIN_FILTER                                      0xFF3FFFFF
#define     V_008F38_SQ_TEX_XY_FILTER_POINT                         0x00
#define     V_008F38_SQ_TEX_XY_FILTER_BILINEAR                      0x01
#define   S_008F38_Z_FILTER(x)                                        (((x) & 0x03) << 24)
#define   G_008F38_Z_FILTER(x)                                        (((x) >> 24) & 0x03)
#define   C_008F38_Z_FILTER                                           0xFCFFFFFF
#define     V_008F38_SQ_TEX_Z_FILTER_NONE                           0x00
#define     V_008F38_SQ_TEX_Z_FILTER_POINT                          0x01
#define     V_008F38_SQ_TEX_Z_FILTER_LINEAR                         0x02
#define   S_008F38_MIP_FILTER(x)                                      (((x) & 0x03) << 26)
#define   G_008F38_MIP_FILTER(x)                                      (((x) >> 26) & 0x03)
#define   C_008F38_MIP_FILTER                                         0xF3FFFFFF
#define     V_008F38_SQ_TEX_Z_FILTER_NONE                           0x00
#define     V_008F38_SQ_TEX_Z_FILTER_POINT                          0x01
#define     V_008F38_SQ_TEX_Z_FILTER_LINEAR                         0x02
#define   S_008F38_MIP_POINT_PRECLAMP(x)                              (((x) & 0x1) << 28)
#define   G_008F38_MIP_POINT_PRECLAMP(x)                              (((x) >> 28) & 0x1)
#define   C_008F38_MIP_POINT_PRECLAMP                                 0xEFFFFFFF
#define   S_008F38_DISABLE_LSB_CEIL(x)                                (((x) & 0x1) << 29)
#define   G_008F38_DISABLE_LSB_CEIL(x)                                (((x) >> 29) & 0x1)
#define   C_008F38_DISABLE_LSB_CEIL                                   0xDFFFFFFF
#define   S_008F38_FILTER_PREC_FIX(x)                                 (((x) & 0x1) << 30)
#define   G_008F38_FILTER_PREC_FIX(x)                                 (((x) >> 30) & 0x1)
#define   C_008F38_FILTER_PREC_FIX                                    0xBFFFFFFF
#define R_008F3C_SQ_IMG_SAMP_WORD3                                      0x008F3C
#define   S_008F3C_BORDER_COLOR_PTR(x)                                (((x) & 0xFFF) << 0)
#define   G_008F3C_BORDER_COLOR_PTR(x)                                (((x) >> 0) & 0xFFF)
#define   C_008F3C_BORDER_COLOR_PTR                                   0xFFFFF000
#define   S_008F3C_BORDER_COLOR_TYPE(x)                               (((x) & 0x03) << 30)
#define   G_008F3C_BORDER_COLOR_TYPE(x)                               (((x) >> 30) & 0x03)
#define   C_008F3C_BORDER_COLOR_TYPE                                  0x3FFFFFFF
#define     V_008F3C_SQ_TEX_BORDER_COLOR_TRANS_BLACK                0x00
#define     V_008F3C_SQ_TEX_BORDER_COLOR_OPAQUE_BLACK               0x01
#define     V_008F3C_SQ_TEX_BORDER_COLOR_OPAQUE_WHITE               0x02
#define     V_008F3C_SQ_TEX_BORDER_COLOR_REGISTER                   0x03
#define R_0090DC_SPI_DYN_GPR_LOCK_EN                                    0x0090DC
#define   S_0090DC_VS_LOW_THRESHOLD(x)                                (((x) & 0x0F) << 0)
#define   G_0090DC_VS_LOW_THRESHOLD(x)                                (((x) >> 0) & 0x0F)
#define   C_0090DC_VS_LOW_THRESHOLD                                   0xFFFFFFF0
#define   S_0090DC_GS_LOW_THRESHOLD(x)                                (((x) & 0x0F) << 4)
#define   G_0090DC_GS_LOW_THRESHOLD(x)                                (((x) >> 4) & 0x0F)
#define   C_0090DC_GS_LOW_THRESHOLD                                   0xFFFFFF0F
#define   S_0090DC_ES_LOW_THRESHOLD(x)                                (((x) & 0x0F) << 8)
#define   G_0090DC_ES_LOW_THRESHOLD(x)                                (((x) >> 8) & 0x0F)
#define   C_0090DC_ES_LOW_THRESHOLD                                   0xFFFFF0FF
#define   S_0090DC_HS_LOW_THRESHOLD(x)                                (((x) & 0x0F) << 12)
#define   G_0090DC_HS_LOW_THRESHOLD(x)                                (((x) >> 12) & 0x0F)
#define   C_0090DC_HS_LOW_THRESHOLD                                   0xFFFF0FFF
#define   S_0090DC_LS_LOW_THRESHOLD(x)                                (((x) & 0x0F) << 16)
#define   G_0090DC_LS_LOW_THRESHOLD(x)                                (((x) >> 16) & 0x0F)
#define   C_0090DC_LS_LOW_THRESHOLD                                   0xFFF0FFFF
#define R_0090E0_SPI_STATIC_THREAD_MGMT_1                               0x0090E0
#define   S_0090E0_PS_CU_EN(x)                                        (((x) & 0xFFFF) << 0)
#define   G_0090E0_PS_CU_EN(x)                                        (((x) >> 0) & 0xFFFF)
#define   C_0090E0_PS_CU_EN                                           0xFFFF0000
#define   S_0090E0_VS_CU_EN(x)                                        (((x) & 0xFFFF) << 16)
#define   G_0090E0_VS_CU_EN(x)                                        (((x) >> 16) & 0xFFFF)
#define   C_0090E0_VS_CU_EN                                           0x0000FFFF
#define R_0090E4_SPI_STATIC_THREAD_MGMT_2                               0x0090E4
#define   S_0090E4_GS_CU_EN(x)                                        (((x) & 0xFFFF) << 0)
#define   G_0090E4_GS_CU_EN(x)                                        (((x) >> 0) & 0xFFFF)
#define   C_0090E4_GS_CU_EN                                           0xFFFF0000
#define   S_0090E4_ES_CU_EN(x)                                        (((x) & 0xFFFF) << 16)
#define   G_0090E4_ES_CU_EN(x)                                        (((x) >> 16) & 0xFFFF)
#define   C_0090E4_ES_CU_EN                                           0x0000FFFF
#define R_0090E8_SPI_STATIC_THREAD_MGMT_3                               0x0090E8
#define   S_0090E8_LSHS_CU_EN(x)                                      (((x) & 0xFFFF) << 0)
#define   G_0090E8_LSHS_CU_EN(x)                                      (((x) >> 0) & 0xFFFF)
#define   C_0090E8_LSHS_CU_EN                                         0xFFFF0000
#define R_0090EC_SPI_PS_MAX_WAVE_ID                                     0x0090EC
#define   S_0090EC_MAX_WAVE_ID(x)                                     (((x) & 0xFFF) << 0)
#define   G_0090EC_MAX_WAVE_ID(x)                                     (((x) >> 0) & 0xFFF)
#define   C_0090EC_MAX_WAVE_ID                                        0xFFFFF000
#define R_0090F0_SPI_ARB_PRIORITY                                       0x0090F0
#define   S_0090F0_RING_ORDER_TS0(x)                                  (((x) & 0x07) << 0)
#define   G_0090F0_RING_ORDER_TS0(x)                                  (((x) >> 0) & 0x07)
#define   C_0090F0_RING_ORDER_TS0                                     0xFFFFFFF8
#define     V_0090F0_X_R0                                           0x00
#define   S_0090F0_RING_ORDER_TS1(x)                                  (((x) & 0x07) << 3)
#define   G_0090F0_RING_ORDER_TS1(x)                                  (((x) >> 3) & 0x07)
#define   C_0090F0_RING_ORDER_TS1                                     0xFFFFFFC7
#define   S_0090F0_RING_ORDER_TS2(x)                                  (((x) & 0x07) << 6)
#define   G_0090F0_RING_ORDER_TS2(x)                                  (((x) >> 6) & 0x07)
#define   C_0090F0_RING_ORDER_TS2                                     0xFFFFFE3F
#define R_0090F4_SPI_ARB_CYCLES_0                                       0x0090F4
#define   S_0090F4_TS0_DURATION(x)                                    (((x) & 0xFFFF) << 0)
#define   G_0090F4_TS0_DURATION(x)                                    (((x) >> 0) & 0xFFFF)
#define   C_0090F4_TS0_DURATION                                       0xFFFF0000
#define   S_0090F4_TS1_DURATION(x)                                    (((x) & 0xFFFF) << 16)
#define   G_0090F4_TS1_DURATION(x)                                    (((x) >> 16) & 0xFFFF)
#define   C_0090F4_TS1_DURATION                                       0x0000FFFF
#define R_0090F8_SPI_ARB_CYCLES_1                                       0x0090F8
#define   S_0090F8_TS2_DURATION(x)                                    (((x) & 0xFFFF) << 0)
#define   G_0090F8_TS2_DURATION(x)                                    (((x) >> 0) & 0xFFFF)
#define   C_0090F8_TS2_DURATION                                       0xFFFF0000
#define R_009100_SPI_CONFIG_CNTL                                        0x009100
#define   S_009100_GPR_WRITE_PRIORITY(x)                              (((x) & 0x1FFFFF) << 0)
#define   G_009100_GPR_WRITE_PRIORITY(x)                              (((x) >> 0) & 0x1FFFFF)
#define   C_009100_GPR_WRITE_PRIORITY                                 0xFFE00000
#define   S_009100_EXP_PRIORITY_ORDER(x)                              (((x) & 0x07) << 21)
#define   G_009100_EXP_PRIORITY_ORDER(x)                              (((x) >> 21) & 0x07)
#define   C_009100_EXP_PRIORITY_ORDER                                 0xFF1FFFFF
#define   S_009100_ENABLE_SQG_TOP_EVENTS(x)                           (((x) & 0x1) << 24)
#define   G_009100_ENABLE_SQG_TOP_EVENTS(x)                           (((x) >> 24) & 0x1)
#define   C_009100_ENABLE_SQG_TOP_EVENTS                              0xFEFFFFFF
#define   S_009100_ENABLE_SQG_BOP_EVENTS(x)                           (((x) & 0x1) << 25)
#define   G_009100_ENABLE_SQG_BOP_EVENTS(x)                           (((x) >> 25) & 0x1)
#define   C_009100_ENABLE_SQG_BOP_EVENTS                              0xFDFFFFFF
#define   S_009100_RSRC_MGMT_RESET(x)                                 (((x) & 0x1) << 26)
#define   G_009100_RSRC_MGMT_RESET(x)                                 (((x) >> 26) & 0x1)
#define   C_009100_RSRC_MGMT_RESET                                    0xFBFFFFFF
#define R_00913C_SPI_CONFIG_CNTL_1                                      0x00913C
#define   S_00913C_VTX_DONE_DELAY(x)                                  (((x) & 0x0F) << 0)
#define   G_00913C_VTX_DONE_DELAY(x)                                  (((x) >> 0) & 0x0F)
#define   C_00913C_VTX_DONE_DELAY                                     0xFFFFFFF0
#define     V_00913C_X_DELAY_14_CLKS                                0x00
#define     V_00913C_X_DELAY_16_CLKS                                0x01
#define     V_00913C_X_DELAY_18_CLKS                                0x02
#define     V_00913C_X_DELAY_20_CLKS                                0x03
#define     V_00913C_X_DELAY_22_CLKS                                0x04
#define     V_00913C_X_DELAY_24_CLKS                                0x05
#define     V_00913C_X_DELAY_26_CLKS                                0x06
#define     V_00913C_X_DELAY_28_CLKS                                0x07
#define     V_00913C_X_DELAY_30_CLKS                                0x08
#define     V_00913C_X_DELAY_32_CLKS                                0x09
#define     V_00913C_X_DELAY_34_CLKS                                0x0A
#define     V_00913C_X_DELAY_4_CLKS                                 0x0B
#define     V_00913C_X_DELAY_6_CLKS                                 0x0C
#define     V_00913C_X_DELAY_8_CLKS                                 0x0D
#define     V_00913C_X_DELAY_10_CLKS                                0x0E
#define     V_00913C_X_DELAY_12_CLKS                                0x0F
#define   S_00913C_INTERP_ONE_PRIM_PER_ROW(x)                         (((x) & 0x1) << 4)
#define   G_00913C_INTERP_ONE_PRIM_PER_ROW(x)                         (((x) >> 4) & 0x1)
#define   C_00913C_INTERP_ONE_PRIM_PER_ROW                            0xFFFFFFEF
#define   S_00913C_PC_LIMIT_ENABLE(x)                                 (((x) & 0x1) << 6)
#define   G_00913C_PC_LIMIT_ENABLE(x)                                 (((x) >> 6) & 0x1)
#define   C_00913C_PC_LIMIT_ENABLE                                    0xFFFFFFBF
#define   S_00913C_PC_LIMIT_STRICT(x)                                 (((x) & 0x1) << 7)
#define   G_00913C_PC_LIMIT_STRICT(x)                                 (((x) >> 7) & 0x1)
#define   C_00913C_PC_LIMIT_STRICT                                    0xFFFFFF7F
#define   S_00913C_PC_LIMIT_SIZE(x)                                   (((x) & 0xFFFF) << 16)
#define   G_00913C_PC_LIMIT_SIZE(x)                                   (((x) >> 16) & 0xFFFF)
#define   C_00913C_PC_LIMIT_SIZE                                      0x0000FFFF
#define R_00936C_SPI_RESOURCE_RESERVE_CU_AB_0                           0x00936C
#define   S_00936C_TYPE_A(x)                                          (((x) & 0x0F) << 0)
#define   G_00936C_TYPE_A(x)                                          (((x) >> 0) & 0x0F)
#define   C_00936C_TYPE_A                                             0xFFFFFFF0
#define   S_00936C_VGPR_A(x)                                          (((x) & 0x07) << 4)
#define   G_00936C_VGPR_A(x)                                          (((x) >> 4) & 0x07)
#define   C_00936C_VGPR_A                                             0xFFFFFF8F
#define   S_00936C_SGPR_A(x)                                          (((x) & 0x07) << 7)
#define   G_00936C_SGPR_A(x)                                          (((x) >> 7) & 0x07)
#define   C_00936C_SGPR_A                                             0xFFFFFC7F
#define   S_00936C_LDS_A(x)                                           (((x) & 0x07) << 10)
#define   G_00936C_LDS_A(x)                                           (((x) >> 10) & 0x07)
#define   C_00936C_LDS_A                                              0xFFFFE3FF
#define   S_00936C_WAVES_A(x)                                         (((x) & 0x03) << 13)
#define   G_00936C_WAVES_A(x)                                         (((x) >> 13) & 0x03)
#define   C_00936C_WAVES_A                                            0xFFFF9FFF
#define   S_00936C_EN_A(x)                                            (((x) & 0x1) << 15)
#define   G_00936C_EN_A(x)                                            (((x) >> 15) & 0x1)
#define   C_00936C_EN_A                                               0xFFFF7FFF
#define   S_00936C_TYPE_B(x)                                          (((x) & 0x0F) << 16)
#define   G_00936C_TYPE_B(x)                                          (((x) >> 16) & 0x0F)
#define   C_00936C_TYPE_B                                             0xFFF0FFFF
#define   S_00936C_VGPR_B(x)                                          (((x) & 0x07) << 20)
#define   G_00936C_VGPR_B(x)                                          (((x) >> 20) & 0x07)
#define   C_00936C_VGPR_B                                             0xFF8FFFFF
#define   S_00936C_SGPR_B(x)                                          (((x) & 0x07) << 23)
#define   G_00936C_SGPR_B(x)                                          (((x) >> 23) & 0x07)
#define   C_00936C_SGPR_B                                             0xFC7FFFFF
#define   S_00936C_LDS_B(x)                                           (((x) & 0x07) << 26)
#define   G_00936C_LDS_B(x)                                           (((x) >> 26) & 0x07)
#define   C_00936C_LDS_B                                              0xE3FFFFFF
#define   S_00936C_WAVES_B(x)                                         (((x) & 0x03) << 29)
#define   G_00936C_WAVES_B(x)                                         (((x) >> 29) & 0x03)
#define   C_00936C_WAVES_B                                            0x9FFFFFFF
#define   S_00936C_EN_B(x)                                            (((x) & 0x1) << 31)
#define   G_00936C_EN_B(x)                                            (((x) >> 31) & 0x1)
#define   C_00936C_EN_B                                               0x7FFFFFFF
#define R_00950C_TA_CS_BC_BASE_ADDR                                     0x00950C
#define R_009858_DB_SUBTILE_CONTROL                                     0x009858
#define   S_009858_MSAA1_X(x)                                         (((x) & 0x03) << 0)
#define   G_009858_MSAA1_X(x)                                         (((x) >> 0) & 0x03)
#define   C_009858_MSAA1_X                                            0xFFFFFFFC
#define   S_009858_MSAA1_Y(x)                                         (((x) & 0x03) << 2)
#define   G_009858_MSAA1_Y(x)                                         (((x) >> 2) & 0x03)
#define   C_009858_MSAA1_Y                                            0xFFFFFFF3
#define   S_009858_MSAA2_X(x)                                         (((x) & 0x03) << 4)
#define   G_009858_MSAA2_X(x)                                         (((x) >> 4) & 0x03)
#define   C_009858_MSAA2_X                                            0xFFFFFFCF
#define   S_009858_MSAA2_Y(x)                                         (((x) & 0x03) << 6)
#define   G_009858_MSAA2_Y(x)                                         (((x) >> 6) & 0x03)
#define   C_009858_MSAA2_Y                                            0xFFFFFF3F
#define   S_009858_MSAA4_X(x)                                         (((x) & 0x03) << 8)
#define   G_009858_MSAA4_X(x)                                         (((x) >> 8) & 0x03)
#define   C_009858_MSAA4_X                                            0xFFFFFCFF
#define   S_009858_MSAA4_Y(x)                                         (((x) & 0x03) << 10)
#define   G_009858_MSAA4_Y(x)                                         (((x) >> 10) & 0x03)
#define   C_009858_MSAA4_Y                                            0xFFFFF3FF
#define   S_009858_MSAA8_X(x)                                         (((x) & 0x03) << 12)
#define   G_009858_MSAA8_X(x)                                         (((x) >> 12) & 0x03)
#define   C_009858_MSAA8_X                                            0xFFFFCFFF
#define   S_009858_MSAA8_Y(x)                                         (((x) & 0x03) << 14)
#define   G_009858_MSAA8_Y(x)                                         (((x) >> 14) & 0x03)
#define   C_009858_MSAA8_Y                                            0xFFFF3FFF
#define   S_009858_MSAA16_X(x)                                        (((x) & 0x03) << 16)
#define   G_009858_MSAA16_X(x)                                        (((x) >> 16) & 0x03)
#define   C_009858_MSAA16_X                                           0xFFFCFFFF
#define   S_009858_MSAA16_Y(x)                                        (((x) & 0x03) << 18)
#define   G_009858_MSAA16_Y(x)                                        (((x) >> 18) & 0x03)
#define   C_009858_MSAA16_Y                                           0xFFF3FFFF
#define R_009910_GB_TILE_MODE0                                          0x009910
#define   S_009910_MICRO_TILE_MODE(x)                                 (((x) & 0x03) << 0)
#define   G_009910_MICRO_TILE_MODE(x)                                 (((x) >> 0) & 0x03)
#define   C_009910_MICRO_TILE_MODE                                    0xFFFFFFFC
#define     V_009910_ADDR_SURF_DISPLAY_MICRO_TILING                 0x00
#define     V_009910_ADDR_SURF_THIN_MICRO_TILING                    0x01
#define     V_009910_ADDR_SURF_DEPTH_MICRO_TILING                   0x02
#define     V_009910_ADDR_SURF_THICK_MICRO_TILING                   0x03
#define   S_009910_ARRAY_MODE(x)                                      (((x) & 0x0F) << 2)
#define   G_009910_ARRAY_MODE(x)                                      (((x) >> 2) & 0x0F)
#define   C_009910_ARRAY_MODE                                         0xFFFFFFC3
#define     V_009910_ARRAY_LINEAR_GENERAL                           0x00
#define     V_009910_ARRAY_LINEAR_ALIGNED                           0x01
#define     V_009910_ARRAY_1D_TILED_THIN1                           0x02
#define     V_009910_ARRAY_1D_TILED_THICK                           0x03
#define     V_009910_ARRAY_2D_TILED_THIN1                           0x04
#define     V_009910_ARRAY_2D_TILED_THICK                           0x07
#define     V_009910_ARRAY_2D_TILED_XTHICK                          0x08
#define     V_009910_ARRAY_3D_TILED_THIN1                           0x0C
#define     V_009910_ARRAY_3D_TILED_THICK                           0x0D
#define     V_009910_ARRAY_3D_TILED_XTHICK                          0x0E
#define     V_009910_ARRAY_POWER_SAVE                               0x0F
#define   S_009910_PIPE_CONFIG(x)                                     (((x) & 0x1F) << 6)
#define   G_009910_PIPE_CONFIG(x)                                     (((x) >> 6) & 0x1F)
#define   C_009910_PIPE_CONFIG                                        0xFFFFF83F
#define     V_009910_ADDR_SURF_P2                                   0x00
#define     V_009910_ADDR_SURF_P2_RESERVED0                         0x01
#define     V_009910_ADDR_SURF_P2_RESERVED1                         0x02
#define     V_009910_ADDR_SURF_P2_RESERVED2                         0x03
#define     V_009910_X_ADDR_SURF_P4_8X16                            0x04
#define     V_009910_X_ADDR_SURF_P4_16X16                           0x05
#define     V_009910_X_ADDR_SURF_P4_16X32                           0x06
#define     V_009910_X_ADDR_SURF_P4_32X32                           0x07
#define     V_009910_X_ADDR_SURF_P8_16X16_8X16                      0x08
#define     V_009910_X_ADDR_SURF_P8_16X32_8X16                      0x09
#define     V_009910_X_ADDR_SURF_P8_32X32_8X16                      0x0A
#define     V_009910_X_ADDR_SURF_P8_16X32_16X16                     0x0B
#define     V_009910_X_ADDR_SURF_P8_32X32_16X16                     0x0C
#define     V_009910_X_ADDR_SURF_P8_32X32_16X32                     0x0D
#define     V_009910_X_ADDR_SURF_P8_32X64_32X32                     0x0E
#define   S_009910_TILE_SPLIT(x)                                      (((x) & 0x07) << 11)
#define   G_009910_TILE_SPLIT(x)                                      (((x) >> 11) & 0x07)
#define   C_009910_TILE_SPLIT                                         0xFFFFC7FF
#define     V_009910_ADDR_SURF_TILE_SPLIT_64B                       0x00
#define     V_009910_ADDR_SURF_TILE_SPLIT_128B                      0x01
#define     V_009910_ADDR_SURF_TILE_SPLIT_256B                      0x02
#define     V_009910_ADDR_SURF_TILE_SPLIT_512B                      0x03
#define     V_009910_ADDR_SURF_TILE_SPLIT_1KB                       0x04
#define     V_009910_ADDR_SURF_TILE_SPLIT_2KB                       0x05
#define     V_009910_ADDR_SURF_TILE_SPLIT_4KB                       0x06
#define   S_009910_BANK_WIDTH(x)                                      (((x) & 0x03) << 14)
#define   G_009910_BANK_WIDTH(x)                                      (((x) >> 14) & 0x03)
#define   C_009910_BANK_WIDTH                                         0xFFFF3FFF
#define     V_009910_ADDR_SURF_BANK_WIDTH_1                         0x00
#define     V_009910_ADDR_SURF_BANK_WIDTH_2                         0x01
#define     V_009910_ADDR_SURF_BANK_WIDTH_4                         0x02
#define     V_009910_ADDR_SURF_BANK_WIDTH_8                         0x03
#define   S_009910_BANK_HEIGHT(x)                                     (((x) & 0x03) << 16)
#define   G_009910_BANK_HEIGHT(x)                                     (((x) >> 16) & 0x03)
#define   C_009910_BANK_HEIGHT                                        0xFFFCFFFF
#define     V_009910_ADDR_SURF_BANK_HEIGHT_1                        0x00
#define     V_009910_ADDR_SURF_BANK_HEIGHT_2                        0x01
#define     V_009910_ADDR_SURF_BANK_HEIGHT_4                        0x02
#define     V_009910_ADDR_SURF_BANK_HEIGHT_8                        0x03
#define   S_009910_MACRO_TILE_ASPECT(x)                               (((x) & 0x03) << 18)
#define   G_009910_MACRO_TILE_ASPECT(x)                               (((x) >> 18) & 0x03)
#define   C_009910_MACRO_TILE_ASPECT                                  0xFFF3FFFF
#define     V_009910_ADDR_SURF_MACRO_ASPECT_1                       0x00
#define     V_009910_ADDR_SURF_MACRO_ASPECT_2                       0x01
#define     V_009910_ADDR_SURF_MACRO_ASPECT_4                       0x02
#define     V_009910_ADDR_SURF_MACRO_ASPECT_8                       0x03
#define   S_009910_NUM_BANKS(x)                                       (((x) & 0x03) << 20)
#define   G_009910_NUM_BANKS(x)                                       (((x) >> 20) & 0x03)
#define   C_009910_NUM_BANKS                                          0xFFCFFFFF
#define     V_009910_ADDR_SURF_2_BANK                               0x00
#define     V_009910_ADDR_SURF_4_BANK                               0x01
#define     V_009910_ADDR_SURF_8_BANK                               0x02
#define     V_009910_ADDR_SURF_16_BANK                              0x03
#define R_00B020_SPI_SHADER_PGM_LO_PS                                   0x00B020
#define R_00B024_SPI_SHADER_PGM_HI_PS                                   0x00B024
#define   S_00B024_MEM_BASE(x)                                        (((x) & 0xFF) << 0)
#define   G_00B024_MEM_BASE(x)                                        (((x) >> 0) & 0xFF)
#define   C_00B024_MEM_BASE                                           0xFFFFFF00
#define R_00B028_SPI_SHADER_PGM_RSRC1_PS                                0x00B028
#define   S_00B028_VGPRS(x)                                           (((x) & 0x3F) << 0)
#define   G_00B028_VGPRS(x)                                           (((x) >> 0) & 0x3F)
#define   C_00B028_VGPRS                                              0xFFFFFFC0
#define   S_00B028_SGPRS(x)                                           (((x) & 0x0F) << 6)
#define   G_00B028_SGPRS(x)                                           (((x) >> 6) & 0x0F)
#define   C_00B028_SGPRS                                              0xFFFFFC3F
#define   S_00B028_PRIORITY(x)                                        (((x) & 0x03) << 10)
#define   G_00B028_PRIORITY(x)                                        (((x) >> 10) & 0x03)
#define   C_00B028_PRIORITY                                           0xFFFFF3FF
#define   S_00B028_FLOAT_MODE(x)                                      (((x) & 0xFF) << 12)
#define   G_00B028_FLOAT_MODE(x)                                      (((x) >> 12) & 0xFF)
#define   C_00B028_FLOAT_MODE                                         0xFFF00FFF
#define   S_00B028_PRIV(x)                                            (((x) & 0x1) << 20)
#define   G_00B028_PRIV(x)                                            (((x) >> 20) & 0x1)
#define   C_00B028_PRIV                                               0xFFEFFFFF
#define   S_00B028_DX10_CLAMP(x)                                      (((x) & 0x1) << 21)
#define   G_00B028_DX10_CLAMP(x)                                      (((x) >> 21) & 0x1)
#define   C_00B028_DX10_CLAMP                                         0xFFDFFFFF
#define   S_00B028_DEBUG_MODE(x)                                      (((x) & 0x1) << 22)
#define   G_00B028_DEBUG_MODE(x)                                      (((x) >> 22) & 0x1)
#define   C_00B028_DEBUG_MODE                                         0xFFBFFFFF
#define   S_00B028_IEEE_MODE(x)                                       (((x) & 0x1) << 23)
#define   G_00B028_IEEE_MODE(x)                                       (((x) >> 23) & 0x1)
#define   C_00B028_IEEE_MODE                                          0xFF7FFFFF
#define   S_00B028_CU_GROUP_DISABLE(x)                                (((x) & 0x1) << 24)
#define   G_00B028_CU_GROUP_DISABLE(x)                                (((x) >> 24) & 0x1)
#define   C_00B028_CU_GROUP_DISABLE                                   0xFEFFFFFF
#define R_00B02C_SPI_SHADER_PGM_RSRC2_PS                                0x00B02C
#define   S_00B02C_SCRATCH_EN(x)                                      (((x) & 0x1) << 0)
#define   G_00B02C_SCRATCH_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_00B02C_SCRATCH_EN                                         0xFFFFFFFE
#define   S_00B02C_USER_SGPR(x)                                       (((x) & 0x1F) << 1)
#define   G_00B02C_USER_SGPR(x)                                       (((x) >> 1) & 0x1F)
#define   C_00B02C_USER_SGPR                                          0xFFFFFFC1
#define   S_00B02C_WAVE_CNT_EN(x)                                     (((x) & 0x1) << 7)
#define   G_00B02C_WAVE_CNT_EN(x)                                     (((x) >> 7) & 0x1)
#define   C_00B02C_WAVE_CNT_EN                                        0xFFFFFF7F
#define   S_00B02C_EXTRA_LDS_SIZE(x)                                  (((x) & 0xFF) << 8)
#define   G_00B02C_EXTRA_LDS_SIZE(x)                                  (((x) >> 8) & 0xFF)
#define   C_00B02C_EXTRA_LDS_SIZE                                     0xFFFF00FF
#define   S_00B02C_EXCP_EN(x)                                         (((x) & 0x7F) << 16)
#define   G_00B02C_EXCP_EN(x)                                         (((x) >> 16) & 0x7F)
#define   C_00B02C_EXCP_EN                                            0xFF80FFFF
#define R_00B030_SPI_SHADER_USER_DATA_PS_0                              0x00B030
#define R_00B034_SPI_SHADER_USER_DATA_PS_1                              0x00B034
#define R_00B038_SPI_SHADER_USER_DATA_PS_2                              0x00B038
#define R_00B03C_SPI_SHADER_USER_DATA_PS_3                              0x00B03C
#define R_00B040_SPI_SHADER_USER_DATA_PS_4                              0x00B040
#define R_00B044_SPI_SHADER_USER_DATA_PS_5                              0x00B044
#define R_00B048_SPI_SHADER_USER_DATA_PS_6                              0x00B048
#define R_00B04C_SPI_SHADER_USER_DATA_PS_7                              0x00B04C
#define R_00B050_SPI_SHADER_USER_DATA_PS_8                              0x00B050
#define R_00B054_SPI_SHADER_USER_DATA_PS_9                              0x00B054
#define R_00B058_SPI_SHADER_USER_DATA_PS_10                             0x00B058
#define R_00B05C_SPI_SHADER_USER_DATA_PS_11                             0x00B05C
#define R_00B060_SPI_SHADER_USER_DATA_PS_12                             0x00B060
#define R_00B064_SPI_SHADER_USER_DATA_PS_13                             0x00B064
#define R_00B068_SPI_SHADER_USER_DATA_PS_14                             0x00B068
#define R_00B06C_SPI_SHADER_USER_DATA_PS_15                             0x00B06C
#define R_00B120_SPI_SHADER_PGM_LO_VS                                   0x00B120
#define R_00B124_SPI_SHADER_PGM_HI_VS                                   0x00B124
#define   S_00B124_MEM_BASE(x)                                        (((x) & 0xFF) << 0)
#define   G_00B124_MEM_BASE(x)                                        (((x) >> 0) & 0xFF)
#define   C_00B124_MEM_BASE                                           0xFFFFFF00
#define R_00B128_SPI_SHADER_PGM_RSRC1_VS                                0x00B128
#define   S_00B128_VGPRS(x)                                           (((x) & 0x3F) << 0)
#define   G_00B128_VGPRS(x)                                           (((x) >> 0) & 0x3F)
#define   C_00B128_VGPRS                                              0xFFFFFFC0
#define   S_00B128_SGPRS(x)                                           (((x) & 0x0F) << 6)
#define   G_00B128_SGPRS(x)                                           (((x) >> 6) & 0x0F)
#define   C_00B128_SGPRS                                              0xFFFFFC3F
#define   S_00B128_PRIORITY(x)                                        (((x) & 0x03) << 10)
#define   G_00B128_PRIORITY(x)                                        (((x) >> 10) & 0x03)
#define   C_00B128_PRIORITY                                           0xFFFFF3FF
#define   S_00B128_FLOAT_MODE(x)                                      (((x) & 0xFF) << 12)
#define   G_00B128_FLOAT_MODE(x)                                      (((x) >> 12) & 0xFF)
#define   C_00B128_FLOAT_MODE                                         0xFFF00FFF
#define   S_00B128_PRIV(x)                                            (((x) & 0x1) << 20)
#define   G_00B128_PRIV(x)                                            (((x) >> 20) & 0x1)
#define   C_00B128_PRIV                                               0xFFEFFFFF
#define   S_00B128_DX10_CLAMP(x)                                      (((x) & 0x1) << 21)
#define   G_00B128_DX10_CLAMP(x)                                      (((x) >> 21) & 0x1)
#define   C_00B128_DX10_CLAMP                                         0xFFDFFFFF
#define   S_00B128_DEBUG_MODE(x)                                      (((x) & 0x1) << 22)
#define   G_00B128_DEBUG_MODE(x)                                      (((x) >> 22) & 0x1)
#define   C_00B128_DEBUG_MODE                                         0xFFBFFFFF
#define   S_00B128_IEEE_MODE(x)                                       (((x) & 0x1) << 23)
#define   G_00B128_IEEE_MODE(x)                                       (((x) >> 23) & 0x1)
#define   C_00B128_IEEE_MODE                                          0xFF7FFFFF
#define   S_00B128_VGPR_COMP_CNT(x)                                   (((x) & 0x03) << 24)
#define   G_00B128_VGPR_COMP_CNT(x)                                   (((x) >> 24) & 0x03)
#define   C_00B128_VGPR_COMP_CNT                                      0xFCFFFFFF
#define   S_00B128_CU_GROUP_ENABLE(x)                                 (((x) & 0x1) << 26)
#define   G_00B128_CU_GROUP_ENABLE(x)                                 (((x) >> 26) & 0x1)
#define   C_00B128_CU_GROUP_ENABLE                                    0xFBFFFFFF
#define R_00B12C_SPI_SHADER_PGM_RSRC2_VS                                0x00B12C
#define   S_00B12C_SCRATCH_EN(x)                                      (((x) & 0x1) << 0)
#define   G_00B12C_SCRATCH_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_00B12C_SCRATCH_EN                                         0xFFFFFFFE
#define   S_00B12C_USER_SGPR(x)                                       (((x) & 0x1F) << 1)
#define   G_00B12C_USER_SGPR(x)                                       (((x) >> 1) & 0x1F)
#define   C_00B12C_USER_SGPR                                          0xFFFFFFC1
#define   S_00B12C_OC_LDS_EN(x)                                       (((x) & 0x1) << 7)
#define   G_00B12C_OC_LDS_EN(x)                                       (((x) >> 7) & 0x1)
#define   C_00B12C_OC_LDS_EN                                          0xFFFFFF7F
#define   S_00B12C_SO_BASE0_EN(x)                                     (((x) & 0x1) << 8)
#define   G_00B12C_SO_BASE0_EN(x)                                     (((x) >> 8) & 0x1)
#define   C_00B12C_SO_BASE0_EN                                        0xFFFFFEFF
#define   S_00B12C_SO_BASE1_EN(x)                                     (((x) & 0x1) << 9)
#define   G_00B12C_SO_BASE1_EN(x)                                     (((x) >> 9) & 0x1)
#define   C_00B12C_SO_BASE1_EN                                        0xFFFFFDFF
#define   S_00B12C_SO_BASE2_EN(x)                                     (((x) & 0x1) << 10)
#define   G_00B12C_SO_BASE2_EN(x)                                     (((x) >> 10) & 0x1)
#define   C_00B12C_SO_BASE2_EN                                        0xFFFFFBFF
#define   S_00B12C_SO_BASE3_EN(x)                                     (((x) & 0x1) << 11)
#define   G_00B12C_SO_BASE3_EN(x)                                     (((x) >> 11) & 0x1)
#define   C_00B12C_SO_BASE3_EN                                        0xFFFFF7FF
#define   S_00B12C_SO_EN(x)                                           (((x) & 0x1) << 12)
#define   G_00B12C_SO_EN(x)                                           (((x) >> 12) & 0x1)
#define   C_00B12C_SO_EN                                              0xFFFFEFFF
#define   S_00B12C_EXCP_EN(x)                                         (((x) & 0x7F) << 13)
#define   G_00B12C_EXCP_EN(x)                                         (((x) >> 13) & 0x7F)
#define   C_00B12C_EXCP_EN                                            0xFFF01FFF
#define R_00B130_SPI_SHADER_USER_DATA_VS_0                              0x00B130
#define R_00B134_SPI_SHADER_USER_DATA_VS_1                              0x00B134
#define R_00B138_SPI_SHADER_USER_DATA_VS_2                              0x00B138
#define R_00B13C_SPI_SHADER_USER_DATA_VS_3                              0x00B13C
#define R_00B140_SPI_SHADER_USER_DATA_VS_4                              0x00B140
#define R_00B144_SPI_SHADER_USER_DATA_VS_5                              0x00B144
#define R_00B148_SPI_SHADER_USER_DATA_VS_6                              0x00B148
#define R_00B14C_SPI_SHADER_USER_DATA_VS_7                              0x00B14C
#define R_00B150_SPI_SHADER_USER_DATA_VS_8                              0x00B150
#define R_00B154_SPI_SHADER_USER_DATA_VS_9                              0x00B154
#define R_00B158_SPI_SHADER_USER_DATA_VS_10                             0x00B158
#define R_00B15C_SPI_SHADER_USER_DATA_VS_11                             0x00B15C
#define R_00B160_SPI_SHADER_USER_DATA_VS_12                             0x00B160
#define R_00B164_SPI_SHADER_USER_DATA_VS_13                             0x00B164
#define R_00B168_SPI_SHADER_USER_DATA_VS_14                             0x00B168
#define R_00B16C_SPI_SHADER_USER_DATA_VS_15                             0x00B16C
#define R_00B220_SPI_SHADER_PGM_LO_GS                                   0x00B220
#define R_00B224_SPI_SHADER_PGM_HI_GS                                   0x00B224
#define   S_00B224_MEM_BASE(x)                                        (((x) & 0xFF) << 0)
#define   G_00B224_MEM_BASE(x)                                        (((x) >> 0) & 0xFF)
#define   C_00B224_MEM_BASE                                           0xFFFFFF00
#define R_00B228_SPI_SHADER_PGM_RSRC1_GS                                0x00B228
#define   S_00B228_VGPRS(x)                                           (((x) & 0x3F) << 0)
#define   G_00B228_VGPRS(x)                                           (((x) >> 0) & 0x3F)
#define   C_00B228_VGPRS                                              0xFFFFFFC0
#define   S_00B228_SGPRS(x)                                           (((x) & 0x0F) << 6)
#define   G_00B228_SGPRS(x)                                           (((x) >> 6) & 0x0F)
#define   C_00B228_SGPRS                                              0xFFFFFC3F
#define   S_00B228_PRIORITY(x)                                        (((x) & 0x03) << 10)
#define   G_00B228_PRIORITY(x)                                        (((x) >> 10) & 0x03)
#define   C_00B228_PRIORITY                                           0xFFFFF3FF
#define   S_00B228_FLOAT_MODE(x)                                      (((x) & 0xFF) << 12)
#define   G_00B228_FLOAT_MODE(x)                                      (((x) >> 12) & 0xFF)
#define   C_00B228_FLOAT_MODE                                         0xFFF00FFF
#define   S_00B228_PRIV(x)                                            (((x) & 0x1) << 20)
#define   G_00B228_PRIV(x)                                            (((x) >> 20) & 0x1)
#define   C_00B228_PRIV                                               0xFFEFFFFF
#define   S_00B228_DX10_CLAMP(x)                                      (((x) & 0x1) << 21)
#define   G_00B228_DX10_CLAMP(x)                                      (((x) >> 21) & 0x1)
#define   C_00B228_DX10_CLAMP                                         0xFFDFFFFF
#define   S_00B228_DEBUG_MODE(x)                                      (((x) & 0x1) << 22)
#define   G_00B228_DEBUG_MODE(x)                                      (((x) >> 22) & 0x1)
#define   C_00B228_DEBUG_MODE                                         0xFFBFFFFF
#define   S_00B228_IEEE_MODE(x)                                       (((x) & 0x1) << 23)
#define   G_00B228_IEEE_MODE(x)                                       (((x) >> 23) & 0x1)
#define   C_00B228_IEEE_MODE                                          0xFF7FFFFF
#define   S_00B228_CU_GROUP_ENABLE(x)                                 (((x) & 0x1) << 24)
#define   G_00B228_CU_GROUP_ENABLE(x)                                 (((x) >> 24) & 0x1)
#define   C_00B228_CU_GROUP_ENABLE                                    0xFEFFFFFF
#define R_00B22C_SPI_SHADER_PGM_RSRC2_GS                                0x00B22C
#define   S_00B22C_SCRATCH_EN(x)                                      (((x) & 0x1) << 0)
#define   G_00B22C_SCRATCH_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_00B22C_SCRATCH_EN                                         0xFFFFFFFE
#define   S_00B22C_USER_SGPR(x)                                       (((x) & 0x1F) << 1)
#define   G_00B22C_USER_SGPR(x)                                       (((x) >> 1) & 0x1F)
#define   C_00B22C_USER_SGPR                                          0xFFFFFFC1
#define   S_00B22C_EXCP_EN(x)                                         (((x) & 0x7F) << 7)
#define   G_00B22C_EXCP_EN(x)                                         (((x) >> 7) & 0x7F)
#define   C_00B22C_EXCP_EN                                            0xFFFFC07F
#define R_00B230_SPI_SHADER_USER_DATA_GS_0                              0x00B230
#define R_00B320_SPI_SHADER_PGM_LO_ES                                   0x00B320
#define R_00B324_SPI_SHADER_PGM_HI_ES                                   0x00B324
#define   S_00B324_MEM_BASE(x)                                        (((x) & 0xFF) << 0)
#define   G_00B324_MEM_BASE(x)                                        (((x) >> 0) & 0xFF)
#define   C_00B324_MEM_BASE                                           0xFFFFFF00
#define R_00B328_SPI_SHADER_PGM_RSRC1_ES                                0x00B328
#define   S_00B328_VGPRS(x)                                           (((x) & 0x3F) << 0)
#define   G_00B328_VGPRS(x)                                           (((x) >> 0) & 0x3F)
#define   C_00B328_VGPRS                                              0xFFFFFFC0
#define   S_00B328_SGPRS(x)                                           (((x) & 0x0F) << 6)
#define   G_00B328_SGPRS(x)                                           (((x) >> 6) & 0x0F)
#define   C_00B328_SGPRS                                              0xFFFFFC3F
#define   S_00B328_PRIORITY(x)                                        (((x) & 0x03) << 10)
#define   G_00B328_PRIORITY(x)                                        (((x) >> 10) & 0x03)
#define   C_00B328_PRIORITY                                           0xFFFFF3FF
#define   S_00B328_FLOAT_MODE(x)                                      (((x) & 0xFF) << 12)
#define   G_00B328_FLOAT_MODE(x)                                      (((x) >> 12) & 0xFF)
#define   C_00B328_FLOAT_MODE                                         0xFFF00FFF
#define   S_00B328_PRIV(x)                                            (((x) & 0x1) << 20)
#define   G_00B328_PRIV(x)                                            (((x) >> 20) & 0x1)
#define   C_00B328_PRIV                                               0xFFEFFFFF
#define   S_00B328_DX10_CLAMP(x)                                      (((x) & 0x1) << 21)
#define   G_00B328_DX10_CLAMP(x)                                      (((x) >> 21) & 0x1)
#define   C_00B328_DX10_CLAMP                                         0xFFDFFFFF
#define   S_00B328_DEBUG_MODE(x)                                      (((x) & 0x1) << 22)
#define   G_00B328_DEBUG_MODE(x)                                      (((x) >> 22) & 0x1)
#define   C_00B328_DEBUG_MODE                                         0xFFBFFFFF
#define   S_00B328_IEEE_MODE(x)                                       (((x) & 0x1) << 23)
#define   G_00B328_IEEE_MODE(x)                                       (((x) >> 23) & 0x1)
#define   C_00B328_IEEE_MODE                                          0xFF7FFFFF
#define   S_00B328_VGPR_COMP_CNT(x)                                   (((x) & 0x03) << 24)
#define   G_00B328_VGPR_COMP_CNT(x)                                   (((x) >> 24) & 0x03)
#define   C_00B328_VGPR_COMP_CNT                                      0xFCFFFFFF
#define   S_00B328_CU_GROUP_ENABLE(x)                                 (((x) & 0x1) << 26)
#define   G_00B328_CU_GROUP_ENABLE(x)                                 (((x) >> 26) & 0x1)
#define   C_00B328_CU_GROUP_ENABLE                                    0xFBFFFFFF
#define R_00B32C_SPI_SHADER_PGM_RSRC2_ES                                0x00B32C
#define   S_00B32C_SCRATCH_EN(x)                                      (((x) & 0x1) << 0)
#define   G_00B32C_SCRATCH_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_00B32C_SCRATCH_EN                                         0xFFFFFFFE
#define   S_00B32C_USER_SGPR(x)                                       (((x) & 0x1F) << 1)
#define   G_00B32C_USER_SGPR(x)                                       (((x) >> 1) & 0x1F)
#define   C_00B32C_USER_SGPR                                          0xFFFFFFC1
#define   S_00B32C_OC_LDS_EN(x)                                       (((x) & 0x1) << 7)
#define   G_00B32C_OC_LDS_EN(x)                                       (((x) >> 7) & 0x1)
#define   C_00B32C_OC_LDS_EN                                          0xFFFFFF7F
#define   S_00B32C_EXCP_EN(x)                                         (((x) & 0x7F) << 8)
#define   G_00B32C_EXCP_EN(x)                                         (((x) >> 8) & 0x7F)
#define   C_00B32C_EXCP_EN                                            0xFFFF80FF
#define R_00B330_SPI_SHADER_USER_DATA_ES_0                              0x00B330
#define R_00B420_SPI_SHADER_PGM_LO_HS                                   0x00B420
#define R_00B424_SPI_SHADER_PGM_HI_HS                                   0x00B424
#define   S_00B424_MEM_BASE(x)                                        (((x) & 0xFF) << 0)
#define   G_00B424_MEM_BASE(x)                                        (((x) >> 0) & 0xFF)
#define   C_00B424_MEM_BASE                                           0xFFFFFF00
#define R_00B428_SPI_SHADER_PGM_RSRC1_HS                                0x00B428
#define   S_00B428_VGPRS(x)                                           (((x) & 0x3F) << 0)
#define   G_00B428_VGPRS(x)                                           (((x) >> 0) & 0x3F)
#define   C_00B428_VGPRS                                              0xFFFFFFC0
#define   S_00B428_SGPRS(x)                                           (((x) & 0x0F) << 6)
#define   G_00B428_SGPRS(x)                                           (((x) >> 6) & 0x0F)
#define   C_00B428_SGPRS                                              0xFFFFFC3F
#define   S_00B428_PRIORITY(x)                                        (((x) & 0x03) << 10)
#define   G_00B428_PRIORITY(x)                                        (((x) >> 10) & 0x03)
#define   C_00B428_PRIORITY                                           0xFFFFF3FF
#define   S_00B428_FLOAT_MODE(x)                                      (((x) & 0xFF) << 12)
#define   G_00B428_FLOAT_MODE(x)                                      (((x) >> 12) & 0xFF)
#define   C_00B428_FLOAT_MODE                                         0xFFF00FFF
#define   S_00B428_PRIV(x)                                            (((x) & 0x1) << 20)
#define   G_00B428_PRIV(x)                                            (((x) >> 20) & 0x1)
#define   C_00B428_PRIV                                               0xFFEFFFFF
#define   S_00B428_DX10_CLAMP(x)                                      (((x) & 0x1) << 21)
#define   G_00B428_DX10_CLAMP(x)                                      (((x) >> 21) & 0x1)
#define   C_00B428_DX10_CLAMP                                         0xFFDFFFFF
#define   S_00B428_DEBUG_MODE(x)                                      (((x) & 0x1) << 22)
#define   G_00B428_DEBUG_MODE(x)                                      (((x) >> 22) & 0x1)
#define   C_00B428_DEBUG_MODE                                         0xFFBFFFFF
#define   S_00B428_IEEE_MODE(x)                                       (((x) & 0x1) << 23)
#define   G_00B428_IEEE_MODE(x)                                       (((x) >> 23) & 0x1)
#define   C_00B428_IEEE_MODE                                          0xFF7FFFFF
#define R_00B42C_SPI_SHADER_PGM_RSRC2_HS                                0x00B42C
#define   S_00B42C_SCRATCH_EN(x)                                      (((x) & 0x1) << 0)
#define   G_00B42C_SCRATCH_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_00B42C_SCRATCH_EN                                         0xFFFFFFFE
#define   S_00B42C_USER_SGPR(x)                                       (((x) & 0x1F) << 1)
#define   G_00B42C_USER_SGPR(x)                                       (((x) >> 1) & 0x1F)
#define   C_00B42C_USER_SGPR                                          0xFFFFFFC1
#define   S_00B42C_OC_LDS_EN(x)                                       (((x) & 0x1) << 7)
#define   G_00B42C_OC_LDS_EN(x)                                       (((x) >> 7) & 0x1)
#define   C_00B42C_OC_LDS_EN                                          0xFFFFFF7F
#define   S_00B42C_TG_SIZE_EN(x)                                      (((x) & 0x1) << 8)
#define   G_00B42C_TG_SIZE_EN(x)                                      (((x) >> 8) & 0x1)
#define   C_00B42C_TG_SIZE_EN                                         0xFFFFFEFF
#define   S_00B42C_EXCP_EN(x)                                         (((x) & 0x7F) << 9)
#define   G_00B42C_EXCP_EN(x)                                         (((x) >> 9) & 0x7F)
#define   C_00B42C_EXCP_EN                                            0xFFFF01FF
#define R_00B430_SPI_SHADER_USER_DATA_HS_0                              0x00B430
#define R_00B520_SPI_SHADER_PGM_LO_LS                                   0x00B520
#define R_00B524_SPI_SHADER_PGM_HI_LS                                   0x00B524
#define   S_00B524_MEM_BASE(x)                                        (((x) & 0xFF) << 0)
#define   G_00B524_MEM_BASE(x)                                        (((x) >> 0) & 0xFF)
#define   C_00B524_MEM_BASE                                           0xFFFFFF00
#define R_00B528_SPI_SHADER_PGM_RSRC1_LS                                0x00B528
#define   S_00B528_VGPRS(x)                                           (((x) & 0x3F) << 0)
#define   G_00B528_VGPRS(x)                                           (((x) >> 0) & 0x3F)
#define   C_00B528_VGPRS                                              0xFFFFFFC0
#define   S_00B528_SGPRS(x)                                           (((x) & 0x0F) << 6)
#define   G_00B528_SGPRS(x)                                           (((x) >> 6) & 0x0F)
#define   C_00B528_SGPRS                                              0xFFFFFC3F
#define   S_00B528_PRIORITY(x)                                        (((x) & 0x03) << 10)
#define   G_00B528_PRIORITY(x)                                        (((x) >> 10) & 0x03)
#define   C_00B528_PRIORITY                                           0xFFFFF3FF
#define   S_00B528_FLOAT_MODE(x)                                      (((x) & 0xFF) << 12)
#define   G_00B528_FLOAT_MODE(x)                                      (((x) >> 12) & 0xFF)
#define   C_00B528_FLOAT_MODE                                         0xFFF00FFF
#define   S_00B528_PRIV(x)                                            (((x) & 0x1) << 20)
#define   G_00B528_PRIV(x)                                            (((x) >> 20) & 0x1)
#define   C_00B528_PRIV                                               0xFFEFFFFF
#define   S_00B528_DX10_CLAMP(x)                                      (((x) & 0x1) << 21)
#define   G_00B528_DX10_CLAMP(x)                                      (((x) >> 21) & 0x1)
#define   C_00B528_DX10_CLAMP                                         0xFFDFFFFF
#define   S_00B528_DEBUG_MODE(x)                                      (((x) & 0x1) << 22)
#define   G_00B528_DEBUG_MODE(x)                                      (((x) >> 22) & 0x1)
#define   C_00B528_DEBUG_MODE                                         0xFFBFFFFF
#define   S_00B528_IEEE_MODE(x)                                       (((x) & 0x1) << 23)
#define   G_00B528_IEEE_MODE(x)                                       (((x) >> 23) & 0x1)
#define   C_00B528_IEEE_MODE                                          0xFF7FFFFF
#define   S_00B528_VGPR_COMP_CNT(x)                                   (((x) & 0x03) << 24)
#define   G_00B528_VGPR_COMP_CNT(x)                                   (((x) >> 24) & 0x03)
#define   C_00B528_VGPR_COMP_CNT                                      0xFCFFFFFF
#define R_00B52C_SPI_SHADER_PGM_RSRC2_LS                                0x00B52C
#define   S_00B52C_SCRATCH_EN(x)                                      (((x) & 0x1) << 0)
#define   G_00B52C_SCRATCH_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_00B52C_SCRATCH_EN                                         0xFFFFFFFE
#define   S_00B52C_USER_SGPR(x)                                       (((x) & 0x1F) << 1)
#define   G_00B52C_USER_SGPR(x)                                       (((x) >> 1) & 0x1F)
#define   C_00B52C_USER_SGPR                                          0xFFFFFFC1
#define   S_00B52C_LDS_SIZE(x)                                        (((x) & 0x1FF) << 7)
#define   G_00B52C_LDS_SIZE(x)                                        (((x) >> 7) & 0x1FF)
#define   C_00B52C_LDS_SIZE                                           0xFFFF007F
#define   S_00B52C_EXCP_EN(x)                                         (((x) & 0x7F) << 16)
#define   G_00B52C_EXCP_EN(x)                                         (((x) >> 16) & 0x7F)
#define   C_00B52C_EXCP_EN                                            0xFF80FFFF
#define R_00B530_SPI_SHADER_USER_DATA_LS_0                              0x00B530
#define R_00B800_COMPUTE_DISPATCH_INITIATOR                             0x00B800
#define   S_00B800_COMPUTE_SHADER_EN(x)                               (((x) & 0x1) << 0)
#define   G_00B800_COMPUTE_SHADER_EN(x)                               (((x) >> 0) & 0x1)
#define   C_00B800_COMPUTE_SHADER_EN                                  0xFFFFFFFE
#define   S_00B800_PARTIAL_TG_EN(x)                                   (((x) & 0x1) << 1)
#define   G_00B800_PARTIAL_TG_EN(x)                                   (((x) >> 1) & 0x1)
#define   C_00B800_PARTIAL_TG_EN                                      0xFFFFFFFD
#define   S_00B800_FORCE_START_AT_000(x)                              (((x) & 0x1) << 2)
#define   G_00B800_FORCE_START_AT_000(x)                              (((x) >> 2) & 0x1)
#define   C_00B800_FORCE_START_AT_000                                 0xFFFFFFFB
#define   S_00B800_ORDERED_APPEND_ENBL(x)                             (((x) & 0x1) << 3)
#define   G_00B800_ORDERED_APPEND_ENBL(x)                             (((x) >> 3) & 0x1)
#define   C_00B800_ORDERED_APPEND_ENBL                                0xFFFFFFF7
#define R_00B804_COMPUTE_DIM_X                                          0x00B804
#define R_00B808_COMPUTE_DIM_Y                                          0x00B808
#define R_00B80C_COMPUTE_DIM_Z                                          0x00B80C
#define R_00B810_COMPUTE_START_X                                        0x00B810
#define R_00B814_COMPUTE_START_Y                                        0x00B814
#define R_00B818_COMPUTE_START_Z                                        0x00B818
#define R_00B81C_COMPUTE_NUM_THREAD_X                                   0x00B81C
#define   S_00B81C_NUM_THREAD_FULL(x)                                 (((x) & 0xFFFF) << 0)
#define   G_00B81C_NUM_THREAD_FULL(x)                                 (((x) >> 0) & 0xFFFF)
#define   C_00B81C_NUM_THREAD_FULL                                    0xFFFF0000
#define   S_00B81C_NUM_THREAD_PARTIAL(x)                              (((x) & 0xFFFF) << 16)
#define   G_00B81C_NUM_THREAD_PARTIAL(x)                              (((x) >> 16) & 0xFFFF)
#define   C_00B81C_NUM_THREAD_PARTIAL                                 0x0000FFFF
#define R_00B820_COMPUTE_NUM_THREAD_Y                                   0x00B820
#define   S_00B820_NUM_THREAD_FULL(x)                                 (((x) & 0xFFFF) << 0)
#define   G_00B820_NUM_THREAD_FULL(x)                                 (((x) >> 0) & 0xFFFF)
#define   C_00B820_NUM_THREAD_FULL                                    0xFFFF0000
#define   S_00B820_NUM_THREAD_PARTIAL(x)                              (((x) & 0xFFFF) << 16)
#define   G_00B820_NUM_THREAD_PARTIAL(x)                              (((x) >> 16) & 0xFFFF)
#define   C_00B820_NUM_THREAD_PARTIAL                                 0x0000FFFF
#define R_00B824_COMPUTE_NUM_THREAD_Z                                   0x00B824
#define   S_00B824_NUM_THREAD_FULL(x)                                 (((x) & 0xFFFF) << 0)
#define   G_00B824_NUM_THREAD_FULL(x)                                 (((x) >> 0) & 0xFFFF)
#define   C_00B824_NUM_THREAD_FULL                                    0xFFFF0000
#define   S_00B824_NUM_THREAD_PARTIAL(x)                              (((x) & 0xFFFF) << 16)
#define   G_00B824_NUM_THREAD_PARTIAL(x)                              (((x) >> 16) & 0xFFFF)
#define   C_00B824_NUM_THREAD_PARTIAL                                 0x0000FFFF
#define R_00B82C_COMPUTE_MAX_WAVE_ID                                    0x00B82C
#define   S_00B82C_MAX_WAVE_ID(x)                                     (((x) & 0xFFF) << 0)
#define   G_00B82C_MAX_WAVE_ID(x)                                     (((x) >> 0) & 0xFFF)
#define   C_00B82C_MAX_WAVE_ID                                        0xFFFFF000
#define R_00B830_COMPUTE_PGM_LO                                         0x00B830
#define R_00B834_COMPUTE_PGM_HI                                         0x00B834
#define   S_00B834_DATA(x)                                            (((x) & 0xFF) << 0)
#define   G_00B834_DATA(x)                                            (((x) >> 0) & 0xFF)
#define   C_00B834_DATA                                               0xFFFFFF00
#define R_00B848_COMPUTE_PGM_RSRC1                                      0x00B848
#define   S_00B848_VGPRS(x)                                           (((x) & 0x3F) << 0)
#define   G_00B848_VGPRS(x)                                           (((x) >> 0) & 0x3F)
#define   C_00B848_VGPRS                                              0xFFFFFFC0
#define   S_00B848_SGPRS(x)                                           (((x) & 0x0F) << 6)
#define   G_00B848_SGPRS(x)                                           (((x) >> 6) & 0x0F)
#define   C_00B848_SGPRS                                              0xFFFFFC3F
#define   S_00B848_PRIORITY(x)                                        (((x) & 0x03) << 10)
#define   G_00B848_PRIORITY(x)                                        (((x) >> 10) & 0x03)
#define   C_00B848_PRIORITY                                           0xFFFFF3FF
#define   S_00B848_FLOAT_MODE(x)                                      (((x) & 0xFF) << 12)
#define   G_00B848_FLOAT_MODE(x)                                      (((x) >> 12) & 0xFF)
#define   C_00B848_FLOAT_MODE                                         0xFFF00FFF
#define   S_00B848_PRIV(x)                                            (((x) & 0x1) << 20)
#define   G_00B848_PRIV(x)                                            (((x) >> 20) & 0x1)
#define   C_00B848_PRIV                                               0xFFEFFFFF
#define   S_00B848_DX10_CLAMP(x)                                      (((x) & 0x1) << 21)
#define   G_00B848_DX10_CLAMP(x)                                      (((x) >> 21) & 0x1)
#define   C_00B848_DX10_CLAMP                                         0xFFDFFFFF
#define   S_00B848_DEBUG_MODE(x)                                      (((x) & 0x1) << 22)
#define   G_00B848_DEBUG_MODE(x)                                      (((x) >> 22) & 0x1)
#define   C_00B848_DEBUG_MODE                                         0xFFBFFFFF
#define   S_00B848_IEEE_MODE(x)                                       (((x) & 0x1) << 23)
#define   G_00B848_IEEE_MODE(x)                                       (((x) >> 23) & 0x1)
#define   C_00B848_IEEE_MODE                                          0xFF7FFFFF
#define R_00B84C_COMPUTE_PGM_RSRC2                                      0x00B84C
#define   S_00B84C_SCRATCH_EN(x)                                      (((x) & 0x1) << 0)
#define   G_00B84C_SCRATCH_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_00B84C_SCRATCH_EN                                         0xFFFFFFFE
#define   S_00B84C_USER_SGPR(x)                                       (((x) & 0x1F) << 1)
#define   G_00B84C_USER_SGPR(x)                                       (((x) >> 1) & 0x1F)
#define   C_00B84C_USER_SGPR                                          0xFFFFFFC1
#define   S_00B84C_TGID_X_EN(x)                                       (((x) & 0x1) << 7)
#define   G_00B84C_TGID_X_EN(x)                                       (((x) >> 7) & 0x1)
#define   C_00B84C_TGID_X_EN                                          0xFFFFFF7F
#define   S_00B84C_TGID_Y_EN(x)                                       (((x) & 0x1) << 8)
#define   G_00B84C_TGID_Y_EN(x)                                       (((x) >> 8) & 0x1)
#define   C_00B84C_TGID_Y_EN                                          0xFFFFFEFF
#define   S_00B84C_TGID_Z_EN(x)                                       (((x) & 0x1) << 9)
#define   G_00B84C_TGID_Z_EN(x)                                       (((x) >> 9) & 0x1)
#define   C_00B84C_TGID_Z_EN                                          0xFFFFFDFF
#define   S_00B84C_TG_SIZE_EN(x)                                      (((x) & 0x1) << 10)
#define   G_00B84C_TG_SIZE_EN(x)                                      (((x) >> 10) & 0x1)
#define   C_00B84C_TG_SIZE_EN                                         0xFFFFFBFF
#define   S_00B84C_TIDIG_COMP_CNT(x)                                  (((x) & 0x03) << 11)
#define   G_00B84C_TIDIG_COMP_CNT(x)                                  (((x) >> 11) & 0x03)
#define   C_00B84C_TIDIG_COMP_CNT                                     0xFFFFE7FF
#define   S_00B84C_LDS_SIZE(x)                                        (((x) & 0x1FF) << 15)
#define   G_00B84C_LDS_SIZE(x)                                        (((x) >> 15) & 0x1FF)
#define   C_00B84C_LDS_SIZE                                           0xFF007FFF
#define   S_00B84C_EXCP_EN(x)                                         (((x) & 0x7F) << 24)
#define   G_00B84C_EXCP_EN(x)                                         (((x) >> 24) & 0x7F)
#define   C_00B84C_EXCP_EN                                            0x80FFFFFF
#define R_00B854_COMPUTE_RESOURCE_LIMITS                                0x00B854
#define   S_00B854_WAVES_PER_SH(x)                                    (((x) & 0x3F) << 0)
#define   G_00B854_WAVES_PER_SH(x)                                    (((x) >> 0) & 0x3F)
#define   C_00B854_WAVES_PER_SH                                       0xFFFFFFC0
#define   S_00B854_TG_PER_CU(x)                                       (((x) & 0x0F) << 12)
#define   G_00B854_TG_PER_CU(x)                                       (((x) >> 12) & 0x0F)
#define   C_00B854_TG_PER_CU                                          0xFFFF0FFF
#define   S_00B854_LOCK_THRESHOLD(x)                                  (((x) & 0x3F) << 16)
#define   G_00B854_LOCK_THRESHOLD(x)                                  (((x) >> 16) & 0x3F)
#define   C_00B854_LOCK_THRESHOLD                                     0xFFC0FFFF
#define   S_00B854_SIMD_DEST_CNTL(x)                                  (((x) & 0x1) << 22)
#define   G_00B854_SIMD_DEST_CNTL(x)                                  (((x) >> 22) & 0x1)
#define   C_00B854_SIMD_DEST_CNTL                                     0xFFBFFFFF
#define R_00B858_COMPUTE_STATIC_THREAD_MGMT_SE0                         0x00B858
#define   S_00B858_SH0_CU_EN(x)                                       (((x) & 0xFFFF) << 0)
#define   G_00B858_SH0_CU_EN(x)                                       (((x) >> 0) & 0xFFFF)
#define   C_00B858_SH0_CU_EN                                          0xFFFF0000
#define   S_00B858_SH1_CU_EN(x)                                       (((x) & 0xFFFF) << 16)
#define   G_00B858_SH1_CU_EN(x)                                       (((x) >> 16) & 0xFFFF)
#define   C_00B858_SH1_CU_EN                                          0x0000FFFF
#define R_00B85C_COMPUTE_STATIC_THREAD_MGMT_SE1                         0x00B85C
#define   S_00B85C_SH0_CU_EN(x)                                       (((x) & 0xFFFF) << 0)
#define   G_00B85C_SH0_CU_EN(x)                                       (((x) >> 0) & 0xFFFF)
#define   C_00B85C_SH0_CU_EN                                          0xFFFF0000
#define   S_00B85C_SH1_CU_EN(x)                                       (((x) & 0xFFFF) << 16)
#define   G_00B85C_SH1_CU_EN(x)                                       (((x) >> 16) & 0xFFFF)
#define   C_00B85C_SH1_CU_EN                                          0x0000FFFF
#define R_00B860_COMPUTE_TMPRING_SIZE                                   0x00B860
#define   S_00B860_WAVES(x)                                           (((x) & 0xFFF) << 0)
#define   G_00B860_WAVES(x)                                           (((x) >> 0) & 0xFFF)
#define   C_00B860_WAVES                                              0xFFFFF000
#define   S_00B860_WAVESIZE(x)                                        (((x) & 0x1FFF) << 12)
#define   G_00B860_WAVESIZE(x)                                        (((x) >> 12) & 0x1FFF)
#define   C_00B860_WAVESIZE                                           0xFE000FFF
#define R_00B900_COMPUTE_USER_DATA_0                                    0x00B900
#define R_028000_DB_RENDER_CONTROL                                      0x028000
#define   S_028000_DEPTH_CLEAR_ENABLE(x)                              (((x) & 0x1) << 0)
#define   G_028000_DEPTH_CLEAR_ENABLE(x)                              (((x) >> 0) & 0x1)
#define   C_028000_DEPTH_CLEAR_ENABLE                                 0xFFFFFFFE
#define   S_028000_STENCIL_CLEAR_ENABLE(x)                            (((x) & 0x1) << 1)
#define   G_028000_STENCIL_CLEAR_ENABLE(x)                            (((x) >> 1) & 0x1)
#define   C_028000_STENCIL_CLEAR_ENABLE                               0xFFFFFFFD
#define   S_028000_DEPTH_COPY(x)                                      (((x) & 0x1) << 2)
#define   G_028000_DEPTH_COPY(x)                                      (((x) >> 2) & 0x1)
#define   C_028000_DEPTH_COPY                                         0xFFFFFFFB
#define   S_028000_STENCIL_COPY(x)                                    (((x) & 0x1) << 3)
#define   G_028000_STENCIL_COPY(x)                                    (((x) >> 3) & 0x1)
#define   C_028000_STENCIL_COPY                                       0xFFFFFFF7
#define   S_028000_RESUMMARIZE_ENABLE(x)                              (((x) & 0x1) << 4)
#define   G_028000_RESUMMARIZE_ENABLE(x)                              (((x) >> 4) & 0x1)
#define   C_028000_RESUMMARIZE_ENABLE                                 0xFFFFFFEF
#define   S_028000_STENCIL_COMPRESS_DISABLE(x)                        (((x) & 0x1) << 5)
#define   G_028000_STENCIL_COMPRESS_DISABLE(x)                        (((x) >> 5) & 0x1)
#define   C_028000_STENCIL_COMPRESS_DISABLE                           0xFFFFFFDF
#define   S_028000_DEPTH_COMPRESS_DISABLE(x)                          (((x) & 0x1) << 6)
#define   G_028000_DEPTH_COMPRESS_DISABLE(x)                          (((x) >> 6) & 0x1)
#define   C_028000_DEPTH_COMPRESS_DISABLE                             0xFFFFFFBF
#define   S_028000_COPY_CENTROID(x)                                   (((x) & 0x1) << 7)
#define   G_028000_COPY_CENTROID(x)                                   (((x) >> 7) & 0x1)
#define   C_028000_COPY_CENTROID                                      0xFFFFFF7F
#define   S_028000_COPY_SAMPLE(x)                                     (((x) & 0x0F) << 8)
#define   G_028000_COPY_SAMPLE(x)                                     (((x) >> 8) & 0x0F)
#define   C_028000_COPY_SAMPLE                                        0xFFFFF0FF
#define R_028004_DB_COUNT_CONTROL                                       0x028004
#define   S_028004_ZPASS_INCREMENT_DISABLE(x)                         (((x) & 0x1) << 0)
#define   G_028004_ZPASS_INCREMENT_DISABLE(x)                         (((x) >> 0) & 0x1)
#define   C_028004_ZPASS_INCREMENT_DISABLE                            0xFFFFFFFE
#define   S_028004_PERFECT_ZPASS_COUNTS(x)                            (((x) & 0x1) << 1)
#define   G_028004_PERFECT_ZPASS_COUNTS(x)                            (((x) >> 1) & 0x1)
#define   C_028004_PERFECT_ZPASS_COUNTS                               0xFFFFFFFD
#define   S_028004_SAMPLE_RATE(x)                                     (((x) & 0x07) << 4)
#define   G_028004_SAMPLE_RATE(x)                                     (((x) >> 4) & 0x07)
#define   C_028004_SAMPLE_RATE                                        0xFFFFFF8F
#define R_028008_DB_DEPTH_VIEW                                          0x028008
#define   S_028008_SLICE_START(x)                                     (((x) & 0x7FF) << 0)
#define   G_028008_SLICE_START(x)                                     (((x) >> 0) & 0x7FF)
#define   C_028008_SLICE_START                                        0xFFFFF800
#define   S_028008_SLICE_MAX(x)                                       (((x) & 0x7FF) << 13)
#define   G_028008_SLICE_MAX(x)                                       (((x) >> 13) & 0x7FF)
#define   C_028008_SLICE_MAX                                          0xFF001FFF
#define   S_028008_Z_READ_ONLY(x)                                     (((x) & 0x1) << 24)
#define   G_028008_Z_READ_ONLY(x)                                     (((x) >> 24) & 0x1)
#define   C_028008_Z_READ_ONLY                                        0xFEFFFFFF
#define   S_028008_STENCIL_READ_ONLY(x)                               (((x) & 0x1) << 25)
#define   G_028008_STENCIL_READ_ONLY(x)                               (((x) >> 25) & 0x1)
#define   C_028008_STENCIL_READ_ONLY                                  0xFDFFFFFF
#define R_02800C_DB_RENDER_OVERRIDE                                     0x02800C
#define   S_02800C_FORCE_HIZ_ENABLE(x)                                (((x) & 0x03) << 0)
#define   G_02800C_FORCE_HIZ_ENABLE(x)                                (((x) >> 0) & 0x03)
#define   C_02800C_FORCE_HIZ_ENABLE                                   0xFFFFFFFC
#define     V_02800C_FORCE_OFF                                      0x00
#define     V_02800C_FORCE_ENABLE                                   0x01
#define     V_02800C_FORCE_DISABLE                                  0x02
#define     V_02800C_FORCE_RESERVED                                 0x03
#define   S_02800C_FORCE_HIS_ENABLE0(x)                               (((x) & 0x03) << 2)
#define   G_02800C_FORCE_HIS_ENABLE0(x)                               (((x) >> 2) & 0x03)
#define   C_02800C_FORCE_HIS_ENABLE0                                  0xFFFFFFF3
#define     V_02800C_FORCE_OFF                                      0x00
#define     V_02800C_FORCE_ENABLE                                   0x01
#define     V_02800C_FORCE_DISABLE                                  0x02
#define     V_02800C_FORCE_RESERVED                                 0x03
#define   S_02800C_FORCE_HIS_ENABLE1(x)                               (((x) & 0x03) << 4)
#define   G_02800C_FORCE_HIS_ENABLE1(x)                               (((x) >> 4) & 0x03)
#define   C_02800C_FORCE_HIS_ENABLE1                                  0xFFFFFFCF
#define     V_02800C_FORCE_OFF                                      0x00
#define     V_02800C_FORCE_ENABLE                                   0x01
#define     V_02800C_FORCE_DISABLE                                  0x02
#define     V_02800C_FORCE_RESERVED                                 0x03
#define   S_02800C_FORCE_SHADER_Z_ORDER(x)                            (((x) & 0x1) << 6)
#define   G_02800C_FORCE_SHADER_Z_ORDER(x)                            (((x) >> 6) & 0x1)
#define   C_02800C_FORCE_SHADER_Z_ORDER                               0xFFFFFFBF
#define   S_02800C_FAST_Z_DISABLE(x)                                  (((x) & 0x1) << 7)
#define   G_02800C_FAST_Z_DISABLE(x)                                  (((x) >> 7) & 0x1)
#define   C_02800C_FAST_Z_DISABLE                                     0xFFFFFF7F
#define   S_02800C_FAST_STENCIL_DISABLE(x)                            (((x) & 0x1) << 8)
#define   G_02800C_FAST_STENCIL_DISABLE(x)                            (((x) >> 8) & 0x1)
#define   C_02800C_FAST_STENCIL_DISABLE                               0xFFFFFEFF
#define   S_02800C_NOOP_CULL_DISABLE(x)                               (((x) & 0x1) << 9)
#define   G_02800C_NOOP_CULL_DISABLE(x)                               (((x) >> 9) & 0x1)
#define   C_02800C_NOOP_CULL_DISABLE                                  0xFFFFFDFF
#define   S_02800C_FORCE_COLOR_KILL(x)                                (((x) & 0x1) << 10)
#define   G_02800C_FORCE_COLOR_KILL(x)                                (((x) >> 10) & 0x1)
#define   C_02800C_FORCE_COLOR_KILL                                   0xFFFFFBFF
#define   S_02800C_FORCE_Z_READ(x)                                    (((x) & 0x1) << 11)
#define   G_02800C_FORCE_Z_READ(x)                                    (((x) >> 11) & 0x1)
#define   C_02800C_FORCE_Z_READ                                       0xFFFFF7FF
#define   S_02800C_FORCE_STENCIL_READ(x)                              (((x) & 0x1) << 12)
#define   G_02800C_FORCE_STENCIL_READ(x)                              (((x) >> 12) & 0x1)
#define   C_02800C_FORCE_STENCIL_READ                                 0xFFFFEFFF
#define   S_02800C_FORCE_FULL_Z_RANGE(x)                              (((x) & 0x03) << 13)
#define   G_02800C_FORCE_FULL_Z_RANGE(x)                              (((x) >> 13) & 0x03)
#define   C_02800C_FORCE_FULL_Z_RANGE                                 0xFFFF9FFF
#define     V_02800C_FORCE_OFF                                      0x00
#define     V_02800C_FORCE_ENABLE                                   0x01
#define     V_02800C_FORCE_DISABLE                                  0x02
#define     V_02800C_FORCE_RESERVED                                 0x03
#define   S_02800C_FORCE_QC_SMASK_CONFLICT(x)                         (((x) & 0x1) << 15)
#define   G_02800C_FORCE_QC_SMASK_CONFLICT(x)                         (((x) >> 15) & 0x1)
#define   C_02800C_FORCE_QC_SMASK_CONFLICT                            0xFFFF7FFF
#define   S_02800C_DISABLE_VIEWPORT_CLAMP(x)                          (((x) & 0x1) << 16)
#define   G_02800C_DISABLE_VIEWPORT_CLAMP(x)                          (((x) >> 16) & 0x1)
#define   C_02800C_DISABLE_VIEWPORT_CLAMP                             0xFFFEFFFF
#define   S_02800C_IGNORE_SC_ZRANGE(x)                                (((x) & 0x1) << 17)
#define   G_02800C_IGNORE_SC_ZRANGE(x)                                (((x) >> 17) & 0x1)
#define   C_02800C_IGNORE_SC_ZRANGE                                   0xFFFDFFFF
#define   S_02800C_DISABLE_FULLY_COVERED(x)                           (((x) & 0x1) << 18)
#define   G_02800C_DISABLE_FULLY_COVERED(x)                           (((x) >> 18) & 0x1)
#define   C_02800C_DISABLE_FULLY_COVERED                              0xFFFBFFFF
#define   S_02800C_FORCE_Z_LIMIT_SUMM(x)                              (((x) & 0x03) << 19)
#define   G_02800C_FORCE_Z_LIMIT_SUMM(x)                              (((x) >> 19) & 0x03)
#define   C_02800C_FORCE_Z_LIMIT_SUMM                                 0xFFE7FFFF
#define     V_02800C_FORCE_SUMM_OFF                                 0x00
#define     V_02800C_FORCE_SUMM_MINZ                                0x01
#define     V_02800C_FORCE_SUMM_MAXZ                                0x02
#define     V_02800C_FORCE_SUMM_BOTH                                0x03
#define   S_02800C_MAX_TILES_IN_DTT(x)                                (((x) & 0x1F) << 21)
#define   G_02800C_MAX_TILES_IN_DTT(x)                                (((x) >> 21) & 0x1F)
#define   C_02800C_MAX_TILES_IN_DTT                                   0xFC1FFFFF
#define   S_02800C_DISABLE_TILE_RATE_TILES(x)                         (((x) & 0x1) << 26)
#define   G_02800C_DISABLE_TILE_RATE_TILES(x)                         (((x) >> 26) & 0x1)
#define   C_02800C_DISABLE_TILE_RATE_TILES                            0xFBFFFFFF
#define   S_02800C_FORCE_Z_DIRTY(x)                                   (((x) & 0x1) << 27)
#define   G_02800C_FORCE_Z_DIRTY(x)                                   (((x) >> 27) & 0x1)
#define   C_02800C_FORCE_Z_DIRTY                                      0xF7FFFFFF
#define   S_02800C_FORCE_STENCIL_DIRTY(x)                             (((x) & 0x1) << 28)
#define   G_02800C_FORCE_STENCIL_DIRTY(x)                             (((x) >> 28) & 0x1)
#define   C_02800C_FORCE_STENCIL_DIRTY                                0xEFFFFFFF
#define   S_02800C_FORCE_Z_VALID(x)                                   (((x) & 0x1) << 29)
#define   G_02800C_FORCE_Z_VALID(x)                                   (((x) >> 29) & 0x1)
#define   C_02800C_FORCE_Z_VALID                                      0xDFFFFFFF
#define   S_02800C_FORCE_STENCIL_VALID(x)                             (((x) & 0x1) << 30)
#define   G_02800C_FORCE_STENCIL_VALID(x)                             (((x) >> 30) & 0x1)
#define   C_02800C_FORCE_STENCIL_VALID                                0xBFFFFFFF
#define   S_02800C_PRESERVE_COMPRESSION(x)                            (((x) & 0x1) << 31)
#define   G_02800C_PRESERVE_COMPRESSION(x)                            (((x) >> 31) & 0x1)
#define   C_02800C_PRESERVE_COMPRESSION                               0x7FFFFFFF
#define R_028010_DB_RENDER_OVERRIDE2                                    0x028010
#define   S_028010_PARTIAL_SQUAD_LAUNCH_CONTROL(x)                    (((x) & 0x03) << 0)
#define   G_028010_PARTIAL_SQUAD_LAUNCH_CONTROL(x)                    (((x) >> 0) & 0x03)
#define   C_028010_PARTIAL_SQUAD_LAUNCH_CONTROL                       0xFFFFFFFC
#define     V_028010_PSLC_AUTO                                      0x00
#define     V_028010_PSLC_ON_HANG_ONLY                              0x01
#define     V_028010_PSLC_ASAP                                      0x02
#define     V_028010_PSLC_COUNTDOWN                                 0x03
#define   S_028010_PARTIAL_SQUAD_LAUNCH_COUNTDOWN(x)                  (((x) & 0x07) << 2)
#define   G_028010_PARTIAL_SQUAD_LAUNCH_COUNTDOWN(x)                  (((x) >> 2) & 0x07)
#define   C_028010_PARTIAL_SQUAD_LAUNCH_COUNTDOWN                     0xFFFFFFE3
#define   S_028010_DISABLE_ZMASK_EXPCLEAR_OPTIMIZATIO(x)              (((x) & 0x1) << 5)
#define   G_028010_DISABLE_ZMASK_EXPCLEAR_OPTIMIZATIO(x)              (((x) >> 5) & 0x1)
#define   C_028010_DISABLE_ZMASK_EXPCLEAR_OPTIMIZATIO                 0xFFFFFFDF
#define   S_028010_DISABLE_SMEM_EXPCLEAR_OPTIMIZATION(x)              (((x) & 0x1) << 6)
#define   G_028010_DISABLE_SMEM_EXPCLEAR_OPTIMIZATION(x)              (((x) >> 6) & 0x1)
#define   C_028010_DISABLE_SMEM_EXPCLEAR_OPTIMIZATION                 0xFFFFFFBF
#define   S_028010_DISABLE_COLOR_ON_VALIDATION(x)                     (((x) & 0x1) << 7)
#define   G_028010_DISABLE_COLOR_ON_VALIDATION(x)                     (((x) >> 7) & 0x1)
#define   C_028010_DISABLE_COLOR_ON_VALIDATION                        0xFFFFFF7F
#define   S_028010_DECOMPRESS_Z_ON_FLUSH(x)                           (((x) & 0x1) << 8)
#define   G_028010_DECOMPRESS_Z_ON_FLUSH(x)                           (((x) >> 8) & 0x1)
#define   C_028010_DECOMPRESS_Z_ON_FLUSH                              0xFFFFFEFF
#define   S_028010_DISABLE_REG_SNOOP(x)                               (((x) & 0x1) << 9)
#define   G_028010_DISABLE_REG_SNOOP(x)                               (((x) >> 9) & 0x1)
#define   C_028010_DISABLE_REG_SNOOP                                  0xFFFFFDFF
#define   S_028010_DEPTH_BOUNDS_HIER_DEPTH_DISABLE(x)                 (((x) & 0x1) << 10)
#define   G_028010_DEPTH_BOUNDS_HIER_DEPTH_DISABLE(x)                 (((x) >> 10) & 0x1)
#define   C_028010_DEPTH_BOUNDS_HIER_DEPTH_DISABLE                    0xFFFFFBFF
#define R_028014_DB_HTILE_DATA_BASE                                     0x028014
#define R_028020_DB_DEPTH_BOUNDS_MIN                                    0x028020
#define R_028024_DB_DEPTH_BOUNDS_MAX                                    0x028024
#define R_028028_DB_STENCIL_CLEAR                                       0x028028
#define   S_028028_CLEAR(x)                                           (((x) & 0xFF) << 0)
#define   G_028028_CLEAR(x)                                           (((x) >> 0) & 0xFF)
#define   C_028028_CLEAR                                              0xFFFFFF00
#define R_02802C_DB_DEPTH_CLEAR                                         0x02802C
#define R_028030_PA_SC_SCREEN_SCISSOR_TL                                0x028030
#define   S_028030_TL_X(x)                                            (((x) & 0xFFFF) << 0)
#define   G_028030_TL_X(x)                                            (((x) >> 0) & 0xFFFF)
#define   C_028030_TL_X                                               0xFFFF0000
#define   S_028030_TL_Y(x)                                            (((x) & 0xFFFF) << 16)
#define   G_028030_TL_Y(x)                                            (((x) >> 16) & 0xFFFF)
#define   C_028030_TL_Y                                               0x0000FFFF
#define R_028034_PA_SC_SCREEN_SCISSOR_BR                                0x028034
#define   S_028034_BR_X(x)                                            (((x) & 0xFFFF) << 0)
#define   G_028034_BR_X(x)                                            (((x) >> 0) & 0xFFFF)
#define   C_028034_BR_X                                               0xFFFF0000
#define   S_028034_BR_Y(x)                                            (((x) & 0xFFFF) << 16)
#define   G_028034_BR_Y(x)                                            (((x) >> 16) & 0xFFFF)
#define   C_028034_BR_Y                                               0x0000FFFF
#define R_02803C_DB_DEPTH_INFO                                          0x02803C
#define   S_02803C_ADDR5_SWIZZLE_MASK(x)                              (((x) & 0x0F) << 0)
#define   G_02803C_ADDR5_SWIZZLE_MASK(x)                              (((x) >> 0) & 0x0F)
#define   C_02803C_ADDR5_SWIZZLE_MASK                                 0xFFFFFFF0
#define R_028040_DB_Z_INFO                                              0x028040
#define   S_028040_FORMAT(x)                                          (((x) & 0x03) << 0)
#define   G_028040_FORMAT(x)                                          (((x) >> 0) & 0x03)
#define   C_028040_FORMAT                                             0xFFFFFFFC
#define     V_028040_Z_INVALID                                      0x00
#define     V_028040_Z_16                                           0x01
#define     V_028040_Z_24                                           0x02 /* deprecated */
#define     V_028040_Z_32_FLOAT                                     0x03
#define   S_028040_NUM_SAMPLES(x)                                     (((x) & 0x03) << 2)
#define   G_028040_NUM_SAMPLES(x)                                     (((x) >> 2) & 0x03)
#define   C_028040_NUM_SAMPLES                                        0xFFFFFFF3
#define   S_028040_TILE_MODE_INDEX(x)                                 (((x) & 0x07) << 20)
#define   G_028040_TILE_MODE_INDEX(x)                                 (((x) >> 20) & 0x07)
#define   C_028040_TILE_MODE_INDEX                                    0xFF8FFFFF
#define   S_028040_ALLOW_EXPCLEAR(x)                                  (((x) & 0x1) << 27)
#define   G_028040_ALLOW_EXPCLEAR(x)                                  (((x) >> 27) & 0x1)
#define   C_028040_ALLOW_EXPCLEAR                                     0xF7FFFFFF
#define   S_028040_READ_SIZE(x)                                       (((x) & 0x1) << 28)
#define   G_028040_READ_SIZE(x)                                       (((x) >> 28) & 0x1)
#define   C_028040_READ_SIZE                                          0xEFFFFFFF
#define   S_028040_TILE_SURFACE_ENABLE(x)                             (((x) & 0x1) << 29)
#define   G_028040_TILE_SURFACE_ENABLE(x)                             (((x) >> 29) & 0x1)
#define   C_028040_TILE_SURFACE_ENABLE                                0xDFFFFFFF
#define   S_028040_ZRANGE_PRECISION(x)                                (((x) & 0x1) << 31)
#define   G_028040_ZRANGE_PRECISION(x)                                (((x) >> 31) & 0x1)
#define   C_028040_ZRANGE_PRECISION                                   0x7FFFFFFF
#define R_028044_DB_STENCIL_INFO                                        0x028044
#define   S_028044_FORMAT(x)                                          (((x) & 0x1) << 0)
#define   G_028044_FORMAT(x)                                          (((x) >> 0) & 0x1)
#define   C_028044_FORMAT                                             0xFFFFFFFE
#define   S_028044_TILE_MODE_INDEX(x)                                 (((x) & 0x07) << 20)
#define   G_028044_TILE_MODE_INDEX(x)                                 (((x) >> 20) & 0x07)
#define   C_028044_TILE_MODE_INDEX                                    0xFF8FFFFF
#define   S_028044_ALLOW_EXPCLEAR(x)                                  (((x) & 0x1) << 27)
#define   G_028044_ALLOW_EXPCLEAR(x)                                  (((x) >> 27) & 0x1)
#define   C_028044_ALLOW_EXPCLEAR                                     0xF7FFFFFF
#define   S_028044_TILE_STENCIL_DISABLE(x)                            (((x) & 0x1) << 29)
#define   G_028044_TILE_STENCIL_DISABLE(x)                            (((x) >> 29) & 0x1)
#define   C_028044_TILE_STENCIL_DISABLE                               0xDFFFFFFF
#define R_028048_DB_Z_READ_BASE                                         0x028048
#define R_02804C_DB_STENCIL_READ_BASE                                   0x02804C
#define R_028050_DB_Z_WRITE_BASE                                        0x028050
#define R_028054_DB_STENCIL_WRITE_BASE                                  0x028054
#define R_028058_DB_DEPTH_SIZE                                          0x028058
#define   S_028058_PITCH_TILE_MAX(x)                                  (((x) & 0x7FF) << 0)
#define   G_028058_PITCH_TILE_MAX(x)                                  (((x) >> 0) & 0x7FF)
#define   C_028058_PITCH_TILE_MAX                                     0xFFFFF800
#define   S_028058_HEIGHT_TILE_MAX(x)                                 (((x) & 0x7FF) << 11)
#define   G_028058_HEIGHT_TILE_MAX(x)                                 (((x) >> 11) & 0x7FF)
#define   C_028058_HEIGHT_TILE_MAX                                    0xFFC007FF
#define R_02805C_DB_DEPTH_SLICE                                         0x02805C
#define   S_02805C_SLICE_TILE_MAX(x)                                  (((x) & 0x3FFFFF) << 0)
#define   G_02805C_SLICE_TILE_MAX(x)                                  (((x) >> 0) & 0x3FFFFF)
#define   C_02805C_SLICE_TILE_MAX                                     0xFFC00000
#define R_028080_TA_BC_BASE_ADDR                                        0x028080
#define R_028200_PA_SC_WINDOW_OFFSET                                    0x028200
#define   S_028200_WINDOW_X_OFFSET(x)                                 (((x) & 0xFFFF) << 0)
#define   G_028200_WINDOW_X_OFFSET(x)                                 (((x) >> 0) & 0xFFFF)
#define   C_028200_WINDOW_X_OFFSET                                    0xFFFF0000
#define   S_028200_WINDOW_Y_OFFSET(x)                                 (((x) & 0xFFFF) << 16)
#define   G_028200_WINDOW_Y_OFFSET(x)                                 (((x) >> 16) & 0xFFFF)
#define   C_028200_WINDOW_Y_OFFSET                                    0x0000FFFF
#define R_028204_PA_SC_WINDOW_SCISSOR_TL                                0x028204
#define   S_028204_TL_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028204_TL_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028204_TL_X                                               0xFFFF8000
#define   S_028204_TL_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028204_TL_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028204_TL_Y                                               0x8000FFFF
#define   S_028204_WINDOW_OFFSET_DISABLE(x)                           (((x) & 0x1) << 31)
#define   G_028204_WINDOW_OFFSET_DISABLE(x)                           (((x) >> 31) & 0x1)
#define   C_028204_WINDOW_OFFSET_DISABLE                              0x7FFFFFFF
#define R_028208_PA_SC_WINDOW_SCISSOR_BR                                0x028208
#define   S_028208_BR_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028208_BR_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028208_BR_X                                               0xFFFF8000
#define   S_028208_BR_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028208_BR_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028208_BR_Y                                               0x8000FFFF
#define R_02820C_PA_SC_CLIPRECT_RULE                                    0x02820C
#define   S_02820C_CLIP_RULE(x)                                       (((x) & 0xFFFF) << 0)
#define   G_02820C_CLIP_RULE(x)                                       (((x) >> 0) & 0xFFFF)
#define   C_02820C_CLIP_RULE                                          0xFFFF0000
#define R_028210_PA_SC_CLIPRECT_0_TL                                    0x028210
#define   S_028210_TL_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028210_TL_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028210_TL_X                                               0xFFFF8000
#define   S_028210_TL_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028210_TL_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028210_TL_Y                                               0x8000FFFF
#define R_028214_PA_SC_CLIPRECT_0_BR                                    0x028214
#define   S_028214_BR_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028214_BR_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028214_BR_X                                               0xFFFF8000
#define   S_028214_BR_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028214_BR_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028214_BR_Y                                               0x8000FFFF
#define R_028218_PA_SC_CLIPRECT_1_TL                                    0x028218
#define R_02821C_PA_SC_CLIPRECT_1_BR                                    0x02821C
#define R_028220_PA_SC_CLIPRECT_2_TL                                    0x028220
#define R_028224_PA_SC_CLIPRECT_2_BR                                    0x028224
#define R_028228_PA_SC_CLIPRECT_3_TL                                    0x028228
#define R_02822C_PA_SC_CLIPRECT_3_BR                                    0x02822C
#define R_028230_PA_SC_EDGERULE                                         0x028230
#define   S_028230_ER_TRI(x)                                          (((x) & 0x0F) << 0)
#define   G_028230_ER_TRI(x)                                          (((x) >> 0) & 0x0F)
#define   C_028230_ER_TRI                                             0xFFFFFFF0
#define   S_028230_ER_POINT(x)                                        (((x) & 0x0F) << 4)
#define   G_028230_ER_POINT(x)                                        (((x) >> 4) & 0x0F)
#define   C_028230_ER_POINT                                           0xFFFFFF0F
#define   S_028230_ER_RECT(x)                                         (((x) & 0x0F) << 8)
#define   G_028230_ER_RECT(x)                                         (((x) >> 8) & 0x0F)
#define   C_028230_ER_RECT                                            0xFFFFF0FF
#define   S_028230_ER_LINE_LR(x)                                      (((x) & 0x3F) << 12)
#define   G_028230_ER_LINE_LR(x)                                      (((x) >> 12) & 0x3F)
#define   C_028230_ER_LINE_LR                                         0xFFFC0FFF
#define   S_028230_ER_LINE_RL(x)                                      (((x) & 0x3F) << 18)
#define   G_028230_ER_LINE_RL(x)                                      (((x) >> 18) & 0x3F)
#define   C_028230_ER_LINE_RL                                         0xFF03FFFF
#define   S_028230_ER_LINE_TB(x)                                      (((x) & 0x0F) << 24)
#define   G_028230_ER_LINE_TB(x)                                      (((x) >> 24) & 0x0F)
#define   C_028230_ER_LINE_TB                                         0xF0FFFFFF
#define   S_028230_ER_LINE_BT(x)                                      (((x) & 0x0F) << 28)
#define   G_028230_ER_LINE_BT(x)                                      (((x) >> 28) & 0x0F)
#define   C_028230_ER_LINE_BT                                         0x0FFFFFFF
#define R_028234_PA_SU_HARDWARE_SCREEN_OFFSET                           0x028234
#define   S_028234_HW_SCREEN_OFFSET_X(x)                              (((x) & 0x1FF) << 0)
#define   G_028234_HW_SCREEN_OFFSET_X(x)                              (((x) >> 0) & 0x1FF)
#define   C_028234_HW_SCREEN_OFFSET_X                                 0xFFFFFE00
#define   S_028234_HW_SCREEN_OFFSET_Y(x)                              (((x) & 0x1FF) << 16)
#define   G_028234_HW_SCREEN_OFFSET_Y(x)                              (((x) >> 16) & 0x1FF)
#define   C_028234_HW_SCREEN_OFFSET_Y                                 0xFE00FFFF
#define R_028238_CB_TARGET_MASK                                         0x028238
#define   S_028238_TARGET0_ENABLE(x)                                  (((x) & 0x0F) << 0)
#define   G_028238_TARGET0_ENABLE(x)                                  (((x) >> 0) & 0x0F)
#define   C_028238_TARGET0_ENABLE                                     0xFFFFFFF0
#define   S_028238_TARGET1_ENABLE(x)                                  (((x) & 0x0F) << 4)
#define   G_028238_TARGET1_ENABLE(x)                                  (((x) >> 4) & 0x0F)
#define   C_028238_TARGET1_ENABLE                                     0xFFFFFF0F
#define   S_028238_TARGET2_ENABLE(x)                                  (((x) & 0x0F) << 8)
#define   G_028238_TARGET2_ENABLE(x)                                  (((x) >> 8) & 0x0F)
#define   C_028238_TARGET2_ENABLE                                     0xFFFFF0FF
#define   S_028238_TARGET3_ENABLE(x)                                  (((x) & 0x0F) << 12)
#define   G_028238_TARGET3_ENABLE(x)                                  (((x) >> 12) & 0x0F)
#define   C_028238_TARGET3_ENABLE                                     0xFFFF0FFF
#define   S_028238_TARGET4_ENABLE(x)                                  (((x) & 0x0F) << 16)
#define   G_028238_TARGET4_ENABLE(x)                                  (((x) >> 16) & 0x0F)
#define   C_028238_TARGET4_ENABLE                                     0xFFF0FFFF
#define   S_028238_TARGET5_ENABLE(x)                                  (((x) & 0x0F) << 20)
#define   G_028238_TARGET5_ENABLE(x)                                  (((x) >> 20) & 0x0F)
#define   C_028238_TARGET5_ENABLE                                     0xFF0FFFFF
#define   S_028238_TARGET6_ENABLE(x)                                  (((x) & 0x0F) << 24)
#define   G_028238_TARGET6_ENABLE(x)                                  (((x) >> 24) & 0x0F)
#define   C_028238_TARGET6_ENABLE                                     0xF0FFFFFF
#define   S_028238_TARGET7_ENABLE(x)                                  (((x) & 0x0F) << 28)
#define   G_028238_TARGET7_ENABLE(x)                                  (((x) >> 28) & 0x0F)
#define   C_028238_TARGET7_ENABLE                                     0x0FFFFFFF
#define R_02823C_CB_SHADER_MASK                                         0x02823C
#define   S_02823C_OUTPUT0_ENABLE(x)                                  (((x) & 0x0F) << 0)
#define   G_02823C_OUTPUT0_ENABLE(x)                                  (((x) >> 0) & 0x0F)
#define   C_02823C_OUTPUT0_ENABLE                                     0xFFFFFFF0
#define   S_02823C_OUTPUT1_ENABLE(x)                                  (((x) & 0x0F) << 4)
#define   G_02823C_OUTPUT1_ENABLE(x)                                  (((x) >> 4) & 0x0F)
#define   C_02823C_OUTPUT1_ENABLE                                     0xFFFFFF0F
#define   S_02823C_OUTPUT2_ENABLE(x)                                  (((x) & 0x0F) << 8)
#define   G_02823C_OUTPUT2_ENABLE(x)                                  (((x) >> 8) & 0x0F)
#define   C_02823C_OUTPUT2_ENABLE                                     0xFFFFF0FF
#define   S_02823C_OUTPUT3_ENABLE(x)                                  (((x) & 0x0F) << 12)
#define   G_02823C_OUTPUT3_ENABLE(x)                                  (((x) >> 12) & 0x0F)
#define   C_02823C_OUTPUT3_ENABLE                                     0xFFFF0FFF
#define   S_02823C_OUTPUT4_ENABLE(x)                                  (((x) & 0x0F) << 16)
#define   G_02823C_OUTPUT4_ENABLE(x)                                  (((x) >> 16) & 0x0F)
#define   C_02823C_OUTPUT4_ENABLE                                     0xFFF0FFFF
#define   S_02823C_OUTPUT5_ENABLE(x)                                  (((x) & 0x0F) << 20)
#define   G_02823C_OUTPUT5_ENABLE(x)                                  (((x) >> 20) & 0x0F)
#define   C_02823C_OUTPUT5_ENABLE                                     0xFF0FFFFF
#define   S_02823C_OUTPUT6_ENABLE(x)                                  (((x) & 0x0F) << 24)
#define   G_02823C_OUTPUT6_ENABLE(x)                                  (((x) >> 24) & 0x0F)
#define   C_02823C_OUTPUT6_ENABLE                                     0xF0FFFFFF
#define   S_02823C_OUTPUT7_ENABLE(x)                                  (((x) & 0x0F) << 28)
#define   G_02823C_OUTPUT7_ENABLE(x)                                  (((x) >> 28) & 0x0F)
#define   C_02823C_OUTPUT7_ENABLE                                     0x0FFFFFFF
#define R_028240_PA_SC_GENERIC_SCISSOR_TL                               0x028240
#define   S_028240_TL_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028240_TL_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028240_TL_X                                               0xFFFF8000
#define   S_028240_TL_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028240_TL_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028240_TL_Y                                               0x8000FFFF
#define   S_028240_WINDOW_OFFSET_DISABLE(x)                           (((x) & 0x1) << 31)
#define   G_028240_WINDOW_OFFSET_DISABLE(x)                           (((x) >> 31) & 0x1)
#define   C_028240_WINDOW_OFFSET_DISABLE                              0x7FFFFFFF
#define R_028244_PA_SC_GENERIC_SCISSOR_BR                               0x028244
#define   S_028244_BR_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028244_BR_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028244_BR_X                                               0xFFFF8000
#define   S_028244_BR_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028244_BR_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028244_BR_Y                                               0x8000FFFF
#define R_028250_PA_SC_VPORT_SCISSOR_0_TL                               0x028250
#define   S_028250_TL_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028250_TL_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028250_TL_X                                               0xFFFF8000
#define   S_028250_TL_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028250_TL_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028250_TL_Y                                               0x8000FFFF
#define   S_028250_WINDOW_OFFSET_DISABLE(x)                           (((x) & 0x1) << 31)
#define   G_028250_WINDOW_OFFSET_DISABLE(x)                           (((x) >> 31) & 0x1)
#define   C_028250_WINDOW_OFFSET_DISABLE                              0x7FFFFFFF
#define R_028254_PA_SC_VPORT_SCISSOR_0_BR                               0x028254
#define   S_028254_BR_X(x)                                            (((x) & 0x7FFF) << 0)
#define   G_028254_BR_X(x)                                            (((x) >> 0) & 0x7FFF)
#define   C_028254_BR_X                                               0xFFFF8000
#define   S_028254_BR_Y(x)                                            (((x) & 0x7FFF) << 16)
#define   G_028254_BR_Y(x)                                            (((x) >> 16) & 0x7FFF)
#define   C_028254_BR_Y                                               0x8000FFFF
#define R_0282D0_PA_SC_VPORT_ZMIN_0                                     0x0282D0
#define R_0282D4_PA_SC_VPORT_ZMAX_0                                     0x0282D4
#define R_028350_PA_SC_RASTER_CONFIG                                    0x028350
#define   S_028350_RB_MAP_PKR0(x)                                     (((x) & 0x03) << 0)
#define   G_028350_RB_MAP_PKR0(x)                                     (((x) >> 0) & 0x03)
#define   C_028350_RB_MAP_PKR0                                        0xFFFFFFFC
#define     V_028350_RASTER_CONFIG_RB_MAP_0                         0x00
#define     V_028350_RASTER_CONFIG_RB_MAP_1                         0x01
#define     V_028350_RASTER_CONFIG_RB_MAP_2                         0x02
#define     V_028350_RASTER_CONFIG_RB_MAP_3                         0x03
#define   S_028350_RB_MAP_PKR1(x)                                     (((x) & 0x03) << 2)
#define   G_028350_RB_MAP_PKR1(x)                                     (((x) >> 2) & 0x03)
#define   C_028350_RB_MAP_PKR1                                        0xFFFFFFF3
#define     V_028350_RASTER_CONFIG_RB_MAP_0                         0x00
#define     V_028350_RASTER_CONFIG_RB_MAP_1                         0x01
#define     V_028350_RASTER_CONFIG_RB_MAP_2                         0x02
#define     V_028350_RASTER_CONFIG_RB_MAP_3                         0x03
#define   S_028350_RB_XSEL2(x)                                        (((x) & 0x03) << 4)
#define   G_028350_RB_XSEL2(x)                                        (((x) >> 4) & 0x03)
#define   C_028350_RB_XSEL2                                           0xFFFFFFCF
#define     V_028350_RASTER_CONFIG_RB_XSEL2_0                       0x00
#define     V_028350_RASTER_CONFIG_RB_XSEL2_1                       0x01
#define     V_028350_RASTER_CONFIG_RB_XSEL2_2                       0x02
#define     V_028350_RASTER_CONFIG_RB_XSEL2_3                       0x03
#define   S_028350_RB_XSEL(x)                                         (((x) & 0x1) << 6)
#define   G_028350_RB_XSEL(x)                                         (((x) >> 6) & 0x1)
#define   C_028350_RB_XSEL                                            0xFFFFFFBF
#define   S_028350_RB_YSEL(x)                                         (((x) & 0x1) << 7)
#define   G_028350_RB_YSEL(x)                                         (((x) >> 7) & 0x1)
#define   C_028350_RB_YSEL                                            0xFFFFFF7F
#define   S_028350_PKR_MAP(x)                                         (((x) & 0x03) << 8)
#define   G_028350_PKR_MAP(x)                                         (((x) >> 8) & 0x03)
#define   C_028350_PKR_MAP                                            0xFFFFFCFF
#define     V_028350_RASTER_CONFIG_PKR_MAP_0                        0x00
#define     V_028350_RASTER_CONFIG_PKR_MAP_1                        0x01
#define     V_028350_RASTER_CONFIG_PKR_MAP_2                        0x02
#define     V_028350_RASTER_CONFIG_PKR_MAP_3                        0x03
#define   S_028350_PKR_XSEL(x)                                        (((x) & 0x03) << 10)
#define   G_028350_PKR_XSEL(x)                                        (((x) >> 10) & 0x03)
#define   C_028350_PKR_XSEL                                           0xFFFFF3FF
#define     V_028350_RASTER_CONFIG_PKR_XSEL_0                       0x00
#define     V_028350_RASTER_CONFIG_PKR_XSEL_1                       0x01
#define     V_028350_RASTER_CONFIG_PKR_XSEL_2                       0x02
#define     V_028350_RASTER_CONFIG_PKR_XSEL_3                       0x03
#define   S_028350_PKR_YSEL(x)                                        (((x) & 0x03) << 12)
#define   G_028350_PKR_YSEL(x)                                        (((x) >> 12) & 0x03)
#define   C_028350_PKR_YSEL                                           0xFFFFCFFF
#define     V_028350_RASTER_CONFIG_PKR_YSEL_0                       0x00
#define     V_028350_RASTER_CONFIG_PKR_YSEL_1                       0x01
#define     V_028350_RASTER_CONFIG_PKR_YSEL_2                       0x02
#define     V_028350_RASTER_CONFIG_PKR_YSEL_3                       0x03
#define   S_028350_SC_MAP(x)                                          (((x) & 0x03) << 16)
#define   G_028350_SC_MAP(x)                                          (((x) >> 16) & 0x03)
#define   C_028350_SC_MAP                                             0xFFFCFFFF
#define     V_028350_RASTER_CONFIG_SC_MAP_0                         0x00
#define     V_028350_RASTER_CONFIG_SC_MAP_1                         0x01
#define     V_028350_RASTER_CONFIG_SC_MAP_2                         0x02
#define     V_028350_RASTER_CONFIG_SC_MAP_3                         0x03
#define   S_028350_SC_XSEL(x)                                         (((x) & 0x03) << 18)
#define   G_028350_SC_XSEL(x)                                         (((x) >> 18) & 0x03)
#define   C_028350_SC_XSEL                                            0xFFF3FFFF
#define     V_028350_RASTER_CONFIG_SC_XSEL_8_WIDE_TILE              0x00
#define     V_028350_RASTER_CONFIG_SC_XSEL_16_WIDE_TILE             0x01
#define     V_028350_RASTER_CONFIG_SC_XSEL_32_WIDE_TILE             0x02
#define     V_028350_RASTER_CONFIG_SC_XSEL_64_WIDE_TILE             0x03
#define   S_028350_SC_YSEL(x)                                         (((x) & 0x03) << 20)
#define   G_028350_SC_YSEL(x)                                         (((x) >> 20) & 0x03)
#define   C_028350_SC_YSEL                                            0xFFCFFFFF
#define     V_028350_RASTER_CONFIG_SC_YSEL_8_WIDE_TILE              0x00
#define     V_028350_RASTER_CONFIG_SC_YSEL_16_WIDE_TILE             0x01
#define     V_028350_RASTER_CONFIG_SC_YSEL_32_WIDE_TILE             0x02
#define     V_028350_RASTER_CONFIG_SC_YSEL_64_WIDE_TILE             0x03
#define   S_028350_SE_MAP(x)                                          (((x) & 0x03) << 24)
#define   G_028350_SE_MAP(x)                                          (((x) >> 24) & 0x03)
#define   C_028350_SE_MAP                                             0xFCFFFFFF
#define     V_028350_RASTER_CONFIG_SE_MAP_0                         0x00
#define     V_028350_RASTER_CONFIG_SE_MAP_1                         0x01
#define     V_028350_RASTER_CONFIG_SE_MAP_2                         0x02
#define     V_028350_RASTER_CONFIG_SE_MAP_3                         0x03
#define   S_028350_SE_XSEL(x)                                         (((x) & 0x03) << 26)
#define   G_028350_SE_XSEL(x)                                         (((x) >> 26) & 0x03)
#define   C_028350_SE_XSEL                                            0xF3FFFFFF
#define     V_028350_RASTER_CONFIG_SE_XSEL_8_WIDE_TILE              0x00
#define     V_028350_RASTER_CONFIG_SE_XSEL_16_WIDE_TILE             0x01
#define     V_028350_RASTER_CONFIG_SE_XSEL_32_WIDE_TILE             0x02
#define     V_028350_RASTER_CONFIG_SE_XSEL_64_WIDE_TILE             0x03
#define   S_028350_SE_YSEL(x)                                         (((x) & 0x03) << 28)
#define   G_028350_SE_YSEL(x)                                         (((x) >> 28) & 0x03)
#define   C_028350_SE_YSEL                                            0xCFFFFFFF
#define     V_028350_RASTER_CONFIG_SE_YSEL_8_WIDE_TILE              0x00
#define     V_028350_RASTER_CONFIG_SE_YSEL_16_WIDE_TILE             0x01
#define     V_028350_RASTER_CONFIG_SE_YSEL_32_WIDE_TILE             0x02
#define     V_028350_RASTER_CONFIG_SE_YSEL_64_WIDE_TILE             0x03
#define R_028400_VGT_MAX_VTX_INDX                                       0x028400
#define R_028404_VGT_MIN_VTX_INDX                                       0x028404
#define R_028408_VGT_INDX_OFFSET                                        0x028408
#define R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX                           0x02840C
#define R_028414_CB_BLEND_RED                                           0x028414
#define R_028418_CB_BLEND_GREEN                                         0x028418
#define R_02841C_CB_BLEND_BLUE                                          0x02841C
#define R_028420_CB_BLEND_ALPHA                                         0x028420
#define R_02842C_DB_STENCIL_CONTROL                                     0x02842C
#define   S_02842C_STENCILFAIL(x)                                     (((x) & 0x0F) << 0)
#define   G_02842C_STENCILFAIL(x)                                     (((x) >> 0) & 0x0F)
#define   C_02842C_STENCILFAIL                                        0xFFFFFFF0
#define     V_02842C_STENCIL_KEEP                                   0x00
#define     V_02842C_STENCIL_ZERO                                   0x01
#define     V_02842C_STENCIL_ONES                                   0x02
#define     V_02842C_STENCIL_REPLACE_TEST                           0x03
#define     V_02842C_STENCIL_REPLACE_OP                             0x04
#define     V_02842C_STENCIL_ADD_CLAMP                              0x05
#define     V_02842C_STENCIL_SUB_CLAMP                              0x06
#define     V_02842C_STENCIL_INVERT                                 0x07
#define     V_02842C_STENCIL_ADD_WRAP                               0x08
#define     V_02842C_STENCIL_SUB_WRAP                               0x09
#define     V_02842C_STENCIL_AND                                    0x0A
#define     V_02842C_STENCIL_OR                                     0x0B
#define     V_02842C_STENCIL_XOR                                    0x0C
#define     V_02842C_STENCIL_NAND                                   0x0D
#define     V_02842C_STENCIL_NOR                                    0x0E
#define     V_02842C_STENCIL_XNOR                                   0x0F
#define   S_02842C_STENCILZPASS(x)                                    (((x) & 0x0F) << 4)
#define   G_02842C_STENCILZPASS(x)                                    (((x) >> 4) & 0x0F)
#define   C_02842C_STENCILZPASS                                       0xFFFFFF0F
#define     V_02842C_STENCIL_KEEP                                   0x00
#define     V_02842C_STENCIL_ZERO                                   0x01
#define     V_02842C_STENCIL_ONES                                   0x02
#define     V_02842C_STENCIL_REPLACE_TEST                           0x03
#define     V_02842C_STENCIL_REPLACE_OP                             0x04
#define     V_02842C_STENCIL_ADD_CLAMP                              0x05
#define     V_02842C_STENCIL_SUB_CLAMP                              0x06
#define     V_02842C_STENCIL_INVERT                                 0x07
#define     V_02842C_STENCIL_ADD_WRAP                               0x08
#define     V_02842C_STENCIL_SUB_WRAP                               0x09
#define     V_02842C_STENCIL_AND                                    0x0A
#define     V_02842C_STENCIL_OR                                     0x0B
#define     V_02842C_STENCIL_XOR                                    0x0C
#define     V_02842C_STENCIL_NAND                                   0x0D
#define     V_02842C_STENCIL_NOR                                    0x0E
#define     V_02842C_STENCIL_XNOR                                   0x0F
#define   S_02842C_STENCILZFAIL(x)                                    (((x) & 0x0F) << 8)
#define   G_02842C_STENCILZFAIL(x)                                    (((x) >> 8) & 0x0F)
#define   C_02842C_STENCILZFAIL                                       0xFFFFF0FF
#define     V_02842C_STENCIL_KEEP                                   0x00
#define     V_02842C_STENCIL_ZERO                                   0x01
#define     V_02842C_STENCIL_ONES                                   0x02
#define     V_02842C_STENCIL_REPLACE_TEST                           0x03
#define     V_02842C_STENCIL_REPLACE_OP                             0x04
#define     V_02842C_STENCIL_ADD_CLAMP                              0x05
#define     V_02842C_STENCIL_SUB_CLAMP                              0x06
#define     V_02842C_STENCIL_INVERT                                 0x07
#define     V_02842C_STENCIL_ADD_WRAP                               0x08
#define     V_02842C_STENCIL_SUB_WRAP                               0x09
#define     V_02842C_STENCIL_AND                                    0x0A
#define     V_02842C_STENCIL_OR                                     0x0B
#define     V_02842C_STENCIL_XOR                                    0x0C
#define     V_02842C_STENCIL_NAND                                   0x0D
#define     V_02842C_STENCIL_NOR                                    0x0E
#define     V_02842C_STENCIL_XNOR                                   0x0F
#define   S_02842C_STENCILFAIL_BF(x)                                  (((x) & 0x0F) << 12)
#define   G_02842C_STENCILFAIL_BF(x)                                  (((x) >> 12) & 0x0F)
#define   C_02842C_STENCILFAIL_BF                                     0xFFFF0FFF
#define     V_02842C_STENCIL_KEEP                                   0x00
#define     V_02842C_STENCIL_ZERO                                   0x01
#define     V_02842C_STENCIL_ONES                                   0x02
#define     V_02842C_STENCIL_REPLACE_TEST                           0x03
#define     V_02842C_STENCIL_REPLACE_OP                             0x04
#define     V_02842C_STENCIL_ADD_CLAMP                              0x05
#define     V_02842C_STENCIL_SUB_CLAMP                              0x06
#define     V_02842C_STENCIL_INVERT                                 0x07
#define     V_02842C_STENCIL_ADD_WRAP                               0x08
#define     V_02842C_STENCIL_SUB_WRAP                               0x09
#define     V_02842C_STENCIL_AND                                    0x0A
#define     V_02842C_STENCIL_OR                                     0x0B
#define     V_02842C_STENCIL_XOR                                    0x0C
#define     V_02842C_STENCIL_NAND                                   0x0D
#define     V_02842C_STENCIL_NOR                                    0x0E
#define     V_02842C_STENCIL_XNOR                                   0x0F
#define   S_02842C_STENCILZPASS_BF(x)                                 (((x) & 0x0F) << 16)
#define   G_02842C_STENCILZPASS_BF(x)                                 (((x) >> 16) & 0x0F)
#define   C_02842C_STENCILZPASS_BF                                    0xFFF0FFFF
#define     V_02842C_STENCIL_KEEP                                   0x00
#define     V_02842C_STENCIL_ZERO                                   0x01
#define     V_02842C_STENCIL_ONES                                   0x02
#define     V_02842C_STENCIL_REPLACE_TEST                           0x03
#define     V_02842C_STENCIL_REPLACE_OP                             0x04
#define     V_02842C_STENCIL_ADD_CLAMP                              0x05
#define     V_02842C_STENCIL_SUB_CLAMP                              0x06
#define     V_02842C_STENCIL_INVERT                                 0x07
#define     V_02842C_STENCIL_ADD_WRAP                               0x08
#define     V_02842C_STENCIL_SUB_WRAP                               0x09
#define     V_02842C_STENCIL_AND                                    0x0A
#define     V_02842C_STENCIL_OR                                     0x0B
#define     V_02842C_STENCIL_XOR                                    0x0C
#define     V_02842C_STENCIL_NAND                                   0x0D
#define     V_02842C_STENCIL_NOR                                    0x0E
#define     V_02842C_STENCIL_XNOR                                   0x0F
#define   S_02842C_STENCILZFAIL_BF(x)                                 (((x) & 0x0F) << 20)
#define   G_02842C_STENCILZFAIL_BF(x)                                 (((x) >> 20) & 0x0F)
#define   C_02842C_STENCILZFAIL_BF                                    0xFF0FFFFF
#define     V_02842C_STENCIL_KEEP                                   0x00
#define     V_02842C_STENCIL_ZERO                                   0x01
#define     V_02842C_STENCIL_ONES                                   0x02
#define     V_02842C_STENCIL_REPLACE_TEST                           0x03
#define     V_02842C_STENCIL_REPLACE_OP                             0x04
#define     V_02842C_STENCIL_ADD_CLAMP                              0x05
#define     V_02842C_STENCIL_SUB_CLAMP                              0x06
#define     V_02842C_STENCIL_INVERT                                 0x07
#define     V_02842C_STENCIL_ADD_WRAP                               0x08
#define     V_02842C_STENCIL_SUB_WRAP                               0x09
#define     V_02842C_STENCIL_AND                                    0x0A
#define     V_02842C_STENCIL_OR                                     0x0B
#define     V_02842C_STENCIL_XOR                                    0x0C
#define     V_02842C_STENCIL_NAND                                   0x0D
#define     V_02842C_STENCIL_NOR                                    0x0E
#define     V_02842C_STENCIL_XNOR                                   0x0F
#define R_028430_DB_STENCILREFMASK                                      0x028430
#define   S_028430_STENCILTESTVAL(x)                                  (((x) & 0xFF) << 0)
#define   G_028430_STENCILTESTVAL(x)                                  (((x) >> 0) & 0xFF)
#define   C_028430_STENCILTESTVAL                                     0xFFFFFF00
#define   S_028430_STENCILMASK(x)                                     (((x) & 0xFF) << 8)
#define   G_028430_STENCILMASK(x)                                     (((x) >> 8) & 0xFF)
#define   C_028430_STENCILMASK                                        0xFFFF00FF
#define   S_028430_STENCILWRITEMASK(x)                                (((x) & 0xFF) << 16)
#define   G_028430_STENCILWRITEMASK(x)                                (((x) >> 16) & 0xFF)
#define   C_028430_STENCILWRITEMASK                                   0xFF00FFFF
#define   S_028430_STENCILOPVAL(x)                                    (((x) & 0xFF) << 24)
#define   G_028430_STENCILOPVAL(x)                                    (((x) >> 24) & 0xFF)
#define   C_028430_STENCILOPVAL                                       0x00FFFFFF
#define R_028434_DB_STENCILREFMASK_BF                                   0x028434
#define   S_028434_STENCILTESTVAL_BF(x)                               (((x) & 0xFF) << 0)
#define   G_028434_STENCILTESTVAL_BF(x)                               (((x) >> 0) & 0xFF)
#define   C_028434_STENCILTESTVAL_BF                                  0xFFFFFF00
#define   S_028434_STENCILMASK_BF(x)                                  (((x) & 0xFF) << 8)
#define   G_028434_STENCILMASK_BF(x)                                  (((x) >> 8) & 0xFF)
#define   C_028434_STENCILMASK_BF                                     0xFFFF00FF
#define   S_028434_STENCILWRITEMASK_BF(x)                             (((x) & 0xFF) << 16)
#define   G_028434_STENCILWRITEMASK_BF(x)                             (((x) >> 16) & 0xFF)
#define   C_028434_STENCILWRITEMASK_BF                                0xFF00FFFF
#define   S_028434_STENCILOPVAL_BF(x)                                 (((x) & 0xFF) << 24)
#define   G_028434_STENCILOPVAL_BF(x)                                 (((x) >> 24) & 0xFF)
#define   C_028434_STENCILOPVAL_BF                                    0x00FFFFFF
#define R_02843C_PA_CL_VPORT_XSCALE_0                                   0x02843C
#define R_028440_PA_CL_VPORT_XOFFSET_0                                  0x028440
#define R_028444_PA_CL_VPORT_YSCALE_0                                   0x028444
#define R_028448_PA_CL_VPORT_YOFFSET_0                                  0x028448
#define R_02844C_PA_CL_VPORT_ZSCALE_0                                   0x02844C
#define R_028450_PA_CL_VPORT_ZOFFSET_0                                  0x028450
#define R_0285BC_PA_CL_UCP_0_X                                          0x0285BC
#define R_0285C0_PA_CL_UCP_0_Y                                          0x0285C0
#define R_0285C4_PA_CL_UCP_0_Z                                          0x0285C4
#define R_0285C8_PA_CL_UCP_0_W                                          0x0285C8
#define R_0285CC_PA_CL_UCP_1_X                                          0x0285CC
#define R_0285D0_PA_CL_UCP_1_Y                                          0x0285D0
#define R_0285D4_PA_CL_UCP_1_Z                                          0x0285D4
#define R_0285D8_PA_CL_UCP_1_W                                          0x0285D8
#define R_0285DC_PA_CL_UCP_2_X                                          0x0285DC
#define R_0285E0_PA_CL_UCP_2_Y                                          0x0285E0
#define R_0285E4_PA_CL_UCP_2_Z                                          0x0285E4
#define R_0285E8_PA_CL_UCP_2_W                                          0x0285E8
#define R_0285EC_PA_CL_UCP_3_X                                          0x0285EC
#define R_0285F0_PA_CL_UCP_3_Y                                          0x0285F0
#define R_0285F4_PA_CL_UCP_3_Z                                          0x0285F4
#define R_0285F8_PA_CL_UCP_3_W                                          0x0285F8
#define R_0285FC_PA_CL_UCP_4_X                                          0x0285FC
#define R_028600_PA_CL_UCP_4_Y                                          0x028600
#define R_028604_PA_CL_UCP_4_Z                                          0x028604
#define R_028608_PA_CL_UCP_4_W                                          0x028608
#define R_02860C_PA_CL_UCP_5_X                                          0x02860C
#define R_028610_PA_CL_UCP_5_Y                                          0x028610
#define R_028614_PA_CL_UCP_5_Z                                          0x028614
#define R_028618_PA_CL_UCP_5_W                                          0x028618
#define R_028644_SPI_PS_INPUT_CNTL_0                                    0x028644
#define   S_028644_OFFSET(x)                                          (((x) & 0x3F) << 0)
#define   G_028644_OFFSET(x)                                          (((x) >> 0) & 0x3F)
#define   C_028644_OFFSET                                             0xFFFFFFC0
#define   S_028644_DEFAULT_VAL(x)                                     (((x) & 0x03) << 8)
#define   G_028644_DEFAULT_VAL(x)                                     (((x) >> 8) & 0x03)
#define   C_028644_DEFAULT_VAL                                        0xFFFFFCFF
#define     V_028644_X_0_0F                                         0x00
#define   S_028644_FLAT_SHADE(x)                                      (((x) & 0x1) << 10)
#define   G_028644_FLAT_SHADE(x)                                      (((x) >> 10) & 0x1)
#define   C_028644_FLAT_SHADE                                         0xFFFFFBFF
#define   S_028644_CYL_WRAP(x)                                        (((x) & 0x0F) << 13)
#define   G_028644_CYL_WRAP(x)                                        (((x) >> 13) & 0x0F)
#define   C_028644_CYL_WRAP                                           0xFFFE1FFF
#define   S_028644_PT_SPRITE_TEX(x)                                   (((x) & 0x1) << 17)
#define   G_028644_PT_SPRITE_TEX(x)                                   (((x) >> 17) & 0x1)
#define   C_028644_PT_SPRITE_TEX                                      0xFFFDFFFF
#define R_028648_SPI_PS_INPUT_CNTL_1                                    0x028648
#define R_02864C_SPI_PS_INPUT_CNTL_2                                    0x02864C
#define R_028650_SPI_PS_INPUT_CNTL_3                                    0x028650
#define R_028654_SPI_PS_INPUT_CNTL_4                                    0x028654
#define R_028658_SPI_PS_INPUT_CNTL_5                                    0x028658
#define R_02865C_SPI_PS_INPUT_CNTL_6                                    0x02865C
#define R_028660_SPI_PS_INPUT_CNTL_7                                    0x028660
#define R_028664_SPI_PS_INPUT_CNTL_8                                    0x028664
#define R_028668_SPI_PS_INPUT_CNTL_9                                    0x028668
#define R_02866C_SPI_PS_INPUT_CNTL_10                                   0x02866C
#define R_028670_SPI_PS_INPUT_CNTL_11                                   0x028670
#define R_028674_SPI_PS_INPUT_CNTL_12                                   0x028674
#define R_028678_SPI_PS_INPUT_CNTL_13                                   0x028678
#define R_02867C_SPI_PS_INPUT_CNTL_14                                   0x02867C
#define R_028680_SPI_PS_INPUT_CNTL_15                                   0x028680
#define R_028684_SPI_PS_INPUT_CNTL_16                                   0x028684
#define R_028688_SPI_PS_INPUT_CNTL_17                                   0x028688
#define R_02868C_SPI_PS_INPUT_CNTL_18                                   0x02868C
#define R_028690_SPI_PS_INPUT_CNTL_19                                   0x028690
#define R_028694_SPI_PS_INPUT_CNTL_20                                   0x028694
#define R_028698_SPI_PS_INPUT_CNTL_21                                   0x028698
#define R_02869C_SPI_PS_INPUT_CNTL_22                                   0x02869C
#define R_0286A0_SPI_PS_INPUT_CNTL_23                                   0x0286A0
#define R_0286A4_SPI_PS_INPUT_CNTL_24                                   0x0286A4
#define R_0286A8_SPI_PS_INPUT_CNTL_25                                   0x0286A8
#define R_0286AC_SPI_PS_INPUT_CNTL_26                                   0x0286AC
#define R_0286B0_SPI_PS_INPUT_CNTL_27                                   0x0286B0
#define R_0286B4_SPI_PS_INPUT_CNTL_28                                   0x0286B4
#define R_0286B8_SPI_PS_INPUT_CNTL_29                                   0x0286B8
#define R_0286BC_SPI_PS_INPUT_CNTL_30                                   0x0286BC
#define R_0286C0_SPI_PS_INPUT_CNTL_31                                   0x0286C0
#define R_0286C4_SPI_VS_OUT_CONFIG                                      0x0286C4
#define   S_0286C4_VS_EXPORT_COUNT(x)                                 (((x) & 0x1F) << 1)
#define   G_0286C4_VS_EXPORT_COUNT(x)                                 (((x) >> 1) & 0x1F)
#define   C_0286C4_VS_EXPORT_COUNT                                    0xFFFFFFC1
#define   S_0286C4_VS_HALF_PACK(x)                                    (((x) & 0x1) << 6)
#define   G_0286C4_VS_HALF_PACK(x)                                    (((x) >> 6) & 0x1)
#define   C_0286C4_VS_HALF_PACK                                       0xFFFFFFBF
#define   S_0286C4_VS_EXPORTS_FOG(x)                                  (((x) & 0x1) << 7)
#define   G_0286C4_VS_EXPORTS_FOG(x)                                  (((x) >> 7) & 0x1)
#define   C_0286C4_VS_EXPORTS_FOG                                     0xFFFFFF7F
#define   S_0286C4_VS_OUT_FOG_VEC_ADDR(x)                             (((x) & 0x1F) << 8)
#define   G_0286C4_VS_OUT_FOG_VEC_ADDR(x)                             (((x) >> 8) & 0x1F)
#define   C_0286C4_VS_OUT_FOG_VEC_ADDR                                0xFFFFE0FF
#define R_0286CC_SPI_PS_INPUT_ENA                                       0x0286CC
#define   S_0286CC_PERSP_SAMPLE_ENA(x)                                (((x) & 0x1) << 0)
#define   G_0286CC_PERSP_SAMPLE_ENA(x)                                (((x) >> 0) & 0x1)
#define   C_0286CC_PERSP_SAMPLE_ENA                                   0xFFFFFFFE
#define   S_0286CC_PERSP_CENTER_ENA(x)                                (((x) & 0x1) << 1)
#define   G_0286CC_PERSP_CENTER_ENA(x)                                (((x) >> 1) & 0x1)
#define   C_0286CC_PERSP_CENTER_ENA                                   0xFFFFFFFD
#define   S_0286CC_PERSP_CENTROID_ENA(x)                              (((x) & 0x1) << 2)
#define   G_0286CC_PERSP_CENTROID_ENA(x)                              (((x) >> 2) & 0x1)
#define   C_0286CC_PERSP_CENTROID_ENA                                 0xFFFFFFFB
#define   S_0286CC_PERSP_PULL_MODEL_ENA(x)                            (((x) & 0x1) << 3)
#define   G_0286CC_PERSP_PULL_MODEL_ENA(x)                            (((x) >> 3) & 0x1)
#define   C_0286CC_PERSP_PULL_MODEL_ENA                               0xFFFFFFF7
#define   S_0286CC_LINEAR_SAMPLE_ENA(x)                               (((x) & 0x1) << 4)
#define   G_0286CC_LINEAR_SAMPLE_ENA(x)                               (((x) >> 4) & 0x1)
#define   C_0286CC_LINEAR_SAMPLE_ENA                                  0xFFFFFFEF
#define   S_0286CC_LINEAR_CENTER_ENA(x)                               (((x) & 0x1) << 5)
#define   G_0286CC_LINEAR_CENTER_ENA(x)                               (((x) >> 5) & 0x1)
#define   C_0286CC_LINEAR_CENTER_ENA                                  0xFFFFFFDF
#define   S_0286CC_LINEAR_CENTROID_ENA(x)                             (((x) & 0x1) << 6)
#define   G_0286CC_LINEAR_CENTROID_ENA(x)                             (((x) >> 6) & 0x1)
#define   C_0286CC_LINEAR_CENTROID_ENA                                0xFFFFFFBF
#define   S_0286CC_LINE_STIPPLE_TEX_ENA(x)                            (((x) & 0x1) << 7)
#define   G_0286CC_LINE_STIPPLE_TEX_ENA(x)                            (((x) >> 7) & 0x1)
#define   C_0286CC_LINE_STIPPLE_TEX_ENA                               0xFFFFFF7F
#define   S_0286CC_POS_X_FLOAT_ENA(x)                                 (((x) & 0x1) << 8)
#define   G_0286CC_POS_X_FLOAT_ENA(x)                                 (((x) >> 8) & 0x1)
#define   C_0286CC_POS_X_FLOAT_ENA                                    0xFFFFFEFF
#define   S_0286CC_POS_Y_FLOAT_ENA(x)                                 (((x) & 0x1) << 9)
#define   G_0286CC_POS_Y_FLOAT_ENA(x)                                 (((x) >> 9) & 0x1)
#define   C_0286CC_POS_Y_FLOAT_ENA                                    0xFFFFFDFF
#define   S_0286CC_POS_Z_FLOAT_ENA(x)                                 (((x) & 0x1) << 10)
#define   G_0286CC_POS_Z_FLOAT_ENA(x)                                 (((x) >> 10) & 0x1)
#define   C_0286CC_POS_Z_FLOAT_ENA                                    0xFFFFFBFF
#define   S_0286CC_POS_W_FLOAT_ENA(x)                                 (((x) & 0x1) << 11)
#define   G_0286CC_POS_W_FLOAT_ENA(x)                                 (((x) >> 11) & 0x1)
#define   C_0286CC_POS_W_FLOAT_ENA                                    0xFFFFF7FF
#define   S_0286CC_FRONT_FACE_ENA(x)                                  (((x) & 0x1) << 12)
#define   G_0286CC_FRONT_FACE_ENA(x)                                  (((x) >> 12) & 0x1)
#define   C_0286CC_FRONT_FACE_ENA                                     0xFFFFEFFF
#define   S_0286CC_ANCILLARY_ENA(x)                                   (((x) & 0x1) << 13)
#define   G_0286CC_ANCILLARY_ENA(x)                                   (((x) >> 13) & 0x1)
#define   C_0286CC_ANCILLARY_ENA                                      0xFFFFDFFF
#define   S_0286CC_SAMPLE_COVERAGE_ENA(x)                             (((x) & 0x1) << 14)
#define   G_0286CC_SAMPLE_COVERAGE_ENA(x)                             (((x) >> 14) & 0x1)
#define   C_0286CC_SAMPLE_COVERAGE_ENA                                0xFFFFBFFF
#define   S_0286CC_POS_FIXED_PT_ENA(x)                                (((x) & 0x1) << 15)
#define   G_0286CC_POS_FIXED_PT_ENA(x)                                (((x) >> 15) & 0x1)
#define   C_0286CC_POS_FIXED_PT_ENA                                   0xFFFF7FFF
#define R_0286D0_SPI_PS_INPUT_ADDR                                      0x0286D0
#define   S_0286D0_PERSP_SAMPLE_ENA(x)                                (((x) & 0x1) << 0)
#define   G_0286D0_PERSP_SAMPLE_ENA(x)                                (((x) >> 0) & 0x1)
#define   C_0286D0_PERSP_SAMPLE_ENA                                   0xFFFFFFFE
#define   S_0286D0_PERSP_CENTER_ENA(x)                                (((x) & 0x1) << 1)
#define   G_0286D0_PERSP_CENTER_ENA(x)                                (((x) >> 1) & 0x1)
#define   C_0286D0_PERSP_CENTER_ENA                                   0xFFFFFFFD
#define   S_0286D0_PERSP_CENTROID_ENA(x)                              (((x) & 0x1) << 2)
#define   G_0286D0_PERSP_CENTROID_ENA(x)                              (((x) >> 2) & 0x1)
#define   C_0286D0_PERSP_CENTROID_ENA                                 0xFFFFFFFB
#define   S_0286D0_PERSP_PULL_MODEL_ENA(x)                            (((x) & 0x1) << 3)
#define   G_0286D0_PERSP_PULL_MODEL_ENA(x)                            (((x) >> 3) & 0x1)
#define   C_0286D0_PERSP_PULL_MODEL_ENA                               0xFFFFFFF7
#define   S_0286D0_LINEAR_SAMPLE_ENA(x)                               (((x) & 0x1) << 4)
#define   G_0286D0_LINEAR_SAMPLE_ENA(x)                               (((x) >> 4) & 0x1)
#define   C_0286D0_LINEAR_SAMPLE_ENA                                  0xFFFFFFEF
#define   S_0286D0_LINEAR_CENTER_ENA(x)                               (((x) & 0x1) << 5)
#define   G_0286D0_LINEAR_CENTER_ENA(x)                               (((x) >> 5) & 0x1)
#define   C_0286D0_LINEAR_CENTER_ENA                                  0xFFFFFFDF
#define   S_0286D0_LINEAR_CENTROID_ENA(x)                             (((x) & 0x1) << 6)
#define   G_0286D0_LINEAR_CENTROID_ENA(x)                             (((x) >> 6) & 0x1)
#define   C_0286D0_LINEAR_CENTROID_ENA                                0xFFFFFFBF
#define   S_0286D0_LINE_STIPPLE_TEX_ENA(x)                            (((x) & 0x1) << 7)
#define   G_0286D0_LINE_STIPPLE_TEX_ENA(x)                            (((x) >> 7) & 0x1)
#define   C_0286D0_LINE_STIPPLE_TEX_ENA                               0xFFFFFF7F
#define   S_0286D0_POS_X_FLOAT_ENA(x)                                 (((x) & 0x1) << 8)
#define   G_0286D0_POS_X_FLOAT_ENA(x)                                 (((x) >> 8) & 0x1)
#define   C_0286D0_POS_X_FLOAT_ENA                                    0xFFFFFEFF
#define   S_0286D0_POS_Y_FLOAT_ENA(x)                                 (((x) & 0x1) << 9)
#define   G_0286D0_POS_Y_FLOAT_ENA(x)                                 (((x) >> 9) & 0x1)
#define   C_0286D0_POS_Y_FLOAT_ENA                                    0xFFFFFDFF
#define   S_0286D0_POS_Z_FLOAT_ENA(x)                                 (((x) & 0x1) << 10)
#define   G_0286D0_POS_Z_FLOAT_ENA(x)                                 (((x) >> 10) & 0x1)
#define   C_0286D0_POS_Z_FLOAT_ENA                                    0xFFFFFBFF
#define   S_0286D0_POS_W_FLOAT_ENA(x)                                 (((x) & 0x1) << 11)
#define   G_0286D0_POS_W_FLOAT_ENA(x)                                 (((x) >> 11) & 0x1)
#define   C_0286D0_POS_W_FLOAT_ENA                                    0xFFFFF7FF
#define   S_0286D0_FRONT_FACE_ENA(x)                                  (((x) & 0x1) << 12)
#define   G_0286D0_FRONT_FACE_ENA(x)                                  (((x) >> 12) & 0x1)
#define   C_0286D0_FRONT_FACE_ENA                                     0xFFFFEFFF
#define   S_0286D0_ANCILLARY_ENA(x)                                   (((x) & 0x1) << 13)
#define   G_0286D0_ANCILLARY_ENA(x)                                   (((x) >> 13) & 0x1)
#define   C_0286D0_ANCILLARY_ENA                                      0xFFFFDFFF
#define   S_0286D0_SAMPLE_COVERAGE_ENA(x)                             (((x) & 0x1) << 14)
#define   G_0286D0_SAMPLE_COVERAGE_ENA(x)                             (((x) >> 14) & 0x1)
#define   C_0286D0_SAMPLE_COVERAGE_ENA                                0xFFFFBFFF
#define   S_0286D0_POS_FIXED_PT_ENA(x)                                (((x) & 0x1) << 15)
#define   G_0286D0_POS_FIXED_PT_ENA(x)                                (((x) >> 15) & 0x1)
#define   C_0286D0_POS_FIXED_PT_ENA                                   0xFFFF7FFF
#define R_0286D4_SPI_INTERP_CONTROL_0                                   0x0286D4
#define   S_0286D4_FLAT_SHADE_ENA(x)                                  (((x) & 0x1) << 0)
#define   G_0286D4_FLAT_SHADE_ENA(x)                                  (((x) >> 0) & 0x1)
#define   C_0286D4_FLAT_SHADE_ENA                                     0xFFFFFFFE
#define   S_0286D4_PNT_SPRITE_ENA(x)                                  (((x) & 0x1) << 1)
#define   G_0286D4_PNT_SPRITE_ENA(x)                                  (((x) >> 1) & 0x1)
#define   C_0286D4_PNT_SPRITE_ENA                                     0xFFFFFFFD
#define   S_0286D4_PNT_SPRITE_OVRD_X(x)                               (((x) & 0x07) << 2)
#define   G_0286D4_PNT_SPRITE_OVRD_X(x)                               (((x) >> 2) & 0x07)
#define   C_0286D4_PNT_SPRITE_OVRD_X                                  0xFFFFFFE3
#define     V_0286D4_SPI_PNT_SPRITE_SEL_0                           0x00
#define     V_0286D4_SPI_PNT_SPRITE_SEL_1                           0x01
#define     V_0286D4_SPI_PNT_SPRITE_SEL_S                           0x02
#define     V_0286D4_SPI_PNT_SPRITE_SEL_T                           0x03
#define     V_0286D4_SPI_PNT_SPRITE_SEL_NONE                        0x04
#define   S_0286D4_PNT_SPRITE_OVRD_Y(x)                               (((x) & 0x07) << 5)
#define   G_0286D4_PNT_SPRITE_OVRD_Y(x)                               (((x) >> 5) & 0x07)
#define   C_0286D4_PNT_SPRITE_OVRD_Y                                  0xFFFFFF1F
#define     V_0286D4_SPI_PNT_SPRITE_SEL_0                           0x00
#define     V_0286D4_SPI_PNT_SPRITE_SEL_1                           0x01
#define     V_0286D4_SPI_PNT_SPRITE_SEL_S                           0x02
#define     V_0286D4_SPI_PNT_SPRITE_SEL_T                           0x03
#define     V_0286D4_SPI_PNT_SPRITE_SEL_NONE                        0x04
#define   S_0286D4_PNT_SPRITE_OVRD_Z(x)                               (((x) & 0x07) << 8)
#define   G_0286D4_PNT_SPRITE_OVRD_Z(x)                               (((x) >> 8) & 0x07)
#define   C_0286D4_PNT_SPRITE_OVRD_Z                                  0xFFFFF8FF
#define     V_0286D4_SPI_PNT_SPRITE_SEL_0                           0x00
#define     V_0286D4_SPI_PNT_SPRITE_SEL_1                           0x01
#define     V_0286D4_SPI_PNT_SPRITE_SEL_S                           0x02
#define     V_0286D4_SPI_PNT_SPRITE_SEL_T                           0x03
#define     V_0286D4_SPI_PNT_SPRITE_SEL_NONE                        0x04
#define   S_0286D4_PNT_SPRITE_OVRD_W(x)                               (((x) & 0x07) << 11)
#define   G_0286D4_PNT_SPRITE_OVRD_W(x)                               (((x) >> 11) & 0x07)
#define   C_0286D4_PNT_SPRITE_OVRD_W                                  0xFFFFC7FF
#define     V_0286D4_SPI_PNT_SPRITE_SEL_0                           0x00
#define     V_0286D4_SPI_PNT_SPRITE_SEL_1                           0x01
#define     V_0286D4_SPI_PNT_SPRITE_SEL_S                           0x02
#define     V_0286D4_SPI_PNT_SPRITE_SEL_T                           0x03
#define     V_0286D4_SPI_PNT_SPRITE_SEL_NONE                        0x04
#define   S_0286D4_PNT_SPRITE_TOP_1(x)                                (((x) & 0x1) << 14)
#define   G_0286D4_PNT_SPRITE_TOP_1(x)                                (((x) >> 14) & 0x1)
#define   C_0286D4_PNT_SPRITE_TOP_1                                   0xFFFFBFFF
#define R_0286D8_SPI_PS_IN_CONTROL                                      0x0286D8
#define   S_0286D8_NUM_INTERP(x)                                      (((x) & 0x3F) << 0)
#define   G_0286D8_NUM_INTERP(x)                                      (((x) >> 0) & 0x3F)
#define   C_0286D8_NUM_INTERP                                         0xFFFFFFC0
#define   S_0286D8_PARAM_GEN(x)                                       (((x) & 0x1) << 6)
#define   G_0286D8_PARAM_GEN(x)                                       (((x) >> 6) & 0x1)
#define   C_0286D8_PARAM_GEN                                          0xFFFFFFBF
#define   S_0286D8_FOG_ADDR(x)                                        (((x) & 0x7F) << 7)
#define   G_0286D8_FOG_ADDR(x)                                        (((x) >> 7) & 0x7F)
#define   C_0286D8_FOG_ADDR                                           0xFFFFC07F
#define   S_0286D8_BC_OPTIMIZE_DISABLE(x)                             (((x) & 0x1) << 14)
#define   G_0286D8_BC_OPTIMIZE_DISABLE(x)                             (((x) >> 14) & 0x1)
#define   C_0286D8_BC_OPTIMIZE_DISABLE                                0xFFFFBFFF
#define   S_0286D8_PASS_FOG_THROUGH_PS(x)                             (((x) & 0x1) << 15)
#define   G_0286D8_PASS_FOG_THROUGH_PS(x)                             (((x) >> 15) & 0x1)
#define   C_0286D8_PASS_FOG_THROUGH_PS                                0xFFFF7FFF
#define R_0286E0_SPI_BARYC_CNTL                                         0x0286E0
#define   S_0286E0_PERSP_CENTER_CNTL(x)                               (((x) & 0x1) << 0)
#define   G_0286E0_PERSP_CENTER_CNTL(x)                               (((x) >> 0) & 0x1)
#define   C_0286E0_PERSP_CENTER_CNTL                                  0xFFFFFFFE
#define   S_0286E0_PERSP_CENTROID_CNTL(x)                             (((x) & 0x1) << 4)
#define   G_0286E0_PERSP_CENTROID_CNTL(x)                             (((x) >> 4) & 0x1)
#define   C_0286E0_PERSP_CENTROID_CNTL                                0xFFFFFFEF
#define   S_0286E0_LINEAR_CENTER_CNTL(x)                              (((x) & 0x1) << 8)
#define   G_0286E0_LINEAR_CENTER_CNTL(x)                              (((x) >> 8) & 0x1)
#define   C_0286E0_LINEAR_CENTER_CNTL                                 0xFFFFFEFF
#define   S_0286E0_LINEAR_CENTROID_CNTL(x)                            (((x) & 0x1) << 12)
#define   G_0286E0_LINEAR_CENTROID_CNTL(x)                            (((x) >> 12) & 0x1)
#define   C_0286E0_LINEAR_CENTROID_CNTL                               0xFFFFEFFF
#define   S_0286E0_POS_FLOAT_LOCATION(x)                              (((x) & 0x03) << 16)
#define   G_0286E0_POS_FLOAT_LOCATION(x)                              (((x) >> 16) & 0x03)
#define   C_0286E0_POS_FLOAT_LOCATION                                 0xFFFCFFFF
#define     V_0286E0_X_CALCULATE_PER_PIXEL_FLOATING_POINT_POSITION_AT 0x00
#define   S_0286E0_POS_FLOAT_ULC(x)                                   (((x) & 0x1) << 20)
#define   G_0286E0_POS_FLOAT_ULC(x)                                   (((x) >> 20) & 0x1)
#define   C_0286E0_POS_FLOAT_ULC                                      0xFFEFFFFF
#define   S_0286E0_FRONT_FACE_ALL_BITS(x)                             (((x) & 0x1) << 24)
#define   G_0286E0_FRONT_FACE_ALL_BITS(x)                             (((x) >> 24) & 0x1)
#define   C_0286E0_FRONT_FACE_ALL_BITS                                0xFEFFFFFF
#define R_0286E8_SPI_TMPRING_SIZE                                       0x0286E8
#define   S_0286E8_WAVES(x)                                           (((x) & 0xFFF) << 0)
#define   G_0286E8_WAVES(x)                                           (((x) >> 0) & 0xFFF)
#define   C_0286E8_WAVES                                              0xFFFFF000
#define   S_0286E8_WAVESIZE(x)                                        (((x) & 0x1FFF) << 12)
#define   G_0286E8_WAVESIZE(x)                                        (((x) >> 12) & 0x1FFF)
#define   C_0286E8_WAVESIZE                                           0xFE000FFF
#define R_028704_SPI_WAVE_MGMT_1                                        0x028704
#define   S_028704_NUM_PS_WAVES(x)                                    (((x) & 0x3F) << 0)
#define   G_028704_NUM_PS_WAVES(x)                                    (((x) >> 0) & 0x3F)
#define   C_028704_NUM_PS_WAVES                                       0xFFFFFFC0
#define   S_028704_NUM_VS_WAVES(x)                                    (((x) & 0x3F) << 6)
#define   G_028704_NUM_VS_WAVES(x)                                    (((x) >> 6) & 0x3F)
#define   C_028704_NUM_VS_WAVES                                       0xFFFFF03F
#define   S_028704_NUM_GS_WAVES(x)                                    (((x) & 0x3F) << 12)
#define   G_028704_NUM_GS_WAVES(x)                                    (((x) >> 12) & 0x3F)
#define   C_028704_NUM_GS_WAVES                                       0xFFFC0FFF
#define   S_028704_NUM_ES_WAVES(x)                                    (((x) & 0x3F) << 18)
#define   G_028704_NUM_ES_WAVES(x)                                    (((x) >> 18) & 0x3F)
#define   C_028704_NUM_ES_WAVES                                       0xFF03FFFF
#define   S_028704_NUM_HS_WAVES(x)                                    (((x) & 0x3F) << 24)
#define   G_028704_NUM_HS_WAVES(x)                                    (((x) >> 24) & 0x3F)
#define   C_028704_NUM_HS_WAVES                                       0xC0FFFFFF
#define R_028708_SPI_WAVE_MGMT_2                                        0x028708
#define   S_028708_NUM_LS_WAVES(x)                                    (((x) & 0x3F) << 0)
#define   G_028708_NUM_LS_WAVES(x)                                    (((x) >> 0) & 0x3F)
#define   C_028708_NUM_LS_WAVES                                       0xFFFFFFC0
#define R_02870C_SPI_SHADER_POS_FORMAT                                  0x02870C
#define   S_02870C_POS0_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 0)
#define   G_02870C_POS0_EXPORT_FORMAT(x)                              (((x) >> 0) & 0x0F)
#define   C_02870C_POS0_EXPORT_FORMAT                                 0xFFFFFFF0
#define     V_02870C_SPI_SHADER_NONE                                0x00
#define     V_02870C_SPI_SHADER_1COMP                               0x01
#define     V_02870C_SPI_SHADER_2COMP                               0x02
#define     V_02870C_SPI_SHADER_4COMPRESS                           0x03
#define     V_02870C_SPI_SHADER_4COMP                               0x04
#define   S_02870C_POS1_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 4)
#define   G_02870C_POS1_EXPORT_FORMAT(x)                              (((x) >> 4) & 0x0F)
#define   C_02870C_POS1_EXPORT_FORMAT                                 0xFFFFFF0F
#define     V_02870C_SPI_SHADER_NONE                                0x00
#define     V_02870C_SPI_SHADER_1COMP                               0x01
#define     V_02870C_SPI_SHADER_2COMP                               0x02
#define     V_02870C_SPI_SHADER_4COMPRESS                           0x03
#define     V_02870C_SPI_SHADER_4COMP                               0x04
#define   S_02870C_POS2_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 8)
#define   G_02870C_POS2_EXPORT_FORMAT(x)                              (((x) >> 8) & 0x0F)
#define   C_02870C_POS2_EXPORT_FORMAT                                 0xFFFFF0FF
#define     V_02870C_SPI_SHADER_NONE                                0x00
#define     V_02870C_SPI_SHADER_1COMP                               0x01
#define     V_02870C_SPI_SHADER_2COMP                               0x02
#define     V_02870C_SPI_SHADER_4COMPRESS                           0x03
#define     V_02870C_SPI_SHADER_4COMP                               0x04
#define   S_02870C_POS3_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 12)
#define   G_02870C_POS3_EXPORT_FORMAT(x)                              (((x) >> 12) & 0x0F)
#define   C_02870C_POS3_EXPORT_FORMAT                                 0xFFFF0FFF
#define     V_02870C_SPI_SHADER_NONE                                0x00
#define     V_02870C_SPI_SHADER_1COMP                               0x01
#define     V_02870C_SPI_SHADER_2COMP                               0x02
#define     V_02870C_SPI_SHADER_4COMPRESS                           0x03
#define     V_02870C_SPI_SHADER_4COMP                               0x04
#define R_028710_SPI_SHADER_Z_FORMAT                                    0x028710
#define   S_028710_Z_EXPORT_FORMAT(x)                                 (((x) & 0x0F) << 0)
#define   G_028710_Z_EXPORT_FORMAT(x)                                 (((x) >> 0) & 0x0F)
#define   C_028710_Z_EXPORT_FORMAT                                    0xFFFFFFF0
#define     V_028710_SPI_SHADER_ZERO                                0x00
#define     V_028710_SPI_SHADER_32_R                                0x01
#define     V_028710_SPI_SHADER_32_GR                               0x02
#define     V_028710_SPI_SHADER_32_AR                               0x03
#define     V_028710_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028710_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028710_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028710_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028710_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028710_SPI_SHADER_32_ABGR                             0x09
#define R_028714_SPI_SHADER_COL_FORMAT                                  0x028714
#define   S_028714_COL0_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 0)
#define   G_028714_COL0_EXPORT_FORMAT(x)                              (((x) >> 0) & 0x0F)
#define   C_028714_COL0_EXPORT_FORMAT                                 0xFFFFFFF0
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define   S_028714_COL1_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 4)
#define   G_028714_COL1_EXPORT_FORMAT(x)                              (((x) >> 4) & 0x0F)
#define   C_028714_COL1_EXPORT_FORMAT                                 0xFFFFFF0F
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define   S_028714_COL2_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 8)
#define   G_028714_COL2_EXPORT_FORMAT(x)                              (((x) >> 8) & 0x0F)
#define   C_028714_COL2_EXPORT_FORMAT                                 0xFFFFF0FF
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define   S_028714_COL3_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 12)
#define   G_028714_COL3_EXPORT_FORMAT(x)                              (((x) >> 12) & 0x0F)
#define   C_028714_COL3_EXPORT_FORMAT                                 0xFFFF0FFF
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define   S_028714_COL4_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 16)
#define   G_028714_COL4_EXPORT_FORMAT(x)                              (((x) >> 16) & 0x0F)
#define   C_028714_COL4_EXPORT_FORMAT                                 0xFFF0FFFF
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define   S_028714_COL5_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 20)
#define   G_028714_COL5_EXPORT_FORMAT(x)                              (((x) >> 20) & 0x0F)
#define   C_028714_COL5_EXPORT_FORMAT                                 0xFF0FFFFF
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define   S_028714_COL6_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 24)
#define   G_028714_COL6_EXPORT_FORMAT(x)                              (((x) >> 24) & 0x0F)
#define   C_028714_COL6_EXPORT_FORMAT                                 0xF0FFFFFF
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define   S_028714_COL7_EXPORT_FORMAT(x)                              (((x) & 0x0F) << 28)
#define   G_028714_COL7_EXPORT_FORMAT(x)                              (((x) >> 28) & 0x0F)
#define   C_028714_COL7_EXPORT_FORMAT                                 0x0FFFFFFF
#define     V_028714_SPI_SHADER_ZERO                                0x00
#define     V_028714_SPI_SHADER_32_R                                0x01
#define     V_028714_SPI_SHADER_32_GR                               0x02
#define     V_028714_SPI_SHADER_32_AR                               0x03
#define     V_028714_SPI_SHADER_FP16_ABGR                           0x04
#define     V_028714_SPI_SHADER_UNORM16_ABGR                        0x05
#define     V_028714_SPI_SHADER_SNORM16_ABGR                        0x06
#define     V_028714_SPI_SHADER_UINT16_ABGR                         0x07
#define     V_028714_SPI_SHADER_SINT16_ABGR                         0x08
#define     V_028714_SPI_SHADER_32_ABGR                             0x09
#define R_028780_CB_BLEND0_CONTROL                                      0x028780
#define   S_028780_COLOR_SRCBLEND(x)                                  (((x) & 0x1F) << 0)
#define   G_028780_COLOR_SRCBLEND(x)                                  (((x) >> 0) & 0x1F)
#define   C_028780_COLOR_SRCBLEND                                     0xFFFFFFE0
#define     V_028780_BLEND_ZERO                                     0x00
#define     V_028780_BLEND_ONE                                      0x01
#define     V_028780_BLEND_SRC_COLOR                                0x02
#define     V_028780_BLEND_ONE_MINUS_SRC_COLOR                      0x03
#define     V_028780_BLEND_SRC_ALPHA                                0x04
#define     V_028780_BLEND_ONE_MINUS_SRC_ALPHA                      0x05
#define     V_028780_BLEND_DST_ALPHA                                0x06
#define     V_028780_BLEND_ONE_MINUS_DST_ALPHA                      0x07
#define     V_028780_BLEND_DST_COLOR                                0x08
#define     V_028780_BLEND_ONE_MINUS_DST_COLOR                      0x09
#define     V_028780_BLEND_SRC_ALPHA_SATURATE                       0x0A
#define     V_028780_BLEND_CONSTANT_COLOR                           0x0D
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_COLOR                 0x0E
#define     V_028780_BLEND_SRC1_COLOR                               0x0F
#define     V_028780_BLEND_INV_SRC1_COLOR                           0x10
#define     V_028780_BLEND_SRC1_ALPHA                               0x11
#define     V_028780_BLEND_INV_SRC1_ALPHA                           0x12
#define     V_028780_BLEND_CONSTANT_ALPHA                           0x13
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_ALPHA                 0x14
#define   S_028780_COLOR_COMB_FCN(x)                                  (((x) & 0x07) << 5)
#define   G_028780_COLOR_COMB_FCN(x)                                  (((x) >> 5) & 0x07)
#define   C_028780_COLOR_COMB_FCN                                     0xFFFFFF1F
#define     V_028780_COMB_DST_PLUS_SRC                              0x00
#define     V_028780_COMB_SRC_MINUS_DST                             0x01
#define     V_028780_COMB_MIN_DST_SRC                               0x02
#define     V_028780_COMB_MAX_DST_SRC                               0x03
#define     V_028780_COMB_DST_MINUS_SRC                             0x04
#define   S_028780_COLOR_DESTBLEND(x)                                 (((x) & 0x1F) << 8)
#define   G_028780_COLOR_DESTBLEND(x)                                 (((x) >> 8) & 0x1F)
#define   C_028780_COLOR_DESTBLEND                                    0xFFFFE0FF
#define     V_028780_BLEND_ZERO                                     0x00
#define     V_028780_BLEND_ONE                                      0x01
#define     V_028780_BLEND_SRC_COLOR                                0x02
#define     V_028780_BLEND_ONE_MINUS_SRC_COLOR                      0x03
#define     V_028780_BLEND_SRC_ALPHA                                0x04
#define     V_028780_BLEND_ONE_MINUS_SRC_ALPHA                      0x05
#define     V_028780_BLEND_DST_ALPHA                                0x06
#define     V_028780_BLEND_ONE_MINUS_DST_ALPHA                      0x07
#define     V_028780_BLEND_DST_COLOR                                0x08
#define     V_028780_BLEND_ONE_MINUS_DST_COLOR                      0x09
#define     V_028780_BLEND_SRC_ALPHA_SATURATE                       0x0A
#define     V_028780_BLEND_CONSTANT_COLOR                           0x0D
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_COLOR                 0x0E
#define     V_028780_BLEND_SRC1_COLOR                               0x0F
#define     V_028780_BLEND_INV_SRC1_COLOR                           0x10
#define     V_028780_BLEND_SRC1_ALPHA                               0x11
#define     V_028780_BLEND_INV_SRC1_ALPHA                           0x12
#define     V_028780_BLEND_CONSTANT_ALPHA                           0x13
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_ALPHA                 0x14
#define   S_028780_ALPHA_SRCBLEND(x)                                  (((x) & 0x1F) << 16)
#define   G_028780_ALPHA_SRCBLEND(x)                                  (((x) >> 16) & 0x1F)
#define   C_028780_ALPHA_SRCBLEND                                     0xFFE0FFFF
#define     V_028780_BLEND_ZERO                                     0x00
#define     V_028780_BLEND_ONE                                      0x01
#define     V_028780_BLEND_SRC_COLOR                                0x02
#define     V_028780_BLEND_ONE_MINUS_SRC_COLOR                      0x03
#define     V_028780_BLEND_SRC_ALPHA                                0x04
#define     V_028780_BLEND_ONE_MINUS_SRC_ALPHA                      0x05
#define     V_028780_BLEND_DST_ALPHA                                0x06
#define     V_028780_BLEND_ONE_MINUS_DST_ALPHA                      0x07
#define     V_028780_BLEND_DST_COLOR                                0x08
#define     V_028780_BLEND_ONE_MINUS_DST_COLOR                      0x09
#define     V_028780_BLEND_SRC_ALPHA_SATURATE                       0x0A
#define     V_028780_BLEND_CONSTANT_COLOR                           0x0D
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_COLOR                 0x0E
#define     V_028780_BLEND_SRC1_COLOR                               0x0F
#define     V_028780_BLEND_INV_SRC1_COLOR                           0x10
#define     V_028780_BLEND_SRC1_ALPHA                               0x11
#define     V_028780_BLEND_INV_SRC1_ALPHA                           0x12
#define     V_028780_BLEND_CONSTANT_ALPHA                           0x13
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_ALPHA                 0x14
#define   S_028780_ALPHA_COMB_FCN(x)                                  (((x) & 0x07) << 21)
#define   G_028780_ALPHA_COMB_FCN(x)                                  (((x) >> 21) & 0x07)
#define   C_028780_ALPHA_COMB_FCN                                     0xFF1FFFFF
#define     V_028780_COMB_DST_PLUS_SRC                              0x00
#define     V_028780_COMB_SRC_MINUS_DST                             0x01
#define     V_028780_COMB_MIN_DST_SRC                               0x02
#define     V_028780_COMB_MAX_DST_SRC                               0x03
#define     V_028780_COMB_DST_MINUS_SRC                             0x04
#define   S_028780_ALPHA_DESTBLEND(x)                                 (((x) & 0x1F) << 24)
#define   G_028780_ALPHA_DESTBLEND(x)                                 (((x) >> 24) & 0x1F)
#define   C_028780_ALPHA_DESTBLEND                                    0xE0FFFFFF
#define     V_028780_BLEND_ZERO                                     0x00
#define     V_028780_BLEND_ONE                                      0x01
#define     V_028780_BLEND_SRC_COLOR                                0x02
#define     V_028780_BLEND_ONE_MINUS_SRC_COLOR                      0x03
#define     V_028780_BLEND_SRC_ALPHA                                0x04
#define     V_028780_BLEND_ONE_MINUS_SRC_ALPHA                      0x05
#define     V_028780_BLEND_DST_ALPHA                                0x06
#define     V_028780_BLEND_ONE_MINUS_DST_ALPHA                      0x07
#define     V_028780_BLEND_DST_COLOR                                0x08
#define     V_028780_BLEND_ONE_MINUS_DST_COLOR                      0x09
#define     V_028780_BLEND_SRC_ALPHA_SATURATE                       0x0A
#define     V_028780_BLEND_CONSTANT_COLOR                           0x0D
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_COLOR                 0x0E
#define     V_028780_BLEND_SRC1_COLOR                               0x0F
#define     V_028780_BLEND_INV_SRC1_COLOR                           0x10
#define     V_028780_BLEND_SRC1_ALPHA                               0x11
#define     V_028780_BLEND_INV_SRC1_ALPHA                           0x12
#define     V_028780_BLEND_CONSTANT_ALPHA                           0x13
#define     V_028780_BLEND_ONE_MINUS_CONSTANT_ALPHA                 0x14
#define   S_028780_SEPARATE_ALPHA_BLEND(x)                            (((x) & 0x1) << 29)
#define   G_028780_SEPARATE_ALPHA_BLEND(x)                            (((x) >> 29) & 0x1)
#define   C_028780_SEPARATE_ALPHA_BLEND                               0xDFFFFFFF
#define   S_028780_ENABLE(x)                                          (((x) & 0x1) << 30)
#define   G_028780_ENABLE(x)                                          (((x) >> 30) & 0x1)
#define   C_028780_ENABLE                                             0xBFFFFFFF
#define   S_028780_DISABLE_ROP3(x)                                    (((x) & 0x1) << 31)
#define   G_028780_DISABLE_ROP3(x)                                    (((x) >> 31) & 0x1)
#define   C_028780_DISABLE_ROP3                                       0x7FFFFFFF
#define R_028784_CB_BLEND1_CONTROL                                      0x028784
#define R_028788_CB_BLEND2_CONTROL                                      0x028788
#define R_02878C_CB_BLEND3_CONTROL                                      0x02878C
#define R_028790_CB_BLEND4_CONTROL                                      0x028790
#define R_028794_CB_BLEND5_CONTROL                                      0x028794
#define R_028798_CB_BLEND6_CONTROL                                      0x028798
#define R_02879C_CB_BLEND7_CONTROL                                      0x02879C
#define R_0287D4_PA_CL_POINT_X_RAD                                      0x0287D4
#define R_0287D8_PA_CL_POINT_Y_RAD                                      0x0287D8
#define R_0287DC_PA_CL_POINT_SIZE                                       0x0287DC
#define R_0287E0_PA_CL_POINT_CULL_RAD                                   0x0287E0
#define R_0287E4_VGT_DMA_BASE_HI                                        0x0287E4
#define   S_0287E4_BASE_ADDR(x)                                       (((x) & 0xFF) << 0)
#define   G_0287E4_BASE_ADDR(x)                                       (((x) >> 0) & 0xFF)
#define   C_0287E4_BASE_ADDR                                          0xFFFFFF00
#define R_0287E8_VGT_DMA_BASE                                           0x0287E8
#define R_0287F0_VGT_DRAW_INITIATOR                                     0x0287F0
#define   S_0287F0_SOURCE_SELECT(x)                                   (((x) & 0x03) << 0)
#define   G_0287F0_SOURCE_SELECT(x)                                   (((x) >> 0) & 0x03)
#define   C_0287F0_SOURCE_SELECT                                      0xFFFFFFFC
#define     V_0287F0_DI_SRC_SEL_DMA                                 0x00
#define     V_0287F0_DI_SRC_SEL_IMMEDIATE                           0x01
#define     V_0287F0_DI_SRC_SEL_AUTO_INDEX                          0x02
#define     V_0287F0_DI_SRC_SEL_RESERVED                            0x03
#define   S_0287F0_MAJOR_MODE(x)                                      (((x) & 0x03) << 2)
#define   G_0287F0_MAJOR_MODE(x)                                      (((x) >> 2) & 0x03)
#define   C_0287F0_MAJOR_MODE                                         0xFFFFFFF3
#define     V_0287F0_DI_MAJOR_MODE_0                                0x00
#define     V_0287F0_DI_MAJOR_MODE_1                                0x01
#define   S_0287F0_NOT_EOP(x)                                         (((x) & 0x1) << 5)
#define   G_0287F0_NOT_EOP(x)                                         (((x) >> 5) & 0x1)
#define   C_0287F0_NOT_EOP                                            0xFFFFFFDF
#define   S_0287F0_USE_OPAQUE(x)                                      (((x) & 0x1) << 6)
#define   G_0287F0_USE_OPAQUE(x)                                      (((x) >> 6) & 0x1)
#define   C_0287F0_USE_OPAQUE                                         0xFFFFFFBF
#define R_0287F4_VGT_IMMED_DATA                                         0x0287F4
#define R_028800_DB_DEPTH_CONTROL                                       0x028800
#define   S_028800_STENCIL_ENABLE(x)                                  (((x) & 0x1) << 0)
#define   G_028800_STENCIL_ENABLE(x)                                  (((x) >> 0) & 0x1)
#define   C_028800_STENCIL_ENABLE                                     0xFFFFFFFE
#define   S_028800_Z_ENABLE(x)                                        (((x) & 0x1) << 1)
#define   G_028800_Z_ENABLE(x)                                        (((x) >> 1) & 0x1)
#define   C_028800_Z_ENABLE                                           0xFFFFFFFD
#define   S_028800_Z_WRITE_ENABLE(x)                                  (((x) & 0x1) << 2)
#define   G_028800_Z_WRITE_ENABLE(x)                                  (((x) >> 2) & 0x1)
#define   C_028800_Z_WRITE_ENABLE                                     0xFFFFFFFB
#define   S_028800_DEPTH_BOUNDS_ENABLE(x)                             (((x) & 0x1) << 3)
#define   G_028800_DEPTH_BOUNDS_ENABLE(x)                             (((x) >> 3) & 0x1)
#define   C_028800_DEPTH_BOUNDS_ENABLE                                0xFFFFFFF7
#define   S_028800_ZFUNC(x)                                           (((x) & 0x07) << 4)
#define   G_028800_ZFUNC(x)                                           (((x) >> 4) & 0x07)
#define   C_028800_ZFUNC                                              0xFFFFFF8F
#define     V_028800_FRAG_NEVER                                     0x00
#define     V_028800_FRAG_LESS                                      0x01
#define     V_028800_FRAG_EQUAL                                     0x02
#define     V_028800_FRAG_LEQUAL                                    0x03
#define     V_028800_FRAG_GREATER                                   0x04
#define     V_028800_FRAG_NOTEQUAL                                  0x05
#define     V_028800_FRAG_GEQUAL                                    0x06
#define     V_028800_FRAG_ALWAYS                                    0x07
#define   S_028800_BACKFACE_ENABLE(x)                                 (((x) & 0x1) << 7)
#define   G_028800_BACKFACE_ENABLE(x)                                 (((x) >> 7) & 0x1)
#define   C_028800_BACKFACE_ENABLE                                    0xFFFFFF7F
#define   S_028800_STENCILFUNC(x)                                     (((x) & 0x07) << 8)
#define   G_028800_STENCILFUNC(x)                                     (((x) >> 8) & 0x07)
#define   C_028800_STENCILFUNC                                        0xFFFFF8FF
#define     V_028800_REF_NEVER                                      0x00
#define     V_028800_REF_LESS                                       0x01
#define     V_028800_REF_EQUAL                                      0x02
#define     V_028800_REF_LEQUAL                                     0x03
#define     V_028800_REF_GREATER                                    0x04
#define     V_028800_REF_NOTEQUAL                                   0x05
#define     V_028800_REF_GEQUAL                                     0x06
#define     V_028800_REF_ALWAYS                                     0x07
#define   S_028800_STENCILFUNC_BF(x)                                  (((x) & 0x07) << 20)
#define   G_028800_STENCILFUNC_BF(x)                                  (((x) >> 20) & 0x07)
#define   C_028800_STENCILFUNC_BF                                     0xFF8FFFFF
#define     V_028800_REF_NEVER                                      0x00
#define     V_028800_REF_LESS                                       0x01
#define     V_028800_REF_EQUAL                                      0x02
#define     V_028800_REF_LEQUAL                                     0x03
#define     V_028800_REF_GREATER                                    0x04
#define     V_028800_REF_NOTEQUAL                                   0x05
#define     V_028800_REF_GEQUAL                                     0x06
#define     V_028800_REF_ALWAYS                                     0x07
#define   S_028800_ENABLE_COLOR_WRITES_ON_DEPTH_FAIL(x)               (((x) & 0x1) << 30)
#define   G_028800_ENABLE_COLOR_WRITES_ON_DEPTH_FAIL(x)               (((x) >> 30) & 0x1)
#define   C_028800_ENABLE_COLOR_WRITES_ON_DEPTH_FAIL                  0xBFFFFFFF
#define   S_028800_DISABLE_COLOR_WRITES_ON_DEPTH_PASS(x)              (((x) & 0x1) << 31)
#define   G_028800_DISABLE_COLOR_WRITES_ON_DEPTH_PASS(x)              (((x) >> 31) & 0x1)
#define   C_028800_DISABLE_COLOR_WRITES_ON_DEPTH_PASS                 0x7FFFFFFF
#define R_028804_DB_EQAA                                                0x028804
#define R_028808_CB_COLOR_CONTROL                                       0x028808
#define   S_028808_DEGAMMA_ENABLE(x)                                  (((x) & 0x1) << 3)
#define   G_028808_DEGAMMA_ENABLE(x)                                  (((x) >> 3) & 0x1)
#define   C_028808_DEGAMMA_ENABLE                                     0xFFFFFFF7
#define   S_028808_MODE(x)                                            (((x) & 0x07) << 4)
#define   G_028808_MODE(x)                                            (((x) >> 4) & 0x07)
#define   C_028808_MODE                                               0xFFFFFF8F
#define     V_028808_CB_DISABLE                                     0x00
#define     V_028808_CB_NORMAL                                      0x01
#define     V_028808_CB_ELIMINATE_FAST_CLEAR                        0x02
#define     V_028808_CB_RESOLVE                                     0x03
#define     V_028808_CB_FMASK_DECOMPRESS                            0x05
#define   S_028808_ROP3(x)                                            (((x) & 0xFF) << 16)
#define   G_028808_ROP3(x)                                            (((x) >> 16) & 0xFF)
#define   C_028808_ROP3                                               0xFF00FFFF
#define     V_028808_X_0X00                                         0x00
#define     V_028808_X_0X05                                         0x05
#define     V_028808_X_0X0A                                         0x0A
#define     V_028808_X_0X0F                                         0x0F
#define     V_028808_X_0X11                                         0x11
#define     V_028808_X_0X22                                         0x22
#define     V_028808_X_0X33                                         0x33
#define     V_028808_X_0X44                                         0x44
#define     V_028808_X_0X50                                         0x50
#define     V_028808_X_0X55                                         0x55
#define     V_028808_X_0X5A                                         0x5A
#define     V_028808_X_0X5F                                         0x5F
#define     V_028808_X_0X66                                         0x66
#define     V_028808_X_0X77                                         0x77
#define     V_028808_X_0X88                                         0x88
#define     V_028808_X_0X99                                         0x99
#define     V_028808_X_0XA0                                         0xA0
#define     V_028808_X_0XA5                                         0xA5
#define     V_028808_X_0XAA                                         0xAA
#define     V_028808_X_0XAF                                         0xAF
#define     V_028808_X_0XBB                                         0xBB
#define     V_028808_X_0XCC                                         0xCC
#define     V_028808_X_0XDD                                         0xDD
#define     V_028808_X_0XEE                                         0xEE
#define     V_028808_X_0XF0                                         0xF0
#define     V_028808_X_0XF5                                         0xF5
#define     V_028808_X_0XFA                                         0xFA
#define     V_028808_X_0XFF                                         0xFF
#define R_02880C_DB_SHADER_CONTROL                                      0x02880C
#define   S_02880C_Z_EXPORT_ENABLE(x)                                 (((x) & 0x1) << 0)
#define   G_02880C_Z_EXPORT_ENABLE(x)                                 (((x) >> 0) & 0x1)
#define   C_02880C_Z_EXPORT_ENABLE                                    0xFFFFFFFE
#define   S_02880C_STENCIL_TEST_VAL_EXPORT_ENAB(x)                    (((x) & 0x1) << 1)
#define   G_02880C_STENCIL_TEST_VAL_EXPORT_ENAB(x)                    (((x) >> 1) & 0x1)
#define   C_02880C_STENCIL_TEST_VAL_EXPORT_ENAB                       0xFFFFFFFD
#define   S_02880C_STENCIL_OP_VAL_EXPORT_ENABLE(x)                    (((x) & 0x1) << 2)
#define   G_02880C_STENCIL_OP_VAL_EXPORT_ENABLE(x)                    (((x) >> 2) & 0x1)
#define   C_02880C_STENCIL_OP_VAL_EXPORT_ENABLE                       0xFFFFFFFB
#define   S_02880C_Z_ORDER(x)                                         (((x) & 0x03) << 4)
#define   G_02880C_Z_ORDER(x)                                         (((x) >> 4) & 0x03)
#define   C_02880C_Z_ORDER                                            0xFFFFFFCF
#define     V_02880C_LATE_Z                                         0x00
#define     V_02880C_EARLY_Z_THEN_LATE_Z                            0x01
#define     V_02880C_RE_Z                                           0x02
#define     V_02880C_EARLY_Z_THEN_RE_Z                              0x03
#define   S_02880C_KILL_ENABLE(x)                                     (((x) & 0x1) << 6)
#define   G_02880C_KILL_ENABLE(x)                                     (((x) >> 6) & 0x1)
#define   C_02880C_KILL_ENABLE                                        0xFFFFFFBF
#define   S_02880C_COVERAGE_TO_MASK_ENABLE(x)                         (((x) & 0x1) << 7)
#define   G_02880C_COVERAGE_TO_MASK_ENABLE(x)                         (((x) >> 7) & 0x1)
#define   C_02880C_COVERAGE_TO_MASK_ENABLE                            0xFFFFFF7F
#define   S_02880C_MASK_EXPORT_ENABLE(x)                              (((x) & 0x1) << 8)
#define   G_02880C_MASK_EXPORT_ENABLE(x)                              (((x) >> 8) & 0x1)
#define   C_02880C_MASK_EXPORT_ENABLE                                 0xFFFFFEFF
#define   S_02880C_EXEC_ON_HIER_FAIL(x)                               (((x) & 0x1) << 9)
#define   G_02880C_EXEC_ON_HIER_FAIL(x)                               (((x) >> 9) & 0x1)
#define   C_02880C_EXEC_ON_HIER_FAIL                                  0xFFFFFDFF
#define   S_02880C_EXEC_ON_NOOP(x)                                    (((x) & 0x1) << 10)
#define   G_02880C_EXEC_ON_NOOP(x)                                    (((x) >> 10) & 0x1)
#define   C_02880C_EXEC_ON_NOOP                                       0xFFFFFBFF
#define   S_02880C_ALPHA_TO_MASK_DISABLE(x)                           (((x) & 0x1) << 11)
#define   G_02880C_ALPHA_TO_MASK_DISABLE(x)                           (((x) >> 11) & 0x1)
#define   C_02880C_ALPHA_TO_MASK_DISABLE                              0xFFFFF7FF
#define   S_02880C_DEPTH_BEFORE_SHADER(x)                             (((x) & 0x1) << 12)
#define   G_02880C_DEPTH_BEFORE_SHADER(x)                             (((x) >> 12) & 0x1)
#define   C_02880C_DEPTH_BEFORE_SHADER                                0xFFFFEFFF
#define R_028810_PA_CL_CLIP_CNTL                                        0x028810
#define   S_028810_UCP_ENA_0(x)                                       (((x) & 0x1) << 0)
#define   G_028810_UCP_ENA_0(x)                                       (((x) >> 0) & 0x1)
#define   C_028810_UCP_ENA_0                                          0xFFFFFFFE
#define   S_028810_UCP_ENA_1(x)                                       (((x) & 0x1) << 1)
#define   G_028810_UCP_ENA_1(x)                                       (((x) >> 1) & 0x1)
#define   C_028810_UCP_ENA_1                                          0xFFFFFFFD
#define   S_028810_UCP_ENA_2(x)                                       (((x) & 0x1) << 2)
#define   G_028810_UCP_ENA_2(x)                                       (((x) >> 2) & 0x1)
#define   C_028810_UCP_ENA_2                                          0xFFFFFFFB
#define   S_028810_UCP_ENA_3(x)                                       (((x) & 0x1) << 3)
#define   G_028810_UCP_ENA_3(x)                                       (((x) >> 3) & 0x1)
#define   C_028810_UCP_ENA_3                                          0xFFFFFFF7
#define   S_028810_UCP_ENA_4(x)                                       (((x) & 0x1) << 4)
#define   G_028810_UCP_ENA_4(x)                                       (((x) >> 4) & 0x1)
#define   C_028810_UCP_ENA_4                                          0xFFFFFFEF
#define   S_028810_UCP_ENA_5(x)                                       (((x) & 0x1) << 5)
#define   G_028810_UCP_ENA_5(x)                                       (((x) >> 5) & 0x1)
#define   C_028810_UCP_ENA_5                                          0xFFFFFFDF
#define   S_028810_PS_UCP_Y_SCALE_NEG(x)                              (((x) & 0x1) << 13)
#define   G_028810_PS_UCP_Y_SCALE_NEG(x)                              (((x) >> 13) & 0x1)
#define   C_028810_PS_UCP_Y_SCALE_NEG                                 0xFFFFDFFF
#define   S_028810_PS_UCP_MODE(x)                                     (((x) & 0x03) << 14)
#define   G_028810_PS_UCP_MODE(x)                                     (((x) >> 14) & 0x03)
#define   C_028810_PS_UCP_MODE                                        0xFFFF3FFF
#define   S_028810_CLIP_DISABLE(x)                                    (((x) & 0x1) << 16)
#define   G_028810_CLIP_DISABLE(x)                                    (((x) >> 16) & 0x1)
#define   C_028810_CLIP_DISABLE                                       0xFFFEFFFF
#define   S_028810_UCP_CULL_ONLY_ENA(x)                               (((x) & 0x1) << 17)
#define   G_028810_UCP_CULL_ONLY_ENA(x)                               (((x) >> 17) & 0x1)
#define   C_028810_UCP_CULL_ONLY_ENA                                  0xFFFDFFFF
#define   S_028810_BOUNDARY_EDGE_FLAG_ENA(x)                          (((x) & 0x1) << 18)
#define   G_028810_BOUNDARY_EDGE_FLAG_ENA(x)                          (((x) >> 18) & 0x1)
#define   C_028810_BOUNDARY_EDGE_FLAG_ENA                             0xFFFBFFFF
#define   S_028810_DX_CLIP_SPACE_DEF(x)                               (((x) & 0x1) << 19)
#define   G_028810_DX_CLIP_SPACE_DEF(x)                               (((x) >> 19) & 0x1)
#define   C_028810_DX_CLIP_SPACE_DEF                                  0xFFF7FFFF
#define   S_028810_DIS_CLIP_ERR_DETECT(x)                             (((x) & 0x1) << 20)
#define   G_028810_DIS_CLIP_ERR_DETECT(x)                             (((x) >> 20) & 0x1)
#define   C_028810_DIS_CLIP_ERR_DETECT                                0xFFEFFFFF
#define   S_028810_VTX_KILL_OR(x)                                     (((x) & 0x1) << 21)
#define   G_028810_VTX_KILL_OR(x)                                     (((x) >> 21) & 0x1)
#define   C_028810_VTX_KILL_OR                                        0xFFDFFFFF
#define   S_028810_DX_RASTERIZATION_KILL(x)                           (((x) & 0x1) << 22)
#define   G_028810_DX_RASTERIZATION_KILL(x)                           (((x) >> 22) & 0x1)
#define   C_028810_DX_RASTERIZATION_KILL                              0xFFBFFFFF
#define   S_028810_DX_LINEAR_ATTR_CLIP_ENA(x)                         (((x) & 0x1) << 24)
#define   G_028810_DX_LINEAR_ATTR_CLIP_ENA(x)                         (((x) >> 24) & 0x1)
#define   C_028810_DX_LINEAR_ATTR_CLIP_ENA                            0xFEFFFFFF
#define   S_028810_VTE_VPORT_PROVOKE_DISABLE(x)                       (((x) & 0x1) << 25)
#define   G_028810_VTE_VPORT_PROVOKE_DISABLE(x)                       (((x) >> 25) & 0x1)
#define   C_028810_VTE_VPORT_PROVOKE_DISABLE                          0xFDFFFFFF
#define   S_028810_ZCLIP_NEAR_DISABLE(x)                              (((x) & 0x1) << 26)
#define   G_028810_ZCLIP_NEAR_DISABLE(x)                              (((x) >> 26) & 0x1)
#define   C_028810_ZCLIP_NEAR_DISABLE                                 0xFBFFFFFF
#define   S_028810_ZCLIP_FAR_DISABLE(x)                               (((x) & 0x1) << 27)
#define   G_028810_ZCLIP_FAR_DISABLE(x)                               (((x) >> 27) & 0x1)
#define   C_028810_ZCLIP_FAR_DISABLE                                  0xF7FFFFFF
#define R_028814_PA_SU_SC_MODE_CNTL                                     0x028814
#define   S_028814_CULL_FRONT(x)                                      (((x) & 0x1) << 0)
#define   G_028814_CULL_FRONT(x)                                      (((x) >> 0) & 0x1)
#define   C_028814_CULL_FRONT                                         0xFFFFFFFE
#define   S_028814_CULL_BACK(x)                                       (((x) & 0x1) << 1)
#define   G_028814_CULL_BACK(x)                                       (((x) >> 1) & 0x1)
#define   C_028814_CULL_BACK                                          0xFFFFFFFD
#define   S_028814_FACE(x)                                            (((x) & 0x1) << 2)
#define   G_028814_FACE(x)                                            (((x) >> 2) & 0x1)
#define   C_028814_FACE                                               0xFFFFFFFB
#define   S_028814_POLY_MODE(x)                                       (((x) & 0x03) << 3)
#define   G_028814_POLY_MODE(x)                                       (((x) >> 3) & 0x03)
#define   C_028814_POLY_MODE                                          0xFFFFFFE7
#define     V_028814_X_DISABLE_POLY_MODE                            0x00
#define     V_028814_X_DUAL_MODE                                    0x01
#define   S_028814_POLYMODE_FRONT_PTYPE(x)                            (((x) & 0x07) << 5)
#define   G_028814_POLYMODE_FRONT_PTYPE(x)                            (((x) >> 5) & 0x07)
#define   C_028814_POLYMODE_FRONT_PTYPE                               0xFFFFFF1F
#define     V_028814_X_DRAW_POINTS                                  0x00
#define     V_028814_X_DRAW_LINES                                   0x01
#define     V_028814_X_DRAW_TRIANGLES                               0x02
#define   S_028814_POLYMODE_BACK_PTYPE(x)                             (((x) & 0x07) << 8)
#define   G_028814_POLYMODE_BACK_PTYPE(x)                             (((x) >> 8) & 0x07)
#define   C_028814_POLYMODE_BACK_PTYPE                                0xFFFFF8FF
#define     V_028814_X_DRAW_POINTS                                  0x00
#define     V_028814_X_DRAW_LINES                                   0x01
#define     V_028814_X_DRAW_TRIANGLES                               0x02
#define   S_028814_POLY_OFFSET_FRONT_ENABLE(x)                        (((x) & 0x1) << 11)
#define   G_028814_POLY_OFFSET_FRONT_ENABLE(x)                        (((x) >> 11) & 0x1)
#define   C_028814_POLY_OFFSET_FRONT_ENABLE                           0xFFFFF7FF
#define   S_028814_POLY_OFFSET_BACK_ENABLE(x)                         (((x) & 0x1) << 12)
#define   G_028814_POLY_OFFSET_BACK_ENABLE(x)                         (((x) >> 12) & 0x1)
#define   C_028814_POLY_OFFSET_BACK_ENABLE                            0xFFFFEFFF
#define   S_028814_POLY_OFFSET_PARA_ENABLE(x)                         (((x) & 0x1) << 13)
#define   G_028814_POLY_OFFSET_PARA_ENABLE(x)                         (((x) >> 13) & 0x1)
#define   C_028814_POLY_OFFSET_PARA_ENABLE                            0xFFFFDFFF
#define   S_028814_VTX_WINDOW_OFFSET_ENABLE(x)                        (((x) & 0x1) << 16)
#define   G_028814_VTX_WINDOW_OFFSET_ENABLE(x)                        (((x) >> 16) & 0x1)
#define   C_028814_VTX_WINDOW_OFFSET_ENABLE                           0xFFFEFFFF
#define   S_028814_PROVOKING_VTX_LAST(x)                              (((x) & 0x1) << 19)
#define   G_028814_PROVOKING_VTX_LAST(x)                              (((x) >> 19) & 0x1)
#define   C_028814_PROVOKING_VTX_LAST                                 0xFFF7FFFF
#define   S_028814_PERSP_CORR_DIS(x)                                  (((x) & 0x1) << 20)
#define   G_028814_PERSP_CORR_DIS(x)                                  (((x) >> 20) & 0x1)
#define   C_028814_PERSP_CORR_DIS                                     0xFFEFFFFF
#define   S_028814_MULTI_PRIM_IB_ENA(x)                               (((x) & 0x1) << 21)
#define   G_028814_MULTI_PRIM_IB_ENA(x)                               (((x) >> 21) & 0x1)
#define   C_028814_MULTI_PRIM_IB_ENA                                  0xFFDFFFFF
#define R_028818_PA_CL_VTE_CNTL                                         0x028818
#define   S_028818_VPORT_X_SCALE_ENA(x)                               (((x) & 0x1) << 0)
#define   G_028818_VPORT_X_SCALE_ENA(x)                               (((x) >> 0) & 0x1)
#define   C_028818_VPORT_X_SCALE_ENA                                  0xFFFFFFFE
#define   S_028818_VPORT_X_OFFSET_ENA(x)                              (((x) & 0x1) << 1)
#define   G_028818_VPORT_X_OFFSET_ENA(x)                              (((x) >> 1) & 0x1)
#define   C_028818_VPORT_X_OFFSET_ENA                                 0xFFFFFFFD
#define   S_028818_VPORT_Y_SCALE_ENA(x)                               (((x) & 0x1) << 2)
#define   G_028818_VPORT_Y_SCALE_ENA(x)                               (((x) >> 2) & 0x1)
#define   C_028818_VPORT_Y_SCALE_ENA                                  0xFFFFFFFB
#define   S_028818_VPORT_Y_OFFSET_ENA(x)                              (((x) & 0x1) << 3)
#define   G_028818_VPORT_Y_OFFSET_ENA(x)                              (((x) >> 3) & 0x1)
#define   C_028818_VPORT_Y_OFFSET_ENA                                 0xFFFFFFF7
#define   S_028818_VPORT_Z_SCALE_ENA(x)                               (((x) & 0x1) << 4)
#define   G_028818_VPORT_Z_SCALE_ENA(x)                               (((x) >> 4) & 0x1)
#define   C_028818_VPORT_Z_SCALE_ENA                                  0xFFFFFFEF
#define   S_028818_VPORT_Z_OFFSET_ENA(x)                              (((x) & 0x1) << 5)
#define   G_028818_VPORT_Z_OFFSET_ENA(x)                              (((x) >> 5) & 0x1)
#define   C_028818_VPORT_Z_OFFSET_ENA                                 0xFFFFFFDF
#define   S_028818_VTX_XY_FMT(x)                                      (((x) & 0x1) << 8)
#define   G_028818_VTX_XY_FMT(x)                                      (((x) >> 8) & 0x1)
#define   C_028818_VTX_XY_FMT                                         0xFFFFFEFF
#define   S_028818_VTX_Z_FMT(x)                                       (((x) & 0x1) << 9)
#define   G_028818_VTX_Z_FMT(x)                                       (((x) >> 9) & 0x1)
#define   C_028818_VTX_Z_FMT                                          0xFFFFFDFF
#define   S_028818_VTX_W0_FMT(x)                                      (((x) & 0x1) << 10)
#define   G_028818_VTX_W0_FMT(x)                                      (((x) >> 10) & 0x1)
#define   C_028818_VTX_W0_FMT                                         0xFFFFFBFF
#define R_02881C_PA_CL_VS_OUT_CNTL                                      0x02881C
#define   S_02881C_CLIP_DIST_ENA_0(x)                                 (((x) & 0x1) << 0)
#define   G_02881C_CLIP_DIST_ENA_0(x)                                 (((x) >> 0) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_0                                    0xFFFFFFFE
#define   S_02881C_CLIP_DIST_ENA_1(x)                                 (((x) & 0x1) << 1)
#define   G_02881C_CLIP_DIST_ENA_1(x)                                 (((x) >> 1) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_1                                    0xFFFFFFFD
#define   S_02881C_CLIP_DIST_ENA_2(x)                                 (((x) & 0x1) << 2)
#define   G_02881C_CLIP_DIST_ENA_2(x)                                 (((x) >> 2) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_2                                    0xFFFFFFFB
#define   S_02881C_CLIP_DIST_ENA_3(x)                                 (((x) & 0x1) << 3)
#define   G_02881C_CLIP_DIST_ENA_3(x)                                 (((x) >> 3) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_3                                    0xFFFFFFF7
#define   S_02881C_CLIP_DIST_ENA_4(x)                                 (((x) & 0x1) << 4)
#define   G_02881C_CLIP_DIST_ENA_4(x)                                 (((x) >> 4) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_4                                    0xFFFFFFEF
#define   S_02881C_CLIP_DIST_ENA_5(x)                                 (((x) & 0x1) << 5)
#define   G_02881C_CLIP_DIST_ENA_5(x)                                 (((x) >> 5) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_5                                    0xFFFFFFDF
#define   S_02881C_CLIP_DIST_ENA_6(x)                                 (((x) & 0x1) << 6)
#define   G_02881C_CLIP_DIST_ENA_6(x)                                 (((x) >> 6) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_6                                    0xFFFFFFBF
#define   S_02881C_CLIP_DIST_ENA_7(x)                                 (((x) & 0x1) << 7)
#define   G_02881C_CLIP_DIST_ENA_7(x)                                 (((x) >> 7) & 0x1)
#define   C_02881C_CLIP_DIST_ENA_7                                    0xFFFFFF7F
#define   S_02881C_CULL_DIST_ENA_0(x)                                 (((x) & 0x1) << 8)
#define   G_02881C_CULL_DIST_ENA_0(x)                                 (((x) >> 8) & 0x1)
#define   C_02881C_CULL_DIST_ENA_0                                    0xFFFFFEFF
#define   S_02881C_CULL_DIST_ENA_1(x)                                 (((x) & 0x1) << 9)
#define   G_02881C_CULL_DIST_ENA_1(x)                                 (((x) >> 9) & 0x1)
#define   C_02881C_CULL_DIST_ENA_1                                    0xFFFFFDFF
#define   S_02881C_CULL_DIST_ENA_2(x)                                 (((x) & 0x1) << 10)
#define   G_02881C_CULL_DIST_ENA_2(x)                                 (((x) >> 10) & 0x1)
#define   C_02881C_CULL_DIST_ENA_2                                    0xFFFFFBFF
#define   S_02881C_CULL_DIST_ENA_3(x)                                 (((x) & 0x1) << 11)
#define   G_02881C_CULL_DIST_ENA_3(x)                                 (((x) >> 11) & 0x1)
#define   C_02881C_CULL_DIST_ENA_3                                    0xFFFFF7FF
#define   S_02881C_CULL_DIST_ENA_4(x)                                 (((x) & 0x1) << 12)
#define   G_02881C_CULL_DIST_ENA_4(x)                                 (((x) >> 12) & 0x1)
#define   C_02881C_CULL_DIST_ENA_4                                    0xFFFFEFFF
#define   S_02881C_CULL_DIST_ENA_5(x)                                 (((x) & 0x1) << 13)
#define   G_02881C_CULL_DIST_ENA_5(x)                                 (((x) >> 13) & 0x1)
#define   C_02881C_CULL_DIST_ENA_5                                    0xFFFFDFFF
#define   S_02881C_CULL_DIST_ENA_6(x)                                 (((x) & 0x1) << 14)
#define   G_02881C_CULL_DIST_ENA_6(x)                                 (((x) >> 14) & 0x1)
#define   C_02881C_CULL_DIST_ENA_6                                    0xFFFFBFFF
#define   S_02881C_CULL_DIST_ENA_7(x)                                 (((x) & 0x1) << 15)
#define   G_02881C_CULL_DIST_ENA_7(x)                                 (((x) >> 15) & 0x1)
#define   C_02881C_CULL_DIST_ENA_7                                    0xFFFF7FFF
#define   S_02881C_USE_VTX_POINT_SIZE(x)                              (((x) & 0x1) << 16)
#define   G_02881C_USE_VTX_POINT_SIZE(x)                              (((x) >> 16) & 0x1)
#define   C_02881C_USE_VTX_POINT_SIZE                                 0xFFFEFFFF
#define   S_02881C_USE_VTX_EDGE_FLAG(x)                               (((x) & 0x1) << 17)
#define   G_02881C_USE_VTX_EDGE_FLAG(x)                               (((x) >> 17) & 0x1)
#define   C_02881C_USE_VTX_EDGE_FLAG                                  0xFFFDFFFF
#define   S_02881C_USE_VTX_RENDER_TARGET_INDX(x)                      (((x) & 0x1) << 18)
#define   G_02881C_USE_VTX_RENDER_TARGET_INDX(x)                      (((x) >> 18) & 0x1)
#define   C_02881C_USE_VTX_RENDER_TARGET_INDX                         0xFFFBFFFF
#define   S_02881C_USE_VTX_VIEWPORT_INDX(x)                           (((x) & 0x1) << 19)
#define   G_02881C_USE_VTX_VIEWPORT_INDX(x)                           (((x) >> 19) & 0x1)
#define   C_02881C_USE_VTX_VIEWPORT_INDX                              0xFFF7FFFF
#define   S_02881C_USE_VTX_KILL_FLAG(x)                               (((x) & 0x1) << 20)
#define   G_02881C_USE_VTX_KILL_FLAG(x)                               (((x) >> 20) & 0x1)
#define   C_02881C_USE_VTX_KILL_FLAG                                  0xFFEFFFFF
#define   S_02881C_VS_OUT_MISC_VEC_ENA(x)                             (((x) & 0x1) << 21)
#define   G_02881C_VS_OUT_MISC_VEC_ENA(x)                             (((x) >> 21) & 0x1)
#define   C_02881C_VS_OUT_MISC_VEC_ENA                                0xFFDFFFFF
#define   S_02881C_VS_OUT_CCDIST0_VEC_ENA(x)                          (((x) & 0x1) << 22)
#define   G_02881C_VS_OUT_CCDIST0_VEC_ENA(x)                          (((x) >> 22) & 0x1)
#define   C_02881C_VS_OUT_CCDIST0_VEC_ENA                             0xFFBFFFFF
#define   S_02881C_VS_OUT_CCDIST1_VEC_ENA(x)                          (((x) & 0x1) << 23)
#define   G_02881C_VS_OUT_CCDIST1_VEC_ENA(x)                          (((x) >> 23) & 0x1)
#define   C_02881C_VS_OUT_CCDIST1_VEC_ENA                             0xFF7FFFFF
#define   S_02881C_VS_OUT_MISC_SIDE_BUS_ENA(x)                        (((x) & 0x1) << 24)
#define   G_02881C_VS_OUT_MISC_SIDE_BUS_ENA(x)                        (((x) >> 24) & 0x1)
#define   C_02881C_VS_OUT_MISC_SIDE_BUS_ENA                           0xFEFFFFFF
#define   S_02881C_USE_VTX_GS_CUT_FLAG(x)                             (((x) & 0x1) << 25)
#define   G_02881C_USE_VTX_GS_CUT_FLAG(x)                             (((x) >> 25) & 0x1)
#define   C_02881C_USE_VTX_GS_CUT_FLAG                                0xFDFFFFFF
#define R_028820_PA_CL_NANINF_CNTL                                      0x028820
#define   S_028820_VTE_XY_INF_DISCARD(x)                              (((x) & 0x1) << 0)
#define   G_028820_VTE_XY_INF_DISCARD(x)                              (((x) >> 0) & 0x1)
#define   C_028820_VTE_XY_INF_DISCARD                                 0xFFFFFFFE
#define   S_028820_VTE_Z_INF_DISCARD(x)                               (((x) & 0x1) << 1)
#define   G_028820_VTE_Z_INF_DISCARD(x)                               (((x) >> 1) & 0x1)
#define   C_028820_VTE_Z_INF_DISCARD                                  0xFFFFFFFD
#define   S_028820_VTE_W_INF_DISCARD(x)                               (((x) & 0x1) << 2)
#define   G_028820_VTE_W_INF_DISCARD(x)                               (((x) >> 2) & 0x1)
#define   C_028820_VTE_W_INF_DISCARD                                  0xFFFFFFFB
#define   S_028820_VTE_0XNANINF_IS_0(x)                               (((x) & 0x1) << 3)
#define   G_028820_VTE_0XNANINF_IS_0(x)                               (((x) >> 3) & 0x1)
#define   C_028820_VTE_0XNANINF_IS_0                                  0xFFFFFFF7
#define   S_028820_VTE_XY_NAN_RETAIN(x)                               (((x) & 0x1) << 4)
#define   G_028820_VTE_XY_NAN_RETAIN(x)                               (((x) >> 4) & 0x1)
#define   C_028820_VTE_XY_NAN_RETAIN                                  0xFFFFFFEF
#define   S_028820_VTE_Z_NAN_RETAIN(x)                                (((x) & 0x1) << 5)
#define   G_028820_VTE_Z_NAN_RETAIN(x)                                (((x) >> 5) & 0x1)
#define   C_028820_VTE_Z_NAN_RETAIN                                   0xFFFFFFDF
#define   S_028820_VTE_W_NAN_RETAIN(x)                                (((x) & 0x1) << 6)
#define   G_028820_VTE_W_NAN_RETAIN(x)                                (((x) >> 6) & 0x1)
#define   C_028820_VTE_W_NAN_RETAIN                                   0xFFFFFFBF
#define   S_028820_VTE_W_RECIP_NAN_IS_0(x)                            (((x) & 0x1) << 7)
#define   G_028820_VTE_W_RECIP_NAN_IS_0(x)                            (((x) >> 7) & 0x1)
#define   C_028820_VTE_W_RECIP_NAN_IS_0                               0xFFFFFF7F
#define   S_028820_VS_XY_NAN_TO_INF(x)                                (((x) & 0x1) << 8)
#define   G_028820_VS_XY_NAN_TO_INF(x)                                (((x) >> 8) & 0x1)
#define   C_028820_VS_XY_NAN_TO_INF                                   0xFFFFFEFF
#define   S_028820_VS_XY_INF_RETAIN(x)                                (((x) & 0x1) << 9)
#define   G_028820_VS_XY_INF_RETAIN(x)                                (((x) >> 9) & 0x1)
#define   C_028820_VS_XY_INF_RETAIN                                   0xFFFFFDFF
#define   S_028820_VS_Z_NAN_TO_INF(x)                                 (((x) & 0x1) << 10)
#define   G_028820_VS_Z_NAN_TO_INF(x)                                 (((x) >> 10) & 0x1)
#define   C_028820_VS_Z_NAN_TO_INF                                    0xFFFFFBFF
#define   S_028820_VS_Z_INF_RETAIN(x)                                 (((x) & 0x1) << 11)
#define   G_028820_VS_Z_INF_RETAIN(x)                                 (((x) >> 11) & 0x1)
#define   C_028820_VS_Z_INF_RETAIN                                    0xFFFFF7FF
#define   S_028820_VS_W_NAN_TO_INF(x)                                 (((x) & 0x1) << 12)
#define   G_028820_VS_W_NAN_TO_INF(x)                                 (((x) >> 12) & 0x1)
#define   C_028820_VS_W_NAN_TO_INF                                    0xFFFFEFFF
#define   S_028820_VS_W_INF_RETAIN(x)                                 (((x) & 0x1) << 13)
#define   G_028820_VS_W_INF_RETAIN(x)                                 (((x) >> 13) & 0x1)
#define   C_028820_VS_W_INF_RETAIN                                    0xFFFFDFFF
#define   S_028820_VS_CLIP_DIST_INF_DISCARD(x)                        (((x) & 0x1) << 14)
#define   G_028820_VS_CLIP_DIST_INF_DISCARD(x)                        (((x) >> 14) & 0x1)
#define   C_028820_VS_CLIP_DIST_INF_DISCARD                           0xFFFFBFFF
#define   S_028820_VTE_NO_OUTPUT_NEG_0(x)                             (((x) & 0x1) << 20)
#define   G_028820_VTE_NO_OUTPUT_NEG_0(x)                             (((x) >> 20) & 0x1)
#define   C_028820_VTE_NO_OUTPUT_NEG_0                                0xFFEFFFFF
#define R_028824_PA_SU_LINE_STIPPLE_CNTL                                0x028824
#define   S_028824_LINE_STIPPLE_RESET(x)                              (((x) & 0x03) << 0)
#define   G_028824_LINE_STIPPLE_RESET(x)                              (((x) >> 0) & 0x03)
#define   C_028824_LINE_STIPPLE_RESET                                 0xFFFFFFFC
#define   S_028824_EXPAND_FULL_LENGTH(x)                              (((x) & 0x1) << 2)
#define   G_028824_EXPAND_FULL_LENGTH(x)                              (((x) >> 2) & 0x1)
#define   C_028824_EXPAND_FULL_LENGTH                                 0xFFFFFFFB
#define   S_028824_FRACTIONAL_ACCUM(x)                                (((x) & 0x1) << 3)
#define   G_028824_FRACTIONAL_ACCUM(x)                                (((x) >> 3) & 0x1)
#define   C_028824_FRACTIONAL_ACCUM                                   0xFFFFFFF7
#define   S_028824_DIAMOND_ADJUST(x)                                  (((x) & 0x1) << 4)
#define   G_028824_DIAMOND_ADJUST(x)                                  (((x) >> 4) & 0x1)
#define   C_028824_DIAMOND_ADJUST                                     0xFFFFFFEF
#define R_028828_PA_SU_LINE_STIPPLE_SCALE                               0x028828
#define R_02882C_PA_SU_PRIM_FILTER_CNTL                                 0x02882C
#define   S_02882C_TRIANGLE_FILTER_DISABLE(x)                         (((x) & 0x1) << 0)
#define   G_02882C_TRIANGLE_FILTER_DISABLE(x)                         (((x) >> 0) & 0x1)
#define   C_02882C_TRIANGLE_FILTER_DISABLE                            0xFFFFFFFE
#define   S_02882C_LINE_FILTER_DISABLE(x)                             (((x) & 0x1) << 1)
#define   G_02882C_LINE_FILTER_DISABLE(x)                             (((x) >> 1) & 0x1)
#define   C_02882C_LINE_FILTER_DISABLE                                0xFFFFFFFD
#define   S_02882C_POINT_FILTER_DISABLE(x)                            (((x) & 0x1) << 2)
#define   G_02882C_POINT_FILTER_DISABLE(x)                            (((x) >> 2) & 0x1)
#define   C_02882C_POINT_FILTER_DISABLE                               0xFFFFFFFB
#define   S_02882C_RECTANGLE_FILTER_DISABLE(x)                        (((x) & 0x1) << 3)
#define   G_02882C_RECTANGLE_FILTER_DISABLE(x)                        (((x) >> 3) & 0x1)
#define   C_02882C_RECTANGLE_FILTER_DISABLE                           0xFFFFFFF7
#define   S_02882C_TRIANGLE_EXPAND_ENA(x)                             (((x) & 0x1) << 4)
#define   G_02882C_TRIANGLE_EXPAND_ENA(x)                             (((x) >> 4) & 0x1)
#define   C_02882C_TRIANGLE_EXPAND_ENA                                0xFFFFFFEF
#define   S_02882C_LINE_EXPAND_ENA(x)                                 (((x) & 0x1) << 5)
#define   G_02882C_LINE_EXPAND_ENA(x)                                 (((x) >> 5) & 0x1)
#define   C_02882C_LINE_EXPAND_ENA                                    0xFFFFFFDF
#define   S_02882C_POINT_EXPAND_ENA(x)                                (((x) & 0x1) << 6)
#define   G_02882C_POINT_EXPAND_ENA(x)                                (((x) >> 6) & 0x1)
#define   C_02882C_POINT_EXPAND_ENA                                   0xFFFFFFBF
#define   S_02882C_RECTANGLE_EXPAND_ENA(x)                            (((x) & 0x1) << 7)
#define   G_02882C_RECTANGLE_EXPAND_ENA(x)                            (((x) >> 7) & 0x1)
#define   C_02882C_RECTANGLE_EXPAND_ENA                               0xFFFFFF7F
#define   S_02882C_PRIM_EXPAND_CONSTANT(x)                            (((x) & 0xFF) << 8)
#define   G_02882C_PRIM_EXPAND_CONSTANT(x)                            (((x) >> 8) & 0xFF)
#define   C_02882C_PRIM_EXPAND_CONSTANT                               0xFFFF00FF
#define R_028A00_PA_SU_POINT_SIZE                                       0x028A00
#define   S_028A00_HEIGHT(x)                                          (((x) & 0xFFFF) << 0)
#define   G_028A00_HEIGHT(x)                                          (((x) >> 0) & 0xFFFF)
#define   C_028A00_HEIGHT                                             0xFFFF0000
#define   S_028A00_WIDTH(x)                                           (((x) & 0xFFFF) << 16)
#define   G_028A00_WIDTH(x)                                           (((x) >> 16) & 0xFFFF)
#define   C_028A00_WIDTH                                              0x0000FFFF
#define R_028A04_PA_SU_POINT_MINMAX                                     0x028A04
#define   S_028A04_MIN_SIZE(x)                                        (((x) & 0xFFFF) << 0)
#define   G_028A04_MIN_SIZE(x)                                        (((x) >> 0) & 0xFFFF)
#define   C_028A04_MIN_SIZE                                           0xFFFF0000
#define   S_028A04_MAX_SIZE(x)                                        (((x) & 0xFFFF) << 16)
#define   G_028A04_MAX_SIZE(x)                                        (((x) >> 16) & 0xFFFF)
#define   C_028A04_MAX_SIZE                                           0x0000FFFF
#define R_028A08_PA_SU_LINE_CNTL                                        0x028A08
#define   S_028A08_WIDTH(x)                                           (((x) & 0xFFFF) << 0)
#define   G_028A08_WIDTH(x)                                           (((x) >> 0) & 0xFFFF)
#define   C_028A08_WIDTH                                              0xFFFF0000
#define R_028A0C_PA_SC_LINE_STIPPLE                                     0x028A0C
#define   S_028A0C_LINE_PATTERN(x)                                    (((x) & 0xFFFF) << 0)
#define   G_028A0C_LINE_PATTERN(x)                                    (((x) >> 0) & 0xFFFF)
#define   C_028A0C_LINE_PATTERN                                       0xFFFF0000
#define   S_028A0C_REPEAT_COUNT(x)                                    (((x) & 0xFF) << 16)
#define   G_028A0C_REPEAT_COUNT(x)                                    (((x) >> 16) & 0xFF)
#define   C_028A0C_REPEAT_COUNT                                       0xFF00FFFF
#define   S_028A0C_PATTERN_BIT_ORDER(x)                               (((x) & 0x1) << 28)
#define   G_028A0C_PATTERN_BIT_ORDER(x)                               (((x) >> 28) & 0x1)
#define   C_028A0C_PATTERN_BIT_ORDER                                  0xEFFFFFFF
#define   S_028A0C_AUTO_RESET_CNTL(x)                                 (((x) & 0x03) << 29)
#define   G_028A0C_AUTO_RESET_CNTL(x)                                 (((x) >> 29) & 0x03)
#define   C_028A0C_AUTO_RESET_CNTL                                    0x9FFFFFFF
#define R_028A10_VGT_OUTPUT_PATH_CNTL                                   0x028A10
#define   S_028A10_PATH_SELECT(x)                                     (((x) & 0x07) << 0)
#define   G_028A10_PATH_SELECT(x)                                     (((x) >> 0) & 0x07)
#define   C_028A10_PATH_SELECT                                        0xFFFFFFF8
#define     V_028A10_VGT_OUTPATH_VTX_REUSE                          0x00
#define     V_028A10_VGT_OUTPATH_TESS_EN                            0x01
#define     V_028A10_VGT_OUTPATH_PASSTHRU                           0x02
#define     V_028A10_VGT_OUTPATH_GS_BLOCK                           0x03
#define     V_028A10_VGT_OUTPATH_HS_BLOCK                           0x04
#define R_028A14_VGT_HOS_CNTL                                           0x028A14
#define   S_028A14_TESS_MODE(x)                                       (((x) & 0x03) << 0)
#define   G_028A14_TESS_MODE(x)                                       (((x) >> 0) & 0x03)
#define   C_028A14_TESS_MODE                                          0xFFFFFFFC
#define R_028A18_VGT_HOS_MAX_TESS_LEVEL                                 0x028A18
#define R_028A1C_VGT_HOS_MIN_TESS_LEVEL                                 0x028A1C
#define R_028A20_VGT_HOS_REUSE_DEPTH                                    0x028A20
#define   S_028A20_REUSE_DEPTH(x)                                     (((x) & 0xFF) << 0)
#define   G_028A20_REUSE_DEPTH(x)                                     (((x) >> 0) & 0xFF)
#define   C_028A20_REUSE_DEPTH                                        0xFFFFFF00
#define R_028A24_VGT_GROUP_PRIM_TYPE                                    0x028A24
#define   S_028A24_PRIM_TYPE(x)                                       (((x) & 0x1F) << 0)
#define   G_028A24_PRIM_TYPE(x)                                       (((x) >> 0) & 0x1F)
#define   C_028A24_PRIM_TYPE                                          0xFFFFFFE0
#define     V_028A24_VGT_GRP_3D_POINT                               0x00
#define     V_028A24_VGT_GRP_3D_LINE                                0x01
#define     V_028A24_VGT_GRP_3D_TRI                                 0x02
#define     V_028A24_VGT_GRP_3D_RECT                                0x03
#define     V_028A24_VGT_GRP_3D_QUAD                                0x04
#define     V_028A24_VGT_GRP_2D_COPY_RECT_V0                        0x05
#define     V_028A24_VGT_GRP_2D_COPY_RECT_V1                        0x06
#define     V_028A24_VGT_GRP_2D_COPY_RECT_V2                        0x07
#define     V_028A24_VGT_GRP_2D_COPY_RECT_V3                        0x08
#define     V_028A24_VGT_GRP_2D_FILL_RECT                           0x09
#define     V_028A24_VGT_GRP_2D_LINE                                0x0A
#define     V_028A24_VGT_GRP_2D_TRI                                 0x0B
#define     V_028A24_VGT_GRP_PRIM_INDEX_LINE                        0x0C
#define     V_028A24_VGT_GRP_PRIM_INDEX_TRI                         0x0D
#define     V_028A24_VGT_GRP_PRIM_INDEX_QUAD                        0x0E
#define     V_028A24_VGT_GRP_3D_LINE_ADJ                            0x0F
#define     V_028A24_VGT_GRP_3D_TRI_ADJ                             0x10
#define     V_028A24_VGT_GRP_3D_PATCH                               0x11
#define   S_028A24_RETAIN_ORDER(x)                                    (((x) & 0x1) << 14)
#define   G_028A24_RETAIN_ORDER(x)                                    (((x) >> 14) & 0x1)
#define   C_028A24_RETAIN_ORDER                                       0xFFFFBFFF
#define   S_028A24_RETAIN_QUADS(x)                                    (((x) & 0x1) << 15)
#define   G_028A24_RETAIN_QUADS(x)                                    (((x) >> 15) & 0x1)
#define   C_028A24_RETAIN_QUADS                                       0xFFFF7FFF
#define   S_028A24_PRIM_ORDER(x)                                      (((x) & 0x07) << 16)
#define   G_028A24_PRIM_ORDER(x)                                      (((x) >> 16) & 0x07)
#define   C_028A24_PRIM_ORDER                                         0xFFF8FFFF
#define     V_028A24_VGT_GRP_LIST                                   0x00
#define     V_028A24_VGT_GRP_STRIP                                  0x01
#define     V_028A24_VGT_GRP_FAN                                    0x02
#define     V_028A24_VGT_GRP_LOOP                                   0x03
#define     V_028A24_VGT_GRP_POLYGON                                0x04
#define R_028A28_VGT_GROUP_FIRST_DECR                                   0x028A28
#define   S_028A28_FIRST_DECR(x)                                      (((x) & 0x0F) << 0)
#define   G_028A28_FIRST_DECR(x)                                      (((x) >> 0) & 0x0F)
#define   C_028A28_FIRST_DECR                                         0xFFFFFFF0
#define R_028A2C_VGT_GROUP_DECR                                         0x028A2C
#define   S_028A2C_DECR(x)                                            (((x) & 0x0F) << 0)
#define   G_028A2C_DECR(x)                                            (((x) >> 0) & 0x0F)
#define   C_028A2C_DECR                                               0xFFFFFFF0
#define R_028A30_VGT_GROUP_VECT_0_CNTL                                  0x028A30
#define   S_028A30_COMP_X_EN(x)                                       (((x) & 0x1) << 0)
#define   G_028A30_COMP_X_EN(x)                                       (((x) >> 0) & 0x1)
#define   C_028A30_COMP_X_EN                                          0xFFFFFFFE
#define   S_028A30_COMP_Y_EN(x)                                       (((x) & 0x1) << 1)
#define   G_028A30_COMP_Y_EN(x)                                       (((x) >> 1) & 0x1)
#define   C_028A30_COMP_Y_EN                                          0xFFFFFFFD
#define   S_028A30_COMP_Z_EN(x)                                       (((x) & 0x1) << 2)
#define   G_028A30_COMP_Z_EN(x)                                       (((x) >> 2) & 0x1)
#define   C_028A30_COMP_Z_EN                                          0xFFFFFFFB
#define   S_028A30_COMP_W_EN(x)                                       (((x) & 0x1) << 3)
#define   G_028A30_COMP_W_EN(x)                                       (((x) >> 3) & 0x1)
#define   C_028A30_COMP_W_EN                                          0xFFFFFFF7
#define   S_028A30_STRIDE(x)                                          (((x) & 0xFF) << 8)
#define   G_028A30_STRIDE(x)                                          (((x) >> 8) & 0xFF)
#define   C_028A30_STRIDE                                             0xFFFF00FF
#define   S_028A30_SHIFT(x)                                           (((x) & 0xFF) << 16)
#define   G_028A30_SHIFT(x)                                           (((x) >> 16) & 0xFF)
#define   C_028A30_SHIFT                                              0xFF00FFFF
#define R_028A34_VGT_GROUP_VECT_1_CNTL                                  0x028A34
#define   S_028A34_COMP_X_EN(x)                                       (((x) & 0x1) << 0)
#define   G_028A34_COMP_X_EN(x)                                       (((x) >> 0) & 0x1)
#define   C_028A34_COMP_X_EN                                          0xFFFFFFFE
#define   S_028A34_COMP_Y_EN(x)                                       (((x) & 0x1) << 1)
#define   G_028A34_COMP_Y_EN(x)                                       (((x) >> 1) & 0x1)
#define   C_028A34_COMP_Y_EN                                          0xFFFFFFFD
#define   S_028A34_COMP_Z_EN(x)                                       (((x) & 0x1) << 2)
#define   G_028A34_COMP_Z_EN(x)                                       (((x) >> 2) & 0x1)
#define   C_028A34_COMP_Z_EN                                          0xFFFFFFFB
#define   S_028A34_COMP_W_EN(x)                                       (((x) & 0x1) << 3)
#define   G_028A34_COMP_W_EN(x)                                       (((x) >> 3) & 0x1)
#define   C_028A34_COMP_W_EN                                          0xFFFFFFF7
#define   S_028A34_STRIDE(x)                                          (((x) & 0xFF) << 8)
#define   G_028A34_STRIDE(x)                                          (((x) >> 8) & 0xFF)
#define   C_028A34_STRIDE                                             0xFFFF00FF
#define   S_028A34_SHIFT(x)                                           (((x) & 0xFF) << 16)
#define   G_028A34_SHIFT(x)                                           (((x) >> 16) & 0xFF)
#define   C_028A34_SHIFT                                              0xFF00FFFF
#define R_028A38_VGT_GROUP_VECT_0_FMT_CNTL                              0x028A38
#define   S_028A38_X_CONV(x)                                          (((x) & 0x0F) << 0)
#define   G_028A38_X_CONV(x)                                          (((x) >> 0) & 0x0F)
#define   C_028A38_X_CONV                                             0xFFFFFFF0
#define     V_028A38_VGT_GRP_INDEX_16                               0x00
#define     V_028A38_VGT_GRP_INDEX_32                               0x01
#define     V_028A38_VGT_GRP_UINT_16                                0x02
#define     V_028A38_VGT_GRP_UINT_32                                0x03
#define     V_028A38_VGT_GRP_SINT_16                                0x04
#define     V_028A38_VGT_GRP_SINT_32                                0x05
#define     V_028A38_VGT_GRP_FLOAT_32                               0x06
#define     V_028A38_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A38_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A38_X_OFFSET(x)                                        (((x) & 0x0F) << 4)
#define   G_028A38_X_OFFSET(x)                                        (((x) >> 4) & 0x0F)
#define   C_028A38_X_OFFSET                                           0xFFFFFF0F
#define   S_028A38_Y_CONV(x)                                          (((x) & 0x0F) << 8)
#define   G_028A38_Y_CONV(x)                                          (((x) >> 8) & 0x0F)
#define   C_028A38_Y_CONV                                             0xFFFFF0FF
#define     V_028A38_VGT_GRP_INDEX_16                               0x00
#define     V_028A38_VGT_GRP_INDEX_32                               0x01
#define     V_028A38_VGT_GRP_UINT_16                                0x02
#define     V_028A38_VGT_GRP_UINT_32                                0x03
#define     V_028A38_VGT_GRP_SINT_16                                0x04
#define     V_028A38_VGT_GRP_SINT_32                                0x05
#define     V_028A38_VGT_GRP_FLOAT_32                               0x06
#define     V_028A38_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A38_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A38_Y_OFFSET(x)                                        (((x) & 0x0F) << 12)
#define   G_028A38_Y_OFFSET(x)                                        (((x) >> 12) & 0x0F)
#define   C_028A38_Y_OFFSET                                           0xFFFF0FFF
#define   S_028A38_Z_CONV(x)                                          (((x) & 0x0F) << 16)
#define   G_028A38_Z_CONV(x)                                          (((x) >> 16) & 0x0F)
#define   C_028A38_Z_CONV                                             0xFFF0FFFF
#define     V_028A38_VGT_GRP_INDEX_16                               0x00
#define     V_028A38_VGT_GRP_INDEX_32                               0x01
#define     V_028A38_VGT_GRP_UINT_16                                0x02
#define     V_028A38_VGT_GRP_UINT_32                                0x03
#define     V_028A38_VGT_GRP_SINT_16                                0x04
#define     V_028A38_VGT_GRP_SINT_32                                0x05
#define     V_028A38_VGT_GRP_FLOAT_32                               0x06
#define     V_028A38_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A38_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A38_Z_OFFSET(x)                                        (((x) & 0x0F) << 20)
#define   G_028A38_Z_OFFSET(x)                                        (((x) >> 20) & 0x0F)
#define   C_028A38_Z_OFFSET                                           0xFF0FFFFF
#define   S_028A38_W_CONV(x)                                          (((x) & 0x0F) << 24)
#define   G_028A38_W_CONV(x)                                          (((x) >> 24) & 0x0F)
#define   C_028A38_W_CONV                                             0xF0FFFFFF
#define     V_028A38_VGT_GRP_INDEX_16                               0x00
#define     V_028A38_VGT_GRP_INDEX_32                               0x01
#define     V_028A38_VGT_GRP_UINT_16                                0x02
#define     V_028A38_VGT_GRP_UINT_32                                0x03
#define     V_028A38_VGT_GRP_SINT_16                                0x04
#define     V_028A38_VGT_GRP_SINT_32                                0x05
#define     V_028A38_VGT_GRP_FLOAT_32                               0x06
#define     V_028A38_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A38_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A38_W_OFFSET(x)                                        (((x) & 0x0F) << 28)
#define   G_028A38_W_OFFSET(x)                                        (((x) >> 28) & 0x0F)
#define   C_028A38_W_OFFSET                                           0x0FFFFFFF
#define R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL                              0x028A3C
#define   S_028A3C_X_CONV(x)                                          (((x) & 0x0F) << 0)
#define   G_028A3C_X_CONV(x)                                          (((x) >> 0) & 0x0F)
#define   C_028A3C_X_CONV                                             0xFFFFFFF0
#define     V_028A3C_VGT_GRP_INDEX_16                               0x00
#define     V_028A3C_VGT_GRP_INDEX_32                               0x01
#define     V_028A3C_VGT_GRP_UINT_16                                0x02
#define     V_028A3C_VGT_GRP_UINT_32                                0x03
#define     V_028A3C_VGT_GRP_SINT_16                                0x04
#define     V_028A3C_VGT_GRP_SINT_32                                0x05
#define     V_028A3C_VGT_GRP_FLOAT_32                               0x06
#define     V_028A3C_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A3C_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A3C_X_OFFSET(x)                                        (((x) & 0x0F) << 4)
#define   G_028A3C_X_OFFSET(x)                                        (((x) >> 4) & 0x0F)
#define   C_028A3C_X_OFFSET                                           0xFFFFFF0F
#define   S_028A3C_Y_CONV(x)                                          (((x) & 0x0F) << 8)
#define   G_028A3C_Y_CONV(x)                                          (((x) >> 8) & 0x0F)
#define   C_028A3C_Y_CONV                                             0xFFFFF0FF
#define     V_028A3C_VGT_GRP_INDEX_16                               0x00
#define     V_028A3C_VGT_GRP_INDEX_32                               0x01
#define     V_028A3C_VGT_GRP_UINT_16                                0x02
#define     V_028A3C_VGT_GRP_UINT_32                                0x03
#define     V_028A3C_VGT_GRP_SINT_16                                0x04
#define     V_028A3C_VGT_GRP_SINT_32                                0x05
#define     V_028A3C_VGT_GRP_FLOAT_32                               0x06
#define     V_028A3C_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A3C_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A3C_Y_OFFSET(x)                                        (((x) & 0x0F) << 12)
#define   G_028A3C_Y_OFFSET(x)                                        (((x) >> 12) & 0x0F)
#define   C_028A3C_Y_OFFSET                                           0xFFFF0FFF
#define   S_028A3C_Z_CONV(x)                                          (((x) & 0x0F) << 16)
#define   G_028A3C_Z_CONV(x)                                          (((x) >> 16) & 0x0F)
#define   C_028A3C_Z_CONV                                             0xFFF0FFFF
#define     V_028A3C_VGT_GRP_INDEX_16                               0x00
#define     V_028A3C_VGT_GRP_INDEX_32                               0x01
#define     V_028A3C_VGT_GRP_UINT_16                                0x02
#define     V_028A3C_VGT_GRP_UINT_32                                0x03
#define     V_028A3C_VGT_GRP_SINT_16                                0x04
#define     V_028A3C_VGT_GRP_SINT_32                                0x05
#define     V_028A3C_VGT_GRP_FLOAT_32                               0x06
#define     V_028A3C_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A3C_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A3C_Z_OFFSET(x)                                        (((x) & 0x0F) << 20)
#define   G_028A3C_Z_OFFSET(x)                                        (((x) >> 20) & 0x0F)
#define   C_028A3C_Z_OFFSET                                           0xFF0FFFFF
#define   S_028A3C_W_CONV(x)                                          (((x) & 0x0F) << 24)
#define   G_028A3C_W_CONV(x)                                          (((x) >> 24) & 0x0F)
#define   C_028A3C_W_CONV                                             0xF0FFFFFF
#define     V_028A3C_VGT_GRP_INDEX_16                               0x00
#define     V_028A3C_VGT_GRP_INDEX_32                               0x01
#define     V_028A3C_VGT_GRP_UINT_16                                0x02
#define     V_028A3C_VGT_GRP_UINT_32                                0x03
#define     V_028A3C_VGT_GRP_SINT_16                                0x04
#define     V_028A3C_VGT_GRP_SINT_32                                0x05
#define     V_028A3C_VGT_GRP_FLOAT_32                               0x06
#define     V_028A3C_VGT_GRP_AUTO_PRIM                              0x07
#define     V_028A3C_VGT_GRP_FIX_1_23_TO_FLOAT                      0x08
#define   S_028A3C_W_OFFSET(x)                                        (((x) & 0x0F) << 28)
#define   G_028A3C_W_OFFSET(x)                                        (((x) >> 28) & 0x0F)
#define   C_028A3C_W_OFFSET                                           0x0FFFFFFF
#define R_028A40_VGT_GS_MODE                                            0x028A40
#define   S_028A40_MODE(x)                                            (((x) & 0x07) << 0)
#define   G_028A40_MODE(x)                                            (((x) >> 0) & 0x07)
#define   C_028A40_MODE                                               0xFFFFFFF8
#define     V_028A40_GS_OFF                                         0x00
#define     V_028A40_GS_SCENARIO_A                                  0x01
#define     V_028A40_GS_SCENARIO_B                                  0x02
#define     V_028A40_GS_SCENARIO_G                                  0x03
#define     V_028A40_GS_SCENARIO_C                                  0x04
#define     V_028A40_SPRITE_EN                                      0x05
#define   S_028A40_CUT_MODE(x)                                        (((x) & 0x03) << 4)
#define   G_028A40_CUT_MODE(x)                                        (((x) >> 4) & 0x03)
#define   C_028A40_CUT_MODE                                           0xFFFFFFCF
#define     V_028A40_GS_CUT_1024                                    0x00
#define     V_028A40_GS_CUT_512                                     0x01
#define     V_028A40_GS_CUT_256                                     0x02
#define     V_028A40_GS_CUT_128                                     0x03
#define   S_028A40_GS_C_PACK_EN(x)                                    (((x) & 0x1) << 11)
#define   G_028A40_GS_C_PACK_EN(x)                                    (((x) >> 11) & 0x1)
#define   C_028A40_GS_C_PACK_EN                                       0xFFFFF7FF
#define   S_028A40_ES_PASSTHRU(x)                                     (((x) & 0x1) << 13)
#define   G_028A40_ES_PASSTHRU(x)                                     (((x) >> 13) & 0x1)
#define   C_028A40_ES_PASSTHRU                                        0xFFFFDFFF
#define   S_028A40_COMPUTE_MODE(x)                                    (((x) & 0x1) << 14)
#define   G_028A40_COMPUTE_MODE(x)                                    (((x) >> 14) & 0x1)
#define   C_028A40_COMPUTE_MODE                                       0xFFFFBFFF
#define   S_028A40_FAST_COMPUTE_MODE(x)                               (((x) & 0x1) << 15)
#define   G_028A40_FAST_COMPUTE_MODE(x)                               (((x) >> 15) & 0x1)
#define   C_028A40_FAST_COMPUTE_MODE                                  0xFFFF7FFF
#define   S_028A40_ELEMENT_INFO_EN(x)                                 (((x) & 0x1) << 16)
#define   G_028A40_ELEMENT_INFO_EN(x)                                 (((x) >> 16) & 0x1)
#define   C_028A40_ELEMENT_INFO_EN                                    0xFFFEFFFF
#define   S_028A40_PARTIAL_THD_AT_EOI(x)                              (((x) & 0x1) << 17)
#define   G_028A40_PARTIAL_THD_AT_EOI(x)                              (((x) >> 17) & 0x1)
#define   C_028A40_PARTIAL_THD_AT_EOI                                 0xFFFDFFFF
#define   S_028A40_SUPPRESS_CUTS(x)                                   (((x) & 0x1) << 18)
#define   G_028A40_SUPPRESS_CUTS(x)                                   (((x) >> 18) & 0x1)
#define   C_028A40_SUPPRESS_CUTS                                      0xFFFBFFFF
#define   S_028A40_ES_WRITE_OPTIMIZE(x)                               (((x) & 0x1) << 19)
#define   G_028A40_ES_WRITE_OPTIMIZE(x)                               (((x) >> 19) & 0x1)
#define   C_028A40_ES_WRITE_OPTIMIZE                                  0xFFF7FFFF
#define   S_028A40_GS_WRITE_OPTIMIZE(x)                               (((x) & 0x1) << 20)
#define   G_028A40_GS_WRITE_OPTIMIZE(x)                               (((x) >> 20) & 0x1)
#define   C_028A40_GS_WRITE_OPTIMIZE                                  0xFFEFFFFF
#define R_028A48_PA_SC_MODE_CNTL_0                                      0x028A48
#define   S_028A48_MSAA_ENABLE(x)                                     (((x) & 0x1) << 0)
#define   G_028A48_MSAA_ENABLE(x)                                     (((x) >> 0) & 0x1)
#define   C_028A48_MSAA_ENABLE                                        0xFFFFFFFE
#define   S_028A48_VPORT_SCISSOR_ENABLE(x)                            (((x) & 0x1) << 1)
#define   G_028A48_VPORT_SCISSOR_ENABLE(x)                            (((x) >> 1) & 0x1)
#define   C_028A48_VPORT_SCISSOR_ENABLE                               0xFFFFFFFD
#define   S_028A48_LINE_STIPPLE_ENABLE(x)                             (((x) & 0x1) << 2)
#define   G_028A48_LINE_STIPPLE_ENABLE(x)                             (((x) >> 2) & 0x1)
#define   C_028A48_LINE_STIPPLE_ENABLE                                0xFFFFFFFB
#define   S_028A48_SEND_UNLIT_STILES_TO_PKR(x)                        (((x) & 0x1) << 3)
#define   G_028A48_SEND_UNLIT_STILES_TO_PKR(x)                        (((x) >> 3) & 0x1)
#define   C_028A48_SEND_UNLIT_STILES_TO_PKR                           0xFFFFFFF7
#define R_028A4C_PA_SC_MODE_CNTL_1                                      0x028A4C
#define   S_028A4C_WALK_SIZE(x)                                       (((x) & 0x1) << 0)
#define   G_028A4C_WALK_SIZE(x)                                       (((x) >> 0) & 0x1)
#define   C_028A4C_WALK_SIZE                                          0xFFFFFFFE
#define   S_028A4C_WALK_ALIGNMENT(x)                                  (((x) & 0x1) << 1)
#define   G_028A4C_WALK_ALIGNMENT(x)                                  (((x) >> 1) & 0x1)
#define   C_028A4C_WALK_ALIGNMENT                                     0xFFFFFFFD
#define   S_028A4C_WALK_ALIGN8_PRIM_FITS_ST(x)                        (((x) & 0x1) << 2)
#define   G_028A4C_WALK_ALIGN8_PRIM_FITS_ST(x)                        (((x) >> 2) & 0x1)
#define   C_028A4C_WALK_ALIGN8_PRIM_FITS_ST                           0xFFFFFFFB
#define   S_028A4C_WALK_FENCE_ENABLE(x)                               (((x) & 0x1) << 3)
#define   G_028A4C_WALK_FENCE_ENABLE(x)                               (((x) >> 3) & 0x1)
#define   C_028A4C_WALK_FENCE_ENABLE                                  0xFFFFFFF7
#define   S_028A4C_WALK_FENCE_SIZE(x)                                 (((x) & 0x07) << 4)
#define   G_028A4C_WALK_FENCE_SIZE(x)                                 (((x) >> 4) & 0x07)
#define   C_028A4C_WALK_FENCE_SIZE                                    0xFFFFFF8F
#define   S_028A4C_SUPERTILE_WALK_ORDER_ENABLE(x)                     (((x) & 0x1) << 7)
#define   G_028A4C_SUPERTILE_WALK_ORDER_ENABLE(x)                     (((x) >> 7) & 0x1)
#define   C_028A4C_SUPERTILE_WALK_ORDER_ENABLE                        0xFFFFFF7F
#define   S_028A4C_TILE_WALK_ORDER_ENABLE(x)                          (((x) & 0x1) << 8)
#define   G_028A4C_TILE_WALK_ORDER_ENABLE(x)                          (((x) >> 8) & 0x1)
#define   C_028A4C_TILE_WALK_ORDER_ENABLE                             0xFFFFFEFF
#define   S_028A4C_TILE_COVER_DISABLE(x)                              (((x) & 0x1) << 9)
#define   G_028A4C_TILE_COVER_DISABLE(x)                              (((x) >> 9) & 0x1)
#define   C_028A4C_TILE_COVER_DISABLE                                 0xFFFFFDFF
#define   S_028A4C_TILE_COVER_NO_SCISSOR(x)                           (((x) & 0x1) << 10)
#define   G_028A4C_TILE_COVER_NO_SCISSOR(x)                           (((x) >> 10) & 0x1)
#define   C_028A4C_TILE_COVER_NO_SCISSOR                              0xFFFFFBFF
#define   S_028A4C_ZMM_LINE_EXTENT(x)                                 (((x) & 0x1) << 11)
#define   G_028A4C_ZMM_LINE_EXTENT(x)                                 (((x) >> 11) & 0x1)
#define   C_028A4C_ZMM_LINE_EXTENT                                    0xFFFFF7FF
#define   S_028A4C_ZMM_LINE_OFFSET(x)                                 (((x) & 0x1) << 12)
#define   G_028A4C_ZMM_LINE_OFFSET(x)                                 (((x) >> 12) & 0x1)
#define   C_028A4C_ZMM_LINE_OFFSET                                    0xFFFFEFFF
#define   S_028A4C_ZMM_RECT_EXTENT(x)                                 (((x) & 0x1) << 13)
#define   G_028A4C_ZMM_RECT_EXTENT(x)                                 (((x) >> 13) & 0x1)
#define   C_028A4C_ZMM_RECT_EXTENT                                    0xFFFFDFFF
#define   S_028A4C_KILL_PIX_POST_HI_Z(x)                              (((x) & 0x1) << 14)
#define   G_028A4C_KILL_PIX_POST_HI_Z(x)                              (((x) >> 14) & 0x1)
#define   C_028A4C_KILL_PIX_POST_HI_Z                                 0xFFFFBFFF
#define   S_028A4C_KILL_PIX_POST_DETAIL_MASK(x)                       (((x) & 0x1) << 15)
#define   G_028A4C_KILL_PIX_POST_DETAIL_MASK(x)                       (((x) >> 15) & 0x1)
#define   C_028A4C_KILL_PIX_POST_DETAIL_MASK                          0xFFFF7FFF
#define   S_028A4C_PS_ITER_SAMPLE(x)                                  (((x) & 0x1) << 16)
#define   G_028A4C_PS_ITER_SAMPLE(x)                                  (((x) >> 16) & 0x1)
#define   C_028A4C_PS_ITER_SAMPLE                                     0xFFFEFFFF
#define   S_028A4C_MULTI_SHADER_ENGINE_PRIM_DISC(x)                   (((x) & 0x1) << 17)
#define   G_028A4C_MULTI_SHADER_ENGINE_PRIM_DISC(x)                   (((x) >> 17) & 0x1)
#define   C_028A4C_MULTI_SHADER_ENGINE_PRIM_DISC                      0xFFFDFFFF
#define   S_028A4C_FORCE_EOV_CNTDWN_ENABLE(x)                         (((x) & 0x1) << 25)
#define   G_028A4C_FORCE_EOV_CNTDWN_ENABLE(x)                         (((x) >> 25) & 0x1)
#define   C_028A4C_FORCE_EOV_CNTDWN_ENABLE                            0xFDFFFFFF
#define   S_028A4C_FORCE_EOV_REZ_ENABLE(x)                            (((x) & 0x1) << 26)
#define   G_028A4C_FORCE_EOV_REZ_ENABLE(x)                            (((x) >> 26) & 0x1)
#define   C_028A4C_FORCE_EOV_REZ_ENABLE                               0xFBFFFFFF
#define   S_028A4C_OUT_OF_ORDER_PRIMITIVE_ENABLE(x)                   (((x) & 0x1) << 27)
#define   G_028A4C_OUT_OF_ORDER_PRIMITIVE_ENABLE(x)                   (((x) >> 27) & 0x1)
#define   C_028A4C_OUT_OF_ORDER_PRIMITIVE_ENABLE                      0xF7FFFFFF
#define   S_028A4C_OUT_OF_ORDER_WATER_MARK(x)                         (((x) & 0x07) << 28)
#define   G_028A4C_OUT_OF_ORDER_WATER_MARK(x)                         (((x) >> 28) & 0x07)
#define   C_028A4C_OUT_OF_ORDER_WATER_MARK                            0x8FFFFFFF
#define R_028A50_VGT_ENHANCE                                            0x028A50
#define R_028A54_VGT_GS_PER_ES                                          0x028A54
#define   S_028A54_GS_PER_ES(x)                                       (((x) & 0x7FF) << 0)
#define   G_028A54_GS_PER_ES(x)                                       (((x) >> 0) & 0x7FF)
#define   C_028A54_GS_PER_ES                                          0xFFFFF800
#define R_028A58_VGT_ES_PER_GS                                          0x028A58
#define   S_028A58_ES_PER_GS(x)                                       (((x) & 0x7FF) << 0)
#define   G_028A58_ES_PER_GS(x)                                       (((x) >> 0) & 0x7FF)
#define   C_028A58_ES_PER_GS                                          0xFFFFF800
#define R_028A5C_VGT_GS_PER_VS                                          0x028A5C
#define   S_028A5C_GS_PER_VS(x)                                       (((x) & 0x0F) << 0)
#define   G_028A5C_GS_PER_VS(x)                                       (((x) >> 0) & 0x0F)
#define   C_028A5C_GS_PER_VS                                          0xFFFFFFF0
#define R_028A60_VGT_GSVS_RING_OFFSET_1                                 0x028A60
#define   S_028A60_OFFSET(x)                                          (((x) & 0x7FFF) << 0)
#define   G_028A60_OFFSET(x)                                          (((x) >> 0) & 0x7FFF)
#define   C_028A60_OFFSET                                             0xFFFF8000
#define R_028A64_VGT_GSVS_RING_OFFSET_2                                 0x028A64
#define   S_028A64_OFFSET(x)                                          (((x) & 0x7FFF) << 0)
#define   G_028A64_OFFSET(x)                                          (((x) >> 0) & 0x7FFF)
#define   C_028A64_OFFSET                                             0xFFFF8000
#define R_028A68_VGT_GSVS_RING_OFFSET_3                                 0x028A68
#define   S_028A68_OFFSET(x)                                          (((x) & 0x7FFF) << 0)
#define   G_028A68_OFFSET(x)                                          (((x) >> 0) & 0x7FFF)
#define   C_028A68_OFFSET                                             0xFFFF8000
#define R_028A6C_VGT_GS_OUT_PRIM_TYPE                                   0x028A6C
#define   S_028A6C_OUTPRIM_TYPE(x)                                    (((x) & 0x3F) << 0)
#define   G_028A6C_OUTPRIM_TYPE(x)                                    (((x) >> 0) & 0x3F)
#define   C_028A6C_OUTPRIM_TYPE                                       0xFFFFFFC0
#define   S_028A6C_OUTPRIM_TYPE_1(x)                                  (((x) & 0x3F) << 8)
#define   G_028A6C_OUTPRIM_TYPE_1(x)                                  (((x) >> 8) & 0x3F)
#define   C_028A6C_OUTPRIM_TYPE_1                                     0xFFFFC0FF
#define   S_028A6C_OUTPRIM_TYPE_2(x)                                  (((x) & 0x3F) << 16)
#define   G_028A6C_OUTPRIM_TYPE_2(x)                                  (((x) >> 16) & 0x3F)
#define   C_028A6C_OUTPRIM_TYPE_2                                     0xFFC0FFFF
#define   S_028A6C_OUTPRIM_TYPE_3(x)                                  (((x) & 0x3F) << 22)
#define   G_028A6C_OUTPRIM_TYPE_3(x)                                  (((x) >> 22) & 0x3F)
#define   C_028A6C_OUTPRIM_TYPE_3                                     0xF03FFFFF
#define   S_028A6C_UNIQUE_TYPE_PER_STREAM(x)                          (((x) & 0x1) << 31)
#define   G_028A6C_UNIQUE_TYPE_PER_STREAM(x)                          (((x) >> 31) & 0x1)
#define   C_028A6C_UNIQUE_TYPE_PER_STREAM                             0x7FFFFFFF
#define R_028A70_IA_ENHANCE                                             0x028A70
#define R_028A74_VGT_DMA_SIZE                                           0x028A74
#define R_028A78_VGT_DMA_MAX_SIZE                                       0x028A78
#define R_028A7C_VGT_DMA_INDEX_TYPE                                     0x028A7C
#define   S_028A7C_INDEX_TYPE(x)                                      (((x) & 0x03) << 0)
#define   G_028A7C_INDEX_TYPE(x)                                      (((x) >> 0) & 0x03)
#define   C_028A7C_INDEX_TYPE                                         0xFFFFFFFC
#define     V_028A7C_VGT_INDEX_16                                   0x00
#define     V_028A7C_VGT_INDEX_32                                   0x01
#define   S_028A7C_SWAP_MODE(x)                                       (((x) & 0x03) << 2)
#define   G_028A7C_SWAP_MODE(x)                                       (((x) >> 2) & 0x03)
#define   C_028A7C_SWAP_MODE                                          0xFFFFFFF3
#define     V_028A7C_VGT_DMA_SWAP_NONE                              0x00
#define     V_028A7C_VGT_DMA_SWAP_16_BIT                            0x01
#define     V_028A7C_VGT_DMA_SWAP_32_BIT                            0x02
#define     V_028A7C_VGT_DMA_SWAP_WORD                              0x03
#define R_028A84_VGT_PRIMITIVEID_EN                                     0x028A84
#define   S_028A84_PRIMITIVEID_EN(x)                                  (((x) & 0x1) << 0)
#define   G_028A84_PRIMITIVEID_EN(x)                                  (((x) >> 0) & 0x1)
#define   C_028A84_PRIMITIVEID_EN                                     0xFFFFFFFE
#define   S_028A84_DISABLE_RESET_ON_EOI(x)                            (((x) & 0x1) << 1)
#define   G_028A84_DISABLE_RESET_ON_EOI(x)                            (((x) >> 1) & 0x1)
#define   C_028A84_DISABLE_RESET_ON_EOI                               0xFFFFFFFD
#define R_028A88_VGT_DMA_NUM_INSTANCES                                  0x028A88
#define R_028A8C_VGT_PRIMITIVEID_RESET                                  0x028A8C
#define R_028A90_VGT_EVENT_INITIATOR                                    0x028A90
#define   S_028A90_EVENT_TYPE(x)                                      (((x) & 0x3F) << 0)
#define   G_028A90_EVENT_TYPE(x)                                      (((x) >> 0) & 0x3F)
#define   C_028A90_EVENT_TYPE                                         0xFFFFFFC0
#define     V_028A90_SAMPLE_STREAMOUTSTATS1                         0x01
#define     V_028A90_SAMPLE_STREAMOUTSTATS2                         0x02
#define     V_028A90_SAMPLE_STREAMOUTSTATS3                         0x03
#define     V_028A90_CACHE_FLUSH_TS                                 0x04
#define     V_028A90_CONTEXT_DONE                                   0x05
#define     V_028A90_CACHE_FLUSH                                    0x06
#define     V_028A90_CS_PARTIAL_FLUSH                               0x07
#define     V_028A90_VGT_STREAMOUT_SYNC                             0x08
#define     V_028A90_VGT_STREAMOUT_RESET                            0x0A
#define     V_028A90_END_OF_PIPE_INCR_DE                            0x0B
#define     V_028A90_END_OF_PIPE_IB_END                             0x0C
#define     V_028A90_RST_PIX_CNT                                    0x0D
#define     V_028A90_VS_PARTIAL_FLUSH                               0x0F
#define     V_028A90_PS_PARTIAL_FLUSH                               0x10
#define     V_028A90_FLUSH_HS_OUTPUT                                0x11
#define     V_028A90_FLUSH_LS_OUTPUT                                0x12
#define     V_028A90_CACHE_FLUSH_AND_INV_TS_EVENT                   0x14
#define     V_028A90_ZPASS_DONE                                     0x15
#define     V_028A90_CACHE_FLUSH_AND_INV_EVENT                      0x16
#define     V_028A90_PERFCOUNTER_START                              0x17
#define     V_028A90_PERFCOUNTER_STOP                               0x18
#define     V_028A90_PIPELINESTAT_START                             0x19
#define     V_028A90_PIPELINESTAT_STOP                              0x1A
#define     V_028A90_PERFCOUNTER_SAMPLE                             0x1B
#define     V_028A90_FLUSH_ES_OUTPUT                                0x1C
#define     V_028A90_FLUSH_GS_OUTPUT                                0x1D
#define     V_028A90_SAMPLE_PIPELINESTAT                            0x1E
#define     V_028A90_SO_VGTSTREAMOUT_FLUSH                          0x1F
#define     V_028A90_SAMPLE_STREAMOUTSTATS                          0x20
#define     V_028A90_RESET_VTX_CNT                                  0x21
#define     V_028A90_BLOCK_CONTEXT_DONE                             0x22
#define     V_028A90_CS_CONTEXT_DONE                                0x23
#define     V_028A90_VGT_FLUSH                                      0x24
#define     V_028A90_SC_SEND_DB_VPZ                                 0x27
#define     V_028A90_BOTTOM_OF_PIPE_TS                              0x28
#define     V_028A90_DB_CACHE_FLUSH_AND_INV                         0x2A
#define     V_028A90_FLUSH_AND_INV_DB_DATA_TS                       0x2B
#define     V_028A90_FLUSH_AND_INV_DB_META                          0x2C
#define     V_028A90_FLUSH_AND_INV_CB_DATA_TS                       0x2D
#define     V_028A90_FLUSH_AND_INV_CB_META                          0x2E
#define     V_028A90_CS_DONE                                        0x2F
#define     V_028A90_PS_DONE                                        0x30
#define     V_028A90_FLUSH_AND_INV_CB_PIXEL_DATA                    0x31
#define     V_028A90_THREAD_TRACE_START                             0x33
#define     V_028A90_THREAD_TRACE_STOP                              0x34
#define     V_028A90_THREAD_TRACE_MARKER                            0x35
#define     V_028A90_THREAD_TRACE_FLUSH                             0x36
#define     V_028A90_THREAD_TRACE_FINISH                            0x37
#define   S_028A90_ADDRESS_HI(x)                                      (((x) & 0x1FF) << 18)
#define   G_028A90_ADDRESS_HI(x)                                      (((x) >> 18) & 0x1FF)
#define   C_028A90_ADDRESS_HI                                         0xF803FFFF
#define   S_028A90_EXTENDED_EVENT(x)                                  (((x) & 0x1) << 27)
#define   G_028A90_EXTENDED_EVENT(x)                                  (((x) >> 27) & 0x1)
#define   C_028A90_EXTENDED_EVENT                                     0xF7FFFFFF
#define R_028A94_VGT_MULTI_PRIM_IB_RESET_EN                             0x028A94
#define   S_028A94_RESET_EN(x)                                        (((x) & 0x1) << 0)
#define   G_028A94_RESET_EN(x)                                        (((x) >> 0) & 0x1)
#define   C_028A94_RESET_EN                                           0xFFFFFFFE
#define R_028AA0_VGT_INSTANCE_STEP_RATE_0                               0x028AA0
#define R_028AA4_VGT_INSTANCE_STEP_RATE_1                               0x028AA4
#define R_028AA8_IA_MULTI_VGT_PARAM                                     0x028AA8
#define   S_028AA8_PRIMGROUP_SIZE(x)                                  (((x) & 0xFFFF) << 0)
#define   G_028AA8_PRIMGROUP_SIZE(x)                                  (((x) >> 0) & 0xFFFF)
#define   C_028AA8_PRIMGROUP_SIZE                                     0xFFFF0000
#define   S_028AA8_PARTIAL_VS_WAVE_ON(x)                              (((x) & 0x1) << 16)
#define   G_028AA8_PARTIAL_VS_WAVE_ON(x)                              (((x) >> 16) & 0x1)
#define   C_028AA8_PARTIAL_VS_WAVE_ON                                 0xFFFEFFFF
#define   S_028AA8_SWITCH_ON_EOP(x)                                   (((x) & 0x1) << 17)
#define   G_028AA8_SWITCH_ON_EOP(x)                                   (((x) >> 17) & 0x1)
#define   C_028AA8_SWITCH_ON_EOP                                      0xFFFDFFFF
#define   S_028AA8_PARTIAL_ES_WAVE_ON(x)                              (((x) & 0x1) << 18)
#define   G_028AA8_PARTIAL_ES_WAVE_ON(x)                              (((x) >> 18) & 0x1)
#define   C_028AA8_PARTIAL_ES_WAVE_ON                                 0xFFFBFFFF
#define   S_028AA8_SWITCH_ON_EOI(x)                                   (((x) & 0x1) << 19)
#define   G_028AA8_SWITCH_ON_EOI(x)                                   (((x) >> 19) & 0x1)
#define   C_028AA8_SWITCH_ON_EOI                                      0xFFF7FFFF
#define R_028AAC_VGT_ESGS_RING_ITEMSIZE                                 0x028AAC
#define   S_028AAC_ITEMSIZE(x)                                        (((x) & 0x7FFF) << 0)
#define   G_028AAC_ITEMSIZE(x)                                        (((x) >> 0) & 0x7FFF)
#define   C_028AAC_ITEMSIZE                                           0xFFFF8000
#define R_028AB0_VGT_GSVS_RING_ITEMSIZE                                 0x028AB0
#define   S_028AB0_ITEMSIZE(x)                                        (((x) & 0x7FFF) << 0)
#define   G_028AB0_ITEMSIZE(x)                                        (((x) >> 0) & 0x7FFF)
#define   C_028AB0_ITEMSIZE                                           0xFFFF8000
#define R_028AB4_VGT_REUSE_OFF                                          0x028AB4
#define   S_028AB4_REUSE_OFF(x)                                       (((x) & 0x1) << 0)
#define   G_028AB4_REUSE_OFF(x)                                       (((x) >> 0) & 0x1)
#define   C_028AB4_REUSE_OFF                                          0xFFFFFFFE
#define R_028AB8_VGT_VTX_CNT_EN                                         0x028AB8
#define   S_028AB8_VTX_CNT_EN(x)                                      (((x) & 0x1) << 0)
#define   G_028AB8_VTX_CNT_EN(x)                                      (((x) >> 0) & 0x1)
#define   C_028AB8_VTX_CNT_EN                                         0xFFFFFFFE
#define R_028ABC_DB_HTILE_SURFACE                                       0x028ABC
#define   S_028ABC_LINEAR(x)                                          (((x) & 0x1) << 0)
#define   G_028ABC_LINEAR(x)                                          (((x) >> 0) & 0x1)
#define   C_028ABC_LINEAR                                             0xFFFFFFFE
#define   S_028ABC_FULL_CACHE(x)                                      (((x) & 0x1) << 1)
#define   G_028ABC_FULL_CACHE(x)                                      (((x) >> 1) & 0x1)
#define   C_028ABC_FULL_CACHE                                         0xFFFFFFFD
#define   S_028ABC_HTILE_USES_PRELOAD_WIN(x)                          (((x) & 0x1) << 2)
#define   G_028ABC_HTILE_USES_PRELOAD_WIN(x)                          (((x) >> 2) & 0x1)
#define   C_028ABC_HTILE_USES_PRELOAD_WIN                             0xFFFFFFFB
#define   S_028ABC_PRELOAD(x)                                         (((x) & 0x1) << 3)
#define   G_028ABC_PRELOAD(x)                                         (((x) >> 3) & 0x1)
#define   C_028ABC_PRELOAD                                            0xFFFFFFF7
#define   S_028ABC_PREFETCH_WIDTH(x)                                  (((x) & 0x3F) << 4)
#define   G_028ABC_PREFETCH_WIDTH(x)                                  (((x) >> 4) & 0x3F)
#define   C_028ABC_PREFETCH_WIDTH                                     0xFFFFFC0F
#define   S_028ABC_PREFETCH_HEIGHT(x)                                 (((x) & 0x3F) << 10)
#define   G_028ABC_PREFETCH_HEIGHT(x)                                 (((x) >> 10) & 0x3F)
#define   C_028ABC_PREFETCH_HEIGHT                                    0xFFFF03FF
#define   S_028ABC_DST_OUTSIDE_ZERO_TO_ONE(x)                         (((x) & 0x1) << 16)
#define   G_028ABC_DST_OUTSIDE_ZERO_TO_ONE(x)                         (((x) >> 16) & 0x1)
#define   C_028ABC_DST_OUTSIDE_ZERO_TO_ONE                            0xFFFEFFFF
#define R_028AC0_DB_SRESULTS_COMPARE_STATE0                             0x028AC0
#define   S_028AC0_COMPAREFUNC0(x)                                    (((x) & 0x07) << 0)
#define   G_028AC0_COMPAREFUNC0(x)                                    (((x) >> 0) & 0x07)
#define   C_028AC0_COMPAREFUNC0                                       0xFFFFFFF8
#define     V_028AC0_REF_NEVER                                      0x00
#define     V_028AC0_REF_LESS                                       0x01
#define     V_028AC0_REF_EQUAL                                      0x02
#define     V_028AC0_REF_LEQUAL                                     0x03
#define     V_028AC0_REF_GREATER                                    0x04
#define     V_028AC0_REF_NOTEQUAL                                   0x05
#define     V_028AC0_REF_GEQUAL                                     0x06
#define     V_028AC0_REF_ALWAYS                                     0x07
#define   S_028AC0_COMPAREVALUE0(x)                                   (((x) & 0xFF) << 4)
#define   G_028AC0_COMPAREVALUE0(x)                                   (((x) >> 4) & 0xFF)
#define   C_028AC0_COMPAREVALUE0                                      0xFFFFF00F
#define   S_028AC0_COMPAREMASK0(x)                                    (((x) & 0xFF) << 12)
#define   G_028AC0_COMPAREMASK0(x)                                    (((x) >> 12) & 0xFF)
#define   C_028AC0_COMPAREMASK0                                       0xFFF00FFF
#define   S_028AC0_ENABLE0(x)                                         (((x) & 0x1) << 24)
#define   G_028AC0_ENABLE0(x)                                         (((x) >> 24) & 0x1)
#define   C_028AC0_ENABLE0                                            0xFEFFFFFF
#define R_028AC4_DB_SRESULTS_COMPARE_STATE1                             0x028AC4
#define   S_028AC4_COMPAREFUNC1(x)                                    (((x) & 0x07) << 0)
#define   G_028AC4_COMPAREFUNC1(x)                                    (((x) >> 0) & 0x07)
#define   C_028AC4_COMPAREFUNC1                                       0xFFFFFFF8
#define     V_028AC4_REF_NEVER                                      0x00
#define     V_028AC4_REF_LESS                                       0x01
#define     V_028AC4_REF_EQUAL                                      0x02
#define     V_028AC4_REF_LEQUAL                                     0x03
#define     V_028AC4_REF_GREATER                                    0x04
#define     V_028AC4_REF_NOTEQUAL                                   0x05
#define     V_028AC4_REF_GEQUAL                                     0x06
#define     V_028AC4_REF_ALWAYS                                     0x07
#define   S_028AC4_COMPAREVALUE1(x)                                   (((x) & 0xFF) << 4)
#define   G_028AC4_COMPAREVALUE1(x)                                   (((x) >> 4) & 0xFF)
#define   C_028AC4_COMPAREVALUE1                                      0xFFFFF00F
#define   S_028AC4_COMPAREMASK1(x)                                    (((x) & 0xFF) << 12)
#define   G_028AC4_COMPAREMASK1(x)                                    (((x) >> 12) & 0xFF)
#define   C_028AC4_COMPAREMASK1                                       0xFFF00FFF
#define   S_028AC4_ENABLE1(x)                                         (((x) & 0x1) << 24)
#define   G_028AC4_ENABLE1(x)                                         (((x) >> 24) & 0x1)
#define   C_028AC4_ENABLE1                                            0xFEFFFFFF
#define R_028AC8_DB_PRELOAD_CONTROL                                     0x028AC8
#define   S_028AC8_START_X(x)                                         (((x) & 0xFF) << 0)
#define   G_028AC8_START_X(x)                                         (((x) >> 0) & 0xFF)
#define   C_028AC8_START_X                                            0xFFFFFF00
#define   S_028AC8_START_Y(x)                                         (((x) & 0xFF) << 8)
#define   G_028AC8_START_Y(x)                                         (((x) >> 8) & 0xFF)
#define   C_028AC8_START_Y                                            0xFFFF00FF
#define   S_028AC8_MAX_X(x)                                           (((x) & 0xFF) << 16)
#define   G_028AC8_MAX_X(x)                                           (((x) >> 16) & 0xFF)
#define   C_028AC8_MAX_X                                              0xFF00FFFF
#define   S_028AC8_MAX_Y(x)                                           (((x) & 0xFF) << 24)
#define   G_028AC8_MAX_Y(x)                                           (((x) >> 24) & 0xFF)
#define   C_028AC8_MAX_Y                                              0x00FFFFFF
#define R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0                              0x028AD0
#define R_028AD4_VGT_STRMOUT_VTX_STRIDE_0                               0x028AD4
#define   S_028AD4_STRIDE(x)                                          (((x) & 0x3FF) << 0)
#define   G_028AD4_STRIDE(x)                                          (((x) >> 0) & 0x3FF)
#define   C_028AD4_STRIDE                                             0xFFFFFC00
#define R_028ADC_VGT_STRMOUT_BUFFER_OFFSET_0                            0x028ADC
#define R_028AE0_VGT_STRMOUT_BUFFER_SIZE_1                              0x028AE0
#define R_028AE4_VGT_STRMOUT_VTX_STRIDE_1                               0x028AE4
#define   S_028AE4_STRIDE(x)                                          (((x) & 0x3FF) << 0)
#define   G_028AE4_STRIDE(x)                                          (((x) >> 0) & 0x3FF)
#define   C_028AE4_STRIDE                                             0xFFFFFC00
#define R_028AEC_VGT_STRMOUT_BUFFER_OFFSET_1                            0x028AEC
#define R_028AF0_VGT_STRMOUT_BUFFER_SIZE_2                              0x028AF0
#define R_028AF4_VGT_STRMOUT_VTX_STRIDE_2                               0x028AF4
#define   S_028AF4_STRIDE(x)                                          (((x) & 0x3FF) << 0)
#define   G_028AF4_STRIDE(x)                                          (((x) >> 0) & 0x3FF)
#define   C_028AF4_STRIDE                                             0xFFFFFC00
#define R_028AFC_VGT_STRMOUT_BUFFER_OFFSET_2                            0x028AFC
#define R_028B00_VGT_STRMOUT_BUFFER_SIZE_3                              0x028B00
#define R_028B04_VGT_STRMOUT_VTX_STRIDE_3                               0x028B04
#define   S_028B04_STRIDE(x)                                          (((x) & 0x3FF) << 0)
#define   G_028B04_STRIDE(x)                                          (((x) >> 0) & 0x3FF)
#define   C_028B04_STRIDE                                             0xFFFFFC00
#define R_028B0C_VGT_STRMOUT_BUFFER_OFFSET_3                            0x028B0C
#define R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET                         0x028B28
#define R_028B2C_VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE             0x028B2C
#define R_028B30_VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE                  0x028B30
#define   S_028B30_VERTEX_STRIDE(x)                                   (((x) & 0x1FF) << 0)
#define   G_028B30_VERTEX_STRIDE(x)                                   (((x) >> 0) & 0x1FF)
#define   C_028B30_VERTEX_STRIDE                                      0xFFFFFE00
#define R_028B38_VGT_GS_MAX_VERT_OUT                                    0x028B38
#define   S_028B38_MAX_VERT_OUT(x)                                    (((x) & 0x7FF) << 0)
#define   G_028B38_MAX_VERT_OUT(x)                                    (((x) >> 0) & 0x7FF)
#define   C_028B38_MAX_VERT_OUT                                       0xFFFFF800
#define R_028B54_VGT_SHADER_STAGES_EN                                   0x028B54
#define   S_028B54_LS_EN(x)                                           (((x) & 0x03) << 0)
#define   G_028B54_LS_EN(x)                                           (((x) >> 0) & 0x03)
#define   C_028B54_LS_EN                                              0xFFFFFFFC
#define     V_028B54_LS_STAGE_OFF                                   0x00
#define     V_028B54_LS_STAGE_ON                                    0x01
#define     V_028B54_CS_STAGE_ON                                    0x02
#define   S_028B54_HS_EN(x)                                           (((x) & 0x1) << 2)
#define   G_028B54_HS_EN(x)                                           (((x) >> 2) & 0x1)
#define   C_028B54_HS_EN                                              0xFFFFFFFB
#define   S_028B54_ES_EN(x)                                           (((x) & 0x03) << 3)
#define   G_028B54_ES_EN(x)                                           (((x) >> 3) & 0x03)
#define   C_028B54_ES_EN                                              0xFFFFFFE7
#define     V_028B54_ES_STAGE_OFF                                   0x00
#define     V_028B54_ES_STAGE_DS                                    0x01
#define     V_028B54_ES_STAGE_REAL                                  0x02
#define   S_028B54_GS_EN(x)                                           (((x) & 0x1) << 5)
#define   G_028B54_GS_EN(x)                                           (((x) >> 5) & 0x1)
#define   C_028B54_GS_EN                                              0xFFFFFFDF
#define   S_028B54_VS_EN(x)                                           (((x) & 0x03) << 6)
#define   G_028B54_VS_EN(x)                                           (((x) >> 6) & 0x03)
#define   C_028B54_VS_EN                                              0xFFFFFF3F
#define     V_028B54_VS_STAGE_REAL                                  0x00
#define     V_028B54_VS_STAGE_DS                                    0x01
#define     V_028B54_VS_STAGE_COPY_SHADER                           0x02
#define   S_028B54_DYNAMIC_HS(x)                                      (((x) & 0x1) << 8)
#define   G_028B54_DYNAMIC_HS(x)                                      (((x) >> 8) & 0x1)
#define   C_028B54_DYNAMIC_HS                                         0xFFFFFEFF
#define R_028B58_VGT_LS_HS_CONFIG                                       0x028B58
#define   S_028B58_NUM_PATCHES(x)                                     (((x) & 0xFF) << 0)
#define   G_028B58_NUM_PATCHES(x)                                     (((x) >> 0) & 0xFF)
#define   C_028B58_NUM_PATCHES                                        0xFFFFFF00
#define   S_028B58_HS_NUM_INPUT_CP(x)                                 (((x) & 0x3F) << 8)
#define   G_028B58_HS_NUM_INPUT_CP(x)                                 (((x) >> 8) & 0x3F)
#define   C_028B58_HS_NUM_INPUT_CP                                    0xFFFFC0FF
#define   S_028B58_HS_NUM_OUTPUT_CP(x)                                (((x) & 0x3F) << 14)
#define   G_028B58_HS_NUM_OUTPUT_CP(x)                                (((x) >> 14) & 0x3F)
#define   C_028B58_HS_NUM_OUTPUT_CP                                   0xFFF03FFF
#define R_028B5C_VGT_GS_VERT_ITEMSIZE                                   0x028B5C
#define   S_028B5C_ITEMSIZE(x)                                        (((x) & 0x7FFF) << 0)
#define   G_028B5C_ITEMSIZE(x)                                        (((x) >> 0) & 0x7FFF)
#define   C_028B5C_ITEMSIZE                                           0xFFFF8000
#define R_028B60_VGT_GS_VERT_ITEMSIZE_1                                 0x028B60
#define   S_028B60_ITEMSIZE(x)                                        (((x) & 0x7FFF) << 0)
#define   G_028B60_ITEMSIZE(x)                                        (((x) >> 0) & 0x7FFF)
#define   C_028B60_ITEMSIZE                                           0xFFFF8000
#define R_028B64_VGT_GS_VERT_ITEMSIZE_2                                 0x028B64
#define   S_028B64_ITEMSIZE(x)                                        (((x) & 0x7FFF) << 0)
#define   G_028B64_ITEMSIZE(x)                                        (((x) >> 0) & 0x7FFF)
#define   C_028B64_ITEMSIZE                                           0xFFFF8000
#define R_028B68_VGT_GS_VERT_ITEMSIZE_3                                 0x028B68
#define   S_028B68_ITEMSIZE(x)                                        (((x) & 0x7FFF) << 0)
#define   G_028B68_ITEMSIZE(x)                                        (((x) >> 0) & 0x7FFF)
#define   C_028B68_ITEMSIZE                                           0xFFFF8000
#define R_028B6C_VGT_TF_PARAM                                           0x028B6C
#define   S_028B6C_TYPE(x)                                            (((x) & 0x03) << 0)
#define   G_028B6C_TYPE(x)                                            (((x) >> 0) & 0x03)
#define   C_028B6C_TYPE                                               0xFFFFFFFC
#define     V_028B6C_TESS_ISOLINE                                   0x00
#define     V_028B6C_TESS_TRIANGLE                                  0x01
#define     V_028B6C_TESS_QUAD                                      0x02
#define   S_028B6C_PARTITIONING(x)                                    (((x) & 0x07) << 2)
#define   G_028B6C_PARTITIONING(x)                                    (((x) >> 2) & 0x07)
#define   C_028B6C_PARTITIONING                                       0xFFFFFFE3
#define     V_028B6C_PART_INTEGER                                   0x00
#define     V_028B6C_PART_POW2                                      0x01
#define     V_028B6C_PART_FRAC_ODD                                  0x02
#define     V_028B6C_PART_FRAC_EVEN                                 0x03
#define   S_028B6C_TOPOLOGY(x)                                        (((x) & 0x07) << 5)
#define   G_028B6C_TOPOLOGY(x)                                        (((x) >> 5) & 0x07)
#define   C_028B6C_TOPOLOGY                                           0xFFFFFF1F
#define     V_028B6C_OUTPUT_POINT                                   0x00
#define     V_028B6C_OUTPUT_LINE                                    0x01
#define     V_028B6C_OUTPUT_TRIANGLE_CW                             0x02
#define     V_028B6C_OUTPUT_TRIANGLE_CCW                            0x03
#define   S_028B6C_RESERVED_REDUC_AXIS(x)                             (((x) & 0x1) << 8)
#define   G_028B6C_RESERVED_REDUC_AXIS(x)                             (((x) >> 8) & 0x1)
#define   C_028B6C_RESERVED_REDUC_AXIS                                0xFFFFFEFF
#define   S_028B6C_NUM_DS_WAVES_PER_SIMD(x)                           (((x) & 0x0F) << 10)
#define   G_028B6C_NUM_DS_WAVES_PER_SIMD(x)                           (((x) >> 10) & 0x0F)
#define   C_028B6C_NUM_DS_WAVES_PER_SIMD                              0xFFFFC3FF
#define   S_028B6C_DISABLE_DONUTS(x)                                  (((x) & 0x1) << 14)
#define   G_028B6C_DISABLE_DONUTS(x)                                  (((x) >> 14) & 0x1)
#define   C_028B6C_DISABLE_DONUTS                                     0xFFFFBFFF
#define R_028B70_DB_ALPHA_TO_MASK                                       0x028B70
#define   S_028B70_ALPHA_TO_MASK_ENABLE(x)                            (((x) & 0x1) << 0)
#define   G_028B70_ALPHA_TO_MASK_ENABLE(x)                            (((x) >> 0) & 0x1)
#define   C_028B70_ALPHA_TO_MASK_ENABLE                               0xFFFFFFFE
#define   S_028B70_ALPHA_TO_MASK_OFFSET0(x)                           (((x) & 0x03) << 8)
#define   G_028B70_ALPHA_TO_MASK_OFFSET0(x)                           (((x) >> 8) & 0x03)
#define   C_028B70_ALPHA_TO_MASK_OFFSET0                              0xFFFFFCFF
#define   S_028B70_ALPHA_TO_MASK_OFFSET1(x)                           (((x) & 0x03) << 10)
#define   G_028B70_ALPHA_TO_MASK_OFFSET1(x)                           (((x) >> 10) & 0x03)
#define   C_028B70_ALPHA_TO_MASK_OFFSET1                              0xFFFFF3FF
#define   S_028B70_ALPHA_TO_MASK_OFFSET2(x)                           (((x) & 0x03) << 12)
#define   G_028B70_ALPHA_TO_MASK_OFFSET2(x)                           (((x) >> 12) & 0x03)
#define   C_028B70_ALPHA_TO_MASK_OFFSET2                              0xFFFFCFFF
#define   S_028B70_ALPHA_TO_MASK_OFFSET3(x)                           (((x) & 0x03) << 14)
#define   G_028B70_ALPHA_TO_MASK_OFFSET3(x)                           (((x) >> 14) & 0x03)
#define   C_028B70_ALPHA_TO_MASK_OFFSET3                              0xFFFF3FFF
#define   S_028B70_OFFSET_ROUND(x)                                    (((x) & 0x1) << 16)
#define   G_028B70_OFFSET_ROUND(x)                                    (((x) >> 16) & 0x1)
#define   C_028B70_OFFSET_ROUND                                       0xFFFEFFFF
#define R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL                          0x028B78
#define   S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS(x)                     (((x) & 0xFF) << 0)
#define   G_028B78_POLY_OFFSET_NEG_NUM_DB_BITS(x)                     (((x) >> 0) & 0xFF)
#define   C_028B78_POLY_OFFSET_NEG_NUM_DB_BITS                        0xFFFFFF00
#define   S_028B78_POLY_OFFSET_DB_IS_FLOAT_FMT(x)                     (((x) & 0x1) << 8)
#define   G_028B78_POLY_OFFSET_DB_IS_FLOAT_FMT(x)                     (((x) >> 8) & 0x1)
#define   C_028B78_POLY_OFFSET_DB_IS_FLOAT_FMT                        0xFFFFFEFF
#define R_028B7C_PA_SU_POLY_OFFSET_CLAMP                                0x028B7C
#define R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE                          0x028B80
#define R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET                         0x028B84
#define R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE                           0x028B88
#define R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET                          0x028B8C
#define R_028B90_VGT_GS_INSTANCE_CNT                                    0x028B90
#define   S_028B90_ENABLE(x)                                          (((x) & 0x1) << 0)
#define   G_028B90_ENABLE(x)                                          (((x) >> 0) & 0x1)
#define   C_028B90_ENABLE                                             0xFFFFFFFE
#define   S_028B90_CNT(x)                                             (((x) & 0x7F) << 2)
#define   G_028B90_CNT(x)                                             (((x) >> 2) & 0x7F)
#define   C_028B90_CNT                                                0xFFFFFE03
#define R_028B94_VGT_STRMOUT_CONFIG                                     0x028B94
#define   S_028B94_STREAMOUT_0_EN(x)                                  (((x) & 0x1) << 0)
#define   G_028B94_STREAMOUT_0_EN(x)                                  (((x) >> 0) & 0x1)
#define   C_028B94_STREAMOUT_0_EN                                     0xFFFFFFFE
#define   S_028B94_STREAMOUT_1_EN(x)                                  (((x) & 0x1) << 1)
#define   G_028B94_STREAMOUT_1_EN(x)                                  (((x) >> 1) & 0x1)
#define   C_028B94_STREAMOUT_1_EN                                     0xFFFFFFFD
#define   S_028B94_STREAMOUT_2_EN(x)                                  (((x) & 0x1) << 2)
#define   G_028B94_STREAMOUT_2_EN(x)                                  (((x) >> 2) & 0x1)
#define   C_028B94_STREAMOUT_2_EN                                     0xFFFFFFFB
#define   S_028B94_STREAMOUT_3_EN(x)                                  (((x) & 0x1) << 3)
#define   G_028B94_STREAMOUT_3_EN(x)                                  (((x) >> 3) & 0x1)
#define   C_028B94_STREAMOUT_3_EN                                     0xFFFFFFF7
#define   S_028B94_RAST_STREAM(x)                                     (((x) & 0x07) << 4)
#define   G_028B94_RAST_STREAM(x)                                     (((x) >> 4) & 0x07)
#define   C_028B94_RAST_STREAM                                        0xFFFFFF8F
#define   S_028B94_RAST_STREAM_MASK(x)                                (((x) & 0x0F) << 8)
#define   G_028B94_RAST_STREAM_MASK(x)                                (((x) >> 8) & 0x0F)
#define   C_028B94_RAST_STREAM_MASK                                   0xFFFFF0FF
#define   S_028B94_USE_RAST_STREAM_MASK(x)                            (((x) & 0x1) << 31)
#define   G_028B94_USE_RAST_STREAM_MASK(x)                            (((x) >> 31) & 0x1)
#define   C_028B94_USE_RAST_STREAM_MASK                               0x7FFFFFFF
#define R_028B98_VGT_STRMOUT_BUFFER_CONFIG                              0x028B98
#define   S_028B98_STREAM_0_BUFFER_EN(x)                              (((x) & 0x0F) << 0)
#define   G_028B98_STREAM_0_BUFFER_EN(x)                              (((x) >> 0) & 0x0F)
#define   C_028B98_STREAM_0_BUFFER_EN                                 0xFFFFFFF0
#define   S_028B98_STREAM_1_BUFFER_EN(x)                              (((x) & 0x0F) << 4)
#define   G_028B98_STREAM_1_BUFFER_EN(x)                              (((x) >> 4) & 0x0F)
#define   C_028B98_STREAM_1_BUFFER_EN                                 0xFFFFFF0F
#define   S_028B98_STREAM_2_BUFFER_EN(x)                              (((x) & 0x0F) << 8)
#define   G_028B98_STREAM_2_BUFFER_EN(x)                              (((x) >> 8) & 0x0F)
#define   C_028B98_STREAM_2_BUFFER_EN                                 0xFFFFF0FF
#define   S_028B98_STREAM_3_BUFFER_EN(x)                              (((x) & 0x0F) << 12)
#define   G_028B98_STREAM_3_BUFFER_EN(x)                              (((x) >> 12) & 0x0F)
#define   C_028B98_STREAM_3_BUFFER_EN                                 0xFFFF0FFF
#define R_028BD4_PA_SC_CENTROID_PRIORITY_0                              0x028BD4
#define   S_028BD4_DISTANCE_0(x)                                      (((x) & 0x0F) << 0)
#define   G_028BD4_DISTANCE_0(x)                                      (((x) >> 0) & 0x0F)
#define   C_028BD4_DISTANCE_0                                         0xFFFFFFF0
#define   S_028BD4_DISTANCE_1(x)                                      (((x) & 0x0F) << 4)
#define   G_028BD4_DISTANCE_1(x)                                      (((x) >> 4) & 0x0F)
#define   C_028BD4_DISTANCE_1                                         0xFFFFFF0F
#define   S_028BD4_DISTANCE_2(x)                                      (((x) & 0x0F) << 8)
#define   G_028BD4_DISTANCE_2(x)                                      (((x) >> 8) & 0x0F)
#define   C_028BD4_DISTANCE_2                                         0xFFFFF0FF
#define   S_028BD4_DISTANCE_3(x)                                      (((x) & 0x0F) << 12)
#define   G_028BD4_DISTANCE_3(x)                                      (((x) >> 12) & 0x0F)
#define   C_028BD4_DISTANCE_3                                         0xFFFF0FFF
#define   S_028BD4_DISTANCE_4(x)                                      (((x) & 0x0F) << 16)
#define   G_028BD4_DISTANCE_4(x)                                      (((x) >> 16) & 0x0F)
#define   C_028BD4_DISTANCE_4                                         0xFFF0FFFF
#define   S_028BD4_DISTANCE_5(x)                                      (((x) & 0x0F) << 20)
#define   G_028BD4_DISTANCE_5(x)                                      (((x) >> 20) & 0x0F)
#define   C_028BD4_DISTANCE_5                                         0xFF0FFFFF
#define   S_028BD4_DISTANCE_6(x)                                      (((x) & 0x0F) << 24)
#define   G_028BD4_DISTANCE_6(x)                                      (((x) >> 24) & 0x0F)
#define   C_028BD4_DISTANCE_6                                         0xF0FFFFFF
#define   S_028BD4_DISTANCE_7(x)                                      (((x) & 0x0F) << 28)
#define   G_028BD4_DISTANCE_7(x)                                      (((x) >> 28) & 0x0F)
#define   C_028BD4_DISTANCE_7                                         0x0FFFFFFF
#define R_028BD8_PA_SC_CENTROID_PRIORITY_1                              0x028BD8
#define   S_028BD8_DISTANCE_8(x)                                      (((x) & 0x0F) << 0)
#define   G_028BD8_DISTANCE_8(x)                                      (((x) >> 0) & 0x0F)
#define   C_028BD8_DISTANCE_8                                         0xFFFFFFF0
#define   S_028BD8_DISTANCE_9(x)                                      (((x) & 0x0F) << 4)
#define   G_028BD8_DISTANCE_9(x)                                      (((x) >> 4) & 0x0F)
#define   C_028BD8_DISTANCE_9                                         0xFFFFFF0F
#define   S_028BD8_DISTANCE_10(x)                                     (((x) & 0x0F) << 8)
#define   G_028BD8_DISTANCE_10(x)                                     (((x) >> 8) & 0x0F)
#define   C_028BD8_DISTANCE_10                                        0xFFFFF0FF
#define   S_028BD8_DISTANCE_11(x)                                     (((x) & 0x0F) << 12)
#define   G_028BD8_DISTANCE_11(x)                                     (((x) >> 12) & 0x0F)
#define   C_028BD8_DISTANCE_11                                        0xFFFF0FFF
#define   S_028BD8_DISTANCE_12(x)                                     (((x) & 0x0F) << 16)
#define   G_028BD8_DISTANCE_12(x)                                     (((x) >> 16) & 0x0F)
#define   C_028BD8_DISTANCE_12                                        0xFFF0FFFF
#define   S_028BD8_DISTANCE_13(x)                                     (((x) & 0x0F) << 20)
#define   G_028BD8_DISTANCE_13(x)                                     (((x) >> 20) & 0x0F)
#define   C_028BD8_DISTANCE_13                                        0xFF0FFFFF
#define   S_028BD8_DISTANCE_14(x)                                     (((x) & 0x0F) << 24)
#define   G_028BD8_DISTANCE_14(x)                                     (((x) >> 24) & 0x0F)
#define   C_028BD8_DISTANCE_14                                        0xF0FFFFFF
#define   S_028BD8_DISTANCE_15(x)                                     (((x) & 0x0F) << 28)
#define   G_028BD8_DISTANCE_15(x)                                     (((x) >> 28) & 0x0F)
#define   C_028BD8_DISTANCE_15                                        0x0FFFFFFF
#define R_028BDC_PA_SC_LINE_CNTL                                        0x028BDC
#define   S_028BDC_EXPAND_LINE_WIDTH(x)                               (((x) & 0x1) << 9)
#define   G_028BDC_EXPAND_LINE_WIDTH(x)                               (((x) >> 9) & 0x1)
#define   C_028BDC_EXPAND_LINE_WIDTH                                  0xFFFFFDFF
#define   S_028BDC_LAST_PIXEL(x)                                      (((x) & 0x1) << 10)
#define   G_028BDC_LAST_PIXEL(x)                                      (((x) >> 10) & 0x1)
#define   C_028BDC_LAST_PIXEL                                         0xFFFFFBFF
#define   S_028BDC_PERPENDICULAR_ENDCAP_ENA(x)                        (((x) & 0x1) << 11)
#define   G_028BDC_PERPENDICULAR_ENDCAP_ENA(x)                        (((x) >> 11) & 0x1)
#define   C_028BDC_PERPENDICULAR_ENDCAP_ENA                           0xFFFFF7FF
#define   S_028BDC_DX10_DIAMOND_TEST_ENA(x)                           (((x) & 0x1) << 12)
#define   G_028BDC_DX10_DIAMOND_TEST_ENA(x)                           (((x) >> 12) & 0x1)
#define   C_028BDC_DX10_DIAMOND_TEST_ENA                              0xFFFFEFFF
#define R_028BE0_PA_SC_AA_CONFIG                                        0x028BE0
#define   S_028BE0_MSAA_NUM_SAMPLES(x)                                (((x) & 0x07) << 0)
#define   G_028BE0_MSAA_NUM_SAMPLES(x)                                (((x) >> 0) & 0x07)
#define   C_028BE0_MSAA_NUM_SAMPLES                                   0xFFFFFFF8
#define   S_028BE0_AA_MASK_CENTROID_DTMN(x)                           (((x) & 0x1) << 4)
#define   G_028BE0_AA_MASK_CENTROID_DTMN(x)                           (((x) >> 4) & 0x1)
#define   C_028BE0_AA_MASK_CENTROID_DTMN                              0xFFFFFFEF
#define   S_028BE0_MAX_SAMPLE_DIST(x)                                 (((x) & 0x0F) << 13)
#define   G_028BE0_MAX_SAMPLE_DIST(x)                                 (((x) >> 13) & 0x0F)
#define   C_028BE0_MAX_SAMPLE_DIST                                    0xFFFE1FFF
#define   S_028BE0_MSAA_EXPOSED_SAMPLES(x)                            (((x) & 0x07) << 20)
#define   G_028BE0_MSAA_EXPOSED_SAMPLES(x)                            (((x) >> 20) & 0x07)
#define   C_028BE0_MSAA_EXPOSED_SAMPLES                               0xFF8FFFFF
#define   S_028BE0_DETAIL_TO_EXPOSED_MODE(x)                          (((x) & 0x03) << 24)
#define   G_028BE0_DETAIL_TO_EXPOSED_MODE(x)                          (((x) >> 24) & 0x03)
#define   C_028BE0_DETAIL_TO_EXPOSED_MODE                             0xFCFFFFFF
#define R_028BE4_PA_SU_VTX_CNTL                                         0x028BE4
#define   S_028BE4_PIX_CENTER(x)                                      (((x) & 0x1) << 0)
#define   G_028BE4_PIX_CENTER(x)                                      (((x) >> 0) & 0x1)
#define   C_028BE4_PIX_CENTER                                         0xFFFFFFFE
#define   S_028BE4_ROUND_MODE(x)                                      (((x) & 0x03) << 1)
#define   G_028BE4_ROUND_MODE(x)                                      (((x) >> 1) & 0x03)
#define   C_028BE4_ROUND_MODE                                         0xFFFFFFF9
#define     V_028BE4_X_TRUNCATE                                     0x00
#define     V_028BE4_X_ROUND                                        0x01
#define     V_028BE4_X_ROUND_TO_EVEN                                0x02
#define     V_028BE4_X_ROUND_TO_ODD                                 0x03
#define   S_028BE4_QUANT_MODE(x)                                      (((x) & 0x07) << 3)
#define   G_028BE4_QUANT_MODE(x)                                      (((x) >> 3) & 0x07)
#define   C_028BE4_QUANT_MODE                                         0xFFFFFFC7
#define     V_028BE4_X_16_8_FIXED_POINT_1_16TH                      0x00
#define     V_028BE4_X_16_8_FIXED_POINT_1_8TH                       0x01
#define     V_028BE4_X_16_8_FIXED_POINT_1_4TH                       0x02
#define     V_028BE4_X_16_8_FIXED_POINT_1_2                         0x03
#define     V_028BE4_X_16_8_FIXED_POINT_1                           0x04
#define     V_028BE4_X_16_8_FIXED_POINT_1_256TH                     0x05
#define     V_028BE4_X_14_10_FIXED_POINT_1_1024TH                   0x06
#define     V_028BE4_X_12_12_FIXED_POINT_1_4096TH                   0x07
#define R_028BE8_PA_CL_GB_VERT_CLIP_ADJ                                 0x028BE8
#define R_028BEC_PA_CL_GB_VERT_DISC_ADJ                                 0x028BEC
#define R_028BF0_PA_CL_GB_HORZ_CLIP_ADJ                                 0x028BF0
#define R_028BF4_PA_CL_GB_HORZ_DISC_ADJ                                 0x028BF4
#define R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0                      0x028BF8
#define   S_028BF8_S0_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028BF8_S0_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028BF8_S0_X                                               0xFFFFFFF0
#define   S_028BF8_S0_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028BF8_S0_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028BF8_S0_Y                                               0xFFFFFF0F
#define   S_028BF8_S1_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028BF8_S1_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028BF8_S1_X                                               0xFFFFF0FF
#define   S_028BF8_S1_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028BF8_S1_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028BF8_S1_Y                                               0xFFFF0FFF
#define   S_028BF8_S2_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028BF8_S2_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028BF8_S2_X                                               0xFFF0FFFF
#define   S_028BF8_S2_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028BF8_S2_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028BF8_S2_Y                                               0xFF0FFFFF
#define   S_028BF8_S3_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028BF8_S3_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028BF8_S3_X                                               0xF0FFFFFF
#define   S_028BF8_S3_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028BF8_S3_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028BF8_S3_Y                                               0x0FFFFFFF
#define R_028BFC_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_1                      0x028BFC
#define   S_028BFC_S4_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028BFC_S4_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028BFC_S4_X                                               0xFFFFFFF0
#define   S_028BFC_S4_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028BFC_S4_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028BFC_S4_Y                                               0xFFFFFF0F
#define   S_028BFC_S5_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028BFC_S5_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028BFC_S5_X                                               0xFFFFF0FF
#define   S_028BFC_S5_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028BFC_S5_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028BFC_S5_Y                                               0xFFFF0FFF
#define   S_028BFC_S6_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028BFC_S6_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028BFC_S6_X                                               0xFFF0FFFF
#define   S_028BFC_S6_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028BFC_S6_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028BFC_S6_Y                                               0xFF0FFFFF
#define   S_028BFC_S7_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028BFC_S7_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028BFC_S7_X                                               0xF0FFFFFF
#define   S_028BFC_S7_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028BFC_S7_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028BFC_S7_Y                                               0x0FFFFFFF
#define R_028C00_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_2                      0x028C00
#define   S_028C00_S8_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C00_S8_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C00_S8_X                                               0xFFFFFFF0
#define   S_028C00_S8_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C00_S8_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C00_S8_Y                                               0xFFFFFF0F
#define   S_028C00_S9_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C00_S9_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C00_S9_X                                               0xFFFFF0FF
#define   S_028C00_S9_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C00_S9_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C00_S9_Y                                               0xFFFF0FFF
#define   S_028C00_S10_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C00_S10_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C00_S10_X                                              0xFFF0FFFF
#define   S_028C00_S10_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C00_S10_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C00_S10_Y                                              0xFF0FFFFF
#define   S_028C00_S11_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C00_S11_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C00_S11_X                                              0xF0FFFFFF
#define   S_028C00_S11_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C00_S11_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C00_S11_Y                                              0x0FFFFFFF
#define R_028C04_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_3                      0x028C04
#define   S_028C04_S12_X(x)                                           (((x) & 0x0F) << 0)
#define   G_028C04_S12_X(x)                                           (((x) >> 0) & 0x0F)
#define   C_028C04_S12_X                                              0xFFFFFFF0
#define   S_028C04_S12_Y(x)                                           (((x) & 0x0F) << 4)
#define   G_028C04_S12_Y(x)                                           (((x) >> 4) & 0x0F)
#define   C_028C04_S12_Y                                              0xFFFFFF0F
#define   S_028C04_S13_X(x)                                           (((x) & 0x0F) << 8)
#define   G_028C04_S13_X(x)                                           (((x) >> 8) & 0x0F)
#define   C_028C04_S13_X                                              0xFFFFF0FF
#define   S_028C04_S13_Y(x)                                           (((x) & 0x0F) << 12)
#define   G_028C04_S13_Y(x)                                           (((x) >> 12) & 0x0F)
#define   C_028C04_S13_Y                                              0xFFFF0FFF
#define   S_028C04_S14_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C04_S14_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C04_S14_X                                              0xFFF0FFFF
#define   S_028C04_S14_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C04_S14_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C04_S14_Y                                              0xFF0FFFFF
#define   S_028C04_S15_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C04_S15_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C04_S15_X                                              0xF0FFFFFF
#define   S_028C04_S15_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C04_S15_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C04_S15_Y                                              0x0FFFFFFF
#define R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0                      0x028C08
#define   S_028C08_S0_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C08_S0_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C08_S0_X                                               0xFFFFFFF0
#define   S_028C08_S0_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C08_S0_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C08_S0_Y                                               0xFFFFFF0F
#define   S_028C08_S1_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C08_S1_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C08_S1_X                                               0xFFFFF0FF
#define   S_028C08_S1_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C08_S1_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C08_S1_Y                                               0xFFFF0FFF
#define   S_028C08_S2_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028C08_S2_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028C08_S2_X                                               0xFFF0FFFF
#define   S_028C08_S2_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028C08_S2_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028C08_S2_Y                                               0xFF0FFFFF
#define   S_028C08_S3_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028C08_S3_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028C08_S3_X                                               0xF0FFFFFF
#define   S_028C08_S3_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028C08_S3_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028C08_S3_Y                                               0x0FFFFFFF
#define R_028C0C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_1                      0x028C0C
#define   S_028C0C_S4_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C0C_S4_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C0C_S4_X                                               0xFFFFFFF0
#define   S_028C0C_S4_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C0C_S4_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C0C_S4_Y                                               0xFFFFFF0F
#define   S_028C0C_S5_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C0C_S5_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C0C_S5_X                                               0xFFFFF0FF
#define   S_028C0C_S5_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C0C_S5_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C0C_S5_Y                                               0xFFFF0FFF
#define   S_028C0C_S6_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028C0C_S6_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028C0C_S6_X                                               0xFFF0FFFF
#define   S_028C0C_S6_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028C0C_S6_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028C0C_S6_Y                                               0xFF0FFFFF
#define   S_028C0C_S7_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028C0C_S7_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028C0C_S7_X                                               0xF0FFFFFF
#define   S_028C0C_S7_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028C0C_S7_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028C0C_S7_Y                                               0x0FFFFFFF
#define R_028C10_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_2                      0x028C10
#define   S_028C10_S8_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C10_S8_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C10_S8_X                                               0xFFFFFFF0
#define   S_028C10_S8_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C10_S8_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C10_S8_Y                                               0xFFFFFF0F
#define   S_028C10_S9_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C10_S9_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C10_S9_X                                               0xFFFFF0FF
#define   S_028C10_S9_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C10_S9_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C10_S9_Y                                               0xFFFF0FFF
#define   S_028C10_S10_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C10_S10_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C10_S10_X                                              0xFFF0FFFF
#define   S_028C10_S10_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C10_S10_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C10_S10_Y                                              0xFF0FFFFF
#define   S_028C10_S11_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C10_S11_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C10_S11_X                                              0xF0FFFFFF
#define   S_028C10_S11_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C10_S11_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C10_S11_Y                                              0x0FFFFFFF
#define R_028C14_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_3                      0x028C14
#define   S_028C14_S12_X(x)                                           (((x) & 0x0F) << 0)
#define   G_028C14_S12_X(x)                                           (((x) >> 0) & 0x0F)
#define   C_028C14_S12_X                                              0xFFFFFFF0
#define   S_028C14_S12_Y(x)                                           (((x) & 0x0F) << 4)
#define   G_028C14_S12_Y(x)                                           (((x) >> 4) & 0x0F)
#define   C_028C14_S12_Y                                              0xFFFFFF0F
#define   S_028C14_S13_X(x)                                           (((x) & 0x0F) << 8)
#define   G_028C14_S13_X(x)                                           (((x) >> 8) & 0x0F)
#define   C_028C14_S13_X                                              0xFFFFF0FF
#define   S_028C14_S13_Y(x)                                           (((x) & 0x0F) << 12)
#define   G_028C14_S13_Y(x)                                           (((x) >> 12) & 0x0F)
#define   C_028C14_S13_Y                                              0xFFFF0FFF
#define   S_028C14_S14_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C14_S14_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C14_S14_X                                              0xFFF0FFFF
#define   S_028C14_S14_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C14_S14_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C14_S14_Y                                              0xFF0FFFFF
#define   S_028C14_S15_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C14_S15_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C14_S15_X                                              0xF0FFFFFF
#define   S_028C14_S15_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C14_S15_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C14_S15_Y                                              0x0FFFFFFF
#define R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0                      0x028C18
#define   S_028C18_S0_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C18_S0_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C18_S0_X                                               0xFFFFFFF0
#define   S_028C18_S0_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C18_S0_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C18_S0_Y                                               0xFFFFFF0F
#define   S_028C18_S1_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C18_S1_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C18_S1_X                                               0xFFFFF0FF
#define   S_028C18_S1_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C18_S1_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C18_S1_Y                                               0xFFFF0FFF
#define   S_028C18_S2_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028C18_S2_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028C18_S2_X                                               0xFFF0FFFF
#define   S_028C18_S2_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028C18_S2_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028C18_S2_Y                                               0xFF0FFFFF
#define   S_028C18_S3_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028C18_S3_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028C18_S3_X                                               0xF0FFFFFF
#define   S_028C18_S3_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028C18_S3_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028C18_S3_Y                                               0x0FFFFFFF
#define R_028C1C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_1                      0x028C1C
#define   S_028C1C_S4_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C1C_S4_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C1C_S4_X                                               0xFFFFFFF0
#define   S_028C1C_S4_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C1C_S4_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C1C_S4_Y                                               0xFFFFFF0F
#define   S_028C1C_S5_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C1C_S5_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C1C_S5_X                                               0xFFFFF0FF
#define   S_028C1C_S5_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C1C_S5_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C1C_S5_Y                                               0xFFFF0FFF
#define   S_028C1C_S6_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028C1C_S6_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028C1C_S6_X                                               0xFFF0FFFF
#define   S_028C1C_S6_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028C1C_S6_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028C1C_S6_Y                                               0xFF0FFFFF
#define   S_028C1C_S7_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028C1C_S7_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028C1C_S7_X                                               0xF0FFFFFF
#define   S_028C1C_S7_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028C1C_S7_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028C1C_S7_Y                                               0x0FFFFFFF
#define R_028C20_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_2                      0x028C20
#define   S_028C20_S8_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C20_S8_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C20_S8_X                                               0xFFFFFFF0
#define   S_028C20_S8_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C20_S8_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C20_S8_Y                                               0xFFFFFF0F
#define   S_028C20_S9_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C20_S9_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C20_S9_X                                               0xFFFFF0FF
#define   S_028C20_S9_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C20_S9_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C20_S9_Y                                               0xFFFF0FFF
#define   S_028C20_S10_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C20_S10_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C20_S10_X                                              0xFFF0FFFF
#define   S_028C20_S10_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C20_S10_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C20_S10_Y                                              0xFF0FFFFF
#define   S_028C20_S11_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C20_S11_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C20_S11_X                                              0xF0FFFFFF
#define   S_028C20_S11_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C20_S11_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C20_S11_Y                                              0x0FFFFFFF
#define R_028C24_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_3                      0x028C24
#define   S_028C24_S12_X(x)                                           (((x) & 0x0F) << 0)
#define   G_028C24_S12_X(x)                                           (((x) >> 0) & 0x0F)
#define   C_028C24_S12_X                                              0xFFFFFFF0
#define   S_028C24_S12_Y(x)                                           (((x) & 0x0F) << 4)
#define   G_028C24_S12_Y(x)                                           (((x) >> 4) & 0x0F)
#define   C_028C24_S12_Y                                              0xFFFFFF0F
#define   S_028C24_S13_X(x)                                           (((x) & 0x0F) << 8)
#define   G_028C24_S13_X(x)                                           (((x) >> 8) & 0x0F)
#define   C_028C24_S13_X                                              0xFFFFF0FF
#define   S_028C24_S13_Y(x)                                           (((x) & 0x0F) << 12)
#define   G_028C24_S13_Y(x)                                           (((x) >> 12) & 0x0F)
#define   C_028C24_S13_Y                                              0xFFFF0FFF
#define   S_028C24_S14_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C24_S14_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C24_S14_X                                              0xFFF0FFFF
#define   S_028C24_S14_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C24_S14_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C24_S14_Y                                              0xFF0FFFFF
#define   S_028C24_S15_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C24_S15_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C24_S15_X                                              0xF0FFFFFF
#define   S_028C24_S15_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C24_S15_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C24_S15_Y                                              0x0FFFFFFF
#define R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0                      0x028C28
#define   S_028C28_S0_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C28_S0_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C28_S0_X                                               0xFFFFFFF0
#define   S_028C28_S0_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C28_S0_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C28_S0_Y                                               0xFFFFFF0F
#define   S_028C28_S1_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C28_S1_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C28_S1_X                                               0xFFFFF0FF
#define   S_028C28_S1_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C28_S1_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C28_S1_Y                                               0xFFFF0FFF
#define   S_028C28_S2_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028C28_S2_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028C28_S2_X                                               0xFFF0FFFF
#define   S_028C28_S2_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028C28_S2_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028C28_S2_Y                                               0xFF0FFFFF
#define   S_028C28_S3_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028C28_S3_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028C28_S3_X                                               0xF0FFFFFF
#define   S_028C28_S3_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028C28_S3_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028C28_S3_Y                                               0x0FFFFFFF
#define R_028C2C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_1                      0x028C2C
#define   S_028C2C_S4_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C2C_S4_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C2C_S4_X                                               0xFFFFFFF0
#define   S_028C2C_S4_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C2C_S4_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C2C_S4_Y                                               0xFFFFFF0F
#define   S_028C2C_S5_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C2C_S5_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C2C_S5_X                                               0xFFFFF0FF
#define   S_028C2C_S5_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C2C_S5_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C2C_S5_Y                                               0xFFFF0FFF
#define   S_028C2C_S6_X(x)                                            (((x) & 0x0F) << 16)
#define   G_028C2C_S6_X(x)                                            (((x) >> 16) & 0x0F)
#define   C_028C2C_S6_X                                               0xFFF0FFFF
#define   S_028C2C_S6_Y(x)                                            (((x) & 0x0F) << 20)
#define   G_028C2C_S6_Y(x)                                            (((x) >> 20) & 0x0F)
#define   C_028C2C_S6_Y                                               0xFF0FFFFF
#define   S_028C2C_S7_X(x)                                            (((x) & 0x0F) << 24)
#define   G_028C2C_S7_X(x)                                            (((x) >> 24) & 0x0F)
#define   C_028C2C_S7_X                                               0xF0FFFFFF
#define   S_028C2C_S7_Y(x)                                            (((x) & 0x0F) << 28)
#define   G_028C2C_S7_Y(x)                                            (((x) >> 28) & 0x0F)
#define   C_028C2C_S7_Y                                               0x0FFFFFFF
#define R_028C30_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_2                      0x028C30
#define   S_028C30_S8_X(x)                                            (((x) & 0x0F) << 0)
#define   G_028C30_S8_X(x)                                            (((x) >> 0) & 0x0F)
#define   C_028C30_S8_X                                               0xFFFFFFF0
#define   S_028C30_S8_Y(x)                                            (((x) & 0x0F) << 4)
#define   G_028C30_S8_Y(x)                                            (((x) >> 4) & 0x0F)
#define   C_028C30_S8_Y                                               0xFFFFFF0F
#define   S_028C30_S9_X(x)                                            (((x) & 0x0F) << 8)
#define   G_028C30_S9_X(x)                                            (((x) >> 8) & 0x0F)
#define   C_028C30_S9_X                                               0xFFFFF0FF
#define   S_028C30_S9_Y(x)                                            (((x) & 0x0F) << 12)
#define   G_028C30_S9_Y(x)                                            (((x) >> 12) & 0x0F)
#define   C_028C30_S9_Y                                               0xFFFF0FFF
#define   S_028C30_S10_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C30_S10_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C30_S10_X                                              0xFFF0FFFF
#define   S_028C30_S10_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C30_S10_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C30_S10_Y                                              0xFF0FFFFF
#define   S_028C30_S11_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C30_S11_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C30_S11_X                                              0xF0FFFFFF
#define   S_028C30_S11_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C30_S11_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C30_S11_Y                                              0x0FFFFFFF
#define R_028C34_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_3                      0x028C34
#define   S_028C34_S12_X(x)                                           (((x) & 0x0F) << 0)
#define   G_028C34_S12_X(x)                                           (((x) >> 0) & 0x0F)
#define   C_028C34_S12_X                                              0xFFFFFFF0
#define   S_028C34_S12_Y(x)                                           (((x) & 0x0F) << 4)
#define   G_028C34_S12_Y(x)                                           (((x) >> 4) & 0x0F)
#define   C_028C34_S12_Y                                              0xFFFFFF0F
#define   S_028C34_S13_X(x)                                           (((x) & 0x0F) << 8)
#define   G_028C34_S13_X(x)                                           (((x) >> 8) & 0x0F)
#define   C_028C34_S13_X                                              0xFFFFF0FF
#define   S_028C34_S13_Y(x)                                           (((x) & 0x0F) << 12)
#define   G_028C34_S13_Y(x)                                           (((x) >> 12) & 0x0F)
#define   C_028C34_S13_Y                                              0xFFFF0FFF
#define   S_028C34_S14_X(x)                                           (((x) & 0x0F) << 16)
#define   G_028C34_S14_X(x)                                           (((x) >> 16) & 0x0F)
#define   C_028C34_S14_X                                              0xFFF0FFFF
#define   S_028C34_S14_Y(x)                                           (((x) & 0x0F) << 20)
#define   G_028C34_S14_Y(x)                                           (((x) >> 20) & 0x0F)
#define   C_028C34_S14_Y                                              0xFF0FFFFF
#define   S_028C34_S15_X(x)                                           (((x) & 0x0F) << 24)
#define   G_028C34_S15_X(x)                                           (((x) >> 24) & 0x0F)
#define   C_028C34_S15_X                                              0xF0FFFFFF
#define   S_028C34_S15_Y(x)                                           (((x) & 0x0F) << 28)
#define   G_028C34_S15_Y(x)                                           (((x) >> 28) & 0x0F)
#define   C_028C34_S15_Y                                              0x0FFFFFFF
#define R_028C38_PA_SC_AA_MASK_X0Y0_X1Y0                                0x028C38
#define   S_028C38_AA_MASK_X0Y0(x)                                    (((x) & 0xFFFF) << 0)
#define   G_028C38_AA_MASK_X0Y0(x)                                    (((x) >> 0) & 0xFFFF)
#define   C_028C38_AA_MASK_X0Y0                                       0xFFFF0000
#define   S_028C38_AA_MASK_X1Y0(x)                                    (((x) & 0xFFFF) << 16)
#define   G_028C38_AA_MASK_X1Y0(x)                                    (((x) >> 16) & 0xFFFF)
#define   C_028C38_AA_MASK_X1Y0                                       0x0000FFFF
#define R_028C3C_PA_SC_AA_MASK_X0Y1_X1Y1                                0x028C3C
#define   S_028C3C_AA_MASK_X0Y1(x)                                    (((x) & 0xFFFF) << 0)
#define   G_028C3C_AA_MASK_X0Y1(x)                                    (((x) >> 0) & 0xFFFF)
#define   C_028C3C_AA_MASK_X0Y1                                       0xFFFF0000
#define   S_028C3C_AA_MASK_X1Y1(x)                                    (((x) & 0xFFFF) << 16)
#define   G_028C3C_AA_MASK_X1Y1(x)                                    (((x) >> 16) & 0xFFFF)
#define   C_028C3C_AA_MASK_X1Y1                                       0x0000FFFF
#define R_028C58_VGT_VERTEX_REUSE_BLOCK_CNTL                            0x028C58
#define   S_028C58_VTX_REUSE_DEPTH(x)                                 (((x) & 0xFF) << 0)
#define   G_028C58_VTX_REUSE_DEPTH(x)                                 (((x) >> 0) & 0xFF)
#define   C_028C58_VTX_REUSE_DEPTH                                    0xFFFFFF00
#define R_028C5C_VGT_OUT_DEALLOC_CNTL                                   0x028C5C
#define   S_028C5C_DEALLOC_DIST(x)                                    (((x) & 0x7F) << 0)
#define   G_028C5C_DEALLOC_DIST(x)                                    (((x) >> 0) & 0x7F)
#define   C_028C5C_DEALLOC_DIST                                       0xFFFFFF80
#define R_028C60_CB_COLOR0_BASE                                         0x028C60
#define R_028C64_CB_COLOR0_PITCH                                        0x028C64
#define   S_028C64_TILE_MAX(x)                                        (((x) & 0x7FF) << 0)
#define   G_028C64_TILE_MAX(x)                                        (((x) >> 0) & 0x7FF)
#define   C_028C64_TILE_MAX                                           0xFFFFF800
#define R_028C68_CB_COLOR0_SLICE                                        0x028C68
#define   S_028C68_TILE_MAX(x)                                        (((x) & 0x3FFFFF) << 0)
#define   G_028C68_TILE_MAX(x)                                        (((x) >> 0) & 0x3FFFFF)
#define   C_028C68_TILE_MAX                                           0xFFC00000
#define R_028C6C_CB_COLOR0_VIEW                                         0x028C6C
#define   S_028C6C_SLICE_START(x)                                     (((x) & 0x7FF) << 0)
#define   G_028C6C_SLICE_START(x)                                     (((x) >> 0) & 0x7FF)
#define   C_028C6C_SLICE_START                                        0xFFFFF800
#define   S_028C6C_SLICE_MAX(x)                                       (((x) & 0x7FF) << 13)
#define   G_028C6C_SLICE_MAX(x)                                       (((x) >> 13) & 0x7FF)
#define   C_028C6C_SLICE_MAX                                          0xFF001FFF
#define R_028C70_CB_COLOR0_INFO                                         0x028C70
#define   S_028C70_ENDIAN(x)                                          (((x) & 0x03) << 0)
#define   G_028C70_ENDIAN(x)                                          (((x) >> 0) & 0x03)
#define   C_028C70_ENDIAN                                             0xFFFFFFFC
#define     V_028C70_ENDIAN_NONE                                    0x00
#define     V_028C70_ENDIAN_8IN16                                   0x01
#define     V_028C70_ENDIAN_8IN32                                   0x02
#define     V_028C70_ENDIAN_8IN64                                   0x03
#define   S_028C70_FORMAT(x)                                          (((x) & 0x1F) << 2)
#define   G_028C70_FORMAT(x)                                          (((x) >> 2) & 0x1F)
#define   C_028C70_FORMAT                                             0xFFFFFF83
#define     V_028C70_COLOR_INVALID                                  0x00
#define     V_028C70_COLOR_8                                        0x01
#define     V_028C70_COLOR_16                                       0x02
#define     V_028C70_COLOR_8_8                                      0x03
#define     V_028C70_COLOR_32                                       0x04
#define     V_028C70_COLOR_16_16                                    0x05
#define     V_028C70_COLOR_10_11_11                                 0x06
#define     V_028C70_COLOR_11_11_10                                 0x07
#define     V_028C70_COLOR_10_10_10_2                               0x08
#define     V_028C70_COLOR_2_10_10_10                               0x09
#define     V_028C70_COLOR_8_8_8_8                                  0x0A
#define     V_028C70_COLOR_32_32                                    0x0B
#define     V_028C70_COLOR_16_16_16_16                              0x0C
#define     V_028C70_COLOR_32_32_32_32                              0x0E
#define     V_028C70_COLOR_5_6_5                                    0x10
#define     V_028C70_COLOR_1_5_5_5                                  0x11
#define     V_028C70_COLOR_5_5_5_1                                  0x12
#define     V_028C70_COLOR_4_4_4_4                                  0x13
#define     V_028C70_COLOR_8_24                                     0x14
#define     V_028C70_COLOR_24_8                                     0x15
#define     V_028C70_COLOR_X24_8_32_FLOAT                           0x16
#define   S_028C70_LINEAR_GENERAL(x)                                  (((x) & 0x1) << 7)
#define   G_028C70_LINEAR_GENERAL(x)                                  (((x) >> 7) & 0x1)
#define   C_028C70_LINEAR_GENERAL                                     0xFFFFFF7F
#define   S_028C70_NUMBER_TYPE(x)                                     (((x) & 0x07) << 8)
#define   G_028C70_NUMBER_TYPE(x)                                     (((x) >> 8) & 0x07)
#define   C_028C70_NUMBER_TYPE                                        0xFFFFF8FF
#define     V_028C70_NUMBER_UNORM                                   0x00
#define     V_028C70_NUMBER_SNORM                                   0x01
#define     V_028C70_NUMBER_UINT                                    0x04
#define     V_028C70_NUMBER_SINT                                    0x05
#define     V_028C70_NUMBER_SRGB                                    0x06
#define     V_028C70_NUMBER_FLOAT                                   0x07
#define   S_028C70_COMP_SWAP(x)                                       (((x) & 0x03) << 11)
#define   G_028C70_COMP_SWAP(x)                                       (((x) >> 11) & 0x03)
#define   C_028C70_COMP_SWAP                                          0xFFFFE7FF
#define     V_028C70_SWAP_STD                                       0x00
#define     V_028C70_SWAP_ALT                                       0x01
#define     V_028C70_SWAP_STD_REV                                   0x02
#define     V_028C70_SWAP_ALT_REV                                   0x03
#define   S_028C70_FAST_CLEAR(x)                                      (((x) & 0x1) << 13)
#define   G_028C70_FAST_CLEAR(x)                                      (((x) >> 13) & 0x1)
#define   C_028C70_FAST_CLEAR                                         0xFFFFDFFF
#define   S_028C70_COMPRESSION(x)                                     (((x) & 0x1) << 14)
#define   G_028C70_COMPRESSION(x)                                     (((x) >> 14) & 0x1)
#define   C_028C70_COMPRESSION                                        0xFFFFBFFF
#define   S_028C70_BLEND_CLAMP(x)                                     (((x) & 0x1) << 15)
#define   G_028C70_BLEND_CLAMP(x)                                     (((x) >> 15) & 0x1)
#define   C_028C70_BLEND_CLAMP                                        0xFFFF7FFF
#define   S_028C70_BLEND_BYPASS(x)                                    (((x) & 0x1) << 16)
#define   G_028C70_BLEND_BYPASS(x)                                    (((x) >> 16) & 0x1)
#define   C_028C70_BLEND_BYPASS                                       0xFFFEFFFF
#define   S_028C70_SIMPLE_FLOAT(x)                                    (((x) & 0x1) << 17)
#define   G_028C70_SIMPLE_FLOAT(x)                                    (((x) >> 17) & 0x1)
#define   C_028C70_SIMPLE_FLOAT                                       0xFFFDFFFF
#define   S_028C70_ROUND_MODE(x)                                      (((x) & 0x1) << 18)
#define   G_028C70_ROUND_MODE(x)                                      (((x) >> 18) & 0x1)
#define   C_028C70_ROUND_MODE                                         0xFFFBFFFF
#define   S_028C70_CMASK_IS_LINEAR(x)                                 (((x) & 0x1) << 19)
#define   G_028C70_CMASK_IS_LINEAR(x)                                 (((x) >> 19) & 0x1)
#define   C_028C70_CMASK_IS_LINEAR                                    0xFFF7FFFF
#define   S_028C70_BLEND_OPT_DONT_RD_DST(x)                           (((x) & 0x07) << 20)
#define   G_028C70_BLEND_OPT_DONT_RD_DST(x)                           (((x) >> 20) & 0x07)
#define   C_028C70_BLEND_OPT_DONT_RD_DST                              0xFF8FFFFF
#define     V_028C70_FORCE_OPT_AUTO                                 0x00
#define     V_028C70_FORCE_OPT_DISABLE                              0x01
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_A_0                    0x02
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_RGB_0                  0x03
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_ARGB_0                 0x04
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_A_1                    0x05
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_RGB_1                  0x06
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_ARGB_1                 0x07
#define   S_028C70_BLEND_OPT_DISCARD_PIXEL(x)                         (((x) & 0x07) << 23)
#define   G_028C70_BLEND_OPT_DISCARD_PIXEL(x)                         (((x) >> 23) & 0x07)
#define   C_028C70_BLEND_OPT_DISCARD_PIXEL                            0xFC7FFFFF
#define     V_028C70_FORCE_OPT_AUTO                                 0x00
#define     V_028C70_FORCE_OPT_DISABLE                              0x01
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_A_0                    0x02
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_RGB_0                  0x03
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_ARGB_0                 0x04
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_A_1                    0x05
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_RGB_1                  0x06
#define     V_028C70_FORCE_OPT_ENABLE_IF_SRC_ARGB_1                 0x07
#define R_028C74_CB_COLOR0_ATTRIB                                       0x028C74
#define   S_028C74_TILE_MODE_INDEX(x)                                 (((x) & 0x1F) << 0)
#define   G_028C74_TILE_MODE_INDEX(x)                                 (((x) >> 0) & 0x1F)
#define   C_028C74_TILE_MODE_INDEX                                    0xFFFFFFE0
#define   S_028C74_FMASK_TILE_MODE_INDEX(x)                           (((x) & 0x1F) << 5)
#define   G_028C74_FMASK_TILE_MODE_INDEX(x)                           (((x) >> 5) & 0x1F)
#define   C_028C74_FMASK_TILE_MODE_INDEX                              0xFFFFFC1F
#define   S_028C74_NUM_SAMPLES(x)                                     (((x) & 0x07) << 12)
#define   G_028C74_NUM_SAMPLES(x)                                     (((x) >> 12) & 0x07)
#define   C_028C74_NUM_SAMPLES                                        0xFFFF8FFF
#define   S_028C74_NUM_FRAGMENTS(x)                                   (((x) & 0x03) << 15)
#define   G_028C74_NUM_FRAGMENTS(x)                                   (((x) >> 15) & 0x03)
#define   C_028C74_NUM_FRAGMENTS                                      0xFFFE7FFF
#define   S_028C74_FORCE_DST_ALPHA_1(x)                               (((x) & 0x1) << 17)
#define   G_028C74_FORCE_DST_ALPHA_1(x)                               (((x) >> 17) & 0x1)
#define   C_028C74_FORCE_DST_ALPHA_1                                  0xFFFDFFFF
#define R_028C7C_CB_COLOR0_CMASK                                        0x028C7C
#define R_028C80_CB_COLOR0_CMASK_SLICE                                  0x028C80
#define   S_028C80_TILE_MAX(x)                                        (((x) & 0x3FFF) << 0)
#define   G_028C80_TILE_MAX(x)                                        (((x) >> 0) & 0x3FFF)
#define   C_028C80_TILE_MAX                                           0xFFFFC000
#define R_028C84_CB_COLOR0_FMASK                                        0x028C84
#define R_028C88_CB_COLOR0_FMASK_SLICE                                  0x028C88
#define   S_028C88_TILE_MAX(x)                                        (((x) & 0x3FFFFF) << 0)
#define   G_028C88_TILE_MAX(x)                                        (((x) >> 0) & 0x3FFFFF)
#define   C_028C88_TILE_MAX                                           0xFFC00000
#define R_028C8C_CB_COLOR0_CLEAR_WORD0                                  0x028C8C
#define R_028C90_CB_COLOR0_CLEAR_WORD1                                  0x028C90
#define R_028C9C_CB_COLOR1_BASE                                         0x028C9C
#define R_028CA0_CB_COLOR1_PITCH                                        0x028CA0
#define R_028CA4_CB_COLOR1_SLICE                                        0x028CA4
#define R_028CA8_CB_COLOR1_VIEW                                         0x028CA8
#define R_028CAC_CB_COLOR1_INFO                                         0x028CAC
#define R_028CB0_CB_COLOR1_ATTRIB                                       0x028CB0
#define R_028CD4_CB_COLOR1_CMASK                                        0x028CB8
#define R_028CBC_CB_COLOR1_CMASK_SLICE                                  0x028CBC
#define R_028CC0_CB_COLOR1_FMASK                                        0x028CC0
#define R_028CC4_CB_COLOR1_FMASK_SLICE                                  0x028CC4
#define R_028CC8_CB_COLOR1_CLEAR_WORD0                                  0x028CC8
#define R_028CCC_CB_COLOR1_CLEAR_WORD1                                  0x028CCC
#define R_028CD8_CB_COLOR2_BASE                                         0x028CD8
#define R_028CDC_CB_COLOR2_PITCH                                        0x028CDC
#define R_028CE0_CB_COLOR2_SLICE                                        0x028CE0
#define R_028CE4_CB_COLOR2_VIEW                                         0x028CE4
#define R_028CE8_CB_COLOR2_INFO                                         0x028CE8
#define R_028CEC_CB_COLOR2_ATTRIB                                       0x028CEC
#define R_028CF4_CB_COLOR2_CMASK                                        0x028CF4
#define R_028CF8_CB_COLOR2_CMASK_SLICE                                  0x028CF8
#define R_028CFC_CB_COLOR2_FMASK                                        0x028CFC
#define R_028D00_CB_COLOR2_FMASK_SLICE                                  0x028D00
#define R_028D04_CB_COLOR2_CLEAR_WORD0                                  0x028D04
#define R_028D08_CB_COLOR2_CLEAR_WORD1                                  0x028D08
#define R_028D14_CB_COLOR3_BASE                                         0x028D14
#define R_028D18_CB_COLOR3_PITCH                                        0x028D18
#define R_028D1C_CB_COLOR3_SLICE                                        0x028D1C
#define R_028D20_CB_COLOR3_VIEW                                         0x028D20
#define R_028D24_CB_COLOR3_INFO                                         0x028D24
#define R_028D28_CB_COLOR3_ATTRIB                                       0x028D28
#define R_028D30_CB_COLOR3_CMASK                                        0x028D30
#define R_028D34_CB_COLOR3_CMASK_SLICE                                  0x028D34
#define R_028D38_CB_COLOR3_FMASK                                        0x028D38
#define R_028D3C_CB_COLOR3_FMASK_SLICE                                  0x028D3C
#define R_028D40_CB_COLOR3_CLEAR_WORD0                                  0x028D40
#define R_028D44_CB_COLOR3_CLEAR_WORD1                                  0x028D44
#define R_028D50_CB_COLOR4_BASE                                         0x028D50
#define R_028D54_CB_COLOR4_PITCH                                        0x028D54
#define R_028D58_CB_COLOR4_SLICE                                        0x028D58
#define R_028D5C_CB_COLOR4_VIEW                                         0x028D5C
#define R_028D60_CB_COLOR4_INFO                                         0x028D60
#define R_028D64_CB_COLOR4_ATTRIB                                       0x028D64
#define R_028D6C_CB_COLOR4_CMASK                                        0x028D6C
#define R_028D70_CB_COLOR4_CMASK_SLICE                                  0x028D70
#define R_028D74_CB_COLOR4_FMASK                                        0x028D74
#define R_028D78_CB_COLOR4_FMASK_SLICE                                  0x028D78
#define R_028D7C_CB_COLOR4_CLEAR_WORD0                                  0x028D7C
#define R_028D80_CB_COLOR4_CLEAR_WORD1                                  0x028D80
#define R_028D8C_CB_COLOR5_BASE                                         0x028D8C
#define R_028D90_CB_COLOR5_PITCH                                        0x028D90
#define R_028D94_CB_COLOR5_SLICE                                        0x028D94
#define R_028D98_CB_COLOR5_VIEW                                         0x028D98
#define R_028D9C_CB_COLOR5_INFO                                         0x028D9C
#define R_028DA0_CB_COLOR5_ATTRIB                                       0x028DA0
#define R_028DA8_CB_COLOR5_CMASK                                        0x028DA8
#define R_028DAC_CB_COLOR5_CMASK_SLICE                                  0x028DAC
#define R_028DB0_CB_COLOR5_FMASK                                        0x028DB0
#define R_028DB4_CB_COLOR5_FMASK_SLICE                                  0x028DB4
#define R_028DB8_CB_COLOR5_CLEAR_WORD0                                  0x028DB8
#define R_028DBC_CB_COLOR5_CLEAR_WORD1                                  0x028DBC
#define R_028DC8_CB_COLOR6_BASE                                         0x028DC8
#define R_028DCC_CB_COLOR6_PITCH                                        0x028DCC
#define R_028DD0_CB_COLOR6_SLICE                                        0x028DD0
#define R_028DD4_CB_COLOR6_VIEW                                         0x028DD4
#define R_028DD8_CB_COLOR6_INFO                                         0x028DD8
#define R_028DDC_CB_COLOR6_ATTRIB                                       0x028DDC
#define R_028DE4_CB_COLOR6_CMASK                                        0x028DE4
#define R_028DE8_CB_COLOR6_CMASK_SLICE                                  0x028DE8
#define R_028DEC_CB_COLOR6_FMASK                                        0x028DEC
#define R_028DF0_CB_COLOR6_FMASK_SLICE                                  0x028DF0
#define R_028DF4_CB_COLOR6_CLEAR_WORD0                                  0x028DF4
#define R_028DF8_CB_COLOR6_CLEAR_WORD1                                  0x028DF8
#define R_028E04_CB_COLOR7_BASE                                         0x028E04
#define R_028E08_CB_COLOR7_PITCH                                        0x028E08
#define R_028E0C_CB_COLOR7_SLICE                                        0x028E0C
#define R_028E10_CB_COLOR7_VIEW                                         0x028E10
#define R_028E14_CB_COLOR7_INFO                                         0x028E14
#define R_028E18_CB_COLOR7_ATTRIB                                       0x028E18
#define R_028E20_CB_COLOR7_CMASK                                        0x028E20
#define R_028E24_CB_COLOR7_CMASK_SLICE                                  0x028E24
#define R_028E28_CB_COLOR7_FMASK                                        0x028E28
#define R_028E2C_CB_COLOR7_FMASK_SLICE                                  0x028E2C
#define R_028E30_CB_COLOR7_CLEAR_WORD0                                  0x028E30
#define R_028E34_CB_COLOR7_CLEAR_WORD1                                  0x028E34

#endif /* _SID_H */

