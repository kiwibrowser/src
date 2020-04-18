// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/open_from_clipboard/clipboard_recent_content.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "components/variations/variations_associated_data.h"
#include "url/url_constants.h"

namespace {
ClipboardRecentContent* g_clipboard_recent_content = nullptr;

}  // namespace

ClipboardRecentContent::ClipboardRecentContent() {}

ClipboardRecentContent::~ClipboardRecentContent() {
}

// static
ClipboardRecentContent* ClipboardRecentContent::GetInstance() {
  return g_clipboard_recent_content;
}

// static
void ClipboardRecentContent::SetInstance(
    std::unique_ptr<ClipboardRecentContent> new_instance) {
  delete g_clipboard_recent_content;
  g_clipboard_recent_content = new_instance.release();
}

// static
base::TimeDelta ClipboardRecentContent::MaximumAgeOfClipboard() {
  // Identify the current setting for this parameter from the omnibox field
  // trial.
  std::string value_str = variations::GetVariationParamValue(
      "OmniboxBundledExperimentV1", "ClipboardURLMaximumAge");
  // If the parameter is not set, use a 1 hour timeout.
  if (value_str.empty())
    return base::TimeDelta::FromHours(1);
  // This is a best-effort conversion; we trust the hand-crafted parameters
  // downloaded from the server to be perfect.  There's no need for handle
  // errors smartly.
  int value;
  // The value in the parameter is stored in seconds.
  base::StringToInt(value_str, &value);
  return base::TimeDelta::FromSeconds(value);
}
