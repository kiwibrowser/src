// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_tcp_socket.h"

#include <vector>

#include "ppapi/cpp/message_loop.h"
#include "ppapi/cpp/tcp_socket.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/tests/testing_instance.h"

namespace {

// Validates the first line of an HTTP response.
bool ValidateHttpResponse(const std::string& s) {
  // Just check that it begins with "HTTP/" and ends with a "\r\n".
  return s.size() >= 5 &&
         s.substr(0, 5) == "HTTP/" &&
         s.substr(s.size() - 2) == "\r\n";
}

}  // namespace

REGISTER_TEST_CASE(TCPSocket);

TestTCPSocket::TestTCPSocket(TestingInstance* instance)
    : TestCase(instance),
      socket_interface_1_0_(NULL) {
}

bool TestTCPSocket::Init() {
  if (!pp::TCPSocket::IsAvailable())
    return false;
  socket_interface_1_0_ =
      static_cast<const PPB_TCPSocket_1_0*>(
          pp::Module::Get()->GetBrowserInterface(PPB_TCPSOCKET_INTERFACE_1_0));
  if (!socket_interface_1_0_)
    return false;

  // We need something to connect to, so we connect to the HTTP server whence we
  // came. Grab the host and port.
  if (!EnsureRunningOverHTTP())
    return false;

  std::string host;
  uint16_t port = 0;
  if (!GetLocalHostPort(instance_->pp_instance(), &host, &port))
    return false;

  if (!ResolveHost(instance_->pp_instance(), host, port, &addr_))
    return false;

  return true;
}

void TestTCPSocket::RunTests(const std::string& filter) {
  RUN_CALLBACK_TEST(TestTCPSocket, Connect, filter);
  RUN_CALLBACK_TEST(TestTCPSocket, ReadWrite, filter);
  RUN_CALLBACK_TEST(TestTCPSocket, SetOption, filter);
  RUN_CALLBACK_TEST(TestTCPSocket, Listen, filter);
  RUN_CALLBACK_TEST(TestTCPSocket, Backlog, filter);
  RUN_CALLBACK_TEST(TestTCPSocket, Interface_1_0, filter);
}

std::string TestTCPSocket::TestConnect() {
  {
    // The basic case.
    pp::TCPSocket socket(instance_);
    TestCompletionCallback cb(instance_->pp_instance(), callback_type());

    cb.WaitForResult(socket.Connect(addr_, cb.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(cb);
    ASSERT_EQ(PP_OK, cb.result());

    pp::NetAddress local_addr, remote_addr;
    local_addr = socket.GetLocalAddress();
    remote_addr = socket.GetRemoteAddress();

    ASSERT_NE(0, local_addr.pp_resource());
    ASSERT_NE(0, remote_addr.pp_resource());
    ASSERT_TRUE(EqualNetAddress(addr_, remote_addr));

    socket.Close();
  }

  {
    // Connect a bound socket.
    pp::TCPSocket socket(instance_);
    TestCompletionCallback cb(instance_->pp_instance(), callback_type());

    pp::NetAddress any_port_address;
    ASSERT_SUBTEST_SUCCESS(GetAddressToBind(&any_port_address));

    cb.WaitForResult(socket.Bind(any_port_address, cb.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(cb);
    ASSERT_EQ(PP_OK, cb.result());

    cb.WaitForResult(socket.Connect(addr_, cb.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(cb);
    ASSERT_EQ(PP_OK, cb.result());

    pp::NetAddress local_addr, remote_addr;
    local_addr = socket.GetLocalAddress();
    remote_addr = socket.GetRemoteAddress();

    ASSERT_NE(0, local_addr.pp_resource());
    ASSERT_NE(0, remote_addr.pp_resource());
    ASSERT_TRUE(EqualNetAddress(addr_, remote_addr));
    ASSERT_NE(0u, GetPort(local_addr));

    socket.Close();
  }

  PASS();
}

std::string TestTCPSocket::TestReadWrite() {
  pp::TCPSocket socket(instance_);
  TestCompletionCallback cb(instance_->pp_instance(), callback_type());

  cb.WaitForResult(socket.Connect(addr_, cb.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(cb);
  ASSERT_EQ(PP_OK, cb.result());

  ASSERT_SUBTEST_SUCCESS(WriteToSocket(&socket, "GET / HTTP/1.0\r\n\r\n"));

  // Read up to the first \n and check that it looks like valid HTTP response.
  std::string s;
  ASSERT_SUBTEST_SUCCESS(ReadFirstLineFromSocket(&socket, &s));
  ASSERT_TRUE(ValidateHttpResponse(s));

  PASS();
}

std::string TestTCPSocket::TestSetOption() {
  pp::TCPSocket socket(instance_);
  TestCompletionCallback cb_1(instance_->pp_instance(), callback_type());
  TestCompletionCallback cb_2(instance_->pp_instance(), callback_type());
  TestCompletionCallback cb_3(instance_->pp_instance(), callback_type());

  // These options can be set even before the socket is connected.
  int32_t result_1 = socket.SetOption(PP_TCPSOCKET_OPTION_NO_DELAY,
                                      true, cb_1.GetCallback());
  int32_t result_2 = socket.SetOption(PP_TCPSOCKET_OPTION_SEND_BUFFER_SIZE,
                                      256, cb_2.GetCallback());
  int32_t result_3 = socket.SetOption(PP_TCPSOCKET_OPTION_RECV_BUFFER_SIZE,
                                      512, cb_3.GetCallback());

  cb_1.WaitForResult(result_1);
  CHECK_CALLBACK_BEHAVIOR(cb_1);
  ASSERT_EQ(PP_OK, cb_1.result());

  cb_2.WaitForResult(result_2);
  CHECK_CALLBACK_BEHAVIOR(cb_2);
  ASSERT_EQ(PP_OK, cb_2.result());

  cb_3.WaitForResult(result_3);
  CHECK_CALLBACK_BEHAVIOR(cb_3);
  ASSERT_EQ(PP_OK, cb_3.result());

  cb_1.WaitForResult(socket.Connect(addr_, cb_1.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(cb_1);
  ASSERT_EQ(PP_OK, cb_1.result());

  result_1 = socket.SetOption(PP_TCPSOCKET_OPTION_NO_DELAY,
                              false, cb_1.GetCallback());
  result_2 = socket.SetOption(PP_TCPSOCKET_OPTION_SEND_BUFFER_SIZE,
                              512, cb_2.GetCallback());
  result_3 = socket.SetOption(PP_TCPSOCKET_OPTION_RECV_BUFFER_SIZE,
                              1024, cb_3.GetCallback());

  cb_1.WaitForResult(result_1);
  CHECK_CALLBACK_BEHAVIOR(cb_1);
  ASSERT_EQ(PP_OK, cb_1.result());

  cb_2.WaitForResult(result_2);
  CHECK_CALLBACK_BEHAVIOR(cb_2);
  ASSERT_EQ(PP_OK, cb_2.result());

  cb_3.WaitForResult(result_3);
  CHECK_CALLBACK_BEHAVIOR(cb_3);
  ASSERT_EQ(PP_OK, cb_3.result());

  PASS();
}

std::string TestTCPSocket::TestListen() {
  static const int kBacklog = 2;

  pp::TCPSocket server_socket(instance_);
  ASSERT_SUBTEST_SUCCESS(StartListen(&server_socket, kBacklog));

  // We can't use a blocking callback for Accept, because it will wait forever
  // for the client to connect, since the client connects after.
  TestCompletionCallbackWithOutput<pp::TCPSocket>
      accept_callback(instance_->pp_instance(), PP_REQUIRED);
  // We need to make sure there's a message loop to run accept_callback on.
  pp::MessageLoop current_thread_loop(pp::MessageLoop::GetCurrent());
  if (current_thread_loop.is_null() && testing_interface_->IsOutOfProcess()) {
    current_thread_loop = pp::MessageLoop(instance_);
    current_thread_loop.AttachToCurrentThread();
  }

  int32_t accept_rv = server_socket.Accept(accept_callback.GetCallback());

  pp::TCPSocket client_socket;
  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  do {
    client_socket = pp::TCPSocket(instance_);

    callback.WaitForResult(client_socket.Connect(
        server_socket.GetLocalAddress(), callback.GetCallback()));
  } while (callback.result() != PP_OK);

  pp::NetAddress client_local_addr = client_socket.GetLocalAddress();
  pp::NetAddress client_remote_addr = client_socket.GetRemoteAddress();
  ASSERT_FALSE(client_local_addr.is_null());
  ASSERT_FALSE(client_remote_addr.is_null());

  accept_callback.WaitForResult(accept_rv);
  CHECK_CALLBACK_BEHAVIOR(accept_callback);
  ASSERT_EQ(PP_OK, accept_callback.result());

  pp::TCPSocket accepted_socket(accept_callback.output());
  pp::NetAddress accepted_local_addr = accepted_socket.GetLocalAddress();
  pp::NetAddress accepted_remote_addr = accepted_socket.GetRemoteAddress();
  ASSERT_FALSE(accepted_local_addr.is_null());
  ASSERT_FALSE(accepted_remote_addr.is_null());

  ASSERT_TRUE(EqualNetAddress(client_local_addr, accepted_remote_addr));

  const char kSentByte = 'a';
  ASSERT_SUBTEST_SUCCESS(WriteToSocket(&client_socket,
                                       std::string(1, kSentByte)));

  char received_byte;
  ASSERT_SUBTEST_SUCCESS(ReadFromSocket(&accepted_socket,
                                        &received_byte,
                                        sizeof(received_byte)));
  ASSERT_EQ(kSentByte, received_byte);

  accepted_socket.Close();
  client_socket.Close();
  server_socket.Close();

  PASS();
}

std::string TestTCPSocket::TestBacklog() {
  static const size_t kBacklog = 5;

  pp::TCPSocket server_socket(instance_);
  ASSERT_SUBTEST_SUCCESS(StartListen(&server_socket, 2 * kBacklog));

  std::vector<pp::TCPSocket*> client_sockets(kBacklog);
  std::vector<TestCompletionCallback*> connect_callbacks(kBacklog);
  std::vector<int32_t> connect_rv(kBacklog);
  pp::NetAddress address = server_socket.GetLocalAddress();
  for (size_t i = 0; i < kBacklog; ++i) {
    client_sockets[i] = new pp::TCPSocket(instance_);
    connect_callbacks[i] = new TestCompletionCallback(instance_->pp_instance(),
                                                      callback_type());
    connect_rv[i] = client_sockets[i]->Connect(
        address, connect_callbacks[i]->GetCallback());
  }

  std::vector<pp::TCPSocket*> accepted_sockets(kBacklog);
  for (size_t i = 0; i < kBacklog; ++i) {
    TestCompletionCallbackWithOutput<pp::TCPSocket> callback(
        instance_->pp_instance(), callback_type());
    callback.WaitForResult(server_socket.Accept(callback.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(callback);
    ASSERT_EQ(PP_OK, callback.result());

    accepted_sockets[i] = new pp::TCPSocket(callback.output());
    ASSERT_FALSE(accepted_sockets[i]->is_null());
  }

  for (size_t i = 0; i < kBacklog; ++i) {
    connect_callbacks[i]->WaitForResult(connect_rv[i]);
    CHECK_CALLBACK_BEHAVIOR(*connect_callbacks[i]);
    ASSERT_EQ(PP_OK, connect_callbacks[i]->result());
  }

  for (size_t i = 0; i < kBacklog; ++i) {
    const char byte = static_cast<char>('a' + i);
    ASSERT_SUBTEST_SUCCESS(WriteToSocket(client_sockets[i],
                                         std::string(1, byte)));
  }

  bool byte_received[kBacklog] = {};
  for (size_t i = 0; i < kBacklog; ++i) {
    char byte;
    ASSERT_SUBTEST_SUCCESS(ReadFromSocket(
        accepted_sockets[i], &byte, sizeof(byte)));
    const size_t index = byte - 'a';
    ASSERT_GE(index, 0u);
    ASSERT_LT(index, kBacklog);
    ASSERT_FALSE(byte_received[index]);
    byte_received[index] = true;
  }

  for (size_t i = 0; i < kBacklog; ++i) {
    ASSERT_TRUE(byte_received[i]);

    delete client_sockets[i];
    delete connect_callbacks[i];
    delete accepted_sockets[i];
  }

  PASS();
}

std::string TestTCPSocket::TestInterface_1_0() {
  PP_Resource socket = socket_interface_1_0_->Create(instance_->pp_instance());
  ASSERT_NE(0, socket);

  TestCompletionCallback cb(instance_->pp_instance(), callback_type());
  cb.WaitForResult(socket_interface_1_0_->Connect(
      socket, addr_.pp_resource(), cb.GetCallback().pp_completion_callback()));
  CHECK_CALLBACK_BEHAVIOR(cb);
  ASSERT_EQ(PP_OK, cb.result());

  ASSERT_SUBTEST_SUCCESS(WriteToSocket_1_0(socket, "GET / HTTP/1.0\r\n\r\n"));

  // Read up to the first \n and check that it looks like valid HTTP response.
  std::string s;
  ASSERT_SUBTEST_SUCCESS(ReadFirstLineFromSocket_1_0(socket, &s));
  ASSERT_TRUE(ValidateHttpResponse(s));

  pp::Module::Get()->core()->ReleaseResource(socket);
  PASS();
}

std::string TestTCPSocket::ReadFirstLineFromSocket(pp::TCPSocket* socket,
                                                   std::string* s) {
  char buffer[1000];

  s->clear();
  // Make sure we don't just hang if |Read()| spews.
  while (s->size() < 10000) {
    TestCompletionCallback cb(instance_->pp_instance(), callback_type());
    cb.WaitForResult(socket->Read(buffer, sizeof(buffer), cb.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(cb);
    ASSERT_GT(cb.result(), 0);
    s->reserve(s->size() + cb.result());
    for (int32_t i = 0; i < cb.result(); ++i) {
      s->push_back(buffer[i]);
      if (buffer[i] == '\n')
        PASS();
    }
  }
  PASS();
}

std::string TestTCPSocket::ReadFirstLineFromSocket_1_0(PP_Resource socket,
                                                       std::string* s) {
  char buffer[1000];

  s->clear();
  // Make sure we don't just hang if |Read()| spews.
  while (s->size() < 10000) {
    TestCompletionCallback cb(instance_->pp_instance(), callback_type());
    cb.WaitForResult(socket_interface_1_0_->Read(
        socket, buffer, sizeof(buffer),
        cb.GetCallback().pp_completion_callback()));
    CHECK_CALLBACK_BEHAVIOR(cb);
    ASSERT_GT(cb.result(), 0);
    s->reserve(s->size() + cb.result());
    for (int32_t i = 0; i < cb.result(); ++i) {
      s->push_back(buffer[i]);
      if (buffer[i] == '\n')
        PASS();
    }
  }
  PASS();
}

std::string TestTCPSocket::ReadFromSocket(pp::TCPSocket* socket,
                                          char* buffer,
                                          size_t num_bytes) {
  while (num_bytes > 0) {
    TestCompletionCallback callback(instance_->pp_instance(), callback_type());
    callback.WaitForResult(
        socket->Read(buffer, static_cast<int32_t>(num_bytes),
        callback.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(callback);
    ASSERT_GT(callback.result(), 0);
    buffer += callback.result();
    num_bytes -= callback.result();
  }
  ASSERT_EQ(0u, num_bytes);
  PASS();
}

std::string TestTCPSocket::WriteToSocket(pp::TCPSocket* socket,
                                         const std::string& s) {
  const char* buffer = s.data();
  size_t written = 0;
  while (written < s.size()) {
    TestCompletionCallback cb(instance_->pp_instance(), callback_type());
    cb.WaitForResult(
        socket->Write(buffer + written,
                      static_cast<int32_t>(s.size() - written),
                      cb.GetCallback()));
    CHECK_CALLBACK_BEHAVIOR(cb);
    ASSERT_GT(cb.result(), 0);
    written += cb.result();
  }
  ASSERT_EQ(written, s.size());
  PASS();
}

std::string TestTCPSocket::WriteToSocket_1_0(
    PP_Resource socket,
    const std::string& s) {
  const char* buffer = s.data();
  size_t written = 0;
  while (written < s.size()) {
    TestCompletionCallback cb(instance_->pp_instance(), callback_type());
    cb.WaitForResult(socket_interface_1_0_->Write(
        socket, buffer + written,
        static_cast<int32_t>(s.size() - written),
        cb.GetCallback().pp_completion_callback()));
    CHECK_CALLBACK_BEHAVIOR(cb);
    ASSERT_GT(cb.result(), 0);
    written += cb.result();
  }
  ASSERT_EQ(written, s.size());
  PASS();
}

std::string TestTCPSocket::GetAddressToBind(pp::NetAddress* address) {
  pp::TCPSocket socket(instance_);
  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(socket.Connect(addr_, callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  ASSERT_TRUE(ReplacePort(instance_->pp_instance(), socket.GetLocalAddress(), 0,
                          address));
  ASSERT_FALSE(address->is_null());
  PASS();
}

std::string TestTCPSocket::StartListen(pp::TCPSocket* socket, int32_t backlog) {
  pp::NetAddress any_port_address;
  ASSERT_SUBTEST_SUCCESS(GetAddressToBind(&any_port_address));

  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(
      socket->Bind(any_port_address, callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  callback.WaitForResult(
      socket->Listen(backlog, callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  PASS();
}
