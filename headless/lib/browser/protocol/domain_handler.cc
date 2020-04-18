// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/protocol/domain_handler.h"

namespace headless {
namespace protocol {

DomainHandler::DomainHandler(const std::string& name,
                             base::WeakPtr<HeadlessBrowserImpl> browser)
    : name_(name), browser_(browser) {}

DomainHandler::~DomainHandler() = default;

void DomainHandler::Wire(UberDispatcher* dispatcher) {}

Response DomainHandler::Disable() {
  return Response::OK();
}

}  // namespace protocol
}  // namespace headless
