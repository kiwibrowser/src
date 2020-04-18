// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_glue;

import org.chromium.android_webview.AwServiceWorkerController;
import org.chromium.support_lib_boundary.ServiceWorkerClientBoundaryInterface;
import org.chromium.support_lib_boundary.ServiceWorkerControllerBoundaryInterface;
import org.chromium.support_lib_boundary.util.BoundaryInterfaceReflectionUtil;

import java.lang.reflect.InvocationHandler;

/**
 * Adapter between AwServiceWorkerController and ServiceWorkerControllerBoundaryInterface.
 */
class SupportLibServiceWorkerControllerAdapter implements ServiceWorkerControllerBoundaryInterface {
    AwServiceWorkerController mAwServiceWorkerController;

    SupportLibServiceWorkerControllerAdapter(AwServiceWorkerController awServiceController) {
        mAwServiceWorkerController = awServiceController;
    }

    @Override
    public InvocationHandler getServiceWorkerWebSettings() {
        return BoundaryInterfaceReflectionUtil.createInvocationHandlerFor(
                new SupportLibServiceWorkerSettingsAdapter(
                        mAwServiceWorkerController.getAwServiceWorkerSettings()));
    }

    @Override
    public void setServiceWorkerClient(InvocationHandler client) {
        mAwServiceWorkerController.setServiceWorkerClient(new SupportLibServiceWorkerClientAdapter(
                BoundaryInterfaceReflectionUtil.castToSuppLibClass(
                        ServiceWorkerClientBoundaryInterface.class, client)));
    }
}
