// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_ONE_GOOGLE_BAR_ONE_GOOGLE_BAR_LOADER_IMPL_H_
#define CHROME_BROWSER_SEARCH_ONE_GOOGLE_BAR_ONE_GOOGLE_BAR_LOADER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_loader.h"

class GoogleURLTracker;

namespace base {
class Value;
}

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

struct OneGoogleBarData;

// TODO(treib): This class uses cookies for authentication. After "Dice" account
// consistency launches, we should switch to using OAuth2 instead.
// See https://crbug.com/751534.
class OneGoogleBarLoaderImpl : public OneGoogleBarLoader {
 public:
  // |api_url_override| can be either absolute, or relative to the Google base
  // URL.
  OneGoogleBarLoaderImpl(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      GoogleURLTracker* google_url_tracker,
      const std::string& application_locale,
      const base::Optional<std::string>& api_url_override,
      bool account_consistency_mirror_required);
  ~OneGoogleBarLoaderImpl() override;

  void Load(OneGoogleCallback callback) override;

  GURL GetLoadURLForTesting() const override;

 private:
  class AuthenticatedURLLoader;

  GURL GetApiUrl() const;

  void LoadDone(const network::SimpleURLLoader* simple_loader,
                std::unique_ptr<std::string> response_body);

  void JsonParsed(std::unique_ptr<base::Value> value);
  void JsonParseFailed(const std::string& message);

  void Respond(Status status, const base::Optional<OneGoogleBarData>& data);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  GoogleURLTracker* google_url_tracker_;
  const std::string application_locale_;
  const base::Optional<std::string> api_url_override_;
  const bool account_consistency_mirror_required_;

  std::vector<OneGoogleCallback> callbacks_;
  std::unique_ptr<AuthenticatedURLLoader> pending_request_;

  base::WeakPtrFactory<OneGoogleBarLoaderImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OneGoogleBarLoaderImpl);
};

#endif  // CHROME_BROWSER_SEARCH_ONE_GOOGLE_BAR_ONE_GOOGLE_BAR_LOADER_IMPL_H_
