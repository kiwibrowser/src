// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/mock_web_midi_accessor.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "content/shell/test_runner/test_interfaces.h"
#include "content/shell/test_runner/test_runner.h"
#include "content/shell/test_runner/web_test_delegate.h"
#include "content/shell/test_runner/web_test_runner.h"
#include "third_party/blink/public/platform/modules/webmidi/web_midi_accessor_client.h"
#include "third_party/blink/public/platform/web_string.h"

using midi::mojom::PortState;
using midi::mojom::Result;

namespace test_runner {

namespace {

constexpr unsigned char kSysexHeader[] = {0xf0, 0x00, 0x02, 0x0d, 0x7f};
constexpr unsigned char kSysexFooter = 0xf7;
constexpr size_t kSysexMinimumLength =
    arraysize(kSysexHeader) + sizeof(kSysexFooter) + 1;

bool isSysexForTesting(const unsigned char* data, size_t length) {
  // It should have five bytes header, one byte footer, and at least one byte
  // payload.
  if (length < kSysexMinimumLength)
    return false;
  if (memcmp(data, kSysexHeader, arraysize(kSysexHeader)))
    return false;
  return data[length - 1] == kSysexFooter;
}

}  // namespace

MockWebMIDIAccessor::MockWebMIDIAccessor(blink::WebMIDIAccessorClient* client,
                                         TestInterfaces* interfaces)
    : client_(client),
      interfaces_(interfaces),
      next_input_port_index_(0),
      next_output_port_index_(0),
      weak_factory_(this) {}

MockWebMIDIAccessor::~MockWebMIDIAccessor() {}

void MockWebMIDIAccessor::StartSession() {
  // Add a mock input and output port.
  addInputPort(PortState::CONNECTED);
  addOutputPort(PortState::CONNECTED);
  interfaces_->GetDelegate()->PostTask(base::BindOnce(
      &MockWebMIDIAccessor::reportStartedSession, weak_factory_.GetWeakPtr(),
      interfaces_->GetTestRunner()->midiAccessorResult()));
}

void MockWebMIDIAccessor::RunDidReceiveMIDIData(unsigned port_index,
                                                const unsigned char* data,
                                                size_t length,
                                                base::TimeTicks timestamp) {
  client_->DidReceiveMIDIData(port_index, data, length, timestamp);
}

void MockWebMIDIAccessor::SendMIDIData(unsigned port_index,
                                       const unsigned char* data,
                                       size_t length,
                                       base::TimeTicks timestamp) {
  // Emulate a loopback device for testing. Make sure if an input port that has
  // the same index exists.
  if (port_index < next_input_port_index_) {
    interfaces_->GetDelegate()->PostDelayedTask(
        base::BindOnce(&MockWebMIDIAccessor::RunDidReceiveMIDIData,
                       weak_factory_.GetWeakPtr(), port_index, data, length,
                       timestamp),
        std::max(base::TimeDelta(), timestamp - base::TimeTicks::Now()));
  }

  // Handle special sysex messages for testing.
  // A special sequence is [0xf0, 0x00, 0x02, 0x0d, 0x7f, <function>, 0xf7].
  // <function> should be one of following sequences.
  //  - [0x00, 0x00]: Add an input port as connected.
  //  - [0x00, 0x01]: Add an output port as connected.
  //  - [0x00, 0x02]: Add an input port as opened.
  //  - [0x00, 0x03]: Add an output port as opened.
  if (!isSysexForTesting(data, length))
    return;
  size_t offset = arraysize(kSysexHeader);
  if (data[offset++] != 0)
    return;
  switch (data[offset]) {
    case 0:
      addInputPort(PortState::CONNECTED);
      break;
    case 1:
      addOutputPort(PortState::CONNECTED);
      break;
    case 2:
      addInputPort(PortState::OPENED);
      break;
    case 3:
      addOutputPort(PortState::OPENED);
      break;
    default:
      break;
  }
}

void MockWebMIDIAccessor::addInputPort(PortState state) {
  std::string id =
      base::StringPrintf("MockInputID-%d", next_input_port_index_++);
  client_->DidAddInputPort(blink::WebString::FromUTF8(id),
                           "MockInputManufacturer", "MockInputName",
                           "MockInputVersion", state);
}

void MockWebMIDIAccessor::addOutputPort(PortState state) {
  std::string id =
      base::StringPrintf("MockOutputID-%d", next_output_port_index_++);
  client_->DidAddOutputPort(blink::WebString::FromUTF8(id),
                            "MockOutputManufacturer", "MockOutputName",
                            "MockOutputVersion", state);
}

void MockWebMIDIAccessor::reportStartedSession(Result result) {
  client_->DidStartSession(result);
}

}  // namespace test_runner
