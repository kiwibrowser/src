// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/reference_bytes_mixer.h"

#include "base/logging.h"
#include "components/zucchini/disassembler.h"

namespace zucchini {

/******** ReferenceBytesMixer ********/

// Default implementation is a stub, i.e., for architectures whose references
// have operation bits and payload bits stored in separate bytes. So during
// patch application, payload bits are copied for matched blocks, ignored by
// bytewise corrections, and fixed by reference target corrections.
ReferenceBytesMixer::ReferenceBytesMixer() {}

ReferenceBytesMixer::~ReferenceBytesMixer() = default;

// static.
std::unique_ptr<ReferenceBytesMixer> ReferenceBytesMixer::Create(
    const Disassembler& src_dis,
    const Disassembler& dst_dis) {
  ExecutableType exe_type = src_dis.GetExeType();
  DCHECK_EQ(exe_type, dst_dis.GetExeType());
  // TODO(huangs): Add ARM handling code when ARM is ready.
  return std::make_unique<ReferenceBytesMixer>();
}

// Stub implementation.
int ReferenceBytesMixer::NumBytes(uint8_t type) const {
  return 0;
}

// Base class implementation is a stub that should not be called.
ConstBufferView ReferenceBytesMixer::Mix(
    uint8_t type,
    ConstBufferView::const_iterator old_base,
    offset_t old_offset,
    ConstBufferView::const_iterator new_base,
    offset_t new_offset) {
  NOTREACHED() << "Stub.";
  return ConstBufferView();
}

}  // namespace zucchini
