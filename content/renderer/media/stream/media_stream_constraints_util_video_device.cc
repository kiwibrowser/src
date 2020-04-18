// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_constraints_util_video_device.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "content/renderer/media/stream/media_stream_constraints_util.h"
#include "content/renderer/media/stream/media_stream_constraints_util_sets.h"
#include "content/renderer/media/stream/media_stream_video_source.h"
#include "media/base/limits.h"
#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_string.h"

namespace content {

namespace {

// Number of default settings to be used as final tie-breaking criteria for
// settings that are equally good at satisfying constraints:
// device ID, power-line frequency, noise reduction, resolution and frame rate.
const int kNumDefaultDistanceEntries = 5;

// The minimum aspect ratio to be supported by sources.
const double kMinSourceAspectRatio = 0.05;

// VideoKind enum values. See https://w3c.github.io/mediacapture-depth.
const char kVideoKindColor[] = "color";
const char kVideoKindDepth[] = "depth";

blink::WebString ToWebString(media::VideoFacingMode facing_mode) {
  switch (facing_mode) {
    case media::MEDIA_VIDEO_FACING_USER:
      return blink::WebString::FromASCII("user");
    case media::MEDIA_VIDEO_FACING_ENVIRONMENT:
      return blink::WebString::FromASCII("environment");
    default:
      return blink::WebString::FromASCII("");
  }
}

struct Candidate {
 public:
  Candidate(const std::string& device_id,
            const std::string& group_id,
            const media::VideoCaptureFormat& format,
            media::VideoFacingMode facing_mode,
            media::PowerLineFrequency power_line_frequency,
            const base::Optional<bool>& noise_reduction)
      : device_id_(device_id),
        group_id_(group_id),
        format_(format),
        facing_mode_(facing_mode),
        power_line_frequency_(power_line_frequency),
        noise_reduction_(noise_reduction) {}

  // These accessor-like methods transform types to what Blink constraint
  // classes expect.
  blink::WebString GetFacingMode() const { return ToWebString(facing_mode_); }
  long GetPowerLineFrequency() const {
    return static_cast<long>(power_line_frequency_);
  }
  blink::WebString GetDeviceId() const {
    return blink::WebString::FromASCII(device_id_.data());
  }
  blink::WebString GetGroupId() const {
    return blink::WebString::FromASCII(group_id_.data());
  }
  blink::WebString GetVideoKind() const {
    return GetVideoKindForFormat(format_);
  }

  // Accessors.
  const media::VideoCaptureFormat& format() const { return format_; }
  const std::string& device_id() const { return device_id_; }
  const std::string& group_id() const { return group_id_; }
  media::VideoFacingMode facing_mode() const { return facing_mode_; }
  media::PowerLineFrequency power_line_frequency() const {
    return power_line_frequency_;
  }
  const base::Optional<bool>& noise_reduction() const {
    return noise_reduction_;
  }

 private:
  std::string device_id_;
  std::string group_id_;
  media::VideoCaptureFormat format_;
  media::VideoFacingMode facing_mode_;
  media::PowerLineFrequency power_line_frequency_;
  base::Optional<bool> noise_reduction_;
};

// The ConstrainedFormat class keeps track of the effect of constraint sets on
// the range of values supported by a video-capture format while iterating over
// the constraint sets.
// For example, suppose a device supports a width of 1024. Then, in principle,
// it can support any width below 1024 using cropping.
// Suppose the first advanced constraint set requests a maximum width of 640,
// and the second advanced constraint set requests a minimum of 800.
// Separately, the camera supports both advanced sets. However, if the first
// set is supported, the second set can no longer be supported because width can
// no longer exceed 640. The ConstrainedFormat class keeps track of this.
class ConstrainedFormat {
 public:
  explicit ConstrainedFormat(const media::VideoCaptureFormat& format)
      : native_height_(format.frame_size.height()),
        native_width_(format.frame_size.width()),
        native_frame_rate_(format.frame_rate),
        constrained_resolution_(1,
                                format.frame_size.height(),
                                1,
                                format.frame_size.width(),
                                0.0,
                                HUGE_VAL) {}

  long native_height() const { return native_height_; }
  long native_width() const { return native_width_; }
  double native_frame_rate() const { return native_frame_rate_; }
  const ResolutionSet& constrained_resolution() const {
    return constrained_resolution_;
  }
  const NumericRangeSet<double>& constrained_frame_rate() const {
    return constrained_frame_rate_;
  }

  long MinHeight() const { return constrained_resolution_.min_height(); }
  long MaxHeight() const { return constrained_resolution_.max_height(); }
  long MinWidth() const { return constrained_resolution_.min_width(); }
  long MaxWidth() const { return constrained_resolution_.max_width(); }
  double MinAspectRatio() const {
    return constrained_resolution_.min_aspect_ratio();
  }
  double MaxAspectRatio() const {
    return constrained_resolution_.max_aspect_ratio();
  }
  const base::Optional<double>& MinFrameRate() const {
    return constrained_frame_rate_.Min();
  }
  const base::Optional<double>& MaxFrameRate() const {
    return constrained_frame_rate_.Max();
  }

  // Returns true if the application of |constraint_set| is successful and
  // results in a nonempty set. Returns true otherwise.
  bool ApplyConstraintSet(
      const blink::WebMediaTrackConstraintSet& constraint_set) {
    auto resolution_intersection = constrained_resolution_.Intersection(
        ResolutionSet::FromConstraintSet(constraint_set));
    auto frame_rate_intersection = constrained_frame_rate_.Intersection(
        NumericRangeSet<double>::FromConstraint(
            constraint_set.frame_rate, 0.0,
            media::limits::kMaxFramesPerSecond));
    if (resolution_intersection.IsEmpty() ||
        frame_rate_intersection.IsEmpty() ||
        frame_rate_intersection.Min().value_or(0.0) > native_frame_rate_) {
      return false;
    }

    constrained_resolution_ = resolution_intersection;
    constrained_frame_rate_ = frame_rate_intersection;
    return true;
  }

 private:
  // Using long for compatibility with Blink constraint classes.
  long native_height_;
  long native_width_;
  double native_frame_rate_;
  ResolutionSet constrained_resolution_;
  NumericRangeSet<double> constrained_frame_rate_;
};

VideoCaptureSettings ComputeVideoDeviceCaptureSettings(
    const Candidate& candidate,
    const ConstrainedFormat& constrained_format,
    const blink::WebMediaTrackConstraintSet& basic_constraint_set) {
  media::VideoCaptureParams capture_params;
  capture_params.requested_format = candidate.format();
  capture_params.power_line_frequency = candidate.power_line_frequency();
  auto track_adapter_settings = SelectVideoTrackAdapterSettings(
      basic_constraint_set, constrained_format.constrained_resolution(),
      constrained_format.constrained_frame_rate(),
      capture_params.requested_format);

  return VideoCaptureSettings(
      candidate.device_id(), capture_params, candidate.noise_reduction(),
      track_adapter_settings, constrained_format.constrained_frame_rate().Min(),
      constrained_format.constrained_frame_rate().Max());
}

// Returns a pair with the minimum and maximum aspect ratios supported by the
// candidate format |constrained_format|, subject to given width and height
// constraints.
void GetSourceAspectRatioRange(const ConstrainedFormat& constrained_format,
                               const blink::LongConstraint& height_constraint,
                               const blink::LongConstraint& width_constraint,
                               double* min_source_aspect_ratio,
                               double* max_source_aspect_ratio) {
  long min_height = constrained_format.MinHeight();
  if (ConstraintHasMin(height_constraint))
    min_height = std::max(min_height, ConstraintMin(height_constraint));

  long max_height = constrained_format.MaxHeight();
  if (ConstraintHasMax(height_constraint))
    max_height = std::min(max_height, ConstraintMax(height_constraint));

  long min_width = constrained_format.MinWidth();
  if (ConstraintHasMin(width_constraint))
    min_width = std::max(min_width, ConstraintMin(width_constraint));

  long max_width = constrained_format.MaxWidth();
  if (ConstraintHasMax(width_constraint))
    max_width = std::min(max_width, ConstraintMax(width_constraint));

  *min_source_aspect_ratio = std::max(
      constrained_format.MinAspectRatio(),
      std::max(static_cast<double>(min_width) / static_cast<double>(max_height),
               kMinSourceAspectRatio));
  *max_source_aspect_ratio = std::min(
      constrained_format.MaxAspectRatio(),
      std::max(static_cast<double>(max_width) / static_cast<double>(min_height),
               kMinSourceAspectRatio));
}

// Returns a custom distance between a string and a string constraint.
// Returns 0 if |value| satisfies |constraint|. HUGE_VAL otherwise.
double StringConstraintSourceDistance(const blink::WebString& value,
                                      const blink::StringConstraint& constraint,
                                      const char** failed_constraint_name) {
  if (constraint.Matches(value))
    return 0.0;

  if (failed_constraint_name)
    *failed_constraint_name = constraint.GetName();
  return HUGE_VAL;
}

// Returns a custom distance between a source screen dimension and |constraint|.
// The source supports the range [|min_source_value|, |max_source_value|], using
// cropping if necessary. This range may differ from the native range of the
// source [1, |native_source_value|] due to the application of previous
// constraint sets.
// If the range supported by the source and the range specified by |constraint|
// are disjoint, the distance is infinite.
// If |constraint| has a maximum, penalize sources whose native resolution
// exceeds the maximum by returning
// Distance(|native_source_value|, |constraint.max()|). This is intended to
// prefer, among sources that satisfy the constraint, those that have lower
// resource usage. Otherwise, return zero.
double ResolutionConstraintSourceDistance(
    int native_source_value,
    int min_source_value,
    int max_source_value,
    const blink::LongConstraint& constraint,
    const char** failed_constraint_name) {
  DCHECK_GE(native_source_value, 0);
  bool constraint_has_min = ConstraintHasMin(constraint);
  long constraint_min = constraint_has_min ? ConstraintMin(constraint) : -1;
  bool constraint_has_max = ConstraintHasMax(constraint);
  long constraint_max = constraint_has_max ? ConstraintMax(constraint) : -1;

  // If the intersection between the source range and the constraint range is
  // empty, return HUGE_VAL.
  if ((constraint_has_max && min_source_value > constraint_max) ||
      (constraint_has_min && max_source_value < constraint_min) ||
      (constraint_has_min && constraint_has_max &&
       constraint_min > constraint_max)) {
    if (failed_constraint_name)
      *failed_constraint_name = constraint.GetName();
    return HUGE_VAL;
  }

  // If the source value exceeds the maximum requested, penalize.
  if (constraint_has_max && native_source_value > constraint_max)
    return NumericConstraintFitnessDistance(native_source_value,
                                            constraint_max);

  return 0.0;
}

// Returns a custom distance function suitable for frame rate, given
// a |constraint| and a candidate format |constrained_format|.
// A source can support track frame rates in the interval
// [min_frame_rate, max_frame_rate], using frame-rate adjustments if
// necessary. If the candidate range and the constraint range are disjoint,
// return HUGE_VAL.
// The supported source range may differ from the native range of the source
// (0, native_source_range] due to the application of previous constraint sets.
// If |constraint| has a maximum, penalize native frame rates that exceed the
// maximum by returning Distance(native_frame_rate, maximum). This is intended
// to prefer, among sources that satisfy the constraint, those that have lower
// resource usage. Otherwise, return zero.
double FrameRateConstraintSourceDistance(
    const ConstrainedFormat& constrained_format,
    const blink::DoubleConstraint& constraint,
    const char** failed_constraint_name) {
  double constraint_min =
      ConstraintHasMin(constraint) ? ConstraintMin(constraint) : -1.0;
  double constraint_max = ConstraintHasMax(constraint)
                              ? ConstraintMax(constraint)
                              : media::limits::kMaxFramesPerSecond;

  bool constraint_min_out_of_range =
      ((constraint_min > constrained_format.native_frame_rate()) ||
       (constraint_min > constrained_format.MaxFrameRate().value_or(
                             media::limits::kMaxFramesPerSecond) +
                             blink::DoubleConstraint::kConstraintEpsilon));
  bool constraint_max_out_of_range =
      ((constraint_max < 1.0) ||
       (constraint_max < constrained_format.MinFrameRate().value_or(0.0) -
                             blink::DoubleConstraint::kConstraintEpsilon));
  bool constraint_self_contradicts = constraint_min > constraint_max;

  if (constraint_min_out_of_range || constraint_max_out_of_range ||
      constraint_self_contradicts) {
    if (failed_constraint_name)
      *failed_constraint_name = constraint.GetName();
    return HUGE_VAL;
  }

  // Compute the cost using the native rate.
  if (constrained_format.native_frame_rate() > constraint_max) {
    return NumericConstraintFitnessDistance(
        constrained_format.native_frame_rate(), constraint_max);
  }

  return 0.0;
}

// Returns a custom distance function suitable for aspect ratio, given
// the values for the aspect_ratio, width and height constraints, and a
// candidate format |constrained_format|.
// A source can support track resolutions that range from
//    min_width x min_height  to  max_width x max_height
// where
//    min_width = max(1, width_constraint.min)
//    min_height = max(1, height_constraint.min)
//    max_width = min(source_width, width_constraint.max)
//    max_height = min(source_height, height_constraint.max)
// The aspect-ratio range supported by the source is determined by the extremes
// of those resolutions.
//    min_ar = min_width / max_height.
//    max_ar = max_width / min_height.
//
// If the supported range [min_ar, max_ar] and the range specified by the
// aspectRatio constraint are disjoint, return HUGE_VAL. Otherwise, return zero.
double AspectRatioConstraintSourceDistance(
    const ConstrainedFormat& constrained_format,
    const blink::LongConstraint& height_constraint,
    const blink::LongConstraint& width_constraint,
    const blink::DoubleConstraint& aspect_ratio_constraint,
    const char** failed_constraint_name) {
  bool ar_constraint_has_min = ConstraintHasMin(aspect_ratio_constraint);
  double ar_constraint_min =
      ar_constraint_has_min ? ConstraintMin(aspect_ratio_constraint) : -1.0;
  bool ar_constraint_has_max = ConstraintHasMax(aspect_ratio_constraint);
  double ar_constraint_max =
      ar_constraint_has_max ? ConstraintMax(aspect_ratio_constraint) : -1.0;

  double min_source_aspect_ratio;
  double max_source_aspect_ratio;
  GetSourceAspectRatioRange(constrained_format, height_constraint,
                            width_constraint, &min_source_aspect_ratio,
                            &max_source_aspect_ratio);

  // If the supported range and the constraint rage are disjoint, return
  // HUGE_VAL.
  if ((ar_constraint_has_min &&
       max_source_aspect_ratio <
           ar_constraint_min - blink::DoubleConstraint::kConstraintEpsilon) ||
      (ar_constraint_has_max &&
       min_source_aspect_ratio >
           ar_constraint_max + blink::DoubleConstraint::kConstraintEpsilon) ||
      (ar_constraint_has_min && ar_constraint_has_max &&
       ar_constraint_min > ar_constraint_max)) {
    if (failed_constraint_name)
      *failed_constraint_name = aspect_ratio_constraint.GetName();
    return HUGE_VAL;
  }

  return 0.0;
}

// Returns a custom distance function suitable for the googPowerLineFrequency
// constraint, given  a |constraint| and a candidate value |source_value|.
// The distance is HUGE_VAL if |source_value| cannot satisfy |constraint|.
// Otherwise, the distance is zero.
double PowerLineFrequencyConstraintSourceDistance(
    const blink::LongConstraint& constraint,
    media::PowerLineFrequency source_value,
    const char** failed_constraint_name) {
  bool constraint_has_min = ConstraintHasMin(constraint);
  bool constraint_has_max = ConstraintHasMax(constraint);
  long constraint_min = constraint_has_min ? ConstraintMin(constraint) : -1L;
  long constraint_max = constraint_has_max ? ConstraintMax(constraint) : -1L;
  long source_value_long = static_cast<long>(source_value);

  if ((constraint_has_max && source_value_long > constraint_max) ||
      (constraint_has_min && source_value_long < constraint_min) ||
      (constraint_has_min && constraint_has_max &&
       constraint_min > constraint_max)) {
    if (failed_constraint_name)
      *failed_constraint_name = constraint.GetName();
    return HUGE_VAL;
  }

  return 0.0;
}

// Returns a custom distance function suitable for the googNoiseReduction
// constraint, given  a |constraint| and a candidate value |value|.
// The distance is HUGE_VAL if |candidate_value| cannot satisfy |constraint|.
// Otherwise, the distance is zero.
double NoiseReductionConstraintSourceDistance(
    const blink::BooleanConstraint& constraint,
    const base::Optional<bool>& value,
    const char** failed_constraint_name) {
  if (!constraint.HasExact())
    return 0.0;

  if (value && *value == constraint.Exact())
    return 0.0;

  if (failed_constraint_name)
    *failed_constraint_name = constraint.GetName();
  return HUGE_VAL;
}

// Returns a custom distance for constraints that depend on the device
// characteristics that have a fixed value.
double DeviceSourceDistance(
    const std::string& device_id,
    const std::string& group_id,
    media::VideoFacingMode facing_mode,
    const blink::WebMediaTrackConstraintSet& constraint_set,
    const char** failed_constraint_name) {
  return StringConstraintSourceDistance(blink::WebString::FromASCII(device_id),
                                        constraint_set.device_id,
                                        failed_constraint_name) +
         StringConstraintSourceDistance(blink::WebString::FromASCII(group_id),
                                        constraint_set.group_id,
                                        failed_constraint_name) +
         StringConstraintSourceDistance(ToWebString(facing_mode),
                                        constraint_set.facing_mode,
                                        failed_constraint_name);
}

// Returns a custom distance between |constraint_set| and |format| given that
// the configuration is already constrained by |constrained_format|.
// |constrained_format| may cause the distance to be infinite if
// |constraint_set| cannot be satisfied together with previous constraint sets,
// but will not influence the numeric value returned if it is not infinite.
// Formats with lower distance satisfy |constraint_set| with lower resource
// usage.
double FormatSourceDistance(
    const media::VideoCaptureFormat& format,
    const ConstrainedFormat& constrained_format,
    const blink::WebMediaTrackConstraintSet& constraint_set,
    const char** failed_constraint_name) {
  return ResolutionConstraintSourceDistance(
             constrained_format.native_height(), constrained_format.MinHeight(),
             constrained_format.MaxHeight(), constraint_set.height,
             failed_constraint_name) +
         ResolutionConstraintSourceDistance(
             constrained_format.native_width(), constrained_format.MinWidth(),
             constrained_format.MaxWidth(), constraint_set.width,
             failed_constraint_name) +
         AspectRatioConstraintSourceDistance(
             constrained_format, constraint_set.height, constraint_set.width,
             constraint_set.aspect_ratio, failed_constraint_name) +
         FrameRateConstraintSourceDistance(constrained_format,
                                           constraint_set.frame_rate,
                                           failed_constraint_name) +
         StringConstraintSourceDistance(GetVideoKindForFormat(format),
                                        constraint_set.video_kind,
                                        failed_constraint_name);
}

// Returns a custom distance between |constraint_set| and |candidate|, given
// that the configuration is already constrained by |constrained_format|.
// |constrained_format| may cause the distance to be infinite if
// |constraint_set| cannot be satisfied together with previous constraint sets,
// but will not influence the numeric value returned if it is not infinite.
// Candidates with lower distance satisfy |constraint_set| with lower resource
// usage.
double CandidateSourceDistance(
    const Candidate& candidate,
    const ConstrainedFormat& constrained_format,
    const blink::WebMediaTrackConstraintSet& constraint_set,
    const char** failed_constraint_name) {
  return DeviceSourceDistance(candidate.device_id(), candidate.group_id(),
                              candidate.facing_mode(), constraint_set,
                              failed_constraint_name) +
         FormatSourceDistance(candidate.format(), constrained_format,
                              constraint_set, failed_constraint_name) +
         PowerLineFrequencyConstraintSourceDistance(
             constraint_set.goog_power_line_frequency,
             candidate.power_line_frequency(), failed_constraint_name) +
         NoiseReductionConstraintSourceDistance(
             constraint_set.goog_noise_reduction, candidate.noise_reduction(),
             failed_constraint_name);
}

// Returns the fitness distance between |value| and |constraint| for
// resolution constraints (i.e., width and height).
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double ResolutionConstraintFitnessDistance(
    long value,
    const blink::LongConstraint& constraint) {
  if (!constraint.HasIdeal())
    return 0.0;

  // Source resolutions greater than ideal support the ideal value with
  // cropping.
  if (value >= constraint.Ideal())
    return 0.0;

  return NumericConstraintFitnessDistance(value, constraint.Ideal());
}

// Returns the fitness distance between |value| and |constraint| for
// resolution constraints (i.e., width and height), ignoring cropping.
// This measures how well a native resolution supports the idea value.
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double ResolutionConstraintNativeFitnessDistance(
    long value,
    const blink::LongConstraint& constraint) {
  return constraint.HasIdeal()
             ? NumericConstraintFitnessDistance(value, constraint.Ideal())
             : 0.0;
}

// Returns the fitness distance between a source resolution settings
// and the aspectRatio constraint, taking into account resolution restrictions
// on the source imposed by the width and height constraints.
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double AspectRatioConstraintFitnessDistance(
    const ConstrainedFormat& constrained_format,
    const blink::LongConstraint& height_constraint,
    const blink::LongConstraint& width_constraint,
    const blink::DoubleConstraint& aspect_ratio_constraint) {
  if (!aspect_ratio_constraint.HasIdeal())
    return 0.0;

  double min_source_aspect_ratio;
  double max_source_aspect_ratio;
  GetSourceAspectRatioRange(constrained_format, height_constraint,
                            width_constraint, &min_source_aspect_ratio,
                            &max_source_aspect_ratio);

  // If the supported aspect ratio range does not include the ideal aspect
  // ratio, compute fitness using the spec formula.
  if (max_source_aspect_ratio <
      aspect_ratio_constraint.Ideal() -
          blink::DoubleConstraint::kConstraintEpsilon) {
    return NumericConstraintFitnessDistance(max_source_aspect_ratio,
                                            aspect_ratio_constraint.Ideal());
  }

  if (min_source_aspect_ratio >
      aspect_ratio_constraint.Ideal() +
          blink::DoubleConstraint::kConstraintEpsilon) {
    return NumericConstraintFitnessDistance(min_source_aspect_ratio,
                                            aspect_ratio_constraint.Ideal());
  }

  // Otherwise, the ideal aspect ratio can be supported and the fitness is 0.
  return 0.0;
}

// Returns the fitness distance between |value| and |constraint| for the
// frameRate constraint.
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double FrameRateConstraintFitnessDistance(
    double value,
    const blink::DoubleConstraint& constraint) {
  if (!constraint.HasIdeal())
    return 0.0;

  // Source frame rates greater than ideal support the ideal value using
  // frame-rate adjustment.
  if (value >=
      constraint.Ideal() - blink::DoubleConstraint::kConstraintEpsilon) {
    return 0.0;
  }

  return NumericConstraintFitnessDistance(value, constraint.Ideal());
}

// Returns the fitness distance between |value| and |constraint| for the
// frameRate constraint, ignoring frame-rate adjustment.
// It measures how well the native frame rate supports the ideal value.
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double FrameRateConstraintNativeFitnessDistance(
    double value,
    const blink::DoubleConstraint& constraint) {
  return constraint.HasIdeal()
             ? NumericConstraintFitnessDistance(value, constraint.Ideal())
             : 0.0;
}

// Returns the fitness distance between |value| and |constraint| for the
// googPowerLineFrequency constraint.
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double PowerLineFrequencyConstraintFitnessDistance(
    long value,
    const blink::LongConstraint& constraint) {
  if (!constraint.HasIdeal())
    return 0.0;

  // This constraint is of type long, but it behaves as an enum. Thus, values
  // equal to ideal have fitness 0.0 and any other values have fitness 1.0.
  if (value == constraint.Ideal())
    return 0.0;

  return 1.0;
}

// Returns the fitness distance between |value| and |constraint| for the
// googNoiseReduction constraint.
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double NoiseReductionConstraintFitnessDistance(
    const base::Optional<bool>& value,
    const blink::BooleanConstraint& constraint) {
  if (!constraint.HasIdeal())
    return 0.0;

  if (value && value == constraint.Ideal())
    return 0.0;

  return 1.0;
}

// Returns the fitness distance between |constraint_set| and |candidate| given
// that the configuration is already constrained by |constrained_format|.
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double CandidateFitnessDistance(
    const Candidate& candidate,
    const ConstrainedFormat& constrained_format,
    const blink::WebMediaTrackConstraintSet& constraint_set) {
  DCHECK(std::isfinite(CandidateSourceDistance(candidate, constrained_format,
                                               constraint_set, nullptr)));
  double fitness = 0.0;
  fitness += AspectRatioConstraintFitnessDistance(
      constrained_format, constraint_set.height, constraint_set.width,
      constraint_set.aspect_ratio);
  fitness += StringConstraintFitnessDistance(candidate.GetDeviceId(),
                                             constraint_set.device_id);
  fitness += StringConstraintFitnessDistance(candidate.GetGroupId(),
                                             constraint_set.group_id);
  fitness += StringConstraintFitnessDistance(candidate.GetFacingMode(),
                                             constraint_set.facing_mode);
  fitness += StringConstraintFitnessDistance(candidate.GetVideoKind(),
                                             constraint_set.video_kind);
  fitness += PowerLineFrequencyConstraintFitnessDistance(
      candidate.GetPowerLineFrequency(),
      constraint_set.goog_power_line_frequency);
  fitness += NoiseReductionConstraintFitnessDistance(
      candidate.noise_reduction(), constraint_set.goog_noise_reduction);
  // No need to pass minimum value to compute fitness for range-based
  // constraints because all candidates start out with the same minimum and are
  // subject to the same constraints.
  fitness += ResolutionConstraintFitnessDistance(constrained_format.MaxHeight(),
                                                 constraint_set.height);
  fitness += ResolutionConstraintFitnessDistance(constrained_format.MaxWidth(),
                                                 constraint_set.width);
  fitness += FrameRateConstraintFitnessDistance(
      constrained_format.MaxFrameRate().value_or(
          constrained_format.native_frame_rate()),
      constraint_set.frame_rate);

  return fitness;
}

// Returns the native fitness distance between a candidate format and a
// constraint set. The returned value is the sum of the fitness distances for
// the native values of settings that support a range of values (i.e., width,
// height and frame rate).
// Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance.
double CandidateNativeFitnessDistance(
    const ConstrainedFormat& constrained_format,
    const blink::WebMediaTrackConstraintSet& constraint_set) {
  double fitness = 0.0;
  fitness += FrameRateConstraintNativeFitnessDistance(
      constrained_format.native_frame_rate(), constraint_set.frame_rate);
  fitness += ResolutionConstraintNativeFitnessDistance(
      constrained_format.native_height(), constraint_set.height);
  fitness += ResolutionConstraintNativeFitnessDistance(
      constrained_format.native_width(), constraint_set.width);

  return fitness;
}

using DistanceVector = std::vector<double>;

// This function appends additional entries to |distance_vector| based on
// custom distance metrics between |candidate| and some default settings.
// These entries are to be used as the final tie breaker for candidates that
// are equally good according to the spec and the custom distance functions
// between candidates and constraints.
void AppendDistanceFromDefault(
    const Candidate& candidate,
    const VideoDeviceCaptureCapabilities& capabilities,
    int default_width,
    int default_height,
    double default_frame_rate,
    DistanceVector* distance_vector) {
  // Favor IDs that appear first in the enumeration.
  for (size_t i = 0; i < capabilities.device_capabilities.size(); ++i) {
    if (candidate.device_id() ==
        capabilities.device_capabilities[i]->device_id) {
      distance_vector->push_back(i);
      break;
    }
  }

  // Prefer default power-line frequency.
  double power_line_frequency_distance =
      candidate.power_line_frequency() ==
              media::PowerLineFrequency::FREQUENCY_DEFAULT
          ? 0.0
          : HUGE_VAL;
  distance_vector->push_back(power_line_frequency_distance);

  // Prefer not having a specific noise-reduction value and let the lower-layers
  // implementation choose a noise-reduction strategy.
  double noise_reduction_distance =
      candidate.noise_reduction() ? HUGE_VAL : 0.0;
  distance_vector->push_back(noise_reduction_distance);

  // Prefer a resolution closest to the default.
  double resolution_distance = ResolutionSet::Point::SquareEuclideanDistance(
      ResolutionSet::Point(candidate.format().frame_size.height(),
                           candidate.format().frame_size.width()),
      ResolutionSet::Point(default_height, default_width));
  distance_vector->push_back(resolution_distance);

  // Prefer a frame rate close to the default.
  double frame_rate_distance =
      candidate.format().frame_rate == default_frame_rate
          ? 0.0
          : NumericConstraintFitnessDistance(candidate.format().frame_rate,
                                             default_frame_rate);
  distance_vector->push_back(frame_rate_distance);
}

}  // namespace

blink::WebString GetVideoKindForFormat(
    const media::VideoCaptureFormat& format) {
  return (format.pixel_format == media::PIXEL_FORMAT_Y16)
             ? blink::WebString::FromASCII(kVideoKindDepth)
             : blink::WebString::FromASCII(kVideoKindColor);
}

blink::WebMediaStreamTrack::FacingMode ToWebFacingMode(
    media::VideoFacingMode video_facing) {
  switch (video_facing) {
    case media::MEDIA_VIDEO_FACING_NONE:
      return blink::WebMediaStreamTrack::FacingMode::kNone;
    case media::MEDIA_VIDEO_FACING_USER:
      return blink::WebMediaStreamTrack::FacingMode::kUser;
    case media::MEDIA_VIDEO_FACING_ENVIRONMENT:
      return blink::WebMediaStreamTrack::FacingMode::kEnvironment;
    default:
      return blink::WebMediaStreamTrack::FacingMode::kNone;
  }
}

VideoDeviceCaptureCapabilities::VideoDeviceCaptureCapabilities() = default;
VideoDeviceCaptureCapabilities::VideoDeviceCaptureCapabilities(
    VideoDeviceCaptureCapabilities&& other) = default;
VideoDeviceCaptureCapabilities::~VideoDeviceCaptureCapabilities() = default;
VideoDeviceCaptureCapabilities& VideoDeviceCaptureCapabilities::operator=(
    VideoDeviceCaptureCapabilities&& other) = default;

VideoCaptureSettings SelectSettingsVideoDeviceCapture(
    const VideoDeviceCaptureCapabilities& capabilities,
    const blink::WebMediaConstraints& constraints,
    int default_width,
    int default_height,
    double default_frame_rate) {
  DCHECK_GT(default_width, 0);
  DCHECK_GT(default_height, 0);
  DCHECK_GE(default_frame_rate, 0.0);
  // This function works only if infinity is defined for the double type.
  static_assert(std::numeric_limits<double>::has_infinity, "Requires infinity");

  // A distance vector contains:
  // a) For each advanced constraint set, a 0/1 value indicating if the
  //    candidate satisfies the corresponding constraint set.
  // b) Fitness distance for the candidate based on support for the ideal values
  //    of the basic constraint set.
  // c) A custom distance value based on how "well" a candidate satisfies each
  //    constraint set, including basic and advanced sets.
  // d) Native fitness distance for the candidate based on support for the
  //    ideal values of the basic constraint set using native values for
  //    settings that can support a range of values.
  // e) A custom distance value based on how close the candidate is to default
  //    settings.
  // Parts (a) and (b) are according to spec. Parts (c) to (e) are
  // implementation specific and used to break ties.
  DistanceVector best_distance(2 * constraints.Advanced().size() + 3 +
                               kNumDefaultDistanceEntries);
  std::fill(best_distance.begin(), best_distance.end(), HUGE_VAL);
  VideoCaptureSettings result;
  const char* failed_constraint_name = result.failed_constraint_name();

  for (auto& device : capabilities.device_capabilities) {
    double basic_device_distance = DeviceSourceDistance(
        device->device_id, device->group_id, device->facing_mode,
        constraints.Basic(), &failed_constraint_name);
    if (!std::isfinite(basic_device_distance))
      continue;

    for (auto& format : device->formats) {
      ConstrainedFormat constrained_format(format);
      double basic_format_distance =
          FormatSourceDistance(format, constrained_format, constraints.Basic(),
                               &failed_constraint_name);
      if (!std::isfinite(basic_format_distance))
        continue;
      if (!constrained_format.ApplyConstraintSet(constraints.Basic()))
        continue;

      for (auto& power_line_frequency : capabilities.power_line_capabilities) {
        double basic_power_line_frequency_distance =
            PowerLineFrequencyConstraintSourceDistance(
                constraints.Basic().goog_power_line_frequency,
                power_line_frequency, &failed_constraint_name);
        if (!std::isfinite(basic_power_line_frequency_distance))
          continue;

        for (auto& noise_reduction :
             capabilities.noise_reduction_capabilities) {
          double basic_noise_reduction_distance =
              NoiseReductionConstraintSourceDistance(
                  constraints.Basic().goog_noise_reduction, noise_reduction,
                  &failed_constraint_name);
          if (!std::isfinite(basic_noise_reduction_distance))
            continue;

          // The candidate satisfies the basic constraint set.
          double candidate_basic_custom_distance =
              basic_device_distance + basic_format_distance +
              basic_power_line_frequency_distance +
              basic_noise_reduction_distance;
          DCHECK(std::isfinite(candidate_basic_custom_distance));

          // Temporary vector to save custom distances for advanced constraints.
          // Custom distances must be added to the candidate distance vector
          // after all the spec-mandated values.
          DistanceVector advanced_custom_distance_vector;
          Candidate candidate(device->device_id, device->group_id, format,
                              device->facing_mode, power_line_frequency,
                              noise_reduction);
          DistanceVector candidate_distance_vector;
          // First criteria for valid candidates is satisfaction of advanced
          // constraint sets.
          for (const auto& advanced_set : constraints.Advanced()) {
            double custom_distance = CandidateSourceDistance(
                candidate, constrained_format, advanced_set, nullptr);
            if (!constrained_format.ApplyConstraintSet(advanced_set))
              custom_distance = HUGE_VAL;
            advanced_custom_distance_vector.push_back(custom_distance);
            double spec_distance = std::isfinite(custom_distance) ? 0 : 1;
            candidate_distance_vector.push_back(spec_distance);
          }

          // Second criterion is fitness distance.
          candidate_distance_vector.push_back(CandidateFitnessDistance(
              candidate, constrained_format, constraints.Basic()));

          // Third criteria are custom distances to constraint sets.
          candidate_distance_vector.push_back(candidate_basic_custom_distance);
          std::copy(advanced_custom_distance_vector.begin(),
                    advanced_custom_distance_vector.end(),
                    std::back_inserter(candidate_distance_vector));

          // Fourth criteria is native fitness distance.
          candidate_distance_vector.push_back(CandidateNativeFitnessDistance(
              constrained_format, constraints.Basic()));

          // Final criteria are custom distances to default settings.
          AppendDistanceFromDefault(candidate, capabilities, default_width,
                                    default_height, default_frame_rate,
                                    &candidate_distance_vector);

          DCHECK_EQ(best_distance.size(), candidate_distance_vector.size());
          if (candidate_distance_vector < best_distance) {
            best_distance = candidate_distance_vector;
            result = ComputeVideoDeviceCaptureSettings(
                candidate, constrained_format, constraints.Basic());
          }
        }
      }
    }
  }

  if (!result.HasValue())
    return VideoCaptureSettings(failed_constraint_name);

  return result;
}

}  // namespace content
