// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_UNRECOVERABLE_ERROR_INFO_H_
#define COMPONENTS_SYNC_BASE_UNRECOVERABLE_ERROR_INFO_H_
// TODO(lipalani): Figure out the right location for this class so it is
// accessible outside of sync engine as well.

#include <string>

#include "base/location.h"

namespace syncer {

class UnrecoverableErrorInfo {
 public:
  UnrecoverableErrorInfo();
  UnrecoverableErrorInfo(const base::Location& location,
                         const std::string& message);
  ~UnrecoverableErrorInfo();

  void Reset(const base::Location& location, const std::string& message);

  bool IsSet() const;

  const base::Location& location() const;
  const std::string& message() const;

 private:
  base::Location location_;
  std::string message_;
  bool is_set_;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_UNRECOVERABLE_ERROR_INFO_H_
