// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/ukm_source.h"

#include "base/atomicops.h"
#include "base/hash.h"
#include "third_party/metrics_proto/ukm/source.pb.h"

namespace ukm {

namespace {

// The maximum length of a URL we will record.
constexpr int kMaxURLLength = 2 * 1024;

// The string sent in place of a URL if the real URL was too long.
constexpr char kMaxUrlLengthMessage[] = "URLTooLong";

// Using a simple global assumes that all access to it will be done on the same
// thread, namely the UI thread. If this becomes not the case then it can be
// changed to an Atomic32 (make CustomTabState derive from int32_t) and accessed
// with no-barrier loads and stores.
UkmSource::CustomTabState g_custom_tab_state = UkmSource::kCustomTabUnset;

// Returns a URL that is under the length limit, by returning a constant
// string when the URl is too long.
std::string GetShortenedURL(const GURL& url) {
  if (url.spec().length() > kMaxURLLength)
    return kMaxUrlLengthMessage;
  return url.spec();
}

}  // namespace

// static
void UkmSource::SetCustomTabVisible(bool visible) {
  g_custom_tab_state = visible ? kCustomTabTrue : kCustomTabFalse;
}

UkmSource::UkmSource(ukm::SourceId id, const GURL& url)
    : id_(id),
      url_(url),
      custom_tab_state_(g_custom_tab_state),
      creation_time_(base::TimeTicks::Now()) {
  DCHECK(!url_.is_empty());
}

UkmSource::~UkmSource() = default;

void UkmSource::UpdateUrl(const GURL& url) {
  DCHECK(!url.is_empty());
  if (url_ == url)
    return;
  // We only track multiple URLs for navigation-based Sources.  These are
  // currently the only sources that intentionally record multiple entries,
  // and this makes it easier to compare other source URLs against a
  // whitelist.
  if (initial_url_.is_empty() &&
      GetSourceIdType(id_) == SourceIdType::NAVIGATION_ID)
    initial_url_ = url_;
  url_ = url;
}

void UkmSource::PopulateProto(Source* proto_source) const {
  DCHECK(!proto_source->has_id());
  DCHECK(!proto_source->has_url());
  DCHECK(!proto_source->has_initial_url());

  proto_source->set_id(id_);
  proto_source->set_url(GetShortenedURL(url_));
  if (!initial_url_.is_empty()) {
    DCHECK_EQ(SourceIdType::NAVIGATION_ID, GetSourceIdType(id_));
    proto_source->set_initial_url(GetShortenedURL(initial_url_));
  }

  if (custom_tab_state_ != kCustomTabUnset)
    proto_source->set_is_custom_tab(custom_tab_state_ == kCustomTabTrue);
}

}  // namespace ukm
