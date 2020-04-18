#ifndef LLVM_WRAPPER_H
#define LLVM_WRAPPER_H

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

LLVMModuleRef llvm_parse_bitcode(const unsigned char * bitcode, unsigned bitcode_len);

#ifdef __cplusplus
}
#endif

#endif
