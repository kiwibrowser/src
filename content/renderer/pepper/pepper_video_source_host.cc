// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_video_source_host.h"

#include <string.h>

#include "base/bind.h"
#include "base/numerics/safe_conversions.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "content/renderer/media/pepper/video_track_to_pepper_adapter.h"
#include "content/renderer/pepper/ppb_image_data_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/video_util.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/ppb_image_data_proxy.h"
#include "ppapi/shared_impl/scoped_pp_resource.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_image_data_api.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/scale.h"
#include "third_party/skia/include/core/SkBitmap.h"

using ppapi::host::HostMessageContext;
using ppapi::host::ReplyMessageContext;

namespace content {

class PepperVideoSourceHost::FrameReceiver
    : public FrameReaderInterface,
      public base::RefCountedThreadSafe<FrameReceiver> {
 public:
  explicit FrameReceiver(const base::WeakPtr<PepperVideoSourceHost>& host);

  // FrameReaderInterface implementation.
  void GotFrame(const scoped_refptr<media::VideoFrame>& frame) override;

 private:
  friend class base::RefCountedThreadSafe<FrameReceiver>;
  ~FrameReceiver() override;

  base::WeakPtr<PepperVideoSourceHost> host_;
  // |thread_checker_| is bound to the main render thread.
  base::ThreadChecker thread_checker_;
};

PepperVideoSourceHost::FrameReceiver::FrameReceiver(
    const base::WeakPtr<PepperVideoSourceHost>& host)
    : host_(host) {}

PepperVideoSourceHost::FrameReceiver::~FrameReceiver() {}

void PepperVideoSourceHost::FrameReceiver::GotFrame(
    const scoped_refptr<media::VideoFrame>& video_frame) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!host_)
    return;

  if (!(video_frame->format() == media::PIXEL_FORMAT_I420 ||
        video_frame->format() == media::PIXEL_FORMAT_I420A)) {
    NOTREACHED();
    return;
  }
  scoped_refptr<media::VideoFrame> frame = video_frame;
  // Drop alpha channel since we do not support it yet.
  if (frame->format() == media::PIXEL_FORMAT_I420A)
    frame = media::WrapAsI420VideoFrame(video_frame);

  // Hold a reference to the new frame and release the previous.
  host_->last_frame_ = frame;
  if (host_->get_frame_pending_)
    host_->SendGetFrameReply();
}

PepperVideoSourceHost::PepperVideoSourceHost(RendererPpapiHost* host,
                                             PP_Instance instance,
                                             PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      frame_source_(new VideoTrackToPepperAdapter(nullptr)),
      get_frame_pending_(false),
      weak_factory_(this) {
  frame_receiver_ = new FrameReceiver(weak_factory_.GetWeakPtr());
  memset(&shared_image_desc_, 0, sizeof(shared_image_desc_));
}

PepperVideoSourceHost::~PepperVideoSourceHost() { Close(); }

int32_t PepperVideoSourceHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperVideoSourceHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_VideoSource_Open,
                                      OnHostMsgOpen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_VideoSource_GetFrame,
                                        OnHostMsgGetFrame)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_VideoSource_Close,
                                        OnHostMsgClose)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperVideoSourceHost::OnHostMsgOpen(HostMessageContext* context,
                                             const std::string& stream_url) {
  GURL gurl(stream_url);
  if (!gurl.is_valid())
    return PP_ERROR_BADARGUMENT;

  if (!frame_source_->Open(gurl.spec(), frame_receiver_.get()))
    return PP_ERROR_BADARGUMENT;

  stream_url_ = gurl.spec();

  ReplyMessageContext reply_context = context->MakeReplyMessageContext();
  reply_context.params.set_result(PP_OK);
  host()->SendReply(reply_context, PpapiPluginMsg_VideoSource_OpenReply());
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperVideoSourceHost::OnHostMsgGetFrame(HostMessageContext* context) {
  if (!frame_source_.get())
    return PP_ERROR_FAILED;
  if (get_frame_pending_)
    return PP_ERROR_INPROGRESS;

  reply_context_ = context->MakeReplyMessageContext();
  get_frame_pending_ = true;

  // If a frame is ready, try to convert it and send the reply.
  if (last_frame_.get())
    SendGetFrameReply();

  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperVideoSourceHost::OnHostMsgClose(HostMessageContext* context) {
  Close();
  return PP_OK;
}

void PepperVideoSourceHost::SendGetFrameReply() {
  DCHECK(get_frame_pending_);
  get_frame_pending_ = false;

  DCHECK(last_frame_.get());
  const gfx::Size dst_size = last_frame_->natural_size();

  // Note: We try to reuse the shared memory for the previous frame here. This
  // means that the previous frame may be overwritten and is no longer valid
  // after calling this function again.
  base::SharedMemoryHandle image_handle;
  uint32_t byte_count;
  if (shared_image_.get() && dst_size.width() == shared_image_->width() &&
      dst_size.height() == shared_image_->height()) {
    // We have already allocated the correct size in shared memory. We need to
    // duplicate the handle for IPC however, which will close down the
    // duplicated handle when it's done.
    base::SharedMemory* local_shm;
    if (shared_image_->GetSharedMemory(&local_shm, &byte_count) != PP_OK) {
      SendGetFrameErrorReply(PP_ERROR_FAILED);
      return;
    }

    ppapi::proxy::HostDispatcher* dispatcher =
        ppapi::proxy::HostDispatcher::GetForInstance(pp_instance());
    if (!dispatcher) {
      SendGetFrameErrorReply(PP_ERROR_FAILED);
      return;
    }

    image_handle =
        dispatcher->ShareSharedMemoryHandleWithRemote(local_shm->handle());
  } else {
    // We need to allocate new shared memory.
    shared_image_ = nullptr;  // Release any previous image.

    ppapi::ScopedPPResource resource(
        ppapi::ScopedPPResource::PassRef(),
        ppapi::proxy::PPB_ImageData_Proxy::CreateImageData(
            pp_instance(),
            ppapi::PPB_ImageData_Shared::SIMPLE,
            PP_IMAGEDATAFORMAT_BGRA_PREMUL,
            PP_MakeSize(dst_size.width(), dst_size.height()),
            false /* init_to_zero */,
            &shared_image_desc_,
            &image_handle,
            &byte_count));
    if (!resource) {
      SendGetFrameErrorReply(PP_ERROR_FAILED);
      return;
    }

    ppapi::thunk::EnterResourceNoLock<ppapi::thunk::PPB_ImageData_API>
        enter_resource(resource, false);
    if (enter_resource.failed()) {
      SendGetFrameErrorReply(PP_ERROR_FAILED);
      return;
    }

    shared_image_ = static_cast<PPB_ImageData_Impl*>(enter_resource.object());
    if (!shared_image_.get()) {
      SendGetFrameErrorReply(PP_ERROR_FAILED);
      return;
    }

    DCHECK(!shared_image_->IsMapped());  // New memory should not be mapped.
    if (!shared_image_->Map() || shared_image_->GetMappedBitmap().empty()) {
      shared_image_ = nullptr;
      SendGetFrameErrorReply(PP_ERROR_FAILED);
      return;
    }
  }

  SkBitmap bitmap(shared_image_->GetMappedBitmap());
  if (bitmap.empty()) {
    SendGetFrameErrorReply(PP_ERROR_FAILED);
    return;
  }

  uint8_t* bitmap_pixels = static_cast<uint8_t*>(bitmap.getPixels());
  if (!bitmap_pixels) {
    SendGetFrameErrorReply(PP_ERROR_FAILED);
    return;
  }

  // Calculate the portion of the |last_frame_| that should be copied into
  // |bitmap|. If |last_frame_| is lazily scaled, then
  // last_frame_->visible_rect()._size() != last_frame_.natural_size().
  scoped_refptr<media::VideoFrame> frame;
  if (dst_size == last_frame_->visible_rect().size()) {
    // No scaling is needed, convert directly from last_frame_.
    frame = last_frame_;
    // Frame resolution doesn't change frequently, so don't keep any unnecessary
    // buffers around.
    scaled_frame_ = nullptr;
  } else {
    // We need to create an intermediate scaled frame. Make sure we have
    // allocated one of correct size.
    if (!scaled_frame_.get() || scaled_frame_->coded_size() != dst_size) {
      scaled_frame_ = media::VideoFrame::CreateFrame(
          media::PIXEL_FORMAT_I420, dst_size, gfx::Rect(dst_size), dst_size,
          last_frame_->timestamp());
      if (!scaled_frame_.get()) {
        LOG(ERROR) << "Failed to allocate a media::VideoFrame";
        SendGetFrameErrorReply(PP_ERROR_FAILED);
        return;
      }
    }
    scaled_frame_->set_timestamp(last_frame_->timestamp());
    libyuv::I420Scale(last_frame_->visible_data(media::VideoFrame::kYPlane),
                      last_frame_->stride(media::VideoFrame::kYPlane),
                      last_frame_->visible_data(media::VideoFrame::kUPlane),
                      last_frame_->stride(media::VideoFrame::kUPlane),
                      last_frame_->visible_data(media::VideoFrame::kVPlane),
                      last_frame_->stride(media::VideoFrame::kVPlane),
                      last_frame_->visible_rect().width(),
                      last_frame_->visible_rect().height(),
                      scaled_frame_->data(media::VideoFrame::kYPlane),
                      scaled_frame_->stride(media::VideoFrame::kYPlane),
                      scaled_frame_->data(media::VideoFrame::kUPlane),
                      scaled_frame_->stride(media::VideoFrame::kUPlane),
                      scaled_frame_->data(media::VideoFrame::kVPlane),
                      scaled_frame_->stride(media::VideoFrame::kVPlane),
                      dst_size.width(),
                      dst_size.height(),
                      libyuv::kFilterBilinear);
    frame = scaled_frame_;
  }
  last_frame_ = nullptr;

  libyuv::I420ToARGB(frame->visible_data(media::VideoFrame::kYPlane),
                     frame->stride(media::VideoFrame::kYPlane),
                     frame->visible_data(media::VideoFrame::kUPlane),
                     frame->stride(media::VideoFrame::kUPlane),
                     frame->visible_data(media::VideoFrame::kVPlane),
                     frame->stride(media::VideoFrame::kVPlane),
                     bitmap_pixels,
                     bitmap.rowBytes(),
                     dst_size.width(),
                     dst_size.height());

  ppapi::HostResource host_resource;
  host_resource.SetHostResource(pp_instance(), shared_image_->GetReference());

  // Convert a video timestamp to a PP_TimeTicks (a double, in seconds).
  const PP_TimeTicks timestamp = frame->timestamp().InSecondsF();

  ppapi::proxy::SerializedHandle serialized_handle;
  serialized_handle.set_shmem(image_handle, byte_count);
  reply_context_.params.AppendHandle(serialized_handle);

  host()->SendReply(reply_context_,
                    PpapiPluginMsg_VideoSource_GetFrameReply(
                        host_resource, shared_image_desc_, timestamp));

  reply_context_ = ppapi::host::ReplyMessageContext();
}

void PepperVideoSourceHost::SendGetFrameErrorReply(int32_t error) {
  reply_context_.params.set_result(error);
  host()->SendReply(
      reply_context_,
      PpapiPluginMsg_VideoSource_GetFrameReply(
          ppapi::HostResource(), PP_ImageDataDesc(), 0.0 /* timestamp */));
  reply_context_ = ppapi::host::ReplyMessageContext();
}

void PepperVideoSourceHost::Close() {
  if (frame_source_.get() && !stream_url_.empty())
    frame_source_->Close(frame_receiver_.get());

  frame_source_.reset(nullptr);
  stream_url_.clear();

  shared_image_ = nullptr;
}

}  // namespace content
