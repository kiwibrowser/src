// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.shell;

import android.app.Application;

import org.chromium.base.CommandLine;

/**
 * The android_webview shell Application subclass.
 */
public class AwShellApplication extends Application {
    public void initCommandLine() {
        if (!CommandLine.isInitialized()) {
            CommandLine.initFromFile("/data/local/tmp/android-webview-command-line");
        }
    }
}
