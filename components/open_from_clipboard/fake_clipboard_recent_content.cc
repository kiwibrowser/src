// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/open_from_clipboard/fake_clipboard_recent_content.h"

FakeClipboardRecentContent::FakeClipboardRecentContent()
    : content_age_(base::TimeDelta::Max()), suppress_content_(false) {}

FakeClipboardRecentContent::~FakeClipboardRecentContent() {}

bool FakeClipboardRecentContent::GetRecentURLFromClipboard(GURL* url) {
  if (suppress_content_)
    return false;

  if (!clipboard_content_.is_valid())
    return false;

  *url = clipboard_content_;
  return true;
}

base::TimeDelta FakeClipboardRecentContent::GetClipboardContentAge() const {
  return content_age_;
}

void FakeClipboardRecentContent::SuppressClipboardContent() {
  suppress_content_ = true;
}

void FakeClipboardRecentContent::SetClipboardContent(
    const GURL& url,
    base::TimeDelta content_age) {
  clipboard_content_ = url;
  content_age_ = content_age;
  suppress_content_ = false;
}
