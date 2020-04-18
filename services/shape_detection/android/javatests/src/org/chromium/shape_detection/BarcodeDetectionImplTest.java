// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.shape_detection;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.shape_detection.mojom.BarcodeDetection;
import org.chromium.shape_detection.mojom.BarcodeDetectionResult;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Test suite for BarcodeDetectionImpl.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class BarcodeDetectionImplTest {
    private static final org.chromium.skia.mojom.Bitmap QR_CODE_BITMAP =
            TestUtils.mojoBitmapFromFile("qr_code.png");

    private static BarcodeDetectionResult[] detect(org.chromium.skia.mojom.Bitmap mojoBitmap) {
        BarcodeDetection detector = new BarcodeDetectionImpl();

        final ArrayBlockingQueue<BarcodeDetectionResult[]> queue = new ArrayBlockingQueue<>(1);
        detector.detect(mojoBitmap, new BarcodeDetection.DetectResponse() {
            @Override
            public void call(BarcodeDetectionResult[] results) {
                queue.add(results);
            }
        });
        BarcodeDetectionResult[] toReturn = null;
        try {
            toReturn = queue.poll(5L, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Assert.fail("Could not get BarcodeDetectionResult: " + e.toString());
        }
        Assert.assertNotNull(toReturn);
        return toReturn;
    }

    @Test
    @SmallTest
    @Feature({"ShapeDetection"})
    public void testDetectBase64ValidImageString() {
        if (!TestUtils.IS_GMS_CORE_SUPPORTED) {
            return;
        }
        BarcodeDetectionResult[] results = detect(QR_CODE_BITMAP);
        Assert.assertEquals(1, results.length);
        Assert.assertEquals("https://chromium.org", results[0].rawValue);
        Assert.assertEquals(40.0, results[0].boundingBox.x, 0.0);
        Assert.assertEquals(40.0, results[0].boundingBox.y, 0.0);
        Assert.assertEquals(250.0, results[0].boundingBox.width, 0.0);
        Assert.assertEquals(250.0, results[0].boundingBox.height, 0.0);
    }
}
