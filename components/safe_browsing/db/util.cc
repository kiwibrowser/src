// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/db/util.h"

#include <stddef.h>

#ifndef NDEBUG
#include "base/base64.h"
#endif
#include "base/environment.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "components/version_info/version_info.h"
#include "crypto/sha2.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "url/gurl.h"

namespace safe_browsing {

// Utility functions -----------------------------------------------------------

namespace {

bool IsKnownList(const std::string& name) {
  for (size_t i = 0; i < arraysize(kAllLists); ++i) {
    if (!strcmp(kAllLists[i], name.c_str())) {
      return true;
    }
  }
  return false;
}

}  // namespace

// ThreatMetadata ------------------------------------------------------------
ThreatMetadata::ThreatMetadata()
    : threat_pattern_type(ThreatPatternType::NONE) {}

ThreatMetadata::ThreatMetadata(const ThreatMetadata& other) = default;

ThreatMetadata::~ThreatMetadata() {}

bool ThreatMetadata::operator==(const ThreatMetadata& other) const {
  return threat_pattern_type == other.threat_pattern_type &&
         api_permissions == other.api_permissions &&
         subresource_filter_match == other.subresource_filter_match &&
         population_id == other.population_id;
}

bool ThreatMetadata::operator!=(const ThreatMetadata& other) const {
  return !operator==(other);
}

std::unique_ptr<base::trace_event::TracedValue> ThreatMetadata::ToTracedValue()
    const {
  auto value = std::make_unique<base::trace_event::TracedValue>();

  value->SetInteger("threat_pattern_type",
                    static_cast<int>(threat_pattern_type));

  value->BeginArray("api_permissions");
  for (const std::string& permission : api_permissions) {
    value->AppendString(permission);
  }
  value->EndArray();

  value->BeginDictionary("subresource_filter_match");
  for (const auto& it : subresource_filter_match) {
    value->BeginArray("match_metadata");
    value->AppendInteger(static_cast<int>(it.first));
    value->AppendInteger(static_cast<int>(it.second));
    value->EndArray();
  }
  value->EndDictionary();

  value->SetString("popuplation_id", population_id);
  return value;
}

// SBCachedFullHashResult ------------------------------------------------------

SBCachedFullHashResult::SBCachedFullHashResult() {}

SBCachedFullHashResult::SBCachedFullHashResult(
    const base::Time& in_expire_after)
    : expire_after(in_expire_after) {}

SBCachedFullHashResult::SBCachedFullHashResult(
    const SBCachedFullHashResult& other) = default;

SBCachedFullHashResult::~SBCachedFullHashResult() {}

// Listnames that browser can process.
const char kMalwareList[] = "goog-malware-shavar";
const char kPhishingList[] = "goog-phish-shavar";
const char kBinUrlList[] = "goog-badbinurl-shavar";
const char kCsdWhiteList[] = "goog-csdwhite-sha256";
const char kDownloadWhiteList[] = "goog-downloadwhite-digest256";
const char kExtensionBlacklist[] = "goog-badcrxids-digestvar";
const char kIPBlacklist[] = "goog-badip-digest256";
const char kUnwantedUrlList[] = "goog-unwanted-shavar";
const char kResourceBlacklist[] = "goog-badresource-shavar";

const char* kAllLists[9] = {
    kMalwareList,  kPhishingList,      kBinUrlList,
    kCsdWhiteList, kDownloadWhiteList, kExtensionBlacklist,
    kIPBlacklist,  kUnwantedUrlList,   kResourceBlacklist,
};

ListType GetListId(const base::StringPiece& name) {
  ListType id;
  if (name == kMalwareList) {
    id = MALWARE;
  } else if (name == kPhishingList) {
    id = PHISH;
  } else if (name == kBinUrlList) {
    id = BINURL;
  } else if (name == kCsdWhiteList) {
    id = CSDWHITELIST;
  } else if (name == kDownloadWhiteList) {
    id = DOWNLOADWHITELIST;
  } else if (name == kExtensionBlacklist) {
    id = EXTENSIONBLACKLIST;
  } else if (name == kIPBlacklist) {
    id = IPBLACKLIST;
  } else if (name == kUnwantedUrlList) {
    id = UNWANTEDURL;
  } else if (name == kResourceBlacklist) {
    id = RESOURCEBLACKLIST;
  } else {
    id = INVALID;
  }
  return id;
}

bool GetListName(ListType list_id, std::string* list) {
  switch (list_id) {
    case MALWARE:
      *list = kMalwareList;
      break;
    case PHISH:
      *list = kPhishingList;
      break;
    case BINURL:
      *list = kBinUrlList;
      break;
    case CSDWHITELIST:
      *list = kCsdWhiteList;
      break;
    case DOWNLOADWHITELIST:
      *list = kDownloadWhiteList;
      break;
    case EXTENSIONBLACKLIST:
      *list = kExtensionBlacklist;
      break;
    case IPBLACKLIST:
      *list = kIPBlacklist;
      break;
    case UNWANTEDURL:
      *list = kUnwantedUrlList;
      break;
    case RESOURCEBLACKLIST:
      *list = kResourceBlacklist;
      break;
    default:
      return false;
  }
  DCHECK(IsKnownList(*list));
  return true;
}

SBFullHash SBFullHashForString(const base::StringPiece& str) {
  SBFullHash h;
  crypto::SHA256HashString(str, &h.full_hash, sizeof(h.full_hash));
  return h;
}

SBFullHash StringToSBFullHash(const std::string& hash_in) {
  DCHECK_EQ(crypto::kSHA256Length, hash_in.size());
  SBFullHash hash_out;
  memcpy(hash_out.full_hash, hash_in.data(), crypto::kSHA256Length);
  return hash_out;
}

std::string SBFullHashToString(const SBFullHash& hash) {
  DCHECK_EQ(crypto::kSHA256Length, sizeof(hash.full_hash));
  return std::string(hash.full_hash, sizeof(hash.full_hash));
}

void UrlToFullHashes(const GURL& url,
                     bool include_whitelist_hashes,
                     std::vector<SBFullHash>* full_hashes) {
  // Include this function in traces because it's not cheap so it should be
  // called sparingly.
  TRACE_EVENT2("loader", "safe_browsing::UrlToFullHashes", "url", url.spec(),
               "include_whitelist_hashes", include_whitelist_hashes);
  std::string canon_host;
  std::string canon_path;
  std::string canon_query;
  V4ProtocolManagerUtil::CanonicalizeUrl(url, &canon_host, &canon_path,
                                         &canon_query);

  std::vector<std::string> hosts;
  if (url.HostIsIPAddress()) {
    hosts.push_back(url.host());
  } else {
    V4ProtocolManagerUtil::GenerateHostVariantsToCheck(canon_host, &hosts);
  }

  std::vector<std::string> paths;
  V4ProtocolManagerUtil::GeneratePathVariantsToCheck(canon_path, canon_query,
                                                     &paths);

  for (const std::string& host : hosts) {
    for (const std::string& path : paths) {
      full_hashes->push_back(SBFullHashForString(host + path));

      // We may have /foo as path-prefix in the whitelist which should
      // also match with /foo/bar and /foo?bar.  Hence, for every path
      // that ends in '/' we also add the path without the slash.
      if (include_whitelist_hashes && path.size() > 1 && path.back() == '/') {
        full_hashes->push_back(
            SBFullHashForString(host + path.substr(0, path.size() - 1)));
      }
    }
  }
}

SafeBrowsingProtocolConfig::SafeBrowsingProtocolConfig()
    : disable_auto_update(false) {}

SafeBrowsingProtocolConfig::SafeBrowsingProtocolConfig(
    const SafeBrowsingProtocolConfig& other) = default;

SafeBrowsingProtocolConfig::~SafeBrowsingProtocolConfig() {}

namespace ProtocolManagerHelper {

std::string Version() {
  if (version_info::GetVersionNumber().empty())
    return "0.1";
  else
    return version_info::GetVersionNumber();
}

std::string ComposeUrl(const std::string& prefix,
                       const std::string& method,
                       const std::string& client_name,
                       const std::string& version,
                       const std::string& additional_query) {
  DCHECK(!prefix.empty() && !method.empty() && !client_name.empty() &&
         !version.empty());
  std::string url =
      base::StringPrintf("%s/%s?client=%s&appver=%s&pver=3.0", prefix.c_str(),
                         method.c_str(), client_name.c_str(), version.c_str());
  std::string api_key = google_apis::GetAPIKey();
  if (!api_key.empty()) {
    base::StringAppendF(&url, "&key=%s",
                        net::EscapeQueryParamValue(api_key, true).c_str());
  }
  if (!additional_query.empty()) {
    DCHECK(url.find("?") != std::string::npos);
    url.append("&");
    url.append(additional_query);
  }
  return url;
}

std::string ComposeUrl(const std::string& prefix,
                       const std::string& method,
                       const std::string& client_name,
                       const std::string& version,
                       const std::string& additional_query,
                       ExtendedReportingLevel reporting_level) {
  std::string url =
      ComposeUrl(prefix, method, client_name, version, additional_query);
  url.append(base::StringPrintf("&ext=%d", reporting_level));
  return url;
}

}  // namespace ProtocolManagerHelper

}  // namespace safe_browsing
