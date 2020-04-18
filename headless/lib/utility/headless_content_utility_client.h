// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_HEADLESS_CONTENT_UTILITY_CLIENT_H_
#define HEADLESS_LIB_HEADLESS_CONTENT_UTILITY_CLIENT_H_

#include <string>

#include "content/public/utility/content_utility_client.h"

namespace headless {

class HeadlessContentUtilityClient : public content::ContentUtilityClient {
 public:
  explicit HeadlessContentUtilityClient(const std::string& user_agent);
  ~HeadlessContentUtilityClient() override;

  // content::ContentUtilityClient:
  void RegisterServices(StaticServiceMap* services) override;

 private:
  const std::string user_agent_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessContentUtilityClient);
};

}  // namespace headless

#endif  // HEADLESS_LIB_HEADLESS_CONTENT_UTILITY_CLIENT_H_
