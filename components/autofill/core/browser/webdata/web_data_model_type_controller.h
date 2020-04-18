// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_WEB_DATA_MODEL_TYPE_CONTROLLER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_WEB_DATA_MODEL_TYPE_CONTROLLER_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/sync/driver/model_type_controller.h"

namespace syncer {
class ModelTypeControllerDelegate;
}  // namespace syncer

namespace autofill {

class WebDataModelTypeController : public syncer::ModelTypeController {
 public:
  using DelegateFromWebData =
      base::Callback<base::WeakPtr<syncer::ModelTypeControllerDelegate>(
          AutofillWebDataService*)>;

  WebDataModelTypeController(
      syncer::ModelType type,
      syncer::SyncClient* sync_client,
      const scoped_refptr<base::SingleThreadTaskRunner>& model_thread,
      const scoped_refptr<AutofillWebDataService>& web_data_service,
      const DelegateFromWebData& delegate_from_web_data);

  ~WebDataModelTypeController() override;

 private:
  // syncer::ModelTypeController implementation.
  syncer::ModelTypeController::DelegateProvider GetDelegateProvider() override;

  // A reference to the AutofillWebDataService for this controller.
  scoped_refptr<AutofillWebDataService> web_data_service_;

  // How to grab the correct delegate from a web data.
  DelegateFromWebData delegate_from_web_data_;

  DISALLOW_COPY_AND_ASSIGN(WebDataModelTypeController);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_WEB_DATA_MODEL_TYPE_CONTROLLER_H_
