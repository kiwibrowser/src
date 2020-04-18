/* MIPS MSA intrinsics include file.

   Copyright (C) 2014 Free Software Foundation, Inc.
   Contributed by Imagination Technologies Ltd.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _MSA_H
#define _MSA_H 1

#if defined(__mips_msa)
typedef signed char v16i8 __attribute__((vector_size(16), aligned(16)));
typedef signed char v16i8_b __attribute__((vector_size(16), aligned(1)));
typedef unsigned char v16u8 __attribute__((vector_size(16), aligned(16)));
typedef unsigned char v16u8_b __attribute__((vector_size(16), aligned(1)));
typedef short v8i16 __attribute__((vector_size(16), aligned(16)));
typedef short v8i16_h __attribute__((vector_size(16), aligned(2)));
typedef unsigned short v8u16 __attribute__((vector_size(16), aligned(16)));
typedef unsigned short v8u16_h __attribute__((vector_size(16), aligned(2)));
typedef int v4i32 __attribute__((vector_size(16), aligned(16)));
typedef int v4i32_w __attribute__((vector_size(16), aligned(4)));
typedef unsigned int v4u32 __attribute__((vector_size(16), aligned(16)));
typedef unsigned int v4u32_w __attribute__((vector_size(16), aligned(4)));
typedef long long v2i64 __attribute__((vector_size(16), aligned(16)));
typedef long long v2i64_d __attribute__((vector_size(16), aligned(8)));
typedef unsigned long long v2u64 __attribute__((vector_size(16), aligned(16)));
typedef unsigned long long v2u64_d __attribute__((vector_size(16), aligned(8)));
typedef float v4f32 __attribute__((vector_size(16), aligned(16)));
typedef float v4f32_w __attribute__((vector_size(16), aligned(4)));
typedef double v2f64 __attribute__ ((vector_size(16), aligned(16)));
typedef double v2f64_d __attribute__ ((vector_size(16), aligned(8)));

#ifndef __clang__
extern v16i8 __builtin_msa_sll_b(v16i8, v16i8);
extern v8i16 __builtin_msa_sll_h(v8i16, v8i16);
extern v4i32 __builtin_msa_sll_w(v4i32, v4i32);
extern v2i64 __builtin_msa_sll_d(v2i64, v2i64);
extern v16i8 __builtin_msa_slli_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_slli_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_slli_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_slli_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_sra_b(v16i8, v16i8);
extern v8i16 __builtin_msa_sra_h(v8i16, v8i16);
extern v4i32 __builtin_msa_sra_w(v4i32, v4i32);
extern v2i64 __builtin_msa_sra_d(v2i64, v2i64);
extern v16i8 __builtin_msa_srai_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_srai_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_srai_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_srai_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_srar_b(v16i8, v16i8);
extern v8i16 __builtin_msa_srar_h(v8i16, v8i16);
extern v4i32 __builtin_msa_srar_w(v4i32, v4i32);
extern v2i64 __builtin_msa_srar_d(v2i64, v2i64);
extern v16i8 __builtin_msa_srari_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_srari_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_srari_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_srari_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_srl_b(v16i8, v16i8);
extern v8i16 __builtin_msa_srl_h(v8i16, v8i16);
extern v4i32 __builtin_msa_srl_w(v4i32, v4i32);
extern v2i64 __builtin_msa_srl_d(v2i64, v2i64);
extern v16i8 __builtin_msa_srli_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_srli_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_srli_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_srli_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_srlr_b(v16i8, v16i8);
extern v8i16 __builtin_msa_srlr_h(v8i16, v8i16);
extern v4i32 __builtin_msa_srlr_w(v4i32, v4i32);
extern v2i64 __builtin_msa_srlr_d(v2i64, v2i64);
extern v16i8 __builtin_msa_srlri_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_srlri_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_srlri_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_srlri_d(v2i64, unsigned char);
extern v16u8 __builtin_msa_bclr_b(v16u8, v16u8);
extern v8u16 __builtin_msa_bclr_h(v8u16, v8u16);
extern v4u32 __builtin_msa_bclr_w(v4u32, v4u32);
extern v2u64 __builtin_msa_bclr_d(v2u64, v2u64);
extern v16u8 __builtin_msa_bclri_b(v16u8, unsigned char);
extern v8u16 __builtin_msa_bclri_h(v8u16, unsigned char);
extern v4u32 __builtin_msa_bclri_w(v4u32, unsigned char);
extern v2u64 __builtin_msa_bclri_d(v2u64, unsigned char);
extern v16u8 __builtin_msa_bset_b(v16u8, v16u8);
extern v8u16 __builtin_msa_bset_h(v8u16, v8u16);
extern v4u32 __builtin_msa_bset_w(v4u32, v4u32);
extern v2u64 __builtin_msa_bset_d(v2u64, v2u64);
extern v16u8 __builtin_msa_bseti_b(v16u8, unsigned char);
extern v8u16 __builtin_msa_bseti_h(v8u16, unsigned char);
extern v4u32 __builtin_msa_bseti_w(v4u32, unsigned char);
extern v2u64 __builtin_msa_bseti_d(v2u64, unsigned char);
extern v16u8 __builtin_msa_bneg_b(v16u8, v16u8);
extern v8u16 __builtin_msa_bneg_h(v8u16, v8u16);
extern v4u32 __builtin_msa_bneg_w(v4u32, v4u32);
extern v2u64 __builtin_msa_bneg_d(v2u64, v2u64);
extern v16u8 __builtin_msa_bnegi_b(v16u8, unsigned char);
extern v8u16 __builtin_msa_bnegi_h(v8u16, unsigned char);
extern v4u32 __builtin_msa_bnegi_w(v4u32, unsigned char);
extern v2u64 __builtin_msa_bnegi_d(v2u64, unsigned char);
extern v16u8 __builtin_msa_binsl_b(v16u8, v16u8, v16u8);
extern v8u16 __builtin_msa_binsl_h(v8u16, v8u16, v8u16);
extern v4u32 __builtin_msa_binsl_w(v4u32, v4u32, v4u32);
extern v2u64 __builtin_msa_binsl_d(v2u64, v2u64, v2u64);
extern v16u8 __builtin_msa_binsli_b(v16u8, v16u8, unsigned char);
extern v8u16 __builtin_msa_binsli_h(v8u16, v8u16, unsigned char);
extern v4u32 __builtin_msa_binsli_w(v4u32, v4u32, unsigned char);
extern v2u64 __builtin_msa_binsli_d(v2u64, v2u64, unsigned char);
extern v16u8 __builtin_msa_binsr_b(v16u8, v16u8, v16u8);
extern v8u16 __builtin_msa_binsr_h(v8u16, v8u16, v8u16);
extern v4u32 __builtin_msa_binsr_w(v4u32, v4u32, v4u32);
extern v2u64 __builtin_msa_binsr_d(v2u64, v2u64, v2u64);
extern v16u8 __builtin_msa_binsri_b(v16u8, v16u8, unsigned char);
extern v8u16 __builtin_msa_binsri_h(v8u16, v8u16, unsigned char);
extern v4u32 __builtin_msa_binsri_w(v4u32, v4u32, unsigned char);
extern v2u64 __builtin_msa_binsri_d(v2u64, v2u64, unsigned char);
extern v16i8 __builtin_msa_addv_b(v16i8, v16i8);
extern v8i16 __builtin_msa_addv_h(v8i16, v8i16);
extern v4i32 __builtin_msa_addv_w(v4i32, v4i32);
extern v2i64 __builtin_msa_addv_d(v2i64, v2i64);
extern v16i8 __builtin_msa_addvi_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_addvi_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_addvi_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_addvi_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_subv_b(v16i8, v16i8);
extern v8i16 __builtin_msa_subv_h(v8i16, v8i16);
extern v4i32 __builtin_msa_subv_w(v4i32, v4i32);
extern v2i64 __builtin_msa_subv_d(v2i64, v2i64);
extern v16i8 __builtin_msa_subvi_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_subvi_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_subvi_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_subvi_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_max_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_max_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_max_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_max_s_d(v2i64, v2i64);
extern v16i8 __builtin_msa_maxi_s_b(v16i8, char);
extern v8i16 __builtin_msa_maxi_s_h(v8i16, char);
extern v4i32 __builtin_msa_maxi_s_w(v4i32, char);
extern v2i64 __builtin_msa_maxi_s_d(v2i64, char);
extern v16u8 __builtin_msa_max_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_max_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_max_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_max_u_d(v2u64, v2u64);
extern v16u8 __builtin_msa_maxi_u_b(v16u8, unsigned char);
extern v8u16 __builtin_msa_maxi_u_h(v8u16, unsigned char);
extern v4u32 __builtin_msa_maxi_u_w(v4u32, unsigned char);
extern v2u64 __builtin_msa_maxi_u_d(v2u64, unsigned char);
extern v16i8 __builtin_msa_min_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_min_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_min_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_min_s_d(v2i64, v2i64);
extern v16i8 __builtin_msa_mini_s_b(v16i8, char);
extern v8i16 __builtin_msa_mini_s_h(v8i16, char);
extern v4i32 __builtin_msa_mini_s_w(v4i32, char);
extern v2i64 __builtin_msa_mini_s_d(v2i64, char);
extern v16u8 __builtin_msa_min_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_min_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_min_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_min_u_d(v2u64, v2u64);
extern v16u8 __builtin_msa_mini_u_b(v16u8, unsigned char);
extern v8u16 __builtin_msa_mini_u_h(v8u16, unsigned char);
extern v4u32 __builtin_msa_mini_u_w(v4u32, unsigned char);
extern v2u64 __builtin_msa_mini_u_d(v2u64, unsigned char);
extern v16i8 __builtin_msa_max_a_b(v16i8, v16i8);
extern v8i16 __builtin_msa_max_a_h(v8i16, v8i16);
extern v4i32 __builtin_msa_max_a_w(v4i32, v4i32);
extern v2i64 __builtin_msa_max_a_d(v2i64, v2i64);
extern v16i8 __builtin_msa_min_a_b(v16i8, v16i8);
extern v8i16 __builtin_msa_min_a_h(v8i16, v8i16);
extern v4i32 __builtin_msa_min_a_w(v4i32, v4i32);
extern v2i64 __builtin_msa_min_a_d(v2i64, v2i64);
extern v16i8 __builtin_msa_ceq_b(v16i8, v16i8);
extern v8i16 __builtin_msa_ceq_h(v8i16, v8i16);
extern v4i32 __builtin_msa_ceq_w(v4i32, v4i32);
extern v2i64 __builtin_msa_ceq_d(v2i64, v2i64);
extern v16i8 __builtin_msa_ceqi_b(v16i8, char);
extern v8i16 __builtin_msa_ceqi_h(v8i16, char);
extern v4i32 __builtin_msa_ceqi_w(v4i32, char);
extern v2i64 __builtin_msa_ceqi_d(v2i64, char);
extern v16i8 __builtin_msa_clt_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_clt_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_clt_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_clt_s_d(v2i64, v2i64);
extern v16i8 __builtin_msa_clti_s_b(v16i8, char);
extern v8i16 __builtin_msa_clti_s_h(v8i16, char);
extern v4i32 __builtin_msa_clti_s_w(v4i32, char);
extern v2i64 __builtin_msa_clti_s_d(v2i64, char);
extern v16i8 __builtin_msa_clt_u_b(v16u8, v16u8);
extern v8i16 __builtin_msa_clt_u_h(v8u16, v8u16);
extern v4i32 __builtin_msa_clt_u_w(v4u32, v4u32);
extern v2i64 __builtin_msa_clt_u_d(v2u64, v2u64);
extern v16i8 __builtin_msa_clti_u_b(v16u8, unsigned char);
extern v8i16 __builtin_msa_clti_u_h(v8u16, unsigned char);
extern v4i32 __builtin_msa_clti_u_w(v4u32, unsigned char);
extern v2i64 __builtin_msa_clti_u_d(v2u64, unsigned char);
extern v16i8 __builtin_msa_cle_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_cle_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_cle_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_cle_s_d(v2i64, v2i64);
extern v16i8 __builtin_msa_clei_s_b(v16i8, char);
extern v8i16 __builtin_msa_clei_s_h(v8i16, char);
extern v4i32 __builtin_msa_clei_s_w(v4i32, char);
extern v2i64 __builtin_msa_clei_s_d(v2i64, char);
extern v16i8 __builtin_msa_cle_u_b(v16u8, v16u8);
extern v8i16 __builtin_msa_cle_u_h(v8u16, v8u16);
extern v4i32 __builtin_msa_cle_u_w(v4u32, v4u32);
extern v2i64 __builtin_msa_cle_u_d(v2u64, v2u64);
extern v16i8 __builtin_msa_clei_u_b(v16u8, unsigned char);
extern v8i16 __builtin_msa_clei_u_h(v8u16, unsigned char);
extern v4i32 __builtin_msa_clei_u_w(v4u32, unsigned char);
extern v2i64 __builtin_msa_clei_u_d(v2u64, unsigned char);
extern v16i8 __builtin_msa_ld_b(void *, int);
extern v8i16 __builtin_msa_ld_h(void *, int);
extern v4i32 __builtin_msa_ld_w(void *, int);
extern v2i64 __builtin_msa_ld_d(void *, int);
extern void __builtin_msa_st_b(v16i8, char *, int);
extern void __builtin_msa_st_h(v8i16, char *, int);
extern void __builtin_msa_st_w(v4i32, char *, int);
extern void __builtin_msa_st_d(v2i64, char *, int);
extern v16i8 __builtin_msa_sat_s_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_sat_s_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_sat_s_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_sat_s_d(v2i64, unsigned char);
extern v16u8 __builtin_msa_sat_u_b(v16u8, unsigned char);
extern v8u16 __builtin_msa_sat_u_h(v8u16, unsigned char);
extern v4u32 __builtin_msa_sat_u_w(v4u32, unsigned char);
extern v2u64 __builtin_msa_sat_u_d(v2u64, unsigned char);
extern v16i8 __builtin_msa_add_a_b(v16i8, v16i8);
extern v8i16 __builtin_msa_add_a_h(v8i16, v8i16);
extern v4i32 __builtin_msa_add_a_w(v4i32, v4i32);
extern v2i64 __builtin_msa_add_a_d(v2i64, v2i64);
extern v16i8 __builtin_msa_adds_a_b(v16i8, v16i8);
extern v8i16 __builtin_msa_adds_a_h(v8i16, v8i16);
extern v4i32 __builtin_msa_adds_a_w(v4i32, v4i32);
extern v2i64 __builtin_msa_adds_a_d(v2i64, v2i64);
extern v16i8 __builtin_msa_adds_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_adds_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_adds_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_adds_s_d(v2i64, v2i64);
extern v16u8 __builtin_msa_adds_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_adds_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_adds_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_adds_u_d(v2u64, v2u64);
extern v16i8 __builtin_msa_ave_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_ave_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_ave_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_ave_s_d(v2i64, v2i64);
extern v16u8 __builtin_msa_ave_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_ave_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_ave_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_ave_u_d(v2u64, v2u64);
extern v16i8 __builtin_msa_aver_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_aver_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_aver_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_aver_s_d(v2i64, v2i64);
extern v16u8 __builtin_msa_aver_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_aver_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_aver_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_aver_u_d(v2u64, v2u64);
extern v16i8 __builtin_msa_subs_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_subs_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_subs_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_subs_s_d(v2i64, v2i64);
extern v16u8 __builtin_msa_subs_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_subs_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_subs_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_subs_u_d(v2u64, v2u64);
extern v16i8 __builtin_msa_subsuu_s_b(v16u8, v16u8);
extern v8i16 __builtin_msa_subsuu_s_h(v8u16, v8u16);
extern v4i32 __builtin_msa_subsuu_s_w(v4u32, v4u32);
extern v2i64 __builtin_msa_subsuu_s_d(v2u64, v2u64);
extern v16u8 __builtin_msa_subsus_u_b(v16u8, v16i8);
extern v8u16 __builtin_msa_subsus_u_h(v8u16, v8i16);
extern v4u32 __builtin_msa_subsus_u_w(v4u32, v4i32);
extern v2u64 __builtin_msa_subsus_u_d(v2u64, v2i64);
extern v16i8 __builtin_msa_asub_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_asub_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_asub_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_asub_s_d(v2i64, v2i64);
extern v16u8 __builtin_msa_asub_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_asub_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_asub_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_asub_u_d(v2u64, v2u64);
extern v16i8 __builtin_msa_mulv_b(v16i8, v16i8);
extern v8i16 __builtin_msa_mulv_h(v8i16, v8i16);
extern v4i32 __builtin_msa_mulv_w(v4i32, v4i32);
extern v2i64 __builtin_msa_mulv_d(v2i64, v2i64);
extern v16i8 __builtin_msa_maddv_b(v16i8, v16i8, v16i8);
extern v8i16 __builtin_msa_maddv_h(v8i16, v8i16, v8i16);
extern v4i32 __builtin_msa_maddv_w(v4i32, v4i32, v4i32);
extern v2i64 __builtin_msa_maddv_d(v2i64, v2i64, v2i64);
extern v16i8 __builtin_msa_msubv_b(v16i8, v16i8, v16i8);
extern v8i16 __builtin_msa_msubv_h(v8i16, v8i16, v8i16);
extern v4i32 __builtin_msa_msubv_w(v4i32, v4i32, v4i32);
extern v2i64 __builtin_msa_msubv_d(v2i64, v2i64, v2i64);
extern v16i8 __builtin_msa_div_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_div_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_div_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_div_s_d(v2i64, v2i64);
extern v16u8 __builtin_msa_div_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_div_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_div_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_div_u_d(v2u64, v2u64);
extern v8i16 __builtin_msa_hadd_s_h(v16i8, v16i8);
extern v4i32 __builtin_msa_hadd_s_w(v8i16, v8i16);
extern v2i64 __builtin_msa_hadd_s_d(v4i32, v4i32);
extern v8u16 __builtin_msa_hadd_u_h(v16u8, v16u8);
extern v4u32 __builtin_msa_hadd_u_w(v8u16, v8u16);
extern v2u64 __builtin_msa_hadd_u_d(v4u32, v4u32);
extern v8i16 __builtin_msa_hsub_s_h(v16i8, v16i8);
extern v4i32 __builtin_msa_hsub_s_w(v8i16, v8i16);
extern v2i64 __builtin_msa_hsub_s_d(v4i32, v4i32);
extern v8i16 __builtin_msa_hsub_u_h(v16u8, v16u8);
extern v4i32 __builtin_msa_hsub_u_w(v8u16, v8u16);
extern v2i64 __builtin_msa_hsub_u_d(v4u32, v4u32);
extern v16i8 __builtin_msa_mod_s_b(v16i8, v16i8);
extern v8i16 __builtin_msa_mod_s_h(v8i16, v8i16);
extern v4i32 __builtin_msa_mod_s_w(v4i32, v4i32);
extern v2i64 __builtin_msa_mod_s_d(v2i64, v2i64);
extern v16u8 __builtin_msa_mod_u_b(v16u8, v16u8);
extern v8u16 __builtin_msa_mod_u_h(v8u16, v8u16);
extern v4u32 __builtin_msa_mod_u_w(v4u32, v4u32);
extern v2u64 __builtin_msa_mod_u_d(v2u64, v2u64);
extern v8i16 __builtin_msa_dotp_s_h(v16i8, v16i8);
extern v4i32 __builtin_msa_dotp_s_w(v8i16, v8i16);
extern v2i64 __builtin_msa_dotp_s_d(v4i32, v4i32);
extern v8u16 __builtin_msa_dotp_u_h(v16u8, v16u8);
extern v4u32 __builtin_msa_dotp_u_w(v8u16, v8u16);
extern v2u64 __builtin_msa_dotp_u_d(v4u32, v4u32);
extern v8i16 __builtin_msa_dpadd_s_h(v8i16, v16i8, v16i8);
extern v4i32 __builtin_msa_dpadd_s_w(v4i32, v8i16, v8i16);
extern v2i64 __builtin_msa_dpadd_s_d(v2i64, v4i32, v4i32);
extern v8u16 __builtin_msa_dpadd_u_h(v8u16, v16u8, v16u8);
extern v4u32 __builtin_msa_dpadd_u_w(v4u32, v8u16, v8u16);
extern v2u64 __builtin_msa_dpadd_u_d(v2u64, v4u32, v4u32);
extern v8i16 __builtin_msa_dpsub_s_h(v8i16, v16i8, v16i8);
extern v4i32 __builtin_msa_dpsub_s_w(v4i32, v8i16, v8i16);
extern v2i64 __builtin_msa_dpsub_s_d(v2i64, v4i32, v4i32);
extern v8i16 __builtin_msa_dpsub_u_h(v8i16, v16u8, v16u8);
extern v4i32 __builtin_msa_dpsub_u_w(v4i32, v8u16, v8u16);
extern v2i64 __builtin_msa_dpsub_u_d(v2i64, v4u32, v4u32);
extern v16i8 __builtin_msa_sld_b(v16i8, v16i8, int);
extern v8i16 __builtin_msa_sld_h(v8i16, v8i16, int);
extern v4i32 __builtin_msa_sld_w(v4i32, v4i32, int);
extern v2i64 __builtin_msa_sld_d(v2i64, v2i64, int);
extern v16i8 __builtin_msa_sldi_b(v16i8, v16i8, unsigned char);
extern v8i16 __builtin_msa_sldi_h(v8i16, v8i16, unsigned char);
extern v4i32 __builtin_msa_sldi_w(v4i32, v4i32, unsigned char);
extern v2i64 __builtin_msa_sldi_d(v2i64, v2i64, unsigned char);
extern v16i8 __builtin_msa_splat_b(v16i8, int);
extern v8i16 __builtin_msa_splat_h(v8i16, int);
extern v4i32 __builtin_msa_splat_w(v4i32, int);
extern v2i64 __builtin_msa_splat_d(v2i64, int);
extern v16i8 __builtin_msa_splati_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_splati_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_splati_w(v4i32, unsigned char);
extern v2i64 __builtin_msa_splati_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_pckev_b(v16i8, v16i8);
extern v8i16 __builtin_msa_pckev_h(v8i16, v8i16);
extern v4i32 __builtin_msa_pckev_w(v4i32, v4i32);
extern v2i64 __builtin_msa_pckev_d(v2i64, v2i64);
extern v16i8 __builtin_msa_pckod_b(v16i8, v16i8);
extern v8i16 __builtin_msa_pckod_h(v8i16, v8i16);
extern v4i32 __builtin_msa_pckod_w(v4i32, v4i32);
extern v2i64 __builtin_msa_pckod_d(v2i64, v2i64);
extern v16i8 __builtin_msa_ilvl_b(v16i8, v16i8);
extern v8i16 __builtin_msa_ilvl_h(v8i16, v8i16);
extern v4i32 __builtin_msa_ilvl_w(v4i32, v4i32);
extern v2i64 __builtin_msa_ilvl_d(v2i64, v2i64);
extern v16i8 __builtin_msa_ilvr_b(v16i8, v16i8);
extern v8i16 __builtin_msa_ilvr_h(v8i16, v8i16);
extern v4i32 __builtin_msa_ilvr_w(v4i32, v4i32);
extern v2i64 __builtin_msa_ilvr_d(v2i64, v2i64);
extern v16i8 __builtin_msa_ilvev_b(v16i8, v16i8);
extern v8i16 __builtin_msa_ilvev_h(v8i16, v8i16);
extern v4i32 __builtin_msa_ilvev_w(v4i32, v4i32);
extern v2i64 __builtin_msa_ilvev_d(v2i64, v2i64);
extern v16i8 __builtin_msa_ilvod_b(v16i8, v16i8);
extern v8i16 __builtin_msa_ilvod_h(v8i16, v8i16);
extern v4i32 __builtin_msa_ilvod_w(v4i32, v4i32);
extern v2i64 __builtin_msa_ilvod_d(v2i64, v2i64);
extern v16i8 __builtin_msa_vshf_b(v16i8, v16i8, v16i8);
extern v8i16 __builtin_msa_vshf_h(v8i16, v8i16, v8i16);
extern v4i32 __builtin_msa_vshf_w(v4i32, v4i32, v4i32);
extern v2i64 __builtin_msa_vshf_d(v2i64, v2i64, v2i64);
extern v16u8 __builtin_msa_and_v(v16u8, v16u8);
extern v16u8 __builtin_msa_andi_b(v16u8, unsigned char);
extern v16u8 __builtin_msa_or_v(v16u8, v16u8);
extern v16u8 __builtin_msa_ori_b(v16u8, unsigned char);
extern v16u8 __builtin_msa_nor_v(v16u8, v16u8);
extern v16u8 __builtin_msa_nori_b(v16u8, unsigned char);
extern v16u8 __builtin_msa_xor_v(v16u8, v16u8);
extern v16u8 __builtin_msa_xori_b(v16u8, unsigned char);
extern v16u8 __builtin_msa_bmnz_v(v16u8, v16u8, v16u8);
extern v16u8 __builtin_msa_bmnzi_b(v16u8, v16u8, unsigned char);
extern v16u8 __builtin_msa_bmz_v(v16u8, v16u8, v16u8);
extern v16u8 __builtin_msa_bmzi_b(v16u8, v16u8, unsigned char);
extern v16u8 __builtin_msa_bsel_v(v16u8, v16u8, v16u8);
extern v16u8 __builtin_msa_bseli_b(v16u8, v16u8, unsigned char);
extern v16i8 __builtin_msa_shf_b(v16i8, unsigned char);
extern v8i16 __builtin_msa_shf_h(v8i16, unsigned char);
extern v4i32 __builtin_msa_shf_w(v4i32, unsigned char);
extern int __builtin_msa_bnz_v(v16u8);
extern int __builtin_msa_bz_v(v16u8);
extern v16i8 __builtin_msa_fill_b(int);
extern v8i16 __builtin_msa_fill_h(int);
extern v4i32 __builtin_msa_fill_w(int);
extern v2i64 __builtin_msa_fill_d(long long);
extern v16i8 __builtin_msa_pcnt_b(v16i8);
extern v8i16 __builtin_msa_pcnt_h(v8i16);
extern v4i32 __builtin_msa_pcnt_w(v4i32);
extern v2i64 __builtin_msa_pcnt_d(v2i64);
extern v16i8 __builtin_msa_nloc_b(v16i8);
extern v8i16 __builtin_msa_nloc_h(v8i16);
extern v4i32 __builtin_msa_nloc_w(v4i32);
extern v2i64 __builtin_msa_nloc_d(v2i64);
extern v16i8 __builtin_msa_nlzc_b(v16i8);
extern v8i16 __builtin_msa_nlzc_h(v8i16);
extern v4i32 __builtin_msa_nlzc_w(v4i32);
extern v2i64 __builtin_msa_nlzc_d(v2i64);
extern int __builtin_msa_copy_s_b(v16i8, unsigned char);
extern int __builtin_msa_copy_s_h(v8i16, unsigned char);
extern int __builtin_msa_copy_s_w(v4i32, unsigned char);
extern long long __builtin_msa_copy_s_d(v2i64, unsigned char);
extern int __builtin_msa_copy_u_b(v16i8, unsigned char);
extern int __builtin_msa_copy_u_h(v8i16, unsigned char);
extern int __builtin_msa_copy_u_w(v4i32, unsigned char);
extern long long __builtin_msa_copy_u_d(v2i64, unsigned char);
extern v16i8 __builtin_msa_insert_b(v16i8, unsigned char, int);
extern v8i16 __builtin_msa_insert_h(v8i16, unsigned char, int);
extern v4i32 __builtin_msa_insert_w(v4i32, unsigned char, int);
extern v2i64 __builtin_msa_insert_d(v2i64, unsigned char, long long);
extern v16i8 __builtin_msa_insve_b(v16i8, unsigned char, v16i8);
extern v8i16 __builtin_msa_insve_h(v8i16, unsigned char, v8i16);
extern v4i32 __builtin_msa_insve_w(v4i32, unsigned char, v4i32);
extern v2i64 __builtin_msa_insve_d(v2i64, unsigned char, v2i64);
extern int __builtin_msa_bnz_b(v16u8);
extern int __builtin_msa_bnz_h(v8u16);
extern int __builtin_msa_bnz_w(v4u32);
extern int __builtin_msa_bnz_d(v2u64);
extern int __builtin_msa_bz_b(v16u8);
extern int __builtin_msa_bz_h(v8u16);
extern int __builtin_msa_bz_w(v4u32);
extern int __builtin_msa_bz_d(v2u64);
extern v16i8 __builtin_msa_ldi_b(short);
extern v8i16 __builtin_msa_ldi_h(short);
extern v4i32 __builtin_msa_ldi_w(short);
extern v2i64 __builtin_msa_ldi_d(short);
extern v4i32 __builtin_msa_fcaf_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcaf_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcor_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcor_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcun_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcun_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcune_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcune_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcueq_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcueq_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fceq_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fceq_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcne_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcne_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fclt_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fclt_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcult_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcult_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcle_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcle_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fcule_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fcule_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsaf_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsaf_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsor_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsor_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsun_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsun_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsune_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsune_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsueq_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsueq_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fseq_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fseq_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsne_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsne_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fslt_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fslt_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsult_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsult_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsle_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsle_d(v2f64, v2f64);
extern v4i32 __builtin_msa_fsule_w(v4f32, v4f32);
extern v2i64 __builtin_msa_fsule_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fadd_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fadd_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fsub_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fsub_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fmul_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fmul_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fdiv_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fdiv_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fmadd_w(v4f32, v4f32, v4f32);
extern v2f64 __builtin_msa_fmadd_d(v2f64, v2f64, v2f64);
extern v4f32 __builtin_msa_fmsub_w(v4f32, v4f32, v4f32);
extern v2f64 __builtin_msa_fmsub_d(v2f64, v2f64, v2f64);
extern v4f32 __builtin_msa_fexp2_w(v4f32, v4i32);
extern v2f64 __builtin_msa_fexp2_d(v2f64, v2i64);
extern v8i16 __builtin_msa_fexdo_h(v4f32, v4f32);
extern v4f32 __builtin_msa_fexdo_w(v2f64, v2f64);
extern v8i16 __builtin_msa_ftq_h(v4f32, v4f32);
extern v4i32 __builtin_msa_ftq_w(v2f64, v2f64);
extern v4f32 __builtin_msa_fmin_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fmin_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fmin_a_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fmin_a_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fmax_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fmax_d(v2f64, v2f64);
extern v4f32 __builtin_msa_fmax_a_w(v4f32, v4f32);
extern v2f64 __builtin_msa_fmax_a_d(v2f64, v2f64);
extern v8i16 __builtin_msa_mul_q_h(v8i16, v8i16);
extern v4i32 __builtin_msa_mul_q_w(v4i32, v4i32);
extern v8i16 __builtin_msa_mulr_q_h(v8i16, v8i16);
extern v4i32 __builtin_msa_mulr_q_w(v4i32, v4i32);
extern v8i16 __builtin_msa_madd_q_h(v8i16, v8i16, v8i16);
extern v4i32 __builtin_msa_madd_q_w(v4i32, v4i32, v4i32);
extern v8i16 __builtin_msa_maddr_q_h(v8i16, v8i16, v8i16);
extern v4i32 __builtin_msa_maddr_q_w(v4i32, v4i32, v4i32);
extern v8i16 __builtin_msa_msub_q_h(v8i16, v8i16, v8i16);
extern v4i32 __builtin_msa_msub_q_w(v4i32, v4i32, v4i32);
extern v8i16 __builtin_msa_msubr_q_h(v8i16, v8i16, v8i16);
extern v4i32 __builtin_msa_msubr_q_w(v4i32, v4i32, v4i32);
extern v4i32 __builtin_msa_fclass_w(v4f32);
extern v2i64 __builtin_msa_fclass_d(v2f64);
extern v4f32 __builtin_msa_fsqrt_w(v4f32);
extern v2f64 __builtin_msa_fsqrt_d(v2f64);
extern v4f32 __builtin_msa_frcp_w(v4f32);
extern v2f64 __builtin_msa_frcp_d(v2f64);
extern v4f32 __builtin_msa_frint_w(v4f32);
extern v2f64 __builtin_msa_frint_d(v2f64);
extern v4f32 __builtin_msa_frsqrt_w(v4f32);
extern v2f64 __builtin_msa_frsqrt_d(v2f64);
extern v4f32 __builtin_msa_flog2_w(v4f32);
extern v2f64 __builtin_msa_flog2_d(v2f64);
extern v4f32 __builtin_msa_fexupl_w(v8i16);
extern v2f64 __builtin_msa_fexupl_d(v4f32);
extern v4f32 __builtin_msa_fexupr_w(v8i16);
extern v2f64 __builtin_msa_fexupr_d(v4f32);
extern v4f32 __builtin_msa_ffql_w(v8i16);
extern v2f64 __builtin_msa_ffql_d(v4i32);
extern v4f32 __builtin_msa_ffqr_w(v8i16);
extern v2f64 __builtin_msa_ffqr_d(v4i32);
extern v4i32 __builtin_msa_ftint_s_w(v4f32);
extern v2i64 __builtin_msa_ftint_s_d(v2f64);
extern v4u32 __builtin_msa_ftint_u_w(v4f32);
extern v2u64 __builtin_msa_ftint_u_d(v2f64);
extern v4i32 __builtin_msa_ftrunc_s_w(v4f32);
extern v2i64 __builtin_msa_ftrunc_s_d(v2f64);
extern v4u32 __builtin_msa_ftrunc_u_w(v4f32);
extern v2u64 __builtin_msa_ftrunc_u_d(v2f64);
extern v4f32 __builtin_msa_ffint_s_w(v4i32);
extern v2f64 __builtin_msa_ffint_s_d(v2i64);
extern v4f32 __builtin_msa_ffint_u_w(v4u32);
extern v2f64 __builtin_msa_ffint_u_d(v2u64);
extern int __builtin_msa_cfcmsa(unsigned char);
extern v16i8 __builtin_msa_move_v(v16i8);
extern v4f32 __builtin_msa_cast_to_vector_float(float);
extern v2f64 __builtin_msa_cast_to_vector_double(double);
extern float __builtin_msa_cast_to_scalar_float(v4f32);
extern double __builtin_msa_cast_to_scalar_double(v2f64);
#endif /* __clang__ */
#define __msa_sll_b __builtin_msa_sll_b
#define __msa_sll_h __builtin_msa_sll_h
#define __msa_sll_w __builtin_msa_sll_w
#define __msa_sll_d __builtin_msa_sll_d
#define __msa_slli_b __builtin_msa_slli_b
#define __msa_slli_h __builtin_msa_slli_h
#define __msa_slli_w __builtin_msa_slli_w
#define __msa_slli_d __builtin_msa_slli_d
#define __msa_sra_b __builtin_msa_sra_b
#define __msa_sra_h __builtin_msa_sra_h
#define __msa_sra_w __builtin_msa_sra_w
#define __msa_sra_d __builtin_msa_sra_d
#define __msa_srai_b __builtin_msa_srai_b
#define __msa_srai_h __builtin_msa_srai_h
#define __msa_srai_w __builtin_msa_srai_w
#define __msa_srai_d __builtin_msa_srai_d
#define __msa_srar_b __builtin_msa_srar_b
#define __msa_srar_h __builtin_msa_srar_h
#define __msa_srar_w __builtin_msa_srar_w
#define __msa_srar_d __builtin_msa_srar_d
#define __msa_srari_b __builtin_msa_srari_b
#define __msa_srari_h __builtin_msa_srari_h
#define __msa_srari_w __builtin_msa_srari_w
#define __msa_srari_d __builtin_msa_srari_d
#define __msa_srl_b __builtin_msa_srl_b
#define __msa_srl_h __builtin_msa_srl_h
#define __msa_srl_w __builtin_msa_srl_w
#define __msa_srl_d __builtin_msa_srl_d
#define __msa_srli_b __builtin_msa_srli_b
#define __msa_srli_h __builtin_msa_srli_h
#define __msa_srli_w __builtin_msa_srli_w
#define __msa_srli_d __builtin_msa_srli_d
#define __msa_srlr_b __builtin_msa_srlr_b
#define __msa_srlr_h __builtin_msa_srlr_h
#define __msa_srlr_w __builtin_msa_srlr_w
#define __msa_srlr_d __builtin_msa_srlr_d
#define __msa_srlri_b __builtin_msa_srlri_b
#define __msa_srlri_h __builtin_msa_srlri_h
#define __msa_srlri_w __builtin_msa_srlri_w
#define __msa_srlri_d __builtin_msa_srlri_d
#define __msa_bclr_b __builtin_msa_bclr_b
#define __msa_bclr_h __builtin_msa_bclr_h
#define __msa_bclr_w __builtin_msa_bclr_w
#define __msa_bclr_d __builtin_msa_bclr_d
#define __msa_bclri_b __builtin_msa_bclri_b
#define __msa_bclri_h __builtin_msa_bclri_h
#define __msa_bclri_w __builtin_msa_bclri_w
#define __msa_bclri_d __builtin_msa_bclri_d
#define __msa_bset_b __builtin_msa_bset_b
#define __msa_bset_h __builtin_msa_bset_h
#define __msa_bset_w __builtin_msa_bset_w
#define __msa_bset_d __builtin_msa_bset_d
#define __msa_bseti_b __builtin_msa_bseti_b
#define __msa_bseti_h __builtin_msa_bseti_h
#define __msa_bseti_w __builtin_msa_bseti_w
#define __msa_bseti_d __builtin_msa_bseti_d
#define __msa_bneg_b __builtin_msa_bneg_b
#define __msa_bneg_h __builtin_msa_bneg_h
#define __msa_bneg_w __builtin_msa_bneg_w
#define __msa_bneg_d __builtin_msa_bneg_d
#define __msa_bnegi_b __builtin_msa_bnegi_b
#define __msa_bnegi_h __builtin_msa_bnegi_h
#define __msa_bnegi_w __builtin_msa_bnegi_w
#define __msa_bnegi_d __builtin_msa_bnegi_d
#define __msa_binsl_b __builtin_msa_binsl_b
#define __msa_binsl_h __builtin_msa_binsl_h
#define __msa_binsl_w __builtin_msa_binsl_w
#define __msa_binsl_d __builtin_msa_binsl_d
#define __msa_binsli_b __builtin_msa_binsli_b
#define __msa_binsli_h __builtin_msa_binsli_h
#define __msa_binsli_w __builtin_msa_binsli_w
#define __msa_binsli_d __builtin_msa_binsli_d
#define __msa_binsr_b __builtin_msa_binsr_b
#define __msa_binsr_h __builtin_msa_binsr_h
#define __msa_binsr_w __builtin_msa_binsr_w
#define __msa_binsr_d __builtin_msa_binsr_d
#define __msa_binsri_b __builtin_msa_binsri_b
#define __msa_binsri_h __builtin_msa_binsri_h
#define __msa_binsri_w __builtin_msa_binsri_w
#define __msa_binsri_d __builtin_msa_binsri_d
#define __msa_addv_b __builtin_msa_addv_b
#define __msa_addv_h __builtin_msa_addv_h
#define __msa_addv_w __builtin_msa_addv_w
#define __msa_addv_d __builtin_msa_addv_d
#define __msa_addvi_b __builtin_msa_addvi_b
#define __msa_addvi_h __builtin_msa_addvi_h
#define __msa_addvi_w __builtin_msa_addvi_w
#define __msa_addvi_d __builtin_msa_addvi_d
#define __msa_subv_b __builtin_msa_subv_b
#define __msa_subv_h __builtin_msa_subv_h
#define __msa_subv_w __builtin_msa_subv_w
#define __msa_subv_d __builtin_msa_subv_d
#define __msa_subvi_b __builtin_msa_subvi_b
#define __msa_subvi_h __builtin_msa_subvi_h
#define __msa_subvi_w __builtin_msa_subvi_w
#define __msa_subvi_d __builtin_msa_subvi_d
#define __msa_max_s_b __builtin_msa_max_s_b
#define __msa_max_s_h __builtin_msa_max_s_h
#define __msa_max_s_w __builtin_msa_max_s_w
#define __msa_max_s_d __builtin_msa_max_s_d
#define __msa_maxi_s_b __builtin_msa_maxi_s_b
#define __msa_maxi_s_h __builtin_msa_maxi_s_h
#define __msa_maxi_s_w __builtin_msa_maxi_s_w
#define __msa_maxi_s_d __builtin_msa_maxi_s_d
#define __msa_max_u_b __builtin_msa_max_u_b
#define __msa_max_u_h __builtin_msa_max_u_h
#define __msa_max_u_w __builtin_msa_max_u_w
#define __msa_max_u_d __builtin_msa_max_u_d
#define __msa_maxi_u_b __builtin_msa_maxi_u_b
#define __msa_maxi_u_h __builtin_msa_maxi_u_h
#define __msa_maxi_u_w __builtin_msa_maxi_u_w
#define __msa_maxi_u_d __builtin_msa_maxi_u_d
#define __msa_min_s_b __builtin_msa_min_s_b
#define __msa_min_s_h __builtin_msa_min_s_h
#define __msa_min_s_w __builtin_msa_min_s_w
#define __msa_min_s_d __builtin_msa_min_s_d
#define __msa_mini_s_b __builtin_msa_mini_s_b
#define __msa_mini_s_h __builtin_msa_mini_s_h
#define __msa_mini_s_w __builtin_msa_mini_s_w
#define __msa_mini_s_d __builtin_msa_mini_s_d
#define __msa_min_u_b __builtin_msa_min_u_b
#define __msa_min_u_h __builtin_msa_min_u_h
#define __msa_min_u_w __builtin_msa_min_u_w
#define __msa_min_u_d __builtin_msa_min_u_d
#define __msa_mini_u_b __builtin_msa_mini_u_b
#define __msa_mini_u_h __builtin_msa_mini_u_h
#define __msa_mini_u_w __builtin_msa_mini_u_w
#define __msa_mini_u_d __builtin_msa_mini_u_d
#define __msa_max_a_b __builtin_msa_max_a_b
#define __msa_max_a_h __builtin_msa_max_a_h
#define __msa_max_a_w __builtin_msa_max_a_w
#define __msa_max_a_d __builtin_msa_max_a_d
#define __msa_min_a_b __builtin_msa_min_a_b
#define __msa_min_a_h __builtin_msa_min_a_h
#define __msa_min_a_w __builtin_msa_min_a_w
#define __msa_min_a_d __builtin_msa_min_a_d
#define __msa_ceq_b __builtin_msa_ceq_b
#define __msa_ceq_h __builtin_msa_ceq_h
#define __msa_ceq_w __builtin_msa_ceq_w
#define __msa_ceq_d __builtin_msa_ceq_d
#define __msa_ceqi_b __builtin_msa_ceqi_b
#define __msa_ceqi_h __builtin_msa_ceqi_h
#define __msa_ceqi_w __builtin_msa_ceqi_w
#define __msa_ceqi_d __builtin_msa_ceqi_d
#define __msa_clt_s_b __builtin_msa_clt_s_b
#define __msa_clt_s_h __builtin_msa_clt_s_h
#define __msa_clt_s_w __builtin_msa_clt_s_w
#define __msa_clt_s_d __builtin_msa_clt_s_d
#define __msa_clti_s_b __builtin_msa_clti_s_b
#define __msa_clti_s_h __builtin_msa_clti_s_h
#define __msa_clti_s_w __builtin_msa_clti_s_w
#define __msa_clti_s_d __builtin_msa_clti_s_d
#define __msa_clt_u_b __builtin_msa_clt_u_b
#define __msa_clt_u_h __builtin_msa_clt_u_h
#define __msa_clt_u_w __builtin_msa_clt_u_w
#define __msa_clt_u_d __builtin_msa_clt_u_d
#define __msa_clti_u_b __builtin_msa_clti_u_b
#define __msa_clti_u_h __builtin_msa_clti_u_h
#define __msa_clti_u_w __builtin_msa_clti_u_w
#define __msa_clti_u_d __builtin_msa_clti_u_d
#define __msa_cle_s_b __builtin_msa_cle_s_b
#define __msa_cle_s_h __builtin_msa_cle_s_h
#define __msa_cle_s_w __builtin_msa_cle_s_w
#define __msa_cle_s_d __builtin_msa_cle_s_d
#define __msa_clei_s_b __builtin_msa_clei_s_b
#define __msa_clei_s_h __builtin_msa_clei_s_h
#define __msa_clei_s_w __builtin_msa_clei_s_w
#define __msa_clei_s_d __builtin_msa_clei_s_d
#define __msa_cle_u_b __builtin_msa_cle_u_b
#define __msa_cle_u_h __builtin_msa_cle_u_h
#define __msa_cle_u_w __builtin_msa_cle_u_w
#define __msa_cle_u_d __builtin_msa_cle_u_d
#define __msa_clei_u_b __builtin_msa_clei_u_b
#define __msa_clei_u_h __builtin_msa_clei_u_h
#define __msa_clei_u_w __builtin_msa_clei_u_w
#define __msa_clei_u_d __builtin_msa_clei_u_d
#define __msa_ld_b __builtin_msa_ld_b
#define __msa_ld_h __builtin_msa_ld_h
#define __msa_ld_w __builtin_msa_ld_w
#define __msa_ld_d __builtin_msa_ld_d
#define __msa_st_b __builtin_msa_st_b
#define __msa_st_h __builtin_msa_st_h
#define __msa_st_w __builtin_msa_st_w
#define __msa_st_d __builtin_msa_st_d
#define __msa_sat_s_b __builtin_msa_sat_s_b
#define __msa_sat_s_h __builtin_msa_sat_s_h
#define __msa_sat_s_w __builtin_msa_sat_s_w
#define __msa_sat_s_d __builtin_msa_sat_s_d
#define __msa_sat_u_b __builtin_msa_sat_u_b
#define __msa_sat_u_h __builtin_msa_sat_u_h
#define __msa_sat_u_w __builtin_msa_sat_u_w
#define __msa_sat_u_d __builtin_msa_sat_u_d
#define __msa_add_a_b __builtin_msa_add_a_b
#define __msa_add_a_h __builtin_msa_add_a_h
#define __msa_add_a_w __builtin_msa_add_a_w
#define __msa_add_a_d __builtin_msa_add_a_d
#define __msa_adds_a_b __builtin_msa_adds_a_b
#define __msa_adds_a_h __builtin_msa_adds_a_h
#define __msa_adds_a_w __builtin_msa_adds_a_w
#define __msa_adds_a_d __builtin_msa_adds_a_d
#define __msa_adds_s_b __builtin_msa_adds_s_b
#define __msa_adds_s_h __builtin_msa_adds_s_h
#define __msa_adds_s_w __builtin_msa_adds_s_w
#define __msa_adds_s_d __builtin_msa_adds_s_d
#define __msa_adds_u_b __builtin_msa_adds_u_b
#define __msa_adds_u_h __builtin_msa_adds_u_h
#define __msa_adds_u_w __builtin_msa_adds_u_w
#define __msa_adds_u_d __builtin_msa_adds_u_d
#define __msa_ave_s_b __builtin_msa_ave_s_b
#define __msa_ave_s_h __builtin_msa_ave_s_h
#define __msa_ave_s_w __builtin_msa_ave_s_w
#define __msa_ave_s_d __builtin_msa_ave_s_d
#define __msa_ave_u_b __builtin_msa_ave_u_b
#define __msa_ave_u_h __builtin_msa_ave_u_h
#define __msa_ave_u_w __builtin_msa_ave_u_w
#define __msa_ave_u_d __builtin_msa_ave_u_d
#define __msa_aver_s_b __builtin_msa_aver_s_b
#define __msa_aver_s_h __builtin_msa_aver_s_h
#define __msa_aver_s_w __builtin_msa_aver_s_w
#define __msa_aver_s_d __builtin_msa_aver_s_d
#define __msa_aver_u_b __builtin_msa_aver_u_b
#define __msa_aver_u_h __builtin_msa_aver_u_h
#define __msa_aver_u_w __builtin_msa_aver_u_w
#define __msa_aver_u_d __builtin_msa_aver_u_d
#define __msa_subs_s_b __builtin_msa_subs_s_b
#define __msa_subs_s_h __builtin_msa_subs_s_h
#define __msa_subs_s_w __builtin_msa_subs_s_w
#define __msa_subs_s_d __builtin_msa_subs_s_d
#define __msa_subs_u_b __builtin_msa_subs_u_b
#define __msa_subs_u_h __builtin_msa_subs_u_h
#define __msa_subs_u_w __builtin_msa_subs_u_w
#define __msa_subs_u_d __builtin_msa_subs_u_d
#define __msa_subsuu_s_b __builtin_msa_subsuu_s_b
#define __msa_subsuu_s_h __builtin_msa_subsuu_s_h
#define __msa_subsuu_s_w __builtin_msa_subsuu_s_w
#define __msa_subsuu_s_d __builtin_msa_subsuu_s_d
#define __msa_subsus_u_b __builtin_msa_subsus_u_b
#define __msa_subsus_u_h __builtin_msa_subsus_u_h
#define __msa_subsus_u_w __builtin_msa_subsus_u_w
#define __msa_subsus_u_d __builtin_msa_subsus_u_d
#define __msa_asub_s_b __builtin_msa_asub_s_b
#define __msa_asub_s_h __builtin_msa_asub_s_h
#define __msa_asub_s_w __builtin_msa_asub_s_w
#define __msa_asub_s_d __builtin_msa_asub_s_d
#define __msa_asub_u_b __builtin_msa_asub_u_b
#define __msa_asub_u_h __builtin_msa_asub_u_h
#define __msa_asub_u_w __builtin_msa_asub_u_w
#define __msa_asub_u_d __builtin_msa_asub_u_d
#define __msa_mulv_b __builtin_msa_mulv_b
#define __msa_mulv_h __builtin_msa_mulv_h
#define __msa_mulv_w __builtin_msa_mulv_w
#define __msa_mulv_d __builtin_msa_mulv_d
#define __msa_maddv_b __builtin_msa_maddv_b
#define __msa_maddv_h __builtin_msa_maddv_h
#define __msa_maddv_w __builtin_msa_maddv_w
#define __msa_maddv_d __builtin_msa_maddv_d
#define __msa_msubv_b __builtin_msa_msubv_b
#define __msa_msubv_h __builtin_msa_msubv_h
#define __msa_msubv_w __builtin_msa_msubv_w
#define __msa_msubv_d __builtin_msa_msubv_d
#define __msa_div_s_b __builtin_msa_div_s_b
#define __msa_div_s_h __builtin_msa_div_s_h
#define __msa_div_s_w __builtin_msa_div_s_w
#define __msa_div_s_d __builtin_msa_div_s_d
#define __msa_div_u_b __builtin_msa_div_u_b
#define __msa_div_u_h __builtin_msa_div_u_h
#define __msa_div_u_w __builtin_msa_div_u_w
#define __msa_div_u_d __builtin_msa_div_u_d
#define __msa_hadd_s_h __builtin_msa_hadd_s_h
#define __msa_hadd_s_w __builtin_msa_hadd_s_w
#define __msa_hadd_s_d __builtin_msa_hadd_s_d
#define __msa_hadd_u_h __builtin_msa_hadd_u_h
#define __msa_hadd_u_w __builtin_msa_hadd_u_w
#define __msa_hadd_u_d __builtin_msa_hadd_u_d
#define __msa_hsub_s_h __builtin_msa_hsub_s_h
#define __msa_hsub_s_w __builtin_msa_hsub_s_w
#define __msa_hsub_s_d __builtin_msa_hsub_s_d
#define __msa_hsub_u_h __builtin_msa_hsub_u_h
#define __msa_hsub_u_w __builtin_msa_hsub_u_w
#define __msa_hsub_u_d __builtin_msa_hsub_u_d
#define __msa_mod_s_b __builtin_msa_mod_s_b
#define __msa_mod_s_h __builtin_msa_mod_s_h
#define __msa_mod_s_w __builtin_msa_mod_s_w
#define __msa_mod_s_d __builtin_msa_mod_s_d
#define __msa_mod_u_b __builtin_msa_mod_u_b
#define __msa_mod_u_h __builtin_msa_mod_u_h
#define __msa_mod_u_w __builtin_msa_mod_u_w
#define __msa_mod_u_d __builtin_msa_mod_u_d
#define __msa_dotp_s_h __builtin_msa_dotp_s_h
#define __msa_dotp_s_w __builtin_msa_dotp_s_w
#define __msa_dotp_s_d __builtin_msa_dotp_s_d
#define __msa_dotp_u_h __builtin_msa_dotp_u_h
#define __msa_dotp_u_w __builtin_msa_dotp_u_w
#define __msa_dotp_u_d __builtin_msa_dotp_u_d
#define __msa_dpadd_s_h __builtin_msa_dpadd_s_h
#define __msa_dpadd_s_w __builtin_msa_dpadd_s_w
#define __msa_dpadd_s_d __builtin_msa_dpadd_s_d
#define __msa_dpadd_u_h __builtin_msa_dpadd_u_h
#define __msa_dpadd_u_w __builtin_msa_dpadd_u_w
#define __msa_dpadd_u_d __builtin_msa_dpadd_u_d
#define __msa_dpsub_s_h __builtin_msa_dpsub_s_h
#define __msa_dpsub_s_w __builtin_msa_dpsub_s_w
#define __msa_dpsub_s_d __builtin_msa_dpsub_s_d
#define __msa_dpsub_u_h __builtin_msa_dpsub_u_h
#define __msa_dpsub_u_w __builtin_msa_dpsub_u_w
#define __msa_dpsub_u_d __builtin_msa_dpsub_u_d
#define __msa_sld_b __builtin_msa_sld_b
#define __msa_sld_h __builtin_msa_sld_h
#define __msa_sld_w __builtin_msa_sld_w
#define __msa_sld_d __builtin_msa_sld_d
#define __msa_sldi_b __builtin_msa_sldi_b
#define __msa_sldi_h __builtin_msa_sldi_h
#define __msa_sldi_w __builtin_msa_sldi_w
#define __msa_sldi_d __builtin_msa_sldi_d
#define __msa_splat_b __builtin_msa_splat_b
#define __msa_splat_h __builtin_msa_splat_h
#define __msa_splat_w __builtin_msa_splat_w
#define __msa_splat_d __builtin_msa_splat_d
#define __msa_splati_b __builtin_msa_splati_b
#define __msa_splati_h __builtin_msa_splati_h
#define __msa_splati_w __builtin_msa_splati_w
#define __msa_splati_d __builtin_msa_splati_d
#define __msa_pckev_b __builtin_msa_pckev_b
#define __msa_pckev_h __builtin_msa_pckev_h
#define __msa_pckev_w __builtin_msa_pckev_w
#define __msa_pckev_d __builtin_msa_pckev_d
#define __msa_pckod_b __builtin_msa_pckod_b
#define __msa_pckod_h __builtin_msa_pckod_h
#define __msa_pckod_w __builtin_msa_pckod_w
#define __msa_pckod_d __builtin_msa_pckod_d
#define __msa_ilvl_b __builtin_msa_ilvl_b
#define __msa_ilvl_h __builtin_msa_ilvl_h
#define __msa_ilvl_w __builtin_msa_ilvl_w
#define __msa_ilvl_d __builtin_msa_ilvl_d
#define __msa_ilvr_b __builtin_msa_ilvr_b
#define __msa_ilvr_h __builtin_msa_ilvr_h
#define __msa_ilvr_w __builtin_msa_ilvr_w
#define __msa_ilvr_d __builtin_msa_ilvr_d
#define __msa_ilvev_b __builtin_msa_ilvev_b
#define __msa_ilvev_h __builtin_msa_ilvev_h
#define __msa_ilvev_w __builtin_msa_ilvev_w
#define __msa_ilvev_d __builtin_msa_ilvev_d
#define __msa_ilvod_b __builtin_msa_ilvod_b
#define __msa_ilvod_h __builtin_msa_ilvod_h
#define __msa_ilvod_w __builtin_msa_ilvod_w
#define __msa_ilvod_d __builtin_msa_ilvod_d
#define __msa_vshf_b __builtin_msa_vshf_b
#define __msa_vshf_h __builtin_msa_vshf_h
#define __msa_vshf_w __builtin_msa_vshf_w
#define __msa_vshf_d __builtin_msa_vshf_d
#define __msa_and_v __builtin_msa_and_v
#define __msa_andi_b __builtin_msa_andi_b
#define __msa_or_v __builtin_msa_or_v
#define __msa_ori_b __builtin_msa_ori_b
#define __msa_nor_v __builtin_msa_nor_v
#define __msa_nori_b __builtin_msa_nori_b
#define __msa_xor_v __builtin_msa_xor_v
#define __msa_xori_b __builtin_msa_xori_b
#define __msa_bmnz_v __builtin_msa_bmnz_v
#define __msa_bmnzi_b __builtin_msa_bmnzi_b
#define __msa_bmz_v __builtin_msa_bmz_v
#define __msa_bmzi_b __builtin_msa_bmzi_b
#define __msa_bsel_v __builtin_msa_bsel_v
#define __msa_bseli_b __builtin_msa_bseli_b
#define __msa_shf_b __builtin_msa_shf_b
#define __msa_shf_h __builtin_msa_shf_h
#define __msa_shf_w __builtin_msa_shf_w
#define __msa_test_bnz_v __builtin_msa_bnz_v
#define __msa_test_bz_v __builtin_msa_bz_v
#define __msa_fill_b __builtin_msa_fill_b
#define __msa_fill_h __builtin_msa_fill_h
#define __msa_fill_w __builtin_msa_fill_w
#define __msa_fill_d __builtin_msa_fill_d
#define __msa_pcnt_b __builtin_msa_pcnt_b
#define __msa_pcnt_h __builtin_msa_pcnt_h
#define __msa_pcnt_w __builtin_msa_pcnt_w
#define __msa_pcnt_d __builtin_msa_pcnt_d
#define __msa_nloc_b __builtin_msa_nloc_b
#define __msa_nloc_h __builtin_msa_nloc_h
#define __msa_nloc_w __builtin_msa_nloc_w
#define __msa_nloc_d __builtin_msa_nloc_d
#define __msa_nlzc_b __builtin_msa_nlzc_b
#define __msa_nlzc_h __builtin_msa_nlzc_h
#define __msa_nlzc_w __builtin_msa_nlzc_w
#define __msa_nlzc_d __builtin_msa_nlzc_d
#define __msa_copy_s_b __builtin_msa_copy_s_b
#define __msa_copy_s_h __builtin_msa_copy_s_h
#define __msa_copy_s_w __builtin_msa_copy_s_w
#define __msa_copy_s_d __builtin_msa_copy_s_d
#define __msa_copy_u_b __builtin_msa_copy_u_b
#define __msa_copy_u_h __builtin_msa_copy_u_h
#define __msa_copy_u_w __builtin_msa_copy_u_w
#define __msa_copy_u_d __builtin_msa_copy_u_d
#define __msa_insert_b __builtin_msa_insert_b
#define __msa_insert_h __builtin_msa_insert_h
#define __msa_insert_w __builtin_msa_insert_w
#define __msa_insert_d __builtin_msa_insert_d
#define __msa_insve_b __builtin_msa_insve_b
#define __msa_insve_h __builtin_msa_insve_h
#define __msa_insve_w __builtin_msa_insve_w
#define __msa_insve_d __builtin_msa_insve_d
#define __msa_test_bnz_b __builtin_msa_bnz_b
#define __msa_test_bnz_h __builtin_msa_bnz_h
#define __msa_test_bnz_w __builtin_msa_bnz_w
#define __msa_test_bnz_d __builtin_msa_bnz_d
#define __msa_test_bz_b __builtin_msa_bz_b
#define __msa_test_bz_h __builtin_msa_bz_h
#define __msa_test_bz_w __builtin_msa_bz_w
#define __msa_test_bz_d __builtin_msa_bz_d
#define __msa_ldi_b __builtin_msa_ldi_b
#define __msa_ldi_h __builtin_msa_ldi_h
#define __msa_ldi_w __builtin_msa_ldi_w
#define __msa_ldi_d __builtin_msa_ldi_d
#define __msa_fcaf_w __builtin_msa_fcaf_w
#define __msa_fcaf_d __builtin_msa_fcaf_d
#define __msa_fcor_w __builtin_msa_fcor_w
#define __msa_fcor_d __builtin_msa_fcor_d
#define __msa_fcun_w __builtin_msa_fcun_w
#define __msa_fcun_d __builtin_msa_fcun_d
#define __msa_fcune_w __builtin_msa_fcune_w
#define __msa_fcune_d __builtin_msa_fcune_d
#define __msa_fcueq_w __builtin_msa_fcueq_w
#define __msa_fcueq_d __builtin_msa_fcueq_d
#define __msa_fceq_w __builtin_msa_fceq_w
#define __msa_fceq_d __builtin_msa_fceq_d
#define __msa_fcne_w __builtin_msa_fcne_w
#define __msa_fcne_d __builtin_msa_fcne_d
#define __msa_fclt_w __builtin_msa_fclt_w
#define __msa_fclt_d __builtin_msa_fclt_d
#define __msa_fcult_w __builtin_msa_fcult_w
#define __msa_fcult_d __builtin_msa_fcult_d
#define __msa_fcle_w __builtin_msa_fcle_w
#define __msa_fcle_d __builtin_msa_fcle_d
#define __msa_fcule_w __builtin_msa_fcule_w
#define __msa_fcule_d __builtin_msa_fcule_d
#define __msa_fsaf_w __builtin_msa_fsaf_w
#define __msa_fsaf_d __builtin_msa_fsaf_d
#define __msa_fsor_w __builtin_msa_fsor_w
#define __msa_fsor_d __builtin_msa_fsor_d
#define __msa_fsun_w __builtin_msa_fsun_w
#define __msa_fsun_d __builtin_msa_fsun_d
#define __msa_fsune_w __builtin_msa_fsune_w
#define __msa_fsune_d __builtin_msa_fsune_d
#define __msa_fsueq_w __builtin_msa_fsueq_w
#define __msa_fsueq_d __builtin_msa_fsueq_d
#define __msa_fseq_w __builtin_msa_fseq_w
#define __msa_fseq_d __builtin_msa_fseq_d
#define __msa_fsne_w __builtin_msa_fsne_w
#define __msa_fsne_d __builtin_msa_fsne_d
#define __msa_fslt_w __builtin_msa_fslt_w
#define __msa_fslt_d __builtin_msa_fslt_d
#define __msa_fsult_w __builtin_msa_fsult_w
#define __msa_fsult_d __builtin_msa_fsult_d
#define __msa_fsle_w __builtin_msa_fsle_w
#define __msa_fsle_d __builtin_msa_fsle_d
#define __msa_fsule_w __builtin_msa_fsule_w
#define __msa_fsule_d __builtin_msa_fsule_d
#define __msa_fadd_w __builtin_msa_fadd_w
#define __msa_fadd_d __builtin_msa_fadd_d
#define __msa_fsub_w __builtin_msa_fsub_w
#define __msa_fsub_d __builtin_msa_fsub_d
#define __msa_fmul_w __builtin_msa_fmul_w
#define __msa_fmul_d __builtin_msa_fmul_d
#define __msa_fdiv_w __builtin_msa_fdiv_w
#define __msa_fdiv_d __builtin_msa_fdiv_d
#define __msa_fmadd_w __builtin_msa_fmadd_w
#define __msa_fmadd_d __builtin_msa_fmadd_d
#define __msa_fmsub_w __builtin_msa_fmsub_w
#define __msa_fmsub_d __builtin_msa_fmsub_d
#define __msa_fexp2_w __builtin_msa_fexp2_w
#define __msa_fexp2_d __builtin_msa_fexp2_d
#define __msa_fexdo_h __builtin_msa_fexdo_h
#define __msa_fexdo_w __builtin_msa_fexdo_w
#define __msa_ftq_h __builtin_msa_ftq_h
#define __msa_ftq_w __builtin_msa_ftq_w
#define __msa_fmin_w __builtin_msa_fmin_w
#define __msa_fmin_d __builtin_msa_fmin_d
#define __msa_fmin_a_w __builtin_msa_fmin_a_w
#define __msa_fmin_a_d __builtin_msa_fmin_a_d
#define __msa_fmax_w __builtin_msa_fmax_w
#define __msa_fmax_d __builtin_msa_fmax_d
#define __msa_fmax_a_w __builtin_msa_fmax_a_w
#define __msa_fmax_a_d __builtin_msa_fmax_a_d
#define __msa_mul_q_h __builtin_msa_mul_q_h
#define __msa_mul_q_w __builtin_msa_mul_q_w
#define __msa_mulr_q_h __builtin_msa_mulr_q_h
#define __msa_mulr_q_w __builtin_msa_mulr_q_w
#define __msa_madd_q_h __builtin_msa_madd_q_h
#define __msa_madd_q_w __builtin_msa_madd_q_w
#define __msa_maddr_q_h __builtin_msa_maddr_q_h
#define __msa_maddr_q_w __builtin_msa_maddr_q_w
#define __msa_msub_q_h __builtin_msa_msub_q_h
#define __msa_msub_q_w __builtin_msa_msub_q_w
#define __msa_msubr_q_h __builtin_msa_msubr_q_h
#define __msa_msubr_q_w __builtin_msa_msubr_q_w
#define __msa_fclass_w __builtin_msa_fclass_w
#define __msa_fclass_d __builtin_msa_fclass_d
#define __msa_fsqrt_w __builtin_msa_fsqrt_w
#define __msa_fsqrt_d __builtin_msa_fsqrt_d
#define __msa_frcp_w __builtin_msa_frcp_w
#define __msa_frcp_d __builtin_msa_frcp_d
#define __msa_frint_w __builtin_msa_frint_w
#define __msa_frint_d __builtin_msa_frint_d
#define __msa_frsqrt_w __builtin_msa_frsqrt_w
#define __msa_frsqrt_d __builtin_msa_frsqrt_d
#define __msa_flog2_w __builtin_msa_flog2_w
#define __msa_flog2_d __builtin_msa_flog2_d
#define __msa_fexupl_w __builtin_msa_fexupl_w
#define __msa_fexupl_d __builtin_msa_fexupl_d
#define __msa_fexupr_w __builtin_msa_fexupr_w
#define __msa_fexupr_d __builtin_msa_fexupr_d
#define __msa_ffql_w __builtin_msa_ffql_w
#define __msa_ffql_d __builtin_msa_ffql_d
#define __msa_ffqr_w __builtin_msa_ffqr_w
#define __msa_ffqr_d __builtin_msa_ffqr_d
#define __msa_ftint_s_w __builtin_msa_ftint_s_w
#define __msa_ftint_s_d __builtin_msa_ftint_s_d
#define __msa_ftint_u_w __builtin_msa_ftint_u_w
#define __msa_ftint_u_d __builtin_msa_ftint_u_d
#define __msa_ftrunc_s_w __builtin_msa_ftrunc_s_w
#define __msa_ftrunc_s_d __builtin_msa_ftrunc_s_d
#define __msa_ftrunc_u_w __builtin_msa_ftrunc_u_w
#define __msa_ftrunc_u_d __builtin_msa_ftrunc_u_d
#define __msa_ffint_s_w __builtin_msa_ffint_s_w
#define __msa_ffint_s_d __builtin_msa_ffint_s_d
#define __msa_ffint_u_w __builtin_msa_ffint_u_w
#define __msa_ffint_u_d __builtin_msa_ffint_u_d
#define __msa_cfcmsa __builtin_msa_cfcmsa
#define __msa_move_v __builtin_msa_move_v
#define __msa_cast_to_vector_float __builtin_msa_cast_to_vector_float
#define __msa_cast_to_vector_double __builtin_msa_cast_to_vector_double
#define __msa_cast_to_scalar_float __builtin_msa_cast_to_scalar_float
#define __msa_cast_to_scalar_double __builtin_msa_cast_to_scalar_double
#endif /* defined(__mips_msa) */
#endif /* _MSA_H */
