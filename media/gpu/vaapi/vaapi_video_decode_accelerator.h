// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains an implementation of VideoDecoderAccelerator
// that utilizes hardware video decoder present on Intel CPUs.

#ifndef MEDIA_GPU_VAAPI_VAAPI_VIDEO_DECODE_ACCELERATOR_H_
#define MEDIA_GPU_VAAPI_VAAPI_VIDEO_DECODE_ACCELERATOR_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/containers/queue.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "media/base/bitstream_buffer.h"
#include "media/gpu/gpu_video_decode_accelerator_helpers.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/shared_memory_region.h"
#include "media/gpu/vaapi/vaapi_picture_factory.h"
#include "media/gpu/vaapi/vaapi_wrapper.h"
#include "media/video/picture.h"
#include "media/video/video_decode_accelerator.h"

namespace gl {
class GLImage;
}

namespace media {

class AcceleratedVideoDecoder;
class VaapiPicture;

// Class to provide video decode acceleration for Intel systems with hardware
// support for it, and on which libva is available.
// Decoding tasks are performed in a separate decoding thread.
//
// Threading/life-cycle: this object is created & destroyed on the GPU
// ChildThread.  A few methods on it are called on the decoder thread which is
// stopped during |this->Destroy()|, so any tasks posted to the decoder thread
// can assume |*this| is still alive.  See |weak_this_| below for more details.
class MEDIA_GPU_EXPORT VaapiVideoDecodeAccelerator
    : public VideoDecodeAccelerator {
 public:
  VaapiVideoDecodeAccelerator(
      const MakeGLContextCurrentCallback& make_context_current_cb,
      const BindGLImageCallback& bind_image_cb);

  ~VaapiVideoDecodeAccelerator() override;

  // VideoDecodeAccelerator implementation.
  bool Initialize(const Config& config, Client* client) override;
  void Decode(const BitstreamBuffer& bitstream_buffer) override;
  void AssignPictureBuffers(const std::vector<PictureBuffer>& buffers) override;
#if defined(USE_OZONE)
  void ImportBufferForPicture(
      int32_t picture_buffer_id,
      VideoPixelFormat pixel_format,
      const gfx::GpuMemoryBufferHandle& gpu_memory_buffer_handle) override;
#endif
  void ReusePictureBuffer(int32_t picture_buffer_id) override;
  void Flush() override;
  void Reset() override;
  void Destroy() override;
  bool TryToSetupDecodeOnSeparateThread(
      const base::WeakPtr<Client>& decode_client,
      const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner)
      override;

  static VideoDecodeAccelerator::SupportedProfiles GetSupportedProfiles();

  //
  // Below methods are used by accelerator implementations.
  //
  // Decode of |va_surface| is ready to be submitted and all codec-specific
  // settings are set in hardware.
  bool DecodeVASurface(const scoped_refptr<VASurface>& va_surface);

  // The |visible_rect| area of |va_surface| associated with |bitstream_id| is
  // ready to be outputted once decode is finished. This can be called before
  // decode is actually done in hardware, and this method is responsible for
  // maintaining the ordering, i.e. the surfaces have to be outputted in the
  // same order as VASurfaceReady is called.  On Intel, we don't have to
  // explicitly maintain the ordering however, as the driver will maintain
  // ordering, as well as dependencies, and will process each submitted command
  // in order, and run each command only if its dependencies are ready.
  void VASurfaceReady(const scoped_refptr<VASurface>& va_surface,
                      int32_t bitstream_id,
                      const gfx::Rect& visible_rect);

  // Return a new VASurface for decoding into, or nullptr if not available.
  scoped_refptr<VASurface> CreateVASurface();

 private:
  friend class VaapiVideoDecodeAcceleratorTest;

  // An input buffer with id provided by the client and awaiting consumption.
  class InputBuffer;

  // Notify the client that an error has occurred and decoding cannot continue.
  void NotifyError(Error error);

  // Queue a input buffer for decode.
  void QueueInputBuffer(const BitstreamBuffer& bitstream_buffer);

  // Gets a new |current_input_buffer_| from |input_buffers_| and sets it up in
  // |decoder_|. This method will sleep if no |input_buffers_| are available.
  // Returns true if a new buffer has been set up, false if an early exit has
  // been requested (due to initiated reset/flush/destroy).
  bool GetCurrInputBuffer_Locked();

  // Signals the client that |curr_input_buffer_| has been read and can be
  // returned. Will also release the mapping.
  void ReturnCurrInputBuffer_Locked();

  // Waits for more surfaces to become available. Returns true once they do or
  // false if an early exit has been requested (due to an initiated
  // reset/flush/destroy).
  bool WaitForSurfaces_Locked();

  // Continue decoding given input buffers and sleep waiting for input/output
  // as needed. Will exit if a new set of surfaces or reset/flush/destroy
  // is requested.
  void DecodeTask();

  // Scheduled after receiving a flush request and executed after the current
  // decoding task finishes decoding pending inputs. Makes the decoder return
  // all remaining output pictures and puts it in an idle state, ready
  // to resume if needed and schedules a FinishFlush.
  void FlushTask();

  // Scheduled by the FlushTask after decoder is flushed to put VAVDA into idle
  // state and notify the client that flushing has been finished.
  void FinishFlush();

  // Scheduled after receiving a reset request and executed after the current
  // decoding task finishes decoding the current frame. Puts the decoder into
  // an idle state, ready to resume if needed, discarding decoded but not yet
  // outputted pictures (decoder keeps ownership of their associated picture
  // buffers). Schedules a FinishReset afterwards.
  void ResetTask();

  // Scheduled by ResetTask after it's done putting VAVDA into an idle state.
  // Drops remaining input buffers and notifies the client that reset has been
  // finished.
  void FinishReset();

  // Helper for Destroy(), doing all the actual work except for deleting self.
  void Cleanup();

  // Get a usable framebuffer configuration for use in binding textures
  // or return false on failure.
  bool InitializeFBConfig();

  // Callback to be executed once we have a |va_surface| to be output and
  // an available |picture| to use for output.
  // Puts contents of |va_surface| into given |picture|, releases the surface
  // and passes the resulting picture to client to output the given
  // |visible_rect| part of it.
  void OutputPicture(const scoped_refptr<VASurface>& va_surface,
                     int32_t input_id,
                     gfx::Rect visible_rect,
                     VaapiPicture* picture);

  // Try to OutputPicture() if we have both a ready surface and picture.
  void TryOutputSurface();

  // Called when a VASurface is no longer in use by the decoder or is not being
  // synced/waiting to be synced to a picture. Returns it to available surfaces
  // pool.
  void RecycleVASurfaceID(VASurfaceID va_surface_id);

  // Initiate wait cycle for surfaces to be released before we release them
  // and allocate new ones, as requested by the decoder.
  void InitiateSurfaceSetChange(size_t num_pics, gfx::Size size);

  // Check if the surfaces have been released or post ourselves for later.
  void TryFinishSurfaceSetChange();

  // VAVDA state.
  enum State {
    // Initialize() not called yet or failed.
    kUninitialized,
    // DecodeTask running.
    kDecoding,
    // Resetting, waiting for decoder to finish current task and cleanup.
    kResetting,
    // Idle, decoder in state ready to start/resume decoding.
    kIdle,
    // Destroying, waiting for the decoder to finish current task.
    kDestroying,
  };

  // Protects input buffer and surface queues and state_.
  base::Lock lock_;
  State state_;
  Config::OutputMode output_mode_;

  // Queue of available InputBuffers (picture_buffer_ids).
  base::queue<std::unique_ptr<InputBuffer>> input_buffers_;
  // Signalled when input buffers are queued onto |input_buffers_| queue.
  base::ConditionVariable input_ready_;

  // Current input buffer at decoder.
  std::unique_ptr<InputBuffer> curr_input_buffer_;

  // Queue for incoming output buffers (texture ids).
  using OutputBuffers = base::queue<int32_t>;
  OutputBuffers output_buffers_;

  std::unique_ptr<VaapiPictureFactory> vaapi_picture_factory_;

  // Constructed in Initialize() when the codec information is received.
  scoped_refptr<VaapiWrapper> vaapi_wrapper_;
  std::unique_ptr<AcceleratedVideoDecoder> decoder_;

  // All allocated Pictures, regardless of their current state. Pictures are
  // allocated once using |create_vaapi_picture_callback_| and destroyed at the
  // end of decode. Comes after |vaapi_wrapper_| to ensure all pictures are
  // destroyed before said |vaapi_wrapper_| is destroyed.
  using Pictures = std::map<int32_t, std::unique_ptr<VaapiPicture>>;
  Pictures pictures_;

  // Return a VaapiPicture associated with given client-provided id.
  VaapiPicture* PictureById(int32_t picture_buffer_id);

  // VA Surfaces no longer in use that can be passed back to the decoder for
  // reuse, once it requests them.
  std::list<VASurfaceID> available_va_surfaces_;
  // Signalled when output surfaces are queued onto the available_va_surfaces_
  // queue.
  base::ConditionVariable surfaces_available_;

  // Pending output requests from the decoder. When it indicates that we should
  // output a surface and we have an available Picture (i.e. texture) ready
  // to use, we'll execute the callback passing the Picture. The callback
  // will put the contents of the surface into the picture and return it to
  // the client, releasing the surface as well.
  // If we don't have any available Pictures at the time when the decoder
  // requests output, we'll store the request on pending_output_cbs_ queue for
  // later and run it once the client gives us more textures
  // via ReusePictureBuffer().
  using OutputCB = base::Callback<void(VaapiPicture*)>;
  base::queue<OutputCB> pending_output_cbs_;

  // ChildThread's task runner.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // WeakPtr<> pointing to |this| for use in posting tasks from the decoder
  // thread back to the ChildThread.  Because the decoder thread is a member of
  // this class, any task running on the decoder thread is guaranteed that this
  // object is still alive.  As a result, tasks posted from ChildThread to
  // decoder thread should use base::Unretained(this), and tasks posted from the
  // decoder thread to the ChildThread should use |weak_this_|.
  base::WeakPtr<VaapiVideoDecodeAccelerator> weak_this_;

  // Callback used when creating VASurface objects.
  VASurface::ReleaseCB va_surface_release_cb_;

  // To expose client callbacks from VideoDecodeAccelerator.
  // NOTE: all calls to these objects *MUST* be executed on task_runner_.
  std::unique_ptr<base::WeakPtrFactory<Client>> client_ptr_factory_;
  base::WeakPtr<Client> client_;

  base::Thread decoder_thread_;
  // Use this to post tasks to |decoder_thread_| instead of
  // |decoder_thread_.message_loop()| because the latter will be NULL once
  // |decoder_thread_.Stop()| returns.
  scoped_refptr<base::SingleThreadTaskRunner> decoder_thread_task_runner_;

  int num_frames_at_client_;

  // Whether we are waiting for any pending_output_cbs_ to be run before
  // NotifyingFlushDone.
  bool finish_flush_pending_;

  // Decoder requested a new surface set and we are waiting for all the surfaces
  // to be returned before we can free them.
  bool awaiting_va_surfaces_recycle_;

  // Last requested number/resolution of output picture buffers and their
  // format.
  size_t requested_num_pics_;
  gfx::Size requested_pic_size_;
  VideoCodecProfile profile_;

  // Callback to make GL context current.
  MakeGLContextCurrentCallback make_context_current_cb_;

  // Callback to bind a GLImage to a given texture.
  BindGLImageCallback bind_image_cb_;

  // The WeakPtrFactory for |weak_this_|.
  base::WeakPtrFactory<VaapiVideoDecodeAccelerator> weak_this_factory_;

  DISALLOW_COPY_AND_ASSIGN(VaapiVideoDecodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_VAAPI_VIDEO_DECODE_ACCELERATOR_H_
