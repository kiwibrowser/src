// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_VIDEO_DESTINATION_PRIVATE_H_
#define PPAPI_CPP_PRIVATE_VIDEO_DESTINATION_PRIVATE_H_

#include <stdint.h>

#include <string>

#include "ppapi/c/pp_time.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/pass_ref.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the <code>PPB_VideoDestination_Private</code> interface
/// for a video destination resource, which sends video frames to a MediaStream
/// video track in the browser.

namespace pp {

class InstanceHandle;
class VideoFrame_Private;

/// The <code>VideoDestination_Private</code> class contains methods for
/// creating video destination resources and using them to send video frames to
/// a MediaStream video track in the browser.
class VideoDestination_Private : public Resource {
 public:
  /// Default constructor for creating a <code>VideoDestination_Private</code>
  /// object.
  VideoDestination_Private();

  /// Constructor for creating a <code>VideoDestination_Private</code> for an
  /// instance.
  explicit VideoDestination_Private(const InstanceHandle& instance);

  /// The copy constructor for <code>VideoDestination_Private</code>.
  ///
  /// @param[in] other A reference to a <code>VideoDestination_Private</code>.
  VideoDestination_Private(const VideoDestination_Private& other);

  /// A constructor used when you have received a PP_Resource as a return
  /// value that has had its reference count incremented for you.
  ///
  /// @param[in] resource A PP_Resource corresponding to a video destination.
  VideoDestination_Private(PassRef, PP_Resource resource);

  /// Opens a video destination for putting frames.
  ///
  /// @param[in] stream_url A <code>Var</code> string holding a URL identifying
  /// a MediaStream.
  /// @param[in] callback A <code>CompletionCallback</code> to be
  /// called upon completion of Open().
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_BADRESOURCE if destination isn't a valid video
  /// destination.
  /// Returns PP_ERROR_INPROGRESS if destination is already open.
  /// Returns PP_ERROR_FAILED if the MediaStream doesn't exist or if there is
  /// some other browser error.
  int32_t Open(const Var& stream_url, const CompletionCallback& cc);

  /// Puts a frame to the video destination.
  ///
  /// After this call, you should take care to release your references to the
  /// image embedded in the video frame. If you paint to the image after
  /// PutFrame(), there is the possibility of artifacts because the browser may
  /// still be copying the frame to the stream.
  ///
  /// @param[in] frame A <code>VideoFrame_Private</code> holding the video
  /// frame to send to the destination.
  ///
  /// @return An int32_t containing a result code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_BADRESOURCE if destination isn't a valid video
  /// destination.
  /// Returns PP_ERROR_FAILED if destination is not open, if the video frame has
  /// an invalid image data resource, or if some other browser error occurs.
  int32_t PutFrame(const VideoFrame_Private& frame);

  /// Closes the video destination.
  void Close();
};

}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_VIDEO_DESTINATION_PRIVATE_H_
