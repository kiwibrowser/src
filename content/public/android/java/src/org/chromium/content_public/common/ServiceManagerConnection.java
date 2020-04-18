// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.common;

import org.chromium.content.common.ServiceManagerConnectionImpl;
import org.chromium.mojo.system.MessagePipeHandle;

/**
 * This class is used to get a MessagePipeHandle from a connector which has
 * already connected to service manager, note that this class should only be
 * used in UI thread.
 */
public class ServiceManagerConnection {
    public static MessagePipeHandle getConnectorMessagePipeHandle() {
        return ServiceManagerConnectionImpl.getConnectorMessagePipeHandle();
    }
}
