/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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


/**
 * @file
 * Unit tests for blend LLVM IR generation
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 *
 * Blend computation code derived from code written by
 * @author Brian Paul <brian@vmware.com>
 */

#include "util/u_memory.h"

#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_bld_blend.h"
#include "lp_test.h"


enum vector_mode
{
   AoS = 0,
   SoA = 1
};


typedef void (*blend_test_ptr_t)(const void *src, const void *dst, const void *con, void *res);


void
write_tsv_header(FILE *fp)
{
   fprintf(fp,
           "result\t"
           "cycles_per_channel\t"
           "mode\t"
           "type\t"
           "sep_func\t"
           "sep_src_factor\t"
           "sep_dst_factor\t"
           "rgb_func\t"
           "rgb_src_factor\t"
           "rgb_dst_factor\t"
           "alpha_func\t"
           "alpha_src_factor\t"
           "alpha_dst_factor\n");

   fflush(fp);
}


static void
write_tsv_row(FILE *fp,
              const struct pipe_blend_state *blend,
              enum vector_mode mode,
              struct lp_type type,
              double cycles,
              boolean success)
{
   fprintf(fp, "%s\t", success ? "pass" : "fail");

   if (mode == AoS) {
      fprintf(fp, "%.1f\t", cycles / type.length);
      fprintf(fp, "aos\t");
   }

   if (mode == SoA) {
      fprintf(fp, "%.1f\t", cycles / (4 * type.length));
      fprintf(fp, "soa\t");
   }

   fprintf(fp, "%s%u%sx%u\t",
           type.floating ? "f" : (type.fixed ? "h" : (type.sign ? "s" : "u")),
           type.width,
           type.norm ? "n" : "",
           type.length);

   fprintf(fp,
           "%s\t%s\t%s\t",
           blend->rt[0].rgb_func != blend->rt[0].alpha_func ? "true" : "false",
           blend->rt[0].rgb_src_factor != blend->rt[0].alpha_src_factor ? "true" : "false",
           blend->rt[0].rgb_dst_factor != blend->rt[0].alpha_dst_factor ? "true" : "false");

   fprintf(fp,
           "%s\t%s\t%s\t%s\t%s\t%s\n",
           util_dump_blend_func(blend->rt[0].rgb_func, TRUE),
           util_dump_blend_factor(blend->rt[0].rgb_src_factor, TRUE),
           util_dump_blend_factor(blend->rt[0].rgb_dst_factor, TRUE),
           util_dump_blend_func(blend->rt[0].alpha_func, TRUE),
           util_dump_blend_factor(blend->rt[0].alpha_src_factor, TRUE),
           util_dump_blend_factor(blend->rt[0].alpha_dst_factor, TRUE));

   fflush(fp);
}


static void
dump_blend_type(FILE *fp,
                const struct pipe_blend_state *blend,
                enum vector_mode mode,
                struct lp_type type)
{
   fprintf(fp, "%s", mode ? "soa" : "aos");

   fprintf(fp, " type=%s%u%sx%u",
           type.floating ? "f" : (type.fixed ? "h" : (type.sign ? "s" : "u")),
           type.width,
           type.norm ? "n" : "",
           type.length);

   fprintf(fp,
           " %s=%s %s=%s %s=%s %s=%s %s=%s %s=%s",
           "rgb_func",         util_dump_blend_func(blend->rt[0].rgb_func, TRUE),
           "rgb_src_factor",   util_dump_blend_factor(blend->rt[0].rgb_src_factor, TRUE),
           "rgb_dst_factor",   util_dump_blend_factor(blend->rt[0].rgb_dst_factor, TRUE),
           "alpha_func",       util_dump_blend_func(blend->rt[0].alpha_func, TRUE),
           "alpha_src_factor", util_dump_blend_factor(blend->rt[0].alpha_src_factor, TRUE),
           "alpha_dst_factor", util_dump_blend_factor(blend->rt[0].alpha_dst_factor, TRUE));

   fprintf(fp, " ...\n");
   fflush(fp);
}


static LLVMValueRef
add_blend_test(struct gallivm_state *gallivm,
               const struct pipe_blend_state *blend,
               enum vector_mode mode,
               struct lp_type type)
{
   LLVMModuleRef module = gallivm->module;
   LLVMContextRef context = gallivm->context;
   LLVMTypeRef vec_type;
   LLVMTypeRef args[4];
   LLVMValueRef func;
   LLVMValueRef src_ptr;
   LLVMValueRef dst_ptr;
   LLVMValueRef const_ptr;
   LLVMValueRef res_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   const enum pipe_format format = PIPE_FORMAT_R8G8B8A8_UNORM;
   const unsigned rt = 0;
   const unsigned char swizzle[4] = { 0, 1, 2, 3 };

   vec_type = lp_build_vec_type(gallivm, type);

   args[3] = args[2] = args[1] = args[0] = LLVMPointerType(vec_type, 0);
   func = LLVMAddFunction(module, "test", LLVMFunctionType(LLVMVoidTypeInContext(context), args, 4, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   src_ptr = LLVMGetParam(func, 0);
   dst_ptr = LLVMGetParam(func, 1);
   const_ptr = LLVMGetParam(func, 2);
   res_ptr = LLVMGetParam(func, 3);

   block = LLVMAppendBasicBlockInContext(context, func, "entry");
   builder = gallivm->builder;
   LLVMPositionBuilderAtEnd(builder, block);

   if (mode == AoS) {
      LLVMValueRef src;
      LLVMValueRef dst;
      LLVMValueRef con;
      LLVMValueRef res;

      src = LLVMBuildLoad(builder, src_ptr, "src");
      dst = LLVMBuildLoad(builder, dst_ptr, "dst");
      con = LLVMBuildLoad(builder, const_ptr, "const");

      res = lp_build_blend_aos(gallivm, blend, &format, type, rt, src, dst, NULL, con, swizzle);

      lp_build_name(res, "res");

      LLVMBuildStore(builder, res, res_ptr);
   }

   if (mode == SoA) {
      LLVMValueRef src[4];
      LLVMValueRef dst[4];
      LLVMValueRef con[4];
      LLVMValueRef res[4];
      unsigned i;

      for(i = 0; i < 4; ++i) {
         LLVMValueRef index = LLVMConstInt(LLVMInt32TypeInContext(context), i, 0);
         src[i] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, src_ptr, &index, 1, ""), "");
         dst[i] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dst_ptr, &index, 1, ""), "");
         con[i] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, const_ptr, &index, 1, ""), "");
         lp_build_name(src[i], "src.%c", "rgba"[i]);
         lp_build_name(con[i], "con.%c", "rgba"[i]);
         lp_build_name(dst[i], "dst.%c", "rgba"[i]);
      }

      lp_build_blend_soa(gallivm, blend, type, rt, src, dst, con, res);

      for(i = 0; i < 4; ++i) {
         LLVMValueRef index = LLVMConstInt(LLVMInt32TypeInContext(context), i, 0);
         lp_build_name(res[i], "res.%c", "rgba"[i]);
         LLVMBuildStore(builder, res[i], LLVMBuildGEP(builder, res_ptr, &index, 1, ""));
      }
   }

   LLVMBuildRetVoid(builder);;

   return func;
}


static void
compute_blend_ref_term(unsigned rgb_factor,
                       unsigned alpha_factor,
                       const double *factor,
                       const double *src,
                       const double *dst,
                       const double *con,
                       double *term)
{
   double temp;

   switch (rgb_factor) {
   case PIPE_BLENDFACTOR_ONE:
      term[0] = factor[0]; /* R */
      term[1] = factor[1]; /* G */
      term[2] = factor[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      term[0] = factor[0] * src[0]; /* R */
      term[1] = factor[1] * src[1]; /* G */
      term[2] = factor[2] * src[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      term[0] = factor[0] * src[3]; /* R */
      term[1] = factor[1] * src[3]; /* G */
      term[2] = factor[2] * src[3]; /* B */
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      term[0] = factor[0] * dst[0]; /* R */
      term[1] = factor[1] * dst[1]; /* G */
      term[2] = factor[2] * dst[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_DST_ALPHA:
      term[0] = factor[0] * dst[3]; /* R */
      term[1] = factor[1] * dst[3]; /* G */
      term[2] = factor[2] * dst[3]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      temp = MIN2(src[3], 1.0f - dst[3]);
      term[0] = factor[0] * temp; /* R */
      term[1] = factor[1] * temp; /* G */
      term[2] = factor[2] * temp; /* B */
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      term[0] = factor[0] * con[0]; /* R */
      term[1] = factor[1] * con[1]; /* G */
      term[2] = factor[2] * con[2]; /* B */
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      term[0] = factor[0] * con[3]; /* R */
      term[1] = factor[1] * con[3]; /* G */
      term[2] = factor[2] * con[3]; /* B */
      break;
   case PIPE_BLENDFACTOR_SRC1_COLOR:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_ZERO:
      term[0] = 0.0f; /* R */
      term[1] = 0.0f; /* G */
      term[2] = 0.0f; /* B */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      term[0] = factor[0] * (1.0f - src[0]); /* R */
      term[1] = factor[1] * (1.0f - src[1]); /* G */
      term[2] = factor[2] * (1.0f - src[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      term[0] = factor[0] * (1.0f - src[3]); /* R */
      term[1] = factor[1] * (1.0f - src[3]); /* G */
      term[2] = factor[2] * (1.0f - src[3]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      term[0] = factor[0] * (1.0f - dst[3]); /* R */
      term[1] = factor[1] * (1.0f - dst[3]); /* G */
      term[2] = factor[2] * (1.0f - dst[3]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      term[0] = factor[0] * (1.0f - dst[0]); /* R */
      term[1] = factor[1] * (1.0f - dst[1]); /* G */
      term[2] = factor[2] * (1.0f - dst[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      term[0] = factor[0] * (1.0f - con[0]); /* R */
      term[1] = factor[1] * (1.0f - con[1]); /* G */
      term[2] = factor[2] * (1.0f - con[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      term[0] = factor[0] * (1.0f - con[3]); /* R */
      term[1] = factor[1] * (1.0f - con[3]); /* G */
      term[2] = factor[2] * (1.0f - con[3]); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      assert(0); /* to do */
      break;
   default:
      assert(0);
   }

   /*
    * Compute src/first term A
    */
   switch (alpha_factor) {
   case PIPE_BLENDFACTOR_ONE:
      term[3] = factor[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      term[3] = factor[3] * src[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_DST_ALPHA:
      term[3] = factor[3] * dst[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      term[3] = src[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      term[3] = factor[3] * con[3]; /* A */
      break;
   case PIPE_BLENDFACTOR_ZERO:
      term[3] = 0.0f; /* A */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      term[3] = factor[3] * (1.0f - src[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      term[3] = factor[3] * (1.0f - dst[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      term[3] = factor[3] * (1.0f - con[3]);
      break;
   default:
      assert(0);
   }
}


static void
compute_blend_ref(const struct pipe_blend_state *blend,
                  const double *src,
                  const double *dst,
                  const double *con,
                  double *res)
{
   double src_term[4];
   double dst_term[4];

   compute_blend_ref_term(blend->rt[0].rgb_src_factor, blend->rt[0].alpha_src_factor,
                          src, src, dst, con, src_term);
   compute_blend_ref_term(blend->rt[0].rgb_dst_factor, blend->rt[0].alpha_dst_factor,
                          dst, src, dst, con, dst_term);

   /*
    * Combine RGB terms
    */
   switch (blend->rt[0].rgb_func) {
   case PIPE_BLEND_ADD:
      res[0] = src_term[0] + dst_term[0]; /* R */
      res[1] = src_term[1] + dst_term[1]; /* G */
      res[2] = src_term[2] + dst_term[2]; /* B */
      break;
   case PIPE_BLEND_SUBTRACT:
      res[0] = src_term[0] - dst_term[0]; /* R */
      res[1] = src_term[1] - dst_term[1]; /* G */
      res[2] = src_term[2] - dst_term[2]; /* B */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      res[0] = dst_term[0] - src_term[0]; /* R */
      res[1] = dst_term[1] - src_term[1]; /* G */
      res[2] = dst_term[2] - src_term[2]; /* B */
      break;
   case PIPE_BLEND_MIN:
      res[0] = MIN2(src_term[0], dst_term[0]); /* R */
      res[1] = MIN2(src_term[1], dst_term[1]); /* G */
      res[2] = MIN2(src_term[2], dst_term[2]); /* B */
      break;
   case PIPE_BLEND_MAX:
      res[0] = MAX2(src_term[0], dst_term[0]); /* R */
      res[1] = MAX2(src_term[1], dst_term[1]); /* G */
      res[2] = MAX2(src_term[2], dst_term[2]); /* B */
      break;
   default:
      assert(0);
   }

   /*
    * Combine A terms
    */
   switch (blend->rt[0].alpha_func) {
   case PIPE_BLEND_ADD:
      res[3] = src_term[3] + dst_term[3]; /* A */
      break;
   case PIPE_BLEND_SUBTRACT:
      res[3] = src_term[3] - dst_term[3]; /* A */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      res[3] = dst_term[3] - src_term[3]; /* A */
      break;
   case PIPE_BLEND_MIN:
      res[3] = MIN2(src_term[3], dst_term[3]); /* A */
      break;
   case PIPE_BLEND_MAX:
      res[3] = MAX2(src_term[3], dst_term[3]); /* A */
      break;
   default:
      assert(0);
   }
}


PIPE_ALIGN_STACK
static boolean
test_one(unsigned verbose,
         FILE *fp,
         const struct pipe_blend_state *blend,
         enum vector_mode mode,
         struct lp_type type)
{
   struct gallivm_state *gallivm;
   LLVMValueRef func = NULL;
   blend_test_ptr_t blend_test_ptr;
   boolean success;
   const unsigned n = LP_TEST_NUM_SAMPLES;
   int64_t cycles[LP_TEST_NUM_SAMPLES];
   double cycles_avg = 0.0;
   unsigned i, j;
   const unsigned stride = lp_type_width(type)/8;

   if(verbose >= 1)
      dump_blend_type(stdout, blend, mode, type);

   gallivm = gallivm_create();

   func = add_blend_test(gallivm, blend, mode, type);

   gallivm_compile_module(gallivm);

   blend_test_ptr = (blend_test_ptr_t)gallivm_jit_function(gallivm, func);

   success = TRUE;
   if(mode == AoS) {
      uint8_t *src, *dst, *con, *res, *ref;
      src = align_malloc(stride, stride);
      dst = align_malloc(stride, stride);
      con = align_malloc(stride, stride);
      res = align_malloc(stride, stride);
      ref = align_malloc(stride, stride);

      for(i = 0; i < n && success; ++i) {
         int64_t start_counter = 0;
         int64_t end_counter = 0;

         random_vec(type, src);
         random_vec(type, dst);
         random_vec(type, con);

         {
            double fsrc[LP_MAX_VECTOR_LENGTH];
            double fdst[LP_MAX_VECTOR_LENGTH];
            double fcon[LP_MAX_VECTOR_LENGTH];
            double fref[LP_MAX_VECTOR_LENGTH];

            read_vec(type, src, fsrc);
            read_vec(type, dst, fdst);
            read_vec(type, con, fcon);

            for(j = 0; j < type.length; j += 4)
               compute_blend_ref(blend, fsrc + j, fdst + j, fcon + j, fref + j);

            write_vec(type, ref, fref);
         }

         start_counter = rdtsc();
         blend_test_ptr(src, dst, con, res);
         end_counter = rdtsc();

         cycles[i] = end_counter - start_counter;

         if(!compare_vec(type, res, ref)) {
            success = FALSE;

            if(verbose < 1)
               dump_blend_type(stderr, blend, mode, type);
            fprintf(stderr, "MISMATCH\n");

            fprintf(stderr, "  Src: ");
            dump_vec(stderr, type, src);
            fprintf(stderr, "\n");

            fprintf(stderr, "  Dst: ");
            dump_vec(stderr, type, dst);
            fprintf(stderr, "\n");

            fprintf(stderr, "  Con: ");
            dump_vec(stderr, type, con);
            fprintf(stderr, "\n");

            fprintf(stderr, "  Res: ");
            dump_vec(stderr, type, res);
            fprintf(stderr, "\n");

            fprintf(stderr, "  Ref: ");
            dump_vec(stderr, type, ref);
            fprintf(stderr, "\n");
         }
      }
      align_free(src);
      align_free(dst);
      align_free(con);
      align_free(res);
      align_free(ref);
   }
   else if(mode == SoA) {
      uint8_t *src, *dst, *con, *res, *ref;
      src = align_malloc(4*stride, stride);
      dst = align_malloc(4*stride, stride);
      con = align_malloc(4*stride, stride);
      res = align_malloc(4*stride, stride);
      ref = align_malloc(4*stride, stride);

      for(i = 0; i < n && success; ++i) {
         int64_t start_counter = 0;
         int64_t end_counter = 0;
         boolean mismatch;

         for(j = 0; j < 4; ++j) {
            random_vec(type, src + j*stride);
            random_vec(type, dst + j*stride);
            random_vec(type, con + j*stride);
         }

         {
            double fsrc[4];
            double fdst[4];
            double fcon[4];
            double fref[4];
            unsigned k;

            for(k = 0; k < type.length; ++k) {
               for(j = 0; j < 4; ++j) {
                  fsrc[j] = read_elem(type, src + j*stride, k);
                  fdst[j] = read_elem(type, dst + j*stride, k);
                  fcon[j] = read_elem(type, con + j*stride, k);
               }

               compute_blend_ref(blend, fsrc, fdst, fcon, fref);

               for(j = 0; j < 4; ++j)
                  write_elem(type, ref + j*stride, k, fref[j]);
            }
         }

         start_counter = rdtsc();
         blend_test_ptr(src, dst, con, res);
         end_counter = rdtsc();

         cycles[i] = end_counter - start_counter;

         mismatch = FALSE;
         for (j = 0; j < 4; ++j)
            if(!compare_vec(type, res + j*stride, ref + j*stride))
               mismatch = TRUE;

         if (mismatch) {
            success = FALSE;

            if(verbose < 1)
               dump_blend_type(stderr, blend, mode, type);
            fprintf(stderr, "MISMATCH\n");
            for(j = 0; j < 4; ++j) {
               char channel = "RGBA"[j];
               fprintf(stderr, "  Src%c: ", channel);
               dump_vec(stderr, type, src + j*stride);
               fprintf(stderr, "\n");

               fprintf(stderr, "  Dst%c: ", channel);
               dump_vec(stderr, type, dst + j*stride);
               fprintf(stderr, "\n");

               fprintf(stderr, "  Con%c: ", channel);
               dump_vec(stderr, type, con + j*stride);
               fprintf(stderr, "\n");

               fprintf(stderr, "  Res%c: ", channel);
               dump_vec(stderr, type, res + j*stride);
               fprintf(stderr, "\n");

               fprintf(stderr, "  Ref%c: ", channel);
               dump_vec(stderr, type, ref + j*stride);
               fprintf(stderr, "\n");

               fprintf(stderr, "\n");
            }
         }
      }
      align_free(src);
      align_free(dst);
      align_free(con);
      align_free(res);
      align_free(ref);
   }

   /*
    * Unfortunately the output of cycle counter is not very reliable as it comes
    * -- sometimes we get outliers (due IRQs perhaps?) which are
    * better removed to avoid random or biased data.
    */
   {
      double sum = 0.0, sum2 = 0.0;
      double avg, std;
      unsigned m;

      for(i = 0; i < n; ++i) {
         sum += cycles[i];
         sum2 += cycles[i]*cycles[i];
      }

      avg = sum/n;
      std = sqrtf((sum2 - n*avg*avg)/n);

      m = 0;
      sum = 0.0;
      for(i = 0; i < n; ++i) {
         if(fabs(cycles[i] - avg) <= 4.0*std) {
            sum += cycles[i];
            ++m;
         }
      }

      cycles_avg = sum/m;

   }

   if(fp)
      write_tsv_row(fp, blend, mode, type, cycles_avg, success);

   gallivm_free_function(gallivm, func, blend_test_ptr);

   gallivm_destroy(gallivm);

   return success;
}


const unsigned
blend_factors[] = {
   PIPE_BLENDFACTOR_ZERO,
   PIPE_BLENDFACTOR_ONE,
   PIPE_BLENDFACTOR_SRC_COLOR,
   PIPE_BLENDFACTOR_SRC_ALPHA,
   PIPE_BLENDFACTOR_DST_COLOR,
   PIPE_BLENDFACTOR_DST_ALPHA,
   PIPE_BLENDFACTOR_CONST_COLOR,
   PIPE_BLENDFACTOR_CONST_ALPHA,
#if 0
   PIPE_BLENDFACTOR_SRC1_COLOR,
   PIPE_BLENDFACTOR_SRC1_ALPHA,
#endif
   PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE,
   PIPE_BLENDFACTOR_INV_SRC_COLOR,
   PIPE_BLENDFACTOR_INV_SRC_ALPHA,
   PIPE_BLENDFACTOR_INV_DST_COLOR,
   PIPE_BLENDFACTOR_INV_DST_ALPHA,
   PIPE_BLENDFACTOR_INV_CONST_COLOR,
   PIPE_BLENDFACTOR_INV_CONST_ALPHA,
#if 0
   PIPE_BLENDFACTOR_INV_SRC1_COLOR,
   PIPE_BLENDFACTOR_INV_SRC1_ALPHA,
#endif
};


const unsigned
blend_funcs[] = {
   PIPE_BLEND_ADD,
   PIPE_BLEND_SUBTRACT,
   PIPE_BLEND_REVERSE_SUBTRACT,
   PIPE_BLEND_MIN,
   PIPE_BLEND_MAX
};


const struct lp_type blend_types[] = {
   /* float, fixed,  sign,  norm, width, len */
   {   TRUE, FALSE,  TRUE, FALSE,    32,   4 }, /* f32 x 4 */
   {  FALSE, FALSE, FALSE,  TRUE,     8,  16 }, /* u8n x 16 */
};


const unsigned num_funcs = sizeof(blend_funcs)/sizeof(blend_funcs[0]);
const unsigned num_factors = sizeof(blend_factors)/sizeof(blend_factors[0]);
const unsigned num_types = sizeof(blend_types)/sizeof(blend_types[0]);


boolean
test_all(unsigned verbose, FILE *fp)
{
   const unsigned *rgb_func;
   const unsigned *rgb_src_factor;
   const unsigned *rgb_dst_factor;
   const unsigned *alpha_func;
   const unsigned *alpha_src_factor;
   const unsigned *alpha_dst_factor;
   struct pipe_blend_state blend;
   enum vector_mode mode;
   const struct lp_type *type;
   boolean success = TRUE;

   for(rgb_func = blend_funcs; rgb_func < &blend_funcs[num_funcs]; ++rgb_func) {
      for(alpha_func = blend_funcs; alpha_func < &blend_funcs[num_funcs]; ++alpha_func) {
         for(rgb_src_factor = blend_factors; rgb_src_factor < &blend_factors[num_factors]; ++rgb_src_factor) {
            for(rgb_dst_factor = blend_factors; rgb_dst_factor <= rgb_src_factor; ++rgb_dst_factor) {
               for(alpha_src_factor = blend_factors; alpha_src_factor < &blend_factors[num_factors]; ++alpha_src_factor) {
                  for(alpha_dst_factor = blend_factors; alpha_dst_factor <= alpha_src_factor; ++alpha_dst_factor) {
                     for(mode = 0; mode < 2; ++mode) {
                        for(type = blend_types; type < &blend_types[num_types]; ++type) {

                           if(*rgb_dst_factor == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
                              *alpha_dst_factor == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE)
                              continue;

                           memset(&blend, 0, sizeof blend);
                           blend.rt[0].blend_enable      = 1;
                           blend.rt[0].rgb_func          = *rgb_func;
                           blend.rt[0].rgb_src_factor    = *rgb_src_factor;
                           blend.rt[0].rgb_dst_factor    = *rgb_dst_factor;
                           blend.rt[0].alpha_func        = *alpha_func;
                           blend.rt[0].alpha_src_factor  = *alpha_src_factor;
                           blend.rt[0].alpha_dst_factor  = *alpha_dst_factor;
                           blend.rt[0].colormask         = PIPE_MASK_RGBA;

                           if(!test_one(verbose, fp, &blend, mode, *type))
                             success = FALSE;

                        }
                     }
                  }
               }
            }
         }
      }
   }

   return success;
}


boolean
test_some(unsigned verbose, FILE *fp,
          unsigned long n)
{
   const unsigned *rgb_func;
   const unsigned *rgb_src_factor;
   const unsigned *rgb_dst_factor;
   const unsigned *alpha_func;
   const unsigned *alpha_src_factor;
   const unsigned *alpha_dst_factor;
   struct pipe_blend_state blend;
   enum vector_mode mode;
   const struct lp_type *type;
   unsigned long i;
   boolean success = TRUE;

   for(i = 0; i < n; ++i) {
      rgb_func = &blend_funcs[rand() % num_funcs];
      alpha_func = &blend_funcs[rand() % num_funcs];
      rgb_src_factor = &blend_factors[rand() % num_factors];
      alpha_src_factor = &blend_factors[rand() % num_factors];
      
      do {
         rgb_dst_factor = &blend_factors[rand() % num_factors];
      } while(*rgb_dst_factor == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE);

      do {
         alpha_dst_factor = &blend_factors[rand() % num_factors];
      } while(*alpha_dst_factor == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE);

      mode = rand() & 1;

      type = &blend_types[rand() % num_types];

      memset(&blend, 0, sizeof blend);
      blend.rt[0].blend_enable      = 1;
      blend.rt[0].rgb_func          = *rgb_func;
      blend.rt[0].rgb_src_factor    = *rgb_src_factor;
      blend.rt[0].rgb_dst_factor    = *rgb_dst_factor;
      blend.rt[0].alpha_func        = *alpha_func;
      blend.rt[0].alpha_src_factor  = *alpha_src_factor;
      blend.rt[0].alpha_dst_factor  = *alpha_dst_factor;
      blend.rt[0].colormask         = PIPE_MASK_RGBA;

      if(!test_one(verbose, fp, &blend, mode, *type))
        success = FALSE;
   }

   return success;
}


boolean
test_single(unsigned verbose, FILE *fp)
{
   printf("no test_single()");
   return TRUE;
}
