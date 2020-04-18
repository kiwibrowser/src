// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.view.ViewStub;

import org.chromium.base.VisibleForTesting;
import org.chromium.ui.base.WindowAndroid;

/**
 * Handles requests to the manual UI for filling passwords, payments and other user data. Ideally,
 * the caller has no access to Keyboard accessory or sheet and is only interacting with this
 * component.
 * For that, it facilitates the communication between {@link KeyboardAccessoryCoordinator} and
 * {@link AccessorySheetCoordinator} to add and trigger surfaces that may assist users while filling
 * fields.
 */
public class ManualFillingCoordinator {
    private final KeyboardAccessoryCoordinator mKeyboardAccessory;
    private final AccessorySheetCoordinator mAccessorySheet;

    /**
     * Creates a the manual filling controller.
     * @param windowAndroid The window needed to set up the sub components.
     * @param keyboardAccessoryStub The view stub for keyboard accessory bar.
     * @param accessorySheetStub The view stub for the keyboard accessory bottom sheet.
     */
    public ManualFillingCoordinator(WindowAndroid windowAndroid, ViewStub keyboardAccessoryStub,
            ViewStub accessorySheetStub) {
        mKeyboardAccessory = new KeyboardAccessoryCoordinator(windowAndroid, keyboardAccessoryStub);
        mAccessorySheet = new AccessorySheetCoordinator(accessorySheetStub);
    }

    /**
     * Cleans up the manual UI by destroying the accessory bar and its bottom sheet.
     */
    public void destroy() {
        mKeyboardAccessory.destroy();
    }

    /**
     * Links a tab to the manual UI by adding it to the held {@link AccessorySheetCoordinator} and
     * the {@link KeyboardAccessoryCoordinator}.
     * @param tab The tab component to be added.
     */
    public void addTab(KeyboardAccessoryData.Tab tab) {
        mKeyboardAccessory.addTab(tab);
        mAccessorySheet.addTab(tab);
    }

    /**
     * Allows access to the keyboard accessory. This can be used to explicitly modify the the bar of
     * the keyboard accessory (e.g. by providing suggestions or actions).
     * @return The coordinator of the Keyboard accessory component.
     */
    public KeyboardAccessoryCoordinator getKeyboardAccessory() {
        return mKeyboardAccessory;
    }

    /**
     * Allows access to the accessory sheet.
     * @return The coordinator of the Accessory sheet component.
     */
    @VisibleForTesting
    public AccessorySheetCoordinator getAccessorySheetForTesting() {
        return mAccessorySheet;
    }
}