// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.shape_detection;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.services.service_manager.InterfaceRegistry;
import org.chromium.shape_detection.mojom.BarcodeDetection;
import org.chromium.shape_detection.mojom.FaceDetectionProvider;
import org.chromium.shape_detection.mojom.TextDetection;

@JNINamespace("shape_detection")
class InterfaceRegistrar {
    @CalledByNative
    static void createInterfaceRegistryForContext(int nativeHandle) {
        // Note: The bindings code manages the lifetime of this object, so it
        // is not necessary to hold on to a reference to it explicitly.
        InterfaceRegistry registry = InterfaceRegistry.create(
                CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle());
        registry.addInterface(BarcodeDetection.MANAGER, new BarcodeDetectionImpl.Factory());
        registry.addInterface(
                FaceDetectionProvider.MANAGER, new FaceDetectionProviderImpl.Factory());
        registry.addInterface(TextDetection.MANAGER, new TextDetectionImpl.Factory());
    }
}
