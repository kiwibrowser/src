// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/framebust_block_tab_helper.h"

#include "base/logging.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(FramebustBlockTabHelper);

FramebustBlockTabHelper::~FramebustBlockTabHelper() = default;

void FramebustBlockTabHelper::AddBlockedUrl(const GURL& blocked_url,
                                            ClickCallback click_callback) {
  blocked_urls_.push_back(blocked_url);
  callbacks_.push_back(std::move(click_callback));
  DCHECK_EQ(blocked_urls_.size(), callbacks_.size());

  for (Observer& observer : observers_) {
    observer.OnBlockedUrlAdded(blocked_url);
  }
}

bool FramebustBlockTabHelper::HasBlockedUrls() const {
  return !blocked_urls_.empty();
}

void FramebustBlockTabHelper::OnBlockedUrlClicked(size_t index) {
  size_t total_size = blocked_urls_.size();
  DCHECK_LT(index, total_size);
  const GURL& url = blocked_urls_[index];
  if (!callbacks_[index].is_null())
    std::move(callbacks_[index]).Run(url, index, total_size);
  web_contents_->OpenURL(content::OpenURLParams(
      url, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
      ui::PAGE_TRANSITION_LINK, false));
  blocked_urls_.clear();
  callbacks_.clear();
}

void FramebustBlockTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FramebustBlockTabHelper::RemoveObserver(const Observer* observer) {
  observers_.RemoveObserver(observer);
}

FramebustBlockTabHelper::FramebustBlockTabHelper(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}
