// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LANGUAGE_CHROME_LANGUAGE_DETECTION_TAB_HELPER_H_
#define CHROME_BROWSER_LANGUAGE_CHROME_LANGUAGE_DETECTION_TAB_HELPER_H_

#include "base/feature_list.h"
#include "base/macros.h"
#include "components/translate/content/common/translate.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace content {
class WebContents;
}  // namespace content

namespace language {
class UrlLanguageHistogram;
}  // namespace language

// Dispatches language detection messages from render frames to language and
// translate components.
class ChromeLanguageDetectionTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ChromeLanguageDetectionTabHelper>,
      public translate::mojom::ContentTranslateDriver {
 public:
  ~ChromeLanguageDetectionTabHelper() override;

  static void BindContentTranslateDriver(
      translate::mojom::ContentTranslateDriverRequest request,
      content::RenderFrameHost* render_frame_host);

  // translate::mojom::ContentTranslateDriver implementation.
  void RegisterPage(translate::mojom::PagePtr page,
                    const translate::LanguageDetectionDetails& details,
                    bool page_needs_translation) override;

 private:
  explicit ChromeLanguageDetectionTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<ChromeLanguageDetectionTabHelper>;

  // Histogram to be notified about detected language of every page visited. Not
  // owned here.
  language::UrlLanguageHistogram* const language_histogram_;

  // ChromeLanguageDetectionTabHelper is a singleton per web contents, serving
  // for multiple render frames.
  mojo::BindingSet<translate::mojom::ContentTranslateDriver> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ChromeLanguageDetectionTabHelper);
};

#endif  // CHROME_BROWSER_LANGUAGE_CHROME_LANGUAGE_DETECTION_TAB_HELPER_H_
