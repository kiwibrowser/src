// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_MACROS_H_
#define MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_MACROS_H_

#include "ipc/ipc_message_macros.h"
#include "media/capture/video/video_capture_device_descriptor.h"
#include "media/capture/video_capture_types.h"

IPC_STRUCT_TRAITS_BEGIN(media::VideoCaptureDeviceDescriptor::CameraCalibration)
  IPC_STRUCT_TRAITS_MEMBER(focal_length_x)
  IPC_STRUCT_TRAITS_MEMBER(focal_length_y)
  IPC_STRUCT_TRAITS_MEMBER(depth_near)
  IPC_STRUCT_TRAITS_MEMBER(depth_far)
IPC_STRUCT_TRAITS_END()

#endif  // MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_MACROS_H_
