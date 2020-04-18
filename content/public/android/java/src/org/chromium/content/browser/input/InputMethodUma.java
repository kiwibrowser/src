// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import org.chromium.base.metrics.RecordHistogram;

/**
 * A class to record various input method UMAs.
 */
class InputMethodUma {
    // Note that these values should be cross-checked against
    // tools/metrics/histograms/histograms.xml.
    private static final String UMA_REGISTER_PROXYVIEW = "InputMethod.RegisterProxyView";
    private static final int UMA_PROXYVIEW_SUCCESS = 0;
    private static final int UMA_PROXYVIEW_FAILURE = 1;
    private static final int UMA_PROXYVIEW_DETECTION_FAILURE = 2;
    private static final int UMA_PROXYVIEW_REPLICA_INPUT_CONNECTION = 3;
    private static final int UMA_PROXYVIEW_COUNT = 4;

    void recordProxyViewSuccess() {
        RecordHistogram.recordEnumeratedHistogram(
                UMA_REGISTER_PROXYVIEW, UMA_PROXYVIEW_SUCCESS, UMA_PROXYVIEW_COUNT);
    }

    void recordProxyViewFailure() {
        RecordHistogram.recordEnumeratedHistogram(
                UMA_REGISTER_PROXYVIEW, UMA_PROXYVIEW_FAILURE, UMA_PROXYVIEW_COUNT);
    }

    void recordProxyViewDetectionFailure() {
        RecordHistogram.recordEnumeratedHistogram(
                UMA_REGISTER_PROXYVIEW, UMA_PROXYVIEW_DETECTION_FAILURE, UMA_PROXYVIEW_COUNT);
    }
}