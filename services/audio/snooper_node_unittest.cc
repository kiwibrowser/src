// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/snooper_node.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/test/test_mock_time_task_runner.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_parameters.h"
#include "media/base/channel_layout.h"
#include "services/audio/test/fake_consumer.h"
#include "services/audio/test/fake_group_member.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace audio {
namespace {

// Used to test whether the output AudioBuses have had all their values set to
// something finite.
constexpr float kInvalidAudioSample = std::numeric_limits<float>::infinity();

// The tones the source should generate into the left and right channels.
constexpr double kLeftChannelFrequency = 500.0;
constexpr double kRightChannelFrequency = 1200.0;
constexpr double kSourceVolume = 0.5;

// The duration of the audio that flows through the SnooperNode for each test.
constexpr base::TimeDelta kTestDuration = base::TimeDelta::FromSeconds(10);

// The amount of time in the future where the inbound audio is being recorded.
// This simulates an audio output stream that has rendered audio that is
// scheduled to be played out in the near future.
constexpr base::TimeDelta kInputAdvanceTime =
    base::TimeDelta::FromMilliseconds(2);

// The amount of time in the past from which outbound audio is being rendered.
// This simulates the loopback stream's "capture from the recent past" mode-of-
// operation.
constexpr base::TimeDelta kOutputDelayTime =
    base::TimeDelta::FromMilliseconds(20);

// Test parameters.
struct InputAndOutputParams {
  media::AudioParameters input;
  media::AudioParameters output;
};

// Helper so that gtest can produce useful logging of the test parameters.
std::ostream& operator<<(std::ostream& out,
                         const InputAndOutputParams& test_params) {
  return out << "{input=" << test_params.input.AsHumanReadableString()
             << ", output=" << test_params.output.AsHumanReadableString()
             << "}";
}

class SnooperNodeTest : public testing::TestWithParam<InputAndOutputParams> {
 public:
  // Positions is a vector containing positions (in terms of frames elasped
  // since the first) where an AudioBus input or output task should not be
  // scheduled. This simulates missing input or skipped consumption.
  using Positions = std::vector<int>;

  SnooperNodeTest() = default;
  ~SnooperNodeTest() override = default;

  const media::AudioParameters& input_params() const {
    return GetParam().input;
  }
  const media::AudioParameters& output_params() const {
    return GetParam().output;
  }

  FakeGroupMember* group_member() { return &*group_member_; }
  SnooperNode* node() { return &*node_; }
  FakeConsumer* consumer() { return &*consumer_; }

  void SetUp() override {
    // Initialize a test clock and task runner. The starting TimeTicks value is
    // "huge" to ensure time calculations are being tested for overflow cases.
    task_runner_ = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
        base::Time(), base::TimeTicks() +
                          base::TimeDelta::FromMicroseconds(INT64_C(1) << 62));
  }

  void CreateNewPipeline() {
    group_member_.emplace(base::UnguessableToken(), input_params());
    group_member_->SetChannelTone(0, kLeftChannelFrequency);
    if (input_params().channels() > 1) {
      // Set the right channel to kRightChannelFrequency unless the test
      // parameters call for stereo→mono channel down-mixing. In that case, just
      // use kLeftChannelFrequency again, and the test will confirm that the
      // amplitude is 2X because there were two source channels mixed into one.
      group_member_->SetChannelTone(1, output_params().channels() == 1
                                           ? kLeftChannelFrequency
                                           : kRightChannelFrequency);
    }
    group_member_->SetVolume(kSourceVolume);

    node_.emplace(input_params(), output_params());
    group_member_->StartSnooping(node());

    consumer_.emplace(output_params().channels(),
                      output_params().sample_rate());
  }

  void ScheduleInputTasks(double skew, const Positions& drop_positions) {
    CHECK(std::is_sorted(drop_positions.begin(), drop_positions.end()));

    const base::TimeTicks start_time =
        task_runner_->NowTicks() + kInputAdvanceTime;
    const base::TimeTicks end_time = start_time + kTestDuration;
    const double time_step = skew / input_params().sample_rate();

    auto drop_it = drop_positions.begin();
    for (int position = 0;; position += input_params().frames_per_buffer()) {
      // If a drop point has been reached, do not schedule an input task.
      if (drop_it != drop_positions.end() && *drop_it == position) {
        ++drop_it;
        continue;
      }

      const base::TimeTicks next_time =
          start_time + base::TimeDelta::FromSecondsD(position * time_step);
      if (next_time >= end_time) {
        break;
      }

      // FakeGroupMember pushes audio into the SnooperNode.
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&FakeGroupMember::RenderMoreAudio,
                         base::Unretained(group_member()), next_time),
          next_time - start_time);
    }
  }

  void ScheduleOutputTasks(double skew, const Positions& skip_positions) {
    CHECK(std::is_sorted(skip_positions.begin(), skip_positions.end()));

    const base::TimeTicks start_time =
        task_runner_->NowTicks() - kOutputDelayTime;
    const base::TimeTicks end_time = start_time + kTestDuration;
    const double time_step = skew / output_params().sample_rate();

    auto skip_it = skip_positions.begin();
    for (int position = 0;; position += output_params().frames_per_buffer()) {
      // If a skip point has been reached, do not schedule an output task.
      if (skip_it != skip_positions.end() && *skip_it == position) {
        ++skip_it;
        continue;
      }

      const base::TimeTicks next_time =
          start_time + base::TimeDelta::FromSecondsD(position * time_step);
      if (next_time >= end_time) {
        break;
      }

      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(
              [](SnooperNodeTest* test, base::TimeTicks output_time) {
                // Have the SnooperNode render more output data. Before that,
                // assign invalid sample values to the AudioBus. Then, after the
                // Render() call, confirm that every sample was overwritten in
                // the output AudioBus.
                const auto bus = media::AudioBus::Create(test->output_params());
                for (int ch = 0; ch < bus->channels(); ++ch) {
                  std::fill_n(bus->channel(ch), bus->frames(),
                              kInvalidAudioSample);
                }
                test->node_->Render(output_time, bus.get());
                for (int ch = 0; ch < bus->channels(); ++ch) {
                  EXPECT_FALSE(std::any_of(
                      bus->channel(ch), bus->channel(ch) + bus->frames(),
                      [](float x) { return x == kInvalidAudioSample; }))
                      << " at output_time=" << output_time << ", ch=" << ch;
                }

                // Pass the output to the consumer to store for later analysis.
                test->consumer_->Consume(*bus);
              },
              this, next_time),
          next_time - start_time);
    }
  }

  void RunAllPendingTasks() { task_runner_->FastForwardUntilNoTasksRemain(); }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::Optional<FakeGroupMember> group_member_;
  base::Optional<SnooperNode> node_;
  base::Optional<FakeConsumer> consumer_;
};

// Performance of this test on debug builds is abysmal. So, only run it on
// optimized builds.
// TODO(crbug.com/842428): Analyze why only Windows debug test runs have this
// problem and re-enable test.
#ifdef NDEBUG
#define MAYBE_ContinuousAudioFlowAdaptsToSkew ContinuousAudioFlowAdaptsToSkew
#else
#define MAYBE_ContinuousAudioFlowAdaptsToSkew \
  DISABLED_ContinuousAudioFlowAdaptsToSkew
#endif
TEST_P(SnooperNodeTest, MAYBE_ContinuousAudioFlowAdaptsToSkew) {
  // Note: A skew of 0.999 or 1.001 is very extreme. This is like saying the
  // clocks drift 1 ms for every second that goes by. If the implementation can
  // handle that, it's very likely to do a perfect job in-the-wild.
  for (double input_skew = 0.999; input_skew <= 1.001; input_skew += 0.0005) {
    for (double output_skew = 0.999; output_skew <= 1.001;
         output_skew += 0.0005) {
      SCOPED_TRACE(testing::Message() << "input_skew=" << input_skew
                                      << ", output_skew=" << output_skew);

      // Set up the components, schedule all audio generation and consumption
      // tasks, and then run them.
      CreateNewPipeline();
      ScheduleInputTasks(input_skew, Positions());
      ScheduleOutputTasks(output_skew, Positions());
      RunAllPendingTasks();

      // All rendering for points-in-time before the audio from the source was
      // first recorded should be silence.
      const double expected_end_of_silence_position =
          ((input_skew * kInputAdvanceTime.InSecondsF()) +
           (output_skew * kOutputDelayTime.InSecondsF())) *
          output_params().sample_rate();
      const double frames_in_one_millisecond =
          output_params().sample_rate() / 1000.0;
      EXPECT_NEAR(expected_end_of_silence_position,
                  consumer()->FindEndOfSilence(0, 0),
                  frames_in_one_millisecond);
      if (output_params().channels() > 1) {
        EXPECT_NEAR(expected_end_of_silence_position,
                    consumer()->FindEndOfSilence(1, 0),
                    frames_in_one_millisecond);
      }

      // Analyze the recording in several places for the expected tones.
      constexpr int kNumToneChecks = 16;
      for (int i = 1; i <= kNumToneChecks; ++i) {
        const int end_frame =
            consumer()->GetRecordedFrameCount() * i / kNumToneChecks;
        SCOPED_TRACE(testing::Message() << "end_frame=" << end_frame);
        EXPECT_NEAR(
            kSourceVolume,
            consumer()->ComputeAmplitudeAt(0, kLeftChannelFrequency, end_frame),
            0.01);
        if (output_params().channels() > 1) {
          const double freq = input_params().channels() == 1
                                  ? kLeftChannelFrequency
                                  : kRightChannelFrequency;
          EXPECT_NEAR(kSourceVolume,
                      consumer()->ComputeAmplitudeAt(1, freq, end_frame), 0.01);
        }
      }

      if (HasFailure()) {
        return;
      }
    }
  }
}

// Performance of this test on debug builds is abysmal. So, only run it on
// optimized builds.
// TODO(crbug.com/842428): Analyze why only Windows debug test runs have this
// problem and re-enable test.
#ifdef NDEBUG
#define MAYBE_HandlesMissingInput HandlesMissingInput
#else
#define MAYBE_HandlesMissingInput DISABLED_HandlesMissingInput
#endif
TEST_P(SnooperNodeTest, MAYBE_HandlesMissingInput) {
  // Compute drops to occur once per second for 1/4 second duration. Each drop
  // position must be aligned to input_params().frames_per_buffer() for the
  // heuristics in ScheduleInputTasks() to process these drop positions
  // correctly.
  Positions drop_positions;
  const int input_frames_in_one_second = input_params().sample_rate();
  const int input_frames_in_a_quarter_second = input_frames_in_one_second / 4;
  int unaligned_drop_position = input_frames_in_one_second;
  for (int gap = 0; gap < 5; ++gap) {
    const int aligned_drop_position =
        (unaligned_drop_position / input_params().frames_per_buffer()) *
        input_params().frames_per_buffer();
    const int end_position =
        aligned_drop_position + input_frames_in_a_quarter_second;
    for (int i = 0;; ++i) {
      const int next_drop_position =
          aligned_drop_position + i * input_params().frames_per_buffer();
      if (next_drop_position >= end_position) {
        break;
      }
      drop_positions.push_back(next_drop_position);
    }
    unaligned_drop_position += input_frames_in_one_second;
  }

  // Set up the components, schedule all audio generation and consumption tasks,
  // and then run them.
  CreateNewPipeline();
  ScheduleInputTasks(1.0, drop_positions);
  ScheduleOutputTasks(1.0, Positions());
  RunAllPendingTasks();

  // Check that there is silence in the drop positions, and that tones are
  // present around the silent sections. The ranges are adjusted to be 20 ms
  // away from the exact begin/end positions to account for a reasonable amount
  // of variance in due to the input buffer intervals.
  const int output_frames_in_one_second = output_params().sample_rate();
  const int output_frames_in_a_quarter_second = output_frames_in_one_second / 4;
  const int output_frames_in_20_milliseconds =
      output_frames_in_one_second * 20 / 1000;
  int output_silence_position =
      ((kInputAdvanceTime + kOutputDelayTime).InSecondsF() + 1.0) *
      output_params().sample_rate();
  for (int gap = 0; gap < 5; ++gap) {
    SCOPED_TRACE(testing::Message() << "gap=" << gap);

    // Just before the drop, there should be a tone.
    const int position_a_little_before_silence_begins =
        output_silence_position - output_frames_in_20_milliseconds;
    EXPECT_NEAR(
        kSourceVolume,
        consumer()->ComputeAmplitudeAt(0, kLeftChannelFrequency,
                                       position_a_little_before_silence_begins),
        0.01);

    // There should be silence during the drop.
    const int position_a_little_after_silence_begins =
        output_silence_position + output_frames_in_20_milliseconds;
    const int position_a_little_before_silence_ends =
        position_a_little_after_silence_begins +
        output_frames_in_a_quarter_second -
        2 * output_frames_in_20_milliseconds;
    EXPECT_TRUE(
        consumer()->IsSilentInRange(0, position_a_little_after_silence_begins,
                                    position_a_little_before_silence_ends));

    // Finally, the tone should be back after the drop.
    const int position_a_little_after_silence_ends =
        position_a_little_before_silence_ends +
        2 * output_frames_in_20_milliseconds;
    EXPECT_NEAR(
        kSourceVolume,
        consumer()->ComputeAmplitudeAt(0, kLeftChannelFrequency,
                                       position_a_little_after_silence_ends),
        0.01);
    output_silence_position += output_frames_in_one_second;
  }
}

// TODO: TEST_P(SnooperNodeTest, HandlesSkippingOutput) {}

InputAndOutputParams MakeParams(media::ChannelLayout input_channel_layout,
                                int input_sample_rate,
                                int input_frames_per_buffer,
                                media::ChannelLayout output_channel_layout,
                                int output_sample_rate,
                                int output_frames_per_buffer) {
  return InputAndOutputParams{
      media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                             input_channel_layout, input_sample_rate,
                             input_frames_per_buffer),
      media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                             output_channel_layout, output_sample_rate,
                             output_frames_per_buffer)};
}

INSTANTIATE_TEST_CASE_P(,
                        SnooperNodeTest,
                        testing::Values(MakeParams(media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   480,
                                                   media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   480),
                                        MakeParams(media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   64,
                                                   media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   480),
                                        MakeParams(media::CHANNEL_LAYOUT_STEREO,
                                                   44100,
                                                   64,
                                                   media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   480),
                                        MakeParams(media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   512,
                                                   media::CHANNEL_LAYOUT_STEREO,
                                                   44100,
                                                   441),
                                        MakeParams(media::CHANNEL_LAYOUT_MONO,
                                                   8000,
                                                   64,
                                                   media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   480),
                                        MakeParams(media::CHANNEL_LAYOUT_STEREO,
                                                   48000,
                                                   480,
                                                   media::CHANNEL_LAYOUT_MONO,
                                                   8000,
                                                   80)));

}  // namespace
}  // namespace audio
