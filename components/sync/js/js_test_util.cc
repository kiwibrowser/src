// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/js/js_test_util.h"

#include <memory>

#include "base/macros.h"
#include "components/sync/js/js_event_details.h"

namespace syncer {

void PrintTo(const JsEventDetails& details, ::std::ostream* os) {
  *os << details.ToString();
}

namespace {

// Matcher implementation for HasDetails().
class HasDetailsMatcher
    : public ::testing::MatcherInterface<const JsEventDetails&> {
 public:
  explicit HasDetailsMatcher(const JsEventDetails& expected_details)
      : expected_details_(expected_details) {}

  virtual ~HasDetailsMatcher() {}

  virtual bool MatchAndExplain(const JsEventDetails& details,
                               ::testing::MatchResultListener* listener) const {
    // No need to annotate listener since we already define PrintTo().
    return details.Get().Equals(&expected_details_.Get());
  }

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "has details " << expected_details_.ToString();
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't have details " << expected_details_.ToString();
  }

 private:
  const JsEventDetails expected_details_;

  DISALLOW_COPY_AND_ASSIGN(HasDetailsMatcher);
};

}  // namespace

::testing::Matcher<const JsEventDetails&> HasDetails(
    const JsEventDetails& expected_details) {
  return ::testing::MakeMatcher(new HasDetailsMatcher(expected_details));
}

::testing::Matcher<const JsEventDetails&> HasDetailsAsDictionary(
    const base::DictionaryValue& expected_details) {
  std::unique_ptr<base::DictionaryValue> expected_details_copy(
      expected_details.DeepCopy());
  return HasDetails(JsEventDetails(expected_details_copy.get()));
}

MockJsBackend::MockJsBackend() {}

MockJsBackend::~MockJsBackend() {}

WeakHandle<JsBackend> MockJsBackend::AsWeakHandle() {
  return MakeWeakHandle(AsWeakPtr());
}

MockJsController::MockJsController() {}

MockJsController::~MockJsController() {}

MockJsEventHandler::MockJsEventHandler() {}

WeakHandle<JsEventHandler> MockJsEventHandler::AsWeakHandle() {
  return MakeWeakHandle(AsWeakPtr());
}

MockJsEventHandler::~MockJsEventHandler() {}

}  // namespace syncer
