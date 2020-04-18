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
 * InverseBindingMethod is used to identify how to listen for changes to a View property and which
 * getter method to call. InverseBindingMethod should be associated with any class as part of
 * {@link InverseBindingMethods}.
 * <p>
 * <pre>
 * &commat;InverseBindingMethods({&commat;InverseBindingMethod(
 *     type = android.widget.TextView.class,
 *     attribute = "android:text",
 *     event = "android:textAttrChanged",
 *     method = "getText")})
 * public class MyTextViewBindingAdapters { ... }
 * </pre>
 * <p>
 * <code>method</code> is optional. If it isn't provided, the attribute name is used to
 * find the method name, either prefixing with "is" or "get". For the attribute
 * <code>android:text</code>, data binding will search for a
 * <code>public CharSequence getText()</code> method on {@link android.widget.TextView}.
 * <p>
 * <code>event</code> is optional. If it isn't provided, the event name is assigned the
 * attribute name suffixed with <code>AttrChanged</code>. For the <code>android:text</code>
 * attribute, the default event name would be <code>android:textAttrChanged</code>. The event
 * should be set using a {@link BindingAdapter}. For example:
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
 *
 * @see InverseBindingAdapter
 * @see InverseBindingListener
 */
@Target(ElementType.ANNOTATION_TYPE)
public @interface InverseBindingMethod {

    /**
     * The View type that is associated with the attribute.
     */
    Class type();

    /**
     * The attribute that supports two-way binding.
     */
    String attribute();

    /**
     * The event used to notify the data binding system that the attribute value has changed.
     * Defaults to attribute() + "AttrChanged"
     */
    String event() default "";

    /**
     * The getter method to retrieve the attribute value from the View. The default is
     * the bean method name based on the attribute name.
     */
    String method() default "";
}
