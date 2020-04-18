// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SCRIPT_EXECUTION_CALLBACK_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SCRIPT_EXECUTION_CALLBACK_H_

namespace v8 {
class Value;
template <class T>
class Local;
}

namespace blink {

template <typename T>
class WebVector;

class WebScriptExecutionCallback {
 public:
  virtual ~WebScriptExecutionCallback() = default;

  // Method to be invoked when the asynchronous script is about to execute.
  virtual void WillExecute() {}

  // Method to be invoked when the asynchronous script execution is complete.
  // After function call all objects in vector will be collected
  virtual void Completed(const WebVector<v8::Local<v8::Value>>&) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_SCRIPT_EXECUTION_CALLBACK_H_
