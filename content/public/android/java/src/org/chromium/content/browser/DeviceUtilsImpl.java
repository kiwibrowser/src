// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;

import org.chromium.base.CommandLine;
import org.chromium.content_public.common.ContentSwitches;
import org.chromium.ui.base.DeviceFormFactor;

/**
 * A utility class that has helper methods for device configuration.
 */
public class DeviceUtilsImpl {
    private DeviceUtilsImpl() {}

    public static void addDeviceSpecificUserAgentSwitch(Context context) {
        if (!DeviceFormFactor.isTablet()) {
            CommandLine.getInstance().appendSwitch(ContentSwitches.USE_MOBILE_UA);
        }
    }
}
