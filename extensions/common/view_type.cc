// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/view_type.h"

#include "base/strings/string_piece.h"

namespace extensions {

bool GetViewTypeFromString(const std::string& view_type,
                           ViewType* view_type_out) {
  // TODO(devlin): This map doesn't contain the following values:
  // - VIEW_TYPE_BACKGROUND_CONTENTS
  // - VIEW_TYPE_COMPONENT
  // - VIEW_TYPE_EXTENSION_GUEST
  // Why? Is it just because we don't expose those types to JS?
  static const struct {
    ViewType type;
    base::StringPiece name;
  } kTypeMap[] = {
      {VIEW_TYPE_APP_WINDOW, "APP_WINDOW"},
      {VIEW_TYPE_EXTENSION_BACKGROUND_PAGE, "BACKGROUND"},
      {VIEW_TYPE_EXTENSION_DIALOG, "EXTENSION_DIALOG"},
      {VIEW_TYPE_EXTENSION_POPUP, "POPUP"},
      {VIEW_TYPE_PANEL, "PANEL"},
      {VIEW_TYPE_TAB_CONTENTS, "TAB"},
  };

  for (const auto& entry : kTypeMap) {
    if (entry.name == view_type) {
      *view_type_out = entry.type;
      return true;
    }
  }

  return false;
}

}  // namespace extensions
