// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/test/trace_event_analyzer.h"
#include "base/time/default_tick_clock.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/test_launcher_utils.h"
#include "chrome/test/base/test_switches.h"
#include "chrome/test/base/tracing.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/switches.h"
#include "extensions/test/extension_test_message_listener.h"
#include "media/base/audio_bus.h"
#include "media/base/video_frame.h"
#include "media/cast/test/skewed_tick_clock.h"
#include "media/cast/test/utility/audio_utility.h"
#include "media/cast/test/utility/barcode.h"
#include "media/cast/test/utility/default_config.h"
#include "media/cast/test/utility/in_process_receiver.h"
#include "media/cast/test/utility/net_utility.h"
#include "media/cast/test/utility/standalone_cast_environment.h"
#include "media/cast/test/utility/udp_proxy.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"
#include "net/log/net_log_source.h"
#include "net/socket/udp_server_socket.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/gl/gl_switches.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace {

constexpr char kExtensionId[] = "ddchlicdkolnonkihahngkmmmjnjlkkf";

// Number of events to trim from the begining and end. These events don't
// contribute anything toward stable measurements: A brief moment of startup
// "jank" is acceptable, and shutdown may result in missing events (e.g., if
// streaming stops a few frames before capture stops).
constexpr size_t kTrimEvents = 24;  // 1 sec at 24fps, or 0.4 sec at 60 fps.

// Minimum number of events required for a reasonable analysis.
constexpr size_t kMinDataPoints = 100;  // 1 sec of audio, or ~5 sec at 24fps.

enum TestFlags {
  kUseGpu = 1 << 0,           // Only execute test if --enable-gpu was given
                              // on the command line.  This is required for
                              // tests that run on GPU.
  kSmallWindow = 1 << 2,      // Window size: 1 = 800x600, 0 = 2000x1000
  k24fps = 1 << 3,            // Use 24 fps video.
  k30fps = 1 << 4,            // Use 30 fps video.
  k60fps = 1 << 5,            // Use 60 fps video (captured at 30 fps).
  kProxyWifi = 1 << 6,        // Run UDP through UDPProxy wifi profile.
  kProxySlow = 1 << 7,        // Run UDP through UDPProxy slow profile.
  kProxyBad = 1 << 8,         // Run UDP through UDPProxy bad profile.
  kSlowClock = 1 << 9,        // Receiver clock is 10 seconds slow.
  kFastClock = 1 << 10,       // Receiver clock is 10 seconds fast.
  kAutoThrottling = 1 << 11,  // Use auto-resolution/framerate throttling.
};

// These are just for testing! Use cryptographically-secure random keys in
// production code!
static constexpr char kAesKey[16] = {0, 1, 2,  3,  4,  5,  6,  7,
                                     8, 9, 10, 11, 12, 13, 14, 15};
static constexpr char kAesIvMask[16] = {15, 14, 13, 12, 11, 10, 9, 8,
                                        7,  6,  5,  4,  3,  2,  1, 0};

media::cast::FrameReceiverConfig WithAesKeyAndIvSet(
    const media::cast::FrameReceiverConfig& config) {
  media::cast::FrameReceiverConfig result = config;
  result.aes_key = std::string(kAesKey, kAesKey + sizeof(kAesKey));
  result.aes_iv_mask = std::string(kAesIvMask, kAesIvMask + sizeof(kAesIvMask));
  return result;
}

class SkewedCastEnvironment : public media::cast::StandaloneCastEnvironment {
 public:
  explicit SkewedCastEnvironment(const base::TimeDelta& delta)
      : StandaloneCastEnvironment(),
        skewed_clock_(base::DefaultTickClock::GetInstance()) {
    // If testing with a receiver clock that is ahead or behind the sender
    // clock, fake a clock that is offset and also ticks at a rate of 50 parts
    // per million faster or slower than the local sender's clock. This is the
    // worst-case scenario for skew in-the-wild.
    if (!delta.is_zero()) {
      const double skew = delta < base::TimeDelta() ? 0.999950 : 1.000050;
      skewed_clock_.SetSkew(skew, delta);
    }
    clock_ = &skewed_clock_;
  }

 protected:
  ~SkewedCastEnvironment() override {}

 private:
  media::cast::test::SkewedTickClock skewed_clock_;
};

// We log one of these for each call to OnAudioFrame/OnVideoFrame.
struct TimeData {
  TimeData(uint16_t frame_no_, base::TimeTicks playout_time_)
      : frame_no(frame_no_), playout_time(playout_time_) {}
  // The unit here is video frames, for audio data there can be duplicates.
  // This was decoded from the actual audio/video data.
  uint16_t frame_no;
  // This is when we should play this data, according to the sender.
  base::TimeTicks playout_time;
};

// TODO(hubbe): Move to media/cast to use for offline log analysis.
class MeanAndError {
 public:
  explicit MeanAndError(const std::vector<double>& values) {
    double sum = 0.0;
    double sqr_sum = 0.0;
    num_values_ = values.size();
    if (num_values_ > 0) {
      for (size_t i = 0; i < num_values_; i++) {
        sum += values[i];
        sqr_sum += values[i] * values[i];
      }
      mean_ = sum / num_values_;
      std_dev_ =
          sqrt(std::max(0.0, num_values_ * sqr_sum - sum * sum)) / num_values_;
    } else {
      mean_ = NAN;
      std_dev_ = NAN;
    }
  }

  std::string AsString() const {
    return base::StringPrintf("%f,%f", mean_, std_dev_);
  }

  void Print(const std::string& measurement,
             const std::string& modifier,
             const std::string& trace,
             const std::string& unit) {
    if (num_values_ >= 20) {
      perf_test::PrintResultMeanAndError(measurement,
                                         modifier,
                                         trace,
                                         AsString(),
                                         unit,
                                         true);
    } else {
      LOG(ERROR) << "Not enough events (" << num_values_ << ") for "
                 << measurement << modifier << " " << trace;
    }
  }

 private:
  size_t num_values_;
  double mean_;
  double std_dev_;
};

// This function checks how smooth the data in |data| is.
// It computes the average error of deltas and the average delta.
// If data[x] == x * A + B, then this function returns zero.
// The unit is milliseconds.
static MeanAndError AnalyzeJitter(const std::vector<TimeData>& data) {
  CHECK_GT(data.size(), 1UL);
  VLOG(0) << "Jitter analysis on " << data.size() << " values.";
  std::vector<double> deltas;
  double sum = 0.0;
  for (size_t i = 1; i < data.size(); i++) {
    double delta =
        (data[i].playout_time - data[i - 1].playout_time).InMillisecondsF();
    deltas.push_back(delta);
    sum += delta;
  }
  double mean = sum / deltas.size();
  for (size_t i = 0; i < deltas.size(); i++) {
    deltas[i] = fabs(mean - deltas[i]);
  }

  return MeanAndError(deltas);
}

// An in-process Cast receiver that examines the audio/video frames being
// received and logs some data about each received audio/video frame.
class TestPatternReceiver : public media::cast::InProcessReceiver {
 public:
  explicit TestPatternReceiver(
      const scoped_refptr<media::cast::CastEnvironment>& cast_environment,
      const net::IPEndPoint& local_end_point)
      : InProcessReceiver(
            cast_environment,
            local_end_point,
            net::IPEndPoint(),
            WithAesKeyAndIvSet(media::cast::GetDefaultAudioReceiverConfig()),
            WithAesKeyAndIvSet(media::cast::GetDefaultVideoReceiverConfig())) {}

  typedef std::map<uint16_t, base::TimeTicks> TimeMap;

  // Build a map from frame ID (as encoded in the audio and video data)
  // to the rtp timestamp for that frame. Note that there will be multiple
  // audio frames which all have the same frame ID. When that happens we
  // want the minimum rtp timestamp, because that audio frame is supposed
  // to play at the same time that the corresponding image is presented.
  void MapFrameTimes(const std::vector<TimeData>& events, TimeMap* map) {
    for (size_t i = kTrimEvents; i < events.size() - kTrimEvents; i++) {
      base::TimeTicks& frame_tick = (*map)[events[i].frame_no];
      if (frame_tick.is_null()) {
        frame_tick = events[i].playout_time;
      } else {
        frame_tick = std::min(events[i].playout_time, frame_tick);
      }
    }
  }

  void Analyze(const std::string& name, const std::string& modifier) {
    // First, find the minimum rtp timestamp for each audio and video frame.
    // Note that the data encoded in the audio stream contains video frame
    // numbers. So in a 30-fps video stream, there will be 1/30s of "1", then
    // 1/30s of "2", etc.
    TimeMap audio_frame_times, video_frame_times;
    MapFrameTimes(audio_events_, &audio_frame_times);
    EXPECT_GE(audio_frame_times.size(), kMinDataPoints);
    MapFrameTimes(video_events_, &video_frame_times);
    EXPECT_GE(video_frame_times.size(), kMinDataPoints);
    std::vector<double> deltas;
    for (TimeMap::const_iterator i = audio_frame_times.begin();
         i != audio_frame_times.end();
         ++i) {
      TimeMap::const_iterator j = video_frame_times.find(i->first);
      if (j != video_frame_times.end()) {
        deltas.push_back((i->second - j->second).InMillisecondsF());
      }
    }
    EXPECT_GE(deltas.size(), kMinDataPoints);

    // Close to zero is better. (can be negative)
    MeanAndError(deltas).Print(name, modifier, "av_sync", "ms");
    // lower is better.
    AnalyzeJitter(audio_events_).Print(name, modifier, "audio_jitter", "ms");
    // lower is better.
    AnalyzeJitter(video_events_).Print(name, modifier, "video_jitter", "ms");

    // Mean resolution of video at receiver. Lower stddev is better, while the
    // mean should be something reasonable given the network constraints
    // (usually 480 lines or more). Note that this is the video resolution at
    // the receiver, but changes originate on the sender side.
    std::vector<double> slice_for_analysis;
    if (video_frame_lines_.size() > kTrimEvents * 2) {
      slice_for_analysis.reserve(video_frame_lines_.size() - kTrimEvents * 2);
      EXPECT_GE(slice_for_analysis.capacity(), kMinDataPoints);
      std::transform(video_frame_lines_.begin() + kTrimEvents,
                     video_frame_lines_.end() - kTrimEvents,
                     std::back_inserter(slice_for_analysis),
                     [](int lines) { return static_cast<double>(lines); });
    }
    MeanAndError(slice_for_analysis)
        .Print(name, modifier, "playout_resolution", "lines");

    // Number of resolution changes. Lower is better (and 1 is ideal). Zero
    // indicates a lack of data.
    int last_lines = -1;
    int change_count = 0;
    for (size_t i = kTrimEvents; i < video_frame_lines_.size() - kTrimEvents;
         ++i) {
      if (video_frame_lines_[i] != last_lines) {
        ++change_count;
        last_lines = video_frame_lines_[i];
      }
    }
    EXPECT_GT(change_count, 0);
    perf_test::PrintResult(name, modifier, "resolution_changes",
                           base::IntToString(change_count), "count", true);
  }

 private:
  // Invoked by InProcessReceiver for each received audio frame.
  void OnAudioFrame(std::unique_ptr<media::AudioBus> audio_frame,
                    const base::TimeTicks& playout_time,
                    bool is_continuous) override {
    CHECK(cast_env()->CurrentlyOn(media::cast::CastEnvironment::MAIN));

    if (audio_frame->frames() <= 0) {
      NOTREACHED() << "OnAudioFrame called with no samples?!?";
      return;
    }

    // Note: This is the number of the video frame that this audio belongs to.
    uint16_t frame_no;
    if (media::cast::DecodeTimestamp(audio_frame->channel(0),
                                     audio_frame->frames(),
                                     &frame_no)) {
      audio_events_.push_back(TimeData(frame_no, playout_time));
    } else {
      DVLOG(2) << "Failed to decode audio timestamp!";
    }
  }

  void OnVideoFrame(const scoped_refptr<media::VideoFrame>& video_frame,
                    const base::TimeTicks& playout_time,
                    bool is_continuous) override {
    CHECK(cast_env()->CurrentlyOn(media::cast::CastEnvironment::MAIN));

    TRACE_EVENT_INSTANT1("cast_perf_test", "VideoFramePlayout",
                         TRACE_EVENT_SCOPE_THREAD, "playout_time",
                         (playout_time - base::TimeTicks()).InMicroseconds());

    uint16_t frame_no;
    if (media::cast::test::DecodeBarcode(video_frame, &frame_no)) {
      video_events_.push_back(TimeData(frame_no, playout_time));
    } else {
      DVLOG(2) << "Failed to decode barcode!";
    }

    video_frame_lines_.push_back(video_frame->visible_rect().height());
  }

  std::vector<TimeData> audio_events_;
  std::vector<TimeData> video_events_;

  // The height (number of lines) of each video frame received.
  std::vector<int> video_frame_lines_;

  DISALLOW_COPY_AND_ASSIGN(TestPatternReceiver);
};

class CastV2PerformanceTest : public extensions::ExtensionApiTest,
                              public testing::WithParamInterface<int> {
 public:
  CastV2PerformanceTest() {}

  bool HasFlag(TestFlags flag) const {
    return (GetParam() & flag) == flag;
  }

  bool IsGpuAvailable() const {
    return base::CommandLine::ForCurrentProcess()->HasSwitch("enable-gpu");
  }

  std::string GetSuffixForTestFlags() {
    std::string suffix;
    if (HasFlag(kUseGpu))
      suffix += "_gpu";
    if (HasFlag(kSmallWindow))
      suffix += "_small";
    if (HasFlag(k24fps))
      suffix += "_24fps";
    if (HasFlag(k30fps))
      suffix += "_30fps";
    if (HasFlag(k60fps))
      suffix += "_60fps";
    if (HasFlag(kProxyWifi))
      suffix += "_wifi";
    if (HasFlag(kProxySlow))
      suffix += "_slowwifi";
    if (HasFlag(kProxyBad))
      suffix += "_bad";
    if (HasFlag(kSlowClock))
      suffix += "_slow";
    if (HasFlag(kFastClock))
      suffix += "_fast";
    if (HasFlag(kAutoThrottling))
      suffix += "_autothrottling";
    return suffix;
  }

  int getfps() {
    if (HasFlag(k24fps))
      return 24;
    if (HasFlag(k30fps))
      return 30;
    if (HasFlag(k60fps))
      return 60;
    NOTREACHED();
    return 0;
  }

  void SetUp() override {
    EnablePixelOutput();
    if (!HasFlag(kUseGpu))
      UseSoftwareCompositing();
    extensions::ExtensionApiTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Some of the tests may launch http requests through JSON or AJAX
    // which causes a security error (cross domain request) when the page
    // is loaded from the local file system ( file:// ). The following switch
    // fixes that error.
    command_line->AppendSwitch(switches::kAllowFileAccessFromFiles);

    if (HasFlag(kSmallWindow)) {
      command_line->AppendSwitchASCII(switches::kWindowSize, "800,600");
    } else {
      command_line->AppendSwitchASCII(switches::kWindowSize, "2000,1500");
    }

    if (!HasFlag(kUseGpu))
      command_line->AppendSwitch(switches::kDisableGpu);

    command_line->AppendSwitchASCII(
        extensions::switches::kWhitelistedExtensionID,
        kExtensionId);

    extensions::ExtensionApiTest::SetUpCommandLine(command_line);
  }

  void GetTraceEvents(trace_analyzer::TraceAnalyzer* analyzer,
                      const std::string& event_name,
                      trace_analyzer::TraceEventVector* events) {
    trace_analyzer::Query query =
        trace_analyzer::Query::EventNameIs(event_name) &&
        (trace_analyzer::Query::EventPhaseIs(TRACE_EVENT_PHASE_BEGIN) ||
         trace_analyzer::Query::EventPhaseIs(TRACE_EVENT_PHASE_ASYNC_BEGIN) ||
         trace_analyzer::Query::EventPhaseIs(TRACE_EVENT_PHASE_FLOW_BEGIN) ||
         trace_analyzer::Query::EventPhaseIs(TRACE_EVENT_PHASE_INSTANT) ||
         trace_analyzer::Query::EventPhaseIs(TRACE_EVENT_PHASE_COMPLETE));
    analyzer->FindEvents(query, events);
    VLOG(0) << "Retrieved " << events->size() << " events for: " << event_name;
  }

  // The key contains the name of the argument and the argument.
  typedef std::pair<std::string, double> EventMapKey;
  typedef std::map<EventMapKey, const trace_analyzer::TraceEvent*> EventMap;

  // Make events findable by their arguments, for instance, if an
  // event has a "timestamp": 238724 argument, the map will contain
  // pair<"timestamp", 238724> -> &event.  All arguments are indexed.
  void IndexEvents(trace_analyzer::TraceAnalyzer* analyzer,
                   const std::string& event_name,
                   EventMap* event_map) {
    trace_analyzer::TraceEventVector events;
    GetTraceEvents(analyzer, event_name, &events);
    for (size_t i = 0; i < events.size(); i++) {
      std::map<std::string, double>::const_iterator j;
      for (j = events[i]->arg_numbers.begin();
           j != events[i]->arg_numbers.end();
           ++j) {
        (*event_map)[*j] = events[i];
      }
    }
  }

  // Look up an event in |event_map|. The return event will have the same
  // value for the argument |key_name| as |prev_event|.
  const trace_analyzer::TraceEvent* FindNextEvent(
      const EventMap& event_map,
      const trace_analyzer::TraceEvent* prev_event,
      std::string key_name) {
    const auto arg_it = prev_event->arg_numbers.find(key_name);
    if (arg_it == prev_event->arg_numbers.end())
      return nullptr;
    const EventMapKey& key = *arg_it;
    const auto event_it = event_map.find(key);
    if (event_it == event_map.end())
      return nullptr;
    return event_it->second;
  }

  // Given a vector of vector of data, extract the difference between
  // two columns (|col_a| and |col_b|) and output the result as a
  // performance metric.
  void OutputMeasurement(const std::string& test_name,
                         const std::vector<std::vector<double>>& data,
                         const std::string& measurement_name,
                         int col_a,
                         int col_b) {
    std::vector<double> tmp;
    for (size_t i = 0; i < data.size(); i++) {
      tmp.push_back((data[i][col_b] - data[i][col_a]) / 1000.0);
    }
    return MeanAndError(tmp).Print(test_name,
                                   GetSuffixForTestFlags(),
                                   measurement_name,
                                   "ms");
  }

  // Analyze the latency of each frame as it goes from capture to playout. The
  // event tracing system is used to track the frames.
  void AnalyzeLatency(const std::string& test_name,
                      trace_analyzer::TraceAnalyzer* analyzer) {
    // Retrieve and index all "checkpoint" events related to frames progressing
    // from start to finish.
    trace_analyzer::TraceEventVector capture_events;
    // Sender side:
    GetTraceEvents(analyzer, "Capture", &capture_events);
    EventMap onbuffer, sink, inserted, encoded, transmitted, decoded, done;
    IndexEvents(analyzer, "OnBufferReceived", &onbuffer);
    IndexEvents(analyzer, "ConsumeVideoFrame", &sink);
    IndexEvents(analyzer, "InsertRawVideoFrame", &inserted);
    IndexEvents(analyzer, "VideoFrameEncoded", &encoded);
    // Receiver side:
    IndexEvents(analyzer, "PullEncodedVideoFrame", &transmitted);
    IndexEvents(analyzer, "VideoFrameDecoded", &decoded);
    IndexEvents(analyzer, "VideoFramePlayout", &done);

    // Analyzing latency is non-trivial, because only the frame timestamps
    // uniquely identify frames AND the timestamps take varying forms throughout
    // the pipeline (TimeTicks, TimeDelta, RtpTimestamp, etc.). Luckily, each
    // neighboring stage in the pipeline knows about the timestamp from the
    // prior stage, in whatever form it had, and so it's possible to track
    // specific frames all the way from capture until playout at the receiver.
    std::vector<std::pair<EventMap*, std::string>> event_maps;
    event_maps.push_back(std::make_pair(&onbuffer, "timestamp"));
    event_maps.push_back(std::make_pair(&sink, "time_delta"));
    event_maps.push_back(std::make_pair(&inserted, "timestamp"));
    event_maps.push_back(std::make_pair(&encoded, "rtp_timestamp"));
    event_maps.push_back(std::make_pair(&transmitted, "rtp_timestamp"));
    event_maps.push_back(std::make_pair(&decoded, "rtp_timestamp"));
    event_maps.push_back(std::make_pair(&done, "playout_time"));

    // For each "begin capture" event, search for all the events following it,
    // producing a matrix of when each frame reached each pipeline checkpoint.
    // See the "cheat sheet" below for a description of each pipeline
    // checkpoint.
    ASSERT_GT(capture_events.size(), 2 * kTrimEvents);
    std::vector<std::vector<double>> traced_frames;
    for (size_t i = kTrimEvents; i < capture_events.size() - kTrimEvents; i++) {
      std::vector<double> times;
      const trace_analyzer::TraceEvent* event = capture_events[i];
      if (!event->other_event)
        continue;  // Begin capture event without a corresponding end event.
      times.push_back(event->timestamp);  // begin capture
      event = event->other_event;
      times.push_back(event->timestamp);  // end capture
      const trace_analyzer::TraceEvent* prev_event = event;
      for (size_t j = 0; j < event_maps.size(); j++) {
        event = FindNextEvent(*event_maps[j].first, prev_event,
                              event_maps[j].second);
        if (!event)
          break;  // Missing an event: The frame was dropped along the way.
        prev_event = event;
        times.push_back(event->timestamp);
      }
      if (event) {
        // Successfully traced frame from beginning to end.
        traced_frames.push_back(std::move(times));
      }
    }

    // Report the fraction of captured frames that were dropped somewhere along
    // the way (i.e., before playout at the receiver).
    const size_t capture_event_count = capture_events.size() - 2 * kTrimEvents;
    EXPECT_GE(capture_event_count, kMinDataPoints);
    const double success_percent =
        100.0 * traced_frames.size() / capture_event_count;
    perf_test::PrintResult(
        test_name, GetSuffixForTestFlags(), "frame_drop_rate",
        base::StringPrintf("%f", 100 - success_percent), "percent", true);

    // Report the latency between various pairs of checkpoints in the pipeline.
    // Lower latency is better for all of these measurements.
    //
    // Cheat sheet:
    //   0 = Sender: capture begin
    //   1 = Sender: capture end
    //   2 = Sender: memory buffer reached the render process
    //   3 = Sender: frame routed to Cast RTP consumer
    //   4 = Sender: frame reached VideoSender::InsertRawVideoFrame()
    //   5 = Sender: frame encoding complete, queueing for transmission
    //   6 = Receiver: frame fully received from network
    //   7 = Receiver: frame decoded
    //   8 = Receiver: frame played out
    OutputMeasurement(test_name, traced_frames, "total_latency", 0, 8);
    OutputMeasurement(test_name, traced_frames, "capture_duration", 0, 1);
    OutputMeasurement(test_name, traced_frames, "send_to_renderer", 1, 3);
    OutputMeasurement(test_name, traced_frames, "encode", 3, 5);
    OutputMeasurement(test_name, traced_frames, "transmit", 5, 6);
    OutputMeasurement(test_name, traced_frames, "decode", 6, 7);
    OutputMeasurement(test_name, traced_frames, "cast_latency", 3, 8);
  }

  MeanAndError AnalyzeTraceDistance(trace_analyzer::TraceAnalyzer* analyzer,
                                    const std::string& event_name) {
    trace_analyzer::TraceEventVector events;
    GetTraceEvents(analyzer, event_name, &events);

    std::vector<double> deltas;
    for (size_t i = kTrimEvents + 1; i < events.size() - kTrimEvents; ++i) {
      double delta_micros = events[i]->timestamp - events[i - 1]->timestamp;
      deltas.push_back(delta_micros / 1000.0);
    }
    return MeanAndError(deltas);
  }

  void RunTest(const std::string& test_name) {
    if (HasFlag(kUseGpu) && !IsGpuAvailable()) {
      LOG(WARNING) <<
          "Test skipped: requires gpu. Pass --enable-gpu on the command "
          "line if use of GPU is desired.";
      return;
    }

    ASSERT_EQ(1,
              (HasFlag(k24fps) ? 1 : 0) +
              (HasFlag(k30fps) ? 1 : 0) +
              (HasFlag(k60fps) ? 1 : 0));

    net::IPEndPoint receiver_end_point = media::cast::test::GetFreeLocalPort();

    // Start the in-process receiver that examines audio/video for the expected
    // test patterns.
    base::TimeDelta delta = base::TimeDelta::FromSeconds(0);
    if (HasFlag(kFastClock)) {
      delta = base::TimeDelta::FromSeconds(10);
    }
    if (HasFlag(kSlowClock)) {
      delta = base::TimeDelta::FromSeconds(-10);
    }
    scoped_refptr<media::cast::StandaloneCastEnvironment> cast_environment(
        new SkewedCastEnvironment(delta));
    TestPatternReceiver* const receiver =
        new TestPatternReceiver(cast_environment, receiver_end_point);
    receiver->Start();

    std::unique_ptr<media::cast::test::UDPProxy> udp_proxy;
    if (HasFlag(kProxyWifi) || HasFlag(kProxySlow) || HasFlag(kProxyBad)) {
      net::IPEndPoint proxy_end_point = media::cast::test::GetFreeLocalPort();
      if (HasFlag(kProxyWifi)) {
        udp_proxy = media::cast::test::UDPProxy::Create(
            proxy_end_point, receiver_end_point,
            media::cast::test::WifiNetwork(), media::cast::test::WifiNetwork(),
            nullptr);
      } else if (HasFlag(kProxySlow)) {
        udp_proxy = media::cast::test::UDPProxy::Create(
            proxy_end_point, receiver_end_point,
            media::cast::test::SlowNetwork(), media::cast::test::SlowNetwork(),
            nullptr);
      } else if (HasFlag(kProxyBad)) {
        udp_proxy = media::cast::test::UDPProxy::Create(
            proxy_end_point, receiver_end_point,
            media::cast::test::BadNetwork(), media::cast::test::BadNetwork(),
            nullptr);
      }
      receiver_end_point = proxy_end_point;
    }

    std::string json_events;
    ASSERT_TRUE(tracing::BeginTracing("gpu.capture,cast_perf_test"));
    const std::string page_url = base::StringPrintf(
        "performance%d.html?port=%d&autoThrottling=%s&aesKey=%s&aesIvMask=%s",
        getfps(), receiver_end_point.port(),
        HasFlag(kAutoThrottling) ? "true" : "false",
        base::HexEncode(kAesKey, sizeof(kAesKey)).c_str(),
        base::HexEncode(kAesIvMask, sizeof(kAesIvMask)).c_str());
    ASSERT_TRUE(RunExtensionSubtest("cast_streaming", page_url)) << message_;
    ASSERT_TRUE(tracing::EndTracing(&json_events));
    receiver->Stop();

    // Stop all threads, removes the need for synchronization when analyzing
    // the data.
    cast_environment->Shutdown();
    std::unique_ptr<trace_analyzer::TraceAnalyzer> analyzer;
    analyzer.reset(trace_analyzer::TraceAnalyzer::Create(json_events));
    analyzer->AssociateAsyncBeginEndEvents();

    // This prints out the average time between capture events.
    // Depending on the test, the capture frame rate is capped (e.g., at 30fps,
    // this score cannot get any better than 33.33 ms). However, the measurement
    // is important since it provides a valuable check that capture can keep up
    // with the content's framerate.
    MeanAndError capture_data = AnalyzeTraceDistance(analyzer.get(), "Capture");
    // Lower is better.
    capture_data.Print(test_name,
                       GetSuffixForTestFlags(),
                       "time_between_captures",
                       "ms");

    receiver->Analyze(test_name, GetSuffixForTestFlags());

    AnalyzeLatency(test_name, analyzer.get());
  }
};

}  // namespace

#if defined(OS_WIN)
#define MAYBE_Performance DISABLED_Performance
#else
#define MAYBE_Performance Performance
#endif

IN_PROC_BROWSER_TEST_P(CastV2PerformanceTest, MAYBE_Performance) {
  RunTest("CastV2Performance");
}

// Note: First argument is optional and intentionally left blank.
// (it's a prefix for the generated test cases)
INSTANTIATE_TEST_CASE_P(
    ,
    CastV2PerformanceTest,
    testing::Values(kUseGpu | k24fps,
                    kUseGpu | k30fps,
                    kUseGpu | k60fps,
                    kUseGpu | k30fps | kProxyWifi,
                    kUseGpu | k30fps | kProxyBad,
                    kUseGpu | k30fps | kSlowClock,
                    kUseGpu | k30fps | kFastClock,
                    kUseGpu | k30fps | kProxyWifi | kAutoThrottling,
                    kUseGpu | k30fps | kProxySlow | kAutoThrottling,
                    kUseGpu | k30fps | kProxyBad | kAutoThrottling));
