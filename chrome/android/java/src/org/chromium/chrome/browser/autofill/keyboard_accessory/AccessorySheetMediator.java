// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.annotation.Nullable;

import org.chromium.base.VisibleForTesting;

/**
 * Contains the controller logic of the AccessorySheet component.
 * It communicates with data providers and native backends to update a {@link AccessorySheetModel}.
 */
class AccessorySheetMediator {
    private final AccessorySheetModel mModel;

    AccessorySheetMediator(AccessorySheetModel model) {
        mModel = model;
    }

    @Nullable
    KeyboardAccessoryData.Tab getTab() {
        if (mModel.getActiveTabIndex() == AccessorySheetModel.NO_ACTIVE_TAB) return null;
        return mModel.getTabList().get(mModel.getActiveTabIndex());
    }

    @VisibleForTesting
    AccessorySheetModel getModelForTesting() {
        return mModel;
    }

    public void show() {
        mModel.setVisible(true);
    }

    public void hide() {
        mModel.setVisible(false);
    }

    public void addTab(KeyboardAccessoryData.Tab tab) {
        mModel.getTabList().add(tab);
        if (mModel.getActiveTabIndex() == AccessorySheetModel.NO_ACTIVE_TAB) {
            mModel.setActiveTabIndex(mModel.getTabList().getItemCount() - 1);
        }
    }
}