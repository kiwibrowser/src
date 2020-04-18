// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file implements the Windows service controlling Me2Me host processes
// running within user sessions.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/named_platform_channel_pair.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "remoting/base/auto_thread.h"
#include "remoting/base/auto_thread_task_runner.h"
#include "remoting/host/desktop_process.h"
#include "remoting/host/host_exit_codes.h"
#include "remoting/host/host_main.h"
#include "remoting/host/me2me_desktop_environment.h"
#include "remoting/host/switches.h"
#include "remoting/host/win/session_desktop_environment.h"

namespace remoting {

int DesktopProcessMain() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  base::MessageLoopForUI message_loop;
  base::RunLoop run_loop;
  scoped_refptr<AutoThreadTaskRunner> ui_task_runner =
      new AutoThreadTaskRunner(message_loop.task_runner(),
                               run_loop.QuitClosure());

  // Launch the video capture thread.
  scoped_refptr<AutoThreadTaskRunner> video_capture_task_runner =
      AutoThread::Create("Video capture thread", ui_task_runner);

  // Launch the input thread.
  scoped_refptr<AutoThreadTaskRunner> input_task_runner =
      AutoThread::CreateWithType(
          "Input thread", ui_task_runner, base::MessageLoop::TYPE_IO);

  // Launch the I/O thread.
  scoped_refptr<AutoThreadTaskRunner> io_task_runner =
      AutoThread::CreateWithType("I/O thread", ui_task_runner,
                                 base::MessageLoop::TYPE_IO);

  mojo::edk::ScopedIPCSupport ipc_support(
      io_task_runner->task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::FAST);
  mojo::edk::ScopedInternalPlatformHandle parent_pipe =
      mojo::edk::PlatformChannelPair::PassClientHandleFromParentProcess(
          *command_line);
  if (!parent_pipe.is_valid()) {
    parent_pipe =
        mojo::edk::NamedPlatformChannelPair::PassClientHandleFromParentProcess(
            *command_line);
  }
  if (!parent_pipe.is_valid()) {
    return kInvalidCommandLineExitCode;
  }

  auto invitation = mojo::edk::IncomingBrokerClientInvitation::Accept(
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  std::move(parent_pipe)));
  mojo::ScopedMessagePipeHandle message_pipe = invitation->ExtractMessagePipe(
      command_line->GetSwitchValueASCII(kMojoPipeToken));
  DesktopProcess desktop_process(ui_task_runner, input_task_runner,
                                 io_task_runner, std::move(message_pipe));

  // Create a platform-dependent environment factory.
  std::unique_ptr<DesktopEnvironmentFactory> desktop_environment_factory;
#if defined(OS_WIN)
  // base::Unretained() is safe here: |desktop_process| outlives run_loop.Run().
  auto inject_sas_closure = base::Bind(&DesktopProcess::InjectSas,
                                       base::Unretained(&desktop_process));
  auto lock_workstation_closure = base::Bind(
      &DesktopProcess::LockWorkstation, base::Unretained(&desktop_process));

  desktop_environment_factory.reset(new SessionDesktopEnvironmentFactory(
      ui_task_runner, video_capture_task_runner, input_task_runner,
      ui_task_runner, nullptr, inject_sas_closure, lock_workstation_closure));
#else  // !defined(OS_WIN)
  desktop_environment_factory.reset(new Me2MeDesktopEnvironmentFactory(
      ui_task_runner, video_capture_task_runner, input_task_runner,
      ui_task_runner, nullptr));
#endif  // !defined(OS_WIN)

  if (!desktop_process.Start(std::move(desktop_environment_factory)))
    return kInitializationFailed;

  // Run the UI message loop.
  ui_task_runner = nullptr;
  run_loop.Run();

  return kSuccessExitCode;
}

}  // namespace remoting

#if !defined(OS_WIN)
int main(int argc, char** argv) {
  return remoting::HostMain(argc, argv);
}
#endif  // !defined(OS_WIN)
