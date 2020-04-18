// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.shape_detection;

import android.graphics.PointF;
import android.util.SparseArray;

import com.google.android.gms.vision.Frame;
import com.google.android.gms.vision.face.Face;
import com.google.android.gms.vision.face.FaceDetector;
import com.google.android.gms.vision.face.Landmark;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.gfx.mojom.RectF;
import org.chromium.mojo.system.MojoException;
import org.chromium.shape_detection.mojom.FaceDetection;
import org.chromium.shape_detection.mojom.FaceDetectionResult;
import org.chromium.shape_detection.mojom.FaceDetectorOptions;
import org.chromium.shape_detection.mojom.LandmarkType;

import java.util.ArrayList;
import java.util.List;

/**
 * Google Play services implementation of the FaceDetection service defined in
 * services/shape_detection/public/mojom/facedetection.mojom
 */
public class FaceDetectionImplGmsCore implements FaceDetection {
    private static final String TAG = "FaceDetectionImpl";
    private static final int MAX_FACES = 32;
    // Maximum rotation around the z-axis allowed when computing a tighter bounding box for the
    // detected face.
    private static final int MAX_EULER_Z = 15;
    private final int mMaxFaces;
    private final boolean mFastMode;
    private final FaceDetector mFaceDetector;

    FaceDetectionImplGmsCore(FaceDetectorOptions options) {
        FaceDetector.Builder builder =
                new FaceDetector.Builder(ContextUtils.getApplicationContext());
        mMaxFaces = Math.min(options.maxDetectedFaces, MAX_FACES);
        mFastMode = options.fastMode;

        try {
            builder.setMode(mFastMode ? FaceDetector.FAST_MODE : FaceDetector.ACCURATE_MODE);
            builder.setLandmarkType(FaceDetector.ALL_LANDMARKS);
            if (mMaxFaces == 1) {
                builder.setProminentFaceOnly(true);
            }
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Unexpected exception " + e);
            assert false;
        }

        mFaceDetector = builder.build();
    }

    @Override
    public void detect(org.chromium.skia.mojom.Bitmap bitmapData, DetectResponse callback) {
        // The vision library will be downloaded the first time the API is used
        // on the device; this happens "fast", but it might have not completed,
        // bail in this case.
        if (!mFaceDetector.isOperational()) {
            Log.e(TAG, "FaceDetector is not operational");

            // Fallback to Android's FaceDetectionImpl.
            FaceDetectorOptions options = new FaceDetectorOptions();
            options.fastMode = mFastMode;
            options.maxDetectedFaces = mMaxFaces;
            FaceDetectionImpl detector = new FaceDetectionImpl(options);
            detector.detect(bitmapData, callback);
            return;
        }

        Frame frame = BitmapUtils.convertToFrame(bitmapData);
        if (frame == null) {
            Log.e(TAG, "Error converting Mojom Bitmap to Frame");
            callback.call(new FaceDetectionResult[0]);
            return;
        }

        final SparseArray<Face> faces = mFaceDetector.detect(frame);

        FaceDetectionResult[] faceArray = new FaceDetectionResult[faces.size()];
        for (int i = 0; i < faces.size(); i++) {
            faceArray[i] = new FaceDetectionResult();
            final Face face = faces.valueAt(i);

            final List<Landmark> landmarks = face.getLandmarks();
            ArrayList<org.chromium.shape_detection.mojom.Landmark> mojoLandmarks =
                    new ArrayList<org.chromium.shape_detection.mojom.Landmark>(landmarks.size());

            int leftEyeIndex = -1;
            int rightEyeIndex = -1;
            int bottomMouthIndex = -1;
            for (int j = 0; j < landmarks.size(); j++) {
                final Landmark landmark = landmarks.get(j);
                final int landmarkType = landmark.getType();
                if (landmarkType != Landmark.LEFT_EYE && landmarkType != Landmark.RIGHT_EYE
                        && landmarkType != Landmark.BOTTOM_MOUTH
                        && landmarkType != Landmark.NOSE_BASE) {
                    continue;
                }

                org.chromium.shape_detection.mojom.Landmark mojoLandmark =
                        new org.chromium.shape_detection.mojom.Landmark();
                mojoLandmark.locations = new org.chromium.gfx.mojom.PointF[1];
                mojoLandmark.locations[0] = new org.chromium.gfx.mojom.PointF();
                mojoLandmark.locations[0].x = landmark.getPosition().x;
                mojoLandmark.locations[0].y = landmark.getPosition().y;

                if (landmarkType == Landmark.LEFT_EYE) {
                    mojoLandmark.type = LandmarkType.EYE;
                    leftEyeIndex = j;
                } else if (landmarkType == Landmark.RIGHT_EYE) {
                    mojoLandmark.type = LandmarkType.EYE;
                    rightEyeIndex = j;
                } else if (landmarkType == Landmark.BOTTOM_MOUTH) {
                    mojoLandmark.type = LandmarkType.MOUTH;
                    bottomMouthIndex = j;
                } else {
                    assert landmarkType == Landmark.NOSE_BASE;
                    mojoLandmark.type = LandmarkType.NOSE;
                }
                mojoLandmarks.add(mojoLandmark);
            }
            faceArray[i].landmarks = mojoLandmarks.toArray(
                    new org.chromium.shape_detection.mojom.Landmark[mojoLandmarks.size()]);

            final PointF corner = face.getPosition();
            faceArray[i].boundingBox = new RectF();
            if (leftEyeIndex != -1 && rightEyeIndex != -1
                    && Math.abs(face.getEulerZ()) < MAX_EULER_Z) {
                // Tighter calculation of the bounding box because the GMScore
                // and Android Face APIs give different results.
                final PointF leftEyePoint = landmarks.get(leftEyeIndex).getPosition();
                final PointF rightEyePoint = landmarks.get(rightEyeIndex).getPosition();
                final float eyesDistance = leftEyePoint.x - rightEyePoint.x;
                final float eyeMouthDistance = bottomMouthIndex != -1
                        ? landmarks.get(bottomMouthIndex).getPosition().y - leftEyePoint.y
                        : -1;
                final PointF midEyePoint =
                        new PointF(corner.x + face.getWidth() / 2, leftEyePoint.y);
                faceArray[i].boundingBox.x = 2 * rightEyePoint.x - midEyePoint.x;
                faceArray[i].boundingBox.y = midEyePoint.y - eyesDistance;
                faceArray[i].boundingBox.width = 2 * eyesDistance;
                faceArray[i].boundingBox.height = eyeMouthDistance > eyesDistance
                        ? eyeMouthDistance + eyesDistance
                        : 2 * eyesDistance;
            } else {
                faceArray[i].boundingBox.x = corner.x;
                faceArray[i].boundingBox.y = corner.y;
                faceArray[i].boundingBox.width = face.getWidth();
                faceArray[i].boundingBox.height = face.getHeight();
            }
        }
        callback.call(faceArray);
    }

    @Override
    public void close() {
        mFaceDetector.release();
    }

    @Override
    public void onConnectionError(MojoException e) {
        close();
    }
}
