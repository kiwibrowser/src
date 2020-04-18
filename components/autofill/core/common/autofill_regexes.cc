// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/autofill_regexes.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/threading/thread_local.h"
#include "third_party/icu/source/i18n/unicode/regex.h"

namespace {

// A thread-local class that serves as a cache of compiled regex patterns.
//
// The regexp state can be accessed from multiple threads in single process
// mode, and this class offers per-thread instance instead of per-process
// singleton instance (https://crbug.com/812182).
class AutofillRegexes {
 public:
  static AutofillRegexes* ThreadSpecificInstance();

  // Returns the compiled regex matcher corresponding to |pattern|.
  icu::RegexMatcher* GetMatcher(const base::string16& pattern);

 private:
  AutofillRegexes();
  ~AutofillRegexes();

  // Maps patterns to their corresponding regex matchers.
  std::unordered_map<base::string16, std::unique_ptr<icu::RegexMatcher>>
      matchers_;

  DISALLOW_COPY_AND_ASSIGN(AutofillRegexes);
};

base::LazyInstance<base::ThreadLocalPointer<AutofillRegexes>>::Leaky
    g_autofill_regexes_tls = LAZY_INSTANCE_INITIALIZER;

// static
AutofillRegexes* AutofillRegexes::ThreadSpecificInstance() {
  if (g_autofill_regexes_tls.Pointer()->Get())
    return g_autofill_regexes_tls.Pointer()->Get();
  return new AutofillRegexes;
}

AutofillRegexes::AutofillRegexes() {
  g_autofill_regexes_tls.Pointer()->Set(this);
}

AutofillRegexes::~AutofillRegexes() {
  g_autofill_regexes_tls.Pointer()->Set(nullptr);
}

icu::RegexMatcher* AutofillRegexes::GetMatcher(const base::string16& pattern) {
  auto it = matchers_.find(pattern);
  if (it == matchers_.end()) {
    const icu::UnicodeString icu_pattern(FALSE, pattern.data(),
                                         pattern.length());

    UErrorCode status = U_ZERO_ERROR;
    auto matcher = std::make_unique<icu::RegexMatcher>(
        icu_pattern, UREGEX_CASE_INSENSITIVE, status);
    DCHECK(U_SUCCESS(status));

    auto result = matchers_.insert(std::make_pair(pattern, std::move(matcher)));
    DCHECK(result.second);
    it = result.first;
  }
  return it->second.get();
}

}  // namespace

namespace autofill {

bool MatchesPattern(const base::string16& input,
                    const base::string16& pattern) {
  icu::RegexMatcher* matcher =
      AutofillRegexes::ThreadSpecificInstance()->GetMatcher(pattern);
  icu::UnicodeString icu_input(FALSE, input.data(), input.length());
  matcher->reset(icu_input);

  UErrorCode status = U_ZERO_ERROR;
  UBool match = matcher->find(0, status);
  DCHECK(U_SUCCESS(status));
  return match == TRUE;
}

}  // namespace autofill
