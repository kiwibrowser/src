// Copyright 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef LIBLOUIS_NACL_LIBLOUIS_INSTANCE_H_
#define LIBLOUIS_NACL_LIBLOUIS_INSTANCE_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "liblouis_wrapper.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/message_loop.h"
#include "ppapi/cpp/var.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "translation_params.h"

namespace liblouis_nacl {

class LibLouisInstance : public pp::Instance {
 public:
  explicit LibLouisInstance(PP_Instance instance);
  virtual ~LibLouisInstance();

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual void HandleMessage(const pp::Var& var_message);

 private:
  // Post work to the background (liblouis) thread.
  int32_t PostWorkToBackground(const pp::CompletionCallback& callback) {
    return liblouis_thread_.message_loop().PostWork(callback);
  }

  // Encodes a message as JSON and sends it over to JavaScript.
  void PostReply(Json::Value reply, const std::string& in_reply_to);

  // Posts an error response to JavaScript.
  void PostError(const std::string& error);

  // Posts an error response to JavaScript, with the message ID of the call
  // which caused it.
  void PostError(const std::string& error, const std::string& in_reply_to);

  // Parses and executes a table check command.
  void HandleCheckTable(const Json::Value& message,
      const std::string& message_id);

  // Called to check a table on a background thread.
  void CheckTableInBackground(int32_t result, const std::string& table_names,
      const std::string& message_id);

  // Parses and executes a translation command.
  void HandleTranslate(const Json::Value& message,
      const std::string& message_id);

  // Called to translate text on a background thread.
  void TranslateInBackground(int32_t result, const TranslationParams& params,
      const std::string& message_id);

  // Parses and executes a back translation command.
  void HandleBackTranslate(const Json::Value& message,
      const std::string& message_id);

  // Called to back-translate text on a background thread.
  void BackTranslateInBackground(int32_t result,
      const std::string& table_names, const std::vector<unsigned char>& cells,
      const std::string& message_id);

  LibLouisWrapper liblouis_;
  pp::SimpleThread liblouis_thread_;
  pp::CompletionCallbackFactory<LibLouisInstance> cc_factory_;

  DISALLOW_COPY_AND_ASSIGN(LibLouisInstance);
};

}  // namespace liblouis_nacl

#endif  // LIBLOUIS_NACL_LIBLOUIS_INSTANCE_H_
