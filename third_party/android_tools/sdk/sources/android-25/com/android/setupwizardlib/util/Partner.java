/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.setupwizardlib.util;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.util.Log;

import com.android.setupwizardlib.annotations.VisibleForTesting;

/**
 * Utilities to discover and interact with partner customizations. An overlay package is one that
 * registers the broadcast receiver for {@code com.android.setupwizard.action.PARTNER_CUSTOMIZATION}
 * in its manifest. There can only be one customization APK on a device, and it must be bundled with
 * the system.
 *
 * <p>Derived from {@code com.android.launcher3/Partner.java}
 */
public class Partner {
    private static final String TAG = "(SUW) Partner";

    /** Marker action used to discover partner */
    private static final String
            ACTION_PARTNER_CUSTOMIZATION = "com.android.setupwizard.action.PARTNER_CUSTOMIZATION";

    private static boolean sSearched = false;
    private static Partner sPartner;

    /**
     * Convenience to get a drawable from partner overlay, or if not available, the drawable from
     * the original context.
     *
     * @see #getResourceEntry(android.content.Context, int)
     */
    public static Drawable getDrawable(Context context, int id) {
        final ResourceEntry entry = getResourceEntry(context, id);
        return entry.resources.getDrawable(entry.id);
    }

    /**
     * Convenience to get a string from partner overlay, or if not available, the string from the
     * original context.
     *
     * @see #getResourceEntry(android.content.Context, int)
     */
    public static String getString(Context context, int id) {
        final ResourceEntry entry = getResourceEntry(context, id);
        return entry.resources.getString(entry.id);
    }

    /**
     * Find an entry of resource in the overlay package provided by partners. It will first look for
     * the resource in the overlay package, and if not available, will return the one in the
     * original context.
     *
     * @return a ResourceEntry in the partner overlay's resources, if one is defined. Otherwise the
     * resources from the original context is returned. Clients can then get the resource by
     * {@code entry.resources.getString(entry.id)}, or other methods available in
     * {@link android.content.res.Resources}.
     */
    public static ResourceEntry getResourceEntry(Context context, int id) {
        final Partner partner = Partner.get(context);
        if (partner != null) {
            final Resources ourResources = context.getResources();
            final String name = ourResources.getResourceEntryName(id);
            final String type = ourResources.getResourceTypeName(id);
            final int partnerId = partner.getIdentifier(name, type);
            if (partnerId != 0) {
                return new ResourceEntry(partner.mResources, partnerId, true);
            }
        }
        return new ResourceEntry(context.getResources(), id, false);
    }

    public static class ResourceEntry {
        public Resources resources;
        public int id;
        public boolean isOverlay;

        ResourceEntry(Resources resources, int id, boolean isOverlay) {
            this.resources = resources;
            this.id = id;
            this.isOverlay = isOverlay;
        }
    }

    /**
     * Find and return partner details, or {@code null} if none exists. A partner package is marked
     * by a broadcast receiver declared in the manifest that handles the
     * {@code com.android.setupwizard.action.PARTNER_CUSTOMIZATION} intent action. The overlay
     * package must also be a system package.
     */
    public static synchronized Partner get(Context context) {
        if (!sSearched) {
            PackageManager pm = context.getPackageManager();
            final Intent intent = new Intent(ACTION_PARTNER_CUSTOMIZATION);
            for (ResolveInfo info : pm.queryBroadcastReceivers(intent, 0)) {
                if (info.activityInfo == null) {
                    continue;
                }
                final ApplicationInfo appInfo = info.activityInfo.applicationInfo;
                if ((appInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0) {
                    try {
                        final Resources res = pm.getResourcesForApplication(appInfo);
                        sPartner = new Partner(appInfo.packageName, res);
                        break;
                    } catch (NameNotFoundException e) {
                        Log.w(TAG, "Failed to find resources for " + appInfo.packageName);
                    }
                }
            }
            sSearched = true;
        }
        return sPartner;
    }

    @VisibleForTesting
    public static synchronized void resetForTesting() {
        sSearched = false;
        sPartner = null;
    }

    private final String mPackageName;
    private final Resources mResources;

    private Partner(String packageName, Resources res) {
        mPackageName = packageName;
        mResources = res;
    }

    public String getPackageName() {
        return mPackageName;
    }

    public Resources getResources() {
        return mResources;
    }

    public int getIdentifier(String name, String defType) {
        return mResources.getIdentifier(name, defType, mPackageName);
    }
}
