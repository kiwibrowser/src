// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.os.Bundle;

import org.chromium.base.ContextUtils;
import org.chromium.base.library_loader.LibraryProcessType;

/**
 * Allows specifying the package name for looking up child services
 * configuration and classes into (if it differs from the application package
 * name, like in the case of Android WebView). Also allows specifying additional
 * child service binging flags.
 */
public class ChildProcessCreationParams {
    private static final String EXTRA_LIBRARY_PROCESS_TYPE =
            "org.chromium.content.common.child_service_params.library_process_type";

    private static ChildProcessCreationParams sParams;

    /** Set params. This should be called once on start up. */
    public static void set(ChildProcessCreationParams params) {
        assert sParams == null;
        sParams = params;
    }

    public static ChildProcessCreationParams get() {
        return sParams;
    }

    // Members should all be immutable to avoid worrying about thread safety.
    private final String mPackageNameForService;
    private final boolean mIsSandboxedServiceExternal;
    private final int mLibraryProcessType;
    private final boolean mBindToCallerCheck;
    // Use only the explicit WebContents.setImportance signal, and ignore other implicit
    // signals in content.
    private final boolean mIgnoreVisibilityForImportance;

    public ChildProcessCreationParams(String packageNameForService,
            boolean isExternalSandboxedService, int libraryProcessType, boolean bindToCallerCheck,
            boolean ignoreVisibilityForImportance) {
        mPackageNameForService = packageNameForService;
        mIsSandboxedServiceExternal = isExternalSandboxedService;
        mLibraryProcessType = libraryProcessType;
        mBindToCallerCheck = bindToCallerCheck;
        mIgnoreVisibilityForImportance = ignoreVisibilityForImportance;
    }

    public void addIntentExtras(Bundle extras) {
        extras.putInt(EXTRA_LIBRARY_PROCESS_TYPE, mLibraryProcessType);
    }

    public static String getPackageNameForService() {
        ChildProcessCreationParams params = ChildProcessCreationParams.get();
        return params != null ? params.mPackageNameForService
                              : ContextUtils.getApplicationContext().getPackageName();
    }

    public static boolean getIsSandboxedServiceExternal() {
        ChildProcessCreationParams params = ChildProcessCreationParams.get();
        return params != null && params.mIsSandboxedServiceExternal;
    }

    public static boolean getBindToCallerCheck() {
        ChildProcessCreationParams params = ChildProcessCreationParams.get();
        return params != null && params.mBindToCallerCheck;
    }

    public static boolean getIgnoreVisibilityForImportance() {
        ChildProcessCreationParams params = ChildProcessCreationParams.get();
        return params != null && params.mIgnoreVisibilityForImportance;
    }

    public static int getLibraryProcessType(Bundle extras) {
        return extras.getInt(EXTRA_LIBRARY_PROCESS_TYPE, LibraryProcessType.PROCESS_CHILD);
    }
}
