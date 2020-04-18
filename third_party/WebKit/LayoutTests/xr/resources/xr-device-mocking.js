'use strict';

/* This class contains everything required to trick the system into thinking it
 * has a fully functional XRDevices. When moving test to WPT, this file will
 * need to be modified to be consistent with what is decided for the cross-
 * platform APIs. */

let mockVRService;
function addFakeDevice(device) {
  // TODO(offenwanger): Switch out this code with test API code when it's
  // available.
  if (!mockVRService) {
    mockVRService = new MockVRService(mojo.frameInterfaces);
  }

  mockVRService.addVRDisplay(device);
}

function setFakeDevices(devices) {
  // TODO(offenwanger): Switch out this code with test API code when it's
  // available.
  if (!mockVRService) {
    mockVRService = new MockVRService(mojo.frameInterfaces);
  }

  mockVRService.setVRDisplays(devices);
}

// Sets the pose for the first device
function setPose(pose) {
  mockVRService.mockVRDisplays_[0].setPose(pose);
}

function setStageTransform(transform) {
  mockVRService.mockVRDisplays_[0].setStageTransform(transform);
}

// Returns the submitted frame count for the first display
function getSubmitFrameCount() {
  return mockVRService.mockVRDisplays_[0].getSubmitFrameCount();
}

// Returns the missing (not submitted) frame count for the first display
function getMissingFrameCount() {
  return mockVRService.mockVRDisplays_[0].getMissingFrameCount();
}

function addInputSource(input_source) {
  return mockVRService.mockVRDisplays_[0].addInputSource(input_source);
}

function removeInputSource(input_source) {
  return mockVRService.mockVRDisplays_[0].removeInputSource(input_source);
}

function fakeXRDevices() {
  let generic_left_fov = {
    upDegrees: 45,
    downDegrees: 45,
    leftDegrees: 50,
    rightDegrees: 40,
  };

  let generic_right_fov = {
    upDegrees: 45,
    downDegrees: 45,
    leftDegrees: 40,
    rightDegrees: 50,
  };

  let generic_left_eye = {
    fieldOfView: generic_left_fov,
    offset: [-0.03, 0, 0],
    renderWidth: 1024,
    renderHeight: 1024
  };

  let generic_right_eye = {
    fieldOfView: generic_right_fov,
    offset: [0.03, 0, 0],
    renderWidth: 1024,
    renderHeight: 1024
  };

  return {
    FakeMagicWindowOnly: {
      displayName: 'FakeVRDisplay',
      capabilities:
          {hasPosition: false, hasExternalDisplay: false, canPresent: false},
      stageParameters: null,
      leftEye: null,
      rightEye: null,
      webxrDefaultFramebufferScale: 1.0,
    },

    FakeRoomScale: {
      displayName: 'FakeVRDisplayRoom',
      capabilities: {
        hasPosition: true,
        hasExternalDisplay: true,
        canPresent: true,
        maxLayers: 1
      },
      stageParameters: {
        standingTransform: [
          0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 0.1, 0.2, 0.3,
          0.4, 0.5
        ],
        sizeX: 5.0,
        sizeZ: 3.0,
      },
      leftEye: generic_left_eye,
      rightEye: generic_right_eye,
      webxrDefaultFramebufferScale: 1.0,
    },

    FakeGooglePixelPhone: {
      // Pixel info as of Dec. 22 2016
      displayName: 'Google, Inc. Daydream View',
      capabilities: {
        hasPosition: false,
        hasExternalDisplay: false,
        canPresent: true,
        maxLayers: 1
      },
      stageParameters: null,
      leftEye: {
        fieldOfView: {
          upDegrees: 48.316,
          downDegrees: 50.099,
          leftDegrees: 35.197,
          rightDegrees: 50.899,
        },
        offset: [-0.032, 0, 0],
        renderWidth: 1920,
        renderHeight: 2160
      },
      rightEye: {
        fieldOfView: {
          upDegrees: 48.316,
          downDegrees: 50.099,
          leftDegrees: 50.899,
          rightDegrees: 35.197
        },
        offset: [0.032, 0, 0],
        renderWidth: 1920,
        renderHeight: 2160
      },
      webxrDefaultFramebufferScale: 0.7,
    }
  };
}

// TODO(offenwanger): Update this to remove references to 1.1
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
  let upTan = Math.tan(fov.upDegrees * Math.PI / 180.0);
  let downTan = Math.tan(fov.downDegrees * Math.PI / 180.0);
  let leftTan = Math.tan(fov.leftDegrees * Math.PI / 180.0);
  let rightTan = Math.tan(fov.rightDegrees * Math.PI / 180.0);
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

function assert_matrices_approx_equal(matA, matB, epsilon = FLOAT_EPSILON, message = "") {
  if (matA == null && matB == null) {
    return;
  }

  assert_not_equals(matA, null);
  assert_not_equals(matB, null);

  assert_equals(matA.length, 16);
  assert_equals(matB.length, 16);

  let mismatched_element = -1;
  for (let i = 0; i < 16; ++i) {
    if (Math.abs(matA[i] - matB[i]) > epsilon) {
      mismatched_element = i;
      break;
    }
  }

  if (mismatched_element > -1) {
    let matA_str = "[";
    let matB_str = "[";
    for (let i = 0; i < 16; ++i) {
      matA_str += matA[i] + (i < 15 ? ", " : "");
      matB_str += matB[i] + (i < 15 ? ", " : "");
    }
    matA_str += "]";
    matB_str += "]";

    let error_message = message ? message + "\n" : "Matrix comparison failed.\n";
    error_message += " Difference in element " + mismatched_element + " exceeded the given epsilon.\n";

    error_message += " Matrix A: " + matA_str + "\n";
    error_message += " Matrix B: " + matB_str + "\n";

    assert_approx_equals(matA[mismatched_element], matB[mismatched_element], epsilon, error_message);
  }
}

// TODO(offenwanger): Delete everything below when test API code is available.
class MockDevice {
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
    this.presentation_provider_.bind(
        submitFrameClient, request, presentOptions);
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

  setStageTransform(value) {
    if (value) {
      if (!this.displayInfo_.stageParameters) {
        this.displayInfo_.stageParameters = {
          standingTransform: value,
          sizeX: 1.5,
          sizeZ: 1.5,
        };
      } else {
        this.displayInfo_.stageParameters.standingTransform = value;
      }
    } else if (this.displayInfo_.stageParameters) {
      this.displayInfo_.stageParameters = null;
    }

    this.displayClient_.onChanged(this.displayInfo_);
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

  // This function calls to the backend to add this device to the list.
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
    this.service_.client_.onDisplayConnected(
        magicWindowPtr, displayPtr, clientRequest, this.displayInfo_);
  }

  addInputSource(input_source) {
    this.presentation_provider_.OnInputSourceAdded(input_source);
  }

  removeInputSource(input_source) {
    this.presentation_provider_.OnInputSourceRemoved(input_source);
  }
}

class MockVRPresentationProvider {
  constructor() {
    this.binding_ = new mojo.Binding(device.mojom.VRPresentationProvider, this);
    this.pose_ = null;
    this.next_frame_id_ = 0;
    this.submit_frame_count_ = 0;
    this.missing_frame_count_ = 0;

    this.input_sources_ = [];
    this.next_input_source_index_ = 1;
  }

  bind(client, request) {
    this.submitFrameClient_ = client;
    this.binding_.close();
    this.binding_.bind(request);
  }

  submitFrameMissing(frameId, mailboxHolder, timeWaited) {
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

      let input_states = [];
      for (let i = 0; i < this.input_sources_.length; ++i) {
        input_states.push(this.input_sources_[i].getInputSourceState());
      }

      this.pose_.inputState = input_states;
    }

    // Convert current document time to monotonic time.
    var now = window.performance.now() / 1000.0;
    var diff = now - internals.monotonicTimeToZeroBasedDocumentTime(now);
    now += diff;
    now *= 1000000;

    let retval = Promise.resolve({
      pose: this.pose_,
      time: {
        microseconds: now,
      },
      frameId: this.next_frame_id_++,
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
      inputState: null,
      poseReset: false,
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

  OnInputSourceAdded(input_source) {
    let index = this.next_input_source_index_;
    input_source.source_id_ = index;
    this.next_input_source_index_++;
    this.input_sources_.push(input_source);
  }

  OnInputSourceRemoved(input_source) {
    for (let i = 0; i < this.input_sources_.length; ++i) {
      if (input_source.source_id_ == this.input_sources_[i].source_id_) {
        this.input_sources_.splice(i, 1);
        break;
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
    this.interceptor_.oninterfacerequest = e =>
        this.bindingSet_.addBinding(this, e.handle);
    this.interceptor_.start();
  }

  setVRDisplays(displays) {
    this.mockVRDisplays_ = [];
    for (let i = 0; i < displays.length; i++) {
      displays[i].index = i;
      this.mockVRDisplays_.push(new MockDevice(displays[i], this));
    }
  }

  addVRDisplay(display) {
    if (this.mockVRDisplays_.length) {
      display.index = this.mockVRDisplays_[this.mockVRDisplays_.length - 1] + 1;
    } else {
      display.index = 0;
    }
    this.mockVRDisplays_.push(new MockDevice(display, this));
  }

  // This function gets called from mojo, and is what triggers all displays to
  // be added.
  setClient(client) {
    this.client_ = client;
    for (let i = 0; i < this.mockVRDisplays_.length; i++) {
      this.mockVRDisplays_[i].notifyClientOfDisplay();
    }

    let device_number = this.mockVRDisplays_.length;
    return Promise.resolve({numberOfConnectedDevices: device_number});
  }
}

class MockXRInputSource {
  constructor() {
    this.source_id_ = 0;
    this.primary_input_pressed_ = false;
    this.primary_input_clicked_ = false;
    this.grip_ = null;

    this.pointer_origin_ = "head";
    this.pointer_offset_ = null;
    this.emulated_position_ = false;
    this.handedness_ = "";
    this.desc_dirty_ = true;
  }

  get primaryInputPressed() {
    return this.primary_input_pressed_;
  }

  set primaryInputPressed(value) {
    if (this.primary_input_pressed_ && !value) {
      this.primary_input_clicked_ = true;
    }
    this.primary_input_pressed_ = value;
  }

  get grip() {
    if (this.grip_) {
      return this.grip_.matrix;
    }
    return null;
  }

  set grip(value) {
    if (!value) {
      this.grip_ = null;
      return;
    }
    this.grip_ = new gfx.mojom.Transform();
    this.grip_.matrix = new Float32Array(value);
  }

  get pointerOrigin() {
    return this.pointer_origin_;
  }

  set pointerOrigin(value) {
    if (this.pointer_origin_ != value) {
      this.desc_dirty_ = true;
      this.pointer_origin_ = value;
    }
  }

  get pointerOffset() {
    if (this.pointer_offset_) {
      return this.pointer_offset_.matrix;
    }
    return null;
  }

  set pointerOffset(value) {
    this.desc_dirty_ = true;
    if (!value) {
      this.pointer_offset_ = null;
      return;
    }
    this.pointer_offset_ = new gfx.mojom.Transform();
    this.pointer_offset_.matrix = new Float32Array(value);
  }

  get emulatedPosition() {
    return this.emulated_position_;
  }

  set emulatedPosition(value) {
    if (this.emulated_position_ != value) {
      this.desc_dirty_ = true;
      this.emulated_position_ = value;
    }
  }

  get handedness() {
    return this.handedness_;
  }

  set handedness(value) {
    if (this.handedness_ != value) {
      this.desc_dirty_ = true;
      this.handedness_ = value;
    }
  }

  getInputSourceState() {
    let input_state = new device.mojom.XRInputSourceState();

    input_state.sourceId = this.source_id_;

    input_state.primaryInputPressed = this.primary_input_pressed_;
    input_state.primaryInputClicked = this.primary_input_clicked_;

    input_state.grip = this.grip_;

    if (this.desc_dirty_) {
      let input_desc = new device.mojom.XRInputSourceDescription();

      input_desc.emulatedPosition = this.emulated_position_;

      switch (this.pointer_origin_) {
        case "head":
          input_desc.pointerOrigin = device.mojom.XRPointerOrigin.HEAD;
          break;
        case "hand":
          input_desc.pointerOrigin = device.mojom.XRPointerOrigin.HAND;
          break;
      }

      switch (this.handedness_) {
        case "left":
          input_desc.handedness = device.mojom.XRHandedness.LEFT;
          break;
        case "right":
          input_desc.handedness = device.mojom.XRHandedness.RIGHT;
          break;
        default:
          input_desc.handedness = device.mojom.XRHandedness.NONE;
          break;
      }

      input_desc.pointerOffset = this.pointer_offset_;

      input_state.description = input_desc;

      this.desc_dirty_ = false;
    }

    return input_state;
  }
}
