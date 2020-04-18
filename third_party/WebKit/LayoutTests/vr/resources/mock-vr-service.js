'use strict';

class MockVRDisplay {
  constructor(displayInfo, service) {
    this.displayClient_ = new device.mojom.VRDisplayClientPtr();
    this.displayInfo_ = displayInfo;
    this.service_ = service;
    this.presentation_provider_ = new MockVRPresentationProvider();

    if (service.client_) {
      this.notifyClientOfDisplay();
    }
  }

  requestPresent(submitFrameClient, request, presentOptions) {
    this.presentation_provider_.bind(submitFrameClient, request,
                                     presentOptions);
    // The JavaScript bindings convert c_style_names to camelCase names.
    var options = new device.mojom.VRDisplayFrameTransportOptions();
    options.transportMethod =
        device.mojom.VRDisplayFrameTransportMethod.SUBMIT_AS_MAILBOX_HOLDER;
    options.waitForTransferNotification = true;
    options.waitForRenderNotification = true;
    return Promise.resolve({success: true, transportOptions: options});
  }

  setPose(pose) {
    if (pose == null) {
      this.presentation_provider_.pose_ = null;
    } else {
      this.presentation_provider_.initPose();
      this.presentation_provider_.fillPose(pose);
    }
  }

  getPose() {
    return Promise.resolve({
      pose: this.presentation_provider_.pose_,
    });
  }

  getSubmitFrameCount() {
    return this.presentation_provider_.submit_frame_count_;
  }

  getMissingFrameCount() {
    return this.presentation_provider_.missing_frame_count_;
  }

  forceActivate(reason) {
    this.displayClient_.onActivate(reason);
  }

  notifyClientOfDisplay() {
    let displayPtr = new device.mojom.VRDisplayHostPtr();
    let displayRequest = mojo.makeRequest(displayPtr);
    let displayBinding =
        new mojo.Binding(device.mojom.VRDisplayHost, this, displayRequest);

    let magicWindowPtr = new device.mojom.VRMagicWindowProviderPtr();
    let magicWindowRequest = mojo.makeRequest(magicWindowPtr);
    let magicWindowBinding = new mojo.Binding(
        device.mojom.VRMagicWindowProvider, this, magicWindowRequest);

    let clientRequest = mojo.makeRequest(this.displayClient_);
    this.service_.client_.onDisplayConnected(magicWindowPtr, displayPtr,
        clientRequest, this.displayInfo_);
  }
}

class MockVRPresentationProvider {
  constructor() {
    this.binding_ = new mojo.Binding(device.mojom.VRPresentationProvider, this);
    this.pose_ = null;
    this.submit_frame_count_ = 0;
    this.missing_frame_count_ = 0;
  }

  bind(client, request) {
    this.submitFrameClient_ = client;
    this.binding_.close();
    this.binding_.bind(request);
  }

  submitFrameMissing(frameId, syncToken) {
    this.missing_frame_count_++;
  }

  submitFrame(frameId, mailboxHolder, timeWaited) {
    this.submit_frame_count_++;

    // Trigger the submit completion callbacks here. WARNING: The
    // Javascript-based mojo mocks are *not* re-entrant. It's OK to
    // wait for these notifications on the next frame, but waiting
    // within the current frame would never finish since the incoming
    // calls would be queued until the current execution context finishes.
    this.submitFrameClient_.onSubmitFrameTransferred(true);
    this.submitFrameClient_.onSubmitFrameRendered();
  }

  getVSync() {
    if (this.pose_) {
      this.pose_.poseIndex++;
    }

    // Convert current document time to monotonic time.
    var now = window.performance.now() / 1000.0;
    var diff =
        now - internals.monotonicTimeToZeroBasedDocumentTime(now);
    now += diff;
    now *= 1000000;

    let retval = Promise.resolve({
      pose: this.pose_,
      time: {
        microseconds: now,
      },
      frameId: 0,
      status: device.mojom.VRPresentationProvider.VSyncStatus.SUCCESS,
    });

    return retval;
  }

  initPose() {
    this.pose_ = {
      orientation: null,
      position: null,
      angularVelocity: null,
      linearVelocity: null,
      angularAcceleration: null,
      linearAcceleration: null,
      poseIndex: 0
    };
  }

  fillPose(pose) {
    for (var field in pose) {
      if (this.pose_.hasOwnProperty(field)) {
        this.pose_[field] = pose[field];
      }
    }
  }
}

class MockVRService {
  constructor() {
    this.bindingSet_ = new mojo.BindingSet(device.mojom.VRService);
    this.mockVRDisplays_ = [];

    this.interceptor_ =
        new MojoInterfaceInterceptor(device.mojom.VRService.name);
    this.interceptor_.oninterfacerequest =
        e => this.bindingSet_.addBinding(this, e.handle);
    this.interceptor_.start();
  }

  setVRDisplays(displays) {
    this.mockVRDisplays_ = [];
    for (let i = 0; i < displays.length; i++) {
      displays[i].index = i;
      this.mockVRDisplays_.push(new MockVRDisplay(displays[i], this));
    }
  }

  addVRDisplay(display) {
    if (this.mockVRDisplays_.length) {
      display.index =
          this.mockVRDisplays_[this.mockVRDisplays_.length - 1] + 1;
    } else {
      display.index = 0;
    }
    this.mockVRDisplays_.push(new MockVRDisplay(display, this));
  }

  setClient(client) {
    this.client_ = client;
    for (let i = 0; i < this.mockVRDisplays_.length; i++) {
      this.mockVRDisplays_[i].notifyClientOfDisplay();
    }

    let device_number = this.mockVRDisplays_.length;
    return Promise.resolve({numberOfConnectedDevices: device_number});
  }
}

let mockVRService = new MockVRService(mojo.frameInterfaces);

function vr_test(func, vrDisplays, name, properties) {
  mockVRService.setVRDisplays(vrDisplays);
  let t = async_test(name, properties);
  func(t, mockVRService);
}

// TODO(offenwanger) Remove this when we switch over to promise tests.
function xr_session_test(func, vrDevice, sessionOptions, name, properties) {
  mockVRService.setVRDisplays([vrDevice]);
  let t = async_test(name, properties);

  navigator.xr.requestDevice().then(
      (device) => {
        // Perform the session request in a user gesture.
        function thunk() {
          document.removeEventListener('keypress', thunk, false);

          device.requestSession(sessionOptions)
              .then(
                  (session) => {
                    func(t, session, mockVRService);
                  },
                  (err) => {
                    t.step(() => {
                      assert_unreached('requestSession rejected');
                    });
                    t.done();
                  });
        }
        document.addEventListener('keypress', thunk, false);
        eventSender.keyDown(' ', []);
      },
      (err) => {
        t.step(() => {
          assert_unreached(
              'navigator.xr.getDevices rejected in xr_session_test');
        });
        t.done();
      });
}

// Gets the corresponding transform matrix for a WebVR 1.1 pose
function matrixFrom11Pose(pose) {
  let x = pose.orientation[0];
  let y = pose.orientation[1];
  let z = pose.orientation[2];
  let w = pose.orientation[3];
  let x2 = x + x;
  let y2 = y + y;
  let z2 = z + z;
  let xx = x * x2;
  let xy = x * y2;
  let xz = x * z2;
  let yy = y * y2;
  let yz = y * z2;
  let zz = z * z2;
  let wx = w * x2;
  let wy = w * y2;
  let wz = w * z2;

  let out = new Float32Array(16);
  out[0] = 1 - (yy + zz);
  out[1] = xy + wz;
  out[2] = xz - wy;
  out[3] = 0;
  out[4] = xy - wz;
  out[5] = 1 - (xx + zz);
  out[6] = yz + wx;
  out[7] = 0;
  out[8] = xz + wy;
  out[9] = yz - wx;
  out[10] = 1 - (xx + yy);
  out[11] = 0;
  out[12] = pose.position[0];
  out[13] = pose.position[1];
  out[14] = pose.position[2];
  out[15] = 1;

  return out;
}

function perspectiveFromFieldOfView(fov, near, far) {
  let upTan = Math.tan(fov.upDegrees * Math.PI/180.0);
  let downTan = Math.tan(fov.downDegrees * Math.PI/180.0);
  let leftTan = Math.tan(fov.leftDegrees * Math.PI/180.0);
  let rightTan = Math.tan(fov.rightDegrees * Math.PI/180.0);
  let xScale = 2.0 / (leftTan + rightTan);
  let yScale = 2.0 / (upTan + downTan);
  let nf = 1.0 / (near - far);

  let out = new Float32Array(16);
  out[0] = xScale;
  out[1] = 0.0;
  out[2] = 0.0;
  out[3] = 0.0;
  out[4] = 0.0;
  out[5] = yScale;
  out[6] = 0.0;
  out[7] = 0.0;
  out[8] = -((leftTan - rightTan) * xScale * 0.5);
  out[9] = ((upTan - downTan) * yScale * 0.5);
  out[10] = (near + far) * nf;
  out[11] = -1.0;
  out[12] = 0.0;
  out[13] = 0.0;
  out[14] = (2.0 * far * near) * nf;
  out[15] = 0.0;

  return out;
}

function assert_matrices_approx_equal(matA, matB, epsilon = FLOAT_EPSILON) {
  if (matA == null && matB == null) {
    return;
  }

  assert_not_equals(matA, null);
  assert_not_equals(matB, null);

  assert_equals(matA.length, 16);
  assert_equals(matB.length, 16);
  for (let i = 0; i < 16; ++i) {
    assert_approx_equals(matA[i], matB[i], epsilon);
  }
}
