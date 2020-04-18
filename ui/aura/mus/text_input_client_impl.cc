// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/text_input_client_impl.h"

#include "base/strings/utf_string_conversions.h"
#include "services/ui/public/interfaces/ime/ime.mojom.h"
#include "ui/aura/mus/input_method_mus.h"
#include "ui/base/ime/text_input_client.h"

namespace aura {

TextInputClientImpl::TextInputClientImpl(
    ui::TextInputClient* text_input_client,
    ui::internal::InputMethodDelegate* delegate)
    : text_input_client_(text_input_client),
      binding_(this),
      delegate_(delegate) {}

TextInputClientImpl::~TextInputClientImpl() {}

ui::mojom::TextInputClientPtr TextInputClientImpl::CreateInterfacePtrAndBind() {
  ui::mojom::TextInputClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void TextInputClientImpl::SetCompositionText(
    const ui::CompositionText& composition) {
  text_input_client_->SetCompositionText(composition);
}

void TextInputClientImpl::ConfirmCompositionText() {
  text_input_client_->ConfirmCompositionText();
}

void TextInputClientImpl::ClearCompositionText() {
  text_input_client_->ClearCompositionText();
}

void TextInputClientImpl::InsertText(const base::string16& text) {
  text_input_client_->InsertText(text);
}

void TextInputClientImpl::InsertChar(std::unique_ptr<ui::Event> event) {
  DCHECK(event->IsKeyEvent());
  text_input_client_->InsertChar(*event->AsKeyEvent());
}

void TextInputClientImpl::DispatchKeyEventPostIME(
    std::unique_ptr<ui::Event> event,
    DispatchKeyEventPostIMECallback callback) {
  if (delegate_) {
    delegate_->DispatchKeyEventPostIME(event->AsKeyEvent());
    if (callback && !callback.is_null())
      std::move(callback).Run(event->stopped_propagation());
  }
}

}  // namespace aura
