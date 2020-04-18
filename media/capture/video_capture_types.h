// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CAPTURE_TYPES_H_
#define MEDIA_CAPTURE_VIDEO_CAPTURE_TYPES_H_

#include <stddef.h>

#include <vector>

#include "build/build_config.h"
#include "media/base/video_types.h"
#include "media/capture/capture_export.h"
#include "ui/gfx/geometry/size.h"

namespace media {

// TODO(wjia): this type should be defined in a common place and
// shared with device manager.
typedef int VideoCaptureSessionId;

// Policies for capture devices that have source content that varies in size.
// It is up to the implementation how the captured content will be transformed
// (e.g., scaling and/or letterboxing) in order to produce video frames that
// strictly adheree to one of these policies.
enum class ResolutionChangePolicy {
  // Capture device outputs a fixed resolution all the time. The resolution of
  // the first frame is the resolution for all frames.
  FIXED_RESOLUTION,

  // Capture device is allowed to output frames of varying resolutions. The
  // width and height will not exceed the maximum dimensions specified. The
  // aspect ratio of the frames will match the aspect ratio of the maximum
  // dimensions as closely as possible.
  FIXED_ASPECT_RATIO,

  // Capture device is allowed to output frames of varying resolutions not
  // exceeding the maximum dimensions specified.
  ANY_WITHIN_LIMIT,

  // Must always be equal to largest entry in the enum.
  LAST = ANY_WITHIN_LIMIT,
};

// Potential values of the googPowerLineFrequency optional constraint passed to
// getUserMedia. Note that the numeric values are currently significant, and are
// used to map enum values to corresponding frequency values.
// TODO(ajose): http://crbug.com/525167 Consider making this a class.
enum class PowerLineFrequency {
  FREQUENCY_DEFAULT = 0,
  FREQUENCY_50HZ = 50,
  FREQUENCY_60HZ = 60,
  FREQUENCY_MAX = FREQUENCY_60HZ
};

// Assert that the int:frequency mapping is correct.
static_assert(static_cast<int>(PowerLineFrequency::FREQUENCY_DEFAULT) == 0,
              "static_cast<int>(FREQUENCY_DEFAULT) must equal 0.");
static_assert(static_cast<int>(PowerLineFrequency::FREQUENCY_50HZ) == 50,
              "static_cast<int>(FREQUENCY_DEFAULT) must equal 50.");
static_assert(static_cast<int>(PowerLineFrequency::FREQUENCY_60HZ) == 60,
              "static_cast<int>(FREQUENCY_DEFAULT) must equal 60.");

// Some drivers use rational time per frame instead of float frame rate, this
// constant k is used to convert between both: A fps -> [k/k*A] seconds/frame.
const int kFrameRatePrecision = 10000;

// Video capture format specification.
// This class is used by the video capture device to specify the format of every
// frame captured and returned to a client. It is also used to specify a
// supported capture format by a device.
struct CAPTURE_EXPORT VideoCaptureFormat {
  VideoCaptureFormat();
  VideoCaptureFormat(const gfx::Size& frame_size,
                     float frame_rate,
                     VideoPixelFormat pixel_format);

  static std::string ToString(const VideoCaptureFormat& format);

  // Compares the priority of the pixel formats. Returns true if |lhs| is the
  // preferred pixel format in comparison with |rhs|. Returns false otherwise.
  static bool ComparePixelFormatPreference(const VideoPixelFormat& lhs,
                                           const VideoPixelFormat& rhs);

  // Returns the required buffer size to hold an image of a given
  // VideoCaptureFormat with no padding and tightly packed.
  size_t ImageAllocationSize() const;

  // Checks that all values are in the expected range. All limits are specified
  // in media::Limits.
  bool IsValid() const;

  bool operator==(const VideoCaptureFormat& other) const {
    return frame_size == other.frame_size && frame_rate == other.frame_rate &&
           pixel_format == other.pixel_format;
  }

  gfx::Size frame_size;
  float frame_rate;
  VideoPixelFormat pixel_format;
};

typedef std::vector<VideoCaptureFormat> VideoCaptureFormats;

// Parameters for starting video capture.
// This class is used by the client of a video capture device to specify the
// format of frames in which the client would like to have captured frames
// returned.
struct CAPTURE_EXPORT VideoCaptureParams {
  // Result struct for SuggestContraints() method.
  struct SuggestedConstraints {
    gfx::Size min_frame_size;
    gfx::Size max_frame_size;
    bool fixed_aspect_ratio;
  };

  VideoCaptureParams();

  // Returns true if requested_format.IsValid() and all other values are within
  // their expected ranges.
  bool IsValid() const;

  // Computes and returns suggested capture constraints based on the requested
  // format and resolution change policy: minimum resolution, maximum
  // resolution, and whether a fixed aspect ratio is required.
  SuggestedConstraints SuggestConstraints() const;

  bool operator==(const VideoCaptureParams& other) const {
    return requested_format == other.requested_format &&
           resolution_change_policy == other.resolution_change_policy &&
           power_line_frequency == other.power_line_frequency;
  }

  // Requests a resolution and format at which the capture will occur.
  VideoCaptureFormat requested_format;

  // Policy for resolution change.
  ResolutionChangePolicy resolution_change_policy;

  // User-specified power line frequency.
  PowerLineFrequency power_line_frequency;
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CAPTURE_TYPES_H_
