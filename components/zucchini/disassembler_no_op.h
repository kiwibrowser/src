// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZUCCHINI_DISASSEMBLER_NO_OP_H_
#define COMPONENTS_ZUCCHINI_DISASSEMBLER_NO_OP_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/zucchini/buffer_view.h"
#include "components/zucchini/disassembler.h"
#include "components/zucchini/image_utils.h"

namespace zucchini {

// This disassembler works on any file and does not look for reference.
class DisassemblerNoOp : public Disassembler {
 public:
  DisassemblerNoOp();
  ~DisassemblerNoOp() override;

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  std::vector<ReferenceGroup> MakeReferenceGroups() const override;

 private:
  friend Disassembler;

  bool Parse(ConstBufferView image) override;

  DISALLOW_COPY_AND_ASSIGN(DisassemblerNoOp);
};

}  // namespace zucchini

#endif  // COMPONENTS_ZUCCHINI_DISASSEMBLER_NO_OP_H_
