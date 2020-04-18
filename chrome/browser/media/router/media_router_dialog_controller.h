// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/browser/presentation_request.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/blink/public/platform/modules/presentation/presentation.mojom.h"

namespace content {
class WebContents;
}  // namespace content

namespace media_router {

class MediaRoute;
class RouteRequestResult;

// Helper data structure to hold information for a dialog initiated via the
// Presentation API. Contains information on the PresentationRequest, and
// success / error callbacks. Depending on the route creation outcome,
// only one of the callbacks will be invoked exactly once.
class StartPresentationContext {
 public:
  using PresentationConnectionCallback =
      base::OnceCallback<void(const blink::mojom::PresentationInfo&,
                              const MediaRoute&)>;
  using PresentationConnectionErrorCallback =
      content::PresentationConnectionErrorCallback;

  // Handle route creation/joining response by invoking the right callback.
  static void HandleRouteResponse(
      std::unique_ptr<StartPresentationContext> presentation_request,
      const RouteRequestResult& result);

  StartPresentationContext(
      const content::PresentationRequest& presentation_request,
      PresentationConnectionCallback success_cb,
      PresentationConnectionErrorCallback error_cb);
  ~StartPresentationContext();

  const content::PresentationRequest& presentation_request() const {
    return presentation_request_;
  }

  // Invokes |success_cb_| or |error_cb_| with the given arguments.
  void InvokeSuccessCallback(const std::string& presentation_id,
                             const GURL& presentation_url,
                             const MediaRoute& route);
  void InvokeErrorCallback(const blink::mojom::PresentationError& error);

 private:
  content::PresentationRequest presentation_request_;
  PresentationConnectionCallback success_cb_;
  PresentationConnectionErrorCallback error_cb_;
  bool cb_invoked_ = false;
};

// An abstract base class for Media Router dialog controllers. Tied to a
// WebContents known as the |initiator|, and is lazily created when a Media
// Router dialog needs to be shown. The MediaRouterDialogController allows
// showing and closing a Media Router dialog modal to the initiator WebContents.
// This class is not thread safe and must be called on the UI thread.
class MediaRouterDialogController {
 public:
  virtual ~MediaRouterDialogController();

  // Gets a reference to the MediaRouterDialogController associated with
  // |web_contents|, creating one if it does not exist. The returned pointer is
  // guaranteed to be non-null.
  static MediaRouterDialogController* GetOrCreateForWebContents(
      content::WebContents* web_contents);

  // Shows the media router dialog modal to |initiator_|, with additional
  // context for a PresentationRequest coming from the page given by the input
  // parameters.
  // Returns true if the dialog is created as a result of this call.
  // If the dialog already exists, or dialog cannot be created, then false is
  // returned, and |error_cb| will be invoked.
  bool ShowMediaRouterDialogForPresentation(
      const content::PresentationRequest& presentation_request,
      StartPresentationContext::PresentationConnectionCallback success_cb,
      StartPresentationContext::PresentationConnectionErrorCallback error_cb);

  // Shows the media router dialog modal to |initiator_|.
  // Creates the dialog if it did not exist prior to this call, returns true.
  // If the dialog already exists, brings it to the front, returns false.
  virtual bool ShowMediaRouterDialog();

  // Hides the media router dialog.
  // It is a no-op to call this function if there is currently no dialog.
  void HideMediaRouterDialog();

  // Indicates if the media router dialog already exists.
  virtual bool IsShowingMediaRouterDialog() const = 0;

 protected:
  // Use MediaRouterDialogController::GetOrCreateForWebContents() to create an
  // instance.
  explicit MediaRouterDialogController(content::WebContents* initiator);

  // Creates a media router dialog if necessary, then activates the WebContents
  // that initiated the dialog, e.g. focuses the tab.
  void FocusOnMediaRouterDialog(bool dialog_needs_creation);

  // Returns the WebContents that initiated showing the dialog.
  content::WebContents* initiator() const { return initiator_; }

  // Resets the state of the controller. Must be called from the overrides.
  virtual void Reset();
  // Creates a new media router dialog modal to |initiator_|.
  virtual void CreateMediaRouterDialog() = 0;
  // Closes the media router dialog if it exists.
  virtual void CloseMediaRouterDialog() = 0;

  // Data for dialogs created at the request of the Presentation API.
  // Created from arguments passed in via ShowMediaRouterDialogForPresentation.
  std::unique_ptr<StartPresentationContext> start_presentation_context_;

 private:
  class InitiatorWebContentsObserver;

  // An observer for the |initiator_| that closes the dialog when |initiator_|
  // is destroyed or navigated.
  std::unique_ptr<InitiatorWebContentsObserver> initiator_observer_;
  content::WebContents* const initiator_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterDialogController);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_H_
