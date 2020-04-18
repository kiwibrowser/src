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
 * Helper functions for logical operations.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_cpu_detect.h"
#include "util/u_memory.h"
#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_init.h"
#include "lp_bld_intr.h"
#include "lp_bld_debug.h"
#include "lp_bld_logic.h"


/*
 * XXX
 *
 * Selection with vector conditional like
 *
 *    select <4 x i1> %C, %A, %B
 *
 * is valid IR (e.g. llvm/test/Assembler/vector-select.ll), but it is only
 * supported on some backends (x86) starting with llvm 3.1.
 *
 * Expanding the boolean vector to full SIMD register width, as in
 *
 *    sext <4 x i1> %C to <4 x i32>
 *
 * is valid and supported (e.g., llvm/test/CodeGen/X86/vec_compare.ll), but
 * it causes assertion failures in LLVM 2.6. It appears to work correctly on 
 * LLVM 2.7.
 */


/**
 * Build code to compare two values 'a' and 'b' of 'type' using the given func.
 * \param func  one of PIPE_FUNC_x
 * The result values will be 0 for false or ~0 for true.
 */
LLVMValueRef
lp_build_compare(struct gallivm_state *gallivm,
                 const struct lp_type type,
                 unsigned func,
                 LLVMValueRef a,
                 LLVMValueRef b)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(gallivm, type);
   LLVMValueRef zeros = LLVMConstNull(int_vec_type);
   LLVMValueRef ones = LLVMConstAllOnes(int_vec_type);
   LLVMValueRef cond;
   LLVMValueRef res;

   assert(func >= PIPE_FUNC_NEVER);
   assert(func <= PIPE_FUNC_ALWAYS);
   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if(func == PIPE_FUNC_NEVER)
      return zeros;
   if(func == PIPE_FUNC_ALWAYS)
      return ones;

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   /*
    * There are no unsigned integer comparison instructions in SSE.
    */

   if (!type.floating && !type.sign &&
       type.width * type.length == 128 &&
       util_cpu_caps.has_sse2 &&
       (func == PIPE_FUNC_LESS ||
        func == PIPE_FUNC_LEQUAL ||
        func == PIPE_FUNC_GREATER ||
        func == PIPE_FUNC_GEQUAL) &&
       (gallivm_debug & GALLIVM_DEBUG_PERF)) {
         debug_printf("%s: inefficient <%u x i%u> unsigned comparison\n",
                      __FUNCTION__, type.length, type.width);
   }
#endif

#if HAVE_LLVM < 0x0207
#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(type.width * type.length == 128) {
      if(type.floating && util_cpu_caps.has_sse) {
         /* float[4] comparison */
         LLVMTypeRef vec_type = lp_build_vec_type(gallivm, type);
         LLVMValueRef args[3];
         unsigned cc;
         boolean swap;

         swap = FALSE;
         switch(func) {
         case PIPE_FUNC_EQUAL:
            cc = 0;
            break;
         case PIPE_FUNC_NOTEQUAL:
            cc = 4;
            break;
         case PIPE_FUNC_LESS:
            cc = 1;
            break;
         case PIPE_FUNC_LEQUAL:
            cc = 2;
            break;
         case PIPE_FUNC_GREATER:
            cc = 1;
            swap = TRUE;
            break;
         case PIPE_FUNC_GEQUAL:
            cc = 2;
            swap = TRUE;
            break;
         default:
            assert(0);
            return lp_build_undef(gallivm, type);
         }

         if(swap) {
            args[0] = b;
            args[1] = a;
         }
         else {
            args[0] = a;
            args[1] = b;
         }

         args[2] = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), cc, 0);
         res = lp_build_intrinsic(builder,
                                  "llvm.x86.sse.cmp.ps",
                                  vec_type,
                                  args, 3);
         res = LLVMBuildBitCast(builder, res, int_vec_type, "");
         return res;
      }
      else if(util_cpu_caps.has_sse2) {
         /* int[4] comparison */
         static const struct {
            unsigned swap:1;
            unsigned eq:1;
            unsigned gt:1;
            unsigned not:1;
         } table[] = {
            {0, 0, 0, 1}, /* PIPE_FUNC_NEVER */
            {1, 0, 1, 0}, /* PIPE_FUNC_LESS */
            {0, 1, 0, 0}, /* PIPE_FUNC_EQUAL */
            {0, 0, 1, 1}, /* PIPE_FUNC_LEQUAL */
            {0, 0, 1, 0}, /* PIPE_FUNC_GREATER */
            {0, 1, 0, 1}, /* PIPE_FUNC_NOTEQUAL */
            {1, 0, 1, 1}, /* PIPE_FUNC_GEQUAL */
            {0, 0, 0, 0}  /* PIPE_FUNC_ALWAYS */
         };
         const char *pcmpeq;
         const char *pcmpgt;
         LLVMValueRef args[2];
         LLVMValueRef res;
         LLVMTypeRef vec_type = lp_build_vec_type(gallivm, type);

         switch (type.width) {
         case 8:
            pcmpeq = "llvm.x86.sse2.pcmpeq.b";
            pcmpgt = "llvm.x86.sse2.pcmpgt.b";
            break;
         case 16:
            pcmpeq = "llvm.x86.sse2.pcmpeq.w";
            pcmpgt = "llvm.x86.sse2.pcmpgt.w";
            break;
         case 32:
            pcmpeq = "llvm.x86.sse2.pcmpeq.d";
            pcmpgt = "llvm.x86.sse2.pcmpgt.d";
            break;
         default:
            assert(0);
            return lp_build_undef(gallivm, type);
         }

         /* There are no unsigned comparison instructions. So flip the sign bit
          * so that the results match.
          */
         if (table[func].gt && !type.sign) {
            LLVMValueRef msb = lp_build_const_int_vec(gallivm, type, (unsigned long long)1 << (type.width - 1));
            a = LLVMBuildXor(builder, a, msb, "");
            b = LLVMBuildXor(builder, b, msb, "");
         }

         if(table[func].swap) {
            args[0] = b;
            args[1] = a;
         }
         else {
            args[0] = a;
            args[1] = b;
         }

         if(table[func].eq)
            res = lp_build_intrinsic(builder, pcmpeq, vec_type, args, 2);
         else if (table[func].gt)
            res = lp_build_intrinsic(builder, pcmpgt, vec_type, args, 2);
         else
            res = LLVMConstNull(vec_type);

         if(table[func].not)
            res = LLVMBuildNot(builder, res, "");

         return res;
      }
   } /* if (type.width * type.length == 128) */
#endif
#endif /* HAVE_LLVM < 0x0207 */

   /* XXX: It is not clear if we should use the ordered or unordered operators */

   if(type.floating) {
      LLVMRealPredicate op;
      switch(func) {
      case PIPE_FUNC_NEVER:
         op = LLVMRealPredicateFalse;
         break;
      case PIPE_FUNC_ALWAYS:
         op = LLVMRealPredicateTrue;
         break;
      case PIPE_FUNC_EQUAL:
         op = LLVMRealUEQ;
         break;
      case PIPE_FUNC_NOTEQUAL:
         op = LLVMRealUNE;
         break;
      case PIPE_FUNC_LESS:
         op = LLVMRealULT;
         break;
      case PIPE_FUNC_LEQUAL:
         op = LLVMRealULE;
         break;
      case PIPE_FUNC_GREATER:
         op = LLVMRealUGT;
         break;
      case PIPE_FUNC_GEQUAL:
         op = LLVMRealUGE;
         break;
      default:
         assert(0);
         return lp_build_undef(gallivm, type);
      }

#if HAVE_LLVM >= 0x0207
      cond = LLVMBuildFCmp(builder, op, a, b, "");
      res = LLVMBuildSExt(builder, cond, int_vec_type, "");
#else
      if (type.length == 1) {
         cond = LLVMBuildFCmp(builder, op, a, b, "");
         res = LLVMBuildSExt(builder, cond, int_vec_type, "");
      }
      else {
         unsigned i;

         res = LLVMGetUndef(int_vec_type);

         debug_printf("%s: warning: using slow element-wise float"
                      " vector comparison\n", __FUNCTION__);
         for (i = 0; i < type.length; ++i) {
            LLVMValueRef index = lp_build_const_int32(gallivm, i);
            cond = LLVMBuildFCmp(builder, op,
                                 LLVMBuildExtractElement(builder, a, index, ""),
                                 LLVMBuildExtractElement(builder, b, index, ""),
                                 "");
            cond = LLVMBuildSelect(builder, cond,
                                   LLVMConstExtractElement(ones, index),
                                   LLVMConstExtractElement(zeros, index),
                                   "");
            res = LLVMBuildInsertElement(builder, res, cond, index, "");
         }
      }
#endif
   }
   else {
      LLVMIntPredicate op;
      switch(func) {
      case PIPE_FUNC_EQUAL:
         op = LLVMIntEQ;
         break;
      case PIPE_FUNC_NOTEQUAL:
         op = LLVMIntNE;
         break;
      case PIPE_FUNC_LESS:
         op = type.sign ? LLVMIntSLT : LLVMIntULT;
         break;
      case PIPE_FUNC_LEQUAL:
         op = type.sign ? LLVMIntSLE : LLVMIntULE;
         break;
      case PIPE_FUNC_GREATER:
         op = type.sign ? LLVMIntSGT : LLVMIntUGT;
         break;
      case PIPE_FUNC_GEQUAL:
         op = type.sign ? LLVMIntSGE : LLVMIntUGE;
         break;
      default:
         assert(0);
         return lp_build_undef(gallivm, type);
      }

#if HAVE_LLVM >= 0x0207
      cond = LLVMBuildICmp(builder, op, a, b, "");
      res = LLVMBuildSExt(builder, cond, int_vec_type, "");
#else
      if (type.length == 1) {
         cond = LLVMBuildICmp(builder, op, a, b, "");
         res = LLVMBuildSExt(builder, cond, int_vec_type, "");
      }
      else {
         unsigned i;

         res = LLVMGetUndef(int_vec_type);

         if (gallivm_debug & GALLIVM_DEBUG_PERF) {
            debug_printf("%s: using slow element-wise int"
                         " vector comparison\n", __FUNCTION__);
         }

         for(i = 0; i < type.length; ++i) {
            LLVMValueRef index = lp_build_const_int32(gallivm, i);
            cond = LLVMBuildICmp(builder, op,
                                 LLVMBuildExtractElement(builder, a, index, ""),
                                 LLVMBuildExtractElement(builder, b, index, ""),
                                 "");
            cond = LLVMBuildSelect(builder, cond,
                                   LLVMConstExtractElement(ones, index),
                                   LLVMConstExtractElement(zeros, index),
                                   "");
            res = LLVMBuildInsertElement(builder, res, cond, index, "");
         }
      }
#endif
   }

   return res;
}



/**
 * Build code to compare two values 'a' and 'b' using the given func.
 * \param func  one of PIPE_FUNC_x
 * The result values will be 0 for false or ~0 for true.
 */
LLVMValueRef
lp_build_cmp(struct lp_build_context *bld,
             unsigned func,
             LLVMValueRef a,
             LLVMValueRef b)
{
   return lp_build_compare(bld->gallivm, bld->type, func, a, b);
}


/**
 * Return (mask & a) | (~mask & b);
 */
LLVMValueRef
lp_build_select_bitwise(struct lp_build_context *bld,
                        LLVMValueRef mask,
                        LLVMValueRef a,
                        LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if (a == b) {
      return a;
   }

   if(type.floating) {
      LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);
      a = LLVMBuildBitCast(builder, a, int_vec_type, "");
      b = LLVMBuildBitCast(builder, b, int_vec_type, "");
   }

   a = LLVMBuildAnd(builder, a, mask, "");

   /* This often gets translated to PANDN, but sometimes the NOT is
    * pre-computed and stored in another constant. The best strategy depends
    * on available registers, so it is not a big deal -- hopefully LLVM does
    * the right decision attending the rest of the program.
    */
   b = LLVMBuildAnd(builder, b, LLVMBuildNot(builder, mask, ""), "");

   res = LLVMBuildOr(builder, a, b, "");

   if(type.floating) {
      LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
      res = LLVMBuildBitCast(builder, res, vec_type, "");
   }

   return res;
}


/**
 * Return mask ? a : b;
 *
 * mask is a bitwise mask, composed of 0 or ~0 for each element. Any other value
 * will yield unpredictable results.
 */
LLVMValueRef
lp_build_select(struct lp_build_context *bld,
                LLVMValueRef mask,
                LLVMValueRef a,
                LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMContextRef lc = bld->gallivm->context;
   struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if(a == b)
      return a;

   if (type.length == 1) {
      mask = LLVMBuildTrunc(builder, mask, LLVMInt1TypeInContext(lc), "");
      res = LLVMBuildSelect(builder, mask, a, b, "");
   }
   else if (0) {
      /* Generate a vector select.
       *
       * XXX: Using vector selects would avoid emitting intrinsics, but they aren't
       * properly supported yet.
       *
       * LLVM 3.0 includes experimental support provided the -promote-elements
       * options is passed to LLVM's command line (e.g., via
       * llvm::cl::ParseCommandLineOptions), but resulting code quality is much
       * worse, probably because some optimization passes don't know how to
       * handle vector selects.
       *
       * See also:
       * - http://lists.cs.uiuc.edu/pipermail/llvmdev/2011-October/043659.html
       */

      /* Convert the mask to a vector of booleans.
       * XXX: There are two ways to do this. Decide what's best.
       */
      if (1) {
         LLVMTypeRef bool_vec_type = LLVMVectorType(LLVMInt1TypeInContext(lc), type.length);
         mask = LLVMBuildTrunc(builder, mask, bool_vec_type, "");
      } else {
         mask = LLVMBuildICmp(builder, LLVMIntNE, mask, LLVMConstNull(bld->int_vec_type), "");
      }
      res = LLVMBuildSelect(builder, mask, a, b, "");
   }
   else if (((util_cpu_caps.has_sse4_1 &&
              type.width * type.length == 128) ||
             (util_cpu_caps.has_avx &&
              type.width * type.length == 256 && type.width >= 32)) &&
            !LLVMIsConstant(a) &&
            !LLVMIsConstant(b) &&
            !LLVMIsConstant(mask)) {
      const char *intrinsic;
      LLVMTypeRef arg_type;
      LLVMValueRef args[3];

      /*
       *  There's only float blend in AVX but can just cast i32/i64
       *  to float.
       */
      if (type.width * type.length == 256) {
         if (type.width == 64) {
           intrinsic = "llvm.x86.avx.blendv.pd.256";
           arg_type = LLVMVectorType(LLVMDoubleTypeInContext(lc), 4);
         }
         else {
            intrinsic = "llvm.x86.avx.blendv.ps.256";
            arg_type = LLVMVectorType(LLVMFloatTypeInContext(lc), 8);
         }
      }
      else if (type.floating &&
               type.width == 64) {
         intrinsic = "llvm.x86.sse41.blendvpd";
         arg_type = LLVMVectorType(LLVMDoubleTypeInContext(lc), 2);
      } else if (type.floating &&
                 type.width == 32) {
         intrinsic = "llvm.x86.sse41.blendvps";
         arg_type = LLVMVectorType(LLVMFloatTypeInContext(lc), 4);
      } else {
         intrinsic = "llvm.x86.sse41.pblendvb";
         arg_type = LLVMVectorType(LLVMInt8TypeInContext(lc), 16);
      }

      if (arg_type != bld->int_vec_type) {
         mask = LLVMBuildBitCast(builder, mask, arg_type, "");
      }

      if (arg_type != bld->vec_type) {
         a = LLVMBuildBitCast(builder, a, arg_type, "");
         b = LLVMBuildBitCast(builder, b, arg_type, "");
      }

      args[0] = b;
      args[1] = a;
      args[2] = mask;

      res = lp_build_intrinsic(builder, intrinsic,
                               arg_type, args, Elements(args));

      if (arg_type != bld->vec_type) {
         res = LLVMBuildBitCast(builder, res, bld->vec_type, "");
      }
   }
   else {
      res = lp_build_select_bitwise(bld, mask, a, b);
   }

   return res;
}


/**
 * Return mask ? a : b;
 *
 * mask is a TGSI_WRITEMASK_xxx.
 */
LLVMValueRef
lp_build_select_aos(struct lp_build_context *bld,
                    unsigned mask,
                    LLVMValueRef a,
                    LLVMValueRef b)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   const unsigned n = type.length;
   unsigned i, j;

   assert((mask & ~0xf) == 0);
   assert(lp_check_value(type, a));
   assert(lp_check_value(type, b));

   if(a == b)
      return a;
   if((mask & 0xf) == 0xf)
      return a;
   if((mask & 0xf) == 0x0)
      return b;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   /*
    * There are two major ways of accomplishing this:
    * - with a shuffle
    * - with a select
    *
    * The flip between these is empirical and might need to be adjusted.
    */
   if (n <= 4) {
      /*
       * Shuffle.
       */
      LLVMTypeRef elem_type = LLVMInt32TypeInContext(bld->gallivm->context);
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];

      for(j = 0; j < n; j += 4)
         for(i = 0; i < 4; ++i)
            shuffles[j + i] = LLVMConstInt(elem_type,
                                           (mask & (1 << i) ? 0 : n) + j + i,
                                           0);

      return LLVMBuildShuffleVector(builder, a, b, LLVMConstVector(shuffles, n), "");
   }
   else {
      LLVMValueRef mask_vec = lp_build_const_mask_aos(bld->gallivm, type, mask);
      return lp_build_select(bld, mask_vec, a, b);
   }
}


/**
 * Return (scalar-cast)val ? true : false;
 */
LLVMValueRef
lp_build_any_true_range(struct lp_build_context *bld,
                        unsigned real_length,
                        LLVMValueRef val)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMTypeRef scalar_type;
   LLVMTypeRef true_type;

   assert(real_length <= bld->type.length);

   true_type = LLVMIntTypeInContext(bld->gallivm->context,
                                    bld->type.width * real_length);
   scalar_type = LLVMIntTypeInContext(bld->gallivm->context,
                                      bld->type.width * bld->type.length);
   val = LLVMBuildBitCast(builder, val, scalar_type, "");
   /*
    * We're using always native types so we can use intrinsics.
    * However, if we don't do per-element calculations, we must ensure
    * the excess elements aren't used since they may contain garbage.
    */
   if (real_length < bld->type.length) {
      val = LLVMBuildTrunc(builder, val, true_type, "");
   }
   return LLVMBuildICmp(builder, LLVMIntNE,
                        val, LLVMConstNull(true_type), "");
}
