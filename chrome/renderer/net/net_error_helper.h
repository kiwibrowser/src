// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_NET_NET_ERROR_HELPER_H_
#define CHROME_RENDERER_NET_NET_ERROR_HELPER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "chrome/common/navigation_corrector.mojom.h"
#include "chrome/common/network_diagnostics.mojom.h"
#include "chrome/common/supervised_user_commands.mojom.h"
#include "chrome/renderer/net/net_error_helper_core.h"
#include "chrome/renderer/net/net_error_page_controller.h"
#include "chrome/renderer/ssl/ssl_certificate_error_page_controller.h"
#include "chrome/renderer/supervised_user/supervised_user_error_page_controller.h"
#include "chrome/renderer/supervised_user/supervised_user_error_page_controller_delegate.h"
#include "components/error_page/common/net_error_info.h"
#include "components/security_interstitials/core/controller_client.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "content/public/renderer/render_thread_observer.h"
#include "mojo/public/cpp/bindings/associated_binding_set.h"
#include "net/base/net_errors.h"

class GURL;

namespace blink {
class WebURLResponse;
}

namespace content {
class ResourceFetcher;
}

namespace error_page {
class Error;
struct ErrorPageParams;
}

// Listens for NetErrorInfo messages from the NetErrorTabHelper on the
// browser side and updates the error page with more details (currently, just
// DNS probe results) if/when available.
// TODO(crbug.com/578770): Should this class be moved into the error_page
// component?
class NetErrorHelper
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<NetErrorHelper>,
      public content::RenderThreadObserver,
      public NetErrorHelperCore::Delegate,
      public NetErrorPageController::Delegate,
      public SSLCertificateErrorPageController::Delegate,
      public SupervisedUserErrorPageControllerDelegate,
      public chrome::mojom::NetworkDiagnosticsClient,
      public chrome::mojom::NavigationCorrector {
 public:
  explicit NetErrorHelper(content::RenderFrame* render_frame);
  ~NetErrorHelper() override;

  // NetErrorPageController::Delegate implementation
  void ButtonPressed(NetErrorHelperCore::Button button) override;
  void TrackClick(int tracking_id) override;

  // SSLCertificateErrorPageController::Delegate implementation
  void SendCommand(
      security_interstitials::SecurityInterstitialCommand command) override;

  // SupervisedUserErrorPageControllerDelegate implementation
  void GoBack() override;
  void RequestPermission(base::OnceCallback<void(bool)> callback) override;
  void Feedback() override;

  // RenderFrameObserver implementation.
  void DidStartProvisionalLoad(blink::WebDocumentLoader* loader) override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override;
  void DidFinishLoad() override;
  void OnStop() override;
  void WasShown() override;
  void WasHidden() override;
  void OnDestruct() override;

  // RenderThreadObserver implementation.
  void NetworkStateChanged(bool online) override;

  // Sets values in |pending_error_page_info_|. If |error_html| is not null, it
  // initializes |error_html| with the HTML of an error page in response to
  // |error|.  Updates internals state with the assumption the page will be
  // loaded immediately.
  void PrepareErrorPage(const error_page::Error& error,
                        bool is_failed_post,
                        bool is_ignoring_cache,
                        std::string* error_html);

  // Returns whether a load for |url| in the |frame| the NetErrorHelper is
  // attached to should have its error page suppressed.
  bool ShouldSuppressErrorPage(const GURL& url);

 private:
  chrome::mojom::NetworkDiagnostics* GetRemoteNetworkDiagnostics();

  // NetErrorHelperCore::Delegate implementation:
  void GenerateLocalizedErrorPage(
      const error_page::Error& error,
      bool is_failed_post,
      bool can_use_local_diagnostics_service,
      std::unique_ptr<error_page::ErrorPageParams> params,
      bool* reload_button_shown,
      bool* show_saved_copy_button_shown,
      bool* show_cached_copy_button_shown,
      bool* download_button_shown,
      std::string* html) const override;
  void LoadErrorPage(const std::string& html, const GURL& failed_url) override;
  void EnablePageHelperFunctions(net::Error net_error) override;
  void UpdateErrorPage(const error_page::Error& error,
                       bool is_failed_post,
                       bool can_use_local_diagnostics_service) override;
  void FetchNavigationCorrections(
      const GURL& navigation_correction_url,
      const std::string& navigation_correction_request_body) override;
  void CancelFetchNavigationCorrections() override;
  void SendTrackingRequest(const GURL& tracking_url,
                           const std::string& tracking_request_body) override;
  void ReloadPage(bool bypass_cache) override;
  void LoadPageFromCache(const GURL& page_url) override;
  void DiagnoseError(const GURL& page_url) override;
  void DownloadPageLater() override;
  void SetIsShowingDownloadButton(bool show) override;

  void OnSetNavigationCorrectionInfo(const GURL& navigation_correction_url,
                                     const std::string& language,
                                     const std::string& country_code,
                                     const std::string& api_key,
                                     const GURL& search_url);

  void OnNavigationCorrectionsFetched(const blink::WebURLResponse& response,
                                      const std::string& data);

  void OnTrackingRequestComplete(const blink::WebURLResponse& response,
                                 const std::string& data);

  void OnNetworkDiagnosticsClientRequest(
      chrome::mojom::NetworkDiagnosticsClientAssociatedRequest request);
  void OnNavigationCorrectorRequest(
      chrome::mojom::NavigationCorrectorAssociatedRequest request);

  // chrome::mojom::NetworkDiagnosticsClient:
  void SetCanShowNetworkDiagnosticsDialog(bool can_show) override;
  void DNSProbeStatus(int32_t) override;

  // chrome::mojom::NavigationCorrector:
  void SetNavigationCorrectionInfo(const GURL& navigation_correction_url,
                                   const std::string& language,
                                   const std::string& country_code,
                                   const std::string& api_key,
                                   const GURL& search_url) override;

  std::unique_ptr<content::ResourceFetcher> correction_fetcher_;
  std::unique_ptr<content::ResourceFetcher> tracking_fetcher_;

  std::unique_ptr<NetErrorHelperCore> core_;

  mojo::AssociatedBindingSet<chrome::mojom::NetworkDiagnosticsClient>
      network_diagnostics_client_bindings_;
  chrome::mojom::NetworkDiagnosticsAssociatedPtr remote_network_diagnostics_;
  mojo::AssociatedBindingSet<chrome::mojom::NavigationCorrector>
      navigation_corrector_bindings_;

  supervised_user::mojom::SupervisedUserCommandsAssociatedPtr
      supervised_user_interface_;

  // Weak factories for vending weak pointers to PageControllers. Weak
  // pointers are invalidated on each commit, to prevent getting messages from
  // Controllers used for the previous commit that haven't yet been cleaned up.
  base::WeakPtrFactory<NetErrorPageController::Delegate>
      weak_controller_delegate_factory_;

  base::WeakPtrFactory<SSLCertificateErrorPageController::Delegate>
      weak_ssl_error_controller_delegate_factory_;

  base::WeakPtrFactory<SupervisedUserErrorPageControllerDelegate>
      weak_supervised_user_error_controller_delegate_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetErrorHelper);
};

#endif  // CHROME_RENDERER_NET_NET_ERROR_HELPER_H_
