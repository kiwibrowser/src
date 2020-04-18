// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_SERVICE_H_
#define SERVICES_AUDIO_SERVICE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "services/audio/public/mojom/debug_recording.mojom.h"
#include "services/audio/public/mojom/device_notifications.mojom.h"
#include "services/audio/public/mojom/stream_factory.mojom.h"
#include "services/audio/public/mojom/system_info.mojom.h"
#include "services/audio/stream_factory.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace base {
class SystemMonitor;
}

namespace media {
class AudioDeviceListenerMac;
class AudioManager;
}  // namespace media

namespace service_manager {
class ServiceContextRefFactory;
}

namespace audio {
class DebugRecording;
class DeviceNotifier;
class ServiceMetrics;
class SystemInfo;

class Service : public service_manager::Service {
 public:
  // Abstracts AudioManager ownership. Lives and must be accessed on a thread
  // its created on, and that thread must be AudioManager main thread.
  class AudioManagerAccessor {
   public:
    virtual ~AudioManagerAccessor() {}

    // Must be called before destruction to cleanly shut down AudioManager.
    // Service must ensure AudioManager is not called after that.
    virtual void Shutdown() = 0;

    // Returns a pointer to AudioManager.
    virtual media::AudioManager* GetAudioManager() = 0;
  };

  // Service will attempt to quit if there are no connections to it within
  // |quit_timeout| interval. If |quit_timeout| is base::TimeDelta() the
  // service never quits. If |device_notifier_enabled| is true, the service
  // will make available a DeviceNotifier object that allows clients to
  // subscribe to notifications about device changes.
  Service(std::unique_ptr<AudioManagerAccessor> audio_manager_accessor,
          base::TimeDelta quit_timeout,
          bool device_notifier_enabled);
  ~Service() final;

  // service_manager::Service implementation.
  void OnStart() final;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) final;
  bool OnServiceManagerConnectionLost() final;

  void SetQuitClosureForTesting(base::RepeatingClosure quit_closure);

 private:
  void BindSystemInfoRequest(mojom::SystemInfoRequest request);
  void BindDebugRecordingRequest(mojom::DebugRecordingRequest request);
  void BindStreamFactoryRequest(mojom::StreamFactoryRequest request);
  void BindDeviceNotifierRequest(mojom::DeviceNotifierRequest request);

  void MaybeRequestQuitDelayed();
  void MaybeRequestQuit();

  // Initializes a platform-specific device monitor for device-change
  // notifications. If the client uses the DeviceNotifier interface to get
  // notifications this function should be called before the DeviceMonitor is
  // created. If the client uses base::SystemMonitor to get notifications,
  // this function should be called on service startup.
  void InitializeDeviceMonitor();

  // The thread Service runs on should be the same as the main thread of
  // AudioManager provided by AudioManagerAccessor.
  THREAD_CHECKER(thread_checker_);

  // The members below should outlive |ref_factory_|.
  base::RepeatingClosure quit_closure_;
  const base::TimeDelta quit_timeout_;
  base::OneShotTimer quit_timer_;

  std::unique_ptr<AudioManagerAccessor> audio_manager_accessor_;
  const bool device_notifier_enabled_;
  std::unique_ptr<base::SystemMonitor> system_monitor_;
#if defined(OS_MACOSX)
  std::unique_ptr<media::AudioDeviceListenerMac> audio_device_listener_mac_;
#endif
  std::unique_ptr<SystemInfo> system_info_;
  std::unique_ptr<DebugRecording> debug_recording_;
  base::Optional<StreamFactory> stream_factory_;
  std::unique_ptr<DeviceNotifier> device_notifier_;
  std::unique_ptr<ServiceMetrics> metrics_;

  service_manager::BinderRegistry registry_;

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_SERVICE_H_
