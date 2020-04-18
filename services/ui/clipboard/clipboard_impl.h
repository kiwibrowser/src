// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_CLIPBOARD_CLIPBOARD_IMPL_H_
#define SERVICES_UI_CLIPBOARD_CLIPBOARD_IMPL_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/ui/public/interfaces/clipboard.mojom.h"

namespace ui {
namespace clipboard {

// Stub clipboard implementation.
//
// Eventually, we'll actually want to interact with the system clipboard, but
// that's hard today because the system clipboard is asynchronous (on X11), the
// ui::Clipboard interface is synchronous (which is what we'd use), mojo is
// asynchronous across processes, and the WebClipboard interface is synchronous
// (which is at least tractable).
class ClipboardImpl : public mojom::Clipboard {
 public:
  // mojom::Clipboard exposes three possible clipboards.
  static const int kNumClipboards = 2;

  ClipboardImpl();
  ~ClipboardImpl() override;

  void AddBinding(mojom::ClipboardRequest request);

  // mojom::Clipboard implementation.
  void GetSequenceNumber(mojom::Clipboard::Type clipboard_type,
                         GetSequenceNumberCallback callback) override;
  void GetAvailableMimeTypes(mojom::Clipboard::Type clipboard_types,
                             GetAvailableMimeTypesCallback callback) override;
  void ReadClipboardData(mojom::Clipboard::Type clipboard_type,
                         const std::string& mime_type,
                         ReadClipboardDataCallback callback) override;
  void WriteClipboardData(
      mojom::Clipboard::Type clipboard_type,
      const base::Optional<base::flat_map<std::string, std::vector<uint8_t>>>&
          data,
      WriteClipboardDataCallback callback) override;

 private:
  // Internal struct which stores the current state of the clipboard.
  class ClipboardData;

  // The current clipboard state. This is what is read from.
  std::unique_ptr<ClipboardData> clipboard_state_[kNumClipboards];
  mojo::BindingSet<mojom::Clipboard> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ClipboardImpl);
};

}  // namespace clipboard
}  // namespace ui

#endif  // SERVICES_UI_CLIPBOARD_CLIPBOARD_IMPL_H_
