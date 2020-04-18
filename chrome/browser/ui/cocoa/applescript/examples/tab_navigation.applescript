-- Copyright (c) 2010 The Chromium Authors. All rights reserved.
-- Use of this source code is governed by a BSD-style license that can be
-- found in the LICENSE file.

tell application "Chromium"
  tell window 1
    -- creates a new tab and navigates to a particular URL.
    make new tab with properties {URL:"http://google.com"}
    -- Duplicate a tab.
    set var to URL of tab 2
    make new tab with properties {URL:var}
  end tell
end tell
