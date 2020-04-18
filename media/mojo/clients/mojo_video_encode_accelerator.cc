// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_video_encode_accelerator.h"

#include "base/logging.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "media/base/video_frame.h"
#include "media/gpu/gpu_video_accelerator_util.h"
#include "media/mojo/common/mojo_shared_buffer_video_frame.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

namespace {

// Does nothing but keeping |frame| alive.
void KeepVideoFrameAlive(const scoped_refptr<VideoFrame>& frame) {}

// File-static mojom::VideoEncodeAcceleratorClient implementation to trampoline
// method calls to its |client_|. Note that this class is thread hostile when
// bound.
class VideoEncodeAcceleratorClient
    : public mojom::VideoEncodeAcceleratorClient {
 public:
  VideoEncodeAcceleratorClient(
      VideoEncodeAccelerator::Client* client,
      mojom::VideoEncodeAcceleratorClientRequest request);
  ~VideoEncodeAcceleratorClient() override = default;

  // mojom::VideoEncodeAcceleratorClient impl.
  void RequireBitstreamBuffers(uint32_t input_count,
                               const gfx::Size& input_coded_size,
                               uint32_t output_buffer_size) override;
  void BitstreamBufferReady(int32_t bitstream_buffer_id,
                            uint32_t payload_size,
                            bool key_frame,
                            base::TimeDelta timestamp) override;
  void NotifyError(VideoEncodeAccelerator::Error error) override;

 private:
  VideoEncodeAccelerator::Client* client_;
  mojo::Binding<mojom::VideoEncodeAcceleratorClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(VideoEncodeAcceleratorClient);
};

VideoEncodeAcceleratorClient::VideoEncodeAcceleratorClient(
    VideoEncodeAccelerator::Client* client,
    mojom::VideoEncodeAcceleratorClientRequest request)
    : client_(client), binding_(this, std::move(request)) {
  DCHECK(client_);
}

void VideoEncodeAcceleratorClient::RequireBitstreamBuffers(
    uint32_t input_count,
    const gfx::Size& input_coded_size,
    uint32_t output_buffer_size) {
  DVLOG(2) << __func__ << " input_count= " << input_count
           << " input_coded_size= " << input_coded_size.ToString()
           << " output_buffer_size=" << output_buffer_size;
  client_->RequireBitstreamBuffers(input_count, input_coded_size,
                                   output_buffer_size);
}

void VideoEncodeAcceleratorClient::BitstreamBufferReady(
    int32_t bitstream_buffer_id,
    uint32_t payload_size,
    bool key_frame,
    base::TimeDelta timestamp) {
  DVLOG(2) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id
           << ", payload_size=" << payload_size
           << "B,  key_frame=" << key_frame;
  client_->BitstreamBufferReady(bitstream_buffer_id, payload_size, key_frame,
                                timestamp);
}

void VideoEncodeAcceleratorClient::NotifyError(
    VideoEncodeAccelerator::Error error) {
  DVLOG(2) << __func__;
  client_->NotifyError(error);
}

}  // anonymous namespace

MojoVideoEncodeAccelerator::MojoVideoEncodeAccelerator(
    mojom::VideoEncodeAcceleratorPtr vea,
    const gpu::VideoEncodeAcceleratorSupportedProfiles& supported_profiles)
    : vea_(std::move(vea)), supported_profiles_(supported_profiles) {
  DVLOG(1) << __func__;
  DCHECK(vea_);
}

VideoEncodeAccelerator::SupportedProfiles
MojoVideoEncodeAccelerator::GetSupportedProfiles() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GpuVideoAcceleratorUtil::ConvertGpuToMediaEncodeProfiles(
      supported_profiles_);
}

bool MojoVideoEncodeAccelerator::Initialize(VideoPixelFormat input_format,
                                            const gfx::Size& input_visible_size,
                                            VideoCodecProfile output_profile,
                                            uint32_t initial_bitrate,
                                            Client* client) {
  DVLOG(2) << __func__
           << " input_format=" << VideoPixelFormatToString(input_format)
           << ", input_visible_size=" << input_visible_size.ToString()
           << ", output_profile=" << GetProfileName(output_profile)
           << ", initial_bitrate=" << initial_bitrate;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!client)
    return false;

  // Get a mojom::VideoEncodeAcceleratorClient bound to a local implementation
  // (VideoEncodeAcceleratorClient) and send the pointer remotely.
  mojom::VideoEncodeAcceleratorClientPtr vea_client_ptr;
  vea_client_ = std::make_unique<VideoEncodeAcceleratorClient>(
      client, mojo::MakeRequest(&vea_client_ptr));

  bool result = false;
  vea_->Initialize(input_format, input_visible_size, output_profile,
                   initial_bitrate, std::move(vea_client_ptr), &result);
  return result;
}

void MojoVideoEncodeAccelerator::Encode(const scoped_refptr<VideoFrame>& frame,
                                        bool force_keyframe) {
  DVLOG(2) << __func__ << " tstamp=" << frame->timestamp();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(PIXEL_FORMAT_I420, frame->format());
  DCHECK_EQ(VideoFrame::STORAGE_SHMEM, frame->storage_type());
  DCHECK(frame->shared_memory_handle().IsValid());

  // Oftentimes |frame|'s underlying planes will be aligned and not tightly
  // packed, so don't use VideoFrame::AllocationSize().
  const size_t allocation_size = frame->shared_memory_handle().GetSize();

  // WrapSharedMemoryHandle() takes ownership of the handle passed to it, but we
  // don't have ownership of frame->shared_memory_handle(), so Duplicate() it.
  //
  // TODO(https://crbug.com/793446): This should be changed to wrap the frame
  // buffer handle as read-only, but VideoFrame does not seem to guarantee that
  // its shared_memory_handle() is in fact read-only.
  mojo::ScopedSharedBufferHandle handle = mojo::WrapSharedMemoryHandle(
      frame->shared_memory_handle().Duplicate(), allocation_size,
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);

  const size_t y_offset = frame->shared_memory_offset();
  const size_t u_offset = y_offset + frame->data(VideoFrame::kUPlane) -
                          frame->data(VideoFrame::kYPlane);
  const size_t v_offset = y_offset + frame->data(VideoFrame::kVPlane) -
                          frame->data(VideoFrame::kYPlane);
  // Temporary Mojo VideoFrame to allow for marshalling.
  scoped_refptr<MojoSharedBufferVideoFrame> mojo_frame =
      MojoSharedBufferVideoFrame::Create(
          frame->format(), frame->coded_size(), frame->visible_rect(),
          frame->natural_size(), std::move(handle), allocation_size, y_offset,
          u_offset, v_offset, frame->stride(VideoFrame::kYPlane),
          frame->stride(VideoFrame::kUPlane),
          frame->stride(VideoFrame::kVPlane), frame->timestamp());

  // Encode() is synchronous: clients will assume full ownership of |frame| when
  // this gets destroyed and probably recycle its shared_memory_handle(): keep
  // the former alive until the remote end is actually finished.
  DCHECK(vea_.is_bound());
  vea_->Encode(mojo_frame, force_keyframe,
               base::Bind(&KeepVideoFrameAlive, frame));
}

void MojoVideoEncodeAccelerator::UseOutputBitstreamBuffer(
    const BitstreamBuffer& buffer) {
  DVLOG(2) << __func__ << " buffer.id()= " << buffer.id()
           << " buffer.size()= " << buffer.size() << "B";
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(buffer.handle().IsValid());

  // TODO(https://crbug.com/793446): Only wrap read-only handles here and change
  // the protection status to kReadOnly.
  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      buffer.handle().Duplicate(), buffer.size(),
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);

  vea_->UseOutputBitstreamBuffer(buffer.id(), std::move(buffer_handle));
}

void MojoVideoEncodeAccelerator::RequestEncodingParametersChange(
    uint32_t bitrate,
    uint32_t framerate) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(vea_.is_bound());
  vea_->RequestEncodingParametersChange(bitrate, framerate);
}

void MojoVideoEncodeAccelerator::Destroy() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  vea_client_.reset();
  vea_.reset();
  // See media::VideoEncodeAccelerator for more info on this peculiar pattern.
  delete this;
}

MojoVideoEncodeAccelerator::~MojoVideoEncodeAccelerator() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

}  // namespace media
