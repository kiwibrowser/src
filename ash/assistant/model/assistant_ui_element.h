// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_MODEL_ASSISTANT_UI_ELEMENT_H_
#define ASH_ASSISTANT_MODEL_ASSISTANT_UI_ELEMENT_H_

#include <string>

#include "base/macros.h"

namespace ash {

// AssistantUiElementType ------------------------------------------------------

// Defines possible types of Assistant UI elements.
enum class AssistantUiElementType {
  kCard,  // See AssistantCardElement.
  kText,  // See AssistantTextElement.
};

// AssistantUiElement ----------------------------------------------------------

// Base class for a UI element that will be rendered inside of Assistant UI.
class AssistantUiElement {
 public:
  virtual ~AssistantUiElement() = default;

  AssistantUiElementType GetType() const { return type_; }

 protected:
  explicit AssistantUiElement(AssistantUiElementType type) : type_(type) {}

 private:
  const AssistantUiElementType type_;

  DISALLOW_COPY_AND_ASSIGN(AssistantUiElement);
};

// AssistantCardElement --------------------------------------------------------

// An Assistant UI element that will be rendered as an HTML card.
class AssistantCardElement : public AssistantUiElement {
 public:
  explicit AssistantCardElement(const std::string& html)
      : AssistantUiElement(AssistantUiElementType::kCard), html_(html) {}

  ~AssistantCardElement() override = default;

  const std::string& GetHtml() const { return html_; }

 private:
  const std::string html_;

  DISALLOW_COPY_AND_ASSIGN(AssistantCardElement);
};

// AssistantTextElement --------------------------------------------------------

// An Assistant UI element that will be rendered as text.
class AssistantTextElement : public AssistantUiElement {
 public:
  explicit AssistantTextElement(const std::string& text)
      : AssistantUiElement(AssistantUiElementType::kText), text_(text) {}

  ~AssistantTextElement() override = default;

  const std::string& GetText() const { return text_; }

 private:
  const std::string text_;

  DISALLOW_COPY_AND_ASSIGN(AssistantTextElement);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_MODEL_ASSISTANT_UI_ELEMENT_H_
