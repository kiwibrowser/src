// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_mac_vision.h"

#include <dlfcn.h>
#include <objc/runtime.h>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/sys_string_conversions.h"
#include "services/shape_detection/detection_utils_mac.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace shape_detection {

namespace {

mojom::LandmarkPtr BuildLandmark(VNFaceLandmarkRegion2D* landmark_region,
                                 mojom::LandmarkType landmark_type,
                                 gfx::RectF bounding_box) {
  auto landmark = mojom::Landmark::New();
  landmark->type = landmark_type;
  landmark->locations.reserve(landmark_region.pointCount);
  for (NSUInteger i = 0; i < landmark_region.pointCount; ++i) {
    // The points are normalized to the bounding box of the detected face.
    landmark->locations.emplace_back(
        landmark_region.normalizedPoints[i].x * bounding_box.width() +
            bounding_box.x(),
        (1 - landmark_region.normalizedPoints[i].y) * bounding_box.height() +
            bounding_box.y());
  }
  return landmark;
}
}

// The VisionAPIAsyncRequestMac class submits an image analysis request for
// asynchronous execution on a dispatch queue with default priority.
class API_AVAILABLE(macos(10.13))
    FaceDetectionImplMacVision::VisionAPIAsyncRequestMac {
 public:
  // A callback run when the asynchronous execution completes. The callback is
  // repeating for the instance.
  using Callback =
      base::RepeatingCallback<void(VNRequest* request, NSError* error)>;

  ~VisionAPIAsyncRequestMac() = default;

  // Creates an VisionAPIAsyncRequestMac instance which sets |callback| to be
  // called when the asynchronous action completes.
  static std::unique_ptr<VisionAPIAsyncRequestMac> Create(Class request_class,
                                                          Callback callback) {
    return base::WrapUnique(
        new VisionAPIAsyncRequestMac(std::move(callback), request_class));
  }

  // Processes asynchronously an image analysis request and returns results with
  // |callback_| when the asynchronous request completes.
  bool PerformRequest(const SkBitmap& bitmap) {
    Class image_handler_class = NSClassFromString(@"VNImageRequestHandler");
    if (!image_handler_class) {
      DLOG(ERROR) << "Failed to create VNImageRequestHandler";
      return false;
    }

    base::scoped_nsobject<CIImage> ci_image = CreateCIImageFromSkBitmap(bitmap);
    if (!ci_image) {
      DLOG(ERROR) << "Failed to create image from SkBitmap";
      return false;
    }

    base::scoped_nsobject<VNImageRequestHandler> image_handler(
        [[image_handler_class alloc] initWithCIImage:ci_image options:@{}]);
    if (!image_handler) {
      DLOG(ERROR) << "Failed to create image request handler";
      return false;
    }

    dispatch_async(
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
          NSError* ns_error = nil;
          if ([image_handler performRequests:@[ request_ ] error:&ns_error])
            return;
          DLOG(ERROR) << base::SysNSStringToUTF8(
              [ns_error localizedDescription]);
        });
    return true;
  }

 private:
  VisionAPIAsyncRequestMac(Callback callback, Class request_class)
      : callback_(std::move(callback)) {
    DCHECK(callback_);

    scoped_refptr<base::SequencedTaskRunner> task_runner =
        base::SequencedTaskRunnerHandle::Get();

    const auto handler = ^(VNRequest* request, NSError* error) {
      task_runner->PostTask(FROM_HERE,
                            base::BindOnce(callback_, request, error));
    };

    request_.reset([[request_class alloc] initWithCompletionHandler:handler]);
  }

  base::scoped_nsobject<VNRequest> request_;
  const Callback callback_;

  DISALLOW_COPY_AND_ASSIGN(VisionAPIAsyncRequestMac);
};

FaceDetectionImplMacVision::FaceDetectionImplMacVision() : weak_factory_(this) {
  static void* const vision_framework =
      dlopen("/System/Library/Frameworks/Vision.framework/Vision", RTLD_LAZY);
  if (!vision_framework) {
    DLOG(ERROR) << "Failed to load Vision.framework";
    return;
  }

  Class request_class = NSClassFromString(@"VNDetectFaceLandmarksRequest");
  if (!request_class) {
    DLOG(ERROR) << "Failed to create VNDetectFaceLandmarksRequest object";
    return;
  }
  // The repeating callback will not be run if FaceDetectionImplMacVision object
  // has already been destroyed.
  landmarks_async_request_ = VisionAPIAsyncRequestMac::Create(
      request_class,
      base::BindRepeating(&FaceDetectionImplMacVision::OnFacesDetected,
                          weak_factory_.GetWeakPtr()));
}

FaceDetectionImplMacVision::~FaceDetectionImplMacVision() = default;

void FaceDetectionImplMacVision::Detect(const SkBitmap& bitmap,
                                        DetectCallback callback) {
  DCHECK(landmarks_async_request_);

  if (!landmarks_async_request_->PerformRequest(bitmap)) {
    std::move(callback).Run({});
    return;
  }

  image_size_ = CGSizeMake(bitmap.width(), bitmap.height());
  // Hold on the callback until async request completes.
  detected_callback_ = std::move(callback);
  // This prevents the Detect function from being called before the
  // VisionAPIAsyncRequestMac completes.
  if (binding_)  // Can be unbound in unit testing.
    binding_->PauseIncomingMethodCallProcessing();
}

void FaceDetectionImplMacVision::OnFacesDetected(VNRequest* request,
                                                 NSError* error) {
  if (binding_)  // Can be unbound in unit testing.
    binding_->ResumeIncomingMethodCallProcessing();

  if (![request.results count] || error) {
    std::move(detected_callback_).Run({});
    return;
  }

  std::vector<mojom::FaceDetectionResultPtr> results;
  for (VNFaceObservation* const observation in request.results) {
    auto face = mojom::FaceDetectionResult::New();
    // The coordinate are normalized to the dimensions of the processed image.
    face->bounding_box = ConvertCGToGfxCoordinates(
        CGRectMake(observation.boundingBox.origin.x * image_size_.width,
                   observation.boundingBox.origin.y * image_size_.height,
                   observation.boundingBox.size.width * image_size_.width,
                   observation.boundingBox.size.height * image_size_.height),
        image_size_.height);

    if (VNFaceLandmarkRegion2D* leftEye = observation.landmarks.leftEye) {
      face->landmarks.push_back(
          BuildLandmark(leftEye, mojom::LandmarkType::EYE, face->bounding_box));
    }
    if (VNFaceLandmarkRegion2D* rightEye = observation.landmarks.rightEye) {
      face->landmarks.push_back(BuildLandmark(
          rightEye, mojom::LandmarkType::EYE, face->bounding_box));
    }
    if (VNFaceLandmarkRegion2D* outerLips = observation.landmarks.outerLips) {
      face->landmarks.push_back(BuildLandmark(
          outerLips, mojom::LandmarkType::MOUTH, face->bounding_box));
    }
    if (VNFaceLandmarkRegion2D* nose = observation.landmarks.nose) {
      face->landmarks.push_back(
          BuildLandmark(nose, mojom::LandmarkType::NOSE, face->bounding_box));
    }

    results.push_back(std::move(face));
  }
  std::move(detected_callback_).Run(std::move(results));

  return;
}

}  // namespace shape_detection
