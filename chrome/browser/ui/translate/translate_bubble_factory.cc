// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/translate/translate_bubble_factory.h"

#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"

namespace {

ShowTranslateBubbleResult ShowDefault(
    BrowserWindow* window,
    content::WebContents* web_contents,
    translate::TranslateStep step,
    translate::TranslateErrors::Type error_type) {
  // |window| might be null when testing.
  if (!window)
    return ShowTranslateBubbleResult::BROWSER_WINDOW_NOT_VALID;
  return window->ShowTranslateBubble(web_contents, step, error_type, false);
}

}  // namespace

TranslateBubbleFactory::~TranslateBubbleFactory() {
}

// static
ShowTranslateBubbleResult TranslateBubbleFactory::Show(
    BrowserWindow* window,
    content::WebContents* web_contents,
    translate::TranslateStep step,
    translate::TranslateErrors::Type error_type) {
  if (current_factory_) {
    return current_factory_->ShowImplementation(window, web_contents, step,
                                                error_type);
  }

  return ShowDefault(window, web_contents, step, error_type);
}

// static
void TranslateBubbleFactory::SetFactory(TranslateBubbleFactory* factory) {
  current_factory_ = factory;
}

// static
TranslateBubbleFactory* TranslateBubbleFactory::current_factory_ = NULL;
