// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/install/crx_install_error.h"
#include "extensions/browser/install/sandboxed_unpacker_failure_reason.h"

namespace extensions {

CrxInstallError::CrxInstallError(CrxInstallErrorType type,
                                 CrxInstallErrorDetail detail,
                                 const base::string16& message)
    : type_(type), detail_(detail), message_(message) {
  DCHECK_NE(CrxInstallErrorType::NONE, type);
  DCHECK_NE(CrxInstallErrorType::SANDBOXED_UNPACKER_FAILURE, type);
}

CrxInstallError::CrxInstallError(CrxInstallErrorType type,
                                 CrxInstallErrorDetail detail)
    : CrxInstallError(type, detail, base::string16()) {}

CrxInstallError::CrxInstallError(SandboxedUnpackerFailureReason reason,
                                 const base::string16& message)
    : type_(CrxInstallErrorType::SANDBOXED_UNPACKER_FAILURE),
      detail_(CrxInstallErrorDetail::NONE),
      sandbox_failure_detail_(reason),
      message_(message) {}

CrxInstallError::CrxInstallError(const CrxInstallError& other) = default;
CrxInstallError::CrxInstallError(CrxInstallError&& other) = default;
CrxInstallError& CrxInstallError::operator=(const CrxInstallError& other) =
    default;
CrxInstallError& CrxInstallError::operator=(CrxInstallError&& other) = default;

CrxInstallError::~CrxInstallError() = default;

// For SANDBOXED_UNPACKER_FAILURE type, use sandbox_failure_detail().
CrxInstallErrorDetail CrxInstallError::detail() const {
  DCHECK_NE(CrxInstallErrorType::SANDBOXED_UNPACKER_FAILURE, type_);
  return detail_;
}

// sandbox_failure_detail() only returns a value when the error type is
// SANDBOXED_UNPACKER_FAILURE.
SandboxedUnpackerFailureReason CrxInstallError::sandbox_failure_detail() const {
  DCHECK_EQ(CrxInstallErrorType::SANDBOXED_UNPACKER_FAILURE, type_);
  DCHECK(sandbox_failure_detail_);
  return sandbox_failure_detail_.value();
}

}  // namespace extensions
