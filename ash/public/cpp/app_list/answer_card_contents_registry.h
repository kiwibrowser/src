// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_ANSWER_CARD_CONTENTS_REGISTRY_H_
#define ASH_PUBLIC_CPP_APP_LIST_ANSWER_CARD_CONTENTS_REGISTRY_H_

#include <map>

#include "ash/public/cpp/ash_public_export.h"
#include "base/macros.h"
#include "base/unguessable_token.h"

namespace views {
class View;
}

namespace app_list {

// Helper class to provide a token to answer card view map when app list UI code
// runs in the same process of answer card provider (classic ash, or mash before
// UI code is migrated to ash). In such environment, chrome creates and owns an
// instance of this class. AnswerCardContents registers answer card contents
// with it and get a token back. App list UI code gets the token and use it to
// look up the view to show. On mash after the UI code migration, this class
// will NOT be used at all. App list UI code would use the token to embed the
// answer card content directly.
class ASH_PUBLIC_EXPORT AnswerCardContentsRegistry {
 public:
  AnswerCardContentsRegistry();
  ~AnswerCardContentsRegistry();

  static AnswerCardContentsRegistry* Get();

  // Register content with a View.
  base::UnguessableToken Register(views::View* contents_view);

  // Unregister and release the associated resources.
  void Unregister(const base::UnguessableToken& token);

  // Get the view for the given token. Return nullptr for unknown token.
  views::View* GetView(const base::UnguessableToken& token);

 private:
  std::map<base::UnguessableToken, views::View*> contents_map_;

  DISALLOW_COPY_AND_ASSIGN(AnswerCardContentsRegistry);
};

}  // namespace app_list

#endif  // ASH_PUBLIC_CPP_APP_LIST_ANSWER_CARD_CONTENTS_REGISTRY_H_
