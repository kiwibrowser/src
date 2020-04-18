// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_WEB_CONTENTS_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_WEB_CONTENTS_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "chrome/browser/ui/app_list/search/answer_card/answer_card_contents.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

class Profile;

namespace views {
class RemoteViewProvider;
class WebView;
}

namespace app_list {

// Web source of contents for AnswerCardSearchProvider.
class AnswerCardWebContents : public AnswerCardContents,
                              public content::WebContentsDelegate,
                              public content::WebContentsObserver {
 public:
  explicit AnswerCardWebContents(Profile* profile);
  ~AnswerCardWebContents() override;

  // AnswerCardContents overrides:
  void LoadURL(const GURL& url) override;
  const base::UnguessableToken& GetToken() const override;
  gfx::Size GetPreferredSize() const override;

  // content::WebContentsDelegate overrides:
  void ResizeDueToAutoResize(content::WebContents* web_contents,
                             const gfx::Size& new_size) override;
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  bool HandleContextMenu(const content::ContextMenuParams& params) override;

  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidStopLoading() override;
  void DidGetUserInteraction(const blink::WebInputEvent::Type type) override;
  void RenderViewCreated(content::RenderViewHost* host) override;
  void RenderViewDeleted(content::RenderViewHost* host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;

 private:
  void AttachToHost(content::RenderWidgetHost* host);
  void DetachFromHost();

  void OnGotEmbedTokenAndNotify(const base::UnguessableToken& token);

  // Web contents managed by this class.
  const std::unique_ptr<content::WebContents> web_contents_;

  // Web view for the web contents managed by this class.
  // Note this is only used for classic ash.
  std::unique_ptr<views::WebView> web_view_;

  // Current widget host.
  content::RenderWidgetHost* host_ = nullptr;

  Profile* const profile_;  // Unowned

  // Token to embed the web contents. On classic ash, it is a token registered
  // with AnswerCardContentsRegistry. On mash, it is the embedding token for
  // the native window of |web_contents_|.
  base::UnguessableToken token_;

  // Preferred size of web contents.
  gfx::Size preferred_size_;

  // Helper to prepare the native view of |web_contents_| to be embedded under
  // mash.
  std::unique_ptr<views::RemoteViewProvider> remote_view_provider_;

  base::WeakPtrFactory<AnswerCardWebContents> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AnswerCardWebContents);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_WEB_CONTENTS_H_
