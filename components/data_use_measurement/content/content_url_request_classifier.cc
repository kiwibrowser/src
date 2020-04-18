// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_use_measurement/content/content_url_request_classifier.h"

#include <string>

#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/strings/string_util.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_type.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "ui/base/page_transition_types.h"

namespace {

// Data use broken by page transition type. This enum must remain synchronized
// with the enum of the same name in metrics/histograms/histograms.xml. The enum
// values have the same meaning as ui::PageTransition in
// ui/base/page_transition_types.h. These values are written to logs.  New enum
// values can be added, but existing enums must never be renumbered or deleted
// and reused.
enum DataUsePageTransition {
  LINK = 0,
  TYPED = 1,
  AUTO_BOOKMARK = 2,
  SUBFRAME = 3,  // AUTO_SUBFRAME and MANUAL_SUBFRAME transitions.
  GENERATED = 4,
  AUTO_TOPLEVEL = 5,
  FORM_SUBMIT = 6,
  RELOAD = 7,
  KEYWORD = 8,  // KEYWORD and KEYWORD_GENERATED transitions.
  FORWARD_BACK = 9,
  HOME_PAGE = 10,
  TRANSITION_MAX = 11,
};

}  // namespace

namespace data_use_measurement {

bool IsUserRequest(const net::URLRequest& request) {
  // The presence of ResourecRequestInfo in |request| implies that this request
  // was created for a content::WebContents. For now we could add a condition to
  // check ProcessType in info is content::PROCESS_TYPE_RENDERER, but it won't
  // be compatible with upcoming PlzNavigate architecture. So just existence of
  // ResourceRequestInfo is verified, and the current check should be compatible
  // with upcoming changes in PlzNavigate.
  // TODO(rajendrant): Verify this condition for different use cases. See
  // crbug.com/626063.
  return content::ResourceRequestInfo::ForRequest(&request) != nullptr;
}

bool ContentURLRequestClassifier::IsUserRequest(
    const net::URLRequest& request) const {
  return data_use_measurement::IsUserRequest(request);
}

DataUseUserData::DataUseContentType ContentURLRequestClassifier::GetContentType(
    const net::URLRequest& request,
    const net::HttpResponseHeaders& response_headers) const {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);
  std::string mime_type;
  if (response_headers.GetMimeType(&mime_type)) {
    if (mime_type == "text/html" && request_info &&
        request_info->GetResourceType() ==
            content::ResourceType::RESOURCE_TYPE_MAIN_FRAME) {
      return DataUseUserData::MAIN_FRAME_HTML;
    }
    if (mime_type == "text/html")
      return DataUseUserData::NON_MAIN_FRAME_HTML;
    if (mime_type == "text/css")
      return DataUseUserData::CSS;
    if (base::StartsWith(mime_type, "image/", base::CompareCase::SENSITIVE))
      return DataUseUserData::IMAGE;
    if (base::EndsWith(mime_type, "javascript", base::CompareCase::SENSITIVE) ||
        base::EndsWith(mime_type, "ecmascript", base::CompareCase::SENSITIVE)) {
      return DataUseUserData::JAVASCRIPT;
    }
    if (mime_type.find("font") != std::string::npos)
      return DataUseUserData::FONT;
    if (base::StartsWith(mime_type, "audio/", base::CompareCase::SENSITIVE))
      return DataUseUserData::AUDIO;
    if (base::StartsWith(mime_type, "video/", base::CompareCase::SENSITIVE))
      return DataUseUserData::VIDEO;
  }
  return DataUseUserData::OTHER;
}

void ContentURLRequestClassifier::RecordPageTransitionUMA(
    uint64_t page_transition,
    int64_t received_bytes) const {
  DataUsePageTransition data_use_page_transition =
      DataUsePageTransition::TRANSITION_MAX;
  if (received_bytes <= 0)
    return;

  if (page_transition & ui::PAGE_TRANSITION_FORWARD_BACK) {
    data_use_page_transition = DataUsePageTransition::FORWARD_BACK;
  } else if (page_transition & ui::PAGE_TRANSITION_HOME_PAGE) {
    data_use_page_transition = DataUsePageTransition::HOME_PAGE;
  } else {
    switch (page_transition & ui::PAGE_TRANSITION_CORE_MASK) {
      case ui::PAGE_TRANSITION_LINK:
        data_use_page_transition = DataUsePageTransition::LINK;
        break;
      case ui::PAGE_TRANSITION_TYPED:
        data_use_page_transition = DataUsePageTransition::TYPED;
        break;
      case ui::PAGE_TRANSITION_AUTO_BOOKMARK:
        data_use_page_transition = DataUsePageTransition::AUTO_BOOKMARK;
        break;
      case ui::PAGE_TRANSITION_AUTO_SUBFRAME:
      case ui::PAGE_TRANSITION_MANUAL_SUBFRAME:
        data_use_page_transition = DataUsePageTransition::SUBFRAME;
        break;
      case ui::PAGE_TRANSITION_GENERATED:
        data_use_page_transition = DataUsePageTransition::GENERATED;
        break;
      case ui::PAGE_TRANSITION_AUTO_TOPLEVEL:
        data_use_page_transition = DataUsePageTransition::AUTO_TOPLEVEL;
        break;
      case ui::PAGE_TRANSITION_FORM_SUBMIT:
        data_use_page_transition = DataUsePageTransition::FORM_SUBMIT;
        break;
      case ui::PAGE_TRANSITION_RELOAD:
        data_use_page_transition = DataUsePageTransition::RELOAD;
        break;
      case ui::PAGE_TRANSITION_KEYWORD:
      case ui::PAGE_TRANSITION_KEYWORD_GENERATED:
        data_use_page_transition = DataUsePageTransition::KEYWORD;
        break;
      default:
        return;
    }
  }
  DCHECK_NE(DataUsePageTransition::TRANSITION_MAX, data_use_page_transition);

  // Use the more primitive STATIC_HISTOGRAM_POINTER_BLOCK macro because the
  // simple UMA_HISTOGRAM_ENUMERATION macros don't expose 'AddKiB'.
  STATIC_HISTOGRAM_POINTER_BLOCK(
      "DataUse.PageTransition.UserTrafficKB",
      AddKiB(data_use_page_transition, received_bytes),
      base::LinearHistogram::FactoryGet(
          "DataUse.PageTransition.UserTrafficKB", 1,
          DataUsePageTransition::TRANSITION_MAX,
          DataUsePageTransition::TRANSITION_MAX + 1,
          base::HistogramBase::kUmaTargetedHistogramFlag));
}

bool ContentURLRequestClassifier::IsFavIconRequest(
    const net::URLRequest& request) const {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);
  return request_info && request_info->GetResourceType() ==
                             content::ResourceType::RESOURCE_TYPE_FAVICON;
}

}  // namespace data_use_measurement
