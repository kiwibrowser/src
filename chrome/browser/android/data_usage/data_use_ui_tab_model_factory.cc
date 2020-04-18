// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/data_use_ui_tab_model_factory.h"

#include "base/memory/singleton.h"
#include "base/task_runner_util.h"
#include "chrome/browser/android/data_usage/data_use_tab_model.h"
#include "chrome/browser/android/data_usage/data_use_ui_tab_model.h"
#include "chrome/browser/android/data_usage/external_data_use_observer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/browser_thread.h"

namespace android {

namespace {

// Returns the pointer to the DataUseTabModel. Must be called on IO thread. The
// caller does not have the ownership of the returned pointer.
DataUseTabModel* GetDataUseTabModelOnIOThread(IOThread* io_thread) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Avoid null pointer referencing during browser shutdown.
  if (!io_thread || !io_thread->globals() ||
      !io_thread->globals()->external_data_use_observer) {
    return nullptr;
  }

  return io_thread->globals()->external_data_use_observer->GetDataUseTabModel();
}

void SetProfileSigninStatusOnIOThread(IOThread* io_thread, bool signin_status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Avoid null pointer referencing during browser shutdown.
  if (io_thread && io_thread->globals() &&
      io_thread->globals()->external_data_use_observer) {
    io_thread->globals()->external_data_use_observer->SetProfileSigninStatus(
        signin_status);
  }
}

}  // namespace

// static
DataUseUITabModelFactory* DataUseUITabModelFactory::GetInstance() {
  return base::Singleton<DataUseUITabModelFactory>::get();
}

// static
DataUseUITabModel* DataUseUITabModelFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  // Do not create DataUseUITabModel for incognito profiles.
  if (profile->GetOriginalProfile() != profile)
    return nullptr;

  return static_cast<DataUseUITabModel*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

DataUseUITabModelFactory::DataUseUITabModelFactory()
    : BrowserContextKeyedServiceFactory(
          "data_usage::DataUseUITabModel",
          BrowserContextDependencyManager::GetInstance()) {}

DataUseUITabModelFactory::~DataUseUITabModelFactory() {}

KeyedService* DataUseUITabModelFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DataUseUITabModel* data_use_ui_tab_model = new DataUseUITabModel();
  Profile* profile = Profile::FromBrowserContext(context)->GetOriginalProfile();
  const SigninManager* signin_manager =
      SigninManagerFactory::GetForProfileIfExists(profile);

  // DataUseUITabModel should not be created for incognito profile.
  DCHECK_EQ(profile, Profile::FromBrowserContext(context));

  // Pass the DataUseTabModel pointer to DataUseUITabModel.
  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&GetDataUseTabModelOnIOThread, g_browser_process->io_thread()),
      base::Bind(&android::DataUseUITabModel::SetDataUseTabModel,
                 data_use_ui_tab_model->GetWeakPtr()));

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&SetProfileSigninStatusOnIOThread,
                 g_browser_process->io_thread(),
                 signin_manager && signin_manager->IsAuthenticated()));

  return data_use_ui_tab_model;
}

}  // namespace android
