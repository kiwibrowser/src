/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/webmidi/midi_accessor.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/platform/modules/webmidi/web_midi_accessor.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/modules/webmidi/midi_accessor_client.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

using blink::WebString;
using midi::mojom::PortState;
using midi::mojom::Result;

namespace blink {

// Factory method
std::unique_ptr<MIDIAccessor> MIDIAccessor::Create(MIDIAccessorClient* client) {
  return base::WrapUnique(new MIDIAccessor(client));
}

MIDIAccessor::MIDIAccessor(MIDIAccessorClient* client) : client_(client) {
  DCHECK(client);

  accessor_ = Platform::Current()->CreateMIDIAccessor(this);

  DCHECK(accessor_);
}

void MIDIAccessor::StartSession() {
  accessor_->StartSession();
}

void MIDIAccessor::SendMIDIData(unsigned port_index,
                                const unsigned char* data,
                                size_t length,
                                base::TimeTicks time_stamp) {
  accessor_->SendMIDIData(port_index, data, length, time_stamp);
}

void MIDIAccessor::DidAddInputPort(const WebString& id,
                                   const WebString& manufacturer,
                                   const WebString& name,
                                   const WebString& version,
                                   PortState state) {
  client_->DidAddInputPort(id, manufacturer, name, version, state);
}

void MIDIAccessor::DidAddOutputPort(const WebString& id,
                                    const WebString& manufacturer,
                                    const WebString& name,
                                    const WebString& version,
                                    PortState state) {
  client_->DidAddOutputPort(id, manufacturer, name, version, state);
}

void MIDIAccessor::DidSetInputPortState(unsigned port_index, PortState state) {
  client_->DidSetInputPortState(port_index, state);
}

void MIDIAccessor::DidSetOutputPortState(unsigned port_index, PortState state) {
  client_->DidSetOutputPortState(port_index, state);
}

void MIDIAccessor::DidStartSession(Result result) {
  client_->DidStartSession(result);
}

void MIDIAccessor::DidReceiveMIDIData(unsigned port_index,
                                      const unsigned char* data,
                                      size_t length,
                                      base::TimeTicks time_stamp) {
  client_->DidReceiveMIDIData(port_index, data, length, time_stamp);
}

}  // namespace blink
