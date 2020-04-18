// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/plugin_installer_observer.h"

#include "chrome/browser/plugins/plugin_installer.h"

PluginInstallerObserver::PluginInstallerObserver(PluginInstaller* installer)
    : installer_(installer) {
  if (installer_)
    installer_->AddObserver(this);
}

PluginInstallerObserver::~PluginInstallerObserver() {
  if (installer_)
    installer_->RemoveObserver(this);
}

void PluginInstallerObserver::DownloadFinished() {
}

WeakPluginInstallerObserver::WeakPluginInstallerObserver(
    PluginInstaller* installer) : PluginInstallerObserver(installer) {
  if (installer)
    installer->AddWeakObserver(this);
}

WeakPluginInstallerObserver::~WeakPluginInstallerObserver() {
  if (installer())
    installer()->RemoveWeakObserver(this);
}

void WeakPluginInstallerObserver::OnlyWeakObserversLeft() {
}
