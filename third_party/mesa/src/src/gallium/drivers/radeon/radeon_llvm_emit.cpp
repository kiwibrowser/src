/*
 * Copyright 2011 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Tom Stellard <thomas.stellard@amd.com>
 *
 */
#include "radeon_llvm_emit.h"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Threading.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetMachine.h>

#include <llvm/Transforms/Scalar.h>

#include <llvm-c/Target.h>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace llvm;

#ifndef EXTERNAL_LLVM
extern "C" {

void LLVMInitializeAMDGPUAsmPrinter(void);
void LLVMInitializeAMDGPUTargetMC(void);
void LLVMInitializeAMDGPUTarget(void);
void LLVMInitializeAMDGPUTargetInfo(void);
}
#endif

namespace {

class LLVMEnsureMultithreaded {
public:
   LLVMEnsureMultithreaded()
   {
      llvm_start_multithreaded();
   }
};

static LLVMEnsureMultithreaded lLVMEnsureMultithreaded;

}

/**
 * Compile an LLVM module to machine code.
 *
 * @param bytes This function allocates memory for the byte stream, it is the
 * caller's responsibility to free it.
 */
extern "C" unsigned
radeon_llvm_compile(LLVMModuleRef M, unsigned char ** bytes,
                 unsigned * byte_count, const char * gpu_family,
                 unsigned dump) {

   Triple AMDGPUTriple(sys::getDefaultTargetTriple());

#ifdef EXTERNAL_LLVM
   /* XXX: Can we just initialize the AMDGPU target here? */
   InitializeAllTargets();
   InitializeAllTargetMCs();
#else
   LLVMInitializeAMDGPUTargetInfo();
   LLVMInitializeAMDGPUTarget();
   LLVMInitializeAMDGPUTargetMC();
   LLVMInitializeAMDGPUAsmPrinter();
#endif
   std::string err;
   const Target * AMDGPUTarget = TargetRegistry::lookupTarget("r600", err);
   if(!AMDGPUTarget) {
      fprintf(stderr, "Can't find target: %s\n", err.c_str());
      return 1;
   }
   
   Triple::ArchType Arch = Triple::getArchTypeForLLVMName("r600");
   if (Arch == Triple::UnknownArch) {
      fprintf(stderr, "Unknown Arch\n");
   }
   AMDGPUTriple.setArch(Arch);

   Module * mod = unwrap(M);
   std::string FS;
   TargetOptions TO;

   if (dump) {
      mod->dump();
      FS += "+DumpCode";
   }

   std::auto_ptr<TargetMachine> tm(AMDGPUTarget->createTargetMachine(
                     AMDGPUTriple.getTriple(), gpu_family, FS,
                     TO, Reloc::Default, CodeModel::Default,
                     CodeGenOpt::Default
                     ));
   TargetMachine &AMDGPUTargetMachine = *tm.get();
   PassManager PM;
   PM.add(new TargetData(*AMDGPUTargetMachine.getTargetData()));
   PM.add(createPromoteMemoryToRegisterPass());
   AMDGPUTargetMachine.setAsmVerbosityDefault(true);

   std::string CodeString;
   raw_string_ostream oStream(CodeString);
   formatted_raw_ostream out(oStream);

   /* Optional extra paramater true / false to disable verify */
   if (AMDGPUTargetMachine.addPassesToEmitFile(PM, out, TargetMachine::CGFT_ObjectFile,
                                               true)){
      fprintf(stderr, "AddingPasses failed.\n");
      return 1;
   }
   PM.run(*mod);

   out.flush();
   std::string &data = oStream.str();

   *bytes = (unsigned char*)malloc(data.length() * sizeof(unsigned char));
   memcpy(*bytes, data.c_str(), data.length() * sizeof(unsigned char));
   *byte_count = data.length();

   return 0;
}
