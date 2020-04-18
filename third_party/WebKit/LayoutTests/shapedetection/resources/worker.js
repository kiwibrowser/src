importScripts("../../resources/testharness.js");
importScripts("file:///gen/layout_test_data/mojo/public/js/mojo_bindings.js");
importScripts("file:///gen/mojo/public/mojom/base/big_buffer.mojom.js");
importScripts("file:///gen/skia/public/interfaces/image_info.mojom.js");
importScripts("file:///gen/skia/public/interfaces/bitmap.mojom.js");
importScripts("file:///gen/ui/gfx/geometry/mojo/geometry.mojom.js");
importScripts("file:///gen/services/shape_detection/public/mojom/barcodedetection.mojom.js");
importScripts("file:///gen/services/shape_detection/public/mojom/facedetection.mojom.js");
importScripts("file:///gen/services/shape_detection/public/mojom/facedetection_provider.mojom.js");
importScripts("file:///gen/services/shape_detection/public/mojom/textdetection.mojom.js");
importScripts("big-buffer-helpers.js");
importScripts("mock-barcodedetection.js");
importScripts("mock-facedetection.js");
importScripts("mock-textdetection.js");

onmessage = async function(e) {
  let detector;
  switch (e.data.detectorType) {
    case "Face": detector = new FaceDetector(); break;
    case "Barcode": detector = new BarcodeDetector(); break;
    case "Text": detector = new TextDetector(); break;
  }

  let imageBitmap = e.data.bitmap;
  try {
    let detectionResult = await detector.detect(imageBitmap);
    assert_equals(detectionResult.length, e.data.expectedLength,
                  "Number of " + e.data.detectorType);
    postMessage("PASS");
  } catch (error) {
    assert_unreached("Error during detect(img): " + error);
  }
}
