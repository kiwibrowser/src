
#include "radeon_llvm_emit.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <stdio.h>

#include <llvm-c/Core.h>

using namespace llvm;

static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input bitcode>"), cl::init("-"));

static cl::opt<std::string>
TargetGPUName("gpu", cl::desc("target gpu name"), cl::value_desc("gpu_name"));

int main(int argc, char ** argv)
{
	unsigned char * bytes;
	unsigned byte_count;

	std::auto_ptr<Module> M;
	LLVMContext &Context = getGlobalContext();
	SMDiagnostic Err;
	cl::ParseCommandLineOptions(argc, argv, "llvm system compiler\n");
	M.reset(ParseIRFile(InputFilename, Err, Context));

	Module * mod = M.get();
  
	radeon_llvm_compile(wrap(mod), &bytes, &byte_count, TargetGPUName.c_str(), 1);
}
