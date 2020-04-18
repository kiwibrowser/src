// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/battor_connection.h"

#include "base/bind.h"
#include "base/callback.h"
#include "device/serial/buffer.h"
#include "device/serial/serial_io_handler.h"
#include "net/base/io_buffer.h"

namespace battor {

BattOrConnection::BattOrConnection(Listener* listener) : listener_(listener) {}
BattOrConnection::~BattOrConnection() = default;

}  // namespace battor
