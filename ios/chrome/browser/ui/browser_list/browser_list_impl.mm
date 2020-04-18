// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser_list_impl.h"

#include <memory>

#include "base/logging.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_list_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BrowserListImpl::BrowserListImpl(ios::ChromeBrowserState* browser_state,
                                 std::unique_ptr<WebStateListDelegate> delegate)
    : browser_state_(browser_state), delegate_(std::move(delegate)) {
  DCHECK(browser_state_);
  DCHECK(delegate_);
}

BrowserListImpl::~BrowserListImpl() = default;

int BrowserListImpl::GetCount() const {
  return static_cast<int>(browsers_.size());
}

int BrowserListImpl::ContainsIndex(int index) const {
  return 0 <= index && index < GetCount();
}

Browser* BrowserListImpl::GetBrowserAtIndex(int index) const {
  DCHECK(ContainsIndex(index));
  return browsers_[index].get();
}

int BrowserListImpl::GetIndexOfBrowser(const Browser* browser) const {
  for (int index = 0; index < GetCount(); ++index) {
    if (browsers_[index].get() == browser)
      return index;
  }
  return kInvalidIndex;
}

Browser* BrowserListImpl::CreateNewBrowser() {
  DCHECK_LT(browsers_.size(), static_cast<size_t>(INT_MAX));
  browsers_.push_back(
      std::make_unique<Browser>(browser_state_, delegate_.get()));
  Browser* browser_created = browsers_.back().get();
  for (BrowserListObserver& observer : observers_)
    observer.OnBrowserCreated(this, browser_created);
  return browser_created;
}

void BrowserListImpl::CloseBrowserAtIndex(int index) {
  Browser* browser_removed = GetBrowserAtIndex(index);
  for (BrowserListObserver& observer : observers_)
    observer.OnBrowserRemoved(this, browser_removed);
  browsers_.erase(browsers_.begin() + index);
}

void BrowserListImpl::AddObserver(BrowserListObserver* observer) {
  observers_.AddObserver(observer);
}

void BrowserListImpl::RemoveObserver(BrowserListObserver* observer) {
  observers_.RemoveObserver(observer);
}
