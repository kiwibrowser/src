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

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "spirv-tools/libspirv.h"

static void print_usage(char* argv0) {
  printf(
      R"(%s - Disassemble a SPIR-V binary module

Usage: %s [options] [<filename>]

The SPIR-V binary is read from <filename>. If no file is specified,
or if the filename is "-", then the binary is read from standard input.

Options:

  -h, --help      Print this help.
  --version       Display disassembler version information.

  -o <filename>   Set the output filename.
                  Output goes to standard output if this option is
                  not specified, or if the filename is "-".

  --no-color      Don't print in color.
                  The default when output goes to a file.

  --no-indent     Don't indent instructions.

  --offsets       Show byte offsets for each instruction.
)",
      argv0, argv0);
}

int main(int argc, char** argv) {
  const char* inFile = nullptr;
  const char* outFile = nullptr;

  bool allow_color = false;
#ifdef SPIRV_COLOR_TERMINAL
  allow_color = true;
#endif
  bool allow_indent = true;
  bool show_byte_offsets = false;

  for (int argi = 1; argi < argc; ++argi) {
    if ('-' == argv[argi][0]) {
      switch (argv[argi][1]) {
        case 'h':
          print_usage(argv[0]);
          return 0;
        case 'o': {
          if (!outFile && argi + 1 < argc) {
            outFile = argv[++argi];
          } else {
            print_usage(argv[0]);
            return 1;
          }
        } break;
        case '-': {
          // Long options
          if (0 == strcmp(argv[argi], "--no-color")) {
            allow_color = false;
          } else if (0 == strcmp(argv[argi], "--no-indent")) {
            allow_indent = false;
          } else if (0 == strcmp(argv[argi], "--offsets")) {
            show_byte_offsets = true;
          } else if (0 == strcmp(argv[argi], "--help")) {
            print_usage(argv[0]);
            return 0;
          } else if (0 == strcmp(argv[argi], "--version")) {
            printf("%s\n", spvSoftwareVersionDetailsString());
            printf("Target: %s\n",
                   spvTargetEnvDescription(SPV_ENV_UNIVERSAL_1_1));
            return 0;
          } else {
            print_usage(argv[0]);
            return 1;
          }
        } break;
        case 0: {
          // Setting a filename of "-" to indicate stdin.
          if (!inFile) {
            inFile = argv[argi];
          } else {
            fprintf(stderr, "error: More than one input file specified\n");
            return 1;
          }
        } break;
        default:
          print_usage(argv[0]);
          return 1;
      }
    } else {
      if (!inFile) {
        inFile = argv[argi];
      } else {
        fprintf(stderr, "error: More than one input file specified\n");
        return 1;
      }
    }
  }

  uint32_t options = SPV_BINARY_TO_TEXT_OPTION_NONE;

  if (allow_indent) options |= SPV_BINARY_TO_TEXT_OPTION_INDENT;

  if (show_byte_offsets) options |= SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET;

  if (!outFile || (0 == strcmp("-", outFile))) {
    // Print to standard output.
    options |= SPV_BINARY_TO_TEXT_OPTION_PRINT;
    if (allow_color) {
      options |= SPV_BINARY_TO_TEXT_OPTION_COLOR;
    }
  }

  // Read the input binary.
  std::vector<uint32_t> contents;
  {
    FILE* input = stdin;
    const bool use_file = inFile && strcmp("-", inFile);
    if (use_file) {
      input = fopen(inFile, "rb");
      if (!input) {
        auto msg =
            std::string("error: Can't open file ") + inFile + " for reading";
        perror(msg.c_str());
        return 1;
      }
    }
    uint32_t buf[1024];
    while (size_t len = fread(buf, sizeof(uint32_t), 1024, input)) {
      contents.insert(contents.end(), buf, buf + len);
    }
    if (use_file) fclose(input);
  }

  // If printing to standard output, then spvBinaryToText should
  // do the printing.  In particular, colour printing on Windows is
  // controlled by modifying console objects synchronously while
  // outputting to the stream rather than by injecting escape codes
  // into the output stream.
  // If the printing option is off, then save the text in memory, so
  // it can be emitted later in this function.
  const bool print_to_stdout = SPV_BINARY_TO_TEXT_OPTION_PRINT & options;
  spv_text text;
  spv_text* textOrNull = print_to_stdout ? nullptr : &text;
  spv_diagnostic diagnostic = nullptr;
  spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_1);
  spv_result_t error =
      spvBinaryToText(context, contents.data(), contents.size(), options,
                      textOrNull, &diagnostic);
  spvContextDestroy(context);
  if (error) {
    spvDiagnosticPrint(diagnostic);
    spvDiagnosticDestroy(diagnostic);
    return error;
  }

  // Output the result.
  if (!print_to_stdout) {
    if (FILE* fp = fopen(outFile, "w")) {
      size_t written =
          fwrite(text->str, sizeof(char), (size_t)text->length, fp);
      if (text->length != written) {
        spvTextDestroy(text);
        fprintf(stderr, "error: Could not write to file '%s'\n", outFile);
        return 1;
      }
    } else {
      spvTextDestroy(text);
      fprintf(stderr, "error: Could not open file '%s'\n", outFile);
      return 1;
    }
  }

  return 0;
}
