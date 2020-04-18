// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.support.annotation.Nullable;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.autofill.AutofillKeyboardSuggestions;
import org.chromium.chrome.browser.modelutil.ListObservable;
import org.chromium.chrome.browser.modelutil.PropertyObservable;
import org.chromium.ui.base.WindowAndroid;

/**
 * This is the second part of the controller of the keyboard accessory component.
 * It is responsible to update the {@link KeyboardAccessoryModel} based on Backend calls and notify
 * the Backend if the {@link KeyboardAccessoryModel} changes.
 * From the backend, it receives all actions that the accessory can perform (most prominently
 * generating passwords) and lets the {@link KeyboardAccessoryModel} know of these actions and which
 * callback to trigger when selecting them.
 */
class KeyboardAccessoryMediator
        implements WindowAndroid.KeyboardVisibilityListener, ListObservable.ListObserver,
                   PropertyObservable.PropertyObserver<KeyboardAccessoryModel.PropertyKey>,
                   KeyboardAccessoryData.Observer<KeyboardAccessoryData.Action> {
    private final KeyboardAccessoryModel mModel;
    private final WindowAndroid mWindowAndroid;

    // TODO(fhorschig): Look stronger signals than |keyboardVisibilityChanged|.
    // This variable remembers the last state of |keyboardVisibilityChanged| which might not be
    // sufficient for edge cases like hardware keyboards, floating keyboards, etc.
    private boolean mIsKeyboardVisible;

    KeyboardAccessoryMediator(KeyboardAccessoryModel model, WindowAndroid windowAndroid) {
        mModel = model;
        mWindowAndroid = windowAndroid;
        windowAndroid.addKeyboardVisibilityListener(this);

        // Add mediator as observer so it can use model changes as signal for accessory visibility.
        mModel.addObserver(this);
        mModel.getTabList().addObserver(this);
        mModel.getActionList().addObserver(this);
    }

    void destroy() {
        mWindowAndroid.removeKeyboardVisibilityListener(this);
    }

    @Override
    public void onItemsAvailable(KeyboardAccessoryData.Action[] actions) {
        mModel.setActions(actions);
    }

    @Override
    public void keyboardVisibilityChanged(boolean isShowing) {
        mIsKeyboardVisible = isShowing;
        updateVisibility();
    }

    void addTab(KeyboardAccessoryData.Tab tab) {
        mModel.addTab(tab);
    }

    void removeTab(KeyboardAccessoryData.Tab tab) {
        mModel.removeTab(tab);
    }

    void setSuggestions(AutofillKeyboardSuggestions suggestions) {
        mModel.setAutofillSuggestions(suggestions);
    }

    void dismiss() {
        if (mModel.getAutofillSuggestions() == null) return; // Nothing to do here.
        mModel.getAutofillSuggestions().dismiss();
        mModel.setAutofillSuggestions(null);
    }

    @VisibleForTesting
    KeyboardAccessoryModel getModelForTesting() {
        return mModel;
    }

    @Override
    public void onItemRangeInserted(ListObservable source, int index, int count) {
        assert source == mModel.getActionList() || source == mModel.getTabList();
        updateVisibility();
    }

    @Override
    public void onItemRangeRemoved(ListObservable source, int index, int count) {
        assert source == mModel.getActionList() || source == mModel.getTabList();
        updateVisibility();
    }

    @Override
    public void onItemRangeChanged(
            ListObservable source, int index, int count, @Nullable Object payload) {
        assert source == mModel.getActionList() || source == mModel.getTabList();
        updateVisibility();
    }

    @Override
    public void onPropertyChanged(PropertyObservable<KeyboardAccessoryModel.PropertyKey> source,
            @Nullable KeyboardAccessoryModel.PropertyKey propertyKey) {
        // Update the visibility only if we haven't set it just now.
        if (propertyKey == KeyboardAccessoryModel.PropertyKey.VISIBLE) return;
        if (propertyKey == KeyboardAccessoryModel.PropertyKey.SUGGESTIONS) {
            updateVisibility();
            return;
        }
        assert false : "Every property update needs to be handled explicitly!";
    }

    private boolean shouldShowAccessory() {
        if (!mIsKeyboardVisible) return false;
        return mModel.getAutofillSuggestions() != null || mModel.getActionList().getItemCount() > 0
                || mModel.getTabList().getItemCount() > 0;
    }

    private void updateVisibility() {
        mModel.setVisible(shouldShowAccessory());
    }
}
