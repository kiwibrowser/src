// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_CONTENTS_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_CONTENTS_H_

#include <string>

#include "base/observer_list.h"
#include "base/unguessable_token.h"

namespace gfx {
class Size;
}  // namespace gfx

class GURL;

namespace app_list {

class AnswerCardResult;

// Abstract source of contents for AnswerCardSearchProvider.
class AnswerCardContents {
 public:
  // Delegate handling contents-related events.
  class Delegate {
   public:
    Delegate() {}

    // Events that the delegate processes. They have same meaning as same-named
    // events in WebContentsDelegate and WebContentsObserver, however,
    // unnecessary parameters are omitted.
    virtual void UpdatePreferredSize(const AnswerCardContents* source) = 0;
    virtual void DidFinishNavigation(const AnswerCardContents* source,
                                     const GURL& url,
                                     bool has_error,
                                     bool has_answer_card,
                                     const std::string& result_title,
                                     const std::string& issued_query) = 0;

    // Invoked when |source| is ready to be shown.
    virtual void OnContentsReady(const AnswerCardContents* source) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  AnswerCardContents();
  virtual ~AnswerCardContents();

  // Loads contents from |url|.
  virtual void LoadURL(const GURL& url) = 0;

  // Returns the token associated with the contents.
  virtual const base::UnguessableToken& GetToken() const = 0;

  // Returns the preferred contents size.
  virtual gfx::Size GetPreferredSize() const = 0;

  // Sets the delegate to process contents-related events.
  void SetDelegate(Delegate* delegate);

  // Registers a result that will be notified of input events for the view.
  void RegisterResult(AnswerCardResult* result);

  // Unregisters a result.
  void UnregisterResult(AnswerCardResult* result);

 protected:
  Delegate* delegate() const { return delegate_; }

 private:
  // Results receiving input events.
  base::ObserverList<AnswerCardResult> results_;
  // Unowned delegate that handles content-related events.
  Delegate* delegate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AnswerCardContents);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_CONTENTS_H_
