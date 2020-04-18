// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_THEME_SOURCE_H_
#define CHROME_BROWSER_UI_WEBUI_THEME_SOURCE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/url_data_source.h"

class Profile;

class ThemeSource : public content::URLDataSource {
 public:
  explicit ThemeSource(Profile* profile);
  ~ThemeSource() override;

  // content::URLDataSource implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;
  scoped_refptr<base::SingleThreadTaskRunner> TaskRunnerForRequestPath(
      const std::string& path) const override;
  bool AllowCaching() const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;

 private:
  // Fetches and sends the theme bitmap.
  void SendThemeBitmap(const content::URLDataSource::GotDataCallback& callback,
                       int resource_id,
                       float scale);

  // Used in place of SendThemeBitmap when the desired scale is larger than
  // what the resource bundle supports.  This can rescale the provided bitmap up
  // to the desired size.
  void SendThemeImage(const content::URLDataSource::GotDataCallback& callback,
                      int resource_id,
                      float scale);

  // The profile this object was initialized with.
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(ThemeSource);
};

#endif  // CHROME_BROWSER_UI_WEBUI_THEME_SOURCE_H_
