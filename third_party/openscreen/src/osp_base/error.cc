// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/error.h"

namespace openscreen {

Error::Error() = default;

Error::Error(const Error& error) = default;

Error::Error(Error&& error) = default;

Error::Error(Code code) : code_(code) {}

Error::Error(Code code, const std::string& message)
    : code_(code), message_(message) {}

Error::Error(Code code, std::string&& message)
    : code_(code), message_(std::move(message)) {}

Error::~Error() = default;

Error& Error::operator=(const Error& other) = default;

Error& Error::operator=(Error&& other) = default;

bool Error::operator==(const Error& other) const {
  return code_ == other.code_ && message_ == other.message_;
}

// TODO(jophba): integrate with jrw@ utility library once it
// has landed, to avoid kludgy string construction here.
std::string Error::CodeToString(Error::Code code) {
  switch (code) {
    case Error::Code::kNone:
      return "None";
    case Error::Code::kCborParsing:
      return "CborParsingError";
    case Error::Code::kNoAvailableReceivers:
      return "NoAvailableReceivers";
    case Error::Code::kRequestCancelled:
      return "RequestCancelled";
    case Error::Code::kNoPresentationFound:
      return "NoPresentationFound";
    case Error::Code::kPreviousStartInProgress:
      return "PreviousStartInProgress";
    case Error::Code::kUnknownStartError:
      return "UnknownStartError";
    default:
      return "Unknown";
  }
}

// static
const Error& Error::None() {
  static Error& error = *new Error(Code::kNone);
  return error;
}

std::ostream& operator<<(std::ostream& out, const Error& error) {
  out << Error::CodeToString(error.code()) << ": " << error.message();
  return out;
}

}  // namespace openscreen
