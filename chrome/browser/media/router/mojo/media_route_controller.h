// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTE_CONTROLLER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTE_CONTROLLER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/common/media_router/media_route.h"
#include "chrome/common/media_router/mojo/media_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

class PrefService;

namespace content {
class BrowserContext;
}

namespace media_router {

class EventPageRequestManager;
class MediaRouter;

// A controller for a MediaRoute. Notifies its observers whenever there is a
// change in the route's MediaStatus. Forwards commands for controlling the
// route to a controller in the Media Router component extension if the
// extension is ready, and queues commands with EventPageRequestManager
// otherwise.
//
// It is owned by its observers, each of which holds a scoped_refptr to it. All
// the classes that hold a scoped_refptr must inherit from the Observer class.
// An observer should be instantiated with a scoped_refptr obtained through
// MediaRouter::GetRouteController().
//
// |MediaRouteController::InitMojoInterfaces()| must be invoked before commands
// can be issued and MediaStatus status updates are received. This method is
// called in |MediaRouter::GetRouteController()| when the controller is
// initially created. Note that the Mojo bindings / interface ptrs will be reset
// when the event page is suspended; when the event page is woken up,
// |InitMojoInterfaces()| will be invoked again to re-initialize the Mojo
// interfaces / bindings.
//
// A MediaRouteController instance is destroyed when all its observers dispose
// their references to it. When the associated route is destroyed, Invalidate()
// is called to make the controller's observers dispose their refptrs.
class MediaRouteController
    : public mojom::MediaStatusObserver,
      public base::RefCounted<MediaRouteController>,
      public base::SupportsWeakPtr<MediaRouteController> {
 public:
  // Observes MediaRouteController for MediaStatus updates. The ownership of a
  // MediaRouteController is shared by its observers.
  class Observer {
   public:
    // Adds itself as an observer to |controller|.
    explicit Observer(scoped_refptr<MediaRouteController> controller);

    // Removes itself as an observer if |controller_| is still valid.
    virtual ~Observer();

    virtual void OnMediaStatusUpdated(const MediaStatus& status) = 0;

    // Returns a reference to the observed MediaRouteController. The reference
    // should not be stored by any object that does not subclass ::Observer.
    scoped_refptr<MediaRouteController> controller() const {
      return controller_;
    }

   protected:
    scoped_refptr<MediaRouteController> controller_;

   private:
    friend class MediaRouteController;

    // Disposes the reference to the controller.
    void InvalidateController();

    // Called by InvalidateController() after the reference to the controller is
    // disposed. Overridden by subclasses to do custom cleanup.
    virtual void OnControllerInvalidated();

    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  // Value returned by |InitMojoInterfaces()|.
  // The first item is an unbound MediaControllerRequest.
  // The second item is a bound MediaStatusObserverPtr whose binding is owned
  // by |this|.
  using InitMojoResult =
      std::pair<mojom::MediaControllerRequest, mojom::MediaStatusObserverPtr>;

  // Constructs a MediaRouteController that forwards media commands to
  // |mojo_media_controller_|. |media_router_| will be notified when the
  // MediaRouteController is destroyed via DetachRouteController().
  MediaRouteController(const MediaRoute::Id& route_id,
                       content::BrowserContext* context,
                       MediaRouter* router);

  // Initializes the Mojo interfaces/bindings in this MediaRouteController.
  // This should only be called when the Mojo interfaces/bindings are not bound.
  InitMojoResult InitMojoInterfaces();

  virtual RouteControllerType GetType() const;

  // Media controller methods for forwarding commands to a
  // mojom::MediaControllerPtr held in |mojo_media_controller_|.
  virtual void Play();
  virtual void Pause();
  virtual void Seek(base::TimeDelta time);
  virtual void SetMute(bool mute);
  virtual void SetVolume(float volume);

  // mojom::MediaStatusObserver:
  // Notifies |observers_| of a status update.
  void OnMediaStatusUpdated(const MediaStatus& status) override;

  // Notifies |observers_| to dispose their references to the controller. The
  // controller gets destroyed when all the references are disposed.
  // This happens when the route associated with the controller no longer
  // exists.
  // Further operations should not be performed on the controller once it has
  // become invalid, as it will be deleted soon.
  void Invalidate();

  MediaRoute::Id route_id() const { return route_id_; }

  // Returns the latest media status that the controller has been notified with.
  // Returns a nullopt if the controller hasn't been notified yet.
  const base::Optional<MediaStatus>& current_media_status() const {
    return current_media_status_;
  }

 protected:
  ~MediaRouteController() override;

  mojom::MediaControllerPtr& mojo_media_controller() {
    return mojo_media_controller_;
  }

  EventPageRequestManager* request_manager() { return request_manager_; }

  // Called when the connection between |this| and the MediaControllerPtr or
  // the MediaStatusObserver binding is no longer valid. Notifies
  // |media_router_| and |observers_| to dispose their references to |this|.
  virtual void OnMojoConnectionError();

 private:
  friend class base::RefCounted<MediaRouteController>;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Overridable by subclasses to request interfaces for additional commands.
  // Called in |InitMojoConnections()| after |mojo_media_controller_| is bound.
  virtual void InitAdditionalMojoConnections() {}

  // Overridable by subclasses to perform additional cleanup before the main
  // logic in |Invalidate()| executes.
  virtual void InvalidateInternal() {}

  // The ID of the Media Route that |this| controls.
  const MediaRoute::Id route_id_;

  // Handle to the mojom::MediaController that receives media commands.
  mojom::MediaControllerPtr mojo_media_controller_;

  // Request manager responsible for waking the component extension and calling
  // the requests to it.
  EventPageRequestManager* const request_manager_;

  // |media_router_| will be notified when the controller is destroyed and if
  // |Invalidate()| has not been called.
  MediaRouter* const media_router_;

  // The binding to observe the out-of-process provider of status updates.
  mojo::Binding<mojom::MediaStatusObserver> binding_;

  // Observers that are notified of status updates. The observers share the
  // ownership of the controller through scoped_refptr.
  base::ObserverList<Observer> observers_;

  // This becomes false when |Invalidate()| is called on the controller.
  // TODO(imcheng): We need the |is_valid_| bit and make have the dtor depend on
  // it in order to prevent concurrent modifications to MediaRouterMojoImpl's
  // internal mapping while |this| is being deleted due to |Invalidate()|. This
  // behavior coupling is confusing and should be rewritten in a way that does
  // not depend on such subtle behavior.
  bool is_valid_ = true;

  // The latest media status that the controller has been notified with.
  base::Optional<MediaStatus> current_media_status_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouteController);
};

class HangoutsMediaRouteController : public MediaRouteController {
 public:
  // Casts |controller| to a HangoutsMediaRouteController if its
  // RouteControllerType is HANGOUTS. Returns nullptr otherwise.
  static HangoutsMediaRouteController* From(MediaRouteController* controller);

  HangoutsMediaRouteController(const MediaRoute::Id& route_id,
                               content::BrowserContext* context,
                               MediaRouter* router);

  // MediaRouteController
  RouteControllerType GetType() const override;

  void SetLocalPresent(bool local_present);

 protected:
  ~HangoutsMediaRouteController() override;

 private:
  // MediaRouteController
  void InitAdditionalMojoConnections() override;
  void OnMojoConnectionError() override;
  void InvalidateInternal() override;

  mojom::HangoutsMediaRouteControllerPtr mojo_hangouts_controller_;

  DISALLOW_COPY_AND_ASSIGN(HangoutsMediaRouteController);
};

// Controller subclass for Cast streaming mirroring routes. Responsible for:
// (1) updating the media remoting pref according to user input
// (2) augmenting the MediaStatus update sent by the MRP with the value from the
//     media remoting pref.
class MirroringMediaRouteController : public MediaRouteController {
 public:
  // Casts |controller| to a MirroringMediaRouteController if its
  // RouteControllerType is MIRRORING. Returns nullptr otherwise.
  static MirroringMediaRouteController* From(MediaRouteController* controller);

  MirroringMediaRouteController(const MediaRoute::Id& route_id,
                                content::BrowserContext* context,
                                MediaRouter* router);

  // MediaRouteController
  RouteControllerType GetType() const override;
  void OnMediaStatusUpdated(const MediaStatus& status) override;

  // Sets the media remoting pref to |enabled| and notifies the observers.
  // Note that the MRP listens for updates on this pref value and enable/disable
  // media remoting as needed.
  void SetMediaRemotingEnabled(bool enabled);

  bool media_remoting_enabled() const { return media_remoting_enabled_; }

 protected:
  ~MirroringMediaRouteController() override;

 private:
  PrefService* const prefs_;

  // This is initialized from |prefs_| in the constructor and updated in
  // |SetMediaRemotingEnabled()|. This class does not need to listen for pref
  // changes because this is the only place where the media remoting pref value
  // can be modified.
  bool media_remoting_enabled_ = true;
  MediaStatus latest_status_;

  DISALLOW_COPY_AND_ASSIGN(MirroringMediaRouteController);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTE_CONTROLLER_H_
