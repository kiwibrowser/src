// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modelutil;

import org.chromium.chrome.browser.modelutil.PropertyObservable.PropertyObserver;

/**
 * A model change processor for use with a {@link PropertyObservable} model. The
 * {@link PropertyModelChangeProcessor} should be registered as a property observer of the model.
 * Internally uses a view binder to bind model properties to the toolbar view.
 * @param <M> The {@link PropertyObservable} model.
 * @param <V> The view object that is changing.
 * @param <P> The property of the view that changed.
 */
public class PropertyModelChangeProcessor<M extends PropertyObservable<P>, V, P>
        implements PropertyObserver<P> {
    /**
     * A generic view binder that associates a view with a model.
     * @param <M> The {@link PropertyObservable} model.
     * @param <V> The view object that is changing.
     * @param <P> The property of the view that changed.
     */
    public interface ViewBinder<M, V, P> { void bind(M model, V view, P propertyKey); }

    private final V mView;
    private final M mModel;
    private final ViewBinder<M, V, P> mViewBinder;

    /**
     * Construct a new PropertyModelChangeProcessor.
     * @param model The model containing the data to be bound.
     * @param view The view to which data will be bound.
     * @param viewBinder A class that binds the model to the view.
     */
    public PropertyModelChangeProcessor(M model, V view, ViewBinder<M, V, P> viewBinder) {
        mModel = model;
        mView = view;
        mViewBinder = viewBinder;
    }

    @Override
    public void onPropertyChanged(PropertyObservable<P> source, P propertyKey) {
        // TODO(bauerb): Add support for batching and for full model updates.
        mViewBinder.bind(mModel, mView, propertyKey);
    }
}