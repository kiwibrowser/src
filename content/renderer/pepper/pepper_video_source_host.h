// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_VIDEO_SOURCE_HOST_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_VIDEO_SOURCE_HOST_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"

struct PP_ImageDataDesc;

namespace media {
class VideoFrame;
}  // namespace media

namespace content {

class PPB_ImageData_Impl;
class RendererPpapiHost;
class VideoTrackToPepperAdapter;

class CONTENT_EXPORT PepperVideoSourceHost : public ppapi::host::ResourceHost {
 public:
  PepperVideoSourceHost(RendererPpapiHost* host,
                        PP_Instance instance,
                        PP_Resource resource);

  ~PepperVideoSourceHost() override;

  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  // Helper object to receive frames on a video worker thread and pass them on
  // to us.
  class FrameReceiver;

  int32_t OnHostMsgOpen(ppapi::host::HostMessageContext* context,
                        const std::string& stream_url);
  int32_t OnHostMsgGetFrame(ppapi::host::HostMessageContext* context);
  int32_t OnHostMsgClose(ppapi::host::HostMessageContext* context);

  // Sends the reply to a GetFrame message from the plugin. A reply is always
  // sent and last_frame_, reply_context_, and get_frame_pending_ are all reset.
  void SendGetFrameReply();
  // Sends the reply to a GetFrame message from the plugin in case of an error.
  void SendGetFrameErrorReply(int32_t error);

  void Close();

  ppapi::host::ReplyMessageContext reply_context_;

  std::unique_ptr<VideoTrackToPepperAdapter> frame_source_;
  scoped_refptr<FrameReceiver> frame_receiver_;
  std::string stream_url_;
  scoped_refptr<media::VideoFrame> last_frame_;
  // An internal frame buffer to avoid reallocations. It is only allocated if
  // scaling is needed.
  scoped_refptr<media::VideoFrame> scaled_frame_;
  bool get_frame_pending_;
  // We use only one ImageData resource in order to avoid allocating
  // shared memory repeatedly. We send the same one each time the plugin
  // requests a frame. For this to work, the plugin must finish using
  // the ImageData it receives prior to calling GetFrame, and not access
  // the ImageData until it gets its next callback to GetFrame.
  scoped_refptr<PPB_ImageData_Impl> shared_image_;
  PP_ImageDataDesc shared_image_desc_;

  base::WeakPtrFactory<PepperVideoSourceHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperVideoSourceHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_VIDEO_SOURCE_HOST_H_
