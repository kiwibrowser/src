// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/tests/util.h"

#include "base/base_paths.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/process/process.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/connector.mojom.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"
#include "services/service_manager/runner/common/switches.h"

namespace service_manager {
namespace test {

namespace {

void GrabConnectResult(base::RunLoop* loop,
                       mojom::ConnectResult* out_result,
                       mojom::ConnectResult result,
                       const Identity& resolved_identity) {
  loop->Quit();
  *out_result = result;
}

}  // namespace

mojom::ConnectResult LaunchAndConnectToProcess(
    const std::string& target_exe_name,
    const Identity& target,
    service_manager::Connector* connector,
    base::Process* process) {
  base::FilePath target_path;
  CHECK(base::PathService::Get(base::DIR_ASSETS, &target_path));
  target_path = target_path.AppendASCII(target_exe_name);

  base::CommandLine child_command_line(target_path);
  // Forward the wait-for-debugger flag but nothing else - we don't want to
  // stamp on the platform-channel flag.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kWaitForDebugger)) {
    child_command_line.AppendSwitch(::switches::kWaitForDebugger);
  }

  // Create the channel to be shared with the target process. Pass one end
  // on the command line.
  mojo::edk::PlatformChannelPair platform_channel_pair;
  mojo::edk::HandlePassingInformation handle_passing_info;
  platform_channel_pair.PrepareToPassClientHandleToChildProcess(
      &child_command_line, &handle_passing_info);

  mojo::edk::OutgoingBrokerClientInvitation invitation;
  std::string token = mojo::edk::GenerateRandomToken();
  mojo::ScopedMessagePipeHandle pipe = invitation.AttachMessagePipe(token);
  child_command_line.AppendSwitchASCII(switches::kServicePipeToken, token);

  service_manager::mojom::ServicePtr client;
  client.Bind(mojo::InterfacePtrInfo<service_manager::mojom::Service>(
      std::move(pipe), 0u));
  service_manager::mojom::PIDReceiverPtr receiver;

  connector->StartService(target, std::move(client), MakeRequest(&receiver));
  mojom::ConnectResult result;
  {
    base::RunLoop loop(base::RunLoop::Type::kNestableTasksAllowed);
    Connector::TestApi test_api(connector);
    test_api.SetStartServiceCallback(
        base::Bind(&GrabConnectResult, &loop, &result));
    loop.Run();
  }

  base::LaunchOptions options;
#if defined(OS_WIN)
  options.handles_to_inherit = handle_passing_info;
#elif defined(OS_FUCHSIA)
  options.handles_to_transfer = handle_passing_info;
#elif defined(OS_POSIX)
  options.fds_to_remap = handle_passing_info;
#endif
  *process = base::LaunchProcess(child_command_line, options);
  DCHECK(process->IsValid());
  platform_channel_pair.ChildProcessLaunched();
  receiver->SetPID(process->Pid());
  invitation.Send(
      process->Handle(),
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  platform_channel_pair.PassServerHandle()));
  return result;
}

}  // namespace test
}  // namespace service_manager
