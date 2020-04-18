// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/importer/profile_import_impl.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/utility/importer/external_process_importer_bridge.h"
#include "chrome/utility/importer/importer.h"
#include "chrome/utility/importer/importer_creator.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using chrome::mojom::ThreadSafeProfileImportObserverPtr;

ProfileImportImpl::ProfileImportImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

ProfileImportImpl::~ProfileImportImpl() {}

void ProfileImportImpl::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t items,
    const base::flat_map<uint32_t, std::string>& localized_strings,
    chrome::mojom::ProfileImportObserverPtr observer) {
  content::UtilityThread::Get()->EnsureBlinkInitialized();
  importer_ = importer::CreateImporterByType(source_profile.importer_type);
  if (!importer_.get()) {
    observer->OnImportFinished(false, "Importer could not be created.");
    return;
  }

  items_to_import_ = items;

  // Create worker thread in which importer runs.
  import_thread_.reset(new base::Thread("import_thread"));
#if defined(OS_WIN)
  import_thread_->init_com_with_mta(false);
#endif
  if (!import_thread_->Start()) {
    NOTREACHED();
    ImporterCleanup();
  }
  bridge_ = new ExternalProcessImporterBridge(
      localized_strings,
      ThreadSafeProfileImportObserverPtr::Create(std::move(observer)));
  import_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&Importer::StartImport, importer_, source_profile, items,
                     base::RetainedRef(bridge_)));
}

void ProfileImportImpl::CancelImport() {
  ImporterCleanup();
}

void ProfileImportImpl::ReportImportItemFinished(importer::ImportItem item) {
  items_to_import_ ^= item;  // Remove finished item from mask.
  if (items_to_import_ == 0) {
    ImporterCleanup();
  }
}

void ProfileImportImpl::ImporterCleanup() {
  importer_->Cancel();
  importer_ = NULL;
  bridge_ = NULL;
  import_thread_.reset();
}
