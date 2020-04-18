// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_PROTOCOL_DOMAIN_HANDLER_H_
#define HEADLESS_LIB_BROWSER_PROTOCOL_DOMAIN_HANDLER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "headless/lib/browser/protocol/protocol.h"

namespace headless {

class HeadlessBrowserImpl;
class UberDispatcher;

namespace protocol {

class DomainHandler {
 public:
  DomainHandler(const std::string& name,
                base::WeakPtr<HeadlessBrowserImpl> browser);
  virtual ~DomainHandler();

  virtual void Wire(UberDispatcher* dispatcher);
  virtual Response Disable();

  const std::string& name() const { return name_; }
  base::WeakPtr<HeadlessBrowserImpl> browser() { return browser_; }

 private:
  std::string name_;
  base::WeakPtr<HeadlessBrowserImpl> browser_;
  DISALLOW_COPY_AND_ASSIGN(DomainHandler);
};

}  // namespace protocol
}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_PROTOCOL_DOMAIN_HANDLER_H_
