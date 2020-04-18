// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/message_loop.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/private/video_destination_private.h"
#include "ppapi/cpp/private/video_frame_private.h"
#include "ppapi/cpp/private/video_source_private.h"
#include "ppapi/cpp/var.h"
#include "ppapi/utility/completion_callback_factory.h"

// When compiling natively on Windows, PostMessage can be #define-d to
// something else.
#ifdef PostMessage
#undef PostMessage
#endif

namespace {

// Helper functions
std::vector<std::string> SplitStringBySpace(const std::string& str) {
  std::istringstream buf(str);
  std::istream_iterator<std::string> begin(buf), end;
  std::vector<std::string> tokens(begin, end);
  return tokens;
}

// This object is the global object representing this plugin library as long
// as it is loaded.
class VEDemoModule : public pp::Module {
 public:
  VEDemoModule() : pp::Module() {}
  virtual ~VEDemoModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance);
};

class VEDemoInstance : public pp::Instance {
 public:
  VEDemoInstance(PP_Instance instance, pp::Module* module);
  virtual ~VEDemoInstance();

  // pp::Instance implementation (see PPP_Instance).
  virtual void HandleMessage(const pp::Var& message_data);

 private:
  void DestinationOpenDone(int32_t result, const std::string& src_url);
  void SourceOpenDone(int32_t result);
  void GetFrameDone(int32_t result, pp::VideoFrame_Private video_frame);
  void KickoffEffect(int32_t result);
  pp::VideoSource_Private video_source_;
  pp::VideoDestination_Private video_destination_;
  bool effect_on_;
  pp::CompletionCallbackFactory<VEDemoInstance> factory_;
  pp::MessageLoop message_loop_;
};

VEDemoInstance::VEDemoInstance(PP_Instance instance, pp::Module* module)
    : pp::Instance(instance),
      video_source_(this),
      video_destination_(this),
      effect_on_(false),
      message_loop_(pp::MessageLoop::GetCurrent()) {
  factory_.Initialize(this);
}

VEDemoInstance::~VEDemoInstance() {
  video_source_.Close();
  video_destination_.Close();
}

void VEDemoInstance::HandleMessage(const pp::Var& message_data) {
  if (message_data.is_string()) {
    std::vector<std::string> messages;
    messages = SplitStringBySpace(message_data.AsString());
    if (messages.empty()) {
      PostMessage(pp::Var("Ignored empty message."));
      return;
    }
    if (messages[0] == "registerStream") {
      if (messages.size() < 3) {
        PostMessage(pp::Var("Got 'registerStream' with incorrect parameters."));
        return;
      }
      // Open destination stream for write.
      video_destination_.Open(
          messages[2],
          factory_.NewCallback(&VEDemoInstance::DestinationOpenDone,
                               messages[1]));
    } else if (messages[0] == "effectOn") {
      effect_on_ = true;
      PostMessage(pp::Var("Effect ON."));
    } else if (messages[0] == "effectOff") {
      effect_on_ = false;
      PostMessage(pp::Var("Effect OFF."));
    }
  }
}

void VEDemoInstance::DestinationOpenDone(int32_t result,
                                         const std::string& src_url) {
  if (result != PP_OK) {
    PostMessage(pp::Var("Failed to open destination stream."));
    return;
  }
  // Open source stream for read.
  video_source_.Open(src_url,
                     factory_.NewCallback(&VEDemoInstance::SourceOpenDone));
}

void VEDemoInstance::SourceOpenDone(int32_t result) {
  if (result != PP_OK) {
    PostMessage(pp::Var("Failed to open source stream."));
    return;
  }
  // Done with the stream register.
  PostMessage(pp::Var("DoneRegistering"));

  // Kick off the processing loop.
  message_loop_.PostWork(factory_.NewCallback(&VEDemoInstance::KickoffEffect));
}

void VEDemoInstance::GetFrameDone(int32_t result,
                                  pp::VideoFrame_Private video_frame) {
  if (result != PP_OK) {
    PostMessage(pp::Var("Failed to get frame."));
    return;
  }

  // Apply the effect to the received frame.
  if (effect_on_) {
    pp::ImageData image_data = video_frame.image_data();
    pp::Size size = image_data.size();
    std::vector<uint8_t> tmp_row(image_data.stride());
    uint8_t* image = static_cast<uint8_t*>(image_data.data());
    for (int i = 0; i < size.height() / 2; ++i) {
      uint8_t* top = image + i * image_data.stride();
      uint8_t* bottom = image + (size.height() - 1 - i) * image_data.stride();
      memcpy(&tmp_row[0], top, image_data.stride());
      memcpy(top, bottom, image_data.stride());
      memcpy(bottom, &tmp_row[0], image_data.stride());
    }
  }

  // Put frame back to destination stream
  video_destination_.PutFrame(video_frame);

  // Trigger for the next frame.
  message_loop_.PostWork(factory_.NewCallback(&VEDemoInstance::KickoffEffect));
}

void VEDemoInstance::KickoffEffect(int32_t /* result */) {
  // Get the frame from the source stream.
  video_source_.GetFrame(
      factory_.NewCallbackWithOutput<pp::VideoFrame_Private>(
          &VEDemoInstance::GetFrameDone));
}

pp::Instance* VEDemoModule::CreateInstance(PP_Instance instance) {
  return new VEDemoInstance(instance, this);
}

}  // anonymous namespace

namespace pp {
// Factory function for your specialization of the Module object.
Module* CreateModule() {
  return new VEDemoModule();
}
}  // namespace pp
