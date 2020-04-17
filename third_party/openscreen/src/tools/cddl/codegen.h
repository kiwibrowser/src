// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_CDDL_CODEGEN_H_
#define TOOLS_CDDL_CODEGEN_H_

#include "tools/cddl/sema.h"

bool WriteTypeDefinitions(int fd, CppSymbolTable* table);
bool WriteFunctionDeclarations(int fd, CppSymbolTable* table);
bool WriteEncoders(int fd, CppSymbolTable* table);
bool WriteDecoders(int fd, CppSymbolTable* table);
bool WriteEqualityOperators(int fd, CppSymbolTable* table);
bool WriteHeaderPrologue(int fd, const std::string& header_filename);
bool WriteHeaderEpilogue(int fd, const std::string& header_filename);
bool WriteSourcePrologue(int fd, const std::string& header_filename);
bool WriteSourceEpilogue(int fd);

#endif  // TOOLS_CDDL_CODEGEN_H_
