// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_PROTOCOL_BROWSER_HANDLER_H_
#define HEADLESS_LIB_BROWSER_PROTOCOL_BROWSER_HANDLER_H_

#include "headless/lib/browser/protocol/domain_handler.h"
#include "headless/lib/browser/protocol/dp_browser.h"

namespace headless {
namespace protocol {

class BrowserHandler : public DomainHandler, public Browser::Backend {
 public:
  explicit BrowserHandler(base::WeakPtr<HeadlessBrowserImpl> browser);
  ~BrowserHandler() override;

  void Wire(UberDispatcher* dispatcher) override;

  // Browser::Backend implementation
  Response GetWindowForTarget(
      const std::string& target_id,
      int* out_window_id,
      std::unique_ptr<Browser::Bounds>* out_bounds) override;
  Response GetWindowBounds(
      int window_id,
      std::unique_ptr<Browser::Bounds>* out_bounds) override;
  Response Close() override;
  Response SetWindowBounds(
      int window_id,
      std::unique_ptr<Browser::Bounds> out_bounds) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserHandler);
};

}  // namespace protocol
}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_PROTOCOL_BROWSER_HANDLER_H_
