/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.databinding;

/**
 * Listener set on {@link ViewDataBinding#addOnRebindCallback(OnRebindCallback)} that
 * is called when bound values must be reevaluated in {@link
 * ViewDataBinding#executePendingBindings()}.
 */
public abstract class OnRebindCallback<T extends ViewDataBinding> {

    /**
     * Called when values in a ViewDataBinding should be reevaluated. This does not
     * mean that values will actually change, but only that something in the data
     * model that affects the bindings has been perturbed.
     * <p>
     * Return true to allow the reevaluation to happen or false if the reevaluation
     * should be stopped. If false is returned, it is the responsibility of the
     * OnRebindListener implementer to explicitly call {@link
     * ViewDataBinding#executePendingBindings()}.
     * <p>
     * The default implementation only returns <code>true</code>.
     *
     * @param binding The ViewDataBinding that is reevaluating its bound values.
     * @return true to indicate that the reevaluation should continue or false to
     * halt evaluation.
     */
    public boolean onPreBind(T binding) {
        return true;
    }

    /**
     * Called after all callbacks have completed {@link #onPreBind(ViewDataBinding)} when
     * one or more of the calls has returned <code>false</code>.
     * <p>
     * The default implementation does nothing.
     *
     * @param binding The ViewDataBinding that is reevaluating its bound values.
     */
    public void onCanceled(T binding) {
    }

    /**
     * Called after values have been reevaluated in {@link
     * ViewDataBinding#executePendingBindings()}. This is only called if all listeners have
     * returned true from {@link #onPreBind(ViewDataBinding)}.
     * <p>
     * The default implementation does nothing.
     *
     * @param binding The ViewDataBinding that is reevaluating its bound values.
     */
    public void onBound(T binding) {
    }
}
