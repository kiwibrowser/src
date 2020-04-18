// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/automation_extension.h"

#include <utility>

#include "base/time/time.h"
#include "base/values.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/chrome/web_view.h"

AutomationExtension::AutomationExtension(std::unique_ptr<WebView> web_view)
    : web_view_(std::move(web_view)) {}

AutomationExtension::~AutomationExtension() {}

Status AutomationExtension::CaptureScreenshot(std::string* screenshot) {
  base::ListValue args;
  std::unique_ptr<base::Value> result;
  Status status = web_view_->CallAsyncFunction(
      std::string(),
      "captureScreenshot",
      args,
      base::TimeDelta::FromSeconds(10),
      &result);
  if (status.IsError())
    return Status(status.code(), "cannot take screenshot", status);
  if (!result->GetAsString(screenshot))
    return Status(kUnknownError, "screenshot is not a string");
  return Status(kOk);
}

Status AutomationExtension::GetWindowPosition(int* x, int* y) {
  int temp_width, temp_height;
  return GetWindowInfo(x, y, &temp_width, &temp_height);
}

Status AutomationExtension::SetWindowPosition(int x, int y) {
  base::DictionaryValue update_info;
  update_info.SetInteger("left", x);
  update_info.SetInteger("top", y);
  update_info.SetString("state", "normal");
  return UpdateWindow(update_info);
}

Status AutomationExtension::GetWindowSize(int* width, int* height) {
  int temp_x, temp_y;
  return GetWindowInfo(&temp_x, &temp_y, width, height);
}

Status AutomationExtension::SetWindowSize(int width, int height) {
  base::DictionaryValue update_info;
  update_info.SetInteger("width", width);
  update_info.SetInteger("height", height);
  update_info.SetString("state", "normal");
  return UpdateWindow(update_info);
}

Status AutomationExtension::MaximizeWindow() {
  base::DictionaryValue update_info;
  update_info.SetString("state", "maximized");
  return UpdateWindow(update_info);
}

Status AutomationExtension::FullScreenWindow() {
  base::DictionaryValue update_info;
  update_info.SetString("state", "fullscreen");
  return UpdateWindow(update_info);
}

Status AutomationExtension::GetWindowInfo(int* x,
                                          int* y,
                                          int* width,
                                          int* height) {
  base::ListValue args;
  std::unique_ptr<base::Value> result;
  Status status = web_view_->CallAsyncFunction(std::string(),
                                               "getWindowInfo",
                                               args,
                                               base::TimeDelta::FromSeconds(10),
                                               &result);
  if (status.IsError())
    return status;

  base::DictionaryValue* dict;
  int temp_x = 0;
  int temp_y = 0;
  int temp_width = 0;
  int temp_height = 0;
  if (!result->GetAsDictionary(&dict) ||
      !dict->GetInteger("left", &temp_x) ||
      !dict->GetInteger("top", &temp_y) ||
      !dict->GetInteger("width", &temp_width) ||
      !dict->GetInteger("height", &temp_height)) {
    return Status(kUnknownError, "received invalid window info");
  }
  *x = temp_x;
  *y = temp_y;
  *width = temp_width;
  *height = temp_height;
  return Status(kOk);
}

Status AutomationExtension::UpdateWindow(
    const base::DictionaryValue& update_info) {
  base::ListValue args;
  args.Append(update_info.CreateDeepCopy());
  std::unique_ptr<base::Value> result;
  return web_view_->CallAsyncFunction(std::string(),
                                      "updateWindow",
                                      args,
                                      base::TimeDelta::FromSeconds(10),
                                      &result);
}

Status AutomationExtension::LaunchApp(const std::string& id) {
  base::ListValue args;
  args.AppendString(id);
  std::unique_ptr<base::Value> result;
  return web_view_->CallAsyncFunction(std::string(),
                                      "launchApp",
                                      args,
                                      base::TimeDelta::FromSeconds(10),
                                      &result);
}
