#ifndef AV1_RTCD_H_
#define AV1_RTCD_H_

#ifdef RTCD_C
#define RTCD_EXTERN
#else
#define RTCD_EXTERN extern
#endif

/*
 * AV1
 */

#include "aom/aom_integer.h"
#include "aom_dsp/txfm_common.h"
#include "av1/common/av1_txfm.h"
#include "av1/common/common.h"
#include "av1/common/convolve.h"
#include "av1/common/enums.h"
#include "av1/common/filter.h"
#include "av1/common/odintrin.h"
#include "av1/common/quant_common.h"

struct macroblockd;

/* Encoder forward decls */
struct macroblock;
struct txfm_param;
struct aom_variance_vtable;
struct search_site_config;
struct mv;
union int_mv;
struct yv12_buffer_config;

#ifdef __cplusplus
extern "C" {
#endif

void aom_clpf_block_c(uint8_t* dst,
                      const uint16_t* src,
                      int dstride,
                      int sstride,
                      int sizex,
                      int sizey,
                      unsigned int strength,
                      unsigned int bd);
#define aom_clpf_block aom_clpf_block_c

void aom_clpf_block_hbd_c(uint16_t* dst,
                          const uint16_t* src,
                          int dstride,
                          int sstride,
                          int sizex,
                          int sizey,
                          unsigned int strength,
                          unsigned int bd);
#define aom_clpf_block_hbd aom_clpf_block_hbd_c

void aom_clpf_hblock_c(uint8_t* dst,
                       const uint16_t* src,
                       int dstride,
                       int sstride,
                       int sizex,
                       int sizey,
                       unsigned int strength,
                       unsigned int bd);
#define aom_clpf_hblock aom_clpf_hblock_c

void aom_clpf_hblock_hbd_c(uint16_t* dst,
                           const uint16_t* src,
                           int dstride,
                           int sstride,
                           int sizex,
                           int sizey,
                           unsigned int strength,
                           unsigned int bd);
#define aom_clpf_hblock_hbd aom_clpf_hblock_hbd_c

void av1_convolve_2d_c(const uint8_t* src,
                       int src_stride,
                       CONV_BUF_TYPE* dst,
                       int dst_stride,
                       int w,
                       int h,
                       InterpFilterParams* filter_params_x,
                       InterpFilterParams* filter_params_y,
                       const int subpel_x_q4,
                       const int subpel_y_q4,
                       ConvolveParams* conv_params);
#define av1_convolve_2d av1_convolve_2d_c

void av1_convolve_2d_scale_c(const uint8_t* src,
                             int src_stride,
                             CONV_BUF_TYPE* dst,
                             int dst_stride,
                             int w,
                             int h,
                             InterpFilterParams* filter_params_x,
                             InterpFilterParams* filter_params_y,
                             const int subpel_x_qn,
                             const int x_step_qn,
                             const int subpel_y_q4,
                             const int y_step_qn,
                             ConvolveParams* conv_params);
#define av1_convolve_2d_scale av1_convolve_2d_scale_c

void av1_convolve_horiz_c(const uint8_t* src,
                          int src_stride,
                          uint8_t* dst,
                          int dst_stride,
                          int w,
                          int h,
                          const InterpFilterParams fp,
                          const int subpel_x_q4,
                          int x_step_q4,
                          ConvolveParams* conv_params);
#define av1_convolve_horiz av1_convolve_horiz_c

void av1_convolve_rounding_c(const int32_t* src,
                             int src_stride,
                             uint8_t* dst,
                             int dst_stride,
                             int w,
                             int h,
                             int bits);
#define av1_convolve_rounding av1_convolve_rounding_c

void av1_convolve_vert_c(const uint8_t* src,
                         int src_stride,
                         uint8_t* dst,
                         int dst_stride,
                         int w,
                         int h,
                         const InterpFilterParams fp,
                         const int subpel_x_q4,
                         int x_step_q4,
                         ConvolveParams* conv_params);
#define av1_convolve_vert av1_convolve_vert_c

void av1_iht16x16_256_add_c(const tran_low_t* input,
                            uint8_t* output,
                            int pitch,
                            const struct txfm_param* param);
#define av1_iht16x16_256_add av1_iht16x16_256_add_c

void av1_iht16x32_512_add_c(const tran_low_t* input,
                            uint8_t* dest,
                            int dest_stride,
                            const struct txfm_param* param);
#define av1_iht16x32_512_add av1_iht16x32_512_add_c

void av1_iht16x4_64_add_c(const tran_low_t* input,
                          uint8_t* dest,
                          int dest_stride,
                          const struct txfm_param* param);
#define av1_iht16x4_64_add av1_iht16x4_64_add_c

void av1_iht16x8_128_add_c(const tran_low_t* input,
                           uint8_t* dest,
                           int dest_stride,
                           const struct txfm_param* param);
#define av1_iht16x8_128_add av1_iht16x8_128_add_c

void av1_iht32x16_512_add_c(const tran_low_t* input,
                            uint8_t* dest,
                            int dest_stride,
                            const struct txfm_param* param);
#define av1_iht32x16_512_add av1_iht32x16_512_add_c

void av1_iht32x32_1024_add_c(const tran_low_t* input,
                             uint8_t* output,
                             int pitch,
                             const struct txfm_param* param);
#define av1_iht32x32_1024_add av1_iht32x32_1024_add_c

void av1_iht32x8_256_add_c(const tran_low_t* input,
                           uint8_t* dest,
                           int dest_stride,
                           const struct txfm_param* param);
#define av1_iht32x8_256_add av1_iht32x8_256_add_c

void av1_iht4x16_64_add_c(const tran_low_t* input,
                          uint8_t* dest,
                          int dest_stride,
                          const struct txfm_param* param);
#define av1_iht4x16_64_add av1_iht4x16_64_add_c

void av1_iht4x4_16_add_c(const tran_low_t* input,
                         uint8_t* dest,
                         int dest_stride,
                         const struct txfm_param* param);
#define av1_iht4x4_16_add av1_iht4x4_16_add_c

void av1_iht4x8_32_add_c(const tran_low_t* input,
                         uint8_t* dest,
                         int dest_stride,
                         const struct txfm_param* param);
#define av1_iht4x8_32_add av1_iht4x8_32_add_c

void av1_iht8x16_128_add_c(const tran_low_t* input,
                           uint8_t* dest,
                           int dest_stride,
                           const struct txfm_param* param);
#define av1_iht8x16_128_add av1_iht8x16_128_add_c

void av1_iht8x32_256_add_c(const tran_low_t* input,
                           uint8_t* dest,
                           int dest_stride,
                           const struct txfm_param* param);
#define av1_iht8x32_256_add av1_iht8x32_256_add_c

void av1_iht8x4_32_add_c(const tran_low_t* input,
                         uint8_t* dest,
                         int dest_stride,
                         const struct txfm_param* param);
#define av1_iht8x4_32_add av1_iht8x4_32_add_c

void av1_iht8x8_64_add_c(const tran_low_t* input,
                         uint8_t* dest,
                         int dest_stride,
                         const struct txfm_param* param);
#define av1_iht8x8_64_add av1_iht8x8_64_add_c

void av1_inv_txfm2d_add_16x16_c(const int32_t* input,
                                uint16_t* output,
                                int stride,
                                int tx_type,
                                int bd);
#define av1_inv_txfm2d_add_16x16 av1_inv_txfm2d_add_16x16_c

void av1_inv_txfm2d_add_16x32_c(const int32_t* input,
                                uint16_t* output,
                                int stride,
                                int tx_type,
                                int bd);
#define av1_inv_txfm2d_add_16x32 av1_inv_txfm2d_add_16x32_c

void av1_inv_txfm2d_add_16x8_c(const int32_t* input,
                               uint16_t* output,
                               int stride,
                               int tx_type,
                               int bd);
#define av1_inv_txfm2d_add_16x8 av1_inv_txfm2d_add_16x8_c

void av1_inv_txfm2d_add_32x16_c(const int32_t* input,
                                uint16_t* output,
                                int stride,
                                int tx_type,
                                int bd);
#define av1_inv_txfm2d_add_32x16 av1_inv_txfm2d_add_32x16_c

void av1_inv_txfm2d_add_32x32_c(const int32_t* input,
                                uint16_t* output,
                                int stride,
                                int tx_type,
                                int bd);
#define av1_inv_txfm2d_add_32x32 av1_inv_txfm2d_add_32x32_c

void av1_inv_txfm2d_add_4x4_c(const int32_t* input,
                              uint16_t* output,
                              int stride,
                              int tx_type,
                              int bd);
#define av1_inv_txfm2d_add_4x4 av1_inv_txfm2d_add_4x4_c

void av1_inv_txfm2d_add_4x8_c(const int32_t* input,
                              uint16_t* output,
                              int stride,
                              int tx_type,
                              int bd);
#define av1_inv_txfm2d_add_4x8 av1_inv_txfm2d_add_4x8_c

void av1_inv_txfm2d_add_64x64_c(const int32_t* input,
                                uint16_t* output,
                                int stride,
                                int tx_type,
                                int bd);
#define av1_inv_txfm2d_add_64x64 av1_inv_txfm2d_add_64x64_c

void av1_inv_txfm2d_add_8x16_c(const int32_t* input,
                               uint16_t* output,
                               int stride,
                               int tx_type,
                               int bd);
#define av1_inv_txfm2d_add_8x16 av1_inv_txfm2d_add_8x16_c

void av1_inv_txfm2d_add_8x4_c(const int32_t* input,
                              uint16_t* output,
                              int stride,
                              int tx_type,
                              int bd);
#define av1_inv_txfm2d_add_8x4 av1_inv_txfm2d_add_8x4_c

void av1_inv_txfm2d_add_8x8_c(const int32_t* input,
                              uint16_t* output,
                              int stride,
                              int tx_type,
                              int bd);
#define av1_inv_txfm2d_add_8x8 av1_inv_txfm2d_add_8x8_c

void av1_lowbd_convolve_init_c(void);
#define av1_lowbd_convolve_init av1_lowbd_convolve_init_c

void av1_warp_affine_c(const int32_t* mat,
                       const uint8_t* ref,
                       int width,
                       int height,
                       int stride,
                       uint8_t* pred,
                       int p_col,
                       int p_row,
                       int p_width,
                       int p_height,
                       int p_stride,
                       int subsampling_x,
                       int subsampling_y,
                       ConvolveParams* conv_params,
                       int16_t alpha,
                       int16_t beta,
                       int16_t gamma,
                       int16_t delta);
#define av1_warp_affine av1_warp_affine_c

void cdef_direction_4x4_c(uint16_t* y,
                          int ystride,
                          const uint16_t* in,
                          int threshold,
                          int dir,
                          int damping);
#define cdef_direction_4x4 cdef_direction_4x4_c

void cdef_direction_8x8_c(uint16_t* y,
                          int ystride,
                          const uint16_t* in,
                          int threshold,
                          int dir,
                          int damping);
#define cdef_direction_8x8 cdef_direction_8x8_c

int cdef_find_dir_c(const uint16_t* img,
                    int stride,
                    int32_t* var,
                    int coeff_shift);
#define cdef_find_dir cdef_find_dir_c

void copy_4x4_16bit_to_16bit_c(uint16_t* dst,
                               int dstride,
                               const uint16_t* src,
                               int sstride);
#define copy_4x4_16bit_to_16bit copy_4x4_16bit_to_16bit_c

void copy_4x4_16bit_to_8bit_c(uint8_t* dst,
                              int dstride,
                              const uint16_t* src,
                              int sstride);
#define copy_4x4_16bit_to_8bit copy_4x4_16bit_to_8bit_c

void copy_8x8_16bit_to_16bit_c(uint16_t* dst,
                               int dstride,
                               const uint16_t* src,
                               int sstride);
#define copy_8x8_16bit_to_16bit copy_8x8_16bit_to_16bit_c

void copy_8x8_16bit_to_8bit_c(uint8_t* dst,
                              int dstride,
                              const uint16_t* src,
                              int sstride);
#define copy_8x8_16bit_to_8bit copy_8x8_16bit_to_8bit_c

void copy_rect8_16bit_to_16bit_c(uint16_t* dst,
                                 int dstride,
                                 const uint16_t* src,
                                 int sstride,
                                 int v,
                                 int h);
#define copy_rect8_16bit_to_16bit copy_rect8_16bit_to_16bit_c

void copy_rect8_8bit_to_16bit_c(uint16_t* dst,
                                int dstride,
                                const uint8_t* src,
                                int sstride,
                                int v,
                                int h);
#define copy_rect8_8bit_to_16bit copy_rect8_8bit_to_16bit_c

void av1_rtcd(void);

#include "aom_config.h"

#ifdef RTCD_C
static void setup_rtcd_internal(void) {}
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
