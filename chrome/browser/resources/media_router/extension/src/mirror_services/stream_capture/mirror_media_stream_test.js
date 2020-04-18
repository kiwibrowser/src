// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();

goog.require('mr.PlatformUtils');
goog.require('mr.mirror.CaptureParameters');
goog.require('mr.mirror.CaptureSurfaceType');
goog.require('mr.mirror.Config');
goog.require('mr.mirror.Error');
goog.require('mr.mirror.MirrorMediaStream');
goog.require('mr.mirror.Settings');

describe('mr.mirror.MirrorMediaStream', () => {
  let captureParams;
  let instance;
  let mediaStream;
  let mirrorSettings;

  beforeEach(() => {
    chrome.runtime.lastError = null;
    chrome.tabCapture =
        jasmine.createSpyObj('tabCapture', ['capture', 'captureOffscreenTab']);
    chrome.desktopCapture = jasmine.createSpyObj(
        'desktopCapture', ['chooseDesktopMedia', 'cancelChooseDesktopMedia']);
    spyOn(navigator.mediaDevices, 'getUserMedia');
    spyOn(mr.PlatformUtils, 'getCurrentOS');

    mediaStream = jasmine.createSpyObj(
        'mediaStream', ['getAudioTracks', 'getVideoTracks', 'getTracks']);
    mirrorSettings = new mr.mirror.Settings();
    captureParams = new mr.mirror.CaptureParameters(
        mr.mirror.CaptureSurfaceType.DESKTOP, mirrorSettings);
    instance = new mr.mirror.MirrorMediaStream(captureParams);
    jasmine.clock().install();
  });

  afterEach(() => {
    jasmine.clock().uninstall();
  });

  it('accessor for capture params returns initial value', () => {
    expect(instance.getCaptureParams()).toBe(captureParams);
  });

  describe('when capturing a tab', () => {
    beforeEach(() => {
      captureParams.captureSurface = mr.mirror.CaptureSurfaceType.TAB;
    });

    it('stores the stream upon successful capture', (done) => {
      mediaStream.getVideoTracks.and.returnValue([{}]);
      mediaStream.getAudioTracks.and.returnValue([{}]);
      mediaStream.getTracks.and.returnValue([{}]);
      chrome.tabCapture.capture.and.callFake((constraints, callback) => {
        callback(mediaStream);
      });

      instance.start()
          .then(() => {
            expect(chrome.tabCapture.capture).toHaveBeenCalled();
            expect(instance.getMediaStream()).toBe(mediaStream);
            done();
          })
          .catch(fail);
    });

    it('rejects with an error upon empty stream with error message', (done) => {
      chrome.runtime.lastError = {message: 'expected-message'};
      chrome.tabCapture.capture.and.callFake((constraints, callback) => {
        callback();
      });

      instance.start().catch((err) => {
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason).toBe(mr.MirrorAnalytics.CapturingFailure.TAB_FAIL);
        expect(err.message).toBe('expected-message');
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });

    it('rejects with an error upon empty stream with empty message', (done) => {
      chrome.tabCapture.capture.and.callFake((constraints, callback) => {
        callback();
      });

      instance.start().catch((err) => {
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure
                      .CAPTURE_TAB_FAIL_EMPTY_STREAM);
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });

    it('rejects with an error upon timeout', (done) => {
      instance.start().catch((err) => {
        expect(window.chrome.tabCapture.capture).toHaveBeenCalled();
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure.CAPTURE_TAB_TIMEOUT);
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
      jasmine.clock().tick(5001);
    });
  });

  describe('when capturing an off-screen tab', () => {
    beforeEach(() => {
      captureParams.offscreenTabUrl = 'offscreen-tab-url';
      captureParams.presentationId = 'offscreen-tab-presentation-id';
      captureParams.captureSurface = mr.mirror.CaptureSurfaceType.OFFSCREEN_TAB;
    });

    it('stores the stream upon successful capture', (done) => {
      mediaStream.getVideoTracks.and.returnValue([{}]);
      mediaStream.getAudioTracks.and.returnValue([{}]);
      mediaStream.getTracks.and.returnValue([{}]);
      chrome.tabCapture.captureOffscreenTab.and.callFake(
          (tabUrl, constraints, callback) => {
            expect(tabUrl).toBe('offscreen-tab-url');
            callback(mediaStream);
          });

      instance.start()
          .then(() => {
            expect(window.chrome.tabCapture.captureOffscreenTab)
                .toHaveBeenCalled();
            expect(instance.getMediaStream()).toBe(mediaStream);
            done();
          })
          .catch(fail);
    });

    it('rejects with an error upon empty stream with error message', (done) => {
      chrome.runtime.lastError = {message: 'expected-message'};
      chrome.tabCapture.captureOffscreenTab.and.callFake(
          (tabUrl, constraints, callback) => callback());

      instance.start().catch((err) => {
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason).toBe(mr.MirrorAnalytics.CapturingFailure.TAB_FAIL);
        expect(err.message).toBe('expected-message');
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });

    it('rejects with an error upon empty stream with empty message', (done) => {
      chrome.tabCapture.captureOffscreenTab.and.callFake(
          (tabUrl, constraints, callback) => callback());

      instance.start().catch((err) => {
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure
                      .CAPTURE_TAB_FAIL_EMPTY_STREAM);
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });
  });

  describe('when capturing the desktop', () => {
    const supportsAudioCapture =
        mr.mirror.Config.isDesktopAudioCaptureAvailable;

    beforeEach(() => {
      captureParams.captureSurface = mr.mirror.CaptureSurfaceType.DESKTOP;
    });

    afterEach(() => {
      mr.mirror.Config.isDesktopAudioCaptureAvailable = supportsAudioCapture;
    });

    it('stores the audio and video streams upon successful capture', (done) => {
      mediaStream.getVideoTracks.and.returnValue([{}]);
      mediaStream.getAudioTracks.and.returnValue([{}]);
      mediaStream.getTracks.and.returnValue([{}]);

      chrome.desktopCapture.chooseDesktopMedia.and.callFake(
          (config, callback) => {
            callback('source-id');
          });
      navigator.mediaDevices.getUserMedia.and.callFake(
          constraints => Promise.resolve(mediaStream));

      instance.start()
          .then(() => {
            expect(chrome.desktopCapture.chooseDesktopMedia).toHaveBeenCalled();
            expect(navigator.mediaDevices.getUserMedia).toHaveBeenCalled();
            expect(instance.getMediaStream()).toBe(mediaStream);
            done();
          })
          .catch(fail);
    });

    it('stores the audio stream upon successful capture', (done) => {
      mr.mirror.Config.isDesktopAudioCaptureAvailable = true;
      const audioOnlyMirrorSettings = new mr.mirror.Settings();
      audioOnlyMirrorSettings.shouldCaptureVideo = false;
      const audioOnlyCaptureParams = new mr.mirror.CaptureParameters(
          mr.mirror.CaptureSurfaceType.DESKTOP, audioOnlyMirrorSettings);
      const audioOnlyInstance =
          new mr.mirror.MirrorMediaStream(audioOnlyCaptureParams);

      mediaStream.getVideoTracks.and.returnValue([{}]);
      mediaStream.getAudioTracks.and.returnValue([{}]);
      mediaStream.getTracks.and.returnValue([{}]);

      navigator.mediaDevices.getUserMedia.and.callFake(
          constraints => Promise.resolve(mediaStream));

      audioOnlyInstance.start()
          .then(() => {
            expect(chrome.desktopCapture.chooseDesktopMedia)
                .not.toHaveBeenCalled();
            expect(navigator.mediaDevices.getUserMedia).toHaveBeenCalled();
            expect(audioOnlyInstance.getMediaStream()).toBe(mediaStream);
            done();
          })
          .catch(fail);
    });

    it('allows choosing only screen, audio for non-linux platforms', (done) => {
      mr.PlatformUtils.getCurrentOS.and.returnValue(
          mr.PlatformUtils.OS.WINDOWS);
      chrome.desktopCapture.chooseDesktopMedia.and.callFake(
          (config, callback) => {
            expect(config).toContain('screen');
            expect(config).toContain('audio');
            expect(config).not.toContain('window');
            done();
          });
      instance.start();
    });

    it('allows choosing screen, audio, window for linux platforms', (done) => {
      mr.PlatformUtils.getCurrentOS.and.returnValue(mr.PlatformUtils.OS.LINUX);
      chrome.desktopCapture.chooseDesktopMedia.and.callFake(
          (config, callback) => {
            expect(config).toContain('screen');
            expect(config).toContain('audio');
            expect(config).toContain('window');
            done();
          });
      instance.start();
    });

    it('rejects with an error upon timeout in desktop chooser', (done) => {
      chrome.desktopCapture.chooseDesktopMedia.and.returnValue('expected-id');

      instance.start().catch((err) => {
        expect(chrome.desktopCapture.chooseDesktopMedia).toHaveBeenCalled();
        expect(chrome.desktopCapture.cancelChooseDesktopMedia)
            .toHaveBeenCalledWith('expected-id');
        expect(navigator.mediaDevices.getUserMedia).not.toHaveBeenCalled();
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure
                      .CAPTURE_DESKTOP_FAIL_ERROR_TIMEOUT);
        expect(err.message).toBe('timeout');
        expect(instance.getMediaStream()).toBe(null);
        done();
      });

      jasmine.clock().tick(60001);
    });

    it('rejects with an error when user cancels desktop picker', (done) => {
      chrome.desktopCapture.chooseDesktopMedia.and.callFake(
          (config, callback) => {
            callback(/* no source id */);
          });

      instance.start().catch((err) => {
        expect(chrome.desktopCapture.chooseDesktopMedia).toHaveBeenCalled();
        expect(navigator.mediaDevices.getUserMedia).not.toHaveBeenCalled();
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure
                      .CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL);
        expect(err.message).toMatch(/cancelled/i);
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });

    it('rejects with an error upon getUserMedia error', (done) => {
      chrome.desktopCapture.chooseDesktopMedia.and.callFake(
          (config, callback) => {
            callback('source-id');
          });

      navigator.mediaDevices.getUserMedia.and.callFake(
          constraints => Promise.reject(
              new DOMException('expected-message', 'SecurityError')));

      instance.start().catch((err) => {
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure.DESKTOP_FAIL);
        expect(err.message).toMatch(/\bSecurityError\b/);
        expect(err.message).toMatch(/\bexpected-message\b/);
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });

    it('rejects with an cancelled error upon NotAllowedError', (done) => {
      chrome.desktopCapture.chooseDesktopMedia.and.callFake(
          (config, callback) => {
            callback('source-id');
          });
      navigator.mediaDevices.getUserMedia.and.callFake(
          constraints => Promise.reject(
              new DOMException('expected-message', 'NotAllowedError')));

      instance.start().catch((err) => {
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure
                      .CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL);
        expect(err.message).toMatch(/\bNotAllowedError\b/);
        expect(err.message).toMatch(/\bexpected-message\b/);
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });

    it('rejects with an error upon empty stream object', (done) => {
      chrome.desktopCapture.chooseDesktopMedia.and.callFake(
          (config, callback) => {
            callback('source-id');
          });
      navigator.mediaDevices.getUserMedia.and.callFake(
          constraints => Promise.resolve(null));

      instance.start().catch((err) => {
        expect(err instanceof mr.mirror.Error).toBe(true);
        expect(err.reason)
            .toBe(mr.MirrorAnalytics.CapturingFailure.DESKTOP_FAIL);
        expect(instance.getMediaStream()).toBe(null);
        done();
      });
    });
  });

  describe('when stopping a stream', () => {
    let startPromise;
    let track;

    beforeEach(() => {
      track = jasmine.createSpyObj('track', ['stop']);
      mediaStream.getVideoTracks.and.returnValue([{}]);
      mediaStream.getAudioTracks.and.returnValue([{}]);
      mediaStream.getTracks.and.returnValue([track]);
      captureParams.captureSurface = mr.mirror.CaptureSurfaceType.TAB;
      chrome.tabCapture.capture.and.callFake((constraints, callback) => {
        callback(mediaStream);
      });

      startPromise = instance.start();
    });

    it('calls stop() on the tracks', (done) => {
      startPromise
          .then(() => {
            instance.stop();
            expect(track.stop).toHaveBeenCalled();
            expect(track.onended).toBe(null);
            expect(instance.getMediaStream()).toBe(null);
            done();
          })
          .catch(fail);
    });

    it('automatically stops when track ends', (done) => {
      startPromise
          .then(() => {
            track.onended();
            expect(track.stop).toHaveBeenCalled();
            expect(track.onended).toBe(null);
            expect(instance.getMediaStream()).toBe(null);
            done();
          })
          .catch(fail);
    });

    it('calls the onStreamEnded callback if it exists', (done) => {
      const onStreamEndedSpy = jasmine.createSpy('onStreamEnded');
      instance.setOnStreamEnded(onStreamEndedSpy);

      startPromise
          .then(() => {
            instance.stop();
            expect(onStreamEndedSpy).toHaveBeenCalled();
            done();
          })
          .catch(fail);
    });
  });
});
