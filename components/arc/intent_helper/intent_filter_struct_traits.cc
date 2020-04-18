// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/intent_helper/intent_filter_struct_traits.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"

namespace mojo {

bool StructTraits<arc::mojom::IntentFilterDataView, arc::IntentFilter>::
    Read(arc::mojom::IntentFilterDataView data,
         arc::IntentFilter* out) {
  std::vector<arc::IntentFilter::AuthorityEntry> authorities;
  if (!data.ReadDataAuthorities(&authorities))
    return false;

  std::vector<arc::IntentFilter::PatternMatcher> paths;
  if (!data.ReadDataPaths(&paths))
    return false;

  *out = arc::IntentFilter(std::move(authorities), std::move(paths));
  return true;
}

bool StructTraits<arc::mojom::AuthorityEntryDataView,
                    arc::IntentFilter::AuthorityEntry>::
    Read(arc::mojom::AuthorityEntryDataView data,
         arc::IntentFilter::AuthorityEntry* out) {
  std::string host;
  bool result = data.ReadHost(&host);
  if (!result)
    return false;

  *out = arc::IntentFilter::AuthorityEntry(std::move(host), data.port());
  return true;
}

bool StructTraits<arc::mojom::PatternMatcherDataView,
                    arc::IntentFilter::PatternMatcher>::
  Read(arc::mojom::PatternMatcherDataView data,
                   arc::IntentFilter::PatternMatcher* out) {
  std::string pattern;
  if (!data.ReadPattern(&pattern))
    return false;

  arc::mojom::PatternType type;
  if (!data.ReadType(&type))
    return false;

  *out = arc::IntentFilter::PatternMatcher(std::move(pattern), type);
  return true;
}

}  // namespace mojo
