// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_MESSAGE_FILTER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace net {
class URLRequestContextGetter;
}

namespace network {
namespace mojom {
class NetworkContext;
}
}  // namespace network

namespace storage {
class QuotaManager;
}

namespace storage {
class DatabaseTracker;
}

namespace content {

class LayoutTestMessageFilter : public BrowserMessageFilter {
 public:
  LayoutTestMessageFilter(int render_process_id,
                          storage::DatabaseTracker* database_tracker,
                          storage::QuotaManager* quota_manager,
                          net::URLRequestContextGetter* request_context_getter,
                          network::mojom::NetworkContext* network_context);

 private:
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<LayoutTestMessageFilter>;

  ~LayoutTestMessageFilter() override;

  // BrowserMessageFilter implementation.
  void OnDestruct() const override;
  base::TaskRunner* OverrideTaskRunnerForMessage(
      const IPC::Message& message) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnReadFileToString(const base::FilePath& local_file,
                          std::string* contents);
  void OnRegisterIsolatedFileSystem(
      const std::vector<base::FilePath>& absolute_filenames,
      std::string* filesystem_id);
  void OnClearAllDatabases();
  void OnSetDatabaseQuota(int quota);
  void OnGrantWebNotificationPermission(const GURL& origin,
                                        bool permission_granted);
  void OnClearWebNotificationPermissions();
  void OnSimulateWebNotificationClick(
      const std::string& title,
      const base::Optional<int>& action_index,
      const base::Optional<base::string16>& reply);
  void OnSimulateWebNotificationClose(const std::string& title, bool by_user);
  void OnSetPushMessagingPermission(const GURL& origin, bool allowed);
  void OnClearPushMessagingPermissions();
  void OnDeleteAllCookies();
  void OnDeleteAllCookiesForNetworkService();
  void OnSetPermission(const std::string& name,
                       blink::mojom::PermissionStatus status,
                       const GURL& origin,
                       const GURL& embedding_origin);
  void OnResetPermissions();
  void OnLayoutTestRuntimeFlagsChanged(
      const base::DictionaryValue& changed_layout_test_runtime_flags);
  void OnTestFinishedInSecondaryRenderer();
  void OnInitiateCaptureDump(bool capture_navigation_history,
                             bool capture_pixels);
  void OnInspectSecondaryWindow();

  int render_process_id_;

  scoped_refptr<storage::DatabaseTracker> database_tracker_;
  scoped_refptr<storage::QuotaManager> quota_manager_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  network::mojom::CookieManagerPtr cookie_manager_;

  DISALLOW_COPY_AND_ASSIGN(LayoutTestMessageFilter);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_MESSAGE_FILTER_H_
