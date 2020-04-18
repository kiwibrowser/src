// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_registration_notifier.h"

#include "base/bind.h"
#include "base/stl_util.h"

namespace content {

BackgroundFetchRegistrationNotifier::BackgroundFetchRegistrationNotifier()
    : weak_factory_(this) {}

BackgroundFetchRegistrationNotifier::~BackgroundFetchRegistrationNotifier() {}

void BackgroundFetchRegistrationNotifier::AddObserver(
    const std::string& unique_id,
    blink::mojom::BackgroundFetchRegistrationObserverPtr observer) {
  // Observe connection errors, which occur when the JavaScript object or the
  // renderer hosting them goes away. (For example through navigation.) The
  // observer gets freed together with |this|, thus the Unretained is safe.
  // TODO(crbug.com/777764): This doesn't actually work for the cases where
  // the closing of the binding is non-exceptional.
  observer.set_connection_error_handler(
      base::BindOnce(&BackgroundFetchRegistrationNotifier::OnConnectionError,
                     base::Unretained(this), unique_id, observer.get()));

  observers_.emplace(unique_id, std::move(observer));
}

void BackgroundFetchRegistrationNotifier::Notify(const std::string& unique_id,
                                                 uint64_t download_total,
                                                 uint64_t downloaded) {
  auto range = observers_.equal_range(unique_id);
  for (auto it = range.first; it != range.second; ++it) {
    // TODO(crbug.com/774054): Uploads are not yet supported.
    it->second->OnProgress(0 /* upload_total */, 0 /* uploaded */,
                           download_total, downloaded);
  }
}

void BackgroundFetchRegistrationNotifier::AddGarbageCollectionCallback(
    const std::string& unique_id,
    base::OnceClosure callback) {
  if (!observers_.count(unique_id))
    std::move(callback).Run();
  else
    garbage_collection_callbacks_.emplace(unique_id, std::move(callback));
}

void BackgroundFetchRegistrationNotifier::OnConnectionError(
    const std::string& unique_id,
    blink::mojom::BackgroundFetchRegistrationObserver* observer) {
  DCHECK_GE(observers_.count(unique_id), 1u);
  base::EraseIf(observers_,
                [observer](const auto& unique_id_observer_ptr_pair) {
                  return unique_id_observer_ptr_pair.second.get() == observer;
                });

  auto callback_iter = garbage_collection_callbacks_.find(unique_id);
  if (callback_iter != garbage_collection_callbacks_.end() &&
      !observers_.count(unique_id)) {
    std::move(callback_iter->second).Run();
    garbage_collection_callbacks_.erase(callback_iter);
  }
}

}  // namespace content
