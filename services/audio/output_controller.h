// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_OUTPUT_CONTROLLER_H_
#define SERVICES_AUDIO_OUTPUT_CONTROLLER_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/atomic_ref_count.h"
#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/unguessable_token.h"
#include "build/build_config.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_power_monitor.h"
#include "media/audio/audio_source_diverter.h"
#include "services/audio/group_member.h"

// An OutputController controls an AudioOutputStream and provides data to this
// output stream. It executes audio operations like play, pause, stop, etc. on
// the audio manager thread, while the audio data flow occurs on the platform's
// realtime audio thread.
//
// Here is a state transition diagram for the OutputController:
//
//   *[ Empty ]  -->  [ Created ]  -->  [ Playing ]  -------.
//        |                |               |    ^           |
//        |                |               |    |           |
//        |                |               |    |           v
//        |                |               |    `-----  [ Paused ]
//        |                |               |                |
//        |                v               v                |
//        `----------->  [      Closed       ]  <-----------'
//
// * Initial state
//
// At any time after reaching the Created state but before Closed, the
// OutputController may be notified of a device change via OnDeviceChange().  As
// the OnDeviceChange() is processed, state transitions will occur, ultimately
// ending up in an equivalent pre-call state.  E.g., if the state was Paused,
// the new state will be Created, since these states are all functionally
// equivalent and require a Play() call to continue to the next state.
//
// The AudioOutputStream can request data from the OutputController via the
// AudioSourceCallback interface. OutputController uses the SyncReader passed to
// it via construction to synchronously fulfill this read request.

namespace audio {

class OutputController : public media::AudioOutputStream::AudioSourceCallback,
                         public GroupMember,
                         public media::AudioManager::AudioDeviceListener {
 public:
  // An event handler that receives events from the OutputController. The
  // following methods are called on the audio manager thread.
  class EventHandler {
   public:
    virtual void OnControllerPlaying() = 0;
    virtual void OnControllerPaused() = 0;
    virtual void OnControllerError() = 0;
    virtual void OnLog(base::StringPiece message) = 0;

   protected:
    virtual ~EventHandler() {}
  };

  // A synchronous reader interface used by OutputController for synchronous
  // reading.
  // TODO(crogers): find a better name for this class and the Read() method
  // now that it can handle synchronized I/O.
  class SyncReader {
   public:
    virtual ~SyncReader() {}

    // This is used by SyncReader to prepare more data and perform
    // synchronization. Also inform about output delay at a certain moment and
    // if any frames have been skipped by the renderer (typically the OS). The
    // renderer source can handle this appropriately depending on the type of
    // source. An ordinary file playout would ignore this.
    virtual void RequestMoreData(base::TimeDelta delay,
                                 base::TimeTicks delay_timestamp,
                                 int prior_frames_skipped) = 0;

    // Attempts to completely fill |dest|, zeroing |dest| if the request can not
    // be fulfilled (due to timeout).
    virtual void Read(media::AudioBus* dest) = 0;

    // Close this synchronous reader.
    virtual void Close() = 0;
  };

  // |audio_manager| and |handler| must outlive OutputController.  The
  // |output_device_id| can be either empty (default device) or specify a
  // specific hardware device for audio output.
  OutputController(media::AudioManager* audio_manager,
                   EventHandler* handler,
                   const media::AudioParameters& params,
                   const std::string& output_device_id,
                   const base::UnguessableToken& group_id,
                   SyncReader* sync_reader);
  ~OutputController() override;

  // Indicates whether audio power level analysis will be performed.  If false,
  // ReadCurrentPowerAndClip() can not be called.
  static constexpr bool will_monitor_audio_levels() {
#if defined(OS_ANDROID) || defined(OS_IOS)
    return false;
#else
    return true;
#endif
  }

  // Methods to control playback of the stream.

  // Creates the audio output stream. This must be called before Play(). Returns
  // true if successful, and Play() may commence.
  bool Create(bool is_for_device_change);

  // Starts the playback of this audio output stream.
  void Play();

  // Pause this audio output stream.
  void Pause();

  // Closes the audio output stream synchronously. Stops the stream first, if
  // necessary. After this method returns, this OutputController can be
  // destroyed by its owner.
  void Close();

  // Sets the volume of the audio output stream.
  void SetVolume(double volume);

  // AudioSourceCallback implementation.
  int OnMoreData(base::TimeDelta delay,
                 base::TimeTicks delay_timestamp,
                 int prior_frames_skipped,
                 media::AudioBus* dest) override;
  void OnError() override;

  // GroupMember implementation.
  const base::UnguessableToken& GetGroupId() override;
  const media::AudioParameters& GetAudioParameters() override;
  void StartSnooping(Snooper* snooper) override;
  void StopSnooping(Snooper* snooper) override;
  void StartMuting() override;
  void StopMuting() override;

  // AudioDeviceListener implementation.  When called OutputController will
  // shutdown the existing |stream_|, create a new stream, and then transition
  // back to an equivalent state prior to being called.
  void OnDeviceChange() override;

  // Accessor for AudioPowerMonitor::ReadCurrentPowerAndClip().  See comments in
  // audio_power_monitor.h for usage.  This may be called on any thread.
  std::pair<float, bool> ReadCurrentPowerAndClip();

 protected:
  // Internal state of the source.
  enum State {
    kEmpty,
    kCreated,
    kPlaying,
    kPaused,
    kClosed,
    kError,
  };

  // Time constant for AudioPowerMonitor.  See AudioPowerMonitor ctor comments
  // for semantics.  This value was arbitrarily chosen, but seems to work well.
  enum { kPowerMeasurementTimeConstantMillis = 10 };

 private:
  // Used to store various stats about a stream. The lifetime of this object is
  // from play until pause. The underlying physical stream may be changed when
  // resuming playback, hence separate stats are logged for each play/pause
  // cycle.
  class ErrorStatisticsTracker {
   public:
    ErrorStatisticsTracker();

    // Note: the destructor takes care of logging all of the stats.
    ~ErrorStatisticsTracker();

    // Called to indicate an error callback was fired for the stream.
    void RegisterError();

    // This function should be called from the stream callback thread.
    void OnMoreDataCalled();

   private:
    void WedgeCheck();

    const base::TimeTicks start_time_;

    bool error_during_callback_ = false;

    // Flags when we've asked for a stream to start but it never did.
    base::AtomicRefCount on_more_io_data_called_;
    base::OneShotTimer wedge_timer_;
  };

  // Provides a thread-safe AudioSourceDiverter proxy, sitting in front of the
  // simpler threading model of this OutputController.
  // TODO(crbug/824019): Remove this legacy functionality.
  class ThreadHoppingDiverter
      : public base::SupportsWeakPtr<ThreadHoppingDiverter>,
        public media::AudioSourceDiverter {
   public:
    explicit ThreadHoppingDiverter(OutputController* controller);
    ~ThreadHoppingDiverter() final;

    // AudioSourceDiverter implementation.
    const media::AudioParameters& GetAudioParameters() final;
    void StartDiverting(media::AudioOutputStream* to_stream) final;
    void StopDiverting() final;
    void StartDuplicating(media::AudioPushSink* sink) final;
    void StopDuplicating(media::AudioPushSink* sink) final;

   private:
    OutputController* const controller_;
    const base::WeakPtr<ThreadHoppingDiverter> weak_this_;
  };

  // Start/StopDiverting() and Start/StopDuplicating() trampoline to these
  // methods through |task_runner_|.
  // TODO(crbug/824019): Remove this legacy functionality.
  void DoStartDiverting(media::AudioOutputStream* to_stream);
  void DoStopDiverting();
  void DoStartDuplicating(media::AudioPushSink* sink);
  void DoStopDuplicating(media::AudioPushSink* sink);

  // Notifies the EventHandler that an error has occurred.
  void ReportError();

  // Helper method that stops the physical stream.
  void StopStream();

  // Helper method that stops, closes, and NULLs |*stream_|.
  void StopCloseAndClearStream();

  // Send audio data to each Snooper.
  void BroadcastDataToSnoopers(std::unique_ptr<media::AudioBus> audio_bus,
                               base::TimeTicks reference_time);

  // Log the current average power level measured by power_monitor_.
  void LogAudioPowerLevel(const char* call_name);

  media::AudioManager* const audio_manager_;
  const media::AudioParameters params_;
  EventHandler* const handler_;

  // The task runner for the audio manager. All control methods should be called
  // via tasks run by this TaskRunner.
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Specifies the device id of the output device to open or empty for the
  // default output device.
  const std::string output_device_id_;

  // A token indicating membership in a group of output controllers/streams.
  const base::UnguessableToken group_id_;

  media::AudioOutputStream* stream_;

  // When non-NULL, audio is being diverted to this stream.
  // TODO(crbug/824019): Remove this legacy functionality.
  media::AudioOutputStream* diverting_to_stream_;

  // When true, local audio output should be muted; either by having audio
  // diverted to |diverting_to_stream_|, or a fake AudioOutputStream.
  bool disable_local_output_;

  // The targets for audio stream to be copied to. |should_duplicate_| is set to
  // 1 when the OnMoreData() call should proxy the data to
  // BroadcastDataToSnoopers().
  //
  // TODO(crbug/824019): Remove |duplication_targets_| (part of legacy
  // functionality).
  base::flat_set<media::AudioPushSink*> duplication_targets_;
  std::vector<Snooper*> snoopers_;
  base::AtomicRefCount should_duplicate_;

  // The current volume of the audio stream.
  double volume_;

  State state_;

  // SyncReader is used only in low latency mode for synchronous reading.
  SyncReader* const sync_reader_;

  // Scans audio samples from OnMoreData() as input to compute power levels.
  media::AudioPowerMonitor power_monitor_;

  // Updated each time a power measurement is logged.
  base::TimeTicks last_audio_level_log_time_;

  // Used for keeping track of and logging stats. Created when a stream starts
  // and destroyed when a stream stops. Also reset every time there is a stream
  // being created due to device changes.
  base::Optional<ErrorStatisticsTracker> stats_tracker_;

  // Instantiated in Create(), and destroyed in Close().
  // TODO(crbug/824019): Remove this legacy functionality.
  base::Optional<ThreadHoppingDiverter> diverter_;

  // WeakPtrFactory+WeakPtr that is used to post tasks that are canceled when a
  // stream is closed.
  base::WeakPtr<OutputController> weak_this_for_stream_;
  base::WeakPtrFactory<OutputController> weak_factory_for_stream_;

  DISALLOW_COPY_AND_ASSIGN(OutputController);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_OUTPUT_CONTROLLER_H_
