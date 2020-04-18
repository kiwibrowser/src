// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/extension_message_bubble_factory.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/metrics/field_trial.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/dev_mode_bubble_delegate.h"
#include "chrome/browser/extensions/extension_message_bubble_controller.h"
#include "chrome/browser/extensions/install_verifier.h"
#include "chrome/browser/extensions/proxy_overridden_bubble_delegate.h"
#include "chrome/browser/extensions/settings_api_bubble_delegate.h"
#include "chrome/browser/extensions/settings_api_helpers.h"
#include "chrome/browser/extensions/suspicious_extension_bubble_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/version_info.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/feature_switch.h"

namespace {

// A map of all profiles evaluated, so we can tell if it's the initial check.
// TODO(devlin): It would be nice to coalesce all the "profiles evaluated" maps
// that are in the different bubble controllers.
base::LazyInstance<std::set<Profile*>>::DestructorAtExit g_profiles_evaluated =
    LAZY_INSTANCE_INITIALIZER;

// This is used to turn on override whether bubbles are enabled or disabled for
// testing.
ExtensionMessageBubbleFactory::OverrideForTesting g_override_for_testing =
    ExtensionMessageBubbleFactory::NO_OVERRIDE;

const char kEnableDevModeWarningExperimentName[] =
    "ExtensionDeveloperModeWarning";

#if !defined(OS_WIN) && !defined(OS_MACOSX)
const char kEnableProxyWarningExperimentName[] = "ExtensionProxyWarning";
#endif

bool IsExperimentEnabled(const char* experiment_name) {
  // Don't allow turning it off via command line.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kForceFieldTrials)) {
    std::string forced_trials =
        command_line->GetSwitchValueASCII(switches::kForceFieldTrials);
    if (forced_trials.find(experiment_name))
      return true;
  }
  return base::FieldTrialList::FindFullName(experiment_name) == "Enabled";
}

bool EnableSuspiciousExtensionsBubble() {
  return g_override_for_testing ==
             ExtensionMessageBubbleFactory::OVERRIDE_ENABLED ||
         extensions::InstallVerifier::ShouldEnforce();
}

bool EnableSettingsApiBubble() {
#if defined(OS_WIN) || defined(OS_MACOSX)
  return true;
#else
  return g_override_for_testing ==
         ExtensionMessageBubbleFactory::OVERRIDE_ENABLED;
#endif
}

bool EnableProxyOverrideBubble() {
#if defined(OS_WIN) || defined(OS_MACOSX)
  return true;
#else
  return g_override_for_testing ==
             ExtensionMessageBubbleFactory::OVERRIDE_ENABLED ||
         IsExperimentEnabled(kEnableProxyWarningExperimentName);
#endif
}

bool EnableDevModeBubble() {
  if (extensions::FeatureSwitch::force_dev_mode_highlighting()->IsEnabled())
    return true;

  // If an automated test is controlling the browser, we don't show the dev mode
  // bubble because it interferes with focus. This isn't a security concern
  // because we'll instead show an (even scarier) infobar. See also
  // AutomationInfoBarDelegate.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnableAutomation))
    return false;

#if defined(OS_WIN)
  if (chrome::GetChannel() >= version_info::Channel::BETA)
    return true;
#endif

  return g_override_for_testing ==
             ExtensionMessageBubbleFactory::OVERRIDE_ENABLED ||
         IsExperimentEnabled(kEnableDevModeWarningExperimentName);
}

}  // namespace

ExtensionMessageBubbleFactory::ExtensionMessageBubbleFactory(Browser* browser)
    : browser_(browser) {
}

ExtensionMessageBubbleFactory::~ExtensionMessageBubbleFactory() {
}

std::unique_ptr<extensions::ExtensionMessageBubbleController>
ExtensionMessageBubbleFactory::GetController() {
  Profile* original_profile = browser_->profile()->GetOriginalProfile();
  std::set<Profile*>& profiles_evaluated = g_profiles_evaluated.Get();
  bool is_initial_check = profiles_evaluated.count(original_profile) == 0;
  profiles_evaluated.insert(original_profile);

  std::unique_ptr<extensions::ExtensionMessageBubbleController> controller;

  if (g_override_for_testing == OVERRIDE_DISABLED)
    return controller;

  // The list of suspicious extensions takes priority over the dev mode bubble
  // and the settings API bubble, since that needs to be shown as soon as we
  // disable something. The settings API bubble is shown on first startup after
  // an extension has changed the startup pages and it is acceptable if that
  // waits until the next startup because of the suspicious extension bubble.
  // The dev mode bubble is not time sensitive like the other two so we'll catch
  // the dev mode extensions on the next startup/next window that opens. That
  // way, we're not too spammy with the bubbles.
  if (EnableSuspiciousExtensionsBubble()) {
    controller.reset(
        new extensions::ExtensionMessageBubbleController(
            new extensions::SuspiciousExtensionBubbleDelegate(
                browser_->profile()),
            browser_));
    if (controller->ShouldShow())
      return controller;
  }

  if (EnableSettingsApiBubble()) {
    // No use showing this if it's not the startup of the profile, and if the
    // browser was restarted, then we always do a session restore (rather than
    // showing normal startup pages).
    if (is_initial_check && !StartupBrowserCreator::WasRestarted()) {
      controller.reset(new extensions::ExtensionMessageBubbleController(
              new extensions::SettingsApiBubbleDelegate(
                  browser_->profile(), extensions::BUBBLE_TYPE_STARTUP_PAGES),
                  browser_));
      if (controller->ShouldShow())
        return controller;
    }
  }

  if (EnableProxyOverrideBubble()) {
    controller.reset(
        new extensions::ExtensionMessageBubbleController(
            new extensions::ProxyOverriddenBubbleDelegate(
                browser_->profile()),
            browser_));
    if (controller->ShouldShow())
      return controller;
  }

  if (EnableDevModeBubble()) {
    controller.reset(
        new extensions::ExtensionMessageBubbleController(
            new extensions::DevModeBubbleDelegate(
                browser_->profile()),
            browser_));
    if (controller->ShouldShow())
      return controller;
  }

  controller.reset();
  return controller;
}

// static
void ExtensionMessageBubbleFactory::set_override_for_tests(
    OverrideForTesting override) {
  g_override_for_testing = override;
}
