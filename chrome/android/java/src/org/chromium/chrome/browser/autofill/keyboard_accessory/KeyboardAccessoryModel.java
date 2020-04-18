// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import org.chromium.chrome.browser.autofill.AutofillKeyboardSuggestions;
import org.chromium.chrome.browser.modelutil.ListObservable;
import org.chromium.chrome.browser.modelutil.PropertyObservable;
import org.chromium.chrome.browser.modelutil.SimpleListObservable;

import java.util.ArrayList;
import java.util.List;

/**
 * As model of the keyboard accessory component, this class holds the data relevant to the visual
 * state of the accessory.
 * This includes the visibility of the accessory in general, any available tabs and actions.
 * Whenever the state changes, it notifies its listeners - like the
 * {@link KeyboardAccessoryMediator} or the ModelChangeProcessor.
 */
class KeyboardAccessoryModel extends PropertyObservable<KeyboardAccessoryModel.PropertyKey> {
    /** Keys uniquely identifying model properties. */
    static class PropertyKey {
        // This list contains all properties and is only necessary because the view needs to
        // iterate over all properties when it's created to ensure it is in sync with this model.
        static final List<PropertyKey> ALL_PROPERTIES = new ArrayList<>();

        static final PropertyKey VISIBLE = new PropertyKey();
        static final PropertyKey SUGGESTIONS = new PropertyKey();

        private PropertyKey() {
            ALL_PROPERTIES.add(this);
        }
    }

    private SimpleListObservable<KeyboardAccessoryData.Action> mActionListObservable;
    private SimpleListObservable<KeyboardAccessoryData.Tab> mTabListObservable;
    private boolean mVisible;

    // TODO(fhorschig): Ideally, make this a ListObservable populating a RecyclerView.
    private AutofillKeyboardSuggestions mAutofillSuggestions;

    KeyboardAccessoryModel() {
        mActionListObservable = new SimpleListObservable<>();
        mTabListObservable = new SimpleListObservable<>();
    }

    void addActionListObserver(ListObservable.ListObserver observer) {
        mActionListObservable.addObserver(observer);
    }

    void setActions(KeyboardAccessoryData.Action[] actions) {
        mActionListObservable.set(actions);
    }

    SimpleListObservable<KeyboardAccessoryData.Action> getActionList() {
        return mActionListObservable;
    }

    void addTabListObserver(ListObservable.ListObserver observer) {
        mTabListObservable.addObserver(observer);
    }

    void addTab(KeyboardAccessoryData.Tab tab) {
        mTabListObservable.add(tab);
    }

    void removeTab(KeyboardAccessoryData.Tab tab) {
        mTabListObservable.remove(tab);
    }

    SimpleListObservable<KeyboardAccessoryData.Tab> getTabList() {
        return mTabListObservable;
    }

    void setVisible(boolean visible) {
        if (mVisible == visible) return; // Nothing to do here: same value.
        mVisible = visible;
        notifyPropertyChanged(PropertyKey.VISIBLE);
    }

    boolean isVisible() {
        return mVisible;
    }

    AutofillKeyboardSuggestions getAutofillSuggestions() {
        return mAutofillSuggestions;
    }

    void setAutofillSuggestions(AutofillKeyboardSuggestions autofillSuggestions) {
        if (autofillSuggestions == mAutofillSuggestions) return; // Nothing to do: same object.
        mAutofillSuggestions = autofillSuggestions;
        notifyPropertyChanged(PropertyKey.SUGGESTIONS);
    }
}
