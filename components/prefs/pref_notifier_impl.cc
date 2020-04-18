// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/prefs/pref_notifier_impl.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/prefs/pref_service.h"

PrefNotifierImpl::PrefNotifierImpl() : pref_service_(nullptr) {}

PrefNotifierImpl::PrefNotifierImpl(PrefService* service)
    : pref_service_(service) {
}

PrefNotifierImpl::~PrefNotifierImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Verify that there are no pref observers when we shut down.
  for (const auto& observer_list : pref_observers_) {
    if (observer_list.second->begin() != observer_list.second->end())
      LOG(WARNING) << "Pref observer found at shutdown.";
  }

  // Same for initialization observers.
  if (!init_observers_.empty())
    LOG(WARNING) << "Init observer found at shutdown.";

  pref_observers_.clear();
  init_observers_.clear();
}

void PrefNotifierImpl::AddPrefObserver(const std::string& path,
                                       PrefObserver* obs) {
  // Get the pref observer list associated with the path.
  PrefObserverList* observer_list = nullptr;
  auto observer_iterator = pref_observers_.find(path);
  if (observer_iterator == pref_observers_.end()) {
    observer_list = new PrefObserverList;
    pref_observers_[path] = base::WrapUnique(observer_list);
  } else {
    observer_list = observer_iterator->second.get();
  }

  // Add the pref observer. ObserverList will DCHECK if it already is
  // in the list.
  observer_list->AddObserver(obs);
}

void PrefNotifierImpl::RemovePrefObserver(const std::string& path,
                                          PrefObserver* obs) {
  DCHECK(thread_checker_.CalledOnValidThread());

  auto observer_iterator = pref_observers_.find(path);
  if (observer_iterator == pref_observers_.end()) {
    return;
  }

  PrefObserverList* observer_list = observer_iterator->second.get();
  observer_list->RemoveObserver(obs);
}

void PrefNotifierImpl::AddPrefObserverAllPrefs(PrefObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  all_prefs_pref_observers_.AddObserver(observer);
}

void PrefNotifierImpl::RemovePrefObserverAllPrefs(PrefObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  all_prefs_pref_observers_.RemoveObserver(observer);
}

void PrefNotifierImpl::AddInitObserver(base::OnceCallback<void(bool)> obs) {
  init_observers_.push_back(std::move(obs));
}

void PrefNotifierImpl::OnPreferenceChanged(const std::string& path) {
  FireObservers(path);
}

void PrefNotifierImpl::OnInitializationCompleted(bool succeeded) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // We must move init_observers_ to a local variable before we run
  // observers, or we can end up in this method re-entrantly before
  // clearing the observers list.
  PrefInitObserverList observers;
  std::swap(observers, init_observers_);

  for (auto& observer : observers)
    std::move(observer).Run(succeeded);
}

void PrefNotifierImpl::FireObservers(const std::string& path) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Only send notifications for registered preferences.
  if (!pref_service_->FindPreference(path))
    return;

  // Fire observers for any preference change.
  for (auto& observer : all_prefs_pref_observers_)
    observer.OnPreferenceChanged(pref_service_, path);

  auto observer_iterator = pref_observers_.find(path);
  if (observer_iterator == pref_observers_.end())
    return;

  for (PrefObserver& observer : *(observer_iterator->second))
    observer.OnPreferenceChanged(pref_service_, path);
}

void PrefNotifierImpl::SetPrefService(PrefService* pref_service) {
  DCHECK(pref_service_ == nullptr);
  pref_service_ = pref_service;
}
