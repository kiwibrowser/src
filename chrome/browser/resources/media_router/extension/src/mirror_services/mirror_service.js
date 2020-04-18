// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Service supporting tab and screen mirroring.
 *
 * This object is a singleton that controls all capture MediaStreams and all
 * instances of MirrorSession.
 *

 */

goog.provide('mr.mirror.Service');

goog.require('mr.Assertions');
goog.require('mr.CancellablePromise');
goog.require('mr.EventAnalytics');
goog.require('mr.Issue');
goog.require('mr.IssueSeverity');
goog.require('mr.Logger');
goog.require('mr.MediaSourceUtils');
goog.require('mr.MirrorAnalytics');
goog.require('mr.Module');
goog.require('mr.mirror.CaptureParameters');
goog.require('mr.mirror.CaptureSurfaceType');
goog.require('mr.mirror.Error');
goog.require('mr.mirror.Messages');
goog.require('mr.mirror.MirrorMediaStream');

mr.mirror.Service = class extends mr.Module {
  /**
   * @param {!mr.mirror.ServiceName} serviceName
   * @param {mr.ProviderManagerMirrorServiceCallbacks=} mirrorServiceCallbacks
   */
  constructor(serviceName, mirrorServiceCallbacks) {
    super();

    /** @private @const {!mr.mirror.ServiceName} */
    this.serviceName_ = serviceName;

    /** @private {?mr.ProviderManagerMirrorServiceCallbacks} */
    this.mirrorServiceCallbacks_ = mirrorServiceCallbacks || null;

    /** @protected {?mr.mirror.Session} */
    this.currentSession = null;

    /** @private {?mr.mirror.MirrorMediaStream} */
    this.currentMediaStream_ = null;

    /** @protected @const */
    this.logger = mr.Logger.getInstance('mr.mirror.Service.' + serviceName);

    /** @private @const */
    this.onTabUpdated_ = this.handleTabUpdate_.bind(this);

    /** @private {boolean} */
    this.initialized_ = false;
  }

  /**
   * Initializes the service. Sets callbacks to provider manager. No-ops if
   * already initialized.
   * @param {!mr.ProviderManagerMirrorServiceCallbacks} mirrorServiceCallbacks
   *     Callbacks to provider manager.
   */
  initialize(mirrorServiceCallbacks) {
    if (this.initialized_) {
      return;
    }
    this.mirrorServiceCallbacks_ = mirrorServiceCallbacks;
    this.initialized_ = true;
    this.doInitialize();
  }

  /**
   * Called during initialization to perform service-specific initialization.
   * @protected
   */
  doInitialize() {}

  /**
   * @return {mr.mirror.ServiceName}
   */
  getName() {
    return this.serviceName_;
  }

  /**
   * @param {!mr.Route} route
   * @param {string} sourceUrn
   * @param {!mr.mirror.Settings} mirrorSettings
   * @param {string=} opt_presentationId
   * @param {(function(!mr.Route): !mr.CancellablePromise)=}
   *     opt_streamStartedCallback Callback to invoke after stream capture
   *         succeeded and before the mirror session is created. The callback
   *         may update the route.
   * @return {!mr.CancellablePromise<!mr.Route>} A promise fulfilled
   *    when mirroring has started successfully.
   */
  startMirroring(
      route, sourceUrn, mirrorSettings, opt_presentationId,
      opt_streamStartedCallback) {
    this.logger.info('Start mirroring on route ' + route.id);
    if (!this.initialized_) {
      return mr.CancellablePromise.reject(Error('Not initialized'));
    }
    const promise = new Promise((resolve, reject) => {
      this.stopCurrentMirroring()
          .then(() => {
            const captureParams = mr.mirror.Service.createCaptureParameters_(
                sourceUrn, mirrorSettings, opt_presentationId);
            return new mr.mirror.MirrorMediaStream(captureParams).start();
          })
          .then(stream => {
            if (this.currentMediaStream_) {
              stream.stop();
              throw new mr.mirror.Error('Cannot start multiple streams');
            }
            this.currentMediaStream_ = stream;
            this.currentMediaStream_.setOnStreamEnded(this.cleanup_.bind(this));
            if (opt_streamStartedCallback) {
              // Yuck.  Converting a CancellablePromise to a plain Promise
              // prevents cancellation from propagating correctly.
              return opt_streamStartedCallback(route).promise;
            }
            return route;
          })
          .then(updatedRoute => {
            if (this.currentSession) {
              throw new mr.mirror.Error('Cannot start multiple sessions');
            }
            if (!this.currentMediaStream_) {
              throw new mr.mirror.Error(
                  'Media stream ended before session could start.');
            }
            this.currentSession =
                this.createMirrorSession(mirrorSettings, updatedRoute);
            updatedRoute.mirrorInitData.activity =
                this.currentSession.getActivity();
            this.currentSession.setOnActivityUpdate(
                this.mirrorServiceCallbacks_.handleMirrorActivityUpdate.bind(
                    this.mirrorServiceCallbacks_));
            return this.currentSession.start(/** @type {!MediaStream} */ (
                this.currentMediaStream_.getMediaStream()));
          })
          .then(() => {
            if (mr.MediaSourceUtils.isTabMirrorSource(sourceUrn) &&
                !chrome.tabs.onUpdated.hasListener(this.onTabUpdated_)) {
              chrome.tabs.onUpdated.addListener(this.onTabUpdated_);
            }
            return this.postProcessMirroring_(route, sourceUrn, mirrorSettings);
          })
          .then(() => {
            resolve(route);
          })
          .catch(err => {

            this.onStartError_(/** @type {!Error} */ (err));
            return this.cleanup_().then(() => {
              reject(err);
            });
          });
    });
    return mr.CancellablePromise.forPromise(promise);
  }

  /**
   * @param {!mr.Route} route
   * @param {string} sourceUrn
   * @param {!mr.mirror.Settings} mirrorSettings
   * @param {string=} opt_presentationId
   * @param {number=} opt_tabId
   * @param {(function(!mr.Route): !mr.CancellablePromise<!mr.Route>)=}
   *     opt_streamStartedCallback Callback to invoke after stream capture
   *     succeeded and before the mirror session is created. The callback may
   *     update the route.
   * @return {!mr.CancellablePromise<!mr.Route>} A promise fulfilled
   *    when mirroring has started successfully.
   */
  updateMirroring(
      route, sourceUrn, mirrorSettings, opt_presentationId, opt_tabId,
      opt_streamStartedCallback) {
    this.logger.info('Update mirroring on route ' + route.id);
    if (!this.initialized_) {
      return mr.CancellablePromise.reject(Error('Not initialized'));
    }
    return mr.CancellablePromise.forPromise(this.doUpdateMirroring_(
        route, sourceUrn, mirrorSettings, opt_presentationId,
        opt_streamStartedCallback));
  }

  /**
   * A helper method for updateMirroring.
   * @param {!mr.Route} route
   * @param {string} sourceUrn
   * @param {!mr.mirror.Settings} mirrorSettings
   * @param {string=} opt_presentationId
   * @param {(function(!mr.Route): !mr.CancellablePromise<!mr.Route>)=}
   *     opt_streamStartedCallback Callback to invoke after stream capture
   *     succeeded and before the mirror session is created. The callback may
   *     update the route.
   * @return {!Promise<!mr.Route>} A promise fulfilled
   *    when mirroring has started successfully.
   * @private
   */
  doUpdateMirroring_(
      route, sourceUrn, mirrorSettings, opt_presentationId,
      opt_streamStartedCallback) {
    if (!this.currentSession) {
      return Promise.reject(new mr.mirror.Error(
          'No session to update streams on',
          mr.MirrorAnalytics.CapturingFailure.TAB_FAIL));
    }
    if (!this.currentSession.supportsUpdateStream()) {
      return Promise.reject(new mr.mirror.Error(
          'Session does not support updating stream',
          mr.MirrorAnalytics.CapturingFailure.TAB_FAIL));
    }

    let streamSwapped = false;
    return new Promise((resolve, reject) => {
      const captureParams = mr.mirror.Service.createCaptureParameters_(
          sourceUrn, mirrorSettings, opt_presentationId);
      new mr.mirror.MirrorMediaStream(captureParams)
          .start()
          .then(stream => {
            if (this.currentMediaStream_) {
              this.currentMediaStream_.setOnStreamEnded(null);
              this.currentMediaStream_.stop();
              this.recordStreamEnded();
            }
            this.currentMediaStream_ = stream;
            this.currentMediaStream_.setOnStreamEnded(this.cleanup_.bind(this));
            streamSwapped = true;

            if (opt_streamStartedCallback) {
              return opt_streamStartedCallback(route).promise;
            }
            return route;
          })
          .then(_ => {
            if (!this.currentSession) {
              throw new mr.mirror.Error('Session ended while updating stream');
            }
            if (!this.currentMediaStream_) {
              throw new mr.mirror.Error(
                  'Media stream ended before session could be updated.');
            }
            return this.currentSession.updateStream(
                /** @type {!MediaStream} */ (
                    this.currentMediaStream_.getMediaStream()));
          })
          .then(this.postProcessMirroring_.bind(
              this, route, sourceUrn, mirrorSettings))
          .then(() => resolve(route))
          .catch(err => {
            this.onStartError_(/** @type {!Error} */ (err));
            if (streamSwapped) {
              return this.cleanup_().then(() => {
                reject(err);
              });
            } else {
              reject(err);
            }
          });
    });
  }

  /**
   * @param {!mr.Route} route
   * @param {string} sourceUrn
   * @param {!mr.mirror.Settings} mirrorSettings
   * @return {!Promise<void>} Resolves when done.
   * @private
   */
  postProcessMirroring_(route, sourceUrn, mirrorSettings) {
    return new Promise((resolve, reject) => {
      if (!this.currentSession) {
        reject(new mr.mirror.Error(
            'Session gone before executing post-startup steps',
            mr.MirrorAnalytics.CapturingFailure.TAB_FAIL));
        return;
      }
      if (mr.MediaSourceUtils.isTabMirrorSource(sourceUrn)) {
        this.currentSession.setTabId(
            /** @type {number} */ (route.mirrorInitData.tabId));
        this.recordTabMirrorStartSuccess();
      } else if (mr.MediaSourceUtils.isPresentationSource(sourceUrn)) {
        this.currentSession.setTabId(
            /** @type {number} */ (route.mirrorInitData.tabId));
        this.recordOffscreenTabMirrorStartSuccess();
      } else {
        this.recordDesktopMirrorStartSuccess();
      }
      this.checkCaptureIssues_(
          mirrorSettings,
          /** @type {!mr.mirror.MirrorMediaStream} */
          (this.currentMediaStream_));
      this.currentSession.onActivityUpdated();
      resolve();
    });
  }

  /**
   * @param {!Error|!mr.mirror.Error} error The error that occurred when
   * starting mirroring.
   * @private
   */
  onStartError_(error) {
    error.reason = (error.reason != null) ?
        error.reason :
        mr.MirrorAnalytics.CapturingFailure.UNKNOWN;

    this.logger.error(
        `Failed to start mirroring: ${error.message}` +
        `,  reason = ${error.reason}: ${error.stack}`);
    this.recordMirrorStartFailure(error.reason);
  }

  /**
   * @return {!Promise<boolean>} Fulfilled with true if there was a session
   *     and it was stopped, and with false if there is no session to stop.
   */
  stopCurrentMirroring() {
    if (!this.initialized_) {
      return Promise.reject('Not initialized');
    }
    return this.cleanup_().then(hadSession => {
      if (hadSession) this.recordMirrorSessionEnded();

      return hadSession;
    });
  }

  /**
   * @return {!Promise<boolean>} Fulfilled with true if there was a session
   *     and it was stopped. This promise never rejects.
   * @private
   */
  cleanup_() {
    // No-op if the listener was already removed.
    chrome.tabs.onUpdated.removeListener(this.onTabUpdated_);

    const streamToCleanUp = this.currentMediaStream_;
    this.currentMediaStream_ = null;
    if (streamToCleanUp) {
      // Clear the "on ended" callback to prevent recursive calls to this method
      // while the session and MediaStream are being torn down (below).
      streamToCleanUp.setOnStreamEnded(null);
    }

    const sessionToCleanUp = this.currentSession;
    this.currentSession = null;

    // Create a promise chain to execute the stopping of the session, invoke the
    // before/after stop callbacks, and thereafter stop the MediaStream. The
    // MediaStream is stopped after the session to avoid any extra churn in the
    // session shutdown process. All of this is conditional on whether a session
    // and/or MediaStream was ever started since cleanup_() is also used as an
    // all-purpose failed-start recovery handler.
    let cleanupPromise;
    if (sessionToCleanUp) {
      cleanupPromise =
          this.beforeCleanUpSession(sessionToCleanUp)
              .catch(
                  err =>
                      this.logger.error('Error in before-cleanup steps', err))
              .then(() => sessionToCleanUp.stop())
              .catch(err => this.logger.error('Error stopping session', err))
              .then(() => {
                this.mirrorServiceCallbacks_.onMirrorSessionEnded(
                    sessionToCleanUp.getRoute().id);
              })
              .catch(err => this.logger.error('Error in ended callbacks', err))
              .then(() => true);
    } else {
      cleanupPromise = Promise.resolve(false);
    }
    if (streamToCleanUp) {
      cleanupPromise = cleanupPromise.then(hadSession => {
        streamToCleanUp.stop();
        this.recordStreamEnded();
        return hadSession;
      });
    }

    return cleanupPromise;
  }

  /**
   * @param {!mr.mirror.Session} session the session about to be cleaned up.
   * @return {!Promise<void>} Fulfilled when session has been cleaned up.
   * @protected
   */
  beforeCleanUpSession(session) {
    return Promise.resolve();
  }

  /**
   * Handles tab updated event.
   *
   * @param {number} tabId the ID of the tab.
   * @param {!TabChangeInfo} changeInfo The changes to the state of the tab.
   * @param {!Tab} tab The tab.
   * @private
   */
  handleTabUpdate_(tabId, changeInfo, tab) {
    mr.EventAnalytics.recordEvent(mr.EventAnalytics.Event.TABS_ON_UPDATED);
    this.currentSession &&
        this.currentSession.onTabUpdated(tabId, changeInfo, tab);
  }

  /**
   * Creates a new mr.mirror.CaptureParameters from the given inputs.
   *
   * @param {string} sourceUrn
   * @param {!mr.mirror.Settings} mirrorSettings
   * @param {string=} opt_presentationId
   * @return {!mr.mirror.CaptureParameters}
   * @private
   */
  static createCaptureParameters_(
      sourceUrn, mirrorSettings, opt_presentationId) {
    if (mr.MediaSourceUtils.isTabMirrorSource(sourceUrn)) {
      return new mr.mirror.CaptureParameters(
          mr.mirror.CaptureSurfaceType.TAB, mirrorSettings);
    } else if (mr.MediaSourceUtils.isDesktopMirrorSource(sourceUrn)) {
      return new mr.mirror.CaptureParameters(
          mr.mirror.CaptureSurfaceType.DESKTOP, mirrorSettings);
    } else if (mr.MediaSourceUtils.isPresentationSource(sourceUrn)) {
      if (!opt_presentationId) {
        throw new mr.mirror.Error('Missing offscreen tab presentation id');
      }
      return new mr.mirror.CaptureParameters(
          mr.mirror.CaptureSurfaceType.OFFSCREEN_TAB, mirrorSettings, sourceUrn,
          /** @type {!string} */ (opt_presentationId));
    } else {
      throw new mr.mirror.Error(
          'Source URN does not suggest a known capture type.');
    }
  }

  /**
   * Checks for any capture issues after mirroring has started successfully.
   *
   * @param {!mr.mirror.Settings} settings The requested settings.
   * @param {!mr.mirror.MirrorMediaStream} mediaStream The captured stream.
   * @private
   */
  checkCaptureIssues_(settings, mediaStream) {
    if (settings.shouldCaptureAudio && mediaStream.getMediaStream() &&
        !mediaStream.getMediaStream().getAudioTracks().length) {
      this.mirrorServiceCallbacks_.sendIssue(new mr.Issue(
          mr.mirror.Messages.MSG_MR_MIRROR_NO_AUDIO_CAPTURED,
          mr.IssueSeverity.NOTIFICATION));
    }
  }

  /**
   * @return {?mr.mirror.Session}
   * @deprecated Use of this getter is dangerous, since mr.mirror.Service could
   *     be in the middle of a sequence of asynchronous steps to start up or
   *     shut down the current session.
   */
  getCurrentSession() {
    return this.currentSession;
  }

  /**
   * @param {!mr.mirror.Settings} mirrorSettings
   * @param {!mr.Route} route
   * @return {!mr.mirror.Session}
   */
  createMirrorSession(mirrorSettings, route) {}
  /**
   * Records that Tab Mirroring successfully started.
   */
  recordTabMirrorStartSuccess() {}
  /**
   * Records that Desktop Mirroring successfully started.
   */
  recordDesktopMirrorStartSuccess() {}
  /**
   * Records that Offscreen Tab (1UA mode) Mirroring successfully started.
   */
  recordOffscreenTabMirrorStartSuccess() {}
  /**
   * Records that the session has ended.
   */
  recordMirrorSessionEnded() {}
  /**
   * Records that a Mirroring session failed to start.
   * @param {mr.MirrorAnalytics.CapturingFailure} reason
   *     The reason for the failure.
   */
  recordMirrorStartFailure(reason) {}
  /**
   * Records that a Mirroring stream ended.
   */
  recordStreamEnded() {}
  /**
   * Asynchronously uploads logs for the most recent mirroring session.
   * @param {string} feedbackId ID of feedback requesting log upload.
   * @return {!Promise<string|undefined>} Resolved with an identifier (e.g.,
   *     URL) of the log that will be uploaded (which will be undefined if there
   *     is no such identifier), or rejected if upload failed.
   */
  requestLogUpload(feedbackId) {
    return mr.Assertions.rejectNotImplemented();
  }
  /**
   * See documentation in interface_data/mojo.js.
   * @param {string} routeId
   * @param {!mojo.InterfaceRequest} controllerRequest
   * @param {!mojo.MediaStatusObserverPtr} observer
   * @return {!Promise<void>}
   */
  createMediaRouteController(routeId, controllerRequest, observer) {
    return mr.Assertions.rejectNotImplemented();
  }
};
