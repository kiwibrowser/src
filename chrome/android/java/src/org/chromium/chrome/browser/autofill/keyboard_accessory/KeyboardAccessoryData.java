// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import android.graphics.drawable.Drawable;
import android.support.annotation.LayoutRes;
import android.support.annotation.Nullable;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

/**
 * Interfaces in this class are used to pass data into keyboard accessory component.
 */
public class KeyboardAccessoryData {
    /**
     * A provider notifies all registered {@link Observer} if the list of actions
     * changes.
     * TODO(fhorschig): Replace with android.databinding.ObservableField if available.
     * @param <T> Either an {@link Action} or a {@link Tab} that this instance provides.
     */
    public interface Provider<T> {
        /**
         * Every observer added by this need to be notified whenever the list of items changes.
         * @param observer The observer to be notified.
         */
        void addObserver(Observer<T> observer);
    }

    /**
     * An observer receives notifications from an {@link Provider} it is subscribed to.
     * @param <T> Either an {@link Action} or a {@link Tab} that this instance observes.
     */
    public interface Observer<T> {
        /**
         * A provider calls this function with a list of items that should be available in the
         * keyboard accessory.
         * @param actions The actions to be displayed in the Accessory. It's a native array as the
         *                provider is typically a bridge called via JNI which prefers native types.
         */
        void onItemsAvailable(T[] actions);
    }

    /**
     * Describes a tab which should be displayed as a small icon at the start of the keyboard
     * accessory. Typically, a tab is responsible to change the bottom sheet below the accessory.
     */
    public interface Tab {
        /**
         * A Tab's Listener get's notified when e.g. the Tab was assigned a view.
         */
        interface Listener {
            /**
             * Triggered when the tab was successfully created.
             * @param view The newly created accessory sheet of the tab.
             */
            void onTabCreated(ViewGroup view);
        }

        /**
         * Provides the icon that will be displayed in the {@link KeyboardAccessoryCoordinator}.
         * @return The small icon that identifies this tab uniquely.
         */
        Drawable getIcon();

        /**
         * The description for this tab. It will become the content description of the icon.
         * @return A short string describing the task of this tab.
         */
        String getContentDescription();

        /**
         * Returns the tab layout which allows to create the tab's view on demand.
         * @return The layout resource that allows to create the view necessary for this tab.
         */
        @LayoutRes
        int getTabLayout();

        /**
         * Returns the listener which might need to react on changes to this tab.
         * @return A {@link Listener} to be called, e.g. when the tab is created.
         */
        @Nullable
        Listener getListener();
    }

    /**
     * This describes an action that can be invoked from the keyboard accessory.
     * The most prominent example hereof is the "Generate Password" action.
     */
    public interface Action {
        /**
         * The delegate is called when the Action is triggered by a user.
         */
        interface Delegate {
            /**
             * When this function is called, a user interacted with the passed action.
             * @param action The action that the user interacted with.
             */
            void onActionTriggered(Action action);
        }
        String getCaption();
        Delegate getDelegate();
    }

    /**
     * A simple class that holds a list of {@link Observer}s which can be notified about new data by
     * directly passing that data into {@link PropertyProvider#notifyObservers(T[])}.
     * @param <T> Either {@link Action}s or {@link Tab}s provided by this class.
     */
    public static class PropertyProvider<T> implements Provider<T> {
        private final List<Observer<T>> mObservers = new ArrayList<>();

        @Override
        public void addObserver(Observer<T> observer) {
            mObservers.add(observer);
        }

        /**
         * Passes the given items to all subscribed {@link Observer}s.
         * @param items The array of items to be passed to the {@link Observer}s.
         */
        public void notifyObservers(T[] items) {
            for (Observer<T> observer : mObservers) {
                observer.onItemsAvailable(items);
            }
        }
    }

    private KeyboardAccessoryData() {}
}
