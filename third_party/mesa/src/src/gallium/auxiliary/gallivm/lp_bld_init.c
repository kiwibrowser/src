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


#include "pipe/p_config.h"
#include "pipe/p_compiler.h"
#include "util/u_cpu_detect.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "lp_bld.h"
#include "lp_bld_debug.h"
#include "lp_bld_misc.h"
#include "lp_bld_init.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/BitWriter.h>


/**
 * AVX is supported in:
 * - standard JIT from LLVM 3.2 onwards
 * - MC-JIT from LLVM 3.1
 *   - MC-JIT supports limited OSes (MacOSX and Linux)
 * - standard JIT in LLVM 3.1, with backports
 */
#if HAVE_LLVM >= 0x0302 || (HAVE_LLVM == 0x0301 && defined(HAVE_JIT_AVX_SUPPORT))
#  define USE_MCJIT 0
#  define HAVE_AVX 1
#elif HAVE_LLVM == 0x0301 && (defined(PIPE_OS_LINUX) || defined(PIPE_OS_APPLE))
#  define USE_MCJIT 1
#  define HAVE_AVX 1
#else
#  define USE_MCJIT 0
#  define HAVE_AVX 0
#endif


#if USE_MCJIT
void LLVMLinkInMCJIT();
#endif


#ifdef DEBUG
unsigned gallivm_debug = 0;

static const struct debug_named_value lp_bld_debug_flags[] = {
   { "tgsi",   GALLIVM_DEBUG_TGSI, NULL },
   { "ir",     GALLIVM_DEBUG_IR, NULL },
   { "asm",    GALLIVM_DEBUG_ASM, NULL },
   { "nopt",   GALLIVM_DEBUG_NO_OPT, NULL },
   { "perf",   GALLIVM_DEBUG_PERF, NULL },
   { "no_brilinear", GALLIVM_DEBUG_NO_BRILINEAR, NULL },
   { "gc",     GALLIVM_DEBUG_GC, NULL },
   DEBUG_NAMED_VALUE_END
};

DEBUG_GET_ONCE_FLAGS_OPTION(gallivm_debug, "GALLIVM_DEBUG", lp_bld_debug_flags, 0)
#endif


static boolean gallivm_initialized = FALSE;

unsigned lp_native_vector_width;


/*
 * Optimization values are:
 * - 0: None (-O0)
 * - 1: Less (-O1)
 * - 2: Default (-O2, -Os)
 * - 3: Aggressive (-O3)
 *
 * See also CodeGenOpt::Level in llvm/Target/TargetMachine.h
 */
enum LLVM_CodeGenOpt_Level {
#if HAVE_LLVM >= 0x207
   None,        // -O0
   Less,        // -O1
   Default,     // -O2, -Os
   Aggressive   // -O3
#else
   Default,
   None,
   Aggressive
#endif
};


#if HAVE_LLVM <= 0x0206
/**
 * LLVM 2.6 permits only one ExecutionEngine to be created.  So use the
 * same gallivm state everywhere.
 */
static struct gallivm_state *GlobalGallivm = NULL;
#endif


/**
 * Create the LLVM (optimization) pass manager and install
 * relevant optimization passes.
 * \return  TRUE for success, FALSE for failure
 */
static boolean
create_pass_manager(struct gallivm_state *gallivm)
{
   assert(!gallivm->passmgr);
   assert(gallivm->target);

   gallivm->passmgr = LLVMCreateFunctionPassManager(gallivm->provider);
   if (!gallivm->passmgr)
      return FALSE;

   LLVMAddTargetData(gallivm->target, gallivm->passmgr);

   if ((gallivm_debug & GALLIVM_DEBUG_NO_OPT) == 0) {
      /* These are the passes currently listed in llvm-c/Transforms/Scalar.h,
       * but there are more on SVN.
       * TODO: Add more passes.
       */
      LLVMAddCFGSimplificationPass(gallivm->passmgr);

      if (HAVE_LLVM >= 0x207 && sizeof(void*) == 4) {
         /* For LLVM >= 2.7 and 32-bit build, use this order of passes to
          * avoid generating bad code.
          * Test with piglit glsl-vs-sqrt-zero test.
          */
         LLVMAddConstantPropagationPass(gallivm->passmgr);
         LLVMAddPromoteMemoryToRegisterPass(gallivm->passmgr);
      }
      else {
         LLVMAddPromoteMemoryToRegisterPass(gallivm->passmgr);
         LLVMAddConstantPropagationPass(gallivm->passmgr);
      }

      if (util_cpu_caps.has_sse4_1) {
         /* FIXME: There is a bug in this pass, whereby the combination
          * of fptosi and sitofp (necessary for trunc/floor/ceil/round
          * implementation) somehow becomes invalid code.
          */
         LLVMAddInstructionCombiningPass(gallivm->passmgr);
      }
      LLVMAddGVNPass(gallivm->passmgr);
   }
   else {
      /* We need at least this pass to prevent the backends to fail in
       * unexpected ways.
       */
      LLVMAddPromoteMemoryToRegisterPass(gallivm->passmgr);
   }

   return TRUE;
}


/**
 * Free gallivm object's LLVM allocations, but not the gallivm object itself.
 */
static void
free_gallivm_state(struct gallivm_state *gallivm)
{
#if HAVE_LLVM >= 0x207 /* XXX or 0x208? */
   /* This leads to crashes w/ some versions of LLVM */
   LLVMModuleRef mod;
   char *error;

   if (gallivm->engine && gallivm->provider)
      LLVMRemoveModuleProvider(gallivm->engine, gallivm->provider,
                               &mod, &error);
#endif

   if (gallivm->passmgr) {
      LLVMDisposePassManager(gallivm->passmgr);
   }

#if 0
   /* XXX this seems to crash with all versions of LLVM */
   if (gallivm->provider)
      LLVMDisposeModuleProvider(gallivm->provider);
#endif

   if (HAVE_LLVM >= 0x207 && gallivm->engine) {
      /* This will already destroy any associated module */
      LLVMDisposeExecutionEngine(gallivm->engine);
   } else {
      LLVMDisposeModule(gallivm->module);
   }

#if !USE_MCJIT
   /* Don't free the TargetData, it's owned by the exec engine */
#else
   if (gallivm->target) {
      LLVMDisposeTargetData(gallivm->target);
   }
#endif

   /* Never free the LLVM context.
    */
#if 0
   if (gallivm->context)
      LLVMContextDispose(gallivm->context);
#endif

   if (gallivm->builder)
      LLVMDisposeBuilder(gallivm->builder);

   gallivm->engine = NULL;
   gallivm->target = NULL;
   gallivm->module = NULL;
   gallivm->provider = NULL;
   gallivm->passmgr = NULL;
   gallivm->context = NULL;
   gallivm->builder = NULL;
}


static boolean
init_gallivm_engine(struct gallivm_state *gallivm)
{
   if (1) {
      /* We can only create one LLVMExecutionEngine (w/ LLVM 2.6 anyway) */
      enum LLVM_CodeGenOpt_Level optlevel;
      char *error = NULL;
      int ret;

      if (gallivm_debug & GALLIVM_DEBUG_NO_OPT) {
         optlevel = None;
      }
      else {
         optlevel = Default;
      }

#if USE_MCJIT
      ret = lp_build_create_mcjit_compiler_for_module(&gallivm->engine,
                                                      gallivm->module,
                                                      (unsigned) optlevel,
                                                      &error);
#else
      ret = LLVMCreateJITCompiler(&gallivm->engine, gallivm->provider,
                                  (unsigned) optlevel, &error);
#endif
      if (ret) {
         _debug_printf("%s\n", error);
         LLVMDisposeMessage(error);
         goto fail;
      }

#if defined(DEBUG) || defined(PROFILE)
      lp_register_oprofile_jit_event_listener(gallivm->engine);
#endif
   }

   LLVMAddModuleProvider(gallivm->engine, gallivm->provider);//new

#if !USE_MCJIT
   gallivm->target = LLVMGetExecutionEngineTargetData(gallivm->engine);
   if (!gallivm->target)
      goto fail;
#else
   if (0) {
       /*
        * Dump the data layout strings.
        */

       LLVMTargetDataRef target = LLVMGetExecutionEngineTargetData(gallivm->engine);
       char *data_layout;
       char *engine_data_layout;

       data_layout = LLVMCopyStringRepOfTargetData(gallivm->target);
       engine_data_layout = LLVMCopyStringRepOfTargetData(target);

       if (1) {
          debug_printf("module target data = %s\n", data_layout);
          debug_printf("engine target data = %s\n", engine_data_layout);
       }

       free(data_layout);
       free(engine_data_layout);
   }
#endif

   return TRUE;

fail:
   return FALSE;
}


/**
 * Singleton
 *
 * We must never free LLVM contexts, because LLVM has several global caches
 * which pointing/derived from objects owned by the context, causing false
 * memory leaks and false cache hits when these objects are destroyed.
 *
 * TODO: For thread safety on multi-threaded OpenGL we should use one LLVM
 * context per thread, and put them in a pool when threads are destroyed.
 */
static LLVMContextRef gallivm_context = NULL;


/**
 * Allocate gallivm LLVM objects.
 * \return  TRUE for success, FALSE for failure
 */
static boolean
init_gallivm_state(struct gallivm_state *gallivm)
{
   assert(!gallivm->context);
   assert(!gallivm->module);
   assert(!gallivm->provider);

   lp_build_init();

   if (!gallivm_context) {
      gallivm_context = LLVMContextCreate();
   }
   gallivm->context = gallivm_context;
   if (!gallivm->context)
      goto fail;

   gallivm->module = LLVMModuleCreateWithNameInContext("gallivm",
                                                       gallivm->context);
   if (!gallivm->module)
      goto fail;

   gallivm->provider =
      LLVMCreateModuleProviderForExistingModule(gallivm->module);
   if (!gallivm->provider)
      goto fail;

   gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
   if (!gallivm->builder)
      goto fail;

   /* FIXME: MC-JIT only allows compiling one module at a time, and it must be
    * complete when MC-JIT is created. So defer the MC-JIT engine creation for
    * now.
    */
#if !USE_MCJIT
   if (!init_gallivm_engine(gallivm)) {
      goto fail;
   }
#else
   /*
    * MC-JIT engine compiles the module immediately on creation, so we can't
    * obtain the target data from it.  Instead we create a target data layout
    * from a string.
    *
    * The produced layout strings are not precisely the same, but should make
    * no difference for the kind of optimization passes we run.
    *
    * For reference this is the layout string on x64:
    *
    *   e-p:64:64:64-S128-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-f128:128:128-n8:16:32:64
    *
    * See also:
    * - http://llvm.org/docs/LangRef.html#datalayout
    */

   {
      const unsigned pointer_size = 8 * sizeof(void *);
      char layout[512];
      util_snprintf(layout, sizeof layout, "%c-p:%u:%u:%u-i64:64:64-a0:0:%u-s0:%u:%u",
#ifdef PIPE_ARCH_LITTLE_ENDIAN
                    'e', // little endian
#else
                    'E', // big endian
#endif
                    pointer_size, pointer_size, pointer_size, // pointer size, abi alignment, preferred alignment
                    pointer_size, // aggregate preferred alignment
                    pointer_size, pointer_size); // stack objects abi alignment, preferred alignment

      gallivm->target = LLVMCreateTargetData(layout);
      if (!gallivm->target) {
         return FALSE;
      }
   }
#endif

   if (!create_pass_manager(gallivm))
      goto fail;

   return TRUE;

fail:
   free_gallivm_state(gallivm);
   return FALSE;
}


void
lp_build_init(void)
{
   if (gallivm_initialized)
      return;

#ifdef DEBUG
   gallivm_debug = debug_get_option_gallivm_debug();
#endif

   lp_set_target_options();

#if USE_MCJIT
   LLVMLinkInMCJIT();
#else
   LLVMLinkInJIT();
#endif

   util_cpu_detect();

   if (HAVE_AVX &&
       util_cpu_caps.has_avx) {
      lp_native_vector_width = 256;
   } else {
      /* Leave it at 128, even when no SIMD extensions are available.
       * Really needs to be a multiple of 128 so can fit 4 floats.
       */
      lp_native_vector_width = 128;
   }
 
   lp_native_vector_width = debug_get_num_option("LP_NATIVE_VECTOR_WIDTH",
                                                 lp_native_vector_width);

   gallivm_initialized = TRUE;

#if 0
   /* For simulating less capable machines */
   util_cpu_caps.has_sse3 = 0;
   util_cpu_caps.has_ssse3 = 0;
   util_cpu_caps.has_sse4_1 = 0;
#endif
}



/**
 * Create a new gallivm_state object.
 * Note that we return a singleton.
 */
struct gallivm_state *
gallivm_create(void)
{
   struct gallivm_state *gallivm;

#if HAVE_LLVM <= 0x206
   if (GlobalGallivm) {
      return GlobalGallivm;
   }
#endif

   gallivm = CALLOC_STRUCT(gallivm_state);
   if (gallivm) {
      if (!init_gallivm_state(gallivm)) {
         FREE(gallivm);
         gallivm = NULL;
      }
   }

#if HAVE_LLVM <= 0x206
   GlobalGallivm = gallivm;
#endif

   return gallivm;
}


/**
 * Destroy a gallivm_state object.
 */
void
gallivm_destroy(struct gallivm_state *gallivm)
{
#if HAVE_LLVM <= 0x0206
   /* No-op: don't destroy the singleton */
   (void) gallivm;
#else
   free_gallivm_state(gallivm);
   FREE(gallivm);
#endif
}


/**
 * Validate and optimze a function.
 */
static void
gallivm_optimize_function(struct gallivm_state *gallivm,
                          LLVMValueRef func)
{
   if (0) {
      debug_printf("optimizing %s...\n", LLVMGetValueName(func));
   }

   assert(gallivm->passmgr);

   /* Apply optimizations to LLVM IR */
   LLVMRunFunctionPassManager(gallivm->passmgr, func);

   if (0) {
      if (gallivm_debug & GALLIVM_DEBUG_IR) {
         /* Print the LLVM IR to stderr */
         lp_debug_dump_value(func);
         debug_printf("\n");
      }
   }
}


/**
 * Validate a function.
 */
void
gallivm_verify_function(struct gallivm_state *gallivm,
                        LLVMValueRef func)
{
   /* Verify the LLVM IR.  If invalid, dump and abort */
#ifdef DEBUG
   if (LLVMVerifyFunction(func, LLVMPrintMessageAction)) {
      lp_debug_dump_value(func);
      assert(0);
      return;
   }
#endif

   gallivm_optimize_function(gallivm, func);

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      /* Print the LLVM IR to stderr */
      lp_debug_dump_value(func);
      debug_printf("\n");
   }
}


void
gallivm_compile_module(struct gallivm_state *gallivm)
{
#if HAVE_LLVM > 0x206
   assert(!gallivm->compiled);
#endif

   /* Dump byte code to a file */
   if (0) {
      LLVMWriteBitcodeToFile(gallivm->module, "llvmpipe.bc");
      debug_printf("llvmpipe.bc written\n");
      debug_printf("Invoke as \"llc -o - llvmpipe.bc\"\n");
   }

#if USE_MCJIT
   assert(!gallivm->engine);
   if (!init_gallivm_engine(gallivm)) {
      assert(0);
   }
#endif
   assert(gallivm->engine);

   ++gallivm->compiled;
}


func_pointer
gallivm_jit_function(struct gallivm_state *gallivm,
                     LLVMValueRef func)
{
   void *code;
   func_pointer jit_func;

   assert(gallivm->compiled);
   assert(gallivm->engine);

   code = LLVMGetPointerToGlobal(gallivm->engine, func);
   assert(code);
   jit_func = pointer_to_func(code);

   if (gallivm_debug & GALLIVM_DEBUG_ASM) {
      lp_disassemble(code);
   }

   /* Free the function body to save memory */
   lp_func_delete_body(func);

   return jit_func;
}


/**
 * Free the function (and its machine code).
 */
void
gallivm_free_function(struct gallivm_state *gallivm,
                      LLVMValueRef func,
                      const void *code)
{
#if !USE_MCJIT
   if (code) {
      LLVMFreeMachineCodeForFunction(gallivm->engine, func);
   }

   LLVMDeleteFunction(func);
#endif
}
