// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_NET_URL_TRANSLATOR_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_NET_URL_TRANSLATOR_H_

#include <string>

namespace syncer {

// Contains the declaration of a few helper functions used for generating sync
// URLs.

// This method appends the query string to the sync server path.
std::string MakeSyncServerPath(const std::string& path,
                               const std::string& query_string);

std::string MakeSyncQueryString(const std::string& client_id);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_NET_URL_TRANSLATOR_H_
