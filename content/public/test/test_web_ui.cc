// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_web_ui.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace content {

TestWebUI::TestWebUI() : web_contents_(nullptr) {
}

TestWebUI::~TestWebUI() {
  ClearTrackedCalls();
}

void TestWebUI::ClearTrackedCalls() {
  call_data_.clear();
}

WebContents* TestWebUI::GetWebContents() const {
  return web_contents_;
}

WebUIController* TestWebUI::GetController() const {
  return controller_.get();
}

void TestWebUI::SetController(WebUIController* controller) {
  controller_.reset(controller);
}

float TestWebUI::GetDeviceScaleFactor() const {
  return 1.0f;
}

const base::string16& TestWebUI::GetOverriddenTitle() const {
  return temp_string_;
}

int TestWebUI::GetBindings() const {
  return 0;
}

void TestWebUI::AddMessageHandler(
    std::unique_ptr<WebUIMessageHandler> handler) {
  handlers_.push_back(std::move(handler));
}

bool TestWebUI::CanCallJavascript() {
  return true;
}

void TestWebUI::CallJavascriptFunctionUnsafe(const std::string& function_name) {
  call_data_.push_back(base::WrapUnique(new CallData(function_name)));
}

void TestWebUI::CallJavascriptFunctionUnsafe(const std::string& function_name,
                                             const base::Value& arg1) {
  call_data_.push_back(base::WrapUnique(new CallData(function_name)));
  call_data_.back()->TakeAsArg1(arg1.CreateDeepCopy());
}

void TestWebUI::CallJavascriptFunctionUnsafe(const std::string& function_name,
                                             const base::Value& arg1,
                                             const base::Value& arg2) {
  call_data_.push_back(base::WrapUnique(new CallData(function_name)));
  call_data_.back()->TakeAsArg1(arg1.CreateDeepCopy());
  call_data_.back()->TakeAsArg2(arg2.CreateDeepCopy());
}

void TestWebUI::CallJavascriptFunctionUnsafe(const std::string& function_name,
                                             const base::Value& arg1,
                                             const base::Value& arg2,
                                             const base::Value& arg3) {
  call_data_.push_back(base::WrapUnique(new CallData(function_name)));
  call_data_.back()->TakeAsArg1(arg1.CreateDeepCopy());
  call_data_.back()->TakeAsArg2(arg2.CreateDeepCopy());
  call_data_.back()->TakeAsArg3(arg3.CreateDeepCopy());
}

void TestWebUI::CallJavascriptFunctionUnsafe(const std::string& function_name,
                                             const base::Value& arg1,
                                             const base::Value& arg2,
                                             const base::Value& arg3,
                                             const base::Value& arg4) {
  call_data_.push_back(base::WrapUnique(new CallData(function_name)));
  call_data_.back()->TakeAsArg1(arg1.CreateDeepCopy());
  call_data_.back()->TakeAsArg2(arg2.CreateDeepCopy());
  call_data_.back()->TakeAsArg3(arg3.CreateDeepCopy());
  call_data_.back()->TakeAsArg4(arg4.CreateDeepCopy());
}

void TestWebUI::CallJavascriptFunctionUnsafe(
    const std::string& function_name,
    const std::vector<const base::Value*>& args) {
  NOTREACHED();
}

std::vector<std::unique_ptr<WebUIMessageHandler>>*
TestWebUI::GetHandlersForTesting() {
  return &handlers_;
}

TestWebUI::CallData::CallData(const std::string& function_name)
    : function_name_(function_name) {
}

TestWebUI::CallData::~CallData() {
}

void TestWebUI::CallData::TakeAsArg1(std::unique_ptr<base::Value> arg) {
  arg1_ = std::move(arg);
}

void TestWebUI::CallData::TakeAsArg2(std::unique_ptr<base::Value> arg) {
  arg2_ = std::move(arg);
}

void TestWebUI::CallData::TakeAsArg3(std::unique_ptr<base::Value> arg) {
  arg3_ = std::move(arg);
}

void TestWebUI::CallData::TakeAsArg4(std::unique_ptr<base::Value> arg) {
  arg4_ = std::move(arg);
}

}  // namespace content
