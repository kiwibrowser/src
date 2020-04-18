// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_TCP_SERVER_SOCKET_PRIVATE_H_
#define PPAPI_TESTS_TEST_TCP_SERVER_SOCKET_PRIVATE_H_

#include <stddef.h>

#include <cstddef>
#include <string>

#include "ppapi/c/pp_stdint.h"
#include "ppapi/tests/test_case.h"

struct PP_NetAddress_Private;

namespace pp {

class TCPServerSocketPrivate;
class TCPSocketPrivate;

}  // namespace pp

class TestTCPServerSocketPrivate : public TestCase {
 public:
  explicit TestTCPServerSocketPrivate(TestingInstance* instance);

  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

 private:
  std::string GetLocalAddress(PP_NetAddress_Private* address);
  std::string SyncRead(pp::TCPSocketPrivate* socket,
                       char* buffer,
                       size_t num_bytes);
  std::string SyncWrite(pp::TCPSocketPrivate* socket,
                        const char* buffer,
                        size_t num_bytes);
  std::string SyncConnect(pp::TCPSocketPrivate* socket,
                          PP_NetAddress_Private* address);
  void ForceConnect(pp::TCPSocketPrivate* socket,
                    PP_NetAddress_Private* address);
  std::string SyncListen(pp::TCPServerSocketPrivate* socket,
                         PP_NetAddress_Private* address,
                         int32_t backlog);

  std::string TestListen();
  std::string TestBacklog();

  std::string host_;
  uint16_t port_;
};

#endif  // PPAPI_TESTS_TEST_TCP_SERVER_SOCKET_PRIVATE_H_
