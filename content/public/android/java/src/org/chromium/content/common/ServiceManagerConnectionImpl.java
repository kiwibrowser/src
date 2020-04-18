// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.common;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.impl.CoreImpl;

/**
 * Implementation of {@link ServiceManagerConnection}
 */
@JNINamespace("content")
public class ServiceManagerConnectionImpl {
    public static MessagePipeHandle getConnectorMessagePipeHandle() {
        ThreadUtils.assertOnUiThread();
        int handle = nativeGetConnectorMessagePipeHandle();
        Core core = CoreImpl.getInstance();
        return core.acquireNativeHandle(handle).toMessagePipeHandle();
    }

    private static native int nativeGetConnectorMessagePipeHandle();
}
