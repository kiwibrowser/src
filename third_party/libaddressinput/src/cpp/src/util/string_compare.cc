// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "string_compare.h"

#include <cassert>
#include <string>

#include <re2/re2.h>

#include "lru_cache_using_std.h"

// RE2 uses type string, which is not necessarily the same as type std::string.
// In order to create objects of the correct type, to be able to pass pointers
// to these objects to RE2, the function that does that is defined inside an
// unnamed namespace inside the re2 namespace. Oh, my ...
namespace re2 {
namespace {

// In order to (mis-)use RE2 to implement UTF-8 capable less<>, this function
// calls RE2::PossibleMatchRange() to calculate the "lessest" string that would
// be a case-insensitive match to the string. This is far too expensive to do
// repeatedly, so the function is only ever called through an LRU cache.
std::string ComputeMinPossibleMatch(const std::string& str) {
  string min, max;  // N.B.: RE2 type string!

  RE2::Options options;
  options.set_literal(true);
  options.set_case_sensitive(false);
  RE2 matcher(str, options);

  bool success = matcher.PossibleMatchRange(&min, &max, str.size());
  assert(success);
  (void)success;  // Prevent unused variable if assert() is optimized away.

  return min;
}

}  // namespace
}  // namespace re2

namespace i18n {
namespace addressinput {

class StringCompare::Impl {
  enum { MAX_CACHE_SIZE = 1 << 15 };

 public:
  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;

  Impl() : min_possible_match_(&re2::ComputeMinPossibleMatch, MAX_CACHE_SIZE) {
    options_.set_literal(true);
    options_.set_case_sensitive(false);
  }

  ~Impl() {}

  bool NaturalEquals(const std::string& a, const std::string& b) const {
    RE2 matcher(b, options_);
    return RE2::FullMatch(a, matcher);
  }

  bool NaturalLess(const std::string& a, const std::string& b) const {
    const std::string& min_a(min_possible_match_(a));
    const std::string& min_b(min_possible_match_(b));
    return min_a < min_b;
  }

 private:
  RE2::Options options_;
  mutable lru_cache_using_std<std::string, std::string> min_possible_match_;
};

StringCompare::StringCompare() : impl_(new Impl) {}

StringCompare::~StringCompare() {}

bool StringCompare::NaturalEquals(const std::string& a,
                                  const std::string& b) const {
  return impl_->NaturalEquals(a, b);
}

bool StringCompare::NaturalLess(const std::string& a,
                                const std::string& b) const {
  return impl_->NaturalLess(a, b);
}

}  // namespace addressinput
}  // namespace i18n
