// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/public/cpp/pref_store_client.h"

#include <utility>

namespace prefs {

PrefStoreClient::PrefStoreClient(mojom::PrefStoreConnectionPtr connection) {
  Init(base::DictionaryValue::From(
           base::Value::ToUniquePtrValue(std::move(connection->initial_prefs))),
       connection->is_initialized, std::move(connection->observer));
}

PrefStoreClient::~PrefStoreClient() = default;

}  // namespace prefs
