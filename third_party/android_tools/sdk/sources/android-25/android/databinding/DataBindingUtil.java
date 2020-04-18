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

import android.app.Activity;
import android.support.annotation.Nullable;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;

/**
 * Utility class to create {@link ViewDataBinding} from layouts.
 */
public class DataBindingUtil {
    private static DataBinderMapper sMapper = new DataBinderMapper();
    private static DataBindingComponent sDefaultComponent = null;

    /**
     * Prevent DataBindingUtil from being instantiated.
     */
    private DataBindingUtil() {}

    /**
     * Set the default {@link DataBindingComponent} to use for data binding.
     * <p>
     * <code>bindingComponent</code> may be passed as the first parameter of binding adapters.
     * <p>
     * When instance method BindingAdapters are used, the class instance for the binding adapter
     * is retrieved from the DataBindingComponent.
     */
    public static void setDefaultComponent(DataBindingComponent bindingComponent) {
        sDefaultComponent = bindingComponent;
    }

    /**
     * Returns the default {@link DataBindingComponent} used in data binding. This can be
     * <code>null</code> if no default was set in
     * {@link #setDefaultComponent(DataBindingComponent)}.
     *
     * @return the default {@link DataBindingComponent} used in data binding. This can be
     * <code>null</code> if no default was set in
     * {@link #setDefaultComponent(DataBindingComponent)}.
     */
    public static DataBindingComponent getDefaultComponent() {
        return sDefaultComponent;
    }

    /**
     * Inflates a binding layout and returns the newly-created binding for that layout.
     * This uses the DataBindingComponent set in
     * {@link #setDefaultComponent(DataBindingComponent)}.
     * <p>
     * Use this version only if <code>layoutId</code> is unknown in advance. Otherwise, use
     * the generated Binding's inflate method to ensure type-safe inflation.
     *
     * @param inflater The LayoutInflater used to inflate the binding layout.
     * @param layoutId The layout resource ID of the layout to inflate.
     * @param parent Optional view to be the parent of the generated hierarchy
     *               (if attachToParent is true), or else simply an object that provides
     *               a set of LayoutParams values for root of the returned hierarchy
     *               (if attachToParent is false.)
     * @param attachToParent Whether the inflated hierarchy should be attached to the
     *                       parent parameter. If false, parent is only used to create
     *                       the correct subclass of LayoutParams for the root view in the XML.
     * @return The newly-created binding for the inflated layout or <code>null</code> if
     * the layoutId wasn't for a binding layout.
     * @throws InflateException When a merge layout was used and attachToParent was false.
     * @see #setDefaultComponent(DataBindingComponent)
     */
    public static <T extends ViewDataBinding> T inflate(LayoutInflater inflater, int layoutId,
            @Nullable ViewGroup parent, boolean attachToParent) {
        return inflate(inflater, layoutId, parent, attachToParent, sDefaultComponent);
    }

    /**
     * Inflates a binding layout and returns the newly-created binding for that layout.
     * <p>
     * Use this version only if <code>layoutId</code> is unknown in advance. Otherwise, use
     * the generated Binding's inflate method to ensure type-safe inflation.
     *
     * @param inflater The LayoutInflater used to inflate the binding layout.
     * @param layoutId The layout resource ID of the layout to inflate.
     * @param parent Optional view to be the parent of the generated hierarchy
     *               (if attachToParent is true), or else simply an object that provides
     *               a set of LayoutParams values for root of the returned hierarchy
     *               (if attachToParent is false.)
     * @param attachToParent Whether the inflated hierarchy should be attached to the
     *                       parent parameter. If false, parent is only used to create
     *                       the correct subclass of LayoutParams for the root view in the XML.
     * @param bindingComponent The DataBindingComponent to use in the binding.
     * @return The newly-created binding for the inflated layout or <code>null</code> if
     * the layoutId wasn't for a binding layout.
     * @throws InflateException When a merge layout was used and attachToParent was false.
     */
    public static <T extends ViewDataBinding> T inflate(
            LayoutInflater inflater, int layoutId, @Nullable ViewGroup parent,
            boolean attachToParent, DataBindingComponent bindingComponent) {
        final boolean useChildren = parent != null && attachToParent;
        final int startChildren = useChildren ? parent.getChildCount() : 0;
        final View view = inflater.inflate(layoutId, parent, attachToParent);
        if (useChildren) {
            return bindToAddedViews(bindingComponent, parent, startChildren, layoutId);
        } else {
            return bind(bindingComponent, view, layoutId);
        }
    }

    /**
     * Returns the binding for the given layout root or creates a binding if one
     * does not exist. This uses the DataBindingComponent set in
     * {@link #setDefaultComponent(DataBindingComponent)}.
     * <p>
     * Prefer using the generated Binding's <code>bind</code> method to ensure type-safe inflation
     * when it is known that <code>root</code> has not yet been bound.
     *
     * @param root The root View of the inflated binding layout.
     * @return A ViewDataBinding for the given root View. If one already exists, the
     * existing one will be returned.
     * @throws IllegalArgumentException when root is not from an inflated binding layout.
     * @see #getBinding(View)
     */
    @SuppressWarnings("unchecked")
    public static <T extends ViewDataBinding> T bind(View root) {
        return bind(root, sDefaultComponent);
    }

    /**
     * Returns the binding for the given layout root or creates a binding if one
     * does not exist.
     * <p>
     * Prefer using the generated Binding's <code>bind</code> method to ensure type-safe inflation
     * when it is known that <code>root</code> has not yet been bound.
     *
     * @param root The root View of the inflated binding layout.
     * @param bindingComponent The DataBindingComponent to use in data binding.
     * @return A ViewDataBinding for the given root View. If one already exists, the
     * existing one will be returned.
     * @throws IllegalArgumentException when root is not from an inflated binding layout.
     * @see #getBinding(View)
     */
    @SuppressWarnings("unchecked")
    public static <T extends ViewDataBinding> T bind(View root,
            DataBindingComponent bindingComponent) {
        T binding = getBinding(root);
        if (binding != null) {
            return binding;
        }
        Object tagObj = root.getTag();
        if (!(tagObj instanceof String)) {
            throw new IllegalArgumentException("View is not a binding layout");
        } else {
            String tag = (String) tagObj;
            int layoutId = sMapper.getLayoutId(tag);
            if (layoutId == 0) {
                throw new IllegalArgumentException("View is not a binding layout");
            }
            return (T) sMapper.getDataBinder(bindingComponent, root, layoutId);
        }
    }

    @SuppressWarnings("unchecked")
    static <T extends ViewDataBinding> T bind(DataBindingComponent bindingComponent, View[] roots,
            int layoutId) {
        return (T) sMapper.getDataBinder(bindingComponent, roots, layoutId);
    }

    static <T extends ViewDataBinding> T bind(DataBindingComponent bindingComponent, View root,
            int layoutId) {
        return (T) sMapper.getDataBinder(bindingComponent, root, layoutId);
    }

    /**
     * Retrieves the binding responsible for the given View. If <code>view</code> is not a
     * binding layout root, its parents will be searched for the binding. If there is no binding,
     * <code>null</code> will be returned.
     * <p>
     * This differs from {@link #getBinding(View)} in that findBinding takes any view in the
     * layout and searches for the binding associated with the root. <code>getBinding</code>
     * takes only the root view.
     *
     * @param view A <code>View</code> in the bound layout.
     * @return The ViewDataBinding associated with the given view or <code>null</code> if
     * view is not part of a bound layout.
     */
    public static <T extends ViewDataBinding> T findBinding(View view) {
        while (view != null) {
            ViewDataBinding binding = ViewDataBinding.getBinding(view);
            if (binding != null) {
                return (T) binding;
            }
            Object tag = view.getTag();
            if (tag instanceof String) {
                String tagString = (String) tag;
                if (tagString.startsWith("layout") && tagString.endsWith("_0")) {
                    final char nextChar = tagString.charAt(6);
                    final int slashIndex = tagString.indexOf('/', 7);
                    boolean isUnboundRoot = false;
                    if (nextChar == '/') {
                        // only one slash should exist
                        isUnboundRoot = slashIndex == -1;
                    } else if (nextChar == '-' && slashIndex != -1) {
                        int nextSlashIndex = tagString.indexOf('/', slashIndex + 1);
                        // only one slash should exist
                        isUnboundRoot = nextSlashIndex == -1;
                    }
                    if (isUnboundRoot) {
                        // An inflated, but unbound layout
                        return null;
                    }
                }
            }
            ViewParent viewParent = view.getParent();
            if (viewParent instanceof View) {
                view = (View) viewParent;
            } else {
                view = null;
            }
        }
        return null;
    }

    /**
     * Retrieves the binding responsible for the given View layout root. If there is no binding,
     * <code>null</code> will be returned. This uses the DataBindingComponent set in
     * {@link #setDefaultComponent(DataBindingComponent)}.
     *
     * @param view The root <code>View</code> in the layout with binding.
     * @return The ViewDataBinding associated with the given view or <code>null</code> if
     * either the view is not a root View for a layout or view hasn't been bound.
     */
    public static <T extends ViewDataBinding> T getBinding(View view) {
        return (T) ViewDataBinding.getBinding(view);
    }

    /**
     * Set the Activity's content view to the given layout and return the associated binding.
     * The given layout resource must not be a merge layout.
     *
     * @param activity The Activity whose content View should change.
     * @param layoutId The resource ID of the layout to be inflated, bound, and set as the
     *                 Activity's content.
     * @return The binding associated with the inflated content view.
     */
    public static <T extends ViewDataBinding> T setContentView(Activity activity, int layoutId) {
        return setContentView(activity, layoutId, sDefaultComponent);
    }

    /**
     * Set the Activity's content view to the given layout and return the associated binding.
     * The given layout resource must not be a merge layout.
     *
     * @param bindingComponent The DataBindingComponent to use in data binding.
     * @param activity The Activity whose content View should change.
     * @param layoutId The resource ID of the layout to be inflated, bound, and set as the
     *                 Activity's content.
     * @return The binding associated with the inflated content view.
     */
    public static <T extends ViewDataBinding> T setContentView(Activity activity, int layoutId,
            DataBindingComponent bindingComponent) {
        activity.setContentView(layoutId);
        View decorView = activity.getWindow().getDecorView();
        ViewGroup contentView = (ViewGroup) decorView.findViewById(android.R.id.content);
        return bindToAddedViews(bindingComponent, contentView, 0, layoutId);
    }

    /**
     * Converts the given BR id to its string representation which might be useful for logging
     * purposes.
     *
     * @param id The integer id, which should be a field from BR class.
     * @return The name if the BR id or null if id is out of bounds.
     */
    public static String convertBrIdToString(int id) {
        return sMapper.convertBrIdToString(id);
    }

    private static <T extends ViewDataBinding> T bindToAddedViews(DataBindingComponent component,
            ViewGroup parent, int startChildren, int layoutId) {
        final int endChildren = parent.getChildCount();
        final int childrenAdded = endChildren - startChildren;
        if (childrenAdded == 1) {
            final View childView = parent.getChildAt(endChildren - 1);
            return bind(component, childView, layoutId);
        } else {
            final View[] children = new View[childrenAdded];
            for (int i = 0; i < childrenAdded; i++) {
                children[i] = parent.getChildAt(i + startChildren);
            }
            return bind(component, children, layoutId);
        }
    }
}
