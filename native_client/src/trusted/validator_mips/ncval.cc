/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <vector>
#include <algorithm>

#include "native_client/src/include/nacl_string.h"
#include "native_client/src/trusted/validator/ncfileutil.h"
#include "native_client/src/trusted/validator_mips/model.h"
#include "native_client/src/trusted/validator_mips/validator.h"

using nacl_mips_val::SfiValidator;
using nacl_mips_val::CodeSegment;
using nacl_mips_dec::RegisterList;

using std::string;
using std::vector;

/*
 * Reports problems in an easily-parsed textual format, for consumption by a
 * validation-reporting script.
 *
 * The format is as follows:
 *   ncval: <hex vaddr> <decimal safety> <problem ID string> <hex ref vaddr>
 *
 * For possible safety levels, see inst_classes.h.
 *
 * For possible problem ID strings, see validator.h.
 */
class CommandLineProblemSink : public nacl_mips_val::ProblemSink {
 public:
  virtual void ReportProblem(uint32_t vaddr,
                             nacl_mips_dec::SafetyLevel safety,
                             const nacl::string &problem_code,
                             uint32_t ref_vaddr) {
    fprintf(stderr, "ncval: %08X %d %s %08X\n", vaddr, safety,
        problem_code.c_str(), ref_vaddr);
  }
  virtual bool ShouldContinue() {
    // Collect *all* problems before returning!
    return true;
  }
};

const uint32_t kOneGig = 1U * 1024 * 1024 * 1024;
const uint32_t kQuarterGig = 256U * 1024 * 1024;

int Validate(const ncfile *ncf, bool use_zero_masks) {
  SfiValidator validator(
      16,                             // Bytes per bundle.
      kQuarterGig,                    // Code region size.
      kOneGig,                        // Data region size.
      RegisterList::ReservedRegs(),   // Read only registers.
      RegisterList::DataAddrRegs());  // Data addressing registers.

  if (use_zero_masks) {
    validator.ChangeMasks(0, 0);
  }

  CommandLineProblemSink sink;

  Elf_Shdr *shdr = ncf->sheaders;

  vector<CodeSegment> segments;
  for (int i = 0; i < ncf->shnum; i++) {
    if ((shdr[i].sh_flags & SHF_EXECINSTR) != SHF_EXECINSTR) {
      continue;
    }

    CodeSegment segment(ncf->data + (shdr[i].sh_addr - ncf->vbase),
        shdr[i].sh_addr, shdr[i].sh_size);
    segments.push_back(segment);
  }

  std::sort(segments.begin(), segments.end());

  bool success = validator.Validate(segments, &sink);
  if (!success) return 1;
  return 0;
}

int main(int argc, const char *argv[]) {
  bool use_zero_masks = false;
  const char *filename = NULL;

  for (int i = 1; i < argc; ++i) {
    string o = argv[i];
    if (o == "--zero-masks") {
      use_zero_masks = true;
    } else {
      if (filename != NULL) {
        // trigger error when filename is overwritten
        filename = NULL;
        break;
      }
      filename = argv[i];
    }
  }

  if (NULL == filename) {
    fprintf(stderr, "Usage: %s [--zero-masks] <filename>\n", argv[0]);
    return 2;
  }

  ncfile *ncf = nc_loadfile(filename);
  if (!ncf) {
    fprintf(stderr, "Unable to load %s: %s\n", filename, strerror(errno));
    return 1;
  }

  int exit_code = Validate(ncf, use_zero_masks);
  nc_freefile(ncf);
  return exit_code;
}
