/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <sstream>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/debug_stub/packet.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/session.h"
#include "native_client/src/trusted/debug_stub/transport.h"
#include "native_client/src/trusted/debug_stub/util.h"

using port::IPlatform;
using port::ITransport;

namespace gdb_rsp {

Session::Session(ITransport *transport)
  : io_(transport),
    flags_(0),
    seq_(0),
    connected_(true) {
}

Session::~Session() {
}

void Session::SetFlags(uint32_t flags) {
  flags_ |= flags;
}

void Session::ClearFlags(uint32_t flags) {
  flags_ &= ~flags;
}

uint32_t Session::GetFlags() {
  return flags_;
}

bool Session::IsDataAvailable() {
  return io_->IsDataAvailable();
}

bool Session::Connected() {
  return connected_;
}

void Session::Disconnect() {
  io_->Disconnect();
  connected_ = false;
}

bool Session::GetChar(char *ch) {
  if (!io_->Read(ch, 1)) {
    Disconnect();
    return false;
  }

  return true;
}


bool Session::SendPacket(Packet *pkt) {
  char ch;

  do {
    if (!SendPacketOnly(pkt)) return false;

    // If ACKs are off, we are done.
    if (GetFlags() & IGNORE_ACK) break;

    // Otherwise, poll for '+'
    if (!GetChar(&ch)) return false;

    // Retry if we didn't get a '+'
  } while (ch != '+');

  return true;
}


bool Session::SendPacketOnly(Packet *pkt) {
  const char *ptr;
  char ch;
  std::stringstream outstr;

  char run_xsum = 0;
  int32_t seq;

  ptr = pkt->GetPayload();
  size_t size = pkt->GetPayloadSize();

  if (!pkt->GetSequence(&seq) && (GetFlags() & USE_SEQ)) {
    pkt->SetSequence(seq_++);
  }

  // Signal start of response
  outstr << '$';

  // If there is a sequence, send as two nibble 8bit value + ':'
  if (pkt->GetSequence(&seq)) {
    IntToNibble((seq & 0xFF) >> 4, &ch);
    outstr << ch;
    run_xsum += ch;

    IntToNibble(seq & 0xF, &ch);
    outstr << ch;
    run_xsum += ch;

    ch = ':';
    outstr << ch;
    run_xsum += ch;
  }

  // Send the main payload
  for (size_t offs = 0; offs < size; ++offs) {
    ch = ptr[offs];
    outstr << ch;
    run_xsum += ch;
  }

  if (GetFlags() & DEBUG_SEND) {
    NaClLog(1, "TX %s\n", outstr.str().c_str());
  }

  // Send XSUM as two nible 8bit value preceeded by '#'
  outstr << '#';
  IntToNibble((run_xsum >> 4) & 0xF, &ch);
  outstr << ch;
  IntToNibble(run_xsum & 0xF, &ch);
  outstr << ch;

  return io_->Write(outstr.str().data(),
                    static_cast<int32_t>(outstr.str().length()));
}

// Attempt to receive a packet
bool Session::GetPacket(Packet *pkt) {
  char run_xsum, fin_xsum, ch;
  std::string in;

  // Toss characters until we see a start of command
  do {
    if (!GetChar(&ch)) return false;
    in += ch;
  } while (ch != '$');

 retry:
  // Clear the stream
  pkt->Clear();

  // Prepare XSUM calc
  run_xsum = 0;
  fin_xsum = 0;

  // Stream in the characters
  while (1) {
    if (!GetChar(&ch)) return false;

    // If we see a '#' we must be done with the data
    if (ch == '#') break;

    in += ch;

    // If we see a '$' we must have missed the last cmd
    if (ch == '$') {
      NaClLog(LOG_INFO, "RX Missing $, retry.\n");
      goto retry;
    }
    // Keep a running XSUM
    run_xsum += ch;
    pkt->AddRawChar(ch);
  }


  // Get two Nibble XSUM
  if (!GetChar(&ch)) return false;

  int val;
  NibbleToInt(ch, & val);
  fin_xsum = val << 4;

  if (!GetChar(&ch)) return false;
  NibbleToInt(ch, &val);
  fin_xsum |= val;

  if (GetFlags() & DEBUG_RECV) NaClLog(1, "RX %s\n", in.c_str());

  pkt->ParseSequence();

  // If ACKs are off, we are done.
  if (GetFlags() & IGNORE_ACK) return true;

  // If the XSUMs don't match, signal bad packet
  if (fin_xsum == run_xsum) {
    char out[3] = { '+', 0, 0 };
    int32_t seq;

    // If we have a sequence number
    if (pkt->GetSequence(&seq)) {
      // Respond with Sequence number
      IntToNibble(seq >> 4, &out[1]);
      IntToNibble(seq & 0xF, &out[2]);
      return io_->Write(out, 3);
    }
    return io_->Write(out, 1);
  } else {
    // Resend a bad XSUM and look for retransmit
    io_->Write("-", 1);

    NaClLog(LOG_INFO, "RX Bad XSUM, retry\n");
    goto retry;
  }

  return true;
}

}  // End of namespace gdb_rsp

