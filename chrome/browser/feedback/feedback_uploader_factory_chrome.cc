// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feedback/feedback_uploader_factory_chrome.h"

#include "base/memory/singleton.h"
#include "chrome/browser/feedback/feedback_uploader_chrome.h"

namespace feedback {

// static
FeedbackUploaderFactoryChrome* FeedbackUploaderFactoryChrome::GetInstance() {
  return base::Singleton<FeedbackUploaderFactoryChrome>::get();
}

// static
FeedbackUploaderChrome* FeedbackUploaderFactoryChrome::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<FeedbackUploaderChrome*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

FeedbackUploaderFactoryChrome::FeedbackUploaderFactoryChrome()
    : FeedbackUploaderFactory("feedback::FeedbackUploaderChrome") {}

FeedbackUploaderFactoryChrome::~FeedbackUploaderFactoryChrome() = default;

KeyedService* FeedbackUploaderFactoryChrome::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new FeedbackUploaderChrome(context, task_runner_);
}

}  // namespace feedback
