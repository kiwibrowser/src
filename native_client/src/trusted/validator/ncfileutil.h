/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncfileutil.h - open an executable file. FOR TESTING ONLY.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCFILEUTIL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCFILEUTIL_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/elf.h"
#include "native_client/src/trusted/validator/types_memory_model.h"

EXTERN_C_BEGIN

/* memory access permissions have a granularity of 4KB pages. */
static const int kNCFileUtilPageShift =      12;
static const Elf_Addr kNCFileUtilPageSize = (1 << 12);
/* this needs to be a #define because it's used as an array size */
#if NACL_BUILD_SUBARCH == 64
#define kMaxPhnum 64
#else
#define kMaxPhnum 32
#endif

/* Function signature for error printing. */
typedef void (*nc_loadfile_error_fn)(const char* format,
                                     ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

typedef struct {
  const char* fname;     /* name of loaded file */
  NaClPcAddress vbase;   /* base address in virtual memory */
  NaClMemorySize size;   /* size of program memory         */
  uint8_t* data;         /* the actual loaded bytes        */
  Elf_Half phnum;        /* number of Elf program headers */
  Elf_Phdr* pheaders;    /* copy of the Elf program headers */
  Elf_Half shnum;        /* number of Elf section headers */
  Elf_Shdr* sheaders;    /* copy of the Elf section headers */
  nc_loadfile_error_fn error_fn;  /* The error printing routine to use. */
} ncfile;

/* Loads the given filename into memory. If error_fn is NULL, a default
 * error printing routine will be used.
 */
ncfile *nc_loadfile_depending(const char* filename,
                              nc_loadfile_error_fn error_fn);

/* Loads the given filename into memory, applying native client rules.
 * Uses the default error printing function.
*/
ncfile *nc_loadfile(const char *filename);

/* Loads the given filename into memory, applying native client rules.
 * Uses the given error printing function to record errors.
*/
ncfile *nc_loadfile_with_error_fn(const char *filename,
                                  nc_loadfile_error_fn error_fn);

void nc_freefile(ncfile* ncf);

void GetVBaseAndLimit(ncfile* ncf, NaClPcAddress* vbase, NaClPcAddress* vlimit);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCFILEUTIL_H_ */
