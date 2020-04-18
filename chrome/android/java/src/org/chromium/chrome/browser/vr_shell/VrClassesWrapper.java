// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;

import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

/**
 * Abstracts away the VrClassesWrapperImpl class, which may or may not be present at runtime
 * depending on compile flags.
 */
public interface VrClassesWrapper {
    /**
     * Creates a VrShellImpl instance.
     */
    public VrShell createVrShell(
            ChromeActivity activity, VrShellDelegate delegate, TabModelSelector tabModelSelector);

    /**
     * Creates a VrDaydreamApImpl instance.
     */
    public VrDaydreamApi createVrDaydreamApi();

    /**
    * Creates a VrCoreVersionCheckerImpl instance.
    */
    public VrCoreVersionChecker createVrCoreVersionChecker();

    // We put statics that are behind compile flags here to avoid unnecessarily creating DaydreamApi
    // instances.

    /**
     * Sets VR Mode to |enabled|.
     */
    public void setVrModeEnabled(Activity activity, boolean enabled);

    /**
     * @return Whether the current device is Daydream Ready.
     */
    boolean isDaydreamReadyDevice();

    /**
     * @return Whether this device boots directly into VR mode. May be used to detect standalone VR
     * devices.
     */
    boolean bootsToVr();

    /**
     * Adds the necessary VR flags to an intent.
     * @param intent The intent to add VR flags to.
     * @return the intent with VR flags set.
     */
    Intent setupVrIntent(Intent intent);

    /**
     * Create an Intent to launch a VR activity with the given component name.
     */
    Intent createVrIntent(final ComponentName componentName);

    /**
     * Launch the Daydream Settings Activity.
     */
    void launchGvrSettings(Activity activity);

    /**
     * Whether the user is currently in a VR session. Note that this should be treated as heuristic,
     * and may at times be wrong as there's a timeout before this state is cleared after the user
     * has taken off their headset.
     */
    boolean isInVrSession();

    /**
     * @return Whether 2D-in-VR rendering mode is currently supported.
     */
    boolean supports2dInVr();
}
