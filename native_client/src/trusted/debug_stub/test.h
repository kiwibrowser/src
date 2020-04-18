/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_GDB_RSP_TEST_H_
#define NATIVE_CLIENT_GDB_RSP_TEST_H_ 1

#include <vector>

#include "native_client/src/trusted/debug_stub/abi.h"
#include "native_client/src/trusted/debug_stub/packet.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/session.h"
#include "native_client/src/trusted/debug_stub/target.h"
#include "native_client/src/trusted/debug_stub/transport.h"
#include "native_client/src/trusted/debug_stub/util.h"

typedef void (*PacketFunc_t)(void *ctx,
                             gdb_rsp::Packet *wr,
                             gdb_rsp::Packet *rd);

int VerifyPacket(gdb_rsp::Packet *wr, gdb_rsp::Packet *rd,
                 void *ctx, PacketFunc_t tx);

int TestAbi();
int TestPacket();
int TestSession();
int TestUtil();

#endif  // NATIVE_CLIENT_GDB_RSP_TEST_H_

