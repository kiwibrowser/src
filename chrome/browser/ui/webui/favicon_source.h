// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_FAVICON_SOURCE_H_
#define CHROME_BROWSER_UI_WEBUI_FAVICON_SOURCE_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon/core/favicon_service.h"
#include "content/public/browser/url_data_source.h"
#include "ui/gfx/favicon_size.h"

class Profile;

// FaviconSource is the gateway between network-level chrome:
// requests for favicons and the history backend that serves these.
//
// Format:
//   chrome://favicon/size&scalefactor/iconurl/url
// Some parameters are optional as described below. However, the order of the
// parameters is not interchangeable.
//
// Parameter:
//  'url'               Required
//    Specifies the page URL of the requested favicon. If the 'iconurl'
//    parameter is specified, the URL refers to the URL of the favicon image
//    instead.
//  'size&scalefactor'  Optional
//    Values: ['size/aa@bx/']
//      Specifies the requested favicon's size in DIP (aa) and the requested
//      favicon's scale factor. (b).
//      The supported requested DIP sizes are: 16x16, 32x32 and 64x64.
//      If the parameter is unspecified, the requested favicon's size defaults
//      to 16 and the requested scale factor defaults to 1x.
//      Example: chrome://favicon/size/16@2x/https://www.google.com/
//  'iconurl'           Optional
//    Values: ['iconurl']
//    'iconurl': Specifies that the url parameter refers to the URL of
//    the favicon image as opposed to the URL of the page that the favicon is
//    on.
//    Example: chrome://favicon/iconurl/https://www.google.com/favicon.ico
class FaviconSource : public content::URLDataSource {
 public:
  // |type| is the type of icon this FaviconSource will provide.
  explicit FaviconSource(Profile* profile);

  ~FaviconSource() override;

  // content::URLDataSource implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string&) const override;
  bool AllowCaching() const override;
  bool ShouldReplaceExistingSource() const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;

 protected:
  struct IconRequest {
    IconRequest();
    IconRequest(const content::URLDataSource::GotDataCallback& cb,
                const GURL& path,
                int size,
                float scale);
    IconRequest(const IconRequest& other);
    ~IconRequest();

    content::URLDataSource::GotDataCallback callback;
    GURL request_path;
    int size_in_dip;
    float device_scale_factor;
  };

  // Called when the favicon data is missing to perform additional checks to
  // locate the resource.
  // |request| contains information for the failed request.
  // Returns true if the missing resource is found.
  virtual bool HandleMissingResource(const IconRequest& request);

  Profile* profile_;

 private:
  // Defines the allowed pixel sizes for requested favicons.
  enum IconSize {
    SIZE_16,
    SIZE_32,
    SIZE_64,
    NUM_SIZES
  };

  // Called when favicon data is available from the history backend.
  void OnFaviconDataAvailable(
      const IconRequest& request,
      const favicon_base::FaviconRawBitmapResult& bitmap_result);

  // Sends the 16x16 DIP 1x default favicon.
  void SendDefaultResponse(
      const content::URLDataSource::GotDataCallback& callback);

  // Sends the default favicon.
  void SendDefaultResponse(const IconRequest& request);

  base::CancelableTaskTracker cancelable_task_tracker_;

  DISALLOW_COPY_AND_ASSIGN(FaviconSource);
};

#endif  // CHROME_BROWSER_UI_WEBUI_FAVICON_SOURCE_H_
