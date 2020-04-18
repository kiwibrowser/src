// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/net/connectivity_checker.h"

#include "chromecast/net/connectivity_checker_impl.h"

namespace chromecast {

ConnectivityChecker::ConnectivityChecker()
    : connectivity_observer_list_(
          new base::ObserverListThreadSafe<ConnectivityObserver>()) {
}

ConnectivityChecker::~ConnectivityChecker() {
}

void ConnectivityChecker::AddConnectivityObserver(
    ConnectivityObserver* observer) {
  connectivity_observer_list_->AddObserver(observer);
}

void ConnectivityChecker::RemoveConnectivityObserver(
    ConnectivityObserver* observer) {
  connectivity_observer_list_->RemoveObserver(observer);
}

void ConnectivityChecker::Notify(bool connected) {
  DCHECK(connectivity_observer_list_.get());
  connectivity_observer_list_->Notify(
      FROM_HERE, &ConnectivityObserver::OnConnectivityChanged, connected);
}

// static
scoped_refptr<ConnectivityChecker> ConnectivityChecker::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    net::URLRequestContextGetter* url_request_context_getter) {
  return base::MakeRefCounted<ConnectivityCheckerImpl>(
      task_runner, url_request_context_getter);
}

}  // namespace chromecast
