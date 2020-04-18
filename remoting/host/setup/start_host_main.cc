// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/setup/start_host_main.h"

#include <stddef.h>
#include <stdio.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/base/logging.h"
#include "remoting/base/oauth_helper.h"
#include "remoting/base/service_urls.h"
#include "remoting/base/url_request_context_getter.h"
#include "remoting/host/setup/host_starter.h"
#include "remoting/host/setup/pin_validator.h"

#if defined(OS_POSIX)
#include <termios.h>
#include <unistd.h>
#endif  // defined(OS_POSIX)

#if defined(OS_WIN)
#include "base/process/process_info.h"
#endif  // defined(OS_WIN)

namespace remoting {

namespace {

// True if the host was started successfully.
bool g_started = false;

// The main message loop.
base::MessageLoop* g_message_loop = nullptr;

// The active RunLoop.
base::RunLoop* g_active_run_loop = nullptr;

// Lets us hide the PIN that a user types.
void SetEcho(bool echo) {
#if defined(OS_WIN)
  DWORD mode;
  HANDLE console_handle = GetStdHandle(STD_INPUT_HANDLE);
  if (!GetConsoleMode(console_handle, &mode)) {
    LOG(ERROR) << "GetConsoleMode failed";
    return;
  }
  SetConsoleMode(console_handle,
                 (mode & ~ENABLE_ECHO_INPUT) | (echo ? ENABLE_ECHO_INPUT : 0));
#else
  termios term;
  tcgetattr(STDIN_FILENO, &term);
  if (echo) {
    term.c_lflag |= ECHO;
  } else {
    term.c_lflag &= ~ECHO;
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
#endif  // !defined(OS_WIN)
}

// Reads a newline-terminated string from stdin.
std::string ReadString(bool no_echo) {
  if (no_echo)
    SetEcho(false);
  const int kMaxLen = 1024;
  std::string str(kMaxLen, 0);
  char* result = fgets(&str[0], kMaxLen, stdin);
  if (no_echo) {
    printf("\n");
    SetEcho(true);
  }
  if (!result)
    return std::string();
  size_t newline_index = str.find('\n');
  if (newline_index != std::string::npos)
    str[newline_index] = '\0';
  str.resize(strlen(&str[0]));
  return str;
}

// Called when the HostStarter has finished.
void OnDone(HostStarter::Result result) {
  if (!g_message_loop->task_runner()->BelongsToCurrentThread()) {
    g_message_loop->task_runner()->PostTask(FROM_HERE,
                                            base::Bind(&OnDone, result));
    return;
  }
  switch (result) {
    case HostStarter::START_COMPLETE:
      g_started = true;
      break;
    case HostStarter::NETWORK_ERROR:
      fprintf(stderr, "Couldn't start host: network error.\n");
      break;
    case HostStarter::OAUTH_ERROR:
      fprintf(stderr, "Couldn't start host: OAuth error.\n");
      break;
    case HostStarter::START_ERROR:
      fprintf(stderr, "Couldn't start host.\n");
      break;
  }

  g_active_run_loop->Quit();
}

std::string GetAuthorizationCodeUri() {
  return remoting::GetOauthStartUrl(remoting::GetDefaultOauthRedirectUrl());
}

}  // namespace

int StartHostMain(int argc, char** argv) {
  // google_apis::GetOAuth2ClientID/Secret need a static CommandLine.
  base::CommandLine::Init(argc, argv);
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  // This object instance is required by Chrome code (for example,
  // FilePath, LazyInstance, MessageLoop).
  base::AtExitManager exit_manager;

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);

  base::TaskScheduler::CreateAndStartWithDefaultParams("RemotingHostSetup");

  std::string host_name = command_line->GetSwitchValueASCII("name");
  std::string host_pin = command_line->GetSwitchValueASCII("pin");
  std::string auth_code = command_line->GetSwitchValueASCII("code");
  std::string redirect_url = command_line->GetSwitchValueASCII("redirect-url");

#if defined(OS_POSIX)
  // Check if current user is root. If it is root, then throw an error message.
  // This is because start_host should be run in user mode.
  if (geteuid() == 0) {
    fprintf(stderr, "Refusing to run %s as root.", argv[0]);
    return 1;
  }
#endif  // defined(OS_POSIX)

#if defined(OS_WIN)
  // The tool must be run elevated on Windows so the host has access to the
  // directories used to store the configuration JSON files.
  if (!base::IsCurrentProcessElevated()) {
    fprintf(stderr, "Error: %s must be run as an elevated process.", argv[0]);
    return 1;
  }
#endif  // defined(OS_WIN)

  if (command_line->HasSwitch("help") || command_line->HasSwitch("h") ||
      command_line->HasSwitch("?") || !command_line->GetArgs().empty()) {
    fprintf(stderr,
            "Usage: %s [--name=<hostname>] [--code=<auth-code>] [--pin=<PIN>] "
            "[--redirect-url=<redirectURL>]\n",
            argv[0]);
    return 1;
  }

  if (host_name.empty()) {
    fprintf(stdout, "Enter a name for this computer: ");
    fflush(stdout);
    host_name = ReadString(false);
  }

  if (host_pin.empty()) {
    while (true) {
      fprintf(stdout, "Enter a PIN of at least six digits: ");
      fflush(stdout);
      host_pin = ReadString(true);
      if (!remoting::IsPinValid(host_pin)) {
        fprintf(stdout,
                "Please use a PIN consisting of at least six digits.\n");
        fflush(stdout);
        continue;
      }
      std::string host_pin_confirm;
      fprintf(stdout, "Enter the same PIN again: ");
      fflush(stdout);
      host_pin_confirm = ReadString(true);
      if (host_pin != host_pin_confirm) {
        fprintf(stdout, "You entered different PINs.\n");
        fflush(stdout);
        continue;
      }
      break;
    }
  } else {
    if (!remoting::IsPinValid(host_pin)) {
      fprintf(stderr, "Please use a PIN consisting of at least six digits.\n");
      return 1;
    }
  }

  if (auth_code.empty()) {
    fprintf(stdout, "\nAuthorization URL for Production services:\n");
    fprintf(stdout, "%s\n\n", GetAuthorizationCodeUri().c_str());
    fprintf(stdout, "Enter an authorization code: ");
    fflush(stdout);
    auth_code = ReadString(true);
  }

  // Provide message loops and threads for the URLRequestContextGetter.
  base::MessageLoop message_loop;
  g_message_loop = &message_loop;
  base::Thread::Options io_thread_options(base::MessageLoop::TYPE_IO, 0);
  base::Thread io_thread("IO thread");
  io_thread.StartWithOptions(io_thread_options);

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter(
      new remoting::URLRequestContextGetter(io_thread.task_runner()));

  net::URLFetcher::SetIgnoreCertificateRequests(true);

  // Start the host.
  std::unique_ptr<HostStarter> host_starter(HostStarter::Create(
      remoting::ServiceUrls::GetInstance()->directory_hosts_url(),
      url_request_context_getter.get()));
  if (redirect_url.empty()) {
    redirect_url = remoting::GetDefaultOauthRedirectUrl();
  }
  host_starter->StartHost(host_name, host_pin,
                          /*consent_to_data_collection=*/true, auth_code,
                          redirect_url, base::Bind(&OnDone));

  // Run the message loop until the StartHost completion callback.
  base::RunLoop run_loop;
  g_active_run_loop = &run_loop;
  run_loop.Run();

  g_message_loop = nullptr;
  g_active_run_loop = nullptr;

  // Destroy the HostStarter and URLRequestContextGetter before stopping the
  // IO thread.
  host_starter.reset();
  url_request_context_getter = nullptr;

  io_thread.Stop();

  return g_started ? 0 : 1;
}

}  // namespace remoting
