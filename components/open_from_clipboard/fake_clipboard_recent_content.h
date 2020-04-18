// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OPEN_FROM_CLIPBOARD_FAKE_CLIPBOARD_RECENT_CONTENT_H_
#define COMPONENTS_OPEN_FROM_CLIPBOARD_FAKE_CLIPBOARD_RECENT_CONTENT_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "components/open_from_clipboard/clipboard_recent_content.h"
#include "url/gurl.h"

// FakeClipboardRecentContent implements ClipboardRecentContent interface by
// returning configurable values for use by tests.
class FakeClipboardRecentContent : public ClipboardRecentContent {
 public:
  FakeClipboardRecentContent();
  ~FakeClipboardRecentContent() override;

  // ClipboardRecentContent implementation.
  bool GetRecentURLFromClipboard(GURL* url) override;
  base::TimeDelta GetClipboardContentAge() const override;
  void SuppressClipboardContent() override;

  // Sets the URL and clipboard content age.
  void SetClipboardContent(const GURL& url, base::TimeDelta content_age);

 private:
  GURL clipboard_content_;
  base::TimeDelta content_age_;
  bool suppress_content_;

  DISALLOW_COPY_AND_ASSIGN(FakeClipboardRecentContent);
};

#endif  // COMPONENTS_OPEN_FROM_CLIPBOARD_FAKE_CLIPBOARD_RECENT_CONTENT_H_
