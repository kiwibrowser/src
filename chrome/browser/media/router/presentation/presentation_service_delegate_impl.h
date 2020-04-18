// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_PRESENTATION_SERVICE_DELEGATE_IMPL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_PRESENTATION_SERVICE_DELEGATE_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "build/build_config.h"
#include "chrome/browser/media/router/media_router.h"
#include "chrome/browser/media/router/presentation/presentation_service_delegate_observers.h"
#include "chrome/browser/media/router/presentation/render_frame_host_id.h"
#include "chrome/common/media_router/media_source.h"
#include "content/public/browser/media_controller.h"
#include "content/public/browser/presentation_request.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class PresentationScreenAvailabilityListener;
class WebContents;
}  // namespace content

namespace url {
class Origin;
}  // namespace url

namespace media_router {

class MediaRoute;
class PresentationFrame;
class RouteRequestResult;

// Implementation of PresentationServiceDelegate that interfaces an instance of
// WebContents with the Chrome Media Router. It uses the Media Router to handle
// presentation API calls forwarded from PresentationServiceImpl. In addition,
// it also provides default presentation URL that is required for creating
// browser-initiated presentations.  It is scoped to the lifetime of a
// WebContents, and is managed by the associated WebContents.
class PresentationServiceDelegateImpl
    : public content::WebContentsUserData<PresentationServiceDelegateImpl>,
      public content::ControllerPresentationServiceDelegate {
 public:
  // Observer interface for listening to default presentation request
  // changes for the WebContents.
  class DefaultPresentationRequestObserver {
   public:
    virtual ~DefaultPresentationRequestObserver() = default;

    // Called when default presentation request for the corresponding
    // WebContents is set or changed.
    // |default_presentation_info|: New default presentation request.
    virtual void OnDefaultPresentationChanged(
        const content::PresentationRequest& default_presentation_request) = 0;

    // Called when default presentation request for the corresponding
    // WebContents has been removed.
    virtual void OnDefaultPresentationRemoved() = 0;
  };

  // Retrieves the instance of PresentationServiceDelegateImpl that was attached
  // to the specified WebContents.  If no instance was attached, creates one,
  // and attaches it to the specified WebContents.
  static PresentationServiceDelegateImpl* GetOrCreateForWebContents(
      content::WebContents* web_contents);

  ~PresentationServiceDelegateImpl() override;

  // content::PresentationServiceDelegate implementation.
  void AddObserver(
      int render_process_id,
      int render_frame_id,
      content::PresentationServiceDelegate::Observer* observer) override;
  void RemoveObserver(int render_process_id, int render_frame_id) override;
  bool AddScreenAvailabilityListener(
      int render_process_id,
      int render_frame_id,
      content::PresentationScreenAvailabilityListener* listener) override;
  void RemoveScreenAvailabilityListener(
      int render_process_id,
      int render_frame_id,
      content::PresentationScreenAvailabilityListener* listener) override;
  void Reset(int render_process_id, int render_frame_id) override;
  void SetDefaultPresentationUrls(
      const content::PresentationRequest& request,
      content::DefaultPresentationConnectionCallback callback) override;
  void StartPresentation(
      const content::PresentationRequest& request,
      content::PresentationConnectionCallback success_cb,
      content::PresentationConnectionErrorCallback error_cb) override;
  void ReconnectPresentation(
      const content::PresentationRequest& request,
      const std::string& presentation_id,
      content::PresentationConnectionCallback success_cb,
      content::PresentationConnectionErrorCallback error_cb) override;
  void CloseConnection(int render_process_id,
                       int render_frame_id,
                       const std::string& presentation_id) override;
  void Terminate(int render_process_id,
                 int render_frame_id,
                 const std::string& presentation_id) override;
  std::unique_ptr<content::MediaController> GetMediaController(
      int render_process_id,
      int render_frame_id,
      const std::string& presentation_id) override;
  void ListenForConnectionStateChange(
      int render_process_id,
      int render_frame_id,
      const blink::mojom::PresentationInfo& connection,
      const content::PresentationConnectionStateChangedCallback&
          state_changed_cb) override;
  void ConnectToPresentation(
      int render_process_id,
      int render_frame_id,
      const blink::mojom::PresentationInfo& presentation_info,
      content::PresentationConnectionPtr controller_connection_ptr,
      content::PresentationConnectionRequest receiver_connection_request)
      override;

  // Callback invoked when a default PresentationRequest is started from a
  // browser-initiated dialog.
  void OnRouteResponse(const content::PresentationRequest& request,
                       const RouteRequestResult& result);

  // Adds / removes an observer for listening to default PresentationRequest
  // changes. This class does not own |observer|. When |observer| is about to
  // be destroyed, |RemoveDefaultPresentationRequestObserver| must be called.
  void AddDefaultPresentationRequestObserver(
      DefaultPresentationRequestObserver* observer);
  void RemoveDefaultPresentationRequestObserver(
      DefaultPresentationRequestObserver* observer);

  // Gets the default presentation request for the owning WebContents. It
  // is an error to call this method if the default presentation request does
  // not exist.
  const content::PresentationRequest& GetDefaultPresentationRequest() const;

  // Returns true if there is a default presentation request for the owning tab
  // WebContents.
  bool HasDefaultPresentationRequest() const;

  // Returns the WebContents that owns this instance.
  content::WebContents* web_contents() const { return web_contents_; }

  base::WeakPtr<PresentationServiceDelegateImpl> GetWeakPtr();

  bool HasScreenAvailabilityListenerForTest(
      int render_process_id,
      int render_frame_id,
      const MediaSource::Id& source_id) const;

 private:
  friend class content::WebContentsUserData<PresentationServiceDelegateImpl>;
  FRIEND_TEST_ALL_PREFIXES(PresentationServiceDelegateImplTest,
                           DelegateObservers);
  FRIEND_TEST_ALL_PREFIXES(PresentationServiceDelegateImplTest,
                           SetDefaultPresentationUrl);
  FRIEND_TEST_ALL_PREFIXES(PresentationServiceDelegateImplTest,
                           DefaultPresentationRequestObserver);
  FRIEND_TEST_ALL_PREFIXES(PresentationServiceDelegateImplTest,
                           DefaultPresentationUrlCallback);
  FRIEND_TEST_ALL_PREFIXES(PresentationServiceDelegateImplTest,
                           TestCloseConnectionForLocalPresentation);
  FRIEND_TEST_ALL_PREFIXES(PresentationServiceDelegateImplTest,
                           ConnectToLocalPresentation);
  FRIEND_TEST_ALL_PREFIXES(PresentationServiceDelegateImplTest,
                           ConnectToPresentation);

  explicit PresentationServiceDelegateImpl(content::WebContents* web_contents);

  PresentationFrame* GetOrAddPresentationFrame(
      const RenderFrameHostId& render_frame_host_id);

  void OnJoinRouteResponse(
      const RenderFrameHostId& render_frame_host_id,
      const GURL& presentation_url,
      const std::string& presentation_id,
      content::PresentationConnectionCallback success_cb,
      content::PresentationConnectionErrorCallback error_cb,
      const RouteRequestResult& result);

  void OnStartPresentationSucceeded(
      const RenderFrameHostId& render_frame_host_id,
      content::PresentationConnectionCallback success_cb,
      const blink::mojom::PresentationInfo& new_presentation_info,
      const MediaRoute& route);

  // Notifies the PresentationFrame of |render_frame_host_id| that a
  // presentation and its corresponding MediaRoute has been created.
  // The PresentationFrame will be created if it does not already exist.
  // This must be called before |ConnectToPresentation()|.
  void AddPresentation(const RenderFrameHostId& render_frame_host_id,
                       const blink::mojom::PresentationInfo& presentation_info,
                       const MediaRoute& route);

  // Notifies the PresentationFrame of |render_frame_host_id| that a
  // presentation and its corresponding MediaRoute has been removed.
  void RemovePresentation(const RenderFrameHostId& render_frame_host_id,
                          const std::string& presentation_id);

  // Clears the default presentation request for the owning WebContents and
  // notifies observers of changes. Also resets
  // |default_presentation_started_callback_|.
  void ClearDefaultPresentationRequest();

  // Returns the ID of the route corresponding to |presentation_id| in the given
  // frame, or empty if no such route exist.
  MediaRoute::Id GetRouteId(const RenderFrameHostId& render_frame_host_id,
                            const std::string& presentation_id) const;

#if !defined(OS_ANDROID)
  // Returns true if auto-join requests should be cancelled for |origin|.
  bool ShouldCancelAutoJoinForOrigin(const url::Origin& origin) const;
#endif

  // References to the WebContents that owns this instance, and associated
  // browser profile's MediaRouter instance.
  content::WebContents* const web_contents_;
  MediaRouter* router_;

  // References to the observers listening for changes to the default
  // presentation of the associated WebContents.
  base::ObserverList<DefaultPresentationRequestObserver>
      default_presentation_request_observers_;

  // Default presentation request for the owning WebContents.
  base::Optional<content::PresentationRequest> default_presentation_request_;

  // Callback to invoke when the default presentation has started.
  content::DefaultPresentationConnectionCallback
      default_presentation_started_callback_;

  // Maps a frame identifier to a PresentationFrame object for frames
  // that are using Presentation API.
  std::unordered_map<RenderFrameHostId,
                     std::unique_ptr<PresentationFrame>,
                     RenderFrameHostIdHasher>
      presentation_frames_;

  PresentationServiceDelegateObservers observers_;

  base::WeakPtrFactory<PresentationServiceDelegateImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PresentationServiceDelegateImpl);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_PRESENTATION_SERVICE_DELEGATE_IMPL_H_
