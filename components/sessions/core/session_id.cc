// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/core/session_id.h"

#include <ostream>

static SessionID::id_type next_id = 1;

// static
SessionID SessionID::NewUnique() {
  return SessionID(next_id++);
}

std::ostream& operator<<(std::ostream& out, SessionID id) {
  out << id.id();
  return out;
}
