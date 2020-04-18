/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.wifi;

import android.content.pm.UserInfo;
import android.net.wifi.WifiConfiguration;
import android.os.UserHandle;

import java.util.List;

/**
 * Helper for working with {@link android.net.wifi.WifiConfiguration} objects.
 */
public class WifiConfigurationUtil {
    /**
     * Check whether a network configuration is visible to a user or any of its managed profiles.
     * @param config the network configuration whose visibility should be checked
     * @param profiles the user IDs of the user itself and all its managed profiles (can be obtained
     *         via {@link android.os.UserManager.getProfiles})
     * @return whether the network configuration is visible to the user or any of its managed
     *         profiles
     */
    public static boolean isVisibleToAnyProfile(WifiConfiguration config, List<UserInfo> profiles) {
        if (config.shared) {
            return true;
        }
        final int creatorUserId = UserHandle.getUserId(config.creatorUid);
        for (UserInfo profile : profiles) {
            if (profile.id == creatorUserId) {
                return true;
            }
        }
        return false;
    }
}
