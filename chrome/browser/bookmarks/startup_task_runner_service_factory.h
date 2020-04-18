// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARKS_STARTUP_TASK_RUNNER_SERVICE_FACTORY_H_
#define CHROME_BROWSER_BOOKMARKS_STARTUP_TASK_RUNNER_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace bookmarks {
class StartupTaskRunnerService;
}

// Singleton that owns the start-up task runner service.
class StartupTaskRunnerServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of StartupTaskRunnerService associated with this
  // profile (creating one if none exists).
  static bookmarks::StartupTaskRunnerService* GetForProfile(Profile* profile);

  // Returns an instance of the StartupTaskRunnerServiceFactory singleton.
  static StartupTaskRunnerServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<StartupTaskRunnerServiceFactory>;

  StartupTaskRunnerServiceFactory();
  ~StartupTaskRunnerServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;

  DISALLOW_COPY_AND_ASSIGN(StartupTaskRunnerServiceFactory);
};

#endif  // CHROME_BROWSER_BOOKMARKS_STARTUP_TASK_RUNNER_SERVICE_FACTORY_H_
