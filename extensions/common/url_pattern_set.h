// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_URL_PATTERN_SET_H_
#define EXTENSIONS_COMMON_URL_PATTERN_SET_H_

#include <stddef.h>

#include <iosfwd>
#include <memory>
#include <set>

#include "extensions/common/url_pattern.h"

class GURL;

namespace base {
class ListValue;
class Value;
}

namespace extensions {

// Represents the set of URLs an extension uses for web content.
class URLPatternSet {
 public:
  typedef std::set<URLPattern>::const_iterator const_iterator;
  typedef std::set<URLPattern>::iterator iterator;

  // Returns |set1| - |set2|.
  static URLPatternSet CreateDifference(const URLPatternSet& set1,
                                        const URLPatternSet& set2);

  // Returns the intersection of |set1| and |set2|.
  static URLPatternSet CreateIntersection(const URLPatternSet& set1,
                                          const URLPatternSet& set2);

  // Creates an intersection result where result has every element that is
  // contained by both |set1| and |set2|. This is different than
  // CreateIntersection(), which does string comparisons. For example, the
  // semantic intersection of ("http://*.google.com/*") and
  // ("http://google.com/*") is ("http://google.com/*"), but the result from
  // CreateIntersection() would be ().
  // TODO(devlin): This is weird. We probably want to use mostly
  // CreateSemanticIntersection().
  static URLPatternSet CreateSemanticIntersection(const URLPatternSet& set1,
                                                  const URLPatternSet& set2);

  // Returns the union of |set1| and |set2|.
  static URLPatternSet CreateUnion(const URLPatternSet& set1,
                                   const URLPatternSet& set2);

  // Returns the union of all sets in |sets|.
  static URLPatternSet CreateUnion(const std::vector<URLPatternSet>& sets);

  URLPatternSet();
  URLPatternSet(const URLPatternSet& rhs);
  URLPatternSet(URLPatternSet&& rhs);
  explicit URLPatternSet(const std::set<URLPattern>& patterns);
  ~URLPatternSet();

  URLPatternSet& operator=(const URLPatternSet& rhs);
  URLPatternSet& operator=(URLPatternSet&& rhs);
  bool operator==(const URLPatternSet& rhs) const;

  bool is_empty() const;
  size_t size() const;
  const std::set<URLPattern>& patterns() const { return patterns_; }
  const_iterator begin() const { return patterns_.begin(); }
  const_iterator end() const { return patterns_.end(); }

  // Adds a pattern to the set. Returns true if a new pattern was inserted,
  // false if the pattern was already in the set.
  bool AddPattern(const URLPattern& pattern);

  // Adds all patterns from |set| into this.
  void AddPatterns(const URLPatternSet& set);

  void ClearPatterns();

  // Adds a pattern based on |origin| to the set.
  bool AddOrigin(int valid_schemes, const GURL& origin);

  // Returns true if every URL that matches |set| is matched by this. In other
  // words, if every pattern in |set| is encompassed by a pattern in this.
  bool Contains(const URLPatternSet& set) const;

  // Returns true if any pattern in this set encompasses |pattern|.
  bool ContainsPattern(const URLPattern& pattern) const;

  // Test if the extent contains a URL.
  bool MatchesURL(const GURL& url) const;

  // Test if the extent matches all URLs (for example, <all_urls>).
  bool MatchesAllURLs() const;

  bool MatchesSecurityOrigin(const GURL& origin) const;

  // Returns true if there is a single URL that would be in two extents.
  bool OverlapsWith(const URLPatternSet& other) const;

  // Converts to and from Value for serialization to preferences.
  std::unique_ptr<base::ListValue> ToValue() const;
  bool Populate(const base::ListValue& value,
                int valid_schemes,
                bool allow_file_access,
                std::string* error);

  // Converts to and from a vector of strings.
  std::unique_ptr<std::vector<std::string>> ToStringVector() const;
  bool Populate(const std::vector<std::string>& patterns,
                int valid_schemes,
                bool allow_file_access,
                std::string* error);

 private:
  // The list of URL patterns that comprise the extent.
  std::set<URLPattern> patterns_;
};

std::ostream& operator<<(std::ostream& out,
                         const URLPatternSet& url_pattern_set);

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_URL_PATTERN_SET_H_
