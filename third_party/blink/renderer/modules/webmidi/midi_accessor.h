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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBMIDI_MIDI_ACCESSOR_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBMIDI_MIDI_ACCESSOR_H_

#include <memory>
#include "base/time/time.h"
#include "media/midi/midi_service.mojom-blink.h"
#include "third_party/blink/public/platform/modules/webmidi/web_midi_accessor.h"
#include "third_party/blink/public/platform/modules/webmidi/web_midi_accessor_client.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class MIDIAccessorClient;

class MIDIAccessor final : public WebMIDIAccessorClient {
  USING_FAST_MALLOC(MIDIAccessor);

 public:
  static std::unique_ptr<MIDIAccessor> Create(MIDIAccessorClient*);

  ~MIDIAccessor() override = default;

  void StartSession();
  void SendMIDIData(unsigned port_index,
                    const unsigned char* data,
                    size_t length,
                    base::TimeTicks time_stamp);
  // MIDIAccessInitializer and MIDIAccess are both MIDIAccessClient.
  // MIDIAccessInitializer is the first client and MIDIAccess takes over it
  // once the initialization successfully finishes.
  void SetClient(MIDIAccessorClient* client) { client_ = client; }

  // WebMIDIAccessorClient
  void DidAddInputPort(const WebString& id,
                       const WebString& manufacturer,
                       const WebString& name,
                       const WebString& version,
                       midi::mojom::PortState) override;
  void DidAddOutputPort(const WebString& id,
                        const WebString& manufacturer,
                        const WebString& name,
                        const WebString& version,
                        midi::mojom::PortState) override;
  void DidSetInputPortState(unsigned port_index,
                            midi::mojom::PortState) override;
  void DidSetOutputPortState(unsigned port_index,
                             midi::mojom::PortState) override;
  void DidStartSession(midi::mojom::Result) override;
  void DidReceiveMIDIData(unsigned port_index,
                          const unsigned char* data,
                          size_t length,
                          base::TimeTicks time_stamp) override;

 private:
  explicit MIDIAccessor(MIDIAccessorClient*);

  MIDIAccessorClient* client_;
  std::unique_ptr<WebMIDIAccessor> accessor_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBMIDI_MIDI_ACCESSOR_H_
