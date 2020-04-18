// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_VIDEO_SOURCE_PRIVATE_H_
#define PPAPI_CPP_PRIVATE_VIDEO_SOURCE_PRIVATE_H_

#include <stdint.h>

#include <string>

#include "ppapi/c/pp_time.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/pass_ref.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the <code>PPB_VideoSource_Private</code> interface for a
/// video source resource, which receives video frames from a MediaStream video
/// track in the browser.

namespace pp {

class InstanceHandle;
class VideoFrame_Private;

/// The <code>VideoSource_Private</code> class contains methods for creating
/// video source resources and using them to receive video frames from a
/// MediaStream video track in the browser.
class VideoSource_Private : public Resource {
 public:
  /// Default constructor for creating a <code>VideoSource_Private</code>
  /// object.
  VideoSource_Private();

  /// Constructor for creating a <code>VideoSource_Private</code> for an
  /// instance.
  explicit VideoSource_Private(const InstanceHandle& instance);

  /// The copy constructor for <code>VideoSource_Private</code>.
  ///
  /// @param[in] other A reference to a <code>VideoSource_Private</code>.
  VideoSource_Private(const VideoSource_Private& other);

  /// A constructor used when you have received a PP_Resource as a return
  /// value that has had its reference count incremented for you.
  ///
  /// @param[in] resource A PP_Resource corresponding to a video source.
  VideoSource_Private(PassRef, PP_Resource resource);

  /// Opens a video source for getting frames.
  ///
  /// @param[in] stream_url A <code>Var</code> string holding a URL identifying
  /// a MediaStream.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion of Open().
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_BADRESOURCE if source isn't a valid video source.
  /// Returns PP_ERROR_INPROGRESS if source is already open.
  /// Returns PP_ERROR_FAILED if the MediaStream doesn't exist or if there is
  /// some other browser error.
  int32_t Open(const Var& stream_url,
               const CompletionCallback& cc);

  /// Gets a frame from the video source. The returned frame is only valid
  /// until the next call to GetFrame.
  ///
  /// @param[out] frame A <code>VideoFrame_Private</code> to hold a video
  /// frame from the source.
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion of ReceiveFrame().
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_BADRESOURCE if source isn't a valid video source.
  /// Returns PP_ERROR_FAILED if the source is not open, or if some other
  /// browser error occurs.
  int32_t GetFrame(
      const CompletionCallbackWithOutput<VideoFrame_Private>& cc);

  /// Closes the video source.
  void Close();
};

}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_VIDEO_SOURCE_PRIVATE_H_
