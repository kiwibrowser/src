// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INSTALLEDAPP_RELATED_APPS_FETCHER_H
#define CONTENT_RENDERER_INSTALLEDAPP_RELATED_APPS_FETCHER_H

#include <string>

#include "base/compiler_specific.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/modules/installedapp/web_related_apps_fetcher.h"

namespace blink {
struct Manifest;
namespace mojom {
class ManifestManager;
}
}  // namespace blink

namespace content {

class ManifestManager;

class CONTENT_EXPORT RelatedAppsFetcher : public blink::WebRelatedAppsFetcher {
 public:
  explicit RelatedAppsFetcher(blink::mojom::ManifestManager* manifest_manager);
  ~RelatedAppsFetcher() override;

  // blink::WebRelatedAppsFetcher overrides:
  void GetManifestRelatedApplications(
      std::unique_ptr<blink::WebCallbacks<
          const blink::WebVector<blink::WebRelatedApplication>&,
          void>> callbacks) override;

 private:
  // Callback for when the manifest has been fetched, triggered by a call to
  // getManifestRelatedApplications.
  void OnGetManifestForRelatedApplications(
      std::unique_ptr<blink::WebCallbacks<
          const blink::WebVector<blink::WebRelatedApplication>&,
          void>> callbacks,
      const GURL& url,
      const blink::Manifest& manifest);

  blink::mojom::ManifestManager* const manifest_manager_;

  DISALLOW_COPY_AND_ASSIGN(RelatedAppsFetcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INSTALLEDAPP_RELATED_APPS_FETCHER_H
