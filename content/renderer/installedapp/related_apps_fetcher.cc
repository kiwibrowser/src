// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/installedapp/related_apps_fetcher.h"

#include "base/bind.h"
#include "third_party/blink/public/common/manifest/manifest.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom.h"
#include "third_party/blink/public/mojom/manifest/manifest_manager.mojom.h"
#include "third_party/blink/public/platform/modules/installedapp/web_related_application.h"
#include "third_party/blink/public/platform/web_string.h"

namespace content {

RelatedAppsFetcher::RelatedAppsFetcher(
    blink::mojom::ManifestManager* manifest_manager)
    : manifest_manager_(manifest_manager) {}

RelatedAppsFetcher::~RelatedAppsFetcher() {}

void RelatedAppsFetcher::GetManifestRelatedApplications(
    std::unique_ptr<blink::WebCallbacks<
        const blink::WebVector<blink::WebRelatedApplication>&,
        void>> callbacks) {
  manifest_manager_->RequestManifest(
      base::BindOnce(&RelatedAppsFetcher::OnGetManifestForRelatedApplications,
                     base::Unretained(this), std::move(callbacks)));
}

void RelatedAppsFetcher::OnGetManifestForRelatedApplications(
    std::unique_ptr<blink::WebCallbacks<
        const blink::WebVector<blink::WebRelatedApplication>&,
        void>> callbacks,
    const GURL& /*url*/,
    const blink::Manifest& manifest) {
  std::vector<blink::WebRelatedApplication> related_apps;
  for (const auto& relatedApplication : manifest.related_applications) {
    blink::WebRelatedApplication webRelatedApplication;
    webRelatedApplication.platform =
        blink::WebString::FromUTF16(relatedApplication.platform);
    webRelatedApplication.id =
        blink::WebString::FromUTF16(relatedApplication.id);
    if (!relatedApplication.url.is_empty()) {
      webRelatedApplication.url =
          blink::WebString::FromUTF8(relatedApplication.url.spec());
    }
    related_apps.push_back(std::move(webRelatedApplication));
  }
  callbacks->OnSuccess(std::move(related_apps));
}

}  // namespace content
