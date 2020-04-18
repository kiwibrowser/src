// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The matching logic distinguishes between the terms URL pattern and
// subpattern. A URL pattern usually stands for the full thing, e.g.
// "example.com^*path*par=val^", whereas subpattern denotes a maximal substring
// of a pattern not containing the wildcard '*' character. For the example above
// the subpatterns are: "example.com^", "path" and "par=val^".
//
// The separator placeholder '^' symbol is used in subpatterns to match any
// separator character, which is any ASCII symbol except letters, digits, and
// the following: '_', '-', '.', '%'. Note that the separator placeholder
// character '^' is itself a separator, as well as '\0'.

#include "components/url_pattern_index/url_pattern.h"

#include <stddef.h>

#include <ostream>

#include "base/logging.h"
#include "base/numerics/checked_math.h"
#include "components/url_pattern_index/flat/url_pattern_index_generated.h"
#include "components/url_pattern_index/fuzzy_pattern_matching.h"
#include "components/url_pattern_index/string_splitter.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"

namespace url_pattern_index {

namespace {

class IsWildcard {
 public:
  bool operator()(char c) const { return c == '*'; }
};

proto::UrlPatternType ConvertUrlPatternType(flat::UrlPatternType type) {
  switch (type) {
    case flat::UrlPatternType_SUBSTRING:
      return proto::URL_PATTERN_TYPE_SUBSTRING;
    case flat::UrlPatternType_WILDCARDED:
      return proto::URL_PATTERN_TYPE_WILDCARDED;
    case flat::UrlPatternType_REGEXP:
      return proto::URL_PATTERN_TYPE_REGEXP;
    default:
      return proto::URL_PATTERN_TYPE_UNSPECIFIED;
  }
}

proto::AnchorType ConvertAnchorType(flat::AnchorType type) {
  switch (type) {
    case flat::AnchorType_NONE:
      return proto::ANCHOR_TYPE_NONE;
    case flat::AnchorType_BOUNDARY:
      return proto::ANCHOR_TYPE_BOUNDARY;
    case flat::AnchorType_SUBDOMAIN:
      return proto::ANCHOR_TYPE_SUBDOMAIN;
    default:
      return proto::ANCHOR_TYPE_UNSPECIFIED;
  }
}

base::StringPiece ConvertString(const flatbuffers::String* string) {
  return string ? base::StringPiece(string->data(), string->size())
                : base::StringPiece();
}

// Returns whether |position| within the |url| belongs to its |host| component
// and corresponds to the beginning of a (sub-)domain.
inline bool IsSubdomainAnchored(base::StringPiece url,
                                url::Component host,
                                size_t position) {
  DCHECK_LE(position, url.size());
  const size_t host_begin = static_cast<size_t>(host.begin);
  const size_t host_end = static_cast<size_t>(host.end());
  DCHECK_LE(host_end, url.size());

  return position == host_begin ||
         (position > host_begin && position <= host_end &&
          url[position - 1] == '.');
}

// Returns the position of the leftmost occurrence of a |subpattern| in the
// |text| starting no earlier than |from| the specified position. If the
// |subpattern| has separator placeholders, searches for a fuzzy occurrence.
size_t FindSubpattern(base::StringPiece text,
                      base::StringPiece subpattern,
                      size_t from = 0) {
  const bool is_fuzzy =
      (subpattern.find(kSeparatorPlaceholder) != base::StringPiece::npos);
  return is_fuzzy ? FindFuzzy(text, subpattern, from)
                  : text.find(subpattern, from);
}

// Same as FindSubpattern(url, subpattern), but searches for an occurrence that
// starts at the beginning of a (sub-)domain within the url's |host| component.
size_t FindSubdomainAnchoredSubpattern(base::StringPiece url,
                                       url::Component host,
                                       base::StringPiece subpattern) {
  const bool is_fuzzy =
      (subpattern.find(kSeparatorPlaceholder) != base::StringPiece::npos);

  // Any match found after the end of the host will be discarded, so just
  // avoid searching there for the subpattern to begin with.
  //
  // Check for overflow.
  size_t max_match_end = 0;
  if (!base::CheckAdd(host.end(), subpattern.length())
           .AssignIfValid(&max_match_end)) {
    return base::StringPiece::npos;
  }
  const base::StringPiece url_match_candidate = url.substr(0, max_match_end);
  const base::StringPiece url_host = url.substr(0, host.end());

  for (size_t position = static_cast<size_t>(host.begin);
       position <= static_cast<size_t>(host.end()); ++position) {
    // Enforce as a loop precondition that we are always anchored at a
    // sub-domain before calling find. This is to reduce the number of potential
    // searches for |subpattern|.
    DCHECK(IsSubdomainAnchored(url, host, position));

    position = is_fuzzy ? FindFuzzy(url_match_candidate, subpattern, position)
                        : url_match_candidate.find(subpattern, position);
    if (position == base::StringPiece::npos ||
        IsSubdomainAnchored(url, host, position)) {
      return position;
    }

    // Enforce the loop precondition. This skips |position| to the next '.',
    // within the host, which the loop itself increments to the anchored
    // sub-domain.
    position = url_host.find('.', position);
    if (position == base::StringPiece::npos)
      break;
  }
  return base::StringPiece::npos;
}

}  // namespace

UrlPattern::UrlPattern() = default;

UrlPattern::UrlPattern(base::StringPiece url_pattern,
                       proto::UrlPatternType type)
    : type_(type), url_pattern_(url_pattern) {}

UrlPattern::UrlPattern(base::StringPiece url_pattern,
                       proto::AnchorType anchor_left,
                       proto::AnchorType anchor_right)
    : type_(proto::URL_PATTERN_TYPE_WILDCARDED),
      url_pattern_(url_pattern),
      anchor_left_(anchor_left),
      anchor_right_(anchor_right) {}

UrlPattern::UrlPattern(const flat::UrlRule& rule)
    : type_(ConvertUrlPatternType(rule.url_pattern_type())),
      url_pattern_(ConvertString(rule.url_pattern())),
      anchor_left_(ConvertAnchorType(rule.anchor_left())),
      anchor_right_(ConvertAnchorType(rule.anchor_right())),
      match_case_(!!(rule.options() & flat::OptionFlag_IS_MATCH_CASE)) {}

UrlPattern::UrlPattern(const proto::UrlRule& rule)
    : type_(rule.url_pattern_type()),
      url_pattern_(rule.url_pattern()),
      anchor_left_(rule.anchor_left()),
      anchor_right_(rule.anchor_right()),
      match_case_(rule.match_case()) {}

UrlPattern::~UrlPattern() = default;

bool UrlPattern::MatchesUrl(const GURL& url) const {
  // Note: Empty domains are also invalid.
  DCHECK(url.is_valid());
  DCHECK(type_ == proto::URL_PATTERN_TYPE_SUBSTRING ||
         type_ == proto::URL_PATTERN_TYPE_WILDCARDED);

  StringSplitter<IsWildcard> subpatterns(url_pattern_);
  auto subpattern_it = subpatterns.begin();
  auto subpattern_end = subpatterns.end();

  if (subpattern_it == subpattern_end) {
    return anchor_left_ == proto::ANCHOR_TYPE_NONE ||
           anchor_right_ == proto::ANCHOR_TYPE_NONE;
  }

  const base::StringPiece spec = url.possibly_invalid_spec();
  const url::Component host_part = url.parsed_for_possibly_invalid_spec().host;
  DCHECK(!spec.empty());

  base::StringPiece subpattern = *subpattern_it;
  ++subpattern_it;

  // If there is only one |subpattern|, and it has a right anchor, then simply
  // check that it is a suffix of the |spec|, and the left anchor is fulfilled.
  if (subpattern_it == subpattern_end &&
      anchor_right_ == proto::ANCHOR_TYPE_BOUNDARY) {
    if (!EndsWithFuzzy(spec, subpattern))
      return false;
    if (anchor_left_ == proto::ANCHOR_TYPE_BOUNDARY)
      return spec.size() == subpattern.size();
    if (anchor_left_ == proto::ANCHOR_TYPE_SUBDOMAIN) {
      DCHECK_LE(subpattern.size(), spec.size());
      return url.has_host() &&
             IsSubdomainAnchored(spec, host_part,
                                 spec.size() - subpattern.size());
    }
    return true;
  }

  // Otherwise, the first |subpattern| does not have to be a suffix. But it
  // still can have a left anchor. Check and handle that.
  base::StringPiece text = spec;
  if (anchor_left_ == proto::ANCHOR_TYPE_BOUNDARY) {
    if (!StartsWithFuzzy(spec, subpattern))
      return false;
    if (subpattern_it == subpattern_end) {
      DCHECK_EQ(anchor_right_, proto::ANCHOR_TYPE_NONE);
      return true;
    }
    text.remove_prefix(subpattern.size());
  } else if (anchor_left_ == proto::ANCHOR_TYPE_SUBDOMAIN) {
    if (!url.has_host())
      return false;
    const size_t match_begin =
        FindSubdomainAnchoredSubpattern(spec, host_part, subpattern);
    if (match_begin == base::StringPiece::npos)
      return false;
    if (subpattern_it == subpattern_end) {
      DCHECK_EQ(anchor_right_, proto::ANCHOR_TYPE_NONE);
      return true;
    }
    text.remove_prefix(match_begin + subpattern.size());
  } else {
    DCHECK_EQ(anchor_left_, proto::ANCHOR_TYPE_NONE);
    // Get back to the initial |subpattern|, process it in the loop below.
    subpattern_it = subpatterns.begin();
  }

  // Consecutively find all the remaining subpatterns in the |text|. If the
  // pattern has a right anchor, don't search for the last subpattern, but
  // instead check that it is a suffix of the |text|.
  while (subpattern_it != subpattern_end) {
    subpattern = *subpattern_it;
    DCHECK(!subpattern.empty());

    if (++subpattern_it == subpattern_end &&
        anchor_right_ == proto::ANCHOR_TYPE_BOUNDARY) {
      break;
    }

    const size_t match_position = FindSubpattern(text, subpattern);
    if (match_position == base::StringPiece::npos)
      return false;
    text.remove_prefix(match_position + subpattern.size());
  }

  return anchor_right_ != proto::ANCHOR_TYPE_BOUNDARY ||
         EndsWithFuzzy(text, subpattern);
}

std::ostream& operator<<(std::ostream& out, const UrlPattern& pattern) {
  switch (pattern.anchor_left()) {
    case proto::ANCHOR_TYPE_SUBDOMAIN:
      out << '|';
      FALLTHROUGH;
    case proto::ANCHOR_TYPE_BOUNDARY:
      out << '|';
      FALLTHROUGH;
    default:
      break;
  }
  out << pattern.url_pattern();
  if (pattern.anchor_right() == proto::ANCHOR_TYPE_BOUNDARY)
    out << '|';
  if (pattern.match_case())
    out << "$match-case";

  return out;
}

}  // namespace url_pattern_index
