// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.geolocation;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;

/**
 * Factory to create a LocationProvider to allow us to inject a mock for tests.
 */
public class LocationProviderFactory {
    private static LocationProviderFactory.LocationProvider sProviderImpl;
    private static boolean sUseGmsCoreLocationProvider;

    /**
     * LocationProviderFactory.create() returns an instance of this interface.
     */
    public interface LocationProvider {
        /**
         * Start listening for location updates. Calling several times before stop() is interpreted
         * as restart.
         * @param enableHighAccuracy Whether or not to enable high accuracy location.
         */
        public void start(boolean enableHighAccuracy);

        /**
         * Stop listening for location updates.
         */
        public void stop();

        /**
         * Returns true if we are currently listening for location updates, false if not.
         */
        public boolean isRunning();
    }

    private LocationProviderFactory() {}

    @VisibleForTesting
    public static void setLocationProviderImpl(LocationProviderFactory.LocationProvider provider) {
        sProviderImpl = provider;
    }

    public static void useGmsCoreLocationProvider() {
        sUseGmsCoreLocationProvider = true;
    }

    public static LocationProvider create() {
        if (sProviderImpl != null) return sProviderImpl;

        if (sUseGmsCoreLocationProvider
                && LocationProviderGmsCore.isGooglePlayServicesAvailable(
                           ContextUtils.getApplicationContext())) {
            sProviderImpl = new LocationProviderGmsCore(ContextUtils.getApplicationContext());
        } else {
            sProviderImpl = new LocationProviderAndroid();
        }
        return sProviderImpl;
    }
}
