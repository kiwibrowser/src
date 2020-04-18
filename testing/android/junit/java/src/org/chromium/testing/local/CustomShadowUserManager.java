// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.testing.local;

import android.os.Bundle;
import android.os.UserManager;

import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowUserManager;

import java.util.HashMap;
import java.util.Map;

/**
 * TODO(https://crbug.com/779568): Remove this after migrating to Robolectric 3.7+.
 * Robolectric 3.6 and lower doesn't support getApplicationRestrictions.
 */
@Implements(UserManager.class)
public class CustomShadowUserManager extends ShadowUserManager {
    private Map<String, Bundle> mApplicationRestrictions = new HashMap<>();

    @Override
    public Bundle getApplicationRestrictions(String packageName) {
        Bundle bundle = mApplicationRestrictions.get(packageName);
        return bundle != null ? bundle : new Bundle();
    }

    public void setApplicationRestrictions(String packageName, Bundle applicationRestrictions) {
        mApplicationRestrictions.put(packageName, applicationRestrictions);
    }
}
