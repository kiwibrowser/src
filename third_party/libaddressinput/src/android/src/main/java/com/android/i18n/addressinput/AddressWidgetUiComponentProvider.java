/*
 * Copyright (C) 2014 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.i18n.addressinput;

import com.google.i18n.addressinput.common.AddressField.WidthType;

import android.app.ProgressDialog;
import android.content.Context;
import android.view.LayoutInflater;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

/**
 * Base class for customizing widgets for address input.
 * <p>
 * Clients can optionally override this class and pass it to {@link AddressWidget} the constructor.
 * This will be invoked by the widget to create UI components that provide consistent look-and-feel
 * with other UI components clients might use alongside the address widget.
 */
public class AddressWidgetUiComponentProvider {
  protected Context context;
  protected LayoutInflater inflater;

  public AddressWidgetUiComponentProvider(Context context) {
    this.context = context;
    this.inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
  }

  /**
   * Creates a label, e.g. "State", for an address input field.
   *
   * @param label the label of the address input field
   * @param widthType {@link WidthType} of the field
   * @return a custom {@link TextView} created for the field
   */
  protected TextView createUiLabel(CharSequence label, WidthType widthType) {
    TextView textView = (TextView) inflater.inflate(R.layout.address_textview, null, false);
    textView.setText(label);
    return textView;
  }

  /**
   * Creates a text input view for an address input field.
   *
   * @param widthType {@link WidthType} of the field
   * @return a custom {@link EditText} created for the field
   */
  protected EditText createUiTextField(WidthType widthType) {
    return (EditText) inflater.inflate(R.layout.address_edittext, null, false);
  }

  /**
   * Creates a {@link Spinner} for a input field that uses UI picker.
   *
   * @param widthType {@link WidthType} of the field
   * @return a custom {@link Spinner} created for the field
   */
  protected Spinner createUiPickerSpinner(WidthType widthType) {
    return (Spinner) inflater.inflate(R.layout.address_spinner, null, false);
  }

  /**
   * Creates an {@link AutoCompleteTextView} for an input field that uses autocomplete.
   *
   * @param widthType {@link WidthType} of the field
   * @return a custom {@link AutoCompleteTextView} created for the field
   */
  protected AutoCompleteTextView createUiAutoCompleteTextField(WidthType widthType) {
    return (AutoCompleteTextView)
        inflater.inflate(R.layout.address_autocomplete_textview, null, false);
  }

  /**
   * Creates an {@link ArrayAdapter} to work with the custom {@link Spinner} of a input field that
   * uses UI picker.
   *
   * @param widthType {@link WidthType} of the field
   * @return a custom {@link ArrayAdapter} for the field
   */
  protected ArrayAdapter<String> createUiPickerAdapter(WidthType widthType) {
    ArrayAdapter<String> adapter =
        new ArrayAdapter<String>(context, android.R.layout.simple_spinner_item);
    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    return adapter;
  }

  /** Gets an activity indicator to show that a task is in progress. */
  protected ProgressDialog getUiActivityIndicatorView() {
    return new ProgressDialog(context);
  }
}
