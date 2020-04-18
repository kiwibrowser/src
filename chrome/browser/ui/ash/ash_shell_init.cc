// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/ash_shell_init.h"

#include <utility>

#include "ash/content/content_gpu_support.h"
#include "ash/display/display_prefs.h"
#include "ash/shell.h"
#include "ash/shell_init_params.h"
#include "ash/shell_port_classic.h"
#include "ash/window_manager.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/ash/chrome_shell_content_state.h"
#include "chrome/browser/ui/ash/chrome_shell_delegate.h"
#include "content/public/browser/context_factory.h"

namespace {

void CreateClassicShell() {
  ash::ShellInitParams shell_init_params;
  shell_init_params.shell_port = std::make_unique<ash::ShellPortClassic>();
  shell_init_params.delegate = std::make_unique<ChromeShellDelegate>();
  shell_init_params.context_factory = content::GetContextFactory();
  shell_init_params.context_factory_private =
      content::GetContextFactoryPrivate();
  shell_init_params.gpu_support = std::make_unique<ash::ContentGpuSupport>();
  // Pass the initial display prefs to ash::Shell as a Value dictionary.
  // This is done this way to avoid complexities with registering the display
  // prefs in multiple places (i.e. in g_browser_process->local_state() and
  // ash::Shell::local_state_). See https://crbug.com/834775 for details.
  shell_init_params.initial_display_prefs =
      ash::DisplayPrefs::GetInitialDisplayPrefsFromPrefService(
          g_browser_process->local_state());

  ash::Shell::CreateInstance(std::move(shell_init_params));
}

}  // namespace

// static
void AshShellInit::RegisterDisplayPrefs(PrefRegistrySimple* registry) {
  // Note: For CLASSIC/MUS, DisplayPrefs must be registered here so that
  // the initial display prefs can be passed synchronously to ash::Shell.
  ash::DisplayPrefs::RegisterLocalStatePrefs(registry);
}

AshShellInit::AshShellInit() {
  // Balanced by a call to DestroyInstance() below.
  ash::ShellContentState::SetInstance(new ChromeShellContentState);
  CreateClassicShell();
  ash::Shell::GetPrimaryRootWindow()->GetHost()->Show();
}

AshShellInit::~AshShellInit() {
  ash::Shell::DeleteInstance();
  ash::ShellContentState::DestroyInstance();
}
