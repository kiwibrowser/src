// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/tab_loader_delegate.h"

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/resource_coordinator/tab_manager_features.h"
#include "components/variations/variations_associated_data.h"
#include "net/base/network_change_notifier.h"

namespace {

// The timeout time after which the next tab gets loaded if the previous tab did
// not finish loading yet. The used value is half of the median value of all
// ChromeOS devices loading the 25 most common web pages. Half is chosen since
// the loading time is a mix of server response and data bandwidth.
static const int kInitialDelayTimerMS = 1500;

// Similar to the above constant, but the timeout that is afforded to the
// visible tab only. Having this be a longer value ensures the visible time has
// more time during which it is the only one loading, decreasing the time to
// first paint and interactivity of the foreground tab.
static const int kFirstTabLoadTimeoutMS = 60000;

class TabLoaderDelegateImpl
    : public TabLoaderDelegate,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  explicit TabLoaderDelegateImpl(TabLoaderCallback* callback);
  ~TabLoaderDelegateImpl() override;

  // TabLoaderDelegate:
  base::TimeDelta GetFirstTabLoadingTimeout() const override {
    return resource_coordinator::GetTabLoadTimeout(first_timeout_);
  }

  // TabLoaderDelegate:
  base::TimeDelta GetTimeoutBeforeLoadingNextTab() const override {
    return resource_coordinator::GetTabLoadTimeout(timeout_);
  }

  // net::NetworkChangeNotifier::NetworkChangeObserver implementation:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

 private:
  // The function to call when the connection type changes.
  TabLoaderCallback* callback_;

  // The timeouts to use in tab loading.
  base::TimeDelta first_timeout_;
  base::TimeDelta timeout_;

  DISALLOW_COPY_AND_ASSIGN(TabLoaderDelegateImpl);
};

TabLoaderDelegateImpl::TabLoaderDelegateImpl(TabLoaderCallback* callback)
    : callback_(callback) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  if (net::NetworkChangeNotifier::IsOffline()) {
    // When we are off-line we do not allow loading of tabs, since each of
    // these tabs would start loading simultaneously when going online.
    // TODO(skuhne): Once we get a higher level resource control logic which
    // distributes network access, we can remove this.
    callback->SetTabLoadingEnabled(false);
  }

  first_timeout_ = base::TimeDelta::FromMilliseconds(kFirstTabLoadTimeoutMS);
  timeout_ = base::TimeDelta::FromMilliseconds(kInitialDelayTimerMS);
}

TabLoaderDelegateImpl::~TabLoaderDelegateImpl() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void TabLoaderDelegateImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  callback_->SetTabLoadingEnabled(
      type != net::NetworkChangeNotifier::CONNECTION_NONE);
}

}  // namespace

// static
std::unique_ptr<TabLoaderDelegate> TabLoaderDelegate::Create(
    TabLoaderCallback* callback) {
  return std::unique_ptr<TabLoaderDelegate>(
      new TabLoaderDelegateImpl(callback));
}
