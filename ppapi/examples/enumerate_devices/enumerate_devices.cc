// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <vector>

#include "ppapi/c/dev/ppb_video_capture_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/dev/device_ref_dev.h"
#include "ppapi/cpp/dev/video_capture_client_dev.h"
#include "ppapi/cpp/dev/video_capture_dev.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/private/flash.h"
#include "ppapi/cpp/var.h"
#include "ppapi/utility/completion_callback_factory.h"

// When compiling natively on Windows, PostMessage can be #define-d to
// something else.
#ifdef PostMessage
#undef PostMessage
#endif

namespace {

// This object is the global object representing this plugin library as long
// as it is loaded.
class EnumerateDevicesDemoModule : public pp::Module {
 public:
  EnumerateDevicesDemoModule() : pp::Module() {}
  virtual ~EnumerateDevicesDemoModule() {}
  virtual pp::Instance* CreateInstance(PP_Instance instance);
};

class EnumerateDevicesDemoInstance : public pp::Instance,
                                     public pp::VideoCaptureClient_Dev {
 public:
  EnumerateDevicesDemoInstance(PP_Instance instance, pp::Module* module);
  virtual ~EnumerateDevicesDemoInstance();

  // pp::Instance implementation (see PPP_Instance).
  virtual void HandleMessage(const pp::Var& message_data);

  // pp::VideoCaptureClient_Dev implementation.
  virtual void OnDeviceInfo(PP_Resource resource,
                            const PP_VideoCaptureDeviceInfo_Dev& info,
                            const std::vector<pp::Buffer_Dev>& buffers) {}
  virtual void OnStatus(PP_Resource resource, uint32_t status) {}
  virtual void OnError(PP_Resource resource, uint32_t error) {}
  virtual void OnBufferReady(PP_Resource resource, uint32_t buffer) {}

 private:
  void EnumerateDevicesFinished(int32_t result,
                                std::vector<pp::DeviceRef_Dev>& devices);

  pp::VideoCapture_Dev video_capture_;
  pp::CompletionCallbackFactory<EnumerateDevicesDemoInstance> callback_factory_;

  std::vector<pp::DeviceRef_Dev> devices_;
};

EnumerateDevicesDemoInstance::EnumerateDevicesDemoInstance(PP_Instance instance,
                                                           pp::Module* module)
    : pp::Instance(instance),
      pp::VideoCaptureClient_Dev(this),
      video_capture_(this),
      callback_factory_(this) {
}

EnumerateDevicesDemoInstance::~EnumerateDevicesDemoInstance() {
}

void EnumerateDevicesDemoInstance::HandleMessage(const pp::Var& message_data) {
  if (message_data.is_string()) {
    std::string event = message_data.AsString();
    if (event == "EnumerateDevicesAsync") {
      pp::CompletionCallbackWithOutput<std::vector<pp::DeviceRef_Dev> >
          callback = callback_factory_.NewCallbackWithOutput(
              &EnumerateDevicesDemoInstance::EnumerateDevicesFinished);
      video_capture_.EnumerateDevices(callback);
    } else if (event == "EnumerateDevicesSync") {
      std::vector<pp::DeviceRef_Dev> devices;
      int32_t result = pp::flash::Flash::EnumerateVideoCaptureDevices(
          this, video_capture_, &devices);
      EnumerateDevicesFinished(result, devices);
    }
  }
}

void EnumerateDevicesDemoInstance::EnumerateDevicesFinished(
    int32_t result,
    std::vector<pp::DeviceRef_Dev>& devices) {
  static const char* const kDelimiter = "#__#";

  if (result == PP_OK) {
    devices_.swap(devices);
    std::string device_names;
    for (size_t index = 0; index < devices_.size(); ++index) {
      pp::Var name = devices_[index].GetName();
      assert(name.is_string());

      if (index != 0)
        device_names += kDelimiter;
      device_names += name.AsString();
    }
    PostMessage(pp::Var("EnumerationSuccess" + device_names));
  } else {
    PostMessage(pp::Var("EnumerationFailed"));
  }
}

pp::Instance* EnumerateDevicesDemoModule::CreateInstance(PP_Instance instance) {
  return new EnumerateDevicesDemoInstance(instance, this);
}

}  // anonymous namespace

namespace pp {
// Factory function for your specialization of the Module object.
Module* CreateModule() {
  return new EnumerateDevicesDemoModule();
}
}  // namespace pp
