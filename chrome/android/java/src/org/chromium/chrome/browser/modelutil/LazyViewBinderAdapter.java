// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modelutil;

import android.view.View;
import android.view.ViewStub;

import javax.annotation.Nullable;

/**
 * Observes updates to a model {@link M} and calls a given {@link SimpleViewBinder} with a lazily
 * inflated view whenever a property with key {@link K} has changed.
 * The view used by this class is passed in as a {@link ViewStub} wrapped in a {@link StubHolder}.
 * The held stub is inflated when the observed model becomes visible for the first time.
 * @param <M> The observable model that contains the data which should be should to the view.
 * @param <V> The type of the fully inflated ViewStub.
 * @param <K> The type of property keys which are published by model {@link M}.
 */
public class LazyViewBinderAdapter<M extends PropertyObservable<K>, V extends View, K>
        implements PropertyModelChangeProcessor
                           .ViewBinder<M, LazyViewBinderAdapter.StubHolder<V>, K> {
    /**
     *
     * Holds a {@link ViewStub} that is inflated by the {@link LazyViewBinderAdapter} which calls a
     * {@link SimpleViewBinder}.
     * @param <V> The View type that the StubHolder holds.
     */
    public static class StubHolder<V extends View> {
        private @Nullable V mView; // Remains null until |mViewStub| is inflated.
        private final ViewStub mViewStub;

        public StubHolder(ViewStub viewStub) {
            mViewStub = viewStub;
        }

        @Nullable
        public V getView() {
            return mView;
        }

        @SuppressWarnings("unchecked") // The StubHolder always holds a V!
        void inflateView() {
            mView = (V) mViewStub.inflate();
        }
    }

    /**
     * A {@link PropertyModelChangeProcessor.ViewBinder} that provides information about when to
     * inflate a viewStub managed by a {@link LazyViewBinderAdapter}.
     * @param <M> The model holding data about the view and it's visibility.
     * @param <V> The type of the fully inflated ViewStub.
     * @param <K> The class of the properties that model {@link M}  propagates.
     */
    public interface SimpleViewBinder<M, V extends View, K>
            extends PropertyModelChangeProcessor.ViewBinder<M, V, K> {
        /**
         * @return The Property that will indicate when this component becomes visible.
         */
        K getVisibilityProperty();

        /**
         * Returns whether this component should be visible now. The view binder will inflate the
         * given stub if this returns true when the {@link SimpleViewBinder#getVisibilityProperty}
         * is updated.
         * @param model The model that should contain an observable property about the visibility.
         * @return whether this component should be visible now.
         */
        boolean isVisible(M model);

        /**
         * Called when the lazy inflation of the view happens.
         * @param model The model which might be useful for initializing the just inflated view.
         * @param inflatedView The freshly inflated view.
         */
        void onInitialInflation(M model, V inflatedView);
    }

    private final SimpleViewBinder<M, V, K> mBinder;

    /**
     * Creates the adapter with a {@link SimpleViewBinder} that will be used to determine when to
     * inflate the {@link StubHolder} that is passed in by the {@link PropertyModelChangeProcessor}.
     * @param binder The binder to be called with the fully inflated view.
     */
    public LazyViewBinderAdapter(SimpleViewBinder<M, V, K> binder) {
        mBinder = binder;
    }

    @Override
    public void bind(M model, StubHolder<V> stubHolder, K propertyKey) {
        if (stubHolder.getView() == null) {
            if (propertyKey != mBinder.getVisibilityProperty() || !mBinder.isVisible(model)) {
                return; // Ignore model changes before the view is shown for the first time.
            }
            stubHolder.inflateView();
            mBinder.onInitialInflation(model, stubHolder.getView());
        }
        mBinder.bind(model, stubHolder.getView(), propertyKey);
    }
}
