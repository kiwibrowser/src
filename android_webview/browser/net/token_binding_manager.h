// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_TOKEN_BINDING_MANAGER_H_
#define ANDROID_WEBVIEW_BROWSER_NET_TOKEN_BINDING_MANAGER_H_

#include "base/callback.h"
#include "base/lazy_instance.h"
#include "crypto/ec_private_key.h"

namespace android_webview {

/**
 * Manages the token binding protocol. Token binding can be enabled/disabled
 * for each browser context, however all webviews share the same browser
 * context. Webview does not expose the browser context to the embedder app.
 * This complicates enabling the protocol since it has to be done very early,
 * while creating the url request context.
 *
 * As a solution TokenBindingManager is set as a singleton (just like its very
 * similar friend cookiemanager). In future, if webview provides multiple
 * browser contexts,  we may move the ownership to AwBrowserContext after
 * deciding how to expose the relationship between "context" and the "context
 * specific settings" such as this.
 */
class TokenBindingManager {
 public:
  static TokenBindingManager* GetInstance();

  // Should be called before creating the URLRequestContext (starting
  // chromium). Otherwise it has no effect.
  void enable_token_binding() { enabled_ = true; }
  bool is_enabled() { return enabled_; }

  // The callback to indicate the status for fetching the key. The first
  // parameter is one of a network error code (see the underlying protocol
  // implementation for more), and the second parameter is the key. The
  // key is owned by the callback and be destroyed at the end of the call.
  using KeyReadyCallback =
      base::OnceCallback<void(int, crypto::ECPrivateKey* key)>;
  using DeletionCompleteCallback = base::OnceCallback<void(void)>;

  // Retrieve (or create if not exist) the key for the given host. The callback
  // is called asynchonously to indicate the status for the key creation and
  // to provide the key.
  // This method simply pushes the work to the right thread so see the
  // underlying implementation for more details.
  void GetKey(const std::string& host, KeyReadyCallback callback);

  // Deletes the key for the given host.  This method simply pushes the work
  // to the right thread so see the underlying implementation for more details.
  void DeleteKey(const std::string& host, DeletionCompleteCallback callback);

  // Deletes all the keys. This method simply pushes the work
  // to the right thread so see the underlying implementation for more details.
  void DeleteAllKeys(DeletionCompleteCallback callback);

 private:
  friend struct base::LazyInstanceTraitsBase<TokenBindingManager>;

  TokenBindingManager();
  ~TokenBindingManager() {}

  bool enabled_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_TOKEN_BINDING_MANAGER_H_
