// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "tools/cddl/codegen.h"
#include "tools/cddl/logging.h"
#include "tools/cddl/parse.h"
#include "tools/cddl/sema.h"

std::string ReadEntireFile(const std::string& filename) {
  std::ifstream input(filename);
  if (!input) {
    return {};
  }

  input.seekg(0, std::ios_base::end);
  size_t length = input.tellg();
  std::string input_data(length + 1, 0);

  input.seekg(0, std::ios_base::beg);
  input.read(const_cast<char*>(input_data.data()), length);
  input_data[length] = 0;

  return input_data;
}

struct CommandLineArguments {
  std::string header_filename;
  std::string cc_filename;
  std::string gen_dir;
  std::string cddl_filename;
};

CommandLineArguments ParseCommandLineArguments(int argc, char** argv) {
  --argc;
  ++argv;
  CommandLineArguments result;
  while (argc) {
    if (strcmp(*argv, "--header") == 0) {
      // Parse the filename of the output header file. This is also the name
      // that will be used for the include guard and as the  include path in the
      // source file.
      if (!result.header_filename.empty()) {
        return {};
      }
      if (!argc) {
        return {};
      }
      --argc;
      ++argv;
      result.header_filename = *argv;
    } else if (strcmp(*argv, "--cc") == 0) {
      // Parse the filename of the output source file.
      if (!result.cc_filename.empty()) {
        return {};
      }
      if (!argc) {
        return {};
      }
      --argc;
      ++argv;
      result.cc_filename = *argv;
    } else if (strcmp(*argv, "--gen-dir") == 0) {
      // Parse the directory prefix that should be added to the output header.
      // and source file
      if (!result.gen_dir.empty()) {
        return {};
      }
      if (!argc) {
        return {};
      }
      --argc;
      ++argv;
      result.gen_dir = *argv;
    } else if (!result.cddl_filename.empty()) {
      return {};
    } else {
      // The input file which contains the CDDL spec.
      result.cddl_filename = *argv;
    }
    --argc;
    ++argv;
  }

  // If one of the required properties is missed, return empty. Else, return
  // generated struct.
  if (result.header_filename.empty() || result.cc_filename.empty() ||
      result.gen_dir.empty() || result.cddl_filename.empty()) {
    return {};
  }
  return result;
}

int main(int argc, char** argv) {
  // Parse and validate all cmdline arguments.
  CommandLineArguments args = ParseCommandLineArguments(argc, argv);
  if (args.cddl_filename.empty()) {
    std::cerr << "Usage: " << std::endl
              << "cddl --header parsed.h --cc parsed.cc --gen-dir "
                 "output/generated input.cddl"
              << std::endl
              << "All flags are required." << std::endl
              << "Example: " << std::endl
              << "./cddl --header osp_messages.h --cc osp_messages.cc "
                 "--gen-dir gen/msgs ../../msgs/osp_messages.cddl"
              << std::endl;
    return 1;
  }

  size_t pos = args.cddl_filename.find_last_of('.');
  if (pos == std::string::npos) {
    return 1;
  }

  // Validate and open the provided header file.
  std::string header_filename = args.gen_dir + "/" + args.header_filename;
  int header_fd = open(header_filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY,
                       S_IRUSR | S_IWUSR | S_IRGRP);
  if (header_fd == -1) {
    std::cerr << "failed to open " << args.header_filename << std::endl;
    return 1;
  }

  // Validate and open the provided output source file.
  std::string cc_filename = args.gen_dir + "/" + args.cc_filename;
  int cc_fd = open(cc_filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY,
                   S_IRUSR | S_IWUSR | S_IRGRP);
  if (cc_fd == -1) {
    std::cerr << "failed to open " << args.cc_filename << std::endl;
    return 1;
  }

  // Read and parse the CDDL spec file.
  std::string data = ReadEntireFile(args.cddl_filename);
  if (data.empty()) {
    return 1;
  }

  Logger::Log("Successfully initialized CDDL Code generator!");

  // Parse the full CDDL into a graph structure.
  Logger::Log("Parsing CDDL input file...");
  ParseResult parse_result = ParseCddl(data);
  if (!parse_result.root) {
    Logger::Error("Failed to parse CDDL input file");
    return 1;
  }
  Logger::Log("Successfully parsed CDDL input file!");

  // Build the Symbol table from this graph structure.
  Logger::Log("Generating CDDL Symbol Table...");
  std::pair<bool, CddlSymbolTable> cddl_result =
      BuildSymbolTable(*parse_result.root);
  if (!cddl_result.first) {
    Logger::Error("Failed to generate CDDL symbol table");
    return 1;
  }
  Logger::Log("Successfully generated CDDL symbol table!");

  Logger::Log("Generating CPP symbol table...");
  std::pair<bool, CppSymbolTable> cpp_result =
      BuildCppTypes(cddl_result.second);
  if (!cpp_result.first) {
    Logger::Error("Failed to generate CPP symbol table");
    return 1;
  }
  Logger::Log("Successfully generated CPP symbol table!");

  // Validate that the provided CDDL doesnt have duplicated indices.
  if (!ValidateCppTypes(cpp_result.second)) {
    return 1;
  }

  // Create the C++ files from the Symbol table.

  Logger::Log("Writing Header prologue...");
  if (!WriteHeaderPrologue(header_fd, args.header_filename)) {
    Logger::Error("WriteHeaderPrologue failed");
    return 1;
  }
  Logger::Log("Successfully wrote header prologue!");

  Logger::Log("Writing type definitions...");
  if (!WriteTypeDefinitions(header_fd, &cpp_result.second)) {
    Logger::Error("WriteTypeDefinitions failed");
    return 1;
  }
  Logger::Log("Successfully wrote type definitions!");

  Logger::Log("Writing function declaration...");
  if (!WriteFunctionDeclarations(header_fd, &cpp_result.second)) {
    Logger::Error("WriteFunctionDeclarations failed");
    return 1;
  }
  Logger::Log("Successfully wrote function declarations!");

  Logger::Log("Writing header epilogue...");
  if (!WriteHeaderEpilogue(header_fd, args.header_filename)) {
    Logger::Error("WriteHeaderEpilogue failed");
    return 1;
  }
  Logger::Log("Successfully wrote header epilogue!");

  Logger::Log("Writing source prologue...");
  if (!WriteSourcePrologue(cc_fd, args.header_filename)) {
    Logger::Error("WriteSourcePrologue failed");
    return 1;
  }
  Logger::Log("Successfully wrote source prologue!");

  Logger::Log("Writing encoders...");
  if (!WriteEncoders(cc_fd, &cpp_result.second)) {
    Logger::Error("WriteEncoders failed");
    return 1;
  }
  Logger::Log("Successfully wrote encoders!");

  Logger::Log("Writing decoders...");
  if (!WriteDecoders(cc_fd, &cpp_result.second)) {
    Logger::Error("WriteDecoders failed");
    return 1;
  }
  Logger::Log("Successfully wrote decoders!");

  Logger::Log("Writing equality operators...");
  if (!WriteEqualityOperators(cc_fd, &cpp_result.second)) {
    Logger::Error("WriteStructEqualityOperators failed");
    return 1;
  }
  Logger::Log("Successfully wrote equality operators!");

  Logger::Log("Writing source epilogue...");
  if (!WriteSourceEpilogue(cc_fd)) {
    Logger::Error("WriteSourceEpilogue failed");
    return 1;
  }
  Logger::Log("Successfully wrote source epilogue!");

  close(header_fd);
  close(cc_fd);
  Logger::Log("SUCCESSFULLY COMPLETED ALL OPERATIONS");

  return 0;
}
