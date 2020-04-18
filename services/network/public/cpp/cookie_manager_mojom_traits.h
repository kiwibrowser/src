// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_COOKIE_MANAGER_MOJOM_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_COOKIE_MANAGER_MOJOM_TRAITS_H_

#include "ipc/ipc_message_utils.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_options.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace mojo {

template <>
struct EnumTraits<network::mojom::CookiePriority, net::CookiePriority> {
  static network::mojom::CookiePriority ToMojom(net::CookiePriority input);
  static bool FromMojom(network::mojom::CookiePriority input,
                        net::CookiePriority* output);
};

template <>
struct EnumTraits<network::mojom::CookieSameSite, net::CookieSameSite> {
  static network::mojom::CookieSameSite ToMojom(net::CookieSameSite input);
  static bool FromMojom(network::mojom::CookieSameSite input,
                        net::CookieSameSite* output);
};

template <>
struct EnumTraits<network::mojom::CookieSameSiteFilter,
                  net::CookieOptions::SameSiteCookieMode> {
  static network::mojom::CookieSameSiteFilter ToMojom(
      net::CookieOptions::SameSiteCookieMode input);

  static bool FromMojom(network::mojom::CookieSameSiteFilter input,
                        net::CookieOptions::SameSiteCookieMode* output);
};

template <>
struct StructTraits<network::mojom::CookieOptionsDataView, net::CookieOptions> {
  static bool exclude_httponly(const net::CookieOptions& o) {
    return o.exclude_httponly();
  }
  static net::CookieOptions::SameSiteCookieMode cookie_same_site_filter(
      const net::CookieOptions& o) {
    return o.same_site_cookie_mode();
  }
  static bool update_access_time(const net::CookieOptions& o) {
    return o.update_access_time();
  }
  static base::Optional<base::Time> server_time(const net::CookieOptions& o) {
    if (!o.has_server_time())
      return base::nullopt;
    return base::Optional<base::Time>(o.server_time());
  }

  static bool Read(network::mojom::CookieOptionsDataView mojo_options,
                   net::CookieOptions* cookie_options);
};

template <>
struct StructTraits<network::mojom::CanonicalCookieDataView,
                    net::CanonicalCookie> {
  static const std::string& name(const net::CanonicalCookie& c) {
    return c.Name();
  }
  static const std::string& value(const net::CanonicalCookie& c) {
    return c.Value();
  }
  static const std::string& domain(const net::CanonicalCookie& c) {
    return c.Domain();
  }
  static const std::string& path(const net::CanonicalCookie& c) {
    return c.Path();
  }
  static base::Optional<base::Time> creation(const net::CanonicalCookie& c) {
    return c.CreationDate().is_null()
               ? base::Optional<base::Time>()
               : base::Optional<base::Time>(c.CreationDate());
  }
  static base::Optional<base::Time> expiry(const net::CanonicalCookie& c) {
    return c.ExpiryDate().is_null()
               ? base::Optional<base::Time>()
               : base::Optional<base::Time>(c.ExpiryDate());
  }
  static base::Optional<base::Time> last_access(const net::CanonicalCookie& c) {
    return c.LastAccessDate().is_null()
               ? base::Optional<base::Time>()
               : base::Optional<base::Time>(c.LastAccessDate());
  }
  static bool secure(const net::CanonicalCookie& c) { return c.IsSecure(); }
  static bool httponly(const net::CanonicalCookie& c) { return c.IsHttpOnly(); }
  static net::CookieSameSite site_restrictions(const net::CanonicalCookie& c) {
    return c.SameSite();
  }
  static net::CookiePriority priority(const net::CanonicalCookie& c) {
    return c.Priority();
  }

  static bool Read(network::mojom::CanonicalCookieDataView cookie,
                   net::CanonicalCookie* out);
};

}  // namespace mojo

#endif  // SERVICES_NETWORK_PUBLIC_CPP_COOKIE_MANAGER_MOJOM_TRAITS_H_
