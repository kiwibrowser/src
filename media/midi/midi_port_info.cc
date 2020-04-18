// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/midi_port_info.h"

namespace midi {

using mojom::PortState;

MidiPortInfo::MidiPortInfo() = default;

MidiPortInfo::MidiPortInfo(const std::string& in_id,
                           const std::string& in_manufacturer,
                           const std::string& in_name,
                           const std::string& in_version,
                           PortState in_state)
    : id(in_id),
      manufacturer(in_manufacturer),
      name(in_name),
      version(in_version),
      state(in_state) {}

MidiPortInfo::~MidiPortInfo() = default;

MidiPortInfo::MidiPortInfo(const MidiPortInfo& info) = default;

}  // namespace midi
