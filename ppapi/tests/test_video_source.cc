// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Tests PPB_VideoSource_Private interface.

#include "ppapi/tests/test_video_source.h"

#include <string.h>
#include <algorithm>
#include <limits>

#include "ppapi/c/private/ppb_testing_private.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/private/video_frame_private.h"
#include "ppapi/cpp/private/video_source_private.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(VideoSource);

namespace {

const PP_Resource kInvalidResource = 0;
const PP_Instance kInvalidInstance = 0;

}

TestVideoSource::TestVideoSource(TestingInstance* instance)
    : TestCase(instance),
      ppb_video_source_private_interface_(NULL),
      ppb_core_interface_(NULL),
      event_(instance_->pp_instance()) {
}

bool TestVideoSource::Init() {
  ppb_video_source_private_interface_ =
      static_cast<const PPB_VideoSource_Private*>(
          pp::Module::Get()->GetBrowserInterface(
              PPB_VIDEOSOURCE_PRIVATE_INTERFACE));
  if (!ppb_video_source_private_interface_)
    instance_->AppendError("PPB_VideoSource_Private interface not available");

  ppb_core_interface_ = static_cast<const PPB_Core*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CORE_INTERFACE));
  if (!ppb_core_interface_)
    instance_->AppendError("PPB_Core interface not available");

  return ppb_video_source_private_interface_ && ppb_core_interface_;
}

TestVideoSource::~TestVideoSource() {
}

void TestVideoSource::RunTests(const std::string& filter) {
  RUN_TEST(Create, filter);
  RUN_TEST(GetFrame, filter);
}

void TestVideoSource::HandleMessage(const pp::Var& message_data) {
  if (message_data.AsString().find("blob:") == 0) {
    stream_url_ = message_data.AsString();
    event_.Signal();
  }
}

std::string TestVideoSource::TestCreate() {
  PP_Resource video_source;
  // Creating a source from an invalid instance returns an invalid resource.
  video_source = ppb_video_source_private_interface_->Create(kInvalidInstance);
  ASSERT_EQ(kInvalidResource, video_source);
  ASSERT_FALSE(
      ppb_video_source_private_interface_->IsVideoSource(video_source));

  // Creating a source from a valid instance returns a valid resource.
  video_source =
      ppb_video_source_private_interface_->Create(instance_->pp_instance());
  ASSERT_NE(kInvalidResource, video_source);
  ASSERT_TRUE(
      ppb_video_source_private_interface_->IsVideoSource(video_source));

  ppb_core_interface_->ReleaseResource(video_source);
  // Once released, the resource shouldn't be a video source.
  ASSERT_FALSE(
      ppb_video_source_private_interface_->IsVideoSource(video_source));

  PASS();
}

std::string TestVideoSource::TestGetFrame() {
  std::string js_code;
  js_code += "var test_stream;"
             "function gotStream(stream){"
             "  test_stream = stream;"
             "  var url = URL.createObjectURL(test_stream);"
             "  var plugin = document.getElementById('plugin');"
             "  plugin.postMessage(url);"
             "}"
             "navigator.webkitGetUserMedia("
             "{audio:false, video:true}, gotStream, function() {});";
  instance_->EvalScript(js_code);
  event_.Wait();

  pp::VideoSource_Private video_source(instance_);
  TestCompletionCallback cc1(instance_->pp_instance(), false);
  cc1.WaitForResult(video_source.Open(stream_url_, cc1.GetCallback()));
  ASSERT_EQ(PP_OK, cc1.result());
  TestCompletionCallbackWithOutput<pp::VideoFrame_Private> cc2(
      instance_->pp_instance(), false);
  cc2.WaitForResult(video_source.GetFrame(cc2.GetCallback()));
  ASSERT_EQ(PP_OK, cc2.result());
  const pp::VideoFrame_Private video_frame = cc2.output();
  ASSERT_FALSE(video_frame.image_data().is_null());

  video_source.Close();

  PASS();
}

