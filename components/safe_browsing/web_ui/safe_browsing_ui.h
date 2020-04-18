// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_
#define COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_

#include "base/macros.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "components/safe_browsing/proto/webui.pb.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
template <typename T>
struct DefaultSingletonTraits;
}

namespace safe_browsing {
class SafeBrowsingUIHandler : public content::WebUIMessageHandler {
 public:
  SafeBrowsingUIHandler(content::BrowserContext*);
  ~SafeBrowsingUIHandler() override;

  // Get the experiments that are currently enabled per Chrome instance.
  void GetExperiments(const base::ListValue* args);

  // Get the Safe Browsing related preferences for the current user.
  void GetPrefs(const base::ListValue* args);

  // Get the information related to the Safe Browsing database and full hash
  // cache.
  void GetDatabaseManagerInfo(const base::ListValue* args);

  // Get the ThreatDetails that have been collected since the oldest currently
  // open chrome://safe-browsing tab was opened.
  void GetSentThreatDetails(const base::ListValue* args);

  // Get the new ThreatDetails messages sent from ThreatDetails when a ping is
  // sent, while one or more WebUI tabs are opened.
  void NotifyThreatDetailsJsListener(
      ClientSafeBrowsingReportRequest* threat_detail);

  // Register callbacks for WebUI messages.
  void RegisterMessages() override;

 private:
  content::BrowserContext* browser_context_;
  // List that keeps all the WebUI listener objects.
  static std::vector<SafeBrowsingUIHandler*> webui_list_;
  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingUIHandler);
};

// The WebUI for chrome://safe-browsing
class SafeBrowsingUI : public content::WebUIController {
 public:
  explicit SafeBrowsingUI(content::WebUI* web_ui);
  ~SafeBrowsingUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingUI);
};

class WebUIInfoSingleton {
 public:
  static WebUIInfoSingleton* GetInstance();

  // Delete the list of the sent ClientSafeBrowsingReportRequest messages.
  void ClearReportsSent();

  // Add the new message in reports_sent_ and send it to all the
  // chrome://safe-browsing opened tabs.
  void AddToReportsSent(
      std::unique_ptr<ClientSafeBrowsingReportRequest> report_request);

  // Register the new WebUI listener object.
  void RegisterWebUIInstance(SafeBrowsingUIHandler* webui);

  // Unregister the WebUI listener object, and clean the list of reports, if
  // this is last listener.
  void UnregisterWebUIInstance(SafeBrowsingUIHandler* webui);

  // Get the list of the sent reports that have been collected since the oldest
  // currently open chrome://safe-browsing tab was opened.
  const std::vector<std::unique_ptr<ClientSafeBrowsingReportRequest>>&
  reports_sent() const {
    return reports_sent_;
    ;
  }
  // Get the list of WebUI listener objects.
  const std::vector<SafeBrowsingUIHandler*>& webui_instances() const {
    return webui_instances_;
  }

 private:
  WebUIInfoSingleton();
  ~WebUIInfoSingleton();

  friend struct base::DefaultSingletonTraits<WebUIInfoSingleton>;

  // List of reports sent since since the oldest currently
  // open chrome://safe-browsing tab was opened.
  // "ClientSafeBrowsingReportRequest" cannot be const, due to being used by
  // functions that call AllowJavascript(), which is not marked const.
  std::vector<std::unique_ptr<ClientSafeBrowsingReportRequest>> reports_sent_;

  // List of WebUI listener objects. "SafeBrowsingUIHandler*" cannot be const,
  // due to being used by functions that call AllowJavascript(), which is not
  // marked const.
  std::vector<SafeBrowsingUIHandler*> webui_instances_;
  DISALLOW_COPY_AND_ASSIGN(WebUIInfoSingleton);
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_WEBUI_SAFE_BROWSING_UI_H_
