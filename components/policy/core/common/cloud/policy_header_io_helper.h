// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_CLOUD_POLICY_HEADER_IO_HELPER_H_
#define COMPONENTS_POLICY_CORE_COMMON_CLOUD_POLICY_HEADER_IO_HELPER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "components/policy/policy_export.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}

namespace policy {

// Helper class that lives on the I/O thread and adds policy headers to
// outgoing requests. Instances of this class are created by
// PolicyHeaderService on the UI thread, and that class is responsible for
// notifying this class via UpdateHeaderFromUI() when the header changes.
// Ownership is transferred to ProfileIOData, and this object is run and
// destroyed on the I/O thread.
class POLICY_EXPORT PolicyHeaderIOHelper {
 public:
  PolicyHeaderIOHelper(
      const std::string& server_url,
      const std::string& initial_header_value,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner);
  ~PolicyHeaderIOHelper();

  // Sets any necessary policy headers for the specified URL on the passed
  // request. Should be invoked only from the I/O thread.
  void AddPolicyHeaders(const GURL& url,
                        net::URLRequest* request) const;

  // API invoked when the header changes. Can be called from any thread - calls
  // are marshalled via the TaskRunner to run on the appropriate thread.
  // If |new_header| is the empty string, no header will be added to
  // outgoing requests.
  void UpdateHeader(const std::string& new_header);

  // Test-only routine used to inject the server URL at runtime - this is
  // required because PolicyHeaderIOHelper is created very early in BrowserTest
  // initialization, before we startup the EmbeddedTestServer.
  void SetServerURLForTest(const std::string& server_url);

 private:
  // API invoked via the TaskRunner to update the header.
  void UpdateHeaderOnIOThread(const std::string& new_header);

  // Helper routine invoked via the TaskRunner to update the cached server URL.
  void SetServerURLOnIOThread(const std::string& new_header);

  // The URL we should add policy headers to.
  std::string server_url_;

  // The task runner associated with the I/O thread that runs this object.
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // The current policy header value.
  std::string policy_header_;

  DISALLOW_COPY_AND_ASSIGN(PolicyHeaderIOHelper);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_CLOUD_POLICY_HEADER_IO_HELPER_H_
