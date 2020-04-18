// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MIDI_MIDI_MANAGER_USB_H_
#define MEDIA_MIDI_MIDI_MANAGER_USB_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "media/midi/midi_manager.h"
#include "media/midi/usb_midi_device.h"
#include "media/midi/usb_midi_export.h"
#include "media/midi/usb_midi_input_stream.h"
#include "media/midi/usb_midi_jack.h"
#include "media/midi/usb_midi_output_stream.h"

namespace midi {

class MidiService;

// MidiManager for USB-MIDI.
class USB_MIDI_EXPORT MidiManagerUsb : public MidiManager,
                                       public UsbMidiDeviceDelegate,
                                       public UsbMidiInputStream::Delegate {
 public:
  MidiManagerUsb(MidiService* service,
                 std::unique_ptr<UsbMidiDevice::Factory> device_factory);
  ~MidiManagerUsb() override;

  // MidiManager implementation.
  void StartInitialization() override;
  void Finalize() override;
  void DispatchSendMidiData(MidiManagerClient* client,
                            uint32_t port_index,
                            const std::vector<uint8_t>& data,
                            base::TimeTicks timestamp) override;

  // UsbMidiDeviceDelegate implementation.
  void ReceiveUsbMidiData(UsbMidiDevice* device,
                          int endpoint_number,
                          const uint8_t* data,
                          size_t size,
                          base::TimeTicks time) override;
  void OnDeviceAttached(std::unique_ptr<UsbMidiDevice> device) override;
  void OnDeviceDetached(size_t index) override;

  // UsbMidiInputStream::Delegate implementation.
  void OnReceivedData(size_t jack_index,
                      const uint8_t* data,
                      size_t size,
                      base::TimeTicks time) override;

  const std::vector<std::unique_ptr<UsbMidiOutputStream>>& output_streams()
      const {
    return output_streams_;
  }
  const UsbMidiInputStream* input_stream() const { return input_stream_.get(); }

 private:
  using Callback = base::OnceCallback<void(mojom::Result)>;

  // Initializes this object.
  // When the initialization finishes, |callback| will be called with the
  // result.
  // When this factory is destroyed during the operation, the operation
  // will be canceled silently (i.e. |callback| will not be called).
  // The function is public just for unit tests. Do not call this function
  // outside code for testing.
  void Initialize(Callback callback);

  void OnEnumerateDevicesDone(bool result, UsbMidiDevice::Devices* devices);
  bool AddPorts(UsbMidiDevice* device, int device_id);

  // TODO(toyoshim): Remove |lock_| once dynamic instantiation mode is enabled
  // by default. This protects objects allocated on the I/O thread from doubly
  // released on the main thread.
  base::Lock lock_;

  std::unique_ptr<UsbMidiDevice::Factory> device_factory_;
  std::vector<std::unique_ptr<UsbMidiDevice>> devices_;
  std::vector<std::unique_ptr<UsbMidiOutputStream>> output_streams_;
  std::unique_ptr<UsbMidiInputStream> input_stream_;

  Callback initialize_callback_;

  // A map from <endpoint_number, cable_number> to the index of input jacks.
  base::hash_map<std::pair<int, int>, size_t> input_jack_dictionary_;

  DISALLOW_COPY_AND_ASSIGN(MidiManagerUsb);
};

}  // namespace midi

#endif  // MEDIA_MIDI_MIDI_MANAGER_USB_H_
