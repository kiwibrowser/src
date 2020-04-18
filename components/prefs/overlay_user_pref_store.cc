// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/prefs/overlay_user_pref_store.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "components/prefs/in_memory_pref_store.h"

// Allows us to monitor two pref stores and tell updates from them apart. It
// essentially mimics a Callback for the Observer interface (e.g. it allows
// binding additional arguments).
class OverlayUserPrefStore::ObserverAdapter : public PrefStore::Observer {
 public:
  ObserverAdapter(bool overlay, OverlayUserPrefStore* parent)
      : overlay_(overlay), parent_(parent) {}

  // Methods of PrefStore::Observer.
  void OnPrefValueChanged(const std::string& key) override {
    parent_->OnPrefValueChanged(overlay_, key);
  }
  void OnInitializationCompleted(bool succeeded) override {
    parent_->OnInitializationCompleted(overlay_, succeeded);
  }

 private:
  // Is the update for the overlay?
  const bool overlay_;
  OverlayUserPrefStore* const parent_;
};

OverlayUserPrefStore::OverlayUserPrefStore(PersistentPrefStore* underlay)
    : OverlayUserPrefStore(new InMemoryPrefStore(), underlay) {}

OverlayUserPrefStore::OverlayUserPrefStore(PersistentPrefStore* overlay,
                                           PersistentPrefStore* underlay)
    : overlay_observer_(
          std::make_unique<OverlayUserPrefStore::ObserverAdapter>(true, this)),
      underlay_observer_(
          std::make_unique<OverlayUserPrefStore::ObserverAdapter>(false, this)),
      overlay_(overlay),
      underlay_(underlay) {
  DCHECK(overlay->IsInitializationComplete());
  overlay_->AddObserver(overlay_observer_.get());
  underlay_->AddObserver(underlay_observer_.get());
}

bool OverlayUserPrefStore::IsSetInOverlay(const std::string& key) const {
  return overlay_->GetValue(key, nullptr);
}

void OverlayUserPrefStore::AddObserver(PrefStore::Observer* observer) {
  observers_.AddObserver(observer);
}

void OverlayUserPrefStore::RemoveObserver(PrefStore::Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool OverlayUserPrefStore::HasObservers() const {
  return observers_.might_have_observers();
}

bool OverlayUserPrefStore::IsInitializationComplete() const {
  return underlay_->IsInitializationComplete() &&
         overlay_->IsInitializationComplete();
}

bool OverlayUserPrefStore::GetValue(const std::string& key,
                                    const base::Value** result) const {
  // If the |key| shall NOT be stored in the overlay store, there must not
  // be an entry.
  DCHECK(ShallBeStoredInOverlay(key) || !overlay_->GetValue(key, nullptr));

  if (overlay_->GetValue(key, result))
    return true;
  return underlay_->GetValue(key, result);
}

std::unique_ptr<base::DictionaryValue> OverlayUserPrefStore::GetValues() const {
  auto values = underlay_->GetValues();
  auto overlay_values = overlay_->GetValues();
  for (const auto& key : overlay_names_set_) {
    std::unique_ptr<base::Value> out_value;
    overlay_values->Remove(key, &out_value);
    if (out_value) {
      values->Set(key, std::move(out_value));
    }
  }
  return values;
}

bool OverlayUserPrefStore::GetMutableValue(const std::string& key,
                                           base::Value** result) {
  if (!ShallBeStoredInOverlay(key))
    return underlay_->GetMutableValue(key, result);

  written_overlay_names_.insert(key);
  if (overlay_->GetMutableValue(key, result))
    return true;

  // Try to create copy of underlay if the overlay does not contain a value.
  base::Value* underlay_value = nullptr;
  if (!underlay_->GetMutableValue(key, &underlay_value))
    return false;

  *result = underlay_value->DeepCopy();
  overlay_->SetValue(key, base::WrapUnique(*result),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  return true;
}

void OverlayUserPrefStore::SetValue(const std::string& key,
                                    std::unique_ptr<base::Value> value,
                                    uint32_t flags) {
  if (!ShallBeStoredInOverlay(key)) {
    underlay_->SetValue(key, std::move(value), flags);
    return;
  }

  written_overlay_names_.insert(key);
  overlay_->SetValue(key, std::move(value), flags);
}

void OverlayUserPrefStore::SetValueSilently(const std::string& key,
                                            std::unique_ptr<base::Value> value,
                                            uint32_t flags) {
  if (!ShallBeStoredInOverlay(key)) {
    underlay_->SetValueSilently(key, std::move(value), flags);
    return;
  }

  written_overlay_names_.insert(key);
  overlay_->SetValueSilently(key, std::move(value), flags);
}

void OverlayUserPrefStore::RemoveValue(const std::string& key, uint32_t flags) {
  if (!ShallBeStoredInOverlay(key)) {
    underlay_->RemoveValue(key, flags);
    return;
  }

  written_overlay_names_.insert(key);
  overlay_->RemoveValue(key, flags);
}

bool OverlayUserPrefStore::ReadOnly() const {
  return false;
}

PersistentPrefStore::PrefReadError OverlayUserPrefStore::GetReadError() const {
  return PersistentPrefStore::PREF_READ_ERROR_NONE;
}

PersistentPrefStore::PrefReadError OverlayUserPrefStore::ReadPrefs() {
  // We do not read intentionally.
  OnInitializationCompleted(/* overlay */ false, true);
  return PersistentPrefStore::PREF_READ_ERROR_NONE;
}

void OverlayUserPrefStore::ReadPrefsAsync(
    ReadErrorDelegate* error_delegate_raw) {
  std::unique_ptr<ReadErrorDelegate> error_delegate(error_delegate_raw);
  // We do not read intentionally.
  OnInitializationCompleted(/* overlay */ false, true);
}

void OverlayUserPrefStore::CommitPendingWrite(base::OnceClosure done_callback) {
  underlay_->CommitPendingWrite(std::move(done_callback));
  // We do not write our content intentionally.
}

void OverlayUserPrefStore::SchedulePendingLossyWrites() {
  underlay_->SchedulePendingLossyWrites();
}

void OverlayUserPrefStore::ReportValueChanged(const std::string& key,
                                              uint32_t flags) {
  for (PrefStore::Observer& observer : observers_)
    observer.OnPrefValueChanged(key);
}

void OverlayUserPrefStore::RegisterOverlayPref(const std::string& key) {
  DCHECK(!key.empty()) << "Key is empty";
  DCHECK(overlay_names_set_.find(key) == overlay_names_set_.end())
      << "Key already registered";
  overlay_names_set_.insert(key);
}

void OverlayUserPrefStore::ClearMutableValues() {
  for (const auto& key : written_overlay_names_) {
    overlay_->RemoveValue(key, WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  }
}

void OverlayUserPrefStore::OnStoreDeletionFromDisk() {
  underlay_->OnStoreDeletionFromDisk();
}

OverlayUserPrefStore::~OverlayUserPrefStore() {
  overlay_->RemoveObserver(overlay_observer_.get());
  underlay_->RemoveObserver(underlay_observer_.get());
}

void OverlayUserPrefStore::OnPrefValueChanged(bool overlay,
                                              const std::string& key) {
  if (overlay) {
    ReportValueChanged(key, DEFAULT_PREF_WRITE_FLAGS);
  } else {
    if (!overlay_->GetValue(key, nullptr))
      ReportValueChanged(key, DEFAULT_PREF_WRITE_FLAGS);
  }
}

void OverlayUserPrefStore::OnInitializationCompleted(bool overlay,
                                                     bool succeeded) {
  if (!IsInitializationComplete())
    return;
  for (PrefStore::Observer& observer : observers_)
    observer.OnInitializationCompleted(succeeded);
}

bool OverlayUserPrefStore::ShallBeStoredInOverlay(
    const std::string& key) const {
  return overlay_names_set_.find(key) != overlay_names_set_.end();
}
