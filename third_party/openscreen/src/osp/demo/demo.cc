// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "osp/msgs/osp_messages.h"
#include "osp/public/mdns_service_listener_factory.h"
#include "osp/public/mdns_service_publisher_factory.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/presentation/presentation_controller.h"
#include "osp/public/presentation/presentation_receiver.h"
#include "osp/public/protocol_connection_client.h"
#include "osp/public/protocol_connection_client_factory.h"
#include "osp/public/protocol_connection_server.h"
#include "osp/public/protocol_connection_server_factory.h"
#include "osp/public/service_listener.h"
#include "osp/public/service_publisher.h"
#include "platform/api/logging.h"
#include "platform/api/network_interface.h"
#include "platform/api/time.h"
#include "third_party/tinycbor/src/src/cbor.h"

namespace openscreen {
namespace {

const char* kReceiverLogFilename = "_recv_fifo";
const char* kControllerLogFilename = "_cntl_fifo";

bool g_done = false;
bool g_dump_services = false;

void sigusr1_dump_services(int) {
  g_dump_services = true;
}

void sigint_stop(int) {
  OSP_LOG << "caught SIGINT, exiting...";
  g_done = true;
}

void SignalThings() {
  struct sigaction usr1_sa;
  struct sigaction int_sa;
  struct sigaction unused;

  usr1_sa.sa_handler = &sigusr1_dump_services;
  sigemptyset(&usr1_sa.sa_mask);
  usr1_sa.sa_flags = 0;

  int_sa.sa_handler = &sigint_stop;
  sigemptyset(&int_sa.sa_mask);
  int_sa.sa_flags = 0;

  sigaction(SIGUSR1, &usr1_sa, &unused);
  sigaction(SIGINT, &int_sa, &unused);

  OSP_LOG << "signal handlers setup" << std::endl << "pid: " << getpid();
}

class ListenerObserver final : public ServiceListener::Observer {
 public:
  ~ListenerObserver() override = default;
  void OnStarted() override { OSP_LOG << "listener started!"; }
  void OnStopped() override { OSP_LOG << "listener stopped!"; }
  void OnSuspended() override { OSP_LOG << "listener suspended!"; }
  void OnSearching() override { OSP_LOG << "listener searching!"; }

  void OnReceiverAdded(const ServiceInfo& info) override {
    OSP_LOG << "found! " << info.friendly_name;
  }
  void OnReceiverChanged(const ServiceInfo& info) override {
    OSP_LOG << "changed! " << info.friendly_name;
  }
  void OnReceiverRemoved(const ServiceInfo& info) override {
    OSP_LOG << "removed! " << info.friendly_name;
  }
  void OnAllReceiversRemoved() override { OSP_LOG << "all removed!"; }
  void OnError(ServiceListenerError) override {}
  void OnMetrics(ServiceListener::Metrics) override {}
};

std::string SanitizeServiceId(absl::string_view service_id) {
  std::string safe_service_id(service_id);
  for (auto& c : safe_service_id) {
    if (c < ' ' || c > '~') {
      c = '.';
    }
  }
  return safe_service_id;
}

class ReceiverObserver final : public presentation::ReceiverObserver {
 public:
  ~ReceiverObserver() override = default;

  void OnRequestFailed(const std::string& presentation_url,
                       const std::string& service_id) override {
    std::string safe_service_id = SanitizeServiceId(service_id);
    OSP_LOG_WARN << "request failed: (" << presentation_url << ", "
                 << safe_service_id << ")";
  }
  void OnReceiverAvailable(const std::string& presentation_url,
                           const std::string& service_id) override {
    std::string safe_service_id = SanitizeServiceId(service_id);
    safe_service_ids_.emplace(safe_service_id, service_id);
    OSP_LOG << "available! " << safe_service_id;
  }
  void OnReceiverUnavailable(const std::string& presentation_url,
                             const std::string& service_id) override {
    std::string safe_service_id = SanitizeServiceId(service_id);
    safe_service_ids_.erase(safe_service_id);
    OSP_LOG << "unavailable! " << safe_service_id;
  }

  const std::string& GetServiceId(const std::string& safe_service_id) {
    OSP_DCHECK(safe_service_ids_.find(safe_service_id) !=
               safe_service_ids_.end())
        << safe_service_id << " not found in map";
    return safe_service_ids_[safe_service_id];
  }

 private:
  std::map<std::string, std::string> safe_service_ids_;
};

class PublisherObserver final : public ServicePublisher::Observer {
 public:
  ~PublisherObserver() override = default;

  void OnStarted() override { OSP_LOG << "publisher started!"; }
  void OnStopped() override { OSP_LOG << "publisher stopped!"; }
  void OnSuspended() override { OSP_LOG << "publisher suspended!"; }

  void OnError(ServicePublisherError) override {}
  void OnMetrics(ServicePublisher::Metrics) override {}
};

class ConnectionClientObserver final
    : public ProtocolConnectionServiceObserver {
 public:
  ~ConnectionClientObserver() override = default;
  void OnRunning() override {}
  void OnStopped() override {}

  void OnMetrics(const NetworkMetrics& metrics) override {}
  void OnError(const Error& error) override {}
};

class ConnectionServerObserver final
    : public ProtocolConnectionServer::Observer {
 public:
  class ConnectionObserver final : public ProtocolConnection::Observer {
   public:
    explicit ConnectionObserver(ConnectionServerObserver* parent)
        : parent_(parent) {}
    ~ConnectionObserver() override = default;

    void OnConnectionClosed(const ProtocolConnection& connection) override {
      auto& connections = parent_->connections_;
      connections.erase(
          std::remove_if(
              connections.begin(), connections.end(),
              [this](const std::pair<std::unique_ptr<ConnectionObserver>,
                                     std::unique_ptr<ProtocolConnection>>& p) {
                return p.first.get() == this;
              }),
          connections.end());
    }

   private:
    ConnectionServerObserver* const parent_;
  };

  ~ConnectionServerObserver() override = default;

  void OnRunning() override {}
  void OnStopped() override {}
  void OnSuspended() override {}

  void OnMetrics(const NetworkMetrics& metrics) override {}
  void OnError(const Error& error) override {}

  void OnIncomingConnection(
      std::unique_ptr<ProtocolConnection> connection) override {
    auto observer = std::make_unique<ConnectionObserver>(this);
    connection->SetObserver(observer.get());
    connections_.emplace_back(std::move(observer), std::move(connection));
    connections_.back().second->CloseWriteEnd();
  }

 private:
  std::vector<std::pair<std::unique_ptr<ConnectionObserver>,
                        std::unique_ptr<ProtocolConnection>>>
      connections_;
};

class RequestDelegate final : public presentation::RequestDelegate {
 public:
  RequestDelegate() = default;
  ~RequestDelegate() override = default;

  void OnConnection(
      std::unique_ptr<presentation::Connection> connection) override {
    OSP_LOG_INFO << "request successful";
    this->connection = std::move(connection);
  }

  void OnError(const Error& error) override {
    OSP_LOG_INFO << "on request error";
  }

  std::unique_ptr<presentation::Connection> connection;
};

class ConnectionDelegate final : public presentation::Connection::Delegate {
 public:
  ConnectionDelegate() = default;
  ~ConnectionDelegate() override = default;

  void OnConnected() override {
    OSP_LOG_INFO << "presentation connection connected";
  }
  void OnClosedByRemote() override {
    OSP_LOG_INFO << "presentation connection closed by remote";
  }
  void OnDiscarded() override {}
  void OnError(const absl::string_view message) override {}
  void OnTerminated() override { OSP_LOG_INFO << "presentation terminated"; }

  void OnStringMessage(absl::string_view message) override {
    OSP_LOG_INFO << "got message: " << message;
  }
  void OnBinaryMessage(const std::vector<uint8_t>& data) override {}
};

class ReceiverConnectionDelegate final
    : public presentation::Connection::Delegate {
 public:
  ReceiverConnectionDelegate() = default;
  ~ReceiverConnectionDelegate() override = default;

  void OnConnected() override {
    OSP_LOG << "presentation connection connected";
  }
  void OnClosedByRemote() override {
    OSP_LOG << "presentation connection closed by remote";
  }
  void OnDiscarded() override {}
  void OnError(const absl::string_view message) override {}
  void OnTerminated() override { OSP_LOG << "presentation terminated"; }

  void OnStringMessage(const absl::string_view message) override {
    OSP_LOG << "got message: " << message;
    connection->SendString("--echo-- " + std::string(message));
  }
  void OnBinaryMessage(const std::vector<uint8_t>& data) override {}

  presentation::Connection* connection;
};

class ReceiverDelegate final : public presentation::ReceiverDelegate {
 public:
  ~ReceiverDelegate() override = default;

  std::vector<msgs::UrlAvailability> OnUrlAvailabilityRequest(
      uint64_t client_id,
      uint64_t request_duration,
      std::vector<std::string> urls) override {
    std::vector<msgs::UrlAvailability> result;
    result.reserve(urls.size());
    for (const auto& url : urls) {
      OSP_LOG << "got availability request for: " << url;
      result.push_back(msgs::UrlAvailability::kAvailable);
    }
    return result;
  }

  bool StartPresentation(
      const presentation::Connection::PresentationInfo& info,
      uint64_t source_id,
      const std::vector<msgs::HttpHeader>& http_headers) override {
    presentation_id = info.id;
    connection = std::make_unique<presentation::Connection>(
        info, &cd, presentation::Receiver::Get());
    cd.connection = connection.get();
    presentation::Receiver::Get()->OnPresentationStarted(
        info.id, connection.get(), presentation::ResponseResult::kSuccess);
    return true;
  }

  bool ConnectToPresentation(uint64_t request_id,
                             const std::string& id,
                             uint64_t source_id) override {
    connection = std::make_unique<presentation::Connection>(
        presentation::Connection::PresentationInfo{
            id, connection->presentation_info().url},
        &cd, presentation::Receiver::Get());
    cd.connection = connection.get();
    presentation::Receiver::Get()->OnConnectionCreated(
        request_id, connection.get(), presentation::ResponseResult::kSuccess);
    return true;
  }

  void TerminatePresentation(const std::string& id,
                             presentation::TerminationReason reason) override {
    presentation::Receiver::Get()->OnPresentationTerminated(id, reason);
  }

  std::string presentation_id;
  std::unique_ptr<presentation::Connection> connection;
  ReceiverConnectionDelegate cd;
};

struct CommandLineSplit {
  std::string command;
  std::string argument_tail;
};

CommandLineSplit SeparateCommandFromArguments(const std::string& line) {
  size_t split_index = line.find_first_of(' ');
  // NOTE: |split_index| can be std::string::npos because not all commands
  // accept arguments.
  std::string command = line.substr(0, split_index);
  std::string argument_tail =
      split_index < line.size() ? line.substr(split_index + 1) : std::string();
  return {std::move(command), std::move(argument_tail)};
}

struct CommandWaitResult {
  bool done;
  CommandLineSplit command_line;
};

CommandWaitResult WaitForCommand(pollfd* pollfd) {
  NetworkServiceManager* network_service = NetworkServiceManager::Get();
  while (poll(pollfd, 1, 10) >= 0) {
    if (g_done) {
      return {true};
    }
    network_service->RunEventLoopOnce();

    if (pollfd->revents == 0) {
      continue;
    } else if (pollfd->revents & (POLLERR | POLLHUP)) {
      return {true};
    }

    std::string line;
    if (!std::getline(std::cin, line)) {
      return {true};
    }

    CommandWaitResult result;
    result.done = false;
    result.command_line = SeparateCommandFromArguments(line);
    return result;
  }
  return {true};
}

void RunControllerPollLoop(presentation::Controller* controller) {
  ReceiverObserver receiver_observer;
  RequestDelegate request_delegate;
  ConnectionDelegate connection_delegate;
  presentation::Controller::ReceiverWatch watch;
  presentation::Controller::ConnectRequest connect_request;

  pollfd stdin_pollfd{STDIN_FILENO, POLLIN};

  do {
    write(STDOUT_FILENO, "$ ", 2);

    CommandWaitResult command_result = WaitForCommand(&stdin_pollfd);
    if (command_result.done) {
      break;
    }

    if (command_result.command_line.command == "avail") {
      watch = controller->RegisterReceiverWatch(
          {std::string(command_result.command_line.argument_tail)},
          &receiver_observer);
    } else if (command_result.command_line.command == "start") {
      const absl::string_view& argument_tail =
          command_result.command_line.argument_tail;
      size_t next_split = argument_tail.find_first_of(' ');
      const std::string& service_id = receiver_observer.GetServiceId(
          std::string(argument_tail.substr(next_split + 1)));
      const std::string url =
          static_cast<std::string>(argument_tail.substr(0, next_split));
      connect_request = controller->StartPresentation(
          url, service_id, &request_delegate, &connection_delegate);
    } else if (command_result.command_line.command == "msg") {
      request_delegate.connection->SendString(
          command_result.command_line.argument_tail);
    } else if (command_result.command_line.command == "term") {
      request_delegate.connection->Terminate(
          presentation::TerminationReason::kControllerTerminateCalled);
    }
  } while (true);

  watch = presentation::Controller::ReceiverWatch();
}

void ListenerDemo() {
  SignalThings();

  ListenerObserver listener_observer;
  MdnsServiceListenerConfig listener_config;
  auto mdns_listener =
      MdnsServiceListenerFactory::Create(listener_config, &listener_observer);

  MessageDemuxer demuxer(platform::Clock::now,
                         MessageDemuxer::kDefaultBufferLimit);
  ConnectionClientObserver client_observer;
  auto connection_client =
      ProtocolConnectionClientFactory::Create(&demuxer, &client_observer);

  auto* network_service = NetworkServiceManager::Create(
      std::move(mdns_listener), nullptr, std::move(connection_client), nullptr);
  auto controller =
      std::make_unique<presentation::Controller>(platform::Clock::now);

  network_service->GetMdnsServiceListener()->Start();
  network_service->GetProtocolConnectionClient()->Start();

  RunControllerPollLoop(controller.get());

  network_service->GetMdnsServiceListener()->Stop();
  network_service->GetProtocolConnectionClient()->Stop();

  controller.reset();

  NetworkServiceManager::Dispose();
}

void HandleReceiverCommand(absl::string_view command,
                           absl::string_view argument_tail,
                           ReceiverDelegate& delegate,
                           NetworkServiceManager* manager) {
  if (command == "avail") {
    ServicePublisher* publisher = manager->GetMdnsServicePublisher();

    if (publisher->state() == ServicePublisher::State::kSuspended) {
      publisher->Resume();
    } else {
      publisher->Suspend();
    }
  } else if (command == "close") {
    delegate.connection->Close(presentation::Connection::CloseReason::kClosed);
  } else if (command == "msg") {
    delegate.connection->SendString(argument_tail);
  } else if (command == "term") {
    presentation::Receiver::Get()->OnPresentationTerminated(
        delegate.presentation_id,
        presentation::TerminationReason::kReceiverUserTerminated);
  } else {
    OSP_LOG_FATAL << "Received unknown receiver command: " << command;
  }
}

void RunReceiverPollLoop(pollfd& file_descriptor,
                         NetworkServiceManager* manager,
                         ReceiverDelegate& delegate) {
  pollfd stdin_pollfd{STDIN_FILENO, POLLIN};
  do {
    write(STDOUT_FILENO, "$ ", 2);

    CommandWaitResult command_result = WaitForCommand(&stdin_pollfd);
    if (command_result.done) {
      break;
    }

    HandleReceiverCommand(command_result.command_line.command,
                          command_result.command_line.argument_tail, delegate,
                          manager);
  } while (true);
}

void CleanupPublisherDemo(NetworkServiceManager* manager) {
  presentation::Receiver::Get()->SetReceiverDelegate(nullptr);
  presentation::Receiver::Get()->Deinit();
  manager->GetMdnsServicePublisher()->Stop();
  manager->GetProtocolConnectionServer()->Stop();

  NetworkServiceManager::Dispose();
}

void PublisherDemo(absl::string_view friendly_name) {
  SignalThings();

  constexpr uint16_t server_port = 6667;

  PublisherObserver publisher_observer;
  // TODO(btolsch): aggregate initialization probably better?
  ServicePublisher::Config publisher_config;
  publisher_config.friendly_name = std::string(friendly_name);
  publisher_config.hostname = "turtle-deadbeef";
  publisher_config.service_instance_name = "deadbeef";
  publisher_config.connection_server_port = server_port;

  auto mdns_publisher = MdnsServicePublisherFactory::Create(
      publisher_config, &publisher_observer);

  ServerConfig server_config;
  std::vector<platform::InterfaceAddresses> interfaces =
      platform::GetInterfaceAddresses();
  for (const auto& interface : interfaces) {
    server_config.connection_endpoints.push_back(
        IPEndpoint{interface.addresses[0].address, server_port});
  }

  MessageDemuxer demuxer(platform::Clock::now,
                         MessageDemuxer::kDefaultBufferLimit);
  ConnectionServerObserver server_observer;
  auto connection_server = ProtocolConnectionServerFactory::Create(
      server_config, &demuxer, &server_observer);
  auto* network_service =
      NetworkServiceManager::Create(nullptr, std::move(mdns_publisher), nullptr,
                                    std::move(connection_server));

  ReceiverDelegate receiver_delegate;
  presentation::Receiver::Get()->Init();
  presentation::Receiver::Get()->SetReceiverDelegate(&receiver_delegate);
  network_service->GetMdnsServicePublisher()->Start();
  network_service->GetProtocolConnectionServer()->Start();

  pollfd stdin_pollfd{STDIN_FILENO, POLLIN};

  RunReceiverPollLoop(stdin_pollfd, network_service, receiver_delegate);

  CleanupPublisherDemo(network_service);
}

}  // namespace
}  // namespace openscreen

struct InputArgs {
  absl::string_view friendly_server_name;
  bool is_verbose;
};

InputArgs GetInputArgs(int argc, char** argv) {
  InputArgs args = {};

  int c;
  while ((c = getopt(argc, argv, "v")) != -1) {
    switch (c) {
      case 'v':
        args.is_verbose = true;
        break;
    }
  }

  if (optind < argc) {
    args.friendly_server_name = argv[optind];
  }

  return args;
}

int main(int argc, char** argv) {
  using openscreen::platform::LogLevel;

  std::cout << "Usage: demo [-v] [friendly_name]" << std::endl
            << "-v: enable more verbose logging" << std::endl
            << "friendly_name: server name, runs the publisher demo."
            << std::endl
            << "               omitting runs the listener demo." << std::endl
            << std::endl;

  InputArgs args = GetInputArgs(argc, argv);

  const bool is_receiver_demo = !args.friendly_server_name.empty();
  const char* log_filename = is_receiver_demo
                                 ? openscreen::kReceiverLogFilename
                                 : openscreen::kControllerLogFilename;
  openscreen::platform::LogInit(log_filename);

  LogLevel level = args.is_verbose ? LogLevel::kVerbose : LogLevel::kInfo;
  openscreen::platform::SetLogLevel(level);

  if (is_receiver_demo) {
    openscreen::PublisherDemo(args.friendly_server_name);
  } else {
    openscreen::ListenerDemo();
  }

  return 0;
}
