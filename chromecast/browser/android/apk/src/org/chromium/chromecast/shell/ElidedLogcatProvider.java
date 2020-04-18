// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;

import java.io.BufferedReader;
import java.io.IOException;

/**
 * Extracts logcat out of Android devices and elide PII sensitive info from it.
 *
 * <p>Elided information includes: Emails, IP address, MAC address, URL/domains as well as
 * Javascript console messages.
 */
abstract class ElidedLogcatProvider {
    private static final String TAG = "cr_ElidedLogcatProvider";

    protected abstract void getRawLogcat(RawLogcatCallback rawLogcatCallback);

    protected interface RawLogcatCallback { public void onLogsDone(BufferedReader logsFileReader); }
    public interface LogcatCallback { public void onLogsDone(String logs); }

    public void getElidedLogcat(LogcatCallback callback) {
        getRawLogcat((BufferedReader logsFileReader)
                             -> callback.onLogsDone(elideLogcat(logsFileReader)));
    }

    @VisibleForTesting
    protected static String elideLogcat(BufferedReader logsFileReader) {
        StringBuilder builder = new StringBuilder();
        try (BufferedReader autoClosableBufferedReader = logsFileReader) {
            String logLn;
            while ((logLn = autoClosableBufferedReader.readLine()) != null) {
                builder.append(LogcatElision.elide(logLn + "\n"));
            }
        } catch (IOException e) {
            Log.e(TAG, "Can't read logs", e);
        } finally {
            return builder.toString();
        }
    }
}