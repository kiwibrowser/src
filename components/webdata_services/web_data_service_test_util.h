// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_TEST_UTIL_H__
#define COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_TEST_UTIL_H__

#include "base/macros.h"
#include "components/signin/core/browser/webdata/token_web_data.h"
#include "components/webdata_services/web_data_service_wrapper.h"

// Base class for mocks of WebDataService, that does nothing in
// Shutdown().
class MockWebDataServiceWrapperBase : public WebDataServiceWrapper {
 public:
  MockWebDataServiceWrapperBase();
  ~MockWebDataServiceWrapperBase() override;

  void Shutdown() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWebDataServiceWrapperBase);
};

// Pass your fake WebDataService in the constructor and this will
// serve it up via GetWebData().
class MockWebDataServiceWrapper : public MockWebDataServiceWrapperBase {
 public:
  MockWebDataServiceWrapper(
      scoped_refptr<autofill::AutofillWebDataService> fake_autofill,
      scoped_refptr<TokenWebData> fake_token);

  ~MockWebDataServiceWrapper() override;

  scoped_refptr<autofill::AutofillWebDataService> GetAutofillWebData() override;

  scoped_refptr<TokenWebData> GetTokenWebData() override;

 protected:
  scoped_refptr<autofill::AutofillWebDataService> fake_autofill_web_data_;
  scoped_refptr<TokenWebData> fake_token_web_data_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWebDataServiceWrapper);
};

#endif  // COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_TEST_UTIL_H__
