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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_WEBMIDI_WEB_MIDI_ACCESSOR_CLIENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_WEBMIDI_WEB_MIDI_ACCESSOR_CLIENT_H_

#include "media/midi/midi_service.mojom-shared.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class WebMIDIAccessorClient {
 public:
  // didAddInputPort() and didAddOutputPort() can be called before and after
  // didStartSession() is called. But |id| should be unique in each function.
  virtual void DidAddInputPort(const WebString& id,
                               const WebString& manufacturer,
                               const WebString& name,
                               const WebString& version,
                               midi::mojom::PortState) = 0;
  virtual void DidAddOutputPort(const WebString& id,
                                const WebString& manufacturer,
                                const WebString& name,
                                const WebString& version,
                                midi::mojom::PortState) = 0;
  // didSetInputPortState() and didSetOutputPortState() should not be called
  // until didStartSession() is called.
  virtual void DidSetInputPortState(unsigned port_index,
                                    midi::mojom::PortState) = 0;
  virtual void DidSetOutputPortState(unsigned port_index,
                                     midi::mojom::PortState) = 0;

  virtual void DidStartSession(midi::mojom::Result) = 0;

  // |timeStamp| is in milliseconds according to the Web MIDI API.
  virtual void DidReceiveMIDIData(unsigned port_index,
                                  const unsigned char* data,
                                  size_t length,
                                  base::TimeTicks time_stamp) = 0;

 protected:
  virtual ~WebMIDIAccessorClient() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_WEBMIDI_WEB_MIDI_ACCESSOR_CLIENT_H_
