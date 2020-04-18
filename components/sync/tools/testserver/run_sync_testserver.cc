// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <stdio.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_timeouts.h"
#include "components/sync/test/local_sync_test_server.h"
#include "net/test/python_utils.h"

static void PrintUsage() {
  printf("run_sync_testserver [--port=<port>] [--xmpp-port=<xmpp_port>]\n");
}

// Launches the chromiumsync_test.py or xmppserver_test.py scripts, which test
// the sync HTTP and XMPP sever functionality respectively.
static bool RunSyncTest(
    const base::FilePath::StringType& sync_test_script_name) {
  std::unique_ptr<syncer::LocalSyncTestServer> test_server(
      new syncer::LocalSyncTestServer());
  if (!test_server->SetPythonPath()) {
    LOG(ERROR) << "Error trying to set python path. Exiting.";
    return false;
  }

  base::FilePath sync_test_script_path;
  if (!test_server->GetTestScriptPath(sync_test_script_name,
                                      &sync_test_script_path)) {
    LOG(ERROR) << "Error trying to get path for test script "
               << sync_test_script_name;
    return false;
  }

  base::CommandLine python_command(base::CommandLine::NO_PROGRAM);
  if (!GetPythonCommand(&python_command)) {
    LOG(ERROR) << "Could not get python runtime command.";
    return false;
  }

  python_command.AppendArgPath(sync_test_script_path);
  if (!base::LaunchProcess(python_command, base::LaunchOptions()).IsValid()) {
    LOG(ERROR) << "Failed to launch test script " << sync_test_script_name;
    return false;
  }
  return true;
}

// Gets a port value from the switch with name |switch_name| and writes it to
// |port|. Returns true if a port was provided and false otherwise.
static bool GetPortFromSwitch(const std::string& switch_name, uint16_t* port) {
  DCHECK(port != nullptr) << "|port| is null";
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  int port_int = 0;
  if (command_line->HasSwitch(switch_name)) {
    std::string port_str = command_line->GetSwitchValueASCII(switch_name);
    if (!base::StringToInt(port_str, &port_int)) {
      return false;
    }
  }
  *port = static_cast<uint16_t>(port_int);
  return true;
}

int main(int argc, const char* argv[]) {
  base::AtExitManager at_exit_manager;
  base::MessageLoopForIO message_loop;

  // Process command line
  base::CommandLine::Init(argc, argv);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file = FILE_PATH_LITERAL("sync_testserver.log");
  if (!logging::InitLogging(settings)) {
    printf("Error: could not initialize logging. Exiting.\n");
    return -1;
  }

  TestTimeouts::Initialize();

  if (command_line->HasSwitch("help")) {
    PrintUsage();
    return 0;
  }

  if (command_line->HasSwitch("sync-test")) {
    return RunSyncTest(FILE_PATH_LITERAL("chromiumsync_test.py")) ? 0 : -1;
  }

  if (command_line->HasSwitch("xmpp-test")) {
    return RunSyncTest(FILE_PATH_LITERAL("xmppserver_test.py")) ? 0 : -1;
  }

  uint16_t port = 0;
  GetPortFromSwitch("port", &port);

  uint16_t xmpp_port = 0;
  GetPortFromSwitch("xmpp-port", &xmpp_port);

  std::unique_ptr<syncer::LocalSyncTestServer> test_server(
      new syncer::LocalSyncTestServer(port, xmpp_port));
  if (!test_server->Start()) {
    printf("Error: failed to start python sync test server. Exiting.\n");
    return -1;
  }

  printf("Python sync test server running at %s (type ctrl+c to exit)\n",
         test_server->host_port_pair().ToString().c_str());

  base::RunLoop().Run();
  return 0;
}
