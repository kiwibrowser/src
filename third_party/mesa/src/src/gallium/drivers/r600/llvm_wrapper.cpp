#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

#include "llvm_wrapper.h"


extern "C" LLVMModuleRef llvm_parse_bitcode(const unsigned char * bitcode, unsigned bitcode_len)
{
	llvm::OwningPtr<llvm::Module> M;
	llvm::StringRef str((const char*)bitcode, bitcode_len);
	llvm::MemoryBuffer*  buffer = llvm::MemoryBuffer::getMemBufferCopy(str);
	llvm::SMDiagnostic Err;
	M.reset(llvm::ParseIR(buffer, Err, llvm::getGlobalContext()));
	return wrap(M.take());
}
