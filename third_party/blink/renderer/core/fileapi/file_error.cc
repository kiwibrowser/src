/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/fileapi/file_error.h"

#include "third_party/blink/public/platform/web_file_error.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

namespace FileError {

const char kAbortErrorMessage[] =
    "An ongoing operation was aborted, typically with a call to abort().";
const char kEncodingErrorMessage[] =
    "A URI supplied to the API was malformed, or the resulting Data URL has "
    "exceeded the URL length limitations for Data URLs.";
const char kInvalidStateErrorMessage[] =
    "An operation that depends on state cached in an interface object was made "
    "but the state had changed since it was read from disk.";
const char kNoModificationAllowedErrorMessage[] =
    "An attempt was made to write to a file or directory which could not be "
    "modified due to the state of the underlying filesystem.";
const char kNotFoundErrorMessage[] =
    "A requested file or directory could not be found at the time an operation "
    "was processed.";
const char kNotReadableErrorMessage[] =
    "The requested file could not be read, typically due to permission "
    "problems that have occurred after a reference to a file was acquired.";
const char kPathExistsErrorMessage[] =
    "An attempt was made to create a file or directory where an element "
    "already exists.";
const char kQuotaExceededErrorMessage[] =
    "The operation failed because it would cause the application to exceed its "
    "storage quota.";
const char kSecurityErrorMessage[] =
    "It was determined that certain files are unsafe for access within a Web "
    "application, or that too many calls are being made on file resources.";
const char kSyntaxErrorMessage[] =
    "An invalid or unsupported argument was given, like an invalid line ending "
    "specifier.";
const char kTypeMismatchErrorMessage[] =
    "The path supplied exists, but was not an entry of requested type.";

namespace {

ExceptionCode ErrorCodeToExceptionCode(ErrorCode code) {
  switch (code) {
    case kOK:
      return 0;
    case kNotFoundErr:
      return kNotFoundError;
    case kSecurityErr:
      return kSecurityError;
    case kAbortErr:
      return kAbortError;
    case kNotReadableErr:
      return kNotReadableError;
    case kEncodingErr:
      return kEncodingError;
    case kNoModificationAllowedErr:
      return kNoModificationAllowedError;
    case kInvalidStateErr:
      return kInvalidStateError;
    case kSyntaxErr:
      return kSyntaxError;
    case kInvalidModificationErr:
      return kInvalidModificationError;
    case kQuotaExceededErr:
      return kQuotaExceededError;
    case kTypeMismatchErr:
      return kTypeMismatchError;
    case kPathExistsErr:
      return kPathExistsError;
    default:
      NOTREACHED();
      return code;
  }
}

const char* ErrorCodeToMessage(ErrorCode code) {
  // Note that some of these do not set message. If message is 0 then the
  // default message is used.
  switch (code) {
    case kOK:
      return nullptr;
    case kSecurityErr:
      return kSecurityErrorMessage;
    case kNotFoundErr:
      return kNotFoundErrorMessage;
    case kAbortErr:
      return kAbortErrorMessage;
    case kNotReadableErr:
      return kNotReadableErrorMessage;
    case kEncodingErr:
      return kEncodingErrorMessage;
    case kNoModificationAllowedErr:
      return kNoModificationAllowedErrorMessage;
    case kInvalidStateErr:
      return kInvalidStateErrorMessage;
    case kSyntaxErr:
      return kSyntaxErrorMessage;
    case kInvalidModificationErr:
      return nullptr;
    case kQuotaExceededErr:
      return kQuotaExceededErrorMessage;
    case kTypeMismatchErr:
      return nullptr;
    case kPathExistsErr:
      return kPathExistsErrorMessage;
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace

void ThrowDOMException(ExceptionState& exception_state, ErrorCode code) {
  if (code == kOK)
    return;

  // SecurityError is special-cased, as we want to route those exceptions
  // through ExceptionState::throwSecurityError.
  if (code == kSecurityErr) {
    exception_state.ThrowSecurityError(kSecurityErrorMessage);
    return;
  }

  exception_state.ThrowDOMException(ErrorCodeToExceptionCode(code),
                                    ErrorCodeToMessage(code));
}

DOMException* CreateDOMException(ErrorCode code) {
  DCHECK_NE(code, kOK);
  return DOMException::Create(ErrorCodeToExceptionCode(code),
                              ErrorCodeToMessage(code));
}

STATIC_ASSERT_ENUM(kWebFileErrorNotFound, kNotFoundErr);
STATIC_ASSERT_ENUM(kWebFileErrorSecurity, kSecurityErr);
STATIC_ASSERT_ENUM(kWebFileErrorAbort, kAbortErr);
STATIC_ASSERT_ENUM(kWebFileErrorNotReadable, kNotReadableErr);
STATIC_ASSERT_ENUM(kWebFileErrorEncoding, kEncodingErr);
STATIC_ASSERT_ENUM(kWebFileErrorNoModificationAllowed,
                   kNoModificationAllowedErr);
STATIC_ASSERT_ENUM(kWebFileErrorInvalidState, kInvalidStateErr);
STATIC_ASSERT_ENUM(kWebFileErrorSyntax, kSyntaxErr);
STATIC_ASSERT_ENUM(kWebFileErrorInvalidModification, kInvalidModificationErr);
STATIC_ASSERT_ENUM(kWebFileErrorQuotaExceeded, kQuotaExceededErr);
STATIC_ASSERT_ENUM(kWebFileErrorTypeMismatch, kTypeMismatchErr);
STATIC_ASSERT_ENUM(kWebFileErrorPathExists, kPathExistsErr);

}  // namespace FileError

}  // namespace blink
