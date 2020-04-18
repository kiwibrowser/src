// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/url_keyed_metrics_ui.h"

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/ukm/debug/ukm_debug_data_extractor.h"
#include "components/ukm/ukm_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/url_data_source_ios.h"

namespace {

class URLKeyedMetricsUIHTMLSource : public web::URLDataSourceIOS {
 public:
  // Construct a data source for the specified |source_name|.
  explicit URLKeyedMetricsUIHTMLSource(const std::string& source_name);

  // web::URLDataSourceIOS implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const web::URLDataSourceIOS::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool ShouldDenyXFrameOptions() const override;

 private:
  ~URLKeyedMetricsUIHTMLSource() override;

  ukm::UkmService* GetUkmService();
  std::string source_name_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(URLKeyedMetricsUIHTMLSource);
};

}  // namespace

// URLKeyedMetricsUIHTMLSource -------------------------------------------------

URLKeyedMetricsUIHTMLSource::URLKeyedMetricsUIHTMLSource(
    const std::string& source_name)
    : source_name_(source_name) {}

URLKeyedMetricsUIHTMLSource::~URLKeyedMetricsUIHTMLSource() {}

std::string URLKeyedMetricsUIHTMLSource::GetSource() const {
  return source_name_;
}

void URLKeyedMetricsUIHTMLSource::StartDataRequest(
    const std::string& path,
    const web::URLDataSourceIOS::GotDataCallback& callback) {
  std::string data =
      ukm::debug::UkmDebugDataExtractor::GetHTMLData(GetUkmService());
  callback.Run(base::RefCountedString::TakeString(&data));
}

std::string URLKeyedMetricsUIHTMLSource::GetMimeType(
    const std::string& path) const {
  return "text/html";
}

bool URLKeyedMetricsUIHTMLSource::ShouldDenyXFrameOptions() const {
  return web::URLDataSourceIOS::ShouldDenyXFrameOptions();
}

ukm::UkmService* URLKeyedMetricsUIHTMLSource::GetUkmService() {
  return GetApplicationContext()->GetMetricsServicesManager()->GetUkmService();
}

// URLKeyedMetricsUI -----------------------------------------------------------

URLKeyedMetricsUI::URLKeyedMetricsUI(web::WebUIIOS* web_ui,
                                     const std::string& name)
    : web::WebUIIOSController(web_ui) {
  web::URLDataSourceIOS::Add(ios::ChromeBrowserState::FromWebUIIOS(web_ui),
                             new URLKeyedMetricsUIHTMLSource(name));
}

URLKeyedMetricsUI::~URLKeyedMetricsUI() {}
