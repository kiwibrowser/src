// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/url_pattern_set.h"

#include <iterator>
#include <ostream>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/url_pattern.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace extensions {

namespace {

const char kInvalidURLPatternError[] = "Invalid url pattern '*'";

}  // namespace

// static
URLPatternSet URLPatternSet::CreateDifference(const URLPatternSet& set1,
                                              const URLPatternSet& set2) {
  return URLPatternSet(base::STLSetDifference<std::set<URLPattern>>(
      set1.patterns_, set2.patterns_));
}

// static
URLPatternSet URLPatternSet::CreateIntersection(const URLPatternSet& set1,
                                                const URLPatternSet& set2) {
  return URLPatternSet(base::STLSetIntersection<std::set<URLPattern>>(
      set1.patterns_, set2.patterns_));
}

URLPatternSet URLPatternSet::CreateSemanticIntersection(
    const URLPatternSet& set1,
    const URLPatternSet& set2) {
  URLPatternSet result;
  for (const URLPattern& pattern : set1) {
    if (set2.ContainsPattern(pattern))
      result.patterns_.insert(pattern);
  }
  for (const URLPattern& pattern : set2) {
    if (set1.ContainsPattern(pattern))
      result.patterns_.insert(pattern);
  }
  return result;
}

// static
URLPatternSet URLPatternSet::CreateUnion(const URLPatternSet& set1,
                                         const URLPatternSet& set2) {
  return URLPatternSet(
      base::STLSetUnion<std::set<URLPattern>>(set1.patterns_, set2.patterns_));
}

// static
URLPatternSet URLPatternSet::CreateUnion(
    const std::vector<URLPatternSet>& sets) {
  URLPatternSet result;
  if (sets.empty())
    return result;

  // N-way union algorithm is basic O(nlog(n)) merge algorithm.
  //
  // Do the first merge step into a working set so that we don't mutate any of
  // the input.
  std::vector<URLPatternSet> working;
  for (size_t i = 0; i < sets.size(); i += 2) {
    if (i + 1 < sets.size())
      working.push_back(CreateUnion(sets[i], sets[i + 1]));
    else
      working.push_back(sets[i]);
  }

  for (size_t skip = 1; skip < working.size(); skip *= 2) {
    for (size_t i = 0; i < (working.size() - skip); i += skip) {
      URLPatternSet u = CreateUnion(working[i], working[i + skip]);
      working[i].patterns_.swap(u.patterns_);
    }
  }

  result.patterns_.swap(working[0].patterns_);
  return result;
}

URLPatternSet::URLPatternSet() = default;

URLPatternSet::URLPatternSet(const URLPatternSet& rhs) = default;

URLPatternSet::URLPatternSet(URLPatternSet&& rhs) = default;

URLPatternSet::URLPatternSet(const std::set<URLPattern>& patterns)
    : patterns_(patterns) {}

URLPatternSet::~URLPatternSet() = default;

URLPatternSet& URLPatternSet::operator=(const URLPatternSet& rhs) = default;

URLPatternSet& URLPatternSet::operator=(URLPatternSet&& rhs) = default;

bool URLPatternSet::operator==(const URLPatternSet& other) const {
  return patterns_ == other.patterns_;
}

std::ostream& operator<<(std::ostream& out,
                         const URLPatternSet& url_pattern_set) {
  out << "{ ";

  std::set<URLPattern>::const_iterator iter =
      url_pattern_set.patterns().begin();
  if (!url_pattern_set.patterns().empty()) {
    out << *iter;
    ++iter;
  }

  for (;iter != url_pattern_set.patterns().end(); ++iter)
    out << ", " << *iter;

  if (!url_pattern_set.patterns().empty())
    out << " ";

  out << "}";
  return out;
}

bool URLPatternSet::is_empty() const {
  return patterns_.empty();
}

size_t URLPatternSet::size() const {
  return patterns_.size();
}

bool URLPatternSet::AddPattern(const URLPattern& pattern) {
  return patterns_.insert(pattern).second;
}

void URLPatternSet::AddPatterns(const URLPatternSet& set) {
  patterns_.insert(set.patterns().begin(),
                   set.patterns().end());
}

void URLPatternSet::ClearPatterns() {
  patterns_.clear();
}

bool URLPatternSet::AddOrigin(int valid_schemes, const GURL& origin) {
  if (origin.is_empty())
    return false;
  const url::Origin real_origin = url::Origin::Create(origin);
  DCHECK(real_origin.IsSameOriginWith(url::Origin::Create(origin.GetOrigin())));
  URLPattern origin_pattern(valid_schemes);
  // Origin adding could fail if |origin| does not match |valid_schemes|.
  if (origin_pattern.Parse(origin.spec()) != URLPattern::PARSE_SUCCESS) {
    return false;
  }
  origin_pattern.SetPath("/*");
  return AddPattern(origin_pattern);
}

bool URLPatternSet::Contains(const URLPatternSet& other) const {
  for (URLPatternSet::const_iterator it = other.begin();
       it != other.end(); ++it) {
    if (!ContainsPattern(*it))
      return false;
  }

  return true;
}

bool URLPatternSet::ContainsPattern(const URLPattern& pattern) const {
  for (URLPatternSet::const_iterator it = begin();
       it != end(); ++it) {
    if (it->Contains(pattern))
      return true;
  }
  return false;
}

bool URLPatternSet::MatchesURL(const GURL& url) const {
  for (URLPatternSet::const_iterator pattern = patterns_.begin();
       pattern != patterns_.end(); ++pattern) {
    if (pattern->MatchesURL(url))
      return true;
  }

  return false;
}

bool URLPatternSet::MatchesAllURLs() const {
  for (URLPatternSet::const_iterator host = begin(); host != end(); ++host) {
    if (host->match_all_urls() ||
        (host->match_subdomains() && host->host().empty()))
      return true;
  }
  return false;
}

bool URLPatternSet::MatchesSecurityOrigin(const GURL& origin) const {
  for (URLPatternSet::const_iterator pattern = patterns_.begin();
       pattern != patterns_.end(); ++pattern) {
    if (pattern->MatchesSecurityOrigin(origin))
      return true;
  }

  return false;
}

bool URLPatternSet::OverlapsWith(const URLPatternSet& other) const {
  // Two extension extents overlap if there is any one URL that would match at
  // least one pattern in each of the extents.
  for (URLPatternSet::const_iterator i = patterns_.begin();
       i != patterns_.end(); ++i) {
    for (URLPatternSet::const_iterator j = other.patterns().begin();
         j != other.patterns().end(); ++j) {
      if (i->OverlapsWith(*j))
        return true;
    }
  }

  return false;
}

std::unique_ptr<base::ListValue> URLPatternSet::ToValue() const {
  std::unique_ptr<base::ListValue> value(new base::ListValue);
  for (URLPatternSet::const_iterator i = patterns_.begin();
       i != patterns_.end(); ++i)
    value->AppendIfNotPresent(std::make_unique<base::Value>(i->GetAsString()));
  return value;
}

bool URLPatternSet::Populate(const std::vector<std::string>& patterns,
                             int valid_schemes,
                             bool allow_file_access,
                             std::string* error) {
  ClearPatterns();
  for (size_t i = 0; i < patterns.size(); ++i) {
    URLPattern pattern(valid_schemes);
    if (pattern.Parse(patterns[i]) != URLPattern::PARSE_SUCCESS) {
      if (error) {
        *error = ErrorUtils::FormatErrorMessage(kInvalidURLPatternError,
                                                patterns[i]);
      } else {
        LOG(ERROR) << "Invalid url pattern: " << patterns[i];
      }
      return false;
    }
    if (!allow_file_access && pattern.MatchesScheme(url::kFileScheme)) {
      pattern.SetValidSchemes(
          pattern.valid_schemes() & ~URLPattern::SCHEME_FILE);
    }
    AddPattern(pattern);
  }
  return true;
}

std::unique_ptr<std::vector<std::string>> URLPatternSet::ToStringVector()
    const {
  std::unique_ptr<std::vector<std::string>> value(new std::vector<std::string>);
  for (URLPatternSet::const_iterator i = patterns_.begin();
       i != patterns_.end();
       ++i) {
    value->push_back(i->GetAsString());
  }
  return value;
}

bool URLPatternSet::Populate(const base::ListValue& value,
                             int valid_schemes,
                             bool allow_file_access,
                             std::string* error) {
  std::vector<std::string> patterns;
  for (size_t i = 0; i < value.GetSize(); ++i) {
    std::string item;
    if (!value.GetString(i, &item))
      return false;
    patterns.push_back(item);
  }
  return Populate(patterns, valid_schemes, allow_file_access, error);
}

}  // namespace extensions
