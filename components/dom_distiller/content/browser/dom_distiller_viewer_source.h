// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DOM_DISTILLER_VIEWER_SOURCE_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DOM_DISTILLER_VIEWER_SOURCE_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/dom_distiller/content/browser/distiller_ui_handle.h"
#include "content/public/browser/url_data_source.h"

namespace dom_distiller {

class DomDistillerServiceInterface;
class DomDistillerViewerSourceTest;

// Serves HTML and resources for viewing distilled articles.
class DomDistillerViewerSource : public content::URLDataSource {
 public:
  DomDistillerViewerSource(DomDistillerServiceInterface* dom_distiller_service,
                           const std::string& scheme,
                           std::unique_ptr<DistillerUIHandle> ui_handle);
  ~DomDistillerViewerSource() override;

  class RequestViewerHandle;

  // Overridden from content::URLDataSource:
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;
  std::string GetContentSecurityPolicyStyleSrc() const override;
  std::string GetContentSecurityPolicyChildSrc() const override;

 private:
  friend class DomDistillerViewerSourceTest;

  // The scheme this URLDataSource is hosted under.
  std::string scheme_;

  // The service which contains all the functionality needed to interact with
  // the list of articles.
  DomDistillerServiceInterface* dom_distiller_service_;

  // An object for accessing chrome-specific UI controls including external
  // feedback and opening the distiller settings.
  std::unique_ptr<DistillerUIHandle> distiller_ui_handle_;

  DISALLOW_COPY_AND_ASSIGN(DomDistillerViewerSource);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DOM_DISTILLER_VIEWER_SOURCE_H_
