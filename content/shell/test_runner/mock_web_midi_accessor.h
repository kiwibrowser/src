// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_MOCK_WEB_MIDI_ACCESSOR_H_
#define CONTENT_SHELL_TEST_RUNNER_MOCK_WEB_MIDI_ACCESSOR_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/midi/midi_service.mojom.h"
#include "third_party/blink/public/platform/modules/webmidi/web_midi_accessor.h"

namespace blink {
class WebMIDIAccessorClient;
}

namespace test_runner {

class TestInterfaces;

class MockWebMIDIAccessor : public blink::WebMIDIAccessor {
 public:
  MockWebMIDIAccessor(blink::WebMIDIAccessorClient* client,
                      TestInterfaces* interfaces);
  ~MockWebMIDIAccessor() override;

  // blink::WebMIDIAccessor implementation.
  void StartSession() override;
  void SendMIDIData(unsigned port_index,
                    const unsigned char* data,
                    size_t length,
                    base::TimeTicks timestamp) override;

 private:
  void addInputPort(midi::mojom::PortState state);
  void addOutputPort(midi::mojom::PortState state);
  void reportStartedSession(midi::mojom::Result result);

  void RunDidReceiveMIDIData(unsigned port_index,
                             const unsigned char* data,
                             size_t length,
                             base::TimeTicks time_stamp);

  blink::WebMIDIAccessorClient* client_;
  TestInterfaces* interfaces_;
  unsigned next_input_port_index_;
  unsigned next_output_port_index_;

  base::WeakPtrFactory<MockWebMIDIAccessor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MockWebMIDIAccessor);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_MOCK_WEB_MIDI_ACCESSOR_H_
