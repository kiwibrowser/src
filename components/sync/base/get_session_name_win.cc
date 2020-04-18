// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/get_session_name_win.h"

#include <windows.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"

namespace syncer {
namespace internal {

std::string GetComputerName() {
  wchar_t computer_name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
  DWORD size = arraysize(computer_name);
  if (::GetComputerNameW(computer_name, &size)) {
    std::string result;
    bool conversion_successful = base::WideToUTF8(computer_name, size, &result);
    DCHECK(conversion_successful);
    return result;
  }
  return std::string();
}

}  // namespace internal
}  // namespace syncer
