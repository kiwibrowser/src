// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_STATUS_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_STATUS_H_

#include <string>

// WebDriver standard status codes.
enum StatusCode {
  kOk = 0,
  kNoSuchSession = 6,
  kNoSuchElement = 7,
  kNoSuchFrame = 8,
  kUnknownCommand = 9,
  kStaleElementReference = 10,
  kElementNotVisible = 11,
  kInvalidElementState = 12,
  kUnknownError = 13,
  kInvalidArgument = 14,
  kElementNotInteractable = 15,
  kUnsupportedOperation = 16,
  kJavaScriptError = 17,
  kMoveTargetOutOfBounds = 18,
  kXPathLookupError = 19,
  kUnableToSetCookie = 20,
  kTimeout = 21,
  kNoSuchWindow = 23,
  kInvalidCookieDomain = 24,
  kUnexpectedAlertOpen = 26,
  kNoSuchAlert = 27,
  kScriptTimeout = 28,
  kInvalidSelector = 32,
  kSessionNotCreatedException = 33,
  // Chrome-specific status codes.
  kChromeNotReachable = 100,
  kNoSuchExecutionContext,
  kDisconnected,
  kForbidden = 103,
  kTabCrashed,
  kNoSuchCookie,
  kTargetDetached,
};

// Represents a WebDriver status, which may be an error or ok.
class Status {
 public:
  explicit Status(StatusCode code);
  Status(StatusCode code, const std::string& details);
  Status(StatusCode code, const Status& cause);
  Status(StatusCode code, const std::string& details, const Status& cause);
  ~Status();

  void AddDetails(const std::string& details);

  bool IsOk() const;
  bool IsError() const;

  StatusCode code() const;

  const std::string& message() const;

  const std::string& stack_trace() const;

 private:
  StatusCode code_;
  std::string msg_;
  std::string stack_trace_;
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_STATUS_H_
