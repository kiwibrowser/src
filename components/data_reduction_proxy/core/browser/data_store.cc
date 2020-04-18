// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_store.h"

namespace data_reduction_proxy {

DataStore::DataStore() {
}

DataStore::~DataStore() {
}

void DataStore::InitializeOnDBThread() {
}

DataStore::Status DataStore::Get(base::StringPiece key, std::string* value) {
  return DataStore::Status::NOT_FOUND;
}

DataStore::Status DataStore::Put(
    const std::map<std::string, std::string>& map) {
  return DataStore::Status::OK;
}

DataStore::Status DataStore::Delete(base::StringPiece key) {
  return DataStore::Status::OK;
}

DataStore::Status DataStore::RecreateDB() {
  return DataStore::Status::OK;
}

}  // namespace data_reduction_proxy
