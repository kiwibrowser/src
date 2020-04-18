// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_SENSOR_EVENT_PUMP_H_
#define CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_SENSOR_EVENT_PUMP_H_

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/platform_event_observer.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/cpp/generic_sensor/sensor_reading.h"
#include "services/device/public/cpp/generic_sensor/sensor_reading_shared_buffer_reader.h"
#include "services/device/public/cpp/generic_sensor/sensor_traits.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/sensor_provider.mojom.h"
#include "third_party/blink/public/platform/modules/device_orientation/web_device_motion_listener.h"
#include "third_party/blink/public/platform/modules/device_orientation/web_device_orientation_listener.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace content {

template <typename ListenerType>
class CONTENT_EXPORT DeviceSensorEventPump
    : public PlatformEventObserver<ListenerType> {
 public:
  // Default rate for firing events.
  static constexpr int kDefaultPumpFrequencyHz = 60;
  static constexpr int kDefaultPumpDelayMicroseconds =
      base::Time::kMicrosecondsPerSecond / kDefaultPumpFrequencyHz;

  // The pump is a tri-state automaton with allowed transitions as follows:
  // STOPPED -> PENDING_START
  // PENDING_START -> RUNNING
  // PENDING_START -> STOPPED
  // RUNNING -> STOPPED
  enum class PumpState { STOPPED, RUNNING, PENDING_START };

  // The sensor state is an automaton with allowed transitions as follows:
  // NOT_INITIALIZED -> INITIALIZING
  // INITIALIZING -> ACTIVE
  // INITIALIZING -> SHOULD_SUSPEND
  // ACTIVE -> SUSPENDED
  // SHOULD_SUSPEND -> INITIALIZING
  // SHOULD_SUSPEND -> SUSPENDED
  // SUSPENDED -> ACTIVE
  // { INITIALIZING, ACTIVE, SHOULD_SUSPEND, SUSPENDED } -> NOT_INITIALIZED
  enum class SensorState {
    NOT_INITIALIZED,
    INITIALIZING,
    ACTIVE,
    SHOULD_SUSPEND,
    SUSPENDED
  };

  // PlatformEventObserver:
  void Start(blink::WebPlatformEventListener* listener) override {
    DVLOG(2) << "requested start";

    if (state_ != PumpState::STOPPED)
      return;

    DCHECK(!timer_.IsRunning());

    state_ = PumpState::PENDING_START;
    PlatformEventObserver<ListenerType>::Start(listener);
  }

  // PlatformEventObserver:
  void Stop() override {
    DVLOG(2) << "requested stop";

    if (state_ == PumpState::STOPPED)
      return;

    DCHECK((state_ == PumpState::PENDING_START && !timer_.IsRunning()) ||
           (state_ == PumpState::RUNNING && timer_.IsRunning()));

    if (timer_.IsRunning())
      timer_.Stop();

    PlatformEventObserver<ListenerType>::Stop();
    state_ = PumpState::STOPPED;
  }

  void HandleSensorProviderError() { sensor_provider_.reset(); }

  void SetSensorProviderForTesting(
      device::mojom::SensorProviderPtr sensor_provider) {
    sensor_provider_ = std::move(sensor_provider);
  }

  PumpState GetPumpStateForTesting() { return state_; }

 protected:
  explicit DeviceSensorEventPump(RenderThread* thread)
      : PlatformEventObserver<ListenerType>(thread),
        state_(PumpState::STOPPED) {}

  ~DeviceSensorEventPump() override {
    PlatformEventObserver<ListenerType>::StopIfObserving();
  }

  virtual void FireEvent() = 0;

  struct SensorEntry : public device::mojom::SensorClient {
    SensorEntry(DeviceSensorEventPump* pump,
                device::mojom::SensorType sensor_type)
        : event_pump(pump),
          sensor_state(SensorState::NOT_INITIALIZED),
          type(sensor_type),
          client_binding(this) {}

    ~SensorEntry() override {}

    // device::mojom::SensorClient:
    void RaiseError() override { HandleSensorError(); }

    // device::mojom::SensorClient:
    void SensorReadingChanged() override {
      // Since DeviceSensorEventPump::FireEvent is called in a fixed
      // frequency, the |shared_buffer| is read frequently, and
      // Sensor::ConfigureReadingChangeNotifications() is set to false,
      // so this method is not called and doesn't need to be implemented.
      NOTREACHED();
    }

    // Mojo callback for SensorProvider::GetSensor().
    void OnSensorCreated(device::mojom::SensorCreationResult result,
                         device::mojom::SensorInitParamsPtr params) {
      // |sensor_state| can be SensorState::SHOULD_SUSPEND if Stop() is called
      // before OnSensorCreated() is called.
      DCHECK(sensor_state == SensorState::INITIALIZING ||
             sensor_state == SensorState::SHOULD_SUSPEND);

      if (!params) {
        HandleSensorError();
        event_pump->DidStartIfPossible();
        return;
      }
      DCHECK_EQ(device::mojom::SensorCreationResult::SUCCESS, result);

      constexpr size_t kReadBufferSize =
          sizeof(device::SensorReadingSharedBuffer);

      DCHECK_EQ(0u, params->buffer_offset % kReadBufferSize);

      mode = params->mode;
      default_config = params->default_configuration;

      sensor.Bind(std::move(params->sensor));
      client_binding.Bind(std::move(params->client_request));

      shared_buffer_handle = std::move(params->memory);
      DCHECK(!shared_buffer);
      shared_buffer = shared_buffer_handle->MapAtOffset(kReadBufferSize,
                                                        params->buffer_offset);
      if (!shared_buffer) {
        HandleSensorError();
        event_pump->DidStartIfPossible();
        return;
      }

      const device::SensorReadingSharedBuffer* buffer =
          static_cast<const device::SensorReadingSharedBuffer*>(
              shared_buffer.get());
      shared_buffer_reader.reset(
          new device::SensorReadingSharedBufferReader(buffer));

      default_config.set_frequency(
          std::min(static_cast<double>(kDefaultPumpFrequencyHz),
                   params->maximum_frequency));

      sensor.set_connection_error_handler(base::BindOnce(
          &SensorEntry::HandleSensorError, base::Unretained(this)));
      sensor->ConfigureReadingChangeNotifications(false /* disabled */);
      sensor->AddConfiguration(
          default_config, base::BindOnce(&SensorEntry::OnSensorAddConfiguration,
                                         base::Unretained(this)));
    }

    // Mojo callback for Sensor::AddConfiguration().
    void OnSensorAddConfiguration(bool success) {
      if (!success)
        HandleSensorError();

      if (sensor_state == SensorState::INITIALIZING) {
        sensor_state = SensorState::ACTIVE;
        event_pump->DidStartIfPossible();
      } else if (sensor_state == SensorState::SHOULD_SUSPEND) {
        sensor->Suspend();
        sensor_state = SensorState::SUSPENDED;
      }
    }

    void HandleSensorError() {
      sensor.reset();
      sensor_state = SensorState::NOT_INITIALIZED;
      shared_buffer_handle.reset();
      shared_buffer.reset();
      client_binding.Close();
    }

    bool SensorReadingCouldBeRead() {
      if (!sensor)
        return false;

      DCHECK(shared_buffer);

      if (!shared_buffer_handle->is_valid() ||
          !shared_buffer_reader->GetReading(&reading)) {
        HandleSensorError();
        return false;
      }

      return true;
    }

    bool ReadyOrErrored() const {
      // When some sensors are not available, the pump still needs to fire
      // events which set the unavailable sensor data fields to null.
      return sensor_state == SensorState::ACTIVE ||
             sensor_state == SensorState::NOT_INITIALIZED;
    }

    void Start(device::mojom::SensorProvider* sensor_provider) {
      if (sensor_state == SensorState::NOT_INITIALIZED) {
        sensor_state = SensorState::INITIALIZING;
        sensor_provider->GetSensor(type,
                                   base::BindOnce(&SensorEntry::OnSensorCreated,
                                                  base::Unretained(this)));
      } else if (sensor_state == SensorState::SUSPENDED) {
        sensor->Resume();
        sensor_state = SensorState::ACTIVE;
        event_pump->DidStartIfPossible();
      } else if (sensor_state == SensorState::SHOULD_SUSPEND) {
        // This can happen when calling Start(), Stop(), Start() in a sequence:
        // After the first Start() call, the sensor state is
        // SensorState::INITIALIZING. Then after the Stop() call, the sensor
        // state is SensorState::SHOULD_SUSPEND, and the next Start() call needs
        // to set the sensor state to be SensorState::INITIALIZING again.
        sensor_state = SensorState::INITIALIZING;
      } else {
        NOTREACHED();
      }
    }

    void Stop() {
      if (sensor) {
        sensor->Suspend();
        sensor_state = SensorState::SUSPENDED;
      } else if (sensor_state == SensorState::INITIALIZING) {
        // When the sensor needs to be suspended, and it is still in the
        // SensorState::INITIALIZING state, the sensor creation is not affected
        // (the SensorEntry::OnSensorCreated() callback will run as usual), but
        // the sensor is marked as SensorState::SHOULD_SUSPEND, and when the
        // sensor is created successfully, it will be suspended and its state
        // will be marked as SensorState::SUSPENDED in the
        // SensorEntry::OnSensorAddConfiguration().
        sensor_state = SensorState::SHOULD_SUSPEND;
      }
    }

    DeviceSensorEventPump* event_pump;
    device::mojom::SensorPtr sensor;
    SensorState sensor_state;
    device::mojom::SensorType type;
    device::mojom::ReportingMode mode;
    device::PlatformSensorConfiguration default_config;
    mojo::ScopedSharedBufferHandle shared_buffer_handle;
    mojo::ScopedSharedBufferMapping shared_buffer;
    std::unique_ptr<device::SensorReadingSharedBufferReader>
        shared_buffer_reader;
    device::SensorReading reading;
    mojo::Binding<device::mojom::SensorClient> client_binding;
  };

  friend struct SensorEntry;

  virtual void DidStartIfPossible() {
    DVLOG(2) << "did start sensor event pump";

    if (state_ != PumpState::PENDING_START)
      return;

    if (!SensorsReadyOrErrored())
      return;

    DCHECK(!timer_.IsRunning());

    timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMicroseconds(kDefaultPumpDelayMicroseconds), this,
        &DeviceSensorEventPump::FireEvent);
    state_ = PumpState::RUNNING;
  }

  static RenderFrame* GetRenderFrame() {
    blink::WebLocalFrame* const web_frame =
        blink::WebLocalFrame::FrameForCurrentContext();

    return RenderFrame::FromWebFrame(web_frame);
  }

  device::mojom::SensorProviderPtr sensor_provider_;

 private:
  virtual bool SensorsReadyOrErrored() const = 0;

  PumpState state_;
  base::RepeatingTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(DeviceSensorEventPump);
};
}  // namespace content

#endif  // CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_SENSOR_EVENT_PUMP_H_
