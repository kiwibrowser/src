// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/status.h"

#include "base/debug/stack_trace.h"
#include "base/strings/stringprintf.h"

namespace {

// Returns the string equivalent of the given |ErrorCode|.
const char* DefaultMessageForStatusCode(StatusCode code) {
  switch (code) {
    case kOk:
      return "ok";
    case kNoSuchSession:
      return "no such session";
    case kNoSuchElement:
      return "no such element";
    case kNoSuchFrame:
      return "no such frame";
    case kUnknownCommand:
      return "unknown command";
    case kStaleElementReference:
      return "stale element reference";
    case kElementNotVisible:
      return "element not visible";
    case kInvalidElementState:
      return "invalid element state";
    case kUnknownError:
      return "unknown error";
    case kInvalidArgument:
      return "invalid argument";
    case kMoveTargetOutOfBounds:
      return "move target out of bounds";
    case kElementNotInteractable:
      return "element not interactable";
    case kUnsupportedOperation:
      return "unsupported operation";
    case kJavaScriptError:
      return "javascript error";
    case kXPathLookupError:
      return "xpath lookup error";
    case kTimeout:
      return "timeout";
    case kNoSuchWindow:
      return "no such window";
    case kInvalidCookieDomain:
      return "invalid cookie domain";
    case kUnexpectedAlertOpen:
      return "unexpected alert open";
    case kNoSuchAlert:
      return "no such alert";
    case kScriptTimeout:
      return "asynchronous script timeout";
    case kInvalidSelector:
      return "invalid selector";
    case kSessionNotCreatedException:
      return "session not created exception";
    case kNoSuchExecutionContext:
      return "no such execution context";
    case kChromeNotReachable:
      return "chrome not reachable";
    case kDisconnected:
      return "disconnected";
    case kForbidden:
      return "forbidden";
    case kTabCrashed:
      return "tab crashed";
    case kNoSuchCookie:
      return "no such cookie";
    case kUnableToSetCookie:
      return "unable to set cookie";
    case kTargetDetached:
      return "target frame detached";
    default:
      return "<unknown>";
  }
}

}  // namespace

Status::Status(StatusCode code)
    : code_(code), msg_(DefaultMessageForStatusCode(code)) {
  if (code != kOk)
    stack_trace_ = base::debug::StackTrace().ToString();
}

Status::Status(StatusCode code, const std::string& details)
    : code_(code),
      msg_(DefaultMessageForStatusCode(code) + std::string(": ") + details) {
  if (code != kOk)
        stack_trace_ = base::debug::StackTrace().ToString();
}

Status::Status(StatusCode code, const Status& cause)
    : code_(code),
      msg_(DefaultMessageForStatusCode(code) + std::string("\nfrom ") +
           cause.message()) {
  if (code != kOk)
    stack_trace_ = cause.stack_trace();
}

Status::Status(StatusCode code,
               const std::string& details,
               const Status& cause)
    : code_(code),
      msg_(DefaultMessageForStatusCode(code) + std::string(": ") + details +
           "\nfrom " + cause.message()) {
  if (code != kOk)
    stack_trace_ = cause.stack_trace();
}

Status::~Status() {}

void Status::AddDetails(const std::string& details) {
  msg_ += base::StringPrintf("\n  (%s)", details.c_str());
}

bool Status::IsOk() const {
  return code_ == kOk;
}

bool Status::IsError() const {
  return code_ != kOk;
}

StatusCode Status::code() const {
  return code_;
}

const std::string& Status::message() const {
  return msg_;
}

const std::string& Status::stack_trace() const {
  return stack_trace_;
}
