// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_OAUTH_TOKEN_GETTER_PROXY_H_
#define REMOTING_CLIENT_OAUTH_TOKEN_GETTER_PROXY_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "remoting/base/oauth_token_getter.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

// Takes an instance of |OAuthTokenGetter| and runs (and deletes) it on the
// |task_runner| thread. The proxy can be called from any thread.
class OAuthTokenGetterProxy : public OAuthTokenGetter {
 public:
  OAuthTokenGetterProxy(
      base::WeakPtr<OAuthTokenGetter> token_getter,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~OAuthTokenGetterProxy() override;

  // OAuthTokenGetter overrides.
  void CallWithToken(const TokenCallback& on_access_token) override;
  void InvalidateCache() override;

 private:
  base::WeakPtr<OAuthTokenGetter> token_getter_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(OAuthTokenGetterProxy);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_OAUTH_TOKEN_GETTER_PROXY_H_
