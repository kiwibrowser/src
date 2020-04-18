// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_V4L2_V4L2_IMAGE_PROCESSOR_H_
#define MEDIA_GPU_V4L2_V4L2_IMAGE_PROCESSOR_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "media/base/video_frame.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/v4l2/v4l2_device.h"

namespace media {

// Handles image processing accelerators that expose a V4L2 memory-to-memory
// interface. The threading model of this class is the same as for other V4L2
// hardware accelerators (see V4L2VideoDecodeAccelerator) for more details.
class MEDIA_GPU_EXPORT V4L2ImageProcessor {
 public:
  explicit V4L2ImageProcessor(const scoped_refptr<V4L2Device>& device);
  virtual ~V4L2ImageProcessor();

  // Initializes the processor to convert from |input_format| to |output_format|
  // and/or scale from |input_visible_size| to |output_visible_size|.
  // Request the input buffers to be of at least |input_allocated_size| and the
  // output buffers to be of at least |output_allocated_size|. The number of
  // input buffers and output buffers will be |num_buffers|. Provided |error_cb|
  // will be called if an error occurs. Return true if the requested
  // configuration is supported.
  bool Initialize(VideoPixelFormat input_format,
                  VideoPixelFormat output_format,
                  v4l2_memory input_memory_type,
                  v4l2_memory output_memory_type,
                  gfx::Size input_visible_size,
                  gfx::Size input_allocated_size,
                  gfx::Size output_visible_size,
                  gfx::Size output_allocated_size,
                  int num_buffers,
                  const base::Closure& error_cb);

  // Returns a vector of dmabuf file descriptors, exported for V4L2 output
  // buffer with |index|. The size of vector will be the number of planes of the
  // buffer. Return an empty vector on failure.
  std::vector<base::ScopedFD> GetDmabufsForOutputBuffer(
      int output_buffer_index);

  // Returns true if image processing is supported on this platform.
  static bool IsSupported();

  // Returns a vector of supported input formats in fourcc.
  static std::vector<uint32_t> GetSupportedInputFormats();

  // Returns a vector of supported output formats in fourcc.
  static std::vector<uint32_t> GetSupportedOutputFormats();

  // Gets output allocated size and number of planes required by the device
  // for conversion from |input_pixelformat| to |output_pixelformat|, for
  // visible size |size|. Returns true on success. Adjusted coded size will be
  // stored in |size| and the number of planes will be stored in |num_planes|.
  static bool TryOutputFormat(uint32_t input_pixelformat,
                              uint32_t output_pixelformat,
                              gfx::Size* size,
                              size_t* num_planes);

  // Returns input allocated size required by the processor to be fed with.
  gfx::Size input_allocated_size() const { return input_allocated_size_; }

  // Returns output allocated size required by the processor.
  gfx::Size output_allocated_size() const { return output_allocated_size_; }

  // Callback to be used to return the index of a processed image to the
  // client. After the client is done with the frame, call Process with the
  // index to return the output buffer to the image processor.
  typedef base::Callback<void(int output_buffer_index)> FrameReadyCB;

  // Called by client to process |frame|. The resulting processed frame will be
  // stored in |output_buffer_index| output buffer and notified via |cb|. The
  // processor will drop all its references to |frame| after it finishes
  // accessing it. If |output_memory_type_| is V4L2_MEMORY_DMABUF, the caller
  // should pass non-empty |output_dmabuf_fds| and the processed frame will be
  // stored in those buffers. If the number of |output_dmabuf_fds| is not
  // expected, this function will return false.
  bool Process(const scoped_refptr<VideoFrame>& frame,
               int output_buffer_index,
               std::vector<base::ScopedFD> output_dmabuf_fds,
               const FrameReadyCB& cb);

  // Reset all processing frames. After this method returns, no more callbacks
  // will be invoked. V4L2ImageProcessor is ready to process more frames.
  bool Reset();

  // Stop all processing and clean up. After this method returns no more
  // callbacks will be invoked.  Deletes |this| unconditionally, so make sure
  // to drop all pointers to it!
  void Destroy();

 private:
  // Record for input buffers.
  struct InputRecord {
    InputRecord();
    InputRecord(const V4L2ImageProcessor::InputRecord&);
    ~InputRecord();
    scoped_refptr<VideoFrame> frame;
    bool at_device;
  };

  // Record for output buffers.
  struct OutputRecord {
    OutputRecord();
    OutputRecord(OutputRecord&&);
    ~OutputRecord();
    bool at_device;
    // The processed frame will be stored in these buffers if
    // |output_memory_type_| is V4L2_MEMORY_DMABUF
    std::vector<base::ScopedFD> dmabuf_fds;
  };

  // Job record. Jobs are processed in a FIFO order. This is separate from
  // InputRecord, because an InputRecord may be returned before we dequeue
  // the corresponding output buffer. The processed frame will be stored in
  // |output_buffer_index| output buffer. If |output_memory_type_| is
  // V4L2_MEMORY_DMABUF, the processed frame will be stored in
  // |output_dmabuf_fds|.
  struct JobRecord {
    JobRecord();
    ~JobRecord();
    scoped_refptr<VideoFrame> frame;
    int output_buffer_index;
    std::vector<base::ScopedFD> output_dmabuf_fds;
    FrameReadyCB ready_cb;
  };

  void EnqueueInput();
  void EnqueueOutput(int index);
  void Dequeue();
  bool EnqueueInputRecord();
  bool EnqueueOutputRecord(int index);
  bool CreateInputBuffers();
  bool CreateOutputBuffers();
  void DestroyInputBuffers();
  void DestroyOutputBuffers();

  void NotifyError();
  void NotifyErrorOnChildThread(const base::Closure& error_cb);

  void ProcessTask(std::unique_ptr<JobRecord> job_record);
  void ServiceDeviceTask();

  // Attempt to start/stop device_poll_thread_.
  void StartDevicePoll();
  void StopDevicePoll();

  // Ran on device_poll_thread_ to wait for device events.
  void DevicePollTask(bool poll_device);

  // A processed frame is ready.
  void FrameReady(const FrameReadyCB& cb, int output_buffer_index);

  // Size and format-related members remain constant after initialization.
  // The visible/allocated sizes of the input frame.
  gfx::Size input_visible_size_;
  gfx::Size input_allocated_size_;

  // The visible/allocated sizes of the destination frame.
  gfx::Size output_visible_size_;
  gfx::Size output_allocated_size_;

  VideoPixelFormat input_format_;
  VideoPixelFormat output_format_;
  v4l2_memory input_memory_type_;
  v4l2_memory output_memory_type_;
  uint32_t input_format_fourcc_;
  uint32_t output_format_fourcc_;

  size_t input_planes_count_;
  size_t output_planes_count_;

  // Our original calling task runner for the child thread.
  const scoped_refptr<base::SingleThreadTaskRunner> child_task_runner_;

  // V4L2 device in use.
  scoped_refptr<V4L2Device> device_;

  // Thread to communicate with the device on.
  base::Thread device_thread_;
  // Thread used to poll the V4L2 for events only.
  base::Thread device_poll_thread_;

  // All the below members are to be accessed from device_thread_ only
  // (if it's running).
  base::queue<std::unique_ptr<JobRecord>> input_queue_;
  base::queue<std::unique_ptr<JobRecord>> running_jobs_;

  // Input queue state.
  bool input_streamon_;
  // Number of input buffers enqueued to the device.
  int input_buffer_queued_count_;
  // Input buffers ready to use; LIFO since we don't care about ordering.
  std::vector<int> free_input_buffers_;
  // Mapping of int index to an input buffer record.
  std::vector<InputRecord> input_buffer_map_;

  // Output queue state.
  bool output_streamon_;
  // Number of output buffers enqueued to the device.
  int output_buffer_queued_count_;
  // Mapping of int index to an output buffer record.
  std::vector<OutputRecord> output_buffer_map_;
  // The number of input or output buffers.
  int num_buffers_;

  // Error callback to the client.
  base::Closure error_cb_;

  // WeakPtr<> pointing to |this| for use in posting tasks from the device
  // worker threads back to the child thread.  Because the worker threads
  // are members of this class, any task running on those threads is guaranteed
  // that this object is still alive.  As a result, tasks posted from the child
  // thread to the device thread should use base::Unretained(this),
  // and tasks posted the other way should use |weak_this_|.
  base::WeakPtr<V4L2ImageProcessor> weak_this_;

  // Weak factory for producing weak pointers on the child thread.
  base::WeakPtrFactory<V4L2ImageProcessor> weak_this_factory_;

  DISALLOW_COPY_AND_ASSIGN(V4L2ImageProcessor);
};

}  // namespace media

#endif  // MEDIA_GPU_V4L2_V4L2_IMAGE_PROCESSOR_H_
