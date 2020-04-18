// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_LOCAL_SYNC_TEST_SERVER_H_
#define COMPONENTS_SYNC_TEST_LOCAL_SYNC_TEST_SERVER_H_

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/test/spawned_test_server/local_test_server.h"

namespace syncer {

// Runs a Python-based sync test server on the same machine on which the
// LocalSyncTestServer runs.
class LocalSyncTestServer : public net::LocalTestServer {
 public:
  // Initialize a sync server that listens on localhost using ephemeral ports
  // for sync and p2p notifications.
  LocalSyncTestServer();

  // Initialize a sync server that listens on |port| for sync updates and
  // |xmpp_port| for p2p notifications.
  LocalSyncTestServer(uint16_t port, uint16_t xmpp_port);

  ~LocalSyncTestServer() override;

  // Overriden from net::LocalTestServer.
  bool AddCommandLineArguments(base::CommandLine* command_line) const override;
  bool SetPythonPath() const override;
  bool GetTestServerPath(base::FilePath* testserver_path) const override;

  // Returns true if the path to |test_script_name| is successfully stored  in
  // |*test_script_path|. Used by the run_sync_testserver executable.
  bool GetTestScriptPath(const base::FilePath::StringType& test_script_name,
                         base::FilePath* test_script_path) const;

 private:
  // Port on which the Sync XMPP server listens.
  uint16_t xmpp_port_;

  DISALLOW_COPY_AND_ASSIGN(LocalSyncTestServer);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_LOCAL_SYNC_TEST_SERVER_H_
