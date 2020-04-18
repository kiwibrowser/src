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
 * A listener implemented by all two-way bindings to be notified when a triggering change happens.
 * For example, when there is a two-way binding for android:text, an implementation of
 * <code>InverseBindingListener</code> will be generated in the layout's binding class.
 * <pre>
 * private static class InverseListenerTextView implements InverseBindingListener {
 *     &commat;Override
 *     public void onChange() {
 *         mObj.setTextValue(mTextView.getText());
 *     }
 * }
 * </pre>
 * <p>
 * A {@link BindingAdapter} should be used to assign the event listener.
 * For example, <code>android:onTextChanged</code> will need to trigger the event listener
 * for the <code>android:text</code> attribute.
 * <pre>
 * &commat;InverseBindingAdapter(attribute = "android:text", event = "android:textAttrChanged")
 * public static void captureTextValue(TextView view, ObservableField&lt;CharSequence> value) {
 *     CharSequence newValue = view.getText();
 *     CharSequence oldValue = value.get();
 *     if (oldValue == null) {
 *         value.set(newValue);
 *     } else if (!contentEquals(newValue, oldValue)) {
 *         value.set(newValue);
 *     }
 * }
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
 */
public interface InverseBindingListener {
    /**
     * Notifies the data binding system that the attribute value has changed.
     */
    void onChange();
}
