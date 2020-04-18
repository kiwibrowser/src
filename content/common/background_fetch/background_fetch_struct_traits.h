// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_STRUCT_TRAITS_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_STRUCT_TRAITS_H_

#include <string>
#include <vector>

#include "content/common/background_fetch/background_fetch_types.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom.h"

namespace content {
namespace mojom {
class BackgroundFetchSettledFetchDataView;
}
}

namespace mojo {

template <>
struct CONTENT_EXPORT StructTraits<blink::mojom::BackgroundFetchOptionsDataView,
                                   content::BackgroundFetchOptions> {
  static const std::vector<content::IconDefinition>& icons(
      const content::BackgroundFetchOptions& options) {
    return options.icons;
  }
  static const std::string& title(
      const content::BackgroundFetchOptions& options) {
    return options.title;
  }
  static uint64_t download_total(
      const content::BackgroundFetchOptions& options) {
    return options.download_total;
  }

  static bool Read(blink::mojom::BackgroundFetchOptionsDataView data,
                   content::BackgroundFetchOptions* options);
};

template <>
struct CONTENT_EXPORT
    StructTraits<blink::mojom::BackgroundFetchRegistrationDataView,
                 content::BackgroundFetchRegistration> {
  static const std::string& developer_id(
      const content::BackgroundFetchRegistration& registration) {
    return registration.developer_id;
  }
  static const std::string& unique_id(
      const content::BackgroundFetchRegistration& registration) {
    return registration.unique_id;
  }
  static uint64_t upload_total(
      const content::BackgroundFetchRegistration& registration) {
    return registration.upload_total;
  }
  static uint64_t uploaded(
      const content::BackgroundFetchRegistration& registration) {
    return registration.uploaded;
  }
  static uint64_t download_total(
      const content::BackgroundFetchRegistration& registration) {
    return registration.download_total;
  }
  static uint64_t downloaded(
      const content::BackgroundFetchRegistration& registration) {
    return registration.downloaded;
  }

  static bool Read(blink::mojom::BackgroundFetchRegistrationDataView data,
                   content::BackgroundFetchRegistration* registration);
};

template <>
struct CONTENT_EXPORT
    StructTraits<content::mojom::BackgroundFetchSettledFetchDataView,
                 content::BackgroundFetchSettledFetch> {
  static const content::ServiceWorkerFetchRequest& request(
      const content::BackgroundFetchSettledFetch& fetch) {
    return fetch.request;
  }
  static const content::ServiceWorkerResponse& response(
      const content::BackgroundFetchSettledFetch& fetch) {
    return fetch.response;
  }

  static bool Read(content::mojom::BackgroundFetchSettledFetchDataView data,
                   content::BackgroundFetchSettledFetch* definition);
};

template <>
struct CONTENT_EXPORT StructTraits<blink::mojom::IconDefinitionDataView,
                                   content::IconDefinition> {
  static const std::string& src(const content::IconDefinition& definition) {
    return definition.src;
  }
  static const std::string& sizes(const content::IconDefinition& definition) {
    return definition.sizes;
  }
  static const std::string& type(const content::IconDefinition& definition) {
    return definition.type;
  }

  static bool Read(blink::mojom::IconDefinitionDataView data,
                   content::IconDefinition* definition);
};

}  // namespace mojo

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_STRUCT_TRAITS_H_
