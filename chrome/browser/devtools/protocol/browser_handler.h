// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_PROTOCOL_BROWSER_HANDLER_H_
#define CHROME_BROWSER_DEVTOOLS_PROTOCOL_BROWSER_HANDLER_H_

#include "chrome/browser/devtools/protocol/browser.h"

class BrowserHandler : public protocol::Browser::Backend {
 public:
  explicit BrowserHandler(protocol::UberDispatcher* dispatcher);
  ~BrowserHandler() override;

  // Browser::Backend:
  protocol::Response GetWindowForTarget(
      const std::string& target_id,
      int* out_window_id,
      std::unique_ptr<protocol::Browser::Bounds>* out_bounds) override;
  protocol::Response GetWindowBounds(
      int window_id,
      std::unique_ptr<protocol::Browser::Bounds>* out_bounds) override;
  protocol::Response Close() override;
  protocol::Response SetWindowBounds(
      int window_id,
      std::unique_ptr<protocol::Browser::Bounds> out_bounds) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserHandler);
};

#endif  // CHROME_BROWSER_DEVTOOLS_PROTOCOL_BROWSER_HANDLER_H_
