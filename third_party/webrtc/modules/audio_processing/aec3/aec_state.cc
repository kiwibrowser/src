/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_processing/aec3/aec_state.h"

#include <math.h>

#include <numeric>
#include <vector>

#include "api/array_view.h"
#include "modules/audio_processing/logging/apm_data_dumper.h"
#include "rtc_base/atomicops.h"
#include "rtc_base/checks.h"
#include "system_wrappers/include/field_trial.h"

namespace webrtc {
namespace {

bool EnableTransparentMode() {
  return !field_trial::IsEnabled("WebRTC-Aec3TransparentModeKillSwitch");
}

bool EnableStationaryRenderImprovements() {
  return !field_trial::IsEnabled(
      "WebRTC-Aec3StationaryRenderImprovementsKillSwitch");
}

float ComputeGainRampupIncrease(const EchoCanceller3Config& config) {
  const auto& c = config.echo_removal_control.gain_rampup;
  return powf(1.f / c.first_non_zero_gain, 1.f / c.non_zero_gain_blocks);
}

constexpr size_t kBlocksSinceConvergencedFilterInit = 10000;
constexpr size_t kBlocksSinceConsistentEstimateInit = 10000;

}  // namespace

int AecState::instance_count_ = 0;

AecState::AecState(const EchoCanceller3Config& config)
    : data_dumper_(
          new ApmDataDumper(rtc::AtomicOps::Increment(&instance_count_))),
      config_(config),
      allow_transparent_mode_(EnableTransparentMode()),
      use_stationary_properties_(
          EnableStationaryRenderImprovements() &&
          config_.echo_audibility.use_stationary_properties),
      erle_estimator_(config.erle.min, config.erle.max_l, config.erle.max_h),
      max_render_(config_.filter.main.length_blocks, 0.f),
      reverb_decay_(fabsf(config_.ep_strength.default_len)),
      gain_rampup_increase_(ComputeGainRampupIncrease(config_)),
      suppression_gain_limiter_(config_),
      filter_analyzer_(config_),
      blocks_since_converged_filter_(kBlocksSinceConvergencedFilterInit),
      active_blocks_since_consistent_filter_estimate_(
          kBlocksSinceConsistentEstimateInit) {}

AecState::~AecState() = default;

void AecState::HandleEchoPathChange(
    const EchoPathVariability& echo_path_variability) {
  const auto full_reset = [&]() {
    filter_analyzer_.Reset();
    blocks_since_last_saturation_ = 0;
    usable_linear_estimate_ = false;
    capture_signal_saturation_ = false;
    echo_saturation_ = false;
    std::fill(max_render_.begin(), max_render_.end(), 0.f);
    blocks_with_proper_filter_adaptation_ = 0;
    blocks_since_reset_ = 0;
    filter_has_had_time_to_converge_ = false;
    render_received_ = false;
    blocks_with_active_render_ = 0;
    initial_state_ = true;
    suppression_gain_limiter_.Reset();
    blocks_since_converged_filter_ = kBlocksSinceConvergencedFilterInit;
    diverged_blocks_ = 0;
  };

  // TODO(peah): Refine the reset scheme according to the type of gain and
  // delay adjustment.
  if (echo_path_variability.gain_change) {
    full_reset();
  }

  if (echo_path_variability.delay_change !=
      EchoPathVariability::DelayAdjustment::kBufferReadjustment) {
    full_reset();
  } else if (echo_path_variability.delay_change !=
             EchoPathVariability::DelayAdjustment::kBufferFlush) {
    full_reset();
  } else if (echo_path_variability.delay_change !=
             EchoPathVariability::DelayAdjustment::kDelayReset) {
    full_reset();
  } else if (echo_path_variability.delay_change !=
             EchoPathVariability::DelayAdjustment::kNewDetectedDelay) {
    full_reset();
  } else if (echo_path_variability.gain_change) {
    blocks_since_reset_ = kNumBlocksPerSecond;
  }
}

void AecState::Update(
    const rtc::Optional<DelayEstimate>& external_delay,
    const std::vector<std::array<float, kFftLengthBy2Plus1>>&
        adaptive_filter_frequency_response,
    const std::vector<float>& adaptive_filter_impulse_response,
    bool converged_filter,
    bool diverged_filter,
    const RenderBuffer& render_buffer,
    const std::array<float, kFftLengthBy2Plus1>& E2_main,
    const std::array<float, kFftLengthBy2Plus1>& Y2,
    const std::array<float, kBlockSize>& s) {
  // Analyze the filter and compute the delays.
  filter_analyzer_.Update(adaptive_filter_impulse_response, render_buffer);
  filter_delay_blocks_ = filter_analyzer_.DelayBlocks();

  if (filter_analyzer_.Consistent()) {
    internal_delay_ = filter_analyzer_.DelayBlocks();
  } else {
    internal_delay_ = rtc::nullopt;
  }

  external_delay_seen_ = external_delay_seen_ || external_delay;

  const std::vector<float>& x = render_buffer.Block(-filter_delay_blocks_)[0];

  // Update counters.
  ++capture_block_counter_;
  ++blocks_since_reset_;
  const bool active_render_block = DetectActiveRender(x);
  blocks_with_active_render_ += active_render_block ? 1 : 0;
  blocks_with_proper_filter_adaptation_ +=
      active_render_block && !SaturatedCapture() ? 1 : 0;

  // Update the limit on the echo suppression after an echo path change to avoid
  // an initial echo burst.
  suppression_gain_limiter_.Update(render_buffer.GetRenderActivity(),
                                   transparent_mode_);

  if (UseStationaryProperties()) {
    // Update the echo audibility evaluator.
    echo_audibility_.Update(
        render_buffer, FilterDelayBlocks(), external_delay_seen_,
        config_.ep_strength.reverb_based_on_render ? ReverbDecay() : 0.f);
  }

  // Update the ERL and ERLE measures.
  if (blocks_since_reset_ >= 2 * kNumBlocksPerSecond) {
    const auto& X2 = render_buffer.Spectrum(filter_delay_blocks_);
    erle_estimator_.Update(X2, Y2, E2_main, converged_filter);
    if (converged_filter) {
      erl_estimator_.Update(X2, Y2);
    }
  }

  // Detect and flag echo saturation.
  // TODO(peah): Add the delay in this computation to ensure that the render and
  // capture signals are properly aligned.
  if (config_.ep_strength.echo_can_saturate) {
    echo_saturation_ = DetectEchoSaturation(x, EchoPathGain());
  }

  bool filter_has_had_time_to_converge =
      blocks_with_proper_filter_adaptation_ >= 1.5f * kNumBlocksPerSecond;

  if (!filter_should_have_converged_) {
    filter_should_have_converged_ =
        blocks_with_proper_filter_adaptation_ > 6 * kNumBlocksPerSecond;
  }

  // Flag whether the initial state is still active.
  initial_state_ =
      blocks_with_proper_filter_adaptation_ < 5 * kNumBlocksPerSecond;

  // Update counters for the filter divergence and convergence.
  diverged_blocks_ = diverged_filter ? diverged_blocks_ + 1 : 0;
  if (diverged_blocks_ >= 60) {
    blocks_since_converged_filter_ = kBlocksSinceConvergencedFilterInit;
  } else {
    blocks_since_converged_filter_ =
        converged_filter ? 0 : blocks_since_converged_filter_ + 1;
  }
  if (converged_filter) {
    active_blocks_since_converged_filter_ = 0;
  } else if (active_render_block) {
    ++active_blocks_since_converged_filter_;
  }

  bool recently_converged_filter =
      blocks_since_converged_filter_ < 60 * kNumBlocksPerSecond;

  if (blocks_since_converged_filter_ > 20 * kNumBlocksPerSecond) {
    converged_filter_count_ = 0;
  } else if (converged_filter) {
    ++converged_filter_count_;
  }
  if (converged_filter_count_ > 50) {
    finite_erl_ = true;
  }

  if (filter_analyzer_.Consistent() && filter_delay_blocks_ < 5) {
    consistent_filter_seen_ = true;
    active_blocks_since_consistent_filter_estimate_ = 0;
  } else if (active_render_block) {
    ++active_blocks_since_consistent_filter_estimate_;
  }

  bool consistent_filter_estimate_not_seen;
  if (!consistent_filter_seen_) {
    consistent_filter_estimate_not_seen =
        capture_block_counter_ > 5 * kNumBlocksPerSecond;
  } else {
    consistent_filter_estimate_not_seen =
        active_blocks_since_consistent_filter_estimate_ >
        30 * kNumBlocksPerSecond;
  }

  converged_filter_seen_ = converged_filter_seen_ || converged_filter;

  // If no filter convergence is seen for a long time, reset the estimated
  // properties of the echo path.
  if (active_blocks_since_converged_filter_ > 60 * kNumBlocksPerSecond) {
    converged_filter_seen_ = false;
    finite_erl_ = false;
  }

  // After an amount of active render samples for which an echo should have been
  // detected in the capture signal if the ERL was not infinite, flag that a
  // transparent mode should be entered.
  transparent_mode_ = !config_.ep_strength.bounded_erl && !finite_erl_;
  transparent_mode_ =
      transparent_mode_ &&
      (consistent_filter_estimate_not_seen || !converged_filter_seen_);
  transparent_mode_ = transparent_mode_ && filter_should_have_converged_;
  transparent_mode_ = transparent_mode_ && allow_transparent_mode_;

  usable_linear_estimate_ = !echo_saturation_;
  usable_linear_estimate_ =
      usable_linear_estimate_ && filter_has_had_time_to_converge;
  usable_linear_estimate_ =
      usable_linear_estimate_ && recently_converged_filter;
  usable_linear_estimate_ = usable_linear_estimate_ && !diverged_filter;
  usable_linear_estimate_ = usable_linear_estimate_ && external_delay;

  use_linear_filter_output_ = usable_linear_estimate_ && !TransparentMode();

  UpdateReverb(adaptive_filter_impulse_response);

  data_dumper_->DumpRaw("aec3_erle", Erle());
  data_dumper_->DumpRaw("aec3_erle_onset", erle_estimator_.ErleOnsets());
  data_dumper_->DumpRaw("aec3_erl", Erl());
  data_dumper_->DumpRaw("aec3_erle_time_domain", ErleTimeDomain());
  data_dumper_->DumpRaw("aec3_erl_time_domain", ErlTimeDomain());
  data_dumper_->DumpRaw("aec3_usable_linear_estimate", UsableLinearEstimate());
  data_dumper_->DumpRaw("aec3_transparent_mode", transparent_mode_);
  data_dumper_->DumpRaw("aec3_state_internal_delay",
                        internal_delay_ ? *internal_delay_ : -1);
  data_dumper_->DumpRaw("aec3_filter_delay", filter_analyzer_.DelayBlocks());

  data_dumper_->DumpRaw("aec3_consistent_filter",
                        filter_analyzer_.Consistent());
  data_dumper_->DumpRaw("aec3_suppression_gain_limit", SuppressionGainLimit());
  data_dumper_->DumpRaw("aec3_initial_state", InitialState());
  data_dumper_->DumpRaw("aec3_capture_saturation", SaturatedCapture());
  data_dumper_->DumpRaw("aec3_echo_saturation", echo_saturation_);
  data_dumper_->DumpRaw("aec3_converged_filter", converged_filter);
  data_dumper_->DumpRaw("aec3_diverged_filter", diverged_filter);

  data_dumper_->DumpRaw("aec3_external_delay_avaliable",
                        external_delay ? 1 : 0);
  data_dumper_->DumpRaw("aec3_consistent_filter_estimate_not_seen",
                        consistent_filter_estimate_not_seen);
  data_dumper_->DumpRaw("aec3_filter_should_have_converged",
                        filter_should_have_converged_);
  data_dumper_->DumpRaw("aec3_filter_has_had_time_to_converge",
                        filter_has_had_time_to_converge);
  data_dumper_->DumpRaw("aec3_recently_converged_filter",
                        recently_converged_filter);
  data_dumper_->DumpRaw("aec3_filter_tail_energy", GetFilterTailGain());
}

void AecState::UpdateReverb(const std::vector<float>& impulse_response) {
  // Echo tail estimation enabled if the below variable is set as negative.
  if (config_.ep_strength.default_len >= 0.f) {
    return;
  }

  if ((!(filter_delay_blocks_ && usable_linear_estimate_)) ||
      (filter_delay_blocks_ >
       static_cast<int>(config_.filter.main.length_blocks) - 4)) {
    return;
  }

  constexpr float kOneByFftLengthBy2 = 1.f / kFftLengthBy2;

  // Form the data to match against by squaring the impulse response
  // coefficients.
  std::array<float, GetTimeDomainLength(kMaxAdaptiveFilterLength)>
      matching_data_data;
  RTC_DCHECK_LE(GetTimeDomainLength(config_.filter.main.length_blocks),
                matching_data_data.size());
  rtc::ArrayView<float> matching_data(
      matching_data_data.data(),
      GetTimeDomainLength(config_.filter.main.length_blocks));
  std::transform(impulse_response.begin(), impulse_response.end(),
                 matching_data.begin(), [](float a) { return a * a; });

  if (current_reverb_decay_section_ < config_.filter.main.length_blocks) {
    // Update accumulated variables for the current filter section.

    const size_t start_index = current_reverb_decay_section_ * kFftLengthBy2;

    RTC_DCHECK_GT(matching_data.size(), start_index);
    RTC_DCHECK_GE(matching_data.size(), start_index + kFftLengthBy2);
    float section_energy =
        std::accumulate(matching_data.begin() + start_index,
                        matching_data.begin() + start_index + kFftLengthBy2,
                        0.f) *
        kOneByFftLengthBy2;

    section_energy = std::max(
        section_energy, 1e-32f);  // Regularization to avoid division by 0.

    RTC_DCHECK_LT(current_reverb_decay_section_, block_energies_.size());
    const float energy_ratio =
        block_energies_[current_reverb_decay_section_] / section_energy;

    main_filter_is_adapting_ = main_filter_is_adapting_ ||
                               (energy_ratio > 1.1f || energy_ratio < 0.9f);

    // Count consecutive number of "good" filter sections, where "good" means:
    // 1) energy is above noise floor.
    // 2) energy of current section has not changed too much from last check.
    if (!found_end_of_reverb_decay_ && section_energy > tail_energy_ &&
        !main_filter_is_adapting_) {
      ++num_reverb_decay_sections_next_;
    } else {
      found_end_of_reverb_decay_ = true;
    }

    block_energies_[current_reverb_decay_section_] = section_energy;

    if (num_reverb_decay_sections_ > 0) {
      // Linear regression of log squared magnitude of impulse response.
      for (size_t i = 0; i < kFftLengthBy2; i++) {
        auto fast_approx_log2f = [](const float in) {
          RTC_DCHECK_GT(in, .0f);
          // Read and interpret float as uint32_t and then cast to float.
          // This is done to extract the exponent (bits 30 - 23).
          // "Right shift" of the exponent is then performed by multiplying
          // with the constant (1/2^23). Finally, we subtract a constant to
          // remove the bias (https://en.wikipedia.org/wiki/Exponent_bias).
          union {
            float dummy;
            uint32_t a;
          } x = {in};
          float out = x.a;
          out *= 1.1920929e-7f;  // 1/2^23
          out -= 126.942695f;  // Remove bias.
          return out;
        };
        RTC_DCHECK_GT(matching_data.size(), start_index + i);
        float z = fast_approx_log2f(matching_data[start_index + i]);
        accumulated_nz_ += accumulated_count_ * z;
        ++accumulated_count_;
      }
    }

    num_reverb_decay_sections_ =
        num_reverb_decay_sections_ > 0 ? num_reverb_decay_sections_ - 1 : 0;
    ++current_reverb_decay_section_;

  } else {
    constexpr float kMaxDecay = 0.95f;  // ~1 sec min RT60.
    constexpr float kMinDecay = 0.02f;  // ~15 ms max RT60.

    // Accumulated variables throughout whole filter.

    // Solve for decay rate.

    float decay = reverb_decay_;

    if (accumulated_nn_ != 0.f) {
      const float exp_candidate = -accumulated_nz_ / accumulated_nn_;
      decay = powf(2.0f, -exp_candidate * kFftLengthBy2);
      decay = std::min(decay, kMaxDecay);
      decay = std::max(decay, kMinDecay);
    }

    // Filter tail energy (assumed to be noise).

    constexpr size_t kTailLength = kFftLength;
    constexpr float k1ByTailLength = 1.f / kTailLength;
    const size_t tail_index =
        GetTimeDomainLength(config_.filter.main.length_blocks) - kTailLength;

    RTC_DCHECK_GT(matching_data.size(), tail_index);
    tail_energy_ = std::accumulate(matching_data.begin() + tail_index,
                                   matching_data.end(), 0.f) *
                   k1ByTailLength;

    // Update length of decay.
    num_reverb_decay_sections_ = num_reverb_decay_sections_next_;
    num_reverb_decay_sections_next_ = 0;
    // Must have enough data (number of sections) in order
    // to estimate decay rate.
    if (num_reverb_decay_sections_ < 5) {
      num_reverb_decay_sections_ = 0;
    }

    const float N = num_reverb_decay_sections_ * kFftLengthBy2;
    accumulated_nz_ = 0.f;
    const float k1By12 = 1.f / 12.f;
    // Arithmetic sum $2 \sum_{i=0.5}^{(N-1)/2}i^2$ calculated directly.
    accumulated_nn_ = N * (N * N - 1.0f) * k1By12;
    accumulated_count_ = -N * 0.5f;
    // Linear regression approach assumes symmetric index around 0.
    accumulated_count_ += 0.5f;

    // Identify the peak index of the impulse response.
    const size_t peak_index = std::distance(
        matching_data.begin(),
        std::max_element(matching_data.begin(), matching_data.end()));

    current_reverb_decay_section_ = peak_index * kOneByFftLengthBy2 + 3;
    // Make sure we're not out of bounds.
    if (current_reverb_decay_section_ + 1 >=
        config_.filter.main.length_blocks) {
      current_reverb_decay_section_ = config_.filter.main.length_blocks;
    }
    size_t start_index = current_reverb_decay_section_ * kFftLengthBy2;
    float first_section_energy =
        std::accumulate(matching_data.begin() + start_index,
                        matching_data.begin() + start_index + kFftLengthBy2,
                        0.f) *
        kOneByFftLengthBy2;

    // To estimate the reverb decay, the energy of the first filter section
    // must be substantially larger than the last.
    // Also, the first filter section energy must not deviate too much
    // from the max peak.
    bool main_filter_has_reverb = first_section_energy > 4.f * tail_energy_;
    bool main_filter_is_sane = first_section_energy > 2.f * tail_energy_ &&
                               matching_data[peak_index] < 100.f;

    // Not detecting any decay, but tail is over noise - assume max decay.
    if (num_reverb_decay_sections_ == 0 && main_filter_is_sane &&
        main_filter_has_reverb) {
      decay = kMaxDecay;
    }

    if (!main_filter_is_adapting_ && main_filter_is_sane &&
        num_reverb_decay_sections_ > 0) {
      decay = std::max(.97f * reverb_decay_, decay);
      reverb_decay_ -= .1f * (reverb_decay_ - decay);
    }

    found_end_of_reverb_decay_ =
        !(main_filter_is_sane && main_filter_has_reverb);
    main_filter_is_adapting_ = false;
  }

  data_dumper_->DumpRaw("aec3_reverb_decay", reverb_decay_);
  data_dumper_->DumpRaw("aec3_reverb_tail_energy", tail_energy_);
  data_dumper_->DumpRaw("aec3_suppression_gain_limit", SuppressionGainLimit());
}

bool AecState::DetectActiveRender(rtc::ArrayView<const float> x) const {
  const float x_energy = std::inner_product(x.begin(), x.end(), x.begin(), 0.f);
  return x_energy > (config_.render_levels.active_render_limit *
                     config_.render_levels.active_render_limit) *
                        kFftLengthBy2;
}

bool AecState::DetectEchoSaturation(rtc::ArrayView<const float> x,
                                    float echo_path_gain) {
  RTC_DCHECK_LT(0, x.size());
  const float max_sample = fabs(*std::max_element(
      x.begin(), x.end(), [](float a, float b) { return a * a < b * b; }));

  // Set flag for potential presence of saturated echo
  const float kMargin = 10.f;
  float peak_echo_amplitude = max_sample * echo_path_gain * kMargin;
  if (SaturatedCapture() && peak_echo_amplitude > 32000) {
    blocks_since_last_saturation_ = 0;
  } else {
    ++blocks_since_last_saturation_;
  }

  return blocks_since_last_saturation_ < 5;
}

}  // namespace webrtc
