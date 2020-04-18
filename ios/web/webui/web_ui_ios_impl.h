// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEBUI_WEB_UI_IOS_IMPL_H_
#define IOS_WEB_WEBUI_WEB_UI_IOS_IMPL_H_

#include <map>
#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ios/web/public/webui/web_ui_ios.h"

namespace web {
class WebStateImpl;
}

namespace web {

class WebUIIOSImpl : public web::WebUIIOS,
                     public base::SupportsWeakPtr<WebUIIOSImpl> {
 public:
  explicit WebUIIOSImpl(WebStateImpl* web_state);
  ~WebUIIOSImpl() override;

  // WebUIIOS implementation:
  WebState* GetWebState() const override;
  WebUIIOSController* GetController() const override;
  void SetController(std::unique_ptr<WebUIIOSController> controller) override;
  void AddMessageHandler(
      std::unique_ptr<WebUIIOSMessageHandler> handler) override;
  typedef base::Callback<void(const base::ListValue*)> MessageCallback;
  void RegisterMessageCallback(const std::string& message,
                               const MessageCallback& callback) override;
  void ProcessWebUIIOSMessage(const GURL& source_url,
                              const std::string& message,
                              const base::ListValue& args) override;
  void CallJavascriptFunction(const std::string& function_name) override;
  void CallJavascriptFunction(const std::string& function_name,
                              const base::Value& arg) override;
  void CallJavascriptFunction(const std::string& function_name,
                              const base::Value& arg1,
                              const base::Value& arg2) override;
  void CallJavascriptFunction(const std::string& function_name,
                              const base::Value& arg1,
                              const base::Value& arg2,
                              const base::Value& arg3) override;
  void CallJavascriptFunction(const std::string& function_name,
                              const base::Value& arg1,
                              const base::Value& arg2,
                              const base::Value& arg3,
                              const base::Value& arg4) override;
  void CallJavascriptFunction(
      const std::string& function_name,
      const std::vector<const base::Value*>& args) override;

 private:
  // Executes JavaScript asynchronously on the page.
  void ExecuteJavascript(const base::string16& javascript);

  // A map of message name -> message handling callback.
  typedef std::map<std::string, MessageCallback> MessageCallbackMap;
  MessageCallbackMap message_callbacks_;

  // The WebUIIOSMessageHandlers we own.
  std::vector<std::unique_ptr<WebUIIOSMessageHandler>> handlers_;

  // Non-owning pointer to the WebStateImpl this WebUIIOS is associated with.
  WebStateImpl* web_state_;

  std::unique_ptr<WebUIIOSController> controller_;

  DISALLOW_COPY_AND_ASSIGN(WebUIIOSImpl);
};

}  // namespace web

#endif  // IOS_WEB_WEBUI_WEB_UI_IOS_IMPL_H_
