// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/virtual_keyboard_private/chrome_virtual_keyboard_delegate.h"

#include <memory>
#include <string>
#include <utility>

#include "ash/shell.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/string16.h"
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/chromeos/login/ui/user_adding_screen.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/url_constants.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/browser/event_router.h"
#include "extensions/common/api/virtual_keyboard.h"
#include "extensions/common/api/virtual_keyboard_private.h"
#include "media/audio/audio_system.h"
#include "services/audio/public/cpp/audio_system_factory.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/aura/window_tree_host.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_util.h"

namespace keyboard_api = extensions::api::virtual_keyboard_private;

namespace {

aura::Window* GetKeyboardContainer() {
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  return controller ? controller->GetContainerWindow() : nullptr;
}

std::string GenerateFeatureFlag(const std::string& feature, bool enabled) {
  return feature + (enabled ? "-enabled" : "-disabled");
}

keyboard::KeyboardState getKeyboardStateEnum(
    keyboard_api::KeyboardState state) {
  switch (state) {
    case keyboard_api::KEYBOARD_STATE_ENABLED:
      return keyboard::KEYBOARD_STATE_ENABLED;
    case keyboard_api::KEYBOARD_STATE_DISABLED:
      return keyboard::KEYBOARD_STATE_DISABLED;
    case keyboard_api::KEYBOARD_STATE_AUTO:
    case keyboard_api::KEYBOARD_STATE_NONE:
      return keyboard::KEYBOARD_STATE_AUTO;
  }
  return keyboard::KEYBOARD_STATE_AUTO;
}

}  // namespace

namespace extensions {

ChromeVirtualKeyboardDelegate::ChromeVirtualKeyboardDelegate(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context), weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

ChromeVirtualKeyboardDelegate::~ChromeVirtualKeyboardDelegate() {}

void ChromeVirtualKeyboardDelegate::GetKeyboardConfig(
    OnKeyboardSettingsCallback on_settings_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!audio_system_)
    audio_system_ = audio::CreateAudioSystem(
        content::ServiceManagerConnection::GetForProcess()
            ->GetConnector()
            ->Clone());
  audio_system_->HasInputDevices(
      base::BindOnce(&ChromeVirtualKeyboardDelegate::OnHasInputDevices,
                     weak_this_, std::move(on_settings_callback)));
}

void ChromeVirtualKeyboardDelegate::OnKeyboardConfigChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GetKeyboardConfig(base::Bind(
      &ChromeVirtualKeyboardDelegate::DispatchConfigChangeEvent, weak_this_));
}

bool ChromeVirtualKeyboardDelegate::HideKeyboard() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  if (!controller)
    return false;

  // Pass HIDE_REASON_MANUAL since calls to HideKeyboard as part of this API
  // would be user generated.
  controller->HideKeyboard(keyboard::KeyboardController::HIDE_REASON_MANUAL);
  return true;
}

bool ChromeVirtualKeyboardDelegate::InsertText(const base::string16& text) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return keyboard::InsertText(text);
}

bool ChromeVirtualKeyboardDelegate::OnKeyboardLoaded() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::RecordAction(base::UserMetricsAction("VirtualKeyboardLoaded"));
  return true;
}

void ChromeVirtualKeyboardDelegate::SetHotrodKeyboard(bool enable) {
  if (keyboard::GetHotrodKeyboardEnabled() == enable)
    return;

  keyboard::SetHotrodKeyboardEnabled(enable);
  // This reloads virtual keyboard even if it exists. This ensures virtual
  // keyboard gets the correct state of the hotrod keyboard through
  // chrome.virtualKeyboardPrivate.getKeyboardConfig.
  if (keyboard::IsKeyboardEnabled())
    ash::Shell::Get()->CreateKeyboard();
}

bool ChromeVirtualKeyboardDelegate::LockKeyboard(bool state) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  if (!controller)
    return false;

  keyboard::KeyboardController::GetInstance()->set_keyboard_locked(state);
  return true;
}

bool ChromeVirtualKeyboardDelegate::SendKeyEvent(const std::string& type,
                                                 int char_value,
                                                 int key_code,
                                                 const std::string& key_name,
                                                 int modifiers) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  aura::Window* window = GetKeyboardContainer();
  return window && keyboard::SendKeyEvent(type, char_value, key_code, key_name,
                                          modifiers, window->GetHost());
}

bool ChromeVirtualKeyboardDelegate::ShowLanguageSettings() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::RecordAction(base::UserMetricsAction("OpenLanguageOptionsDialog"));
  chrome::ShowSettingsSubPageForProfile(ProfileManager::GetActiveUserProfile(),
                                        chrome::kLanguageOptionsSubPage);
  return true;
}

bool ChromeVirtualKeyboardDelegate::SetVirtualKeyboardMode(
    int mode_enum,
    base::Optional<gfx::Rect> target_bounds,
    OnSetModeCallback on_set_mode_callback) {
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  if (!controller)
    return false;

  controller->SetContainerType(ConvertKeyboardModeToContainerType(mode_enum),
                               std::move(target_bounds),
                               std::move(on_set_mode_callback));
  return true;
}

keyboard::ContainerType
ChromeVirtualKeyboardDelegate::ConvertKeyboardModeToContainerType(
    int mode) const {
  switch (mode) {
    case keyboard_api::KEYBOARD_MODE_FULL_WIDTH:
      return keyboard::ContainerType::FULL_WIDTH;
    case keyboard_api::KEYBOARD_MODE_FLOATING:
      return keyboard::ContainerType::FLOATING;
    case keyboard_api::KEYBOARD_MODE_FULLSCREEN:
      return keyboard::ContainerType::FULLSCREEN;
  }

  NOTREACHED();
  return keyboard::ContainerType::FULL_WIDTH;
}

bool ChromeVirtualKeyboardDelegate::SetDraggableArea(
    const api::virtual_keyboard_private::Bounds& rect) {
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  // Since controller will be destroyed when system switch from VK to
  // physical keyboard, return true to avoid unneccessary exception.
  if (!controller)
    return true;
  return controller->SetDraggableArea(
      gfx::Rect(rect.top, rect.left, rect.width, rect.height));
}

bool ChromeVirtualKeyboardDelegate::SetRequestedKeyboardState(int state_enum) {
  keyboard::KeyboardState keyboard_state = getKeyboardStateEnum(
      static_cast<keyboard_api::KeyboardState>(state_enum));
  bool was_enabled = keyboard::IsKeyboardEnabled();
  keyboard::SetRequestedKeyboardState(keyboard_state);
  bool is_enabled = keyboard::IsKeyboardEnabled();
  if (was_enabled == is_enabled)
    return true;
  if (is_enabled)
    ash::Shell::Get()->CreateKeyboard();
  else
    ash::Shell::Get()->DestroyKeyboard();
  return true;
}

bool ChromeVirtualKeyboardDelegate::IsLanguageSettingsEnabled() {
  return (user_manager::UserManager::Get()->IsUserLoggedIn() &&
          !chromeos::UserAddingScreen::Get()->IsRunning() &&
          !(chromeos::ScreenLocker::default_screen_locker() &&
            chromeos::ScreenLocker::default_screen_locker()->locked()));
}

void ChromeVirtualKeyboardDelegate::OnHasInputDevices(
    OnKeyboardSettingsCallback on_settings_callback,
    bool has_audio_input_devices) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::unique_ptr<base::DictionaryValue> results(new base::DictionaryValue());
  results->SetString("layout", keyboard::GetKeyboardLayout());
  // TODO(bshe): Consolidate a11y, hotrod and normal mode into a mode enum. See
  // crbug.com/529474.
  results->SetBoolean("a11ymode", keyboard::GetAccessibilityKeyboardEnabled());
  results->SetBoolean("hotrodmode", keyboard::GetHotrodKeyboardEnabled());
  std::unique_ptr<base::ListValue> features(new base::ListValue());

  // 'floatingvirtualkeyboard' is the name of the feature flag for the legacy
  // floating keyboard that was prototyped quite some time ago. It is currently
  // referenced by the extension even though we never enable this value and so
  // re-using that value is not feasible due to the semi-tandem nature of the
  // keyboard extension. The 'floatingkeybard' flag represents the new floating
  // keyboard and should be used for new extension-side feature work for the
  // floating keyboard.
  // TODO(blakeo): once the old flag's usages have been removed from the
  // extension and all pushes have settled, remove this overly verbose comment.
  features->AppendString(GenerateFeatureFlag(
      "floatingkeyboard", keyboard::IsFloatingVirtualKeyboardEnabled()));
  features->AppendString(
      GenerateFeatureFlag("gesturetyping", keyboard::IsGestureTypingEnabled()));
  features->AppendString(GenerateFeatureFlag(
      "gestureediting", keyboard::IsGestureEditingEnabled()));
  features->AppendString(GenerateFeatureFlag(
      "experimental", keyboard::IsExperimentalInputViewEnabled()));
  features->AppendString(GenerateFeatureFlag(
      "fullscreenhandwriting",
      keyboard::IsFullscreenHandwritingVirtualKeyboardEnabled()));

  const keyboard::KeyboardConfig config = keyboard::GetKeyboardConfig();
  // TODO(oka): Change this to use config.voice_input.
  features->AppendString(GenerateFeatureFlag(
      "voiceinput", has_audio_input_devices && config.voice_input &&
                        !base::CommandLine::ForCurrentProcess()->HasSwitch(
                            keyboard::switches::kDisableVoiceInput)));
  features->AppendString(
      GenerateFeatureFlag("autocomplete", config.auto_complete));
  features->AppendString(
      GenerateFeatureFlag("autocorrect", config.auto_correct));
  features->AppendString(GenerateFeatureFlag("spellcheck", config.spell_check));
  features->AppendString(
      GenerateFeatureFlag("handwriting", config.handwriting));

  results->Set("features", std::move(features));

  std::move(on_settings_callback).Run(std::move(results));
}

void ChromeVirtualKeyboardDelegate::DispatchConfigChangeEvent(
    std::unique_ptr<base::DictionaryValue> settings) {
  EventRouter* router = EventRouter::Get(browser_context_);

  if (!router->HasEventListener(
          keyboard_api::OnKeyboardConfigChanged::kEventName))
    return;

  auto event_args = std::make_unique<base::ListValue>();
  event_args->Append(std::move(settings));

  auto event = std::make_unique<extensions::Event>(
      extensions::events::VIRTUAL_KEYBOARD_PRIVATE_ON_KEYBOARD_CONFIG_CHANGED,
      keyboard_api::OnKeyboardConfigChanged::kEventName, std::move(event_args),
      browser_context_);
  router->BroadcastEvent(std::move(event));
}

api::virtual_keyboard::FeatureRestrictions
ChromeVirtualKeyboardDelegate::RestrictFeatures(
    const api::virtual_keyboard::RestrictFeatures::Params& params) {
  const api::virtual_keyboard::FeatureRestrictions& restrictions =
      params.restrictions;
  api::virtual_keyboard::FeatureRestrictions update;
  keyboard::KeyboardConfig config = keyboard::GetKeyboardConfig();
  if (restrictions.spell_check_enabled &&
      config.spell_check != *restrictions.spell_check_enabled) {
    update.spell_check_enabled =
        std::make_unique<bool>(*restrictions.spell_check_enabled);
    config.spell_check = *restrictions.spell_check_enabled;
  }
  if (restrictions.auto_complete_enabled &&
      config.auto_complete != *restrictions.auto_complete_enabled) {
    update.auto_complete_enabled =
        std::make_unique<bool>(*restrictions.auto_complete_enabled);
    config.auto_complete = *restrictions.auto_complete_enabled;
  }
  if (restrictions.auto_correct_enabled &&
      config.auto_correct != *restrictions.auto_correct_enabled) {
    update.auto_correct_enabled =
        std::make_unique<bool>(*restrictions.auto_correct_enabled);
    config.auto_correct = *restrictions.auto_correct_enabled;
  }
  if (restrictions.voice_input_enabled &&
      config.voice_input != *restrictions.voice_input_enabled) {
    update.voice_input_enabled =
        std::make_unique<bool>(*restrictions.voice_input_enabled);
    config.voice_input = *restrictions.voice_input_enabled;
  }
  if (restrictions.handwriting_enabled &&
      config.handwriting != *restrictions.handwriting_enabled) {
    update.handwriting_enabled =
        std::make_unique<bool>(*restrictions.handwriting_enabled);
    config.handwriting = *restrictions.handwriting_enabled;
  }

  if (keyboard::UpdateKeyboardConfig(config)) {
    // This reloads virtual keyboard even if it exists. This ensures virtual
    // keyboard gets the correct state through
    // chrome.virtualKeyboardPrivate.getKeyboardConfig.
    // TODO(oka): Extension should reload on it's own by receiving event
    if (keyboard::IsKeyboardEnabled())
      ash::Shell::Get()->CreateKeyboard();
  }
  return update;
}

}  // namespace extensions
