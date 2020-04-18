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

import android.databinding.BindingAdapter;
import android.databinding.InverseBindingListener;
import android.databinding.InverseBindingMethod;
import android.databinding.InverseBindingMethods;
import android.widget.RatingBar;
import android.widget.RatingBar.OnRatingBarChangeListener;

@InverseBindingMethods({
        @InverseBindingMethod(type = RatingBar.class, attribute = "android:rating"),
})
public class RatingBarBindingAdapter {
    @BindingAdapter("android:rating")
    public static void setRating(RatingBar view, float rating) {
        if (view.getRating() != rating) {
            view.setRating(rating);
        }
    }

    @BindingAdapter(value = {"android:onRatingChanged", "android:ratingAttrChanged"},
            requireAll = false)
    public static void setListeners(RatingBar view, final OnRatingBarChangeListener listener,
            final InverseBindingListener ratingChange) {
        if (ratingChange == null) {
            view.setOnRatingBarChangeListener(listener);
        } else {
            view.setOnRatingBarChangeListener(new OnRatingBarChangeListener() {
                @Override
                public void onRatingChanged(RatingBar ratingBar, float rating, boolean fromUser) {
                    if (listener != null) {
                        listener.onRatingChanged(ratingBar, rating, fromUser);
                    }
                    ratingChange.onChange();
                }
            });
        }
    }
}
