/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/color_chooser_ui_controller.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/forms/color_chooser_client.h"
#include "third_party/blink/renderer/platform/graphics/color.h"

namespace blink {

ColorChooserUIController::ColorChooserUIController(
    LocalFrame* frame,
    blink::ColorChooserClient* client)
    : client_(client), frame_(frame), binding_(this) {}

ColorChooserUIController::~ColorChooserUIController() {}

void ColorChooserUIController::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(client_);
  ColorChooser::Trace(visitor);
}

void ColorChooserUIController::Dispose() {
  binding_.Close();
}

void ColorChooserUIController::OpenUI() {
  OpenColorChooser();
}

void ColorChooserUIController::SetSelectedColor(const Color& color) {
  // Color can be set via JS before mojo OpenColorChooser completes.
  if (chooser_)
    chooser_->SetSelectedColor(color.Rgb());
}

void ColorChooserUIController::EndChooser() {
  chooser_.reset();
  client_->DidEndChooser();
}

AXObject* ColorChooserUIController::RootAXObject() {
  return nullptr;
}

void ColorChooserUIController::DidChooseColor(uint32_t color) {
  client_->DidChooseColor(color);
}

void ColorChooserUIController::OpenColorChooser() {
  DCHECK(!chooser_);
  frame_->GetInterfaceProvider().GetInterface(&color_chooser_factory_);
  mojom::blink::ColorChooserClientPtr mojo_client;
  binding_.Bind(mojo::MakeRequest(&mojo_client));
  binding_.set_connection_error_handler(WTF::Bind(
      &ColorChooserUIController::EndChooser, WrapWeakPersistent(this)));
  color_chooser_factory_->OpenColorChooser(
      mojo::MakeRequest(&chooser_), std::move(mojo_client),
      client_->CurrentColor().Rgb(), client_->Suggestions());
}

}  // namespace blink
