// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_INPUT_METHOD_MUS_H_
#define UI_AURA_MUS_INPUT_METHOD_MUS_H_

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/ime/ime.mojom.h"
#include "ui/aura/aura_export.h"
#include "ui/base/ime/input_method_base.h"

namespace ui {
namespace mojom {
enum class EventResult;
}
}

namespace aura {

class InputMethodMusTestApi;
class TextInputClientImpl;
class Window;

class AURA_EXPORT InputMethodMus : public ui::InputMethodBase {
 public:
  using EventResultCallback = base::OnceCallback<void(ui::mojom::EventResult)>;

  InputMethodMus(ui::internal::InputMethodDelegate* delegate, Window* window);
  ~InputMethodMus() override;

  void Init(service_manager::Connector* connector);
  ui::EventDispatchDetails DispatchKeyEvent(ui::KeyEvent* event,
                                            EventResultCallback ack_callback)
      WARN_UNUSED_RESULT;

  // Overridden from ui::InputMethod:
  void OnFocus() override;
  void OnBlur() override;
  ui::EventDispatchDetails DispatchKeyEvent(ui::KeyEvent* event) override;
  void OnTextInputTypeChanged(const ui::TextInputClient* client) override;
  void OnCaretBoundsChanged(const ui::TextInputClient* client) override;
  void CancelComposition(const ui::TextInputClient* client) override;
  void OnInputLocaleChanged() override;
  bool IsCandidatePopupOpen() const override;

 private:
  friend class InputMethodMusTestApi;
  friend TextInputClientImpl;

  // Called from DispatchKeyEvent() to call to the InputMethod.
  ui::EventDispatchDetails SendKeyEventToInputMethod(
      const ui::KeyEvent& event,
      EventResultCallback ack_callback) WARN_UNUSED_RESULT;

  // Overridden from ui::InputMethodBase:
  void OnDidChangeFocusedClient(ui::TextInputClient* focused_before,
                                ui::TextInputClient* focused) override;

  void UpdateTextInputType();

  // Runs all pending callbacks with UNHANDLED. This is called during shutdown,
  // or any time |input_method_ptr_| is reset to ensure we don't leave mus
  // waiting for an ack.
  void AckPendingCallbacksUnhandled();

  // Called when the server responds to our request to process an event.
  void ProcessKeyEventCallback(
      const ui::KeyEvent& event,
      bool handled);

  // The toplevel window which is not owned by this class. This may be null
  // for tests.
  Window* window_;

  // May be null in tests.
  ui::mojom::IMEDriverPtr ime_driver_;
  ui::mojom::InputMethodPtr input_method_ptr_;
  // Typically this is the same as |input_method_ptr_|, but it may be mocked
  // in tests.
  ui::mojom::InputMethod* input_method_ = nullptr;
  std::unique_ptr<TextInputClientImpl> text_input_client_;

  // Callbacks supplied to DispatchKeyEvent() are added here while awaiting
  // the response from the server. These are removed when the response is
  // received (ProcessKeyEventCallback()).
  base::circular_deque<EventResultCallback> pending_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodMus);
};

}  // namespace aura

#endif  // UI_AURA_MUS_INPUT_METHOD_MUS_H_
