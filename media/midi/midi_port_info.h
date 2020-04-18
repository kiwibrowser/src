// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MIDI_MIDI_PORT_INFO_H_
#define MEDIA_MIDI_MIDI_PORT_INFO_H_

#include <string>
#include <vector>

#include "media/midi/midi_export.h"
#include "media/midi/midi_service.mojom.h"

namespace midi {

struct MIDI_EXPORT MidiPortInfo final {
  MidiPortInfo();
  MidiPortInfo(const std::string& in_id,
               const std::string& in_manufacturer,
               const std::string& in_name,
               const std::string& in_version,
               mojom::PortState in_state);

  MidiPortInfo(const MidiPortInfo& info);
  ~MidiPortInfo();

  std::string id;
  std::string manufacturer;
  std::string name;
  std::string version;
  mojom::PortState state;
};

using MidiPortInfoList = std::vector<MidiPortInfo>;

}  // namespace midi

#endif  // MEDIA_MIDI_MIDI_PORT_INFO_H_
