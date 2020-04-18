// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_WEBDATA_PASSWORD_WEB_DATA_SERVICE_WIN_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_WEBDATA_PASSWORD_WEB_DATA_SERVICE_WIN_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner_helpers.h"
#include "components/webdata/common/web_data_results.h"
#include "components/webdata/common/web_data_service_base.h"
#include "components/webdata/common/web_data_service_consumer.h"
#include "components/webdata/common/web_database.h"

struct IE7PasswordInfo;
class WebDatabaseService;

namespace base {
class SingleThreadTaskRunner;
}

namespace content {
class BrowserContext;
}

// PasswordWebDataService is used to access IE7/8 Password data stored in the
// web database. All data is retrieved and archived in an asynchronous way.

class WebDataServiceConsumer;

class PasswordWebDataService : public WebDataServiceBase {
 public:
  // Retrieves a WebDataService for the given context.
  static scoped_refptr<PasswordWebDataService> FromBrowserContext(
      content::BrowserContext* context);

  PasswordWebDataService(
      scoped_refptr<WebDatabaseService> wdbs,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      const ProfileErrorCallback& callback);

  // Adds |info| to the list of imported passwords from ie7/ie8.
  void AddIE7Login(const IE7PasswordInfo& info);

  // Removes |info| from the list of imported passwords from ie7/ie8.
  void RemoveIE7Login(const IE7PasswordInfo& info);

  // Gets the login matching the information in |info|. |consumer| will be
  // notified when the request is done. The result is of type
  // WDResult<IE7PasswordInfo>.
  // If there is no match, the fields of the IE7PasswordInfo will be empty.
  // All requests return a handle. The handle can be used to cancel the request.
  Handle GetIE7Login(const IE7PasswordInfo& info,
                     WebDataServiceConsumer* consumer);

 protected:
  // For unit tests, passes a null callback.
  PasswordWebDataService(
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);

  ~PasswordWebDataService() override;

 private:
  // The following methods are only invoked on the DB sequence.
  WebDatabase::State AddIE7LoginImpl(const IE7PasswordInfo& info,
                                     WebDatabase* db);
  WebDatabase::State RemoveIE7LoginImpl(const IE7PasswordInfo& info,
                                        WebDatabase* db);
  std::unique_ptr<WDTypedResult> GetIE7LoginImpl(const IE7PasswordInfo& info,
                                                 WebDatabase* db);

  DISALLOW_COPY_AND_ASSIGN(PasswordWebDataService);
};

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_WEBDATA_PASSWORD_WEB_DATA_SERVICE_WIN_H_
