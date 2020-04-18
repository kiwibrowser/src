// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/signin/gaia_auth_extension_loader.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/extension_system.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/system/input_device_settings.h"
#endif

using content::BrowserContext;
using content::BrowserThread;

namespace {

extensions::ComponentLoader* GetComponentLoader(BrowserContext* context) {
  extensions::ExtensionSystem* extension_system =
      extensions::ExtensionSystem::Get(context);
  ExtensionService* extension_service = extension_system->extension_service();
  return extension_service->component_loader();
}

void LoadGaiaAuthExtension(BrowserContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  extensions::ComponentLoader* component_loader = GetComponentLoader(context);
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kAuthExtensionPath)) {
    base::FilePath auth_extension_path =
        command_line->GetSwitchValuePath(switches::kAuthExtensionPath);
    component_loader->Add(IDR_GAIA_AUTH_MANIFEST, auth_extension_path);
    return;
  }

  component_loader->Add(IDR_GAIA_AUTH_MANIFEST,
                        base::FilePath(FILE_PATH_LITERAL("gaia_auth")));
}

void UnloadGaiaAuthExtension(BrowserContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GetComponentLoader(context)->Remove(extensions::kGaiaAuthExtensionId);
}

}  // namespace

namespace extensions {

GaiaAuthExtensionLoader::GaiaAuthExtensionLoader(BrowserContext* context)
    : browser_context_(context),
      load_count_(0),
      weak_ptr_factory_(this) {
}

GaiaAuthExtensionLoader::~GaiaAuthExtensionLoader() {
  DCHECK_EQ(0, load_count_);
}

void GaiaAuthExtensionLoader::LoadIfNeeded() {
  if (load_count_ == 0)
    LoadGaiaAuthExtension(browser_context_);
  ++load_count_;
}

void GaiaAuthExtensionLoader::UnloadIfNeededAsync() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&GaiaAuthExtensionLoader::UnloadIfNeeded,
                                weak_ptr_factory_.GetWeakPtr()));
}

void GaiaAuthExtensionLoader::UnloadIfNeeded() {
  --load_count_;
  if (load_count_ == 0) {
    UnloadGaiaAuthExtension(browser_context_);
  }
}

void GaiaAuthExtensionLoader::Shutdown() {
  if (load_count_ > 0) {
    UnloadGaiaAuthExtension(browser_context_);
    load_count_ = 0;
  }
}

// static
GaiaAuthExtensionLoader* GaiaAuthExtensionLoader::Get(BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<GaiaAuthExtensionLoader>::Get(context);
}

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<GaiaAuthExtensionLoader>>::DestructorAtExit
    g_gaia_auth_extension_loader_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<GaiaAuthExtensionLoader>*
GaiaAuthExtensionLoader::GetFactoryInstance() {
  return g_gaia_auth_extension_loader_factory.Pointer();
}

} // namespace extensions
