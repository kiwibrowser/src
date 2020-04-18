// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Tests PPB_VideoDestination_Private interface.

#include "ppapi/tests/test_video_destination.h"

#include <algorithm>
#include <limits>
#include <string>

#include "ppapi/c/private/ppb_testing_private.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/private/video_destination_private.h"
#include "ppapi/cpp/private/video_frame_private.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(VideoDestination);

namespace {

const PP_Resource kInvalidResource = 0;
const PP_Instance kInvalidInstance = 0;

}

TestVideoDestination::TestVideoDestination(TestingInstance* instance)
    : TestCase(instance),
      ppb_video_destination_private_interface_(NULL),
      ppb_core_interface_(NULL),
      event_(instance_->pp_instance()) {
}

bool TestVideoDestination::Init() {
  ppb_video_destination_private_interface_ =
      static_cast<const PPB_VideoDestination_Private*>(
          pp::Module::Get()->GetBrowserInterface(
              PPB_VIDEODESTINATION_PRIVATE_INTERFACE));
  if (!ppb_video_destination_private_interface_)
    instance_->AppendError(
        "PPB_VideoDestination_Private interface not available");

  ppb_core_interface_ = static_cast<const PPB_Core*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CORE_INTERFACE));
  if (!ppb_core_interface_)
    instance_->AppendError("PPB_Core interface not available");

  return
      ppb_video_destination_private_interface_ &&
      ppb_core_interface_;
}

TestVideoDestination::~TestVideoDestination() {
}

void TestVideoDestination::RunTests(const std::string& filter) {
  RUN_TEST(Create, filter);
  RUN_TEST(PutFrame, filter);
}

void TestVideoDestination::HandleMessage(const pp::Var& message_data) {
  if (message_data.AsString().find("blob:") == 0) {
    stream_url_ = message_data.AsString();
    event_.Signal();
  }
}

std::string TestVideoDestination::TestCreate() {
  PP_Resource video_destination;
  // Creating a destination from an invalid instance returns an invalid
  // resource.
  video_destination =
      ppb_video_destination_private_interface_->Create(kInvalidInstance);
  ASSERT_EQ(kInvalidResource, video_destination);
  ASSERT_FALSE(
      ppb_video_destination_private_interface_->IsVideoDestination(
          video_destination));

  // Creating a destination from a valid instance returns a valid resource.
  video_destination =
      ppb_video_destination_private_interface_->Create(
          instance_->pp_instance());
  ASSERT_NE(kInvalidResource, video_destination);
  ASSERT_TRUE(
      ppb_video_destination_private_interface_->IsVideoDestination(
          video_destination));

  ppb_core_interface_->ReleaseResource(video_destination);
  // Once released, the resource shouldn't be a video destination.
  ASSERT_FALSE(
      ppb_video_destination_private_interface_->IsVideoDestination(
          video_destination));

  PASS();
}

std::string TestVideoDestination::TestPutFrame() {
  std::string js_code;
  js_code += "var test_stream = new MediaStream([]);"
             "var url = URL.createObjectURL(test_stream);"
             "var plugin = document.getElementById('plugin');"
             "plugin.postMessage(url);";
  instance_->EvalScript(js_code);
  event_.Wait();

  pp::VideoDestination_Private video_destination(instance_);
  TestCompletionCallback cc1(instance_->pp_instance(), false);
  cc1.WaitForResult(video_destination.Open(stream_url_, cc1.GetCallback()));
  ASSERT_EQ(PP_OK, cc1.result());

  pp::ImageData image_data(instance_,
                           PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                           pp::Size(640, 480),
                           false /* init_to_zero */);
  pp::VideoFrame_Private video_frame(image_data,
                                     0.0 /* timestamp */);
  ASSERT_EQ(PP_OK, video_destination.PutFrame(video_frame));

  video_destination.Close();

  PASS();
}
