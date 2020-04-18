// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "ppapi/c/dev/ppb_video_capture_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/proxy/locking_resource_releaser.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/ppapi_proxy_test.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace proxy {

namespace {

typedef PluginProxyTest FlashResourceTest;

void* Unused(void* user_data, uint32_t element_count, uint32_t element_size) {
  return NULL;
}

}  // namespace

// Does a test of EnumerateVideoCaptureDevices() and reply functionality in
// the plugin side using the public C interfaces.
TEST_F(FlashResourceTest, EnumerateVideoCaptureDevices) {
  // TODO(raymes): This doesn't actually check that the data is converted from
  // |ppapi::DeviceRefData| to |PPB_DeviceRef| correctly, just that the right
  // messages are sent.

  // Set up a sync call handler that should return this message.
  std::vector<ppapi::DeviceRefData> reply_device_ref_data;
  int32_t expected_result = PP_OK;
  PpapiPluginMsg_DeviceEnumeration_EnumerateDevicesReply reply_msg(
      reply_device_ref_data);
  ResourceSyncCallHandler enumerate_video_devices_handler(
      &sink(),
      PpapiHostMsg_DeviceEnumeration_EnumerateDevices::ID,
      expected_result,
      reply_msg);
  sink().AddFilter(&enumerate_video_devices_handler);

  // Set up the arguments to the call.
  LockingResourceReleaser video_capture(
      ::ppapi::thunk::GetPPB_VideoCapture_Dev_0_3_Thunk()->Create(
          pp_instance()));
  std::vector<PP_Resource> unused;
  PP_ArrayOutput output;
  output.GetDataBuffer = &Unused;
  output.user_data = &unused;

  // Make the call.
  const PPB_Flash_12_6* flash_iface = ::ppapi::thunk::GetPPB_Flash_12_6_Thunk();
  int32_t actual_result = flash_iface->EnumerateVideoCaptureDevices(
      pp_instance(), video_capture.get(), output);

  // Check the result is as expected.
  EXPECT_EQ(expected_result, actual_result);

  // Should have sent an "DeviceEnumeration_EnumerateDevices" message.
  ASSERT_TRUE(enumerate_video_devices_handler.last_handled_msg().type() ==
      PpapiHostMsg_DeviceEnumeration_EnumerateDevices::ID);

  // Remove the filter or it will be destroyed before the sink() is destroyed.
  sink().RemoveFilter(&enumerate_video_devices_handler);
}

}  // namespace proxy
}  // namespace ppapi
