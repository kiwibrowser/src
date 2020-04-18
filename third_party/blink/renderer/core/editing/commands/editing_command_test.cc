// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_editing_command_type.h"
#include "third_party/blink/renderer/core/editing/commands/editor_command.h"
#include "third_party/blink/renderer/core/editing/commands/editor_command_names.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/wtf/string_extras.h"

namespace blink {

namespace {

struct CommandNameEntry {
  const char* name;
  WebEditingCommandType type;
};

const CommandNameEntry kCommandNameEntries[] = {
#define V(name) {#name, WebEditingCommandType::k##name},
    FOR_EACH_BLINK_EDITING_COMMAND_NAME(V)
#undef V
};
// Test all commands except WebEditingCommandType::Invalid.
static_assert(
    arraysize(kCommandNameEntries) + 1 ==
        static_cast<size_t>(WebEditingCommandType::kNumberOfCommandTypes),
    "must test all valid WebEditingCommandType");

}  // anonymous namespace

class EditingCommandTest : public EditingTestBase {};

TEST_F(EditingCommandTest, EditorCommandOrder) {
  for (size_t i = 1; i < arraysize(kCommandNameEntries); ++i) {
    EXPECT_GT(0, strcasecmp(kCommandNameEntries[i - 1].name,
                            kCommandNameEntries[i].name))
        << "EDITOR_COMMAND_MAP must be case-folding ordered. Incorrect index:"
        << i;
  }
}

TEST_F(EditingCommandTest, CreateCommandFromString) {
  Editor& dummy_editor = GetDocument().GetFrame()->GetEditor();
  for (const auto& entry : kCommandNameEntries) {
    const EditorCommand command = dummy_editor.CreateCommand(entry.name);
    EXPECT_EQ(static_cast<int>(entry.type), command.IdForHistogram())
        << entry.name;
  }
}

TEST_F(EditingCommandTest, CreateCommandFromStringCaseFolding) {
  Editor& dummy_editor = GetDocument().GetFrame()->GetEditor();
  for (const auto& entry : kCommandNameEntries) {
    const EditorCommand lower_name_command =
        dummy_editor.CreateCommand(String(entry.name).DeprecatedLower());
    EXPECT_EQ(static_cast<int>(entry.type), lower_name_command.IdForHistogram())
        << entry.name;
    const EditorCommand upper_name_command =
        dummy_editor.CreateCommand(String(entry.name).UpperASCII());
    EXPECT_EQ(static_cast<int>(entry.type), upper_name_command.IdForHistogram())
        << entry.name;
  }
}

TEST_F(EditingCommandTest, CreateCommandFromInvalidString) {
  const String kInvalidCommandName[] = {
      "", "iNvAlId", "12345",
  };
  Editor& dummy_editor = GetDocument().GetFrame()->GetEditor();
  for (const auto& command_name : kInvalidCommandName) {
    const EditorCommand command = dummy_editor.CreateCommand(command_name);
    EXPECT_EQ(0, command.IdForHistogram());
  }
}

}  // namespace blink
