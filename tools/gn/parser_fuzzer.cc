// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "tools/gn/input_file.h"
#include "tools/gn/parser.h"
#include "tools/gn/source_file.h"
#include "tools/gn/tokenizer.h"

namespace {

enum { kMaxContentDepth = 256, kMaxDodgy = 256 };

// Some auto generated input is too unreasonable for fuzzing GN.
// We see stack overflow when the parser hits really deeply "nested" input.
// (I.E.: certain input that causes nested parsing function calls).
//
// Abstract max limits are undesirable in the release GN code, so some sanity
// checks in the fuzzer to prevent stack overflow are done here.
// - 1) Too many opening bracket, paren, or brace in a row.
// - 2) Too many '!', '<' or '>' operators in a row.
bool SanityCheckContent(const std::vector<Token>& tokens) {
  int depth = 0;
  int dodgy_count = 0;
  for (const auto& token : tokens) {
    switch (token.type()) {
      case Token::LEFT_PAREN:
      case Token::LEFT_BRACKET:
      case Token::LEFT_BRACE:
        ++depth;
        break;
      case Token::RIGHT_PAREN:
      case Token::RIGHT_BRACKET:
      case Token::RIGHT_BRACE:
        --depth;
        break;
      case Token::BANG:
      case Token::LESS_THAN:
      case Token::GREATER_THAN:
        ++dodgy_count;
        break;
      default:
        break;
    }
    // Bail out as soon as a boundary is hit, inside the loop.
    if (depth >= kMaxContentDepth || dodgy_count >= kMaxDodgy)
      return false;
  }

  return true;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size) {
  SourceFile source;
  InputFile input(source);
  input.SetContents(std::string(reinterpret_cast<const char*>(data), size));

  Err err;
  std::vector<Token> tokens = Tokenizer::Tokenize(&input, &err);
  if (!SanityCheckContent(tokens))
    return 0;

  if (!err.has_error())
    Parser::Parse(tokens, &err);

  return 0;
}
