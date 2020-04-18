// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/spell_check_client.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "content/shell/test_runner/mock_grammar_check.h"
#include "content/shell/test_runner/test_runner.h"
#include "content/shell/test_runner/web_test_delegate.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_text_checking_completion.h"
#include "third_party/blink/public/web/web_text_checking_result.h"

namespace test_runner {

SpellCheckClient::SpellCheckClient(TestRunner* test_runner)
    : last_requested_text_checking_completion_(nullptr),
      test_runner_(test_runner),
      weak_factory_(this) {
  DCHECK(test_runner);
}

SpellCheckClient::~SpellCheckClient() {}

void SpellCheckClient::SetDelegate(WebTestDelegate* delegate) {
  delegate_ = delegate;
}

void SpellCheckClient::SetEnabled(bool enabled) {
  enabled_ = enabled;
}

void SpellCheckClient::Reset() {
  enabled_ = false;
  resolved_callback_.Reset();
}

// blink::WebSpellCheckClient
bool SpellCheckClient::IsSpellCheckingEnabled() const {
  // Ensure that the spellchecker code paths are always tested in layout tests.
  return true;
}

void SpellCheckClient::CheckSpelling(
    const blink::WebString& text,
    int& misspelled_offset,
    int& misspelled_length,
    blink::WebVector<blink::WebString>* optional_suggestions) {
  if (!enabled_) {
    misspelled_offset = 0;
    misspelled_length = 0;
    return;
  }

  // Check the spelling of the given text.
  spell_check_.SpellCheckWord(text, &misspelled_offset, &misspelled_length);
}

void SpellCheckClient::RequestCheckingOfText(
    const blink::WebString& text,
    blink::WebTextCheckingCompletion* completion) {
  if (!enabled_ || text.IsEmpty()) {
    if (completion) {
      completion->DidCancelCheckingText();
      RequestResolved();
    }
    return;
  }

  if (last_requested_text_checking_completion_) {
    last_requested_text_checking_completion_->DidCancelCheckingText();
    last_requested_text_checking_completion_ = nullptr;
    RequestResolved();
  }

  last_requested_text_checking_completion_ = completion;
  last_requested_text_check_string_ = text;
  if (spell_check_.HasInCache(text)) {
    FinishLastTextCheck();
  } else {
    delegate_->PostTask(base::BindOnce(&SpellCheckClient::FinishLastTextCheck,
                                       weak_factory_.GetWeakPtr()));
  }
}

void SpellCheckClient::CancelAllPendingRequests() {
  if (!last_requested_text_checking_completion_)
    return;
  last_requested_text_checking_completion_->DidCancelCheckingText();
  last_requested_text_checking_completion_ = nullptr;
}

void SpellCheckClient::FinishLastTextCheck() {
  if (!last_requested_text_checking_completion_)
    return;
  std::vector<blink::WebTextCheckingResult> results;
  int offset = 0;
  if (!spell_check_.IsMultiWordMisspelling(last_requested_text_check_string_,
                                           &results)) {
    base::string16 text = last_requested_text_check_string_.Utf16();
    while (text.length()) {
      int misspelled_position = 0;
      int misspelled_length = 0;
      spell_check_.SpellCheckWord(blink::WebString::FromUTF16(text),
                                  &misspelled_position, &misspelled_length);
      if (!misspelled_length)
        break;
      blink::WebVector<blink::WebString> suggestions;
      spell_check_.FillSuggestionList(
          blink::WebString::FromUTF16(
              text.substr(misspelled_position, misspelled_length)),
          &suggestions);
      results.push_back(blink::WebTextCheckingResult(
          blink::kWebTextDecorationTypeSpelling, offset + misspelled_position,
          misspelled_length, suggestions));
      text = text.substr(misspelled_position + misspelled_length);
      offset += misspelled_position + misspelled_length;
    }
    MockGrammarCheck::CheckGrammarOfString(last_requested_text_check_string_,
                                           &results);
  }
  last_requested_text_checking_completion_->DidFinishCheckingText(results);
  last_requested_text_checking_completion_ = nullptr;
  RequestResolved();

  if (test_runner_->shouldDumpSpellCheckCallbacks())
    delegate_->PrintMessage("SpellCheckEvent: FinishLastTextCheck\n");
}

void SpellCheckClient::SetSpellCheckResolvedCallback(
    v8::Local<v8::Function> callback) {
  resolved_callback_.Reset(blink::MainThreadIsolate(), callback);
}

void SpellCheckClient::RemoveSpellCheckResolvedCallback() {
  resolved_callback_.Reset();
}

void SpellCheckClient::RequestResolved() {
  if (resolved_callback_.IsEmpty())
    return;

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  blink::WebFrame* frame = test_runner_->mainFrame();
  if (!frame || frame->IsWebRemoteFrame())
    return;
  blink::WebLocalFrame* local_frame = frame->ToWebLocalFrame();

  v8::Local<v8::Context> context = local_frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  local_frame->CallFunctionEvenIfScriptDisabled(
      v8::Local<v8::Function>::New(isolate, resolved_callback_),
      context->Global(), 0, nullptr);
}

}  // namespace test_runner
