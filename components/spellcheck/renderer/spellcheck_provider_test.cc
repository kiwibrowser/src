// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spellcheck/renderer/spellcheck_provider_test.h"

#include <memory>

#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "components/spellcheck/common/spellcheck.mojom.h"
#include "components/spellcheck/common/spellcheck_result.h"
#include "components/spellcheck/renderer/spellcheck.h"
#include "components/spellcheck/spellcheck_buildflags.h"

FakeTextCheckingCompletion::FakeTextCheckingCompletion()
    : completion_count_(0), cancellation_count_(0) {}

FakeTextCheckingCompletion::~FakeTextCheckingCompletion() {}

void FakeTextCheckingCompletion::DidFinishCheckingText(
    const blink::WebVector<blink::WebTextCheckingResult>& results) {
  ++completion_count_;
}

void FakeTextCheckingCompletion::DidCancelCheckingText() {
  ++completion_count_;
  ++cancellation_count_;
}

TestingSpellCheckProvider::TestingSpellCheckProvider(
    service_manager::LocalInterfaceProvider* embedder_provider)
    : SpellCheckProvider(nullptr,
                         new SpellCheck(nullptr, embedder_provider),
                         embedder_provider),
      binding_(this) {}

TestingSpellCheckProvider::TestingSpellCheckProvider(
    SpellCheck* spellcheck,
    service_manager::LocalInterfaceProvider* embedder_provider)
    : SpellCheckProvider(nullptr, spellcheck, embedder_provider),
      binding_(this) {}

TestingSpellCheckProvider::~TestingSpellCheckProvider() {
  binding_.Close();
  // dictionary_update_observer_ must be released before deleting spellcheck_.
  ResetDictionaryUpdateObserverForTesting();
  delete spellcheck_;
}

void TestingSpellCheckProvider::RequestTextChecking(
    const base::string16& text,
    blink::WebTextCheckingCompletion* completion) {
  if (!loop_ && !base::MessageLoopCurrent::Get())
    loop_ = std::make_unique<base::MessageLoop>();
  if (!binding_.is_bound()) {
    spellcheck::mojom::SpellCheckHostPtr host_proxy;
    binding_.Bind(mojo::MakeRequest(&host_proxy));
    SetSpellCheckHostForTesting(std::move(host_proxy));
  }
  SpellCheckProvider::RequestTextChecking(text, completion);
  base::RunLoop().RunUntilIdle();
}

void TestingSpellCheckProvider::RequestDictionary() {}

void TestingSpellCheckProvider::NotifyChecked(const base::string16& word,
                                              bool misspelled) {}

#if !BUILDFLAG(USE_BROWSER_SPELLCHECKER)
void TestingSpellCheckProvider::CallSpellingService(
    const base::string16& text,
    CallSpellingServiceCallback callback) {
  OnCallSpellingService(text);
  std::move(callback).Run(true, std::vector<SpellCheckResult>());
}

void TestingSpellCheckProvider::OnCallSpellingService(
    const base::string16& text) {
  ++spelling_service_call_count_;
  blink::WebTextCheckingCompletion* completion =
      text_check_completions_.Lookup(last_identifier_);
  if (!completion) {
    ResetResult();
    return;
  }
  text_.assign(text);
  text_check_completions_.Remove(last_identifier_);
  std::vector<blink::WebTextCheckingResult> results;
  results.push_back(
      blink::WebTextCheckingResult(blink::kWebTextDecorationTypeSpelling, 0, 5,
                                   std::vector<blink::WebString>({"hello"})));
  completion->DidFinishCheckingText(results);
  last_request_ = text;
  last_results_ = results;
}

void TestingSpellCheckProvider::ResetResult() {
  text_.clear();
}
#endif  // !BUILDFLAG(USE_BROWSER_SPELLCHECKER)

#if BUILDFLAG(USE_BROWSER_SPELLCHECKER)
void TestingSpellCheckProvider::RequestTextCheck(
    const base::string16& text,
    int,
    RequestTextCheckCallback callback) {
  text_check_requests_.push_back(std::make_pair(text, std::move(callback)));
}

void TestingSpellCheckProvider::ToggleSpellCheck(bool, bool) {
  NOTREACHED();
}

void TestingSpellCheckProvider::CheckSpelling(const base::string16&,
                                              int,
                                              CheckSpellingCallback) {
  NOTREACHED();
}

void TestingSpellCheckProvider::FillSuggestionList(const base::string16&,
                                                   FillSuggestionListCallback) {
  NOTREACHED();
}
#endif  // BUILDFLAG(USE_BROWSER_SPELLCHECKER)

void TestingSpellCheckProvider::SetLastResults(
    const base::string16 last_request,
    blink::WebVector<blink::WebTextCheckingResult>& last_results) {
  last_request_ = last_request;
  last_results_ = last_results;
}

bool TestingSpellCheckProvider::SatisfyRequestFromCache(
    const base::string16& text,
    blink::WebTextCheckingCompletion* completion) {
  return SpellCheckProvider::SatisfyRequestFromCache(text, completion);
}

SpellCheckProviderTest::SpellCheckProviderTest()
    : provider_(&embedder_provider_) {}
SpellCheckProviderTest::~SpellCheckProviderTest() {}
