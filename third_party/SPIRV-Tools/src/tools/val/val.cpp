// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "spirv-tools/libspirv.h"

void print_usage(char* argv0) {
  printf(
      R"(%s - Validate a SPIR-V binary file.

USAGE: %s [options] [<filename>]

The SPIR-V binary is read from <filename>. If no file is specified,
or if the filename is "-", then the binary is read from standard input.

NOTE: The validator is a work in progress.

Options:
  -h, --help   Print this help.
  --version    Display validator version information.
)",
      argv0, argv0);
}

int main(int argc, char** argv) {
  const char* inFile = nullptr;
  spv_target_env target_env = SPV_ENV_UNIVERSAL_1_1;

  for (int argi = 1; argi < argc; ++argi) {
    const char* cur_arg = argv[argi];
    if ('-' == cur_arg[0]) {
      if (0 == strcmp(cur_arg, "--version")) {
        printf("%s\n", spvSoftwareVersionDetailsString());
        printf("Targets:\n  %s\n  %s\n",
               spvTargetEnvDescription(SPV_ENV_UNIVERSAL_1_1),
               spvTargetEnvDescription(SPV_ENV_VULKAN_1_0));
        return 0;
      } else if (0 == strcmp(cur_arg, "--help") || 0 == strcmp(cur_arg, "-h")) {
        print_usage(argv[0]);
        return 0;
      } else if (0 == strcmp(cur_arg, "--vulkan")) {
        target_env = SPV_ENV_VULKAN_1_0;
      } else if (0 == cur_arg[1]) {
        // Setting a filename of "-" to indicate stdin.
        if (!inFile) {
          inFile = cur_arg;
        } else {
          fprintf(stderr, "error: More than one input file specified\n");
          return 1;
        }

      } else {
        print_usage(argv[0]);
        return 1;
      }
    } else {
      if (!inFile) {
        inFile = cur_arg;
      } else {
        fprintf(stderr, "error: More than one input file specified\n");
        return 1;
      }
    }
  }

  std::vector<uint32_t> contents;
  const bool use_file = inFile && strcmp("-", inFile);
  if (FILE* fp = (use_file ? fopen(inFile, "rb") : stdin)) {
    uint32_t buf[1024];
    while (size_t len = fread(buf, sizeof(uint32_t),
                              sizeof(buf) / sizeof(uint32_t), fp)) {
      contents.insert(contents.end(), buf, buf + len);
    }
    if (use_file) fclose(fp);
  } else {
    fprintf(stderr, "error: file does not exist '%s'\n", inFile);
    return 1;
  }

  spv_const_binary_t binary = {contents.data(), contents.size()};

  spv_diagnostic diagnostic = nullptr;
  spv_context context = spvContextCreate(target_env);
  spv_result_t error = spvValidate(context, &binary, &diagnostic);
  spvContextDestroy(context);
  if (error) {
    spvDiagnosticPrint(diagnostic);
    spvDiagnosticDestroy(diagnostic);
    return error;
  }

  return 0;
}
