// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/app_list/answer_card_contents_registry.h"

#include "base/logging.h"

namespace app_list {

namespace {

AnswerCardContentsRegistry* instance = nullptr;

}  // namespace

AnswerCardContentsRegistry::AnswerCardContentsRegistry() {
  DCHECK(!instance);
  instance = this;
}

AnswerCardContentsRegistry::~AnswerCardContentsRegistry() {
  DCHECK_EQ(this, instance);
  instance = nullptr;
}

// static
AnswerCardContentsRegistry* AnswerCardContentsRegistry::Get() {
  return instance;
}

base::UnguessableToken AnswerCardContentsRegistry::Register(
    views::View* contents_view) {
  const base::UnguessableToken token = base::UnguessableToken::Create();
  contents_map_[token] = contents_view;
  return token;
}

void AnswerCardContentsRegistry::Unregister(
    const base::UnguessableToken& token) {
  auto it = contents_map_.find(token);
  if (it == contents_map_.end())
    return;

  contents_map_.erase(it);
}

views::View* AnswerCardContentsRegistry::GetView(
    const base::UnguessableToken& token) {
  auto it = contents_map_.find(token);
  if (it == contents_map_.end())
    return nullptr;

  return it->second;
}

}  // namespace app_list
