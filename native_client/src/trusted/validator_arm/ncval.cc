/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <cstdio>
#include <cstdarg>
#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

#include "native_client/src/include/nacl_string.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator/ncfileutil.h"
#include "native_client/src/trusted/cpu_features/arch/arm/cpu_arm.h"
#include "native_client/src/trusted/validator_arm/model.h"
#include "native_client/src/trusted/validator_arm/problem_reporter.h"
#include "native_client/src/trusted/validator_arm/validator.h"

using nacl_arm_val::SfiValidator;
using nacl_arm_val::CodeSegment;
using nacl_arm_val::ProblemReporter;
using nacl_arm_dec::MAY_BE_SAFE;

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
class NcvalProblemReporter : public ProblemReporter {
 protected:
  virtual void ReportProblemMessage(nacl_arm_dec::Violation violation,
                                    uint32_t vaddr,
                                    const char* message);
};

void NcvalProblemReporter::
ReportProblemMessage(nacl_arm_dec::Violation violation,
                     uint32_t vaddr,
                     const char* message) {
  UNREFERENCED_PARAMETER(vaddr);
  UNREFERENCED_PARAMETER(violation);
  printf("%8" NACL_PRIx32 ": %s\n", vaddr, message);
}

const uint32_t kOneGig = 1U * 1024 * 1024 * 1024;

int validate(const ncfile *ncf, const NaClCPUFeaturesArm *cpu_features) {
  SfiValidator validator(
      16,  // bytes per bundle
      // TODO(cbiffle): maybe check region sizes from ELF headers?
      //                verify that instructions are in right region
      kOneGig,  // code region size
      kOneGig,  // data region size
      nacl_arm_dec::RegisterList(nacl_arm_dec::Register::Tp()),
      nacl_arm_dec::RegisterList(nacl_arm_dec::Register::Sp()),
      cpu_features);

  NcvalProblemReporter reporter;

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

  bool success = validator.validate(segments, &reporter);
  if (!success) return 1;
  return 0;
}

static inline uint8_t as_hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 0xa;
  if (c >= 'A' && c <= 'F') return c - 'A' + 0xa;
  CHECK(false);
  return 0;
}

int check_code(const std::string& code) {
  NaClCPUFeaturesArm cpu_features;
  SfiValidator validator(
      16,  // bytes per bundle
      kOneGig,  // code region size
      kOneGig,  // data region size
      nacl_arm_dec::RegisterList(nacl_arm_dec::Register::Tp()),
      nacl_arm_dec::RegisterList(nacl_arm_dec::Register::Sp()),
      &cpu_features);
  NcvalProblemReporter reporter;

  uint8_t* code_buf = new uint8_t[code.length()];
  uint32_t code_len = 0;
  for (uint32_t i = 0; i < code.length(); i += 3) {
    code_buf[code_len++] = (as_hex(code[i]) << 4) | as_hex(code[i+1]);
  }
  printf("Validating code: ");
  for (uint32_t i = 0; i < code_len; i++) {
    printf("%02x ", code_buf[i]);
  }
  printf("\n");

  vector<CodeSegment> segments;
  CodeSegment segment(code_buf, 0, code_len);
  segments.push_back(segment);
  bool success = validator.validate(segments, &reporter);
  if (success) {
    printf("valid!\n");
    return 0;
  } else {
    printf("invalid\n");
    return 1;
  }
}

int main(int argc, const char *argv[]) {
  static const char cond_mem_access_flag[] =
      "--conditional_memory_access_allowed_for_sfi";

  static const char number_runs_flag[] =
      "--number_runs";

  NaClCPUFeaturesArm cpu_features;
  NaClClearCPUFeaturesArm(&cpu_features);

  int exit_code = EXIT_FAILURE;
  bool print_usage = false;
  bool run_validation = false;
  int number_runs = 1;
  std::string filename;
  ncfile *ncf = NULL;
  std::string code;

  for (int i = 1; i < argc; ++i) {
    std::string current_arg = argv[i];
    if (current_arg == "--usage" ||
        current_arg == "--help") {
      print_usage = true;
      run_validation = false;
      break;
    } else if (current_arg == number_runs_flag) {
      // next argument is the number of runs to try. Used when timing
      // performance, and want to run many times to get a good average
      // time result.
      ++i;
      if (i < argc) {
        number_runs = atoi(argv[i]);
      } else {
        // No value to got with argument, fail to run.
        print_usage = true;
        run_validation = false;
      }
    } else if (current_arg == cond_mem_access_flag) {
      // This flag is disallowed by default: not all ARM CPUs support it,
      // so be pessimistic unless the user asks for it.
      NaClSetCPUFeatureArm(&cpu_features, NaClCPUFeatureArm_CanUseTstMem, 1);
    } else if (current_arg == "--check" && i + 1 < argc) {
      code = argv[i+1];
      i++;
    } else if (!filename.empty()) {
      fprintf(stderr, "Error: multiple files specified.\n");
      print_usage = true;
      run_validation = false;
      break;
    } else {
      filename = current_arg;
      run_validation = true;
    }
  }

  if (!code.empty()) {
      return check_code(code);
  }

  if (filename.empty()) {
    fprintf(stderr, "Error: no file specified.\n");
    print_usage = true;
    run_validation = false;
  } else {
    ncf = nc_loadfile(filename.c_str());
    if (!ncf) {
      int err = errno;
      fprintf(stderr, "Error: unable to load file %s: %s.\n",
              filename.c_str(), strerror(err));
      print_usage = true;
      run_validation = false;
    }
  }

  if (print_usage) {
    fprintf(stderr, "Usage: %s [options] <filename>\n", argv[0]);
    fprintf(stderr, "   %s\n", cond_mem_access_flag);
    fprintf(stderr, "      Allow conditions on memory access.\n");
    fprintf(stderr, "   %s n\n", number_runs_flag);
    fprintf(stderr, "      Validate input n times.\n");
    fprintf(stderr, "      Used for performance timing.\n");
  }

  // TODO(cbiffle): check OS ABI, ABI version, align mask

  if (run_validation) {
    exit_code = 0;
    for (int i = 0; i < number_runs; ++i) {
      exit_code |= validate(ncf, &cpu_features);
    }
    if (exit_code == 0)
      printf("Valid.\n");
    else
      printf("Invalid.\n");
  }

  if (ncf)
      nc_freefile(ncf);

  return exit_code;
}
