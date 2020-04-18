// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/answer_card/answer_card_result.h"

#include "base/unguessable_token.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/search/answer_card/answer_card_contents.h"
#include "chrome/browser/ui/app_list/search/search_util.h"

namespace app_list {

AnswerCardResult::AnswerCardResult(Profile* profile,
                                   AppListControllerDelegate* list_controller,
                                   const std::string& result_url,
                                   const std::string& stripped_result_url,
                                   const base::string16& result_title,
                                   AnswerCardContents* contents)
    : profile_(profile),
      list_controller_(list_controller),
      contents_(contents) {
  DCHECK(!stripped_result_url.empty());
  SetDisplayType(ash::SearchResultDisplayType::kCard);
  SetResultType(ash::SearchResultType::kAnswerCard);
  set_id(result_url);
  set_comparable_id(stripped_result_url);
  set_relevance(1);
  SetAnswerCardContentsToken(contents ? contents->GetToken()
                                      : base::UnguessableToken());
  SetAnswerCardSize(contents ? contents->GetPreferredSize() : gfx::Size());
  SetTitle(result_title);

  if (contents)
    contents->RegisterResult(this);
}

AnswerCardResult::~AnswerCardResult() {
  if (contents_)
    contents_->UnregisterResult(this);
}

void AnswerCardResult::OnContentsDestroying() {
  contents_ = nullptr;
}

void AnswerCardResult::Open(int event_flags) {
  list_controller_->OpenURL(profile_, GURL(id()), ui::PAGE_TRANSITION_GENERATED,
                            ui::DispositionFromEventFlags(event_flags));
  RecordHistogram(ANSWER_CARD);
}

}  // namespace app_list
