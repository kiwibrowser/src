// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/ime_bridge.h"

#include <map>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"

namespace ui {

static IMEBridge* g_ime_bridge = nullptr;

// An implementation of IMEBridge.
class IMEBridgeImpl : public IMEBridge {
 public:
#if defined(OS_CHROMEOS)
  IMEBridgeImpl()
      : input_context_handler_(nullptr),
        engine_handler_(nullptr),
        observer_(nullptr),
        current_input_context_(ui::TEXT_INPUT_TYPE_NONE,
                               ui::TEXT_INPUT_MODE_DEFAULT,
                               0,
                               ui::TextInputClient::FOCUS_REASON_NONE,
                               false /* should_do_learning */),
        candidate_window_handler_(nullptr) {}
#else
  IMEBridgeImpl()
      : input_context_handler_(nullptr),
        engine_handler_(nullptr),
        observer_(nullptr),
        current_input_context_(ui::TEXT_INPUT_TYPE_NONE,
                               ui::TEXT_INPUT_MODE_DEFAULT,
                               0,
                               ui::TextInputClient::FOCUS_REASON_NONE,
                               false /* should_do_learning */) {}
#endif

  ~IMEBridgeImpl() override {}

  // IMEBridge override.
  IMEInputContextHandlerInterface* GetInputContextHandler() const override {
    return input_context_handler_;
  }

  // IMEBridge override.
  void SetInputContextHandler(
      IMEInputContextHandlerInterface* handler) override {
    input_context_handler_ = handler;
  }

  // IMEBridge override.
  void SetCurrentEngineHandler(IMEEngineHandlerInterface* handler) override {
    engine_handler_ = handler;
  }

  // IMEBridge override.
  IMEEngineHandlerInterface* GetCurrentEngineHandler() const override {
    return engine_handler_;
  }

  // IMEBridge override.
  void SetCurrentInputContext(
      const IMEEngineHandlerInterface::InputContext& input_context) override {
    current_input_context_ = input_context;
  }

  // IMEBridge override.
  const IMEEngineHandlerInterface::InputContext& GetCurrentInputContext()
      const override {
    return current_input_context_;
  }

  // IMEBridge override.
  void SetObserver(ui::IMEBridgeObserver* observer) override {
    observer_ = observer;
  }

  // IMEBridge override.
  void MaybeSwitchEngine() override {
    if (observer_)
      observer_->OnRequestSwitchEngine();
  }

#if defined(OS_CHROMEOS)
  // IMEBridge override.
  void SetCandidateWindowHandler(
      chromeos::IMECandidateWindowHandlerInterface* handler) override {
    candidate_window_handler_ = handler;
  }

  // IMEBridge override.
  chromeos::IMECandidateWindowHandlerInterface* GetCandidateWindowHandler()
      const override {
    return candidate_window_handler_;
  }
#endif

 private:
  IMEInputContextHandlerInterface* input_context_handler_;
  IMEEngineHandlerInterface* engine_handler_;
  IMEBridgeObserver* observer_;
  IMEEngineHandlerInterface::InputContext current_input_context_;

#if defined(OS_CHROMEOS)
  chromeos::IMECandidateWindowHandlerInterface* candidate_window_handler_;
#endif

  DISALLOW_COPY_AND_ASSIGN(IMEBridgeImpl);
};

///////////////////////////////////////////////////////////////////////////////
// IMEBridge
IMEBridge::IMEBridge() {}

IMEBridge::~IMEBridge() {}

// static.
void IMEBridge::Initialize() {
  if (!g_ime_bridge)
    g_ime_bridge = new IMEBridgeImpl();
}

// static.
void IMEBridge::Shutdown() {
  delete g_ime_bridge;
  g_ime_bridge = nullptr;
}

// static.
IMEBridge* IMEBridge::Get() {
  return g_ime_bridge;
}

}  // namespace ui
