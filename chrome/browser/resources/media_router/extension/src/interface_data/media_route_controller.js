goog.module('mr.MediaRouteController');
goog.module.declareLegacyNamespace();

const Logger = goog.require('mr.Logger');


/**
 * Controls a media that is being routed. This base class defines the same set
 * of APIs as the MediaController Mojo interface defined in
 * media_controller.mojom in Chromium, and contains a MojoBinding to the
 * browser (so that MediaController commands from the browser are routed here).
 * MRPs may extend this class with additional commands.
 */
const MediaRouteController = class {
  /**
   * @param {string} name Name of the controller (e.g. HangoutsRouteController).
   * @param {!mojo.InterfaceRequest} controllerRequest The Mojo interface
   *     request to be bound to this controller.
   * @param {!mojo.MediaStatusObserverPtr} observer The Mojo pointer to the
   *     MediaStatusObserver.
   */
  constructor(name, controllerRequest, observer) {
    /** @protected @const {!Logger} */
    this.logger = Logger.getInstance('mr.MediaRouteController.' + name);

    /**
     * The binding used to maintain a Mojo connection with the browser process.
     * @private {?mojo.Binding}
     */
    this.binding_ =
        new mojo.Binding(mojo.MediaController, this, controllerRequest);

    /**
     * The observer to send media status updates to.
     * @private {?mojo.MediaStatusObserverPtr}
     */
    this.observer_ = observer;

    /**
     * @protected @const {!mojo.MediaStatus}
     */
    this.currentMediaStatus = new mojo.MediaStatus({
      title: '',
      description: '',
      duration: new mojo.TimeDelta({microseconds: 0}),
      current_time: new mojo.TimeDelta({microseconds: 0})
    });

    /**
     * @private {boolean}
     */
    this.disposed_ = false;

    this.binding_.setConnectionErrorHandler(
        this.onMojoConnectionError.bind(this));
    this.observer_.ptr.setConnectionErrorHandler(
        this.onMojoConnectionError.bind(this));
  }

  /**
   * Drops the reference to the binding.
   * @protected
   */
  onMojoConnectionError() {
    this.dispose();
    this.onControllerInvalidated();
  }

  /**
   * Closes the connection in the binding and the observer. There will be no
   * more incoming or outgoing calls after this.
   * @final
   */
  dispose() {
    if (this.disposed_) {
      return;
    }
    this.disposed_ = true;
    this.disposeInternal();
    if (this.binding_) {
      this.binding_.close();
      this.binding_ = null;
    }
    if (this.observer_) {
      this.observer_.ptr.reset();
      this.observer_ = null;
    }
  }

  /**
   * Notifies the observer of media status update, if it exists.
   * @protected
   */
  notifyObserver() {
    if (this.observer_) {
      this.observer_.onMediaStatusUpdated(this.currentMediaStatus);
    }
  }

  /**
   * Performs additional cleanup when the controller is being disposed.
   * @protected
   */
  disposeInternal() {}

  /**
   * Performs final cleanup after the controller is invalidated by a Mojo
   * connection error. By the time this is called, dispose() has already
   * happened. This gives an opportunity for clients to drop their references
   * to the controller.
   * @protected
   */
  onControllerInvalidated() {}

  // The following are methods for handling incoming media commands. The method
  // names must match the ones defined in media_controller.mojom in Chromium
  // and be exported.

  /**
   * Plays the media.
   * @export
   */
  play() {}

  /**
   * Pauses the media.
   * @export
   */
  pause() {}

  /**
   * Mutes or unmutes the media.
   * @param {boolean} mute
   * @export
   */
  setMute(mute) {}

  /**
   * Sets the volume on the media. The given volume must be between 0 and 1.
   * @param {number} volume
   * @export
   */
  setVolume(volume) {}

  /**
   * Seeks to the given time. The given time must be non-negative and less than
   * or equal to the duration of the media.
   * @param {!mojo.TimeDelta} time
   * @export
   */
  seek(time) {}

  /**
   * Binds the given request to an implementation that accepts Hangouts-specific
   * commands.
   * @param {!mojo.InterfaceRequest} hangoutsControllerRequest Interface request
   *     for Hangouts controller.
   * @export
   */
  connectHangoutsMediaRouteController(hangoutsControllerRequest) {
    hangoutsControllerRequest.close();
    throw new Error('Not implemented');
  }
};

exports = MediaRouteController;
