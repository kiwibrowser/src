// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/webdata/web_data_model_type_controller.h"

#include "base/bind.h"
#include "base/bind_helpers.h"

using syncer::ModelType;
using syncer::ModelTypeController;
using syncer::SyncClient;

namespace autofill {

WebDataModelTypeController::WebDataModelTypeController(
    ModelType type,
    SyncClient* sync_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& model_thread,
    const scoped_refptr<AutofillWebDataService>& web_data_service,
    const DelegateFromWebData& delegate_from_web_data)
    : ModelTypeController(type, sync_client, model_thread),
      web_data_service_(web_data_service),
      delegate_from_web_data_(delegate_from_web_data) {}

WebDataModelTypeController::~WebDataModelTypeController() {}

ModelTypeController::DelegateProvider
WebDataModelTypeController::GetDelegateProvider() {
  // As opposed to the default implementation, get the delegate on demand, the
  // web data service requires us to be on the model thread.
  return base::Bind(delegate_from_web_data_,
                    base::RetainedRef(web_data_service_));
}

}  // namespace autofill
