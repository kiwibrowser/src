// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_URL_PATTERN_INDEX_URL_PATTERN_H_
#define COMPONENTS_URL_PATTERN_INDEX_URL_PATTERN_H_

#include <iosfwd>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "components/url_pattern_index/proto/rules.pb.h"

class GURL;

namespace url_pattern_index {

namespace flat {
struct UrlRule;  // The FlatBuffers version of UrlRule.
}

// The structure used to mirror a URL pattern regardless of the representation
// of the UrlRule that owns it, and to match it against URLs.
class UrlPattern {
 public:
  UrlPattern();

  // Creates a |url_pattern| of a certain |type|.
  UrlPattern(base::StringPiece url_pattern,
             proto::UrlPatternType type = proto::URL_PATTERN_TYPE_WILDCARDED);

  // Creates a WILDCARDED |url_pattern| with the specified anchors.
  UrlPattern(base::StringPiece url_pattern,
             proto::AnchorType anchor_left,
             proto::AnchorType anchor_right);

  // The following constructors create UrlPattern from one of the UrlRule
  // representations. The passed in |rule| must outlive the created instance.
  explicit UrlPattern(const flat::UrlRule& rule);
  explicit UrlPattern(const proto::UrlRule& rule);

  ~UrlPattern();

  proto::UrlPatternType type() const { return type_; }
  base::StringPiece url_pattern() const { return url_pattern_; }
  proto::AnchorType anchor_left() const { return anchor_left_; }
  proto::AnchorType anchor_right() const { return anchor_right_; }
  bool match_case() const { return match_case_; }

  // Returns whether the |url| matches the URL |pattern|. Requires the type of
  // |this| pattern to be either SUBSTRING or WILDCARDED.
  //
  // Splits the pattern into subpatterns separated by '*' wildcards, and
  // greedily finds each of them in the spec of the |url|. Respects anchors at
  // either end of the pattern, and '^' separator placeholders when comparing a
  // subpattern to a subtring of the spec.
  bool MatchesUrl(const GURL& url) const;

 private:
  // TODO(pkalinnikov): Store flat:: types instead of proto::, in order to avoid
  // conversions in IndexedRuleset.
  proto::UrlPatternType type_ = proto::URL_PATTERN_TYPE_UNSPECIFIED;
  base::StringPiece url_pattern_;

  proto::AnchorType anchor_left_ = proto::ANCHOR_TYPE_NONE;
  proto::AnchorType anchor_right_ = proto::ANCHOR_TYPE_NONE;

  // TODO(pkalinnikov): Implement case-insensitive matching.
  bool match_case_ = false;

  DISALLOW_COPY_AND_ASSIGN(UrlPattern);
};

// Allow pretty-printing URLPatterns when they are used in GTest assertions.
std::ostream& operator<<(std::ostream& out, const UrlPattern& pattern);

}  // namespace url_pattern_index

#endif  // COMPONENTS_URL_PATTERN_INDEX_URL_PATTERN_H_
