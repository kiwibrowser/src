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

import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

/**
 * InverseBindingAdapter is associated with a method used to retrieve the value for a View
 * when setting values gathered from the View. This is similar to {@link BindingAdapter}s:
 * <pre>
 * &commat;InverseBindingAdapter(attribute = "android:text", event = "android:textAttrChanged")
 * public static String captureTextValue(TextView view, CharSequence originalValue) {
 *     CharSequence newValue = view.getText();
 *     CharSequence oldValue = value.get();
 *     if (oldValue == null) {
 *         value.set(newValue);
 *     } else if (!contentEquals(newValue, oldValue)) {
 *         value.set(newValue);
 *     }
 * }
 * </pre>
 * <p>
 * The default value for event is the attribute name suffixed with "AttrChanged". In the
 * above example, the default value would have been <code>android:textAttrChanged</code> even
 * if it wasn't provided.
 * <p>
 * The event attribute is used to notify the data binding system that the value has changed.
 * The developer will typically create a {@link BindingAdapter} to assign the event. For example:
 * <p>
 * <pre>
 * &commat;BindingAdapter(value = {"android:beforeTextChanged", "android:onTextChanged",
 *                          "android:afterTextChanged", "android:textAttrChanged"},
 *                          requireAll = false)
 * public static void setTextWatcher(TextView view, final BeforeTextChanged before,
 *                                   final OnTextChanged on, final AfterTextChanged after,
 *                                   final InverseBindingListener textAttrChanged) {
 *     TextWatcher newValue = new TextWatcher() {
 *         ...
 *         &commat;Override
 *         public void onTextChanged(CharSequence s, int start, int before, int count) {
 *             if (on != null) {
 *                 on.onTextChanged(s, start, before, count);
 *             }
 *             if (textAttrChanged != null) {
 *                 textAttrChanged.onChange();
 *             }
 *         }
 *     }
 *     TextWatcher oldValue = ListenerUtil.trackListener(view, newValue, R.id.textWatcher);
 *     if (oldValue != null) {
 *         view.removeTextChangedListener(oldValue);
 *     }
 *     view.addTextChangedListener(newValue);
 * }
 * </pre>
 * <p>
 * Like <code>BindingAdapter</code>s, InverseBindingAdapter methods may also take
 * {@link DataBindingComponent} as the first parameter and may be an instance method with the
 * instance retrieved from the <code>DataBindingComponent</code>.
 *
 * @see DataBindingUtil#setDefaultComponent(DataBindingComponent)
 * @see InverseBindingMethod
 */
@Target({ElementType.METHOD, ElementType.ANNOTATION_TYPE})
public @interface InverseBindingAdapter {

    /**
     * The attribute that the value is to be retrieved for.
     */
    String attribute();

    /**
     * The event used to trigger changes. This is used in {@link BindingAdapter}s for the
     * data binding system to set the event listener when two-way binding is used.
     */
    String event() default "";
}
