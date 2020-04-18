// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_PROFILE_CONTEXT_H_
#define CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_PROFILE_CONTEXT_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;
class SubresourceFilterContentSettingsManager;

// This class holds profile scoped context for subresource filtering.
class SubresourceFilterProfileContext : public KeyedService {
 public:
  explicit SubresourceFilterProfileContext(Profile* profile);
  ~SubresourceFilterProfileContext() override;

  SubresourceFilterContentSettingsManager* settings_manager() {
    return settings_manager_.get();
  }

 private:
  // KeyedService:
  void Shutdown() override;

  std::unique_ptr<SubresourceFilterContentSettingsManager> settings_manager_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterProfileContext);
};

#endif  // CHROME_BROWSER_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_PROFILE_CONTEXT_H_
