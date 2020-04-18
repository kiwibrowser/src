// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/content/debug_page/debug_page.h"

#include "base/memory/ref_counted_memory.h"
#include "components/ukm/debug/ukm_debug_data_extractor.h"

namespace ukm {
namespace debug {

DebugPage::DebugPage(ServiceGetter service_getter)
    : service_getter_(service_getter) {}

DebugPage::~DebugPage() {}

std::string DebugPage::GetSource() const {
  return "ukm";
}

std::string DebugPage::GetMimeType(const std::string& path) const {
  return "text/html";
}

void DebugPage::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  std::string data = UkmDebugDataExtractor::GetHTMLData(service_getter_.Run());
  callback.Run(base::RefCountedString::TakeString(&data));
}

bool DebugPage::AllowCaching() const {
  return false;
}

}  // namespace debug
}  // namespace ukm
