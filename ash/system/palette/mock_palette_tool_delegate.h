// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_PALETTE_MOCK_PALETTE_TOOL_DELEGATE_H_
#define ASH_SYSTEM_PALETTE_MOCK_PALETTE_TOOL_DELEGATE_H_

#include "ash/system/palette/palette_tool.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ash {

// Mock PaletteTool::Delegate class.
class MockPaletteToolDelegate : public PaletteTool::Delegate {
 public:
  MockPaletteToolDelegate();
  ~MockPaletteToolDelegate() override;

  MOCK_METHOD1(EnableTool, void(PaletteToolId tool_id));
  MOCK_METHOD1(DisableTool, void(PaletteToolId tool_id));
  MOCK_METHOD0(HidePalette, void());
  MOCK_METHOD0(HidePaletteImmediately, void());
  MOCK_METHOD0(GetWindow, aura::Window*());
  MOCK_METHOD2(RecordPaletteOptionsUsage,
               void(PaletteTrayOptions option, PaletteInvocationMethod method));
  MOCK_METHOD1(RecordPaletteModeCancellation, void(PaletteModeCancelType type));
};

}  // namespace ash

#endif  // ASH_SYSTEM_PALETTE_MOCK_PALETTE_TOOL_DELEGATE_H_
