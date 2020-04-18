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

import android.annotation.TargetApi;
import android.databinding.BindingAdapter;
import android.databinding.BindingMethod;
import android.databinding.BindingMethods;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import com.android.databinding.library.baseAdapters.R;

@BindingMethods({
        @BindingMethod(type = View.class, attribute = "android:backgroundTint", method = "setBackgroundTintList"),
        @BindingMethod(type = View.class, attribute = "android:fadeScrollbars", method = "setScrollbarFadingEnabled"),
        @BindingMethod(type = View.class, attribute = "android:getOutline", method = "setOutlineProvider"),
        @BindingMethod(type = View.class, attribute = "android:nextFocusForward", method = "setNextFocusForwardId"),
        @BindingMethod(type = View.class, attribute = "android:nextFocusLeft", method = "setNextFocusLeftId"),
        @BindingMethod(type = View.class, attribute = "android:nextFocusRight", method = "setNextFocusRightId"),
        @BindingMethod(type = View.class, attribute = "android:nextFocusUp", method = "setNextFocusUpId"),
        @BindingMethod(type = View.class, attribute = "android:nextFocusDown", method = "setNextFocusDownId"),
        @BindingMethod(type = View.class, attribute = "android:requiresFadingEdge", method = "setVerticalFadingEdgeEnabled"),
        @BindingMethod(type = View.class, attribute = "android:scrollbarDefaultDelayBeforeFade", method = "setScrollBarDefaultDelayBeforeFade"),
        @BindingMethod(type = View.class, attribute = "android:scrollbarFadeDuration", method = "setScrollBarFadeDuration"),
        @BindingMethod(type = View.class, attribute = "android:scrollbarSize", method = "setScrollBarSize"),
        @BindingMethod(type = View.class, attribute = "android:scrollbarStyle", method = "setScrollBarStyle"),
        @BindingMethod(type = View.class, attribute = "android:transformPivotX", method = "setPivotX"),
        @BindingMethod(type = View.class, attribute = "android:transformPivotY", method = "setPivotY"),
        @BindingMethod(type = View.class, attribute = "android:onDrag", method = "setOnDragListener"),
        @BindingMethod(type = View.class, attribute = "android:onClick", method = "setOnClickListener"),
        @BindingMethod(type = View.class, attribute = "android:onApplyWindowInsets", method = "setOnApplyWindowInsetsListener"),
        @BindingMethod(type = View.class, attribute = "android:onCreateContextMenu", method = "setOnCreateContextMenuListener"),
        @BindingMethod(type = View.class, attribute = "android:onFocusChange", method = "setOnFocusChangeListener"),
        @BindingMethod(type = View.class, attribute = "android:onGenericMotion", method = "setOnGenericMotionListener"),
        @BindingMethod(type = View.class, attribute = "android:onHover", method = "setOnHoverListener"),
        @BindingMethod(type = View.class, attribute = "android:onKey", method = "setOnKeyListener"),
        @BindingMethod(type = View.class, attribute = "android:onLongClick", method = "setOnLongClickListener"),
        @BindingMethod(type = View.class, attribute = "android:onSystemUiVisibilityChange", method = "setOnSystemUiVisibilityChangeListener"),
        @BindingMethod(type = View.class, attribute = "android:onTouch", method = "setOnTouchListener"),
})
public class ViewBindingAdapter {
    public static int FADING_EDGE_NONE = 0;
    public static int FADING_EDGE_HORIZONTAL = 1;
    public static int FADING_EDGE_VERTICAL = 2;

    @BindingAdapter({"android:padding"})
    public static void setPadding(View view, float paddingFloat) {
        final int padding = pixelsToDimensionPixelSize(paddingFloat);
        view.setPadding(padding, padding, padding, padding);
    }

    @BindingAdapter({"android:paddingBottom"})
    public static void setPaddingBottom(View view, float paddingFloat) {
        final int padding = pixelsToDimensionPixelSize(paddingFloat);
        view.setPadding(view.getPaddingLeft(), view.getPaddingTop(), view.getPaddingRight(),
                padding);
    }

    @BindingAdapter({"android:paddingEnd"})
    public static void setPaddingEnd(View view, float paddingFloat) {
        final int padding = pixelsToDimensionPixelSize(paddingFloat);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            view.setPaddingRelative(view.getPaddingStart(), view.getPaddingTop(), padding,
                    view.getPaddingBottom());
        } else {
            view.setPadding(view.getPaddingLeft(), view.getPaddingTop(), padding,
                    view.getPaddingBottom());
        }
    }

    @BindingAdapter({"android:paddingLeft"})
    public static void setPaddingLeft(View view, float paddingFloat) {
        final int padding = pixelsToDimensionPixelSize(paddingFloat);
        view.setPadding(padding, view.getPaddingTop(), view.getPaddingRight(),
                view.getPaddingBottom());
    }

    @BindingAdapter({"android:paddingRight"})
    public static void setPaddingRight(View view, float paddingFloat) {
        final int padding = pixelsToDimensionPixelSize(paddingFloat);
        view.setPadding(view.getPaddingLeft(), view.getPaddingTop(), padding,
                view.getPaddingBottom());
    }

    @BindingAdapter({"android:paddingStart"})
    public static void setPaddingStart(View view, float paddingFloat) {
        final int padding = pixelsToDimensionPixelSize(paddingFloat);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            view.setPaddingRelative(padding, view.getPaddingTop(), view.getPaddingEnd(),
                    view.getPaddingBottom());
        } else {
            view.setPadding(padding, view.getPaddingTop(), view.getPaddingRight(),
                    view.getPaddingBottom());
        }
    }

    @BindingAdapter({"android:paddingTop"})
    public static void setPaddingTop(View view, float paddingFloat) {
        final int padding = pixelsToDimensionPixelSize(paddingFloat);
        view.setPadding(view.getPaddingLeft(), padding, view.getPaddingRight(),
                view.getPaddingBottom());
    }

    @BindingAdapter({"android:requiresFadingEdge"})
    public static void setRequiresFadingEdge(View view, int value) {
        final boolean vertical = (value & FADING_EDGE_VERTICAL) != 0;
        final boolean horizontal = (value & FADING_EDGE_HORIZONTAL) != 0;
        view.setVerticalFadingEdgeEnabled(vertical);
        view.setHorizontalFadingEdgeEnabled(horizontal);
    }

    @BindingAdapter({"android:onClickListener", "android:clickable"})
    public static void setClickListener(View view, View.OnClickListener clickListener,
            boolean clickable) {
        view.setOnClickListener(clickListener);
        view.setClickable(clickable);
    }

    @BindingAdapter({"android:onClick", "android:clickable"})
    public static void setOnClick(View view, View.OnClickListener clickListener,
            boolean clickable) {
        view.setOnClickListener(clickListener);
        view.setClickable(clickable);
    }

    @BindingAdapter({"android:onLongClickListener", "android:longClickable"})
    public static void setOnLongClickListener(View view, View.OnLongClickListener clickListener,
            boolean clickable) {
        view.setOnLongClickListener(clickListener);
        view.setLongClickable(clickable);
    }

    @BindingAdapter({"android:onLongClick", "android:longClickable"})
    public static void setOnLongClick(View view, View.OnLongClickListener clickListener,
            boolean clickable) {
        view.setOnLongClickListener(clickListener);
        view.setLongClickable(clickable);
    }

    @BindingAdapter(value = {"android:onViewDetachedFromWindow", "android:onViewAttachedToWindow"},
            requireAll = false)
    public static void setOnAttachStateChangeListener(View view,
            final OnViewDetachedFromWindow detach, final OnViewAttachedToWindow attach) {
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB_MR1) {
            final OnAttachStateChangeListener newListener;
            if (detach == null && attach == null) {
                newListener = null;
            } else {
                newListener = new OnAttachStateChangeListener() {
                    @Override
                    public void onViewAttachedToWindow(View v) {
                        if (attach != null) {
                            attach.onViewAttachedToWindow(v);
                        }
                    }

                    @Override
                    public void onViewDetachedFromWindow(View v) {
                        if (detach != null) {
                            detach.onViewDetachedFromWindow(v);
                        }
                    }
                };
            }
            final OnAttachStateChangeListener oldListener = ListenerUtil.trackListener(view,
                    newListener, R.id.onAttachStateChangeListener);
            if (oldListener != null) {
                view.removeOnAttachStateChangeListener(oldListener);
            }
            if (newListener != null) {
                view.addOnAttachStateChangeListener(newListener);
            }
        }
    }

    @BindingAdapter("android:onLayoutChange")
    public static void setOnLayoutChangeListener(View view, View.OnLayoutChangeListener oldValue,
            View.OnLayoutChangeListener newValue) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            if (oldValue != null) {
                view.removeOnLayoutChangeListener(oldValue);
            }
            if (newValue != null) {
                view.addOnLayoutChangeListener(newValue);
            }
        }
    }

    @SuppressWarnings("deprecation")
    @BindingAdapter("android:background")
    public static void setBackground(View view, Drawable drawable) {
        if (VERSION.SDK_INT >= VERSION_CODES.JELLY_BEAN) {
            view.setBackground(drawable);
        } else {
            view.setBackgroundDrawable(drawable);
        }
    }

    // Follows the same conversion mechanism as in TypedValue.complexToDimensionPixelSize as used
    // when setting padding. It rounds off the float value unless the value is < 1.
    // When a value is between 0 and 1, it is set to 1. A value less than 0 is set to -1.
    private static int pixelsToDimensionPixelSize(float pixels) {
        final int result = (int) (pixels + 0.5f);
        if (result != 0) {
            return result;
        } else if (pixels == 0) {
            return 0;
        } else if (pixels > 0) {
            return 1;
        } else {
            return -1;
        }
    }

    @TargetApi(VERSION_CODES.HONEYCOMB_MR1)
    public interface OnViewDetachedFromWindow {
        void onViewDetachedFromWindow(View v);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB_MR1)
    public interface OnViewAttachedToWindow {
        void onViewAttachedToWindow(View v);
    }
}
