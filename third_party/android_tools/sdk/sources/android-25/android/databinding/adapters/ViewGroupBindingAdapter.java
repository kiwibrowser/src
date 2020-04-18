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
package android.databinding.adapters;

import android.animation.LayoutTransition;
import android.annotation.TargetApi;
import android.databinding.BindingAdapter;
import android.databinding.BindingMethod;
import android.databinding.BindingMethods;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.OnHierarchyChangeListener;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;

@BindingMethods({
        @BindingMethod(type = android.view.ViewGroup.class, attribute = "android:alwaysDrawnWithCache", method = "setAlwaysDrawnWithCacheEnabled"),
        @BindingMethod(type = android.view.ViewGroup.class, attribute = "android:animationCache", method = "setAnimationCacheEnabled"),
        @BindingMethod(type = android.view.ViewGroup.class, attribute = "android:splitMotionEvents", method = "setMotionEventSplittingEnabled"),
})
public class ViewGroupBindingAdapter {

    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    @BindingAdapter({"android:animateLayoutChanges"})
    public static void setAnimateLayoutChanges(ViewGroup view, boolean animate) {
        if (animate) {
            view.setLayoutTransition(new LayoutTransition());
        } else {
            view.setLayoutTransition(null);
        }
    }

    @BindingAdapter(value = {"android:onChildViewAdded", "android:onChildViewRemoved"},
            requireAll = false)
    public static void setListener(ViewGroup view, final OnChildViewAdded added,
            final OnChildViewRemoved removed) {
        if (added == null && removed == null) {
            view.setOnHierarchyChangeListener(null);
        } else {
            view.setOnHierarchyChangeListener(new OnHierarchyChangeListener() {
                @Override
                public void onChildViewAdded(View parent, View child) {
                    if (added != null) {
                        added.onChildViewAdded(parent, child);
                    }
                }

                @Override
                public void onChildViewRemoved(View parent, View child) {
                    if (removed != null) {
                        removed.onChildViewRemoved(parent, child);
                    }
                }
            });
        }
    }

    @BindingAdapter(value = {"android:onAnimationStart", "android:onAnimationEnd",
            "android:onAnimationRepeat"}, requireAll = false)
    public static void setListener(ViewGroup view, final OnAnimationStart start,
            final OnAnimationEnd end, final OnAnimationRepeat repeat) {
        if (start == null && end == null && repeat == null) {
            view.setLayoutAnimationListener(null);
        } else {
            view.setLayoutAnimationListener(new AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {
                    if (start != null) {
                        start.onAnimationStart(animation);
                    }
                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    if (end != null) {
                        end.onAnimationEnd(animation);
                    }
                }

                @Override
                public void onAnimationRepeat(Animation animation) {
                    if (repeat != null) {
                        repeat.onAnimationRepeat(animation);
                    }
                }
            });
        }
    }

    public interface OnChildViewAdded {
        void onChildViewAdded(View parent, View child);
    }

    public interface OnChildViewRemoved {
        void onChildViewRemoved(View parent, View child);
    }

    public interface OnAnimationStart {
        void onAnimationStart(Animation animation);
    }

    public interface OnAnimationEnd {
        void onAnimationEnd(Animation animation);
    }

    public interface OnAnimationRepeat {
        void onAnimationRepeat(Animation animation);
    }
}
