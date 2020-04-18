// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_EXTENSIONS_CAST_EXTENSIONS_API_CLIENT_H_
#define CHROMECAST_BROWSER_EXTENSIONS_CAST_EXTENSIONS_API_CLIENT_H_

#include <memory>

#include "extensions/browser/api/extensions_api_client.h"

namespace extensions {

class MessagingDelegate;

class CastExtensionsAPIClient : public ExtensionsAPIClient {
 public:
  CastExtensionsAPIClient();
  ~CastExtensionsAPIClient() override;

  // ExtensionsAPIClient implementation.
  void AttachWebContentsHelpers(
      content::WebContents* web_contents) const override;
  MessagingDelegate* GetMessagingDelegate() override;

 private:
  std::unique_ptr<MessagingDelegate> messaging_delegate_;

  DISALLOW_COPY_AND_ASSIGN(CastExtensionsAPIClient);
};

}  // namespace extensions

#endif  // CHROMECAST_BROWSER_EXTENSIONS_CAST_EXTENSIONS_API_CLIENT_H_
