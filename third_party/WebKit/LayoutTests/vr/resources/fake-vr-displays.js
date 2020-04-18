'use strict';

function fakeVRDisplays(){
  let generic_left_fov = {
    upDegrees : 45,
    downDegrees : 45,
    leftDegrees : 50,
    rightDegrees : 40,
  };

  let generic_right_fov = {
    upDegrees : 45,
    downDegrees : 45,
    leftDegrees : 40,
    rightDegrees : 50,
  };

  let generic_left_eye = {
    fieldOfView : generic_left_fov,
    offset : [-0.03, 0, 0],
    renderWidth : 1024,
    renderHeight : 1024
  };

  let generic_right_eye = {
    fieldOfView :generic_right_fov,
    offset : [0.03, 0, 0],
    renderWidth : 1024,
    renderHeight : 1024
  };

  return {
    FakeMagicWindowOnly: {
      displayName : "FakeVRDisplay",
      capabilities : {
        hasPosition : false,
        hasExternalDisplay : false,
        canPresent : false
      },
      stageParameters : null,
      leftEye : null,
      rightEye : null,
      webvrDefaultFramebufferScale: 1.0,
    },

    FakeRoomScale: {
      displayName : "FakeVRDisplayRoom",
      capabilities : {
        hasPosition : true,
        hasExternalDisplay : true,
        canPresent : true,
        maxLayers : 1
      },
      stageParameters : {
        standingTransform : [0.0, 0.1, 0.2, 0.3,
                             0.4, 0.5, 0.6, 0.7,
                             0.8, 0.9, 1.0, 0.1,
                             0.2, 0.3, 0.4, 0.5],
        sizeX : 5.0,
        sizeZ : 3.0,
      },
      leftEye : generic_left_eye,
      rightEye : generic_right_eye,
      webvrDefaultFramebufferScale: 1.0,
    },

    Pixel: { // Pixel info as of Dec. 22 2016
      displayName : "Google, Inc. Daydream View",
      capabilities : {
        hasPosition : false,
        hasExternalDisplay : false,
        canPresent : true,
        maxLayers : 1
      },
      stageParameters : null,
      leftEye : {
        fieldOfView : {
          upDegrees : 48.316,
          downDegrees : 50.099,
          leftDegrees : 35.197,
          rightDegrees : 50.899,
        },
        offset : [-0.032, 0, 0],
        renderWidth : 1920,
        renderHeight : 2160
      },
      rightEye : {
        fieldOfView : {
          upDegrees : 48.316,
          downDegrees : 50.099,
          leftDegrees: 50.899,
          rightDegrees: 35.197
        },
        offset : [0.032, 0, 0],
        renderWidth : 1920,
        renderHeight : 2160
      },
      webvrDefaultFramebufferScale: 0.5,
    }
    // TODO(bsheedy) add more displays like Rift/Vive
  };
}
