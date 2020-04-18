// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
#define SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_

#include <string>

#include "base/component_export.h"
#include "base/containers/linked_list.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom.h"
#include "url/gurl.h"

namespace net {
class CookieStore;
}  // namespace net

namespace network {

// RestrictedCookieManager implementation.
//
// Instances of this class must be created and used on the I/O thread. Instances
// are created by CreateMojoService() and are bound to the lifetimes of the
// mojo connections that they serve, via mojo::StrongBinding.
class COMPONENT_EXPORT(NETWORK_SERVICE) RestrictedCookieManager
    : public mojom::RestrictedCookieManager {
 public:
  RestrictedCookieManager(net::CookieStore* cookie_store,
                          int render_process_id,
                          int render_frame_id);
  ~RestrictedCookieManager() override;

  // Implements CookieStore.getAll() in the Async Cookies API.
  //
  // This method is also used as the backend for CookieStore.get() and
  // CookieStore.has().
  void GetAllForUrl(const GURL& url,
                    const GURL& site_for_cookies,
                    mojom::CookieManagerGetOptionsPtr options,
                    GetAllForUrlCallback callback) override;

  void SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          const GURL& url,
                          const GURL& site_for_cookies,
                          SetCanonicalCookieCallback callback) override;

  void AddChangeListener(
      const GURL& url,
      const GURL& site_for_cookies,
      network::mojom::CookieChangeListenerPtr listener) override;

 private:
  // The state associated with a CookieChangeListener.
  class Listener;

  // Feeds a net::CookieList to a GetAllForUrl() callback.
  void CookieListToGetAllForUrlCallback(
      const GURL& url,
      const GURL& site_for_cookies,
      mojom::CookieManagerGetOptionsPtr options,
      GetAllForUrlCallback callback,
      const net::CookieList& cookie_list);

  // Called when the Mojo pipe associated with a listener is closed.
  void RemoveChangeListener(Listener* listener);

  net::CookieStore* const cookie_store_;
  const int render_process_id_;
  const int render_frame_id_;

  base::LinkedList<Listener> listeners_;

  base::WeakPtrFactory<RestrictedCookieManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RestrictedCookieManager);
};

}  // namespace network

#endif  // SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
