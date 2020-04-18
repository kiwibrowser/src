// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/app_list/app_list_types.h"

namespace ash {

const char kOemFolderId[] = "ddb1da55-d478-4243-8642-56d3041f0263";

////////////////////////////////////////////////////////////////////////////////
// SearchResultTag:

SearchResultTag::SearchResultTag() = default;

SearchResultTag::SearchResultTag(int styles, uint32_t start, uint32_t end)
    : styles(styles), range(start, end) {}

////////////////////////////////////////////////////////////////////////////////
// SearchResultAction:

SearchResultAction::SearchResultAction() {}

SearchResultAction::SearchResultAction(const gfx::ImageSkia& base_image,
                                       const gfx::ImageSkia& hover_image,
                                       const gfx::ImageSkia& pressed_image,
                                       const base::string16& tooltip_text)
    : base_image(base_image),
      hover_image(hover_image),
      pressed_image(pressed_image),
      tooltip_text(tooltip_text) {}

SearchResultAction::SearchResultAction(const base::string16& label_text,
                                       const base::string16& tooltip_text)
    : tooltip_text(tooltip_text), label_text(label_text) {}

SearchResultAction::SearchResultAction(const SearchResultAction& other) =
    default;

SearchResultAction::~SearchResultAction() = default;

}  // namespace ash
