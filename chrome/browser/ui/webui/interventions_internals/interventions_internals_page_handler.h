// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_INTERVENTIONS_INTERNALS_INTERVENTIONS_INTERNALS_PAGE_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_INTERVENTIONS_INTERNALS_INTERVENTIONS_INTERNALS_PAGE_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/ui/webui/interventions_internals/interventions_internals.mojom.h"
#include "components/previews/content/previews_ui_service.h"
#include "components/previews/core/previews_logger.h"
#include "components/previews/core/previews_logger_observer.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/effective_connection_type_observer.h"

class UINetworkQualityEstimatorService;

class InterventionsInternalsPageHandler
    : public previews::PreviewsLoggerObserver,
      public net::EffectiveConnectionTypeObserver,
      public mojom::InterventionsInternalsPageHandler {
 public:
  InterventionsInternalsPageHandler(
      mojom::InterventionsInternalsPageHandlerRequest request,
      previews::PreviewsUIService* previews_ui_service,
      UINetworkQualityEstimatorService* ui_nqe_service);
  ~InterventionsInternalsPageHandler() override;

  // mojom::InterventionsInternalsPageHandler:
  void GetPreviewsEnabled(GetPreviewsEnabledCallback callback) override;
  void GetPreviewsFlagsDetails(
      GetPreviewsFlagsDetailsCallback callback) override;
  void SetClientPage(mojom::InterventionsInternalsPagePtr page) override;
  void SetIgnorePreviewsBlacklistDecision(bool ignore) override;

  // previews::PreviewsLoggerObserver:
  void OnNewMessageLogAdded(
      const previews::PreviewsLogger::MessageLog& message) override;
  void OnNewBlacklistedHost(const std::string& host, base::Time time) override;
  void OnUserBlacklistedStatusChange(bool blacklisted) override;
  void OnBlacklistCleared(base::Time time) override;
  void OnIgnoreBlacklistDecisionStatusChanged(bool ignored) override;
  void OnLastObserverRemove() override;

 private:
  // net::EffectiveConnectionTypeObserver:
  void OnEffectiveConnectionTypeChanged(
      net::EffectiveConnectionType type) override;

  mojo::Binding<mojom::InterventionsInternalsPageHandler> binding_;

  // The PreviewsLogger that this handler is listening to, and guaranteed to
  // outlive |this|.
  previews::PreviewsLogger* logger_;

  // A pointer to the PreviewsUIService associated with this handler, and
  // guaranteed to outlive |this|.
  previews::PreviewsUIService* previews_ui_service_;

  // A pointer to the UINetworkQualityEsitmatorService, guaranteed to outlive
  // |this|.
  UINetworkQualityEstimatorService* ui_nqe_service_;

  // The current estimated effective connection type.
  net::EffectiveConnectionType current_estimated_ect_;

  // Handle back to the page by which we can pass in new log messages.
  mojom::InterventionsInternalsPagePtr page_;

  DISALLOW_COPY_AND_ASSIGN(InterventionsInternalsPageHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_INTERVENTIONS_INTERNALS_INTERVENTIONS_INTERNALS_PAGE_HANDLER_H_
