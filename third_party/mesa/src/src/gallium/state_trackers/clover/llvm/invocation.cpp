//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "core/compiler.hpp"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/Bitcode/BitstreamWriter.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Linker.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/PathV1.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "pipe/p_state.h"
#include "util/u_memory.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>

using namespace clover;

namespace {
#if 0
   void
   build_binary(const std::string &source, const std::string &target,
                const std::string &name) {
      clang::CompilerInstance c;
      clang::EmitObjAction act(&llvm::getGlobalContext());
      std::string log;
      llvm::raw_string_ostream s_log(log);

      LLVMInitializeTGSITarget();
      LLVMInitializeTGSITargetInfo();
      LLVMInitializeTGSITargetMC();
      LLVMInitializeTGSIAsmPrinter();

      c.getFrontendOpts().Inputs.push_back(
         std::make_pair(clang::IK_OpenCL, name));
      c.getHeaderSearchOpts().UseBuiltinIncludes = false;
      c.getHeaderSearchOpts().UseStandardIncludes = false;
      c.getLangOpts().NoBuiltin = true;
      c.getTargetOpts().Triple = target;
      c.getInvocation().setLangDefaults(clang::IK_OpenCL);
      c.createDiagnostics(0, NULL, new clang::TextDiagnosticPrinter(
                             s_log, c.getDiagnosticOpts()));

      c.getPreprocessorOpts().addRemappedFile(
         name, llvm::MemoryBuffer::getMemBuffer(source));

      if (!c.ExecuteAction(act))
         throw build_error(log);
   }

   module
   load_binary(const char *name) {
      std::ifstream fs((name));
      std::vector<unsigned char> str((std::istreambuf_iterator<char>(fs)),
                                     (std::istreambuf_iterator<char>()));
      compat::istream cs(str);
      return module::deserialize(cs);
   }
#endif

   llvm::Module *
   compile(const std::string &source, const std::string &name,
           const std::string &triple) {

      clang::CompilerInstance c;
      clang::EmitLLVMOnlyAction act(&llvm::getGlobalContext());
      std::string log;
      llvm::raw_string_ostream s_log(log);

      c.getFrontendOpts().Inputs.push_back(
            clang::FrontendInputFile(name, clang::IK_OpenCL));
      c.getFrontendOpts().ProgramAction = clang::frontend::EmitLLVMOnly;
      c.getHeaderSearchOpts().UseBuiltinIncludes = true;
      c.getHeaderSearchOpts().UseStandardSystemIncludes = true;
      c.getHeaderSearchOpts().ResourceDir = CLANG_RESOURCE_DIR;

      // Add libclc generic search path
      c.getHeaderSearchOpts().AddPath(LIBCLC_INCLUDEDIR,
                                      clang::frontend::Angled,
                                      false, false, false);

      // Add libclc include
      c.getPreprocessorOpts().Includes.push_back("clc/clc.h");

      // clc.h requires that this macro be defined:
      c.getPreprocessorOpts().addMacroDef("cl_clang_storage_class_specifiers");

      c.getLangOpts().NoBuiltin = true;
      c.getTargetOpts().Triple = triple;
      c.getInvocation().setLangDefaults(clang::IK_OpenCL);
      c.createDiagnostics(0, NULL, new clang::TextDiagnosticPrinter(
                          s_log, c.getDiagnosticOpts()));

      c.getPreprocessorOpts().addRemappedFile(name,
                                      llvm::MemoryBuffer::getMemBuffer(source));

      // Compile the code
      if (!c.ExecuteAction(act))
         throw build_error(log);

      return act.takeModule();
   }

   void
   find_kernels(llvm::Module *mod, std::vector<llvm::Function *> &kernels) {
      const llvm::NamedMDNode *kernel_node =
                                 mod->getNamedMetadata("opencl.kernels");
      for (unsigned i = 0; i < kernel_node->getNumOperands(); ++i) {
         kernels.push_back(llvm::dyn_cast<llvm::Function>(
                                    kernel_node->getOperand(i)->getOperand(0)));
      }
   }

   void
   link(llvm::Module *mod, const std::string &triple,
        const std::vector<llvm::Function *> &kernels) {

      llvm::PassManager PM;
      llvm::PassManagerBuilder Builder;
      bool isNative;
      llvm::Linker linker("clover", mod);

      // Link the kernel with libclc
      linker.LinkInFile(llvm::sys::Path(LIBCLC_LIBEXECDIR + triple + ".bc"), isNative);
      mod = linker.releaseModule();

      // Add a function internalizer pass.
      //
      // By default, the function internalizer pass will look for a function
      // called "main" and then mark all other functions as internal.  Marking
      // functions as internal enables the optimizer to perform optimizations
      // like function inlining and global dead-code elimination.
      //
      // When there is no "main" function in a module, the internalize pass will
      // treat the module like a library, and it won't internalize any functions.
      // Since there is no "main" function in our kernels, we need to tell
      // the internalizer pass that this module is not a library by passing a
      // list of kernel functions to the internalizer.  The internalizer will
      // treat the functions in the list as "main" functions and internalize
      // all of the other functions.
      std::vector<const char*> export_list;
      for (std::vector<llvm::Function *>::const_iterator I = kernels.begin(),
                                                         E = kernels.end();
                                                         I != E; ++I) {
         llvm::Function *kernel = *I;
         export_list.push_back(kernel->getName().data());
      }
      PM.add(llvm::createInternalizePass(export_list));

      // Run link time optimizations
      Builder.OptLevel = 2;
      Builder.populateLTOPassManager(PM, false, true);
      PM.run(*mod);
   }

   module
   build_module_llvm(llvm::Module *mod,
                     const std::vector<llvm::Function *> &kernels) {

      module m;
      struct pipe_llvm_program_header header;

      llvm::SmallVector<char, 1024> llvm_bitcode;
      llvm::raw_svector_ostream bitcode_ostream(llvm_bitcode);
      llvm::BitstreamWriter writer(llvm_bitcode);
      llvm::WriteBitcodeToFile(mod, bitcode_ostream);
      bitcode_ostream.flush();

      llvm::Function *kernel_func;
      std::string kernel_name;
      compat::vector<module::argument> args;

      // XXX: Support more than one kernel
      assert(kernels.size() == 1);

      kernel_func = kernels[0];
      kernel_name = kernel_func->getName();

      for (llvm::Function::arg_iterator I = kernel_func->arg_begin(),
                                   E = kernel_func->arg_end(); I != E; ++I) {
         llvm::Argument &arg = *I;
         llvm::Type *arg_type = arg.getType();
         llvm::TargetData TD(kernel_func->getParent());
         unsigned arg_size = TD.getTypeStoreSize(arg_type);

         if (llvm::isa<llvm::PointerType>(arg_type) && arg.hasByValAttr()) {
            arg_type =
               llvm::dyn_cast<llvm::PointerType>(arg_type)->getElementType();
         }

         if (arg_type->isPointerTy()) {
            // XXX: Figure out LLVM->OpenCL address space mappings for each
            // target.  I think we need to ask clang what these are.  For now,
            // pretend everything is in the global address space.
            unsigned address_space = llvm::cast<llvm::PointerType>(arg_type)->getAddressSpace();
            switch (address_space) {
               default:
                  args.push_back(module::argument(module::argument::global, arg_size));
                  break;
            }
         } else {
            args.push_back(module::argument(module::argument::scalar, arg_size));
         }
      }

      header.num_bytes = llvm_bitcode.size();
      std::string data;
      data.insert(0, (char*)(&header), sizeof(header));
      data.insert(data.end(), llvm_bitcode.begin(),
                                  llvm_bitcode.end());
      m.syms.push_back(module::symbol(kernel_name, 0, 0, args ));
      m.secs.push_back(module::section(0, module::section::text,
                                       header.num_bytes, data));

      return m;
   }
} // End anonymous namespace

module
clover::compile_program_llvm(const compat::string &source,
                             enum pipe_shader_ir ir,
                             const compat::string &triple) {

   std::vector<llvm::Function *> kernels;

   llvm::Module *mod = compile(source, "cl_input", triple);

   find_kernels(mod, kernels);

   link(mod, triple, kernels);

   // Build the clover::module
   switch (ir) {
      case PIPE_SHADER_IR_TGSI:
         //XXX: Handle TGSI
         assert(0);
         return module();
      default:
         return build_module_llvm(mod, kernels);
   }
}
