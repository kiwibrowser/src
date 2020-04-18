// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_DEVICE_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_DEVICE_H_

#include <string>
#include <vector>

#include "base/optional.h"
#include "content/common/content_export.h"
#include "content/renderer/media/stream/media_stream_constraints_util.h"
#include "media/capture/video_capture_types.h"
#include "third_party/blink/public/platform/modules/mediastream/media_devices.mojom.h"

namespace blink {
class WebString;
class WebMediaConstraints;
}

namespace content {

// Calculates and returns videoKind value for |format|.
// See https://w3c.github.io/mediacapture-depth.
blink::WebString CONTENT_EXPORT
GetVideoKindForFormat(const media::VideoCaptureFormat& format);

blink::WebMediaStreamTrack::FacingMode CONTENT_EXPORT
ToWebFacingMode(media::VideoFacingMode video_facing);

struct CONTENT_EXPORT VideoDeviceCaptureCapabilities {
  VideoDeviceCaptureCapabilities();
  VideoDeviceCaptureCapabilities(VideoDeviceCaptureCapabilities&& other);
  ~VideoDeviceCaptureCapabilities();
  VideoDeviceCaptureCapabilities& operator=(
      VideoDeviceCaptureCapabilities&& other);

  // Each field is independent of each other.
  std::vector<blink::mojom::VideoInputDeviceCapabilitiesPtr>
      device_capabilities;
  std::vector<media::PowerLineFrequency> power_line_capabilities;
  std::vector<base::Optional<bool>> noise_reduction_capabilities;
};

// This function performs source, source-settings and track-settings selection
// based on the given |capabilities| and |constraints|.
// Chromium performs constraint resolution in two steps. First, a source and its
// settings are selected, then track settings are selected based on the source
// settings. This function implements both steps. Sources are not a user-visible
// concept, so the spec only specifies an algorithm for track settings.
// The algorithm for sources is compatible with the spec algorithm for tracks,
// as defined in https://w3c.github.io/mediacapture-main/#dfn-selectsettings,
// but it is customized to account for differences between sources and tracks,
// and to break ties when multiple source settings are equally good according to
// the spec algorithm.
// The main difference between a source and a track with regards to the spec
// algorithm is that a candidate  source can support a range of values for some
// properties while a candidate track supports a single value. For example,
// cropping allows a source with native resolution AxB to support the range of
// resolutions from 1x1 to AxB.
// Only candidates that satisfy the basic constraint set are valid. If no
// candidate can satisfy the basic constraint set, this function returns
// a result without value and with the name of a failed constraint accessible
// via the failed_constraint_name() method. If at least one candidate that
// satisfies the basic constraint set can be found, this function returns a
// result with a valid value.
// If there are no candidates at all, this function returns a result without
// value and an empty failed constraint name.
// The criteria to decide if a valid candidate is better than another one are as
// follows:
// 1. Given advanced constraint sets A[0],A[1]...,A[n], candidate C1 is better
//    than candidate C2 if C1 supports the first advanced set for which C1's
//    support is different than C2's support.
//    Examples:
//    * One advanced set, C1 supports it, and C2 does not. C1 is better.
//    * Two sets, C1 supports both, C2 supports only the first. C1 is better.
//    * Three sets, C1 supports the first and second set, C2 supports the first
//      and third set. C1 is better.
// 2. C1 is better than C2 if C1 has a smaller fitness distance than C2. The
//    fitness distance depends on the ability of the candidate to support ideal
//    values in the basic constraint set. This is the final criterion defined in
//    the spec.
// 3. C1 is better than C2 if C1 has a lower Chromium-specific custom distance
//    from the basic constraint set. This custom distance is the sum of various
//    constraint-specific custom distances.
//    For example, if the constraint set specifies a resolution of exactly
//    1000x1000 for a track, then a candidate with a resolution of 1200x1200
//    is better than a candidate with a resolution of 2000x2000. Both settings
//    satisfy the constraint set because cropping can be used to produce the
//    track setting of 1000x1000, but 1200x1200 is considered better because it
//    has lower resource usage. The same criteria applies for each advanced
//    constraint set.
// 4. C1 is better than C2 if its native settings have a smaller fitness
//    distance. For example, if the ideal resolution is 1000x1000 and C1 has a
//    native resolution of 1200x1200, while C2 has a native resolution of
//    2000x2000, then C1 is better because it can support the ideal value with
//    lower resource usage. Both C1 and C2 are better than a candidate C3 with
//    a native resolution of 999x999, since C3 has a nonzero distance to the
//    ideal value and thus has worse fitness according to step 2, even if C3's
//    native fitness is better than C1's and C2's.
// 5. C1 is better than C2 if its settings are closer to certain default
//    settings that include the device ID, power-line frequency, noise
//    reduction, resolution, and frame rate, in that order. Note that there is
//    no default facing mode or aspect ratio.
// This function uses the SelectVideoTrackAdapterSettings function to compute
// some track-specific settings. These are available in the returned value via
// the track_adapter_settings() accessor. For more details about the algorithm
// for track adapter settings, see the SelectVideoTrackAdapterSettings
// documentation.
VideoCaptureSettings CONTENT_EXPORT SelectSettingsVideoDeviceCapture(
    const VideoDeviceCaptureCapabilities& capabilities,
    const blink::WebMediaConstraints& constraints,
    int default_width,
    int default_height,
    double default_frame_rate);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_DEVICE_H_
