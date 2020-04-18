// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#include "components/zucchini/buffer_view.h"
#include "components/zucchini/disassembler.h"
#include "components/zucchini/disassembler_win32.h"

struct Environment {
  Environment() {
    logging::SetMinLogLevel(3);  // Disable console spamming.
  }
};

Environment* env = new Environment();

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Prep data.
  zucchini::ConstBufferView image(data, size);

  // One of x86 or x64 should return a non-nullptr if the data is valid.

  // Output will be a pointer to zucchini::DisassemblerWin32X86 if successful
  // or nullptr otherwise.
  auto disassembler_win32x86 =
      zucchini::Disassembler::Make<zucchini::DisassemblerWin32X86>(image);
  if (disassembler_win32x86 != nullptr) {
    // Get the image size which has been shruken to the size understood by the
    // parser.
    auto parsed_image_size = disassembler_win32x86->image().size();

    // Parse the Win32 PE file and ensure nothing bad occurs.
    // TODO(ckitagawa): Actually validate that the output reference is within
    // the image.
    auto relocx86 = disassembler_win32x86->MakeReadRelocs(0, parsed_image_size);
    while (relocx86->GetNext().has_value()) {
    }
    auto abs32x86 = disassembler_win32x86->MakeReadAbs32(0, parsed_image_size);
    while (abs32x86->GetNext().has_value()) {
    }
    auto rel32x86 = disassembler_win32x86->MakeReadRel32(0, parsed_image_size);
    while (rel32x86->GetNext().has_value()) {
    }
  }

  // Output will be a pointer to zucchini::DisassemblerWin32X64 if successful
  // or nullptr otherwise.
  auto disassembler_win32x64 =
      zucchini::Disassembler::Make<zucchini::DisassemblerWin32X64>(image);
  if (disassembler_win32x64 != nullptr) {
    // Get the image size which has been shruken to the size understood by the
    // parser.
    auto parsed_image_size = disassembler_win32x64->image().size();

    // Parse the Win32 PE file and ensure nothing bad occurs.
    auto relocx64 = disassembler_win32x64->MakeReadRelocs(0, parsed_image_size);
    while (relocx64->GetNext().has_value()) {
    }
    auto abs32x64 = disassembler_win32x64->MakeReadAbs32(0, parsed_image_size);
    while (abs32x64->GetNext().has_value()) {
    }
    auto rel32x64 = disassembler_win32x64->MakeReadRel32(0, parsed_image_size);
    while (rel32x64->GetNext().has_value()) {
    }
  }
  return 0;
}
