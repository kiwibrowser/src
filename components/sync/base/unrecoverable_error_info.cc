// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/unrecoverable_error_info.h"

namespace syncer {

UnrecoverableErrorInfo::UnrecoverableErrorInfo() : is_set_(false) {}

UnrecoverableErrorInfo::UnrecoverableErrorInfo(const base::Location& location,
                                               const std::string& message)
    : location_(location), message_(message), is_set_(true) {}

UnrecoverableErrorInfo::~UnrecoverableErrorInfo() {}

void UnrecoverableErrorInfo::Reset(const base::Location& location,
                                   const std::string& message) {
  location_ = location;
  message_ = message;
  is_set_ = true;
}

bool UnrecoverableErrorInfo::IsSet() const {
  return is_set_;
}

const base::Location& UnrecoverableErrorInfo::location() const {
  return location_;
}

const std::string& UnrecoverableErrorInfo::message() const {
  return message_;
}

}  // namespace syncer
