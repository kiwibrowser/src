// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/testapp/test_keyboard_delegate.h"

#include <memory>

#include "base/strings/utf_string_conversion_utils.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/dom/dom_code.h"

namespace vr {

namespace {

constexpr gfx::SizeF kKeyboardSize = {1.2f, 0.37f};
constexpr gfx::Vector2dF kKeyboardTranslate = {0, -0.1};

}  // namespace

TestKeyboardDelegate::TestKeyboardDelegate()
    : renderer_(std::make_unique<TestKeyboardRenderer>()) {}

TestKeyboardDelegate::~TestKeyboardDelegate() {}

void TestKeyboardDelegate::ShowKeyboard() {
  editing_ = true;
}

void TestKeyboardDelegate::HideKeyboard() {
  editing_ = false;
}

void TestKeyboardDelegate::SetTransform(const gfx::Transform& transform) {
  world_space_transform_ = transform;
}

bool TestKeyboardDelegate::HitTest(const gfx::Point3F& ray_origin,
                                   const gfx::Point3F& ray_target,
                                   gfx::Point3F* hit_position) {
  // TODO(ymalik): Add hittesting logic for the keyboard.
  return false;
}

void TestKeyboardDelegate::Draw(const CameraModel& model) {
  if (!editing_)
    return;

  // We try to simulate what the gvr keyboard does here by scaling and
  // translating the keyboard on top of the provided transform.
  gfx::Transform world_space_transform = world_space_transform_;
  world_space_transform.Scale(kKeyboardSize.width(), kKeyboardSize.height());
  world_space_transform.Translate(kKeyboardTranslate);
  renderer_->Draw(model, world_space_transform);
}

bool TestKeyboardDelegate::SupportsSelection() {
  return true;
}

void TestKeyboardDelegate::Initialize(vr::SkiaSurfaceProvider* provider,
                                      UiElementRenderer* renderer) {
  renderer_->Initialize(provider, renderer);
}

void TestKeyboardDelegate::UpdateInput(const vr::TextInputInfo& info) {
  input_info_ = info;
}

bool TestKeyboardDelegate::HandleInput(ui::Event* e) {
  DCHECK(ui_interface_);
  DCHECK(e->IsKeyEvent());
  if (!editing_)
    return false;

  TextInputInfo info(input_info_);

  auto* event = e->AsKeyEvent();
  switch (event->key_code()) {
    case ui::VKEY_RETURN:
      info.text.clear();
      info.selection_start = info.selection_end = 0;
      ui_interface_->OnInputCommitted(EditedText(info, input_info_));
      break;
    case ui::VKEY_BACK:
      if (info.selection_start != info.selection_end) {
        info.text.erase(info.selection_start,
                        info.selection_end - info.selection_start);
        info.selection_end = info.selection_start;
      } else if (!info.text.empty() && info.selection_start > 0) {
        info.text.erase(info.selection_start - 1, 1);
        info.selection_start--;
        info.selection_end--;
      }
      ui_interface_->OnInputEdited(EditedText(info, input_info_));
      break;
    default:
      if (info.selection_start != info.selection_end) {
        info.text.erase(info.selection_start,
                        info.selection_end - info.selection_start);
        info.selection_end = info.selection_start;
      }

      std::string character;
      base::WriteUnicodeCharacter(event->GetText(), &character);
      info.text =
          info.text.insert(info.selection_start, base::UTF8ToUTF16(character));
      info.selection_start++;
      info.selection_end++;
      ui_interface_->OnInputEdited(EditedText(info, input_info_));
      break;
  }
  return true;
}

}  // namespace vr
