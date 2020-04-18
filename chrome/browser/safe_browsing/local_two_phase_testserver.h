// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_LOCAL_TWO_PHASE_TESTSERVER_H_
#define CHROME_BROWSER_SAFE_BROWSING_LOCAL_TWO_PHASE_TESTSERVER_H_

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "net/test/spawned_test_server/local_test_server.h"

namespace safe_browsing {

// Runs a Python-based two phase upload test server on the same machine in which
// the LocalTwoPhaseTestServer runs.
class LocalTwoPhaseTestServer : public net::LocalTestServer {
 public:
  // Initialize a two phase protocol test server.
  LocalTwoPhaseTestServer();

  ~LocalTwoPhaseTestServer() override;

  // Returns the path to two_phase_testserver.py.
  bool GetTestServerPath(base::FilePath* testserver_path) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LocalTwoPhaseTestServer);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_LOCAL_TWO_PHASE_TESTSERVER_H_

