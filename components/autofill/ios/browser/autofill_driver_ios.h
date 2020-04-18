// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_BROWSER_AUTOFILL_DRIVER_IOS_H_
#define COMPONENTS_AUTOFILL_CONTENT_BROWSER_AUTOFILL_DRIVER_IOS_H_

#include <string>

#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_driver.h"
#include "components/autofill/core/browser/autofill_external_delegate.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "ios/web/public/web_state/web_state_user_data.h"

namespace web {
class WebState;
}

@protocol AutofillDriverIOSBridge;

namespace autofill {

// Class that drives autofill flow on iOS. There is one instance per
// WebContents.
class AutofillDriverIOS : public AutofillDriver,
                          public web::WebStateUserData<AutofillDriverIOS> {
 public:
  ~AutofillDriverIOS() override;

  static void CreateForWebStateAndDelegate(
      web::WebState* web_state,
      AutofillClient* client,
      id<AutofillDriverIOSBridge> bridge,
      const std::string& app_locale,
      AutofillManager::AutofillDownloadManagerState enable_download_manager);

  // AutofillDriver:
  bool IsIncognito() const override;
  net::URLRequestContextGetter* GetURLRequestContext() override;
  bool RendererIsAvailable() override;
  void SendFormDataToRenderer(int query_id,
                              RendererFormDataAction action,
                              const FormData& data) override;
  void PropagateAutofillPredictions(
      const std::vector<autofill::FormStructure*>& forms) override;
  void SendAutofillTypePredictionsToRenderer(
      const std::vector<FormStructure*>& forms) override;
  void RendererShouldClearFilledSection() override;
  void RendererShouldClearPreviewedForm() override;
  void RendererShouldAcceptDataListSuggestion(
      const base::string16& value) override;
  void DidInteractWithCreditCardForm() override;

  AutofillManager* autofill_manager() { return &autofill_manager_; }

  void RendererShouldFillFieldWithValue(const base::string16& value) override;
  void RendererShouldPreviewFieldWithValue(
      const base::string16& value) override;
  void PopupHidden() override;
  gfx::RectF TransformBoundingBoxToViewportCoordinates(
      const gfx::RectF& bounding_box) override;

 private:
  AutofillDriverIOS(
      web::WebState* web_state,
      AutofillClient* client,
      id<AutofillDriverIOSBridge> bridge,
      const std::string& app_locale,
      AutofillManager::AutofillDownloadManagerState enable_download_manager);

  // The WebState with which this object is associated.
  web::WebState* web_state_;

  // AutofillDriverIOSBridge instance that is passed in.
  __unsafe_unretained id<AutofillDriverIOSBridge> bridge_;

  // AutofillManager instance via which this object drives the shared Autofill
  // code.
  AutofillManager autofill_manager_;
  // AutofillExternalDelegate instance that is passed to the AutofillManager.
  AutofillExternalDelegate autofill_external_delegate_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_BROWSER_AUTOFILL_DRIVER_IOS_H_
