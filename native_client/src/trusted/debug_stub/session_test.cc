/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <sstream>
#include <vector>

#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/session.h"
#include "native_client/src/trusted/debug_stub/test.h"

using gdb_rsp::Session;
using gdb_rsp::Packet;

// Transport simulation class, this stores data and a r/w index
// to simulate one direction of a pipe, or a pipe to self.
class SharedVector {
 public:
  SharedVector() : rd(0), wr(0) {}

 public:
  std::vector<char> data;
  volatile uint32_t rd;
  volatile uint32_t wr;
};

// Simulates a transport (such as a socket), the reports "ready"
// when polled, but fails on TX/RX.
class DCSocketTransport : public port::ITransport {
 public:
  virtual bool Read(void *ptr, int32_t len) {
    (void) ptr;
    (void) len;
    return false;
  }

  virtual bool Write(const void *ptr, int32_t len) {
    (void) ptr;
    (void) len;
    return false;
  }

  virtual bool IsDataAvailable() {
    return true;
  }

  virtual void WaitForDebugStubEvent(struct NaClApp *nap,
                                     bool ignore_gdb) {
    UNREFERENCED_PARAMETER(nap);
    UNREFERENCED_PARAMETER(ignore_gdb);
  }

  virtual void Disconnect() {}

  virtual bool AcceptConnection() {
    return true;
  }
};


// Simulate a transport transmitting data Q'd in TX and verifying that
// inbound data matches expected "golden" string.
class GoldenTransport : public DCSocketTransport {
 public:
  GoldenTransport(const char *rx, const char *tx, int cnt) {
    rx_ = rx;
    tx_ = tx;
    cnt_ = cnt;
    txCnt_ = 0;
    rxCnt_ = 0;
    errs_ = 0;
    disconnected_ = false;
  }

  virtual bool Read(void *ptr, int32_t len) {
    if (disconnected_) return false;
    memcpy(ptr, &rx_[rxCnt_], len);
    rxCnt_ += len;
    if (static_cast<int>(strlen(rx_)) < rxCnt_) {
      printf("End of RX\n");
      errs_++;
    }
    return true;
  }

  //  Read from this link, return a negative value if there is an error
  virtual bool Write(const void *ptr, int32_t len) {
    const char *str = reinterpret_cast<const char *>(ptr);
    if (disconnected_) return false;
    if (strncmp(str, &tx_[txCnt_], len) != 0) {
      printf("TX mismatch in %s vs %s.\n", str, &tx_[txCnt_]);
      errs_++;
    }
    txCnt_ += len;
    return true;
  }

  virtual bool IsDataAvailable() {
    if (disconnected_) return true;

    return rxCnt_ < static_cast<int>(strlen(rx_));
  }

  virtual void Disconnect() {
    disconnected_ = true;
  }

  int errs() { return errs_; }


 protected:
  const char *rx_;
  const char *tx_;
  int cnt_;
  int rxCnt_;
  int txCnt_;
  int errs_;
  bool disconnected_;
};


class TestTransport : public DCSocketTransport {
 public:
  TestTransport(SharedVector *rvec, SharedVector *wvec) {
    rvector_ = rvec;
    wvector_ = wvec;
    disconnected_ = false;
  }

  virtual bool Read(void *ptr, int32_t len) {
    if (disconnected_) return false;

    int max = rvector_->wr - rvector_->rd;
    if (max > len) {
      max = len;
    } else {
      return false;
    }

    if (max > 0) {
      char *src = &rvector_->data[rvector_->rd];
      memcpy(ptr, src, max);
    }
    rvector_->rd += max;
    return true;
  }

  virtual bool Write(const void *ptr, int32_t len) {
    if (disconnected_) return false;

    wvector_->data.resize(wvector_->wr + len);
    memcpy(&wvector_->data[wvector_->wr], ptr, len);
    wvector_->wr += len;
    return true;
  }

  virtual bool IsDataAvailable() {
    if (disconnected_) return true;

    return rvector_->rd < rvector_->wr;
  }

  virtual void Disconnect() {
    disconnected_ = true;
  }

 protected:
  SharedVector *rvector_;
  SharedVector *wvector_;
  bool disconnected_;
};


int TestSession() {
  int errs = 0;
  Packet pktOut;
  Packet pktIn;
  SharedVector vec;

  // Create a "loopback" session by using the same
  // FIFO for ingress and egress.
  Session cli(new TestTransport(&vec, &vec));
  Session srv(new TestTransport(&vec, &vec));

  // Check, Set,Clear,Get flags.
  cli.ClearFlags(static_cast<uint32_t>(-1));
  cli.SetFlags(Session::IGNORE_ACK | Session::DEBUG_RECV);
  if (cli.GetFlags() != (Session::IGNORE_ACK + Session::DEBUG_RECV)) {
    printf("SetFlag failed.\n");
    errs++;
  }
  cli.ClearFlags(Session::IGNORE_ACK | Session::DEBUG_SEND);
  if (cli.GetFlags() != Session::DEBUG_RECV) {
    printf("ClearFlag failed.\n");
    errs++;
  }

  // Check Send Packet of known value.
  const char *str = "1234";

  pktOut.AddString(str);
  cli.SendPacketOnly(&pktOut);
  srv.GetPacket(&pktIn);
  std::string out;
  pktIn.GetString(&out);
  if (out != str) {
    printf("Send Only failed.\n");
    errs++;
  }

  // Check send against golden transactions
  const char tx[] = { "$1234#ca+" };
  const char rx[] = { "+$OK#9a" };
  GoldenTransport gold(rx, tx, 2);
  Session uni(&gold);

  pktOut.Clear();
  pktOut.AddString(str);
  if (!uni.SendPacket(&pktOut)) {
    printf("Send failed.\n");
    errs++;
  }
  if (!uni.GetPacket(&pktIn)) {
    printf("Get failed.\n");
    errs++;
  }
  pktIn.GetString(&out);
  if (out != "OK") {
    printf("Send/Get failed.\n");
    errs++;
  }

  // Check that a closed Transport reports to session
  if (!uni.Connected()) {
    printf("Expecting uni to be connected.\n");
    errs++;
  }
  gold.Disconnect();
  uni.GetPacket(&pktIn);
  if (uni.Connected()) {
    printf("Expecting uni to be disconnected.\n");
    errs++;
  }

  // Check that a failed read/write reports DC
  DCSocketTransport dctrans;
  Session dctest(&dctrans);
  if (!dctest.Connected()) {
    printf("Expecting dctest to be connected.\n");
    errs++;
  }
  dctest.GetPacket(&pktIn);
  if (dctest.Connected()) {
    printf("Expecting dctest to be disconnected.\n");
    errs++;
  }

  errs += gold.errs();
  return errs;
}

