// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_EDIT_COMMAND_H_
#define CONTENT_COMMON_EDIT_COMMAND_H_

#include <string>
#include <vector>

namespace content {

// Types related to sending edit commands to the renderer.
struct EditCommand {
  EditCommand() { }
  EditCommand(const std::string& n, const std::string& v)
      : name(n), value(v) {
  }

  std::string name;
  std::string value;
};

typedef std::vector<EditCommand> EditCommands;

}  // namespace content

#endif  // CONTENT_COMMON_EDIT_COMMAND_H_
