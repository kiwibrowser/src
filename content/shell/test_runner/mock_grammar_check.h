// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_MOCK_GRAMMAR_CHECK_H_
#define CONTENT_SHELL_TEST_RUNNER_MOCK_GRAMMAR_CHECK_H_

#include <vector>

namespace blink {
class WebString;
struct WebTextCheckingResult;
}

namespace test_runner {

// A mock implementation of a grammar-checker used for WebKit tests. This class
// only implements the minimal functionarities required by WebKit tests, i.e.
// this class just compares the given string with known grammar mistakes in
// webkit tests and adds grammar markers on them. Even though this is sufficent
// for webkit tests, this class is not suitable for any other usages.
class MockGrammarCheck {
 public:
  static bool CheckGrammarOfString(const blink::WebString&,
                                   std::vector<blink::WebTextCheckingResult>*);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_MOCK_GRAMMAR_CHECK_H_
