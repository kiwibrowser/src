// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <vector>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "net/quic/quic_chromium_alarm_factory.h"
#include "net/third_party/quic/core/quic_constants.h"
#include "net/third_party/quic/platform/impl/quic_chromium_clock.h"
#include "net/third_party/quic/quartc/quartc_factory.h"
#include "third_party/chromium_quic/demo/delegates.h"

int main(int argc, char** argv) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  logging::LoggingSettings logging_settings;
  logging_settings.logging_dest = logging::LOG_TO_FILE;
  logging_settings.log_file = "/dev/stderr";
  logging_settings.lock_log = logging::DONT_LOCK_LOG_FILE;
  logging::InitLogging(logging_settings);

  if (argc < 2) {
    dprintf(STDERR_FILENO,
            "Missing port number\nusage: demo_client <server-port>\n");
    return 1;
  }
  int port = atoi(argv[1]);
  if (port <= 0 || port > UINT16_MAX) {
    dprintf(STDERR_FILENO, "Invalid port number: %d\n", port);
    return 1;
  }

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) {
    perror("socket()");
    return 1;
  }
  // On non-Linux, the SOCK_NONBLOCK option is not available, so use the
  // more-portable method of calling fcntl() to set this behavior.
  if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
    perror("fcntl(F_SETFL, +O_NONBLOCK)");
    return 1;
  }

  struct sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_port = 0;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int reuse = 1;
  int result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  if (result == -1) {
    perror("setsockopt()");
    return 1;
  }
  result = bind(fd, (struct sockaddr*)(&address), sizeof(address));
  if (result == -1) {
    perror("bind()");
    return 1;
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(static_cast<uint16_t>(port));
  server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  UdpTransport transport(fd, server_address);
  auto* clock = quic::QuicChromiumClock::GetInstance();
  std::vector<FakeTask> tasks;
  scoped_refptr<FakeTaskRunner> task_runner =
      base::MakeRefCounted<FakeTaskRunner>(&tasks);
  auto alarm_factory =
      std::make_unique<net::QuicChromiumAlarmFactory>(task_runner.get(), clock);
  quic::QuartcFactoryConfig factory_config;
  factory_config.alarm_factory = alarm_factory.get();
  factory_config.clock = clock;
  quic::QuartcFactory factory(factory_config);

  quic::QuartcSessionConfig session_config;
  session_config.perspective = quic::Perspective::IS_CLIENT;
  session_config.unique_remote_server_id = "asdf";
  session_config.packet_transport = &transport;
  // rest are defaults

  auto session = factory.CreateQuartcSession(session_config);
  SessionDelegate session_delegate;
  session->SetDelegate(&session_delegate);

  StreamDelegate outgoing_delegate;

  session->OnTransportCanWrite();
  session->StartCryptoHandshake();
  auto last = clock->WallNow();
  bool once = false;
  while (true) {
    auto now = clock->WallNow();
    for (auto it = tasks.begin(); it != tasks.end();) {
      auto& next_task = *it;
      next_task.delay -= base::TimeDelta::FromMicroseconds(
          now.ToUNIXMicroseconds() - last.ToUNIXMicroseconds());
      if (next_task.delay.InMicroseconds() < 0) {
        printf("task: %s\n", next_task.whence.ToString().c_str());
        std::move(next_task.task).Run();
        it = tasks.erase(it);
      } else {
        ++it;
      }
    }
    last = now;
    // NOTE: The stream will not close immediately on send, because it waits for
    // ACKs, including for the fin bit.  Calling CloseConnection, however,
    // immediately closes the connection so it's safe to exit immediately here.
    if (outgoing_delegate.closed()) {
      session->CloseConnection("all your base");
      break;
    }
    struct sockaddr_in peer_address;
    socklen_t addrlen = sizeof(peer_address);
    char buffer[quic::kMaxPacketSize] = {};
    int buffer_length = sizeof(buffer);
    result = recvfrom(fd, buffer, buffer_length, 0,
                      (struct sockaddr*)(&peer_address), &addrlen);
    if ((result == -1 && errno != EWOULDBLOCK && errno != EAGAIN) ||
        addrlen != sizeof(peer_address)) {
      perror("recvfrom()");
      return 1;
    } else if (result > 0) {
      printf("recvfrom()\n");
      session->OnTransportReceived(buffer, result);
      if (session->IsCryptoHandshakeConfirmed() &&
          session->IsEncryptionEstablished() && !once) {
        auto* stream = session->CreateOutgoingDynamicStream();
        stream->SetDelegate(&outgoing_delegate);

        // TODO(btolsch): This seems odd.  Can we do better when reimplementing
        // platform/impl?
        // TODO(btolsch): Specify the number of copies/refcounts per write.
        char data[] = "turtle";
        auto buffer = base::MakeRefCounted<net::IOBuffer>(sizeof(data) - 1);
        int length = sizeof(data) - 1;
        memcpy(buffer->data(), data, sizeof(data) - 1);
        stream->WriteMemSlices(
            quic::QuicMemSliceSpan(
                quic::QuicMemSliceSpanImpl(&buffer, &length, 1)),
            true);
        once = true;
      }
    }
  }

  return 0;
}
