"use strict";

class MockFaceDetectionProvider {
  constructor() {
    this.bindingSet_ = new mojo.BindingSet(
        shapeDetection.mojom.FaceDetectionProvider);

    this.interceptor_ = new MojoInterfaceInterceptor(
        shapeDetection.mojom.FaceDetectionProvider.name);
    this.interceptor_.oninterfacerequest =
        e => this.bindingSet_.addBinding(this, e.handle);
    this.interceptor_.start();
  }

  createFaceDetection(request, options) {
    this.mockService_ = new MockFaceDetection(request, options);
  }

  getFrameData() {
    return this.mockService_.bufferData_;
  }

  getMaxDetectedFaces() {
   return this.mockService_.maxDetectedFaces_;
  }

  getFastMode () {
    return this.mockService_.fastMode_;
  }
}

class MockFaceDetection {
  constructor(request, options) {
    this.maxDetectedFaces_ = options.maxDetectedFaces;
    this.fastMode_ = options.fastMode;
    this.binding_ =
        new mojo.Binding(shapeDetection.mojom.FaceDetection, this, request);
  }

  detect(bitmapData) {
    this.bufferData_ =
        new Uint32Array(getArrayBufferFromBigBuffer(bitmapData.pixelData));
    return Promise.resolve({
      results: [
        {
          boundingBox: {x: 1.0, y: 1.0, width: 100.0, height: 100.0},
          landmarks: [{
            type: shapeDetection.mojom.LandmarkType.EYE,
            locations: [{x: 4.0, y: 5.0}]
          },
          {
            type: shapeDetection.mojom.LandmarkType.EYE,
            locations: [
              {x: 4.0, y: 5.0}, {x: 5.0, y: 4.0}, {x: 6.0, y: 3.0},
              {x: 7.0, y: 4.0}, {x: 8.0, y: 5.0}, {x: 7.0, y: 6.0},
              {x: 6.0, y: 7.0}, {x: 5.0, y: 6.0}
            ]
          }]
        },
        {
          boundingBox: {x: 2.0, y: 2.0, width: 200.0, height: 200.0},
          landmarks: [{
            type: shapeDetection.mojom.LandmarkType.NOSE,
            locations: [{x: 100.0, y: 50.0}]
          },
          {
            type: shapeDetection.mojom.LandmarkType.NOSE,
            locations: [
              {x: 80.0, y: 50.0}, {x: 70.0, y: 60.0}, {x: 60.0, y: 70.0},
              {x: 70.0, y: 60.0}, {x: 80.0, y: 70.0}, {x: 90.0, y: 80.0},
              {x: 100.0, y: 70.0}, {x: 90.0, y: 60.0}, {x: 80.0, y: 50.0}
            ]
          }]
        },
        {
          boundingBox: {x: 3.0, y: 3.0, width: 300.0, height: 300.0},
          landmarks: []
        },
      ]
    });
  }
}

let mockFaceDetectionProvider = new MockFaceDetectionProvider();
