// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/webdata_services/web_data_service_test_util.h"

#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"

using autofill::AutofillWebDataService;

MockWebDataServiceWrapperBase::MockWebDataServiceWrapperBase() {
}

MockWebDataServiceWrapperBase::~MockWebDataServiceWrapperBase() {
}

void MockWebDataServiceWrapperBase::Shutdown() {
}

// TODO(caitkp): This won't scale well. As we get more WebData subclasses, we
// will probably need a better way to create these mocks rather than passing
// all the webdatas in.
MockWebDataServiceWrapper::MockWebDataServiceWrapper(
    scoped_refptr<AutofillWebDataService> fake_autofill,
    scoped_refptr<TokenWebData> fake_token)
    : fake_autofill_web_data_(fake_autofill), fake_token_web_data_(fake_token) {
}

MockWebDataServiceWrapper::~MockWebDataServiceWrapper() {
}

scoped_refptr<AutofillWebDataService>
MockWebDataServiceWrapper::GetAutofillWebData() {
  return fake_autofill_web_data_;
}

scoped_refptr<TokenWebData> MockWebDataServiceWrapper::GetTokenWebData() {
  return fake_token_web_data_;
}
